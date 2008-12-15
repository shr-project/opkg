/* opkglib.h - the itsy package management system

   Florian Boor <florian.boor@kernelconcepts.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/

#ifndef OPKGLIB_H
#define OPKGLIB_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#ifdef OPKG_LIB

#include "opkg_conf.h"
#include "opkg_message.h"

#include "args.h"
#include "pkg.h"

typedef int (*opkg_message_callback)(opkg_conf_t *conf, message_level_t level, 
	char *msg);
typedef int (*opkg_list_callback)(char *name, char *desc, char *version, 
	pkg_state_status_t status, void *userdata);
typedef int (*opkg_status_callback)(char *name, int istatus, char *desc,
	void *userdata);
typedef char* (*opkg_response_callback)(char *question);
typedef void (*opkg_download_progress_callback)(int percent, char *url);

extern int opkg_op(int argc, char *argv[]); /* opkglib.c */
extern int opkg_init (opkg_message_callback mcall, 
                      opkg_response_callback rcall,
					  args_t * args);

extern int opkg_deinit (args_t *args);
extern int opkg_packages_list(args_t *args, 
                              const char *packages, 
                              opkg_list_callback cblist,
                              void *userdata);
extern int opkg_packages_status(args_t *args, 
                                const char *packages, 
                                opkg_status_callback cbstatus,
								void *userdata);
extern int opkg_packages_info(args_t *args,
                              const char *packages,
                              opkg_status_callback cbstatus,
                              void *userdata);
extern int opkg_packages_install(args_t *args, const char *name);
extern int opkg_packages_remove(args_t *args, const char *name, int purge);
extern int opkg_lists_update(args_t *args);
extern int opkg_packages_upgrade(args_t *args);
extern int opkg_packages_download(args_t *args, const char *name);
extern int opkg_package_files(args_t *args,
                              const char *name,
							  opkg_list_callback cblist,
							  void *userdata);
extern int opkg_file_search(args_t *args,
                            const char *file,
							opkg_list_callback cblist,
							void *userdata);
extern int opkg_package_whatdepends(args_t *args, const char *file);
extern int opkg_package_whatrecommends(args_t *args, const char *file);
extern int opkg_package_whatprovides(args_t *args, const char *file);
extern int opkg_package_whatconflicts(args_t *args, const char *file);
extern int opkg_package_whatreplaces(args_t *args, const char *file);

extern opkg_message_callback opkg_cb_message; /* opkglib.c */
extern opkg_response_callback opkg_cb_response;
extern opkg_status_callback opkg_cb_status;
extern opkg_list_callback opkg_cb_list;
extern opkg_download_progress_callback opkg_cb_download_progress; /* ipkg_download.c */

extern void push_error_list(struct errlist **errors,char * msg);
extern void reverse_error_list(struct errlist **errors);
extern void free_error_list();

#endif


#endif
