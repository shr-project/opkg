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


	err = opkg_conf_init (&opkg_conf, &args);
	args_deinit (&args);
	if (err)
	{
		print_error_list();
		free_error_list();
		return err;
	}

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

#ifdef HAVE_CURL
	opkg_curl_cleanup();
#endif
	opkg_conf_deinit (&opkg_conf);

	return err;
}
