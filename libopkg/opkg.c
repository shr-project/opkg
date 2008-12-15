/* opkg.c - the opkg  package management system

   Thomas Wood <thomas@openedhand.com>

   Copyright (C) 2008 OpenMoko Inc

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
 */

#include <config.h>
#include <fnmatch.h>

#include "opkg.h"
#include "opkg_conf.h"
#include "args.h"

#include "opkg_install.h"
#include "opkg_configure.h"

struct _opkg_t
{
  args_t *args;
  opkg_conf_t *conf;
};

/** Private Functions ***/


static int
opkg_configure_packages(opkg_conf_t *conf, char *pkg_name)
{
  pkg_vec_t *all;
  int i;
  pkg_t *pkg;
  int r, err = 0;

  all = pkg_vec_alloc();
  pkg_hash_fetch_available(&conf->pkg_hash, all);

  for (i = 0; i < all->len; i++)
  {
    pkg = all->pkgs[i];

    if (pkg_name && fnmatch(pkg_name, pkg->name, 0))
      continue;

    if (pkg->state_status == SS_UNPACKED)
    {
      r = opkg_configure(conf, pkg);
      if (r == 0)
      {
        pkg->state_status = SS_INSTALLED;
        pkg->parent->state_status = SS_INSTALLED;
        pkg->state_flag &= ~SF_PREFER;
      }
      else
      {
        if (!err)
          err = r;
      }
    }
  }

  pkg_vec_free(all);
  return err;
}



/*** Public API ***/

opkg_t *
opkg_new ()
{
  opkg_t *opkg;
  opkg = malloc (sizeof (opkg_t));
  args_init (opkg->args);
  opkg_conf_init (opkg->conf, opkg->args);
  return opkg;
}

void
opkg_free (opkg_t *opkg)
{
  opkg_conf_deinit (opkg->conf);
  args_deinit (opkg->args);
}

void
opkg_get_option (opkg_t *opkg, char *option, void **value)
{
}

void
opkg_set_option (opkg_t *opkg, char *option, void *value)
{
}

int
opkg_install_package (opkg_t *opkg, char *package_name)
{
  int err;

  pkg_info_preinstall_check(opkg->conf);

  if (opkg->conf->multiple_providers)
  {
    err = opkg_install_multi_by_name (opkg->conf, package_name);
  }
  else
  {
    err = opkg_install_by_name (opkg->conf, package_name);
  }

  err = opkg_configure_packages (opkg->conf, NULL);

  if (opkg->conf->noaction)
    return err;

  opkg_conf_write_status_files (opkg->conf);
  pkg_write_changed_filelists (opkg->conf);

  return err;
}

int
opkg_remove_package (opkg_t *opkg, char *package_name)
{
  return 1;
}

int
opkg_upgrade_package (opkg_t *opkg, char *package_name)
{
  return 1;
}

int
opkg_upgrade_all (opkg_t *opkg)
{
  return 1;
}

int
opkg_update_package_lists (opkg_t *opkg)
{
  return 1;
}
