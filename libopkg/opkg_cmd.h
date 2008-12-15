/* opkg_cmd.h - the opkg package management system

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

#ifndef OPKG_CMD_H
#define OPKG_CMD_H

typedef int (*opkg_cmd_fun_t)(opkg_conf_t *conf, int argc, const char **argv);

struct opkg_cmd
{
    char *name;
    int requires_args;
    opkg_cmd_fun_t fun;
};
typedef struct opkg_cmd opkg_cmd_t;

opkg_cmd_t *opkg_cmd_find(const char *name);
int opkg_cmd_exec(opkg_cmd_t *cmd, opkg_conf_t *conf, int argc, 
                  const char **argv, void *userdata);
int opkg_multiple_files_scan (opkg_conf_t *conf, int argc, char *argv[]);
/* install any packges with state_want == SW_INSTALL */
int opkg_install_wanted_packages(opkg_conf_t *conf);
/* ensure that all dependences are satisfied */
int opkg_configure_packages(opkg_conf_t *conf, char *pkg_name);

int pkg_mark_provides(pkg_t *pkg);

#endif
