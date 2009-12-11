/* opkglib.c - the opkg package management system

   Florian Boor

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

/* This is used for backward compatibility */
int
opkg_op (int argc, char *argv[])
{
	int err, opts;
	args_t args;
	char *cmd_name;
	opkg_cmd_t *cmd;
	int nocheckfordirorfile = 0;
        int noreadfeedsfile = 0;

	args_init (&args);

	opts = args_parse (&args, argc, argv);
	if (opts == argc || opts < 0)
	{
		args_usage ("opkg must have one sub-command argument");
	}

	cmd_name = argv[opts++];

        if ( !strcmp(cmd_name,"print-architecture") ||
             !strcmp(cmd_name,"print_architecture") ||
             !strcmp(cmd_name,"print-installation-architecture") ||
             !strcmp(cmd_name,"print_installation_architecture") )
           nocheckfordirorfile = 1;

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
           noreadfeedsfile = 1;

	cmd = opkg_cmd_find (cmd_name);
	if (cmd == NULL)
	{
		fprintf (stderr, "%s: unknown sub-command %s\n", argv[0],
			 cmd_name);
		args_usage (NULL);
	}

	conf->pfm = cmd->pfm;

	err = opkg_conf_init (&args);
	args_deinit (&args);
	if (err)
		goto err0;

	if (!nocheckfordirorfile) {
		if (!noreadfeedsfile) {
			if ((err = pkg_hash_load_feeds()))
				goto err1;
		}
   
		if ((err = pkg_hash_load_status_files()))
			goto err1;
	}

	if (cmd->requires_args && opts == argc)
	{
		fprintf (stderr,
			 "%s: the ``%s'' command requires at least one argument\n",
			 argv[0], cmd_name);
		args_usage (NULL);
	}

	err = opkg_cmd_exec (cmd, argc - opts, (const char **) (argv + opts));


#ifdef HAVE_CURL
	opkg_curl_cleanup();
#endif
err1:
	opkg_conf_deinit ();

err0:
	print_error_list();
	free_error_list();

	return err;
}
