/* opkg_configure.c - the itsy package management system

   Carl D. Worth

   Copyright (C) 2001 University of Southern California

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/

#include "includes.h"
#include "sprintf_alloc.h"
#include "opkg_configure.h"
#include "opkg_state.h"

int opkg_configure(opkg_conf_t *conf, pkg_t *pkg)
{
    int err;

    /* DPKG_INCOMPATIBILITY:
       dpkg actually does some conffile handling here, rather than at the
       end of opkg_install(). Do we care? */
    /* DPKG_INCOMPATIBILITY:
       dpkg actually includes a version number to this script call */

    char *pkgid;
    sprintf_alloc (&pkgid, "%s;%s;%s;", pkg->name, pkg->version, pkg->architecture);
    opkg_set_current_state (conf, OPKG_STATE_CONFIGURING_PKG, pkgid);
    free (pkgid);

    err = pkg_run_script(conf, pkg, "postinst", "configure");
    if (err) {
	opkg_message(conf, OPKG_ERROR, "ERROR: %s.postinst returned %d\n", pkg->name, err);
	return err;
    }

    opkg_state_changed++;
    opkg_set_current_state (conf, OPKG_STATE_NONE, NULL);
    return 0;
}

