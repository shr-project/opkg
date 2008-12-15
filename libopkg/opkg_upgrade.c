/* opkg_upgrade.c - the itsy package management system

   Carl D. Worth
   Copyright (C) 2001 University of Southern California

   Copyright (C) 2003 Daniele Nicolodi <daniele@grinta.net>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/

#include "opkg.h"
#include "opkg_install.h"
#include "opkg_message.h"

int opkg_upgrade_pkg(opkg_conf_t *conf, pkg_t *old)
{
     pkg_t *new;
     int cmp;
     char *old_version, *new_version;

     if (old->state_flag & SF_HOLD) {
          opkg_message(conf, OPKG_NOTICE,
                       "Not upgrading package %s which is marked "
                       "hold (flags=%#x)\n", old->name, old->state_flag);
          return 0;
     }

     new = pkg_hash_fetch_best_installation_candidate_by_name(conf, old->name);
     if (new == NULL) {
          old_version = pkg_version_str_alloc(old);
          opkg_message(conf, OPKG_NOTICE,
                       "Assuming locally installed package %s (%s) "
                       "is up to date.\n", old->name, old_version);
          free(old_version);
          return 0;
     }
          
     old_version = pkg_version_str_alloc(old);
     new_version = pkg_version_str_alloc(new);
               
     cmp = pkg_compare_versions(old, new);
     opkg_message(conf, OPKG_DEBUG,
                  "comparing visible versions of pkg %s:"
                  "\n\t%s is installed "
                  "\n\t%s is available "
                  "\n\t%d was comparison result\n",
                  old->name, old_version, new_version, cmp);
     if (cmp == 0) {
          opkg_message(conf, OPKG_INFO,
                       "Package %s (%s) installed in %s is up to date.\n",
                       old->name, old_version, old->dest->name);
          free(old_version);
          free(new_version);
          return 0;
     } else if (cmp > 0) {
          opkg_message(conf, OPKG_NOTICE,
                       "Not downgrading package %s on %s from %s to %s.\n",
                       old->name, old->dest->name, old_version, new_version);
          free(old_version);
          free(new_version);
          return 0;
     } else if (cmp < 0) {
          new->dest = old->dest;
          old->state_want = SW_DEINSTALL;
     }

     new->state_flag |= SF_USER;
     return opkg_install_pkg(conf, new,1);
}
