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
typedef struct _opkg_package_t opkg_package_t;

typedef void (*opkg_progress_callback_t) (opkg_t *opkg, int percentage, void *user_data);
typedef void (*opkg_package_callback_t) (opkg_t *opkg, opkg_package_t *package, void *user_data);


struct _opkg_package_t
{
  char *name;
  char *version;
  char *architecture;
  char *repository;
  char *description;
  char *tags;
  int installed;
};

opkg_package_t* opkg_package_new ();
opkg_package_t* opkg_package_new_with_values (const char *name, const char *version, const char *arch, const char *desc, const char *tags, int installed);
void opkg_package_free (opkg_package_t *package);

opkg_t* opkg_new ();
void opkg_free (opkg_t *opkg);
void opkg_get_option (opkg_t *opkg, char *option, void **value);
void opkg_set_option (opkg_t *opkg, char *option, void *value);
int opkg_read_config_files (opkg_t *opkg);

int opkg_install_package (opkg_t *opkg, const char *package_name, opkg_progress_callback_t callback, void *user_data);
int opkg_remove_package (opkg_t *opkg, const char *package_name, opkg_progress_callback_t callback, void *user_data);
int opkg_upgrade_package (opkg_t *opkg, const char *package_name, opkg_progress_callback_t callback, void *user_data);
int opkg_upgrade_all (opkg_t *opkg, opkg_progress_callback_t callback, void *user_data);
int opkg_update_package_lists (opkg_t *opkg, opkg_progress_callback_t callback, void *user_data);

int opkg_list_packages (opkg_t *opkg, opkg_package_callback_t callback, void *user_data);
