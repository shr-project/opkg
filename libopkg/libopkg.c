/* opkglib.c - the opkg package management system

   Florina Boor

   Copyright (C) 2003 kernel concepts

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
#include "libopkg.h"

#include "args.h"
#include "opkg_conf.h"
#include "opkg_cmd.h"
#include "file_util.h"

#include "opkg_message.h"
#include "opkg_error.h"

opkg_status_callback opkg_cb_status = NULL;
opkg_list_callback opkg_cb_list = NULL;

int default_opkg_message_callback(opkg_conf_t *conf, message_level_t level, 
				  char *msg)
{
     if (conf && (conf->verbosity < level)) {
	  return 0;
     } else {
          if ( level == OPKG_ERROR ){
             push_error_list(&error_list, msg); 
          } else
	     printf(msg);
     }
     return 0;
}

int default_opkg_list_callback(char *name, char *desc, char *version, 
			       pkg_state_status_t status, void *userdata)
{
     if (desc)
	  printf("%s - %s - %s\n", name, version, desc);
     else
	  printf("%s - %s\n", name, version);
     return 0;
}

int default_opkg_files_callback(char *name, char *desc, char *version,
                   pkg_state_status_t status, void *userdata)
{
     if (desc)
	  printf("%s\n", desc);
     return 0;
}

int default_opkg_status_callback(char *name, int istatus, char *desc,
				 void *userdata)
{
     printf("%s\n", desc);
     return 0;
}

char* default_opkg_response_callback(char *question)
{
     char *response = NULL;
     printf(question);
     fflush(stdout);
     do {
	  response = (char *)file_read_line_alloc(stdin);
     } while (response == NULL);
     return response;
}

/* This is used for backward compatibility */
int
opkg_op (int argc, char *argv[])
{
	int err, optind;
	args_t args;
	char *cmd_name;
	opkg_cmd_t *cmd;
	opkg_conf_t opkg_conf;

	args_init (&args);

	optind = args_parse (&args, argc, argv);
	if (optind == argc || optind < 0)
	{
		args_usage ("opkg must have one sub-command argument");
	}

	cmd_name = argv[optind++];
/* Pigi: added a flag to disable the checking of structures if the command does not need to 
         read anything from there.
*/
        if ( !strcmp(cmd_name,"print-architecture") ||
             !strcmp(cmd_name,"print_architecture") ||
             !strcmp(cmd_name,"print-installation-architecture") ||
             !strcmp(cmd_name,"print_installation_architecture") )
           args.nocheckfordirorfile = 1;

/* Pigi: added a flag to disable the reading of feed files  if the command does not need to 
         read anything from there.
*/
        if ( !strcmp(cmd_name,"flag") ||
             !strcmp(cmd_name,"configure") ||
             !strcmp(cmd_name,"remove") ||
             !strcmp(cmd_name,"files") ||
             !strcmp(cmd_name,"search") ||
             !strcmp(cmd_name,"compare_versions") ||
             !strcmp(cmd_name,"compare-versions") ||
             !strcmp(cmd_name,"list_installed") ||
             !strcmp(cmd_name,"list-installed") ||
             !strcmp(cmd_name,"status") )
           args.noreadfeedsfile = 1;

	opkg_cb_message = default_opkg_message_callback;
	opkg_cb_response = default_opkg_response_callback;
	opkg_cb_status = default_opkg_status_callback;


	err = opkg_conf_init (&opkg_conf, &args);
	if (err)
	{
		opkg_print_error_list (&opkg_conf);
		return err;
	}

	args_deinit (&args);

 	if ( strcmp(cmd_name, "files")==0)
	     opkg_cb_list = default_opkg_files_callback;
 	else
	     opkg_cb_list = default_opkg_list_callback;

	cmd = opkg_cmd_find (cmd_name);
	if (cmd == NULL)
	{
		fprintf (stderr, "%s: unknown sub-command %s\n", argv[0],
			 cmd_name);
		args_usage (NULL);
	}

	if (cmd->requires_args && optind == argc)
	{
		fprintf (stderr,
			 "%s: the ``%s'' command requires at least one argument\n",
			 __FUNCTION__, cmd_name);
		args_usage (NULL);
	}

	err = opkg_cmd_exec (cmd, &opkg_conf, argc - optind, (const char **) (argv + optind), NULL);

	opkg_conf_deinit (&opkg_conf);

	return err;
}
