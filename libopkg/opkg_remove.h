/* opkg_remove.h - the itsy package management system

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

#ifndef OPKG_REMOVE_H
#define OPKG_REMOVE_H

#include "pkg.h"
#include "opkg_conf.h"

int opkg_remove_pkg(opkg_conf_t *conf, pkg_t *pkg,int message);
int opkg_purge_pkg(opkg_conf_t *conf, pkg_t *pkg);
int possible_broken_removal_of_packages (opkg_conf_t *conf, pkg_t *pkg);
int pkg_has_installed_dependents(opkg_conf_t *conf, abstract_pkg_t *parent_apkg, pkg_t *pkg, abstract_pkg_t *** pdependents);
int remove_data_files_and_list(opkg_conf_t *conf, pkg_t *pkg);
int remove_maintainer_scripts_except_postrm (opkg_conf_t *conf, pkg_t *pkg);
int remove_postrm (opkg_conf_t *conf, pkg_t *pkg);


#endif
