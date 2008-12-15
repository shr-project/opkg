/* opkg.h - the opkg  package management system

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

typedef struct _opkg_t opkg_t;

opkg_t* opkg_new ();
void opkg_free (opkg_t *opkg);
int opkg_install_package (opkg_t *opkg, char *package_name);
int opkg_remove_package (opkg_t *opkg, char *package_name);
int opkg_upgrade_package (opkg_t *opkg, char *package_name);
int opkg_upgrade_all (opkg_t *opkg);
int opkg_update_package_lists (opkg_t *opkg);
