/* opkglib.c - the itsy package management system

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

#include "opkg.h"
#include "includes.h"
#include "libopkg.h"

#include "args.h"
#include "opkg_conf.h"
#include "opkg_cmd.h"
#include "file_util.h"



opkg_status_callback opkg_cb_status = NULL;
opkg_list_callback opkg_cb_list = NULL;


int
opkg_init (opkg_message_callback mcall, 
           opkg_response_callback rcall,
           args_t * args)
{
	opkg_cb_message = mcall;
	opkg_cb_response = rcall;

	args_init (args);

	return 0;
}


int
opkg_deinit (args_t * args)
{
	args_deinit (args);
	opkg_cb_message = NULL;
	opkg_cb_response = NULL;

	/* place other cleanup stuff here */

	return 0;
}


int
opkg_packages_list(args_t *args, 
                   const char *packages, 
                   opkg_list_callback cblist,
                   void *userdata)
{
	opkg_cmd_t *cmd;
	opkg_conf_t opkg_conf;
	int err;

	err = opkg_conf_init (&opkg_conf, args);
	if (err)
	{
		return err;
	}

	opkg_cb_list = cblist;
	/* we need to do this because of static declarations, 
	 * maybe a good idea to change */
	cmd = opkg_cmd_find ("list");
	if (packages)
		err = opkg_cmd_exec (cmd, &opkg_conf, 1, &packages, userdata);
	else
		err = opkg_cmd_exec (cmd, &opkg_conf, 0, NULL, userdata);
	opkg_cb_list = NULL;
	opkg_conf_deinit (&opkg_conf);
	return (err);
}


int
opkg_packages_status(args_t *args,
                     const char *packages,
                     opkg_status_callback cbstatus,
                     void *userdata)
{
	opkg_cmd_t *cmd;
	opkg_conf_t opkg_conf;
	int err;

	err = opkg_conf_init (&opkg_conf, args);
	if (err)
	{
		return err;
	}

	opkg_cb_status = cbstatus;

	/* we need to do this because of static declarations,
	 * maybe a good idea to change */
	cmd = opkg_cmd_find ("status");
	if (packages)
		err = opkg_cmd_exec (cmd, &opkg_conf, 1, &packages, userdata);
	else
		err = opkg_cmd_exec (cmd, &opkg_conf, 0, NULL, userdata);

	opkg_cb_status = NULL;
	opkg_conf_deinit (&opkg_conf);
	return (err);
}


int
opkg_packages_info(args_t *args,
                   const char *packages,
                   opkg_status_callback cbstatus,
                   void *userdata)
{
	opkg_cmd_t *cmd;
	opkg_conf_t opkg_conf;
	int err;

	err = opkg_conf_init (&opkg_conf, args);
	if (err)
	{
		return err;
	}

	opkg_cb_status = cbstatus;

	/* we need to do this because of static declarations,
	 * maybe a good idea to change */
	cmd = opkg_cmd_find ("info");
	if (packages)
		err = opkg_cmd_exec (cmd, &opkg_conf, 1, &packages, userdata);
	else
		err = opkg_cmd_exec (cmd, &opkg_conf, 0, NULL, userdata);

	opkg_cb_status = NULL;
	opkg_conf_deinit (&opkg_conf);
	return (err);
}


int
opkg_packages_install (args_t * args, const char *name)
{
	opkg_cmd_t *cmd;
	opkg_conf_t opkg_conf;
	int err;

	/* this error should be handled in application */
	if (!name || !strlen (name))
		return (-1);

	err = opkg_conf_init (&opkg_conf, args);
	if (err)
	{
		return err;
	}

	/* we need to do this because of static declarations,
	 * maybe a good idea to change */
	cmd = opkg_cmd_find ("install");
	err = opkg_cmd_exec (cmd, &opkg_conf, 1, &name, NULL);

	opkg_conf_deinit(&opkg_conf);
	return (err);
}


int
opkg_packages_remove(args_t *args, const char *name, int purge)
{
	opkg_cmd_t *cmd;
	opkg_conf_t opkg_conf;
	int err;

	/* this error should be handled in application */
	if (!name || !strlen (name))
		return (-1);

	err = opkg_conf_init (&opkg_conf, args);
	if (err)
	{
		return err;
	}

	/* we need to do this because of static declarations, 
	 * maybe a good idea to change */
	if (purge)
		cmd = opkg_cmd_find ("purge");
	else
		cmd = opkg_cmd_find ("remove");

	err = opkg_cmd_exec (cmd, &opkg_conf, 1, &name, NULL);
	
	opkg_conf_deinit(&opkg_conf);
	return (err);
}


int 
opkg_lists_update(args_t *args)
{
	opkg_cmd_t *cmd;
	opkg_conf_t opkg_conf;
	int err;

	err = opkg_conf_init (&opkg_conf, args);
	if (err)
	{
		return err;
	}

	/* we need to do this because of static declarations, 
	 * maybe a good idea to change */
	cmd = opkg_cmd_find ("update");

	err = opkg_cmd_exec (cmd, &opkg_conf, 0, NULL, NULL);
	
	opkg_conf_deinit(&opkg_conf);
	return (err);
}


int 
opkg_packages_upgrade(args_t *args)
{
	opkg_cmd_t *cmd;
	opkg_conf_t opkg_conf;
	int err;

	err = opkg_conf_init (&opkg_conf, args);
	if (err)
	{
		return err;
	}

	/* we need to do this because of static declarations, 
	 * maybe a good idea to change */
	cmd = opkg_cmd_find ("upgrade");

	err = opkg_cmd_exec (cmd, &opkg_conf, 0, NULL, NULL);
	
	opkg_conf_deinit(&opkg_conf);
	return (err);
}


int
opkg_packages_download (args_t * args, const char *name)
{
	opkg_cmd_t *cmd;
	opkg_conf_t opkg_conf;
	int err;

	/* this error should be handled in application */
	if (!name || !strlen (name))
		return (-1);

	err = opkg_conf_init (&opkg_conf, args);
	if (err)
	{
		return err;
	}

	/* we need to do this because of static declarations,
	 * maybe a good idea to change */
	cmd = opkg_cmd_find ("download");
	err = opkg_cmd_exec (cmd, &opkg_conf, 1, &name, NULL);

	opkg_conf_deinit(&opkg_conf);
	return (err);
}


int
opkg_package_files(args_t *args, 
                   const char *name, 
                   opkg_list_callback cblist,
                   void *userdata)
{
	opkg_cmd_t *cmd;
	opkg_conf_t opkg_conf;
	int err;

	/* this error should be handled in application */
	if (!name || !strlen (name))
		return (-1);

	err = opkg_conf_init (&opkg_conf, args);
	if (err)
	{
		return err;
	}

	opkg_cb_list = cblist;
	
	/* we need to do this because of static declarations, 
	 * maybe a good idea to change */
	cmd = opkg_cmd_find ("files");

	err = opkg_cmd_exec (cmd, &opkg_conf, 1, &name, userdata);
	
	opkg_cb_list = NULL;
	opkg_conf_deinit(&opkg_conf);
	return (err);
}


int 
opkg_file_search(args_t *args, 
                const char *file,
				opkg_list_callback cblist,
                void *userdata)
{
	opkg_cmd_t *cmd;
	opkg_conf_t opkg_conf;
	int err;
	
	/* this error should be handled in application */
	if (!file || !strlen (file))
		return (-1);

	err = opkg_conf_init (&opkg_conf, args);
	if (err)
	{
		return err;
	}

	opkg_cb_list = cblist;

	/* we need to do this because of static declarations, 
	 * maybe a good idea to change */
	cmd = opkg_cmd_find ("search");
	err = opkg_cmd_exec (cmd, &opkg_conf, 1, &file, userdata);
	
	opkg_cb_list = NULL;
	opkg_conf_deinit(&opkg_conf);
	return(err);
}


int 
opkg_file_what(args_t *args, const char *file, const char* command)
{
	opkg_cmd_t *cmd;
	opkg_conf_t opkg_conf;
	int err;
	
	/* this error should be handled in application */
	if (!file || !strlen (file))
		return (-1);

	err = opkg_conf_init (&opkg_conf, args);
	if (err)
	{
		return err;
	}

	/* we need to do this because of static declarations, 
	 * maybe a good idea to change */
	cmd = opkg_cmd_find (command);
	err = opkg_cmd_exec (cmd, &opkg_conf, 1, &file, NULL);
	
	opkg_conf_deinit(&opkg_conf);
	return(err);
}

#define opkg_package_whatdepends(args,file) opkg_file_what(args,file,"whatdepends")
#define opkg_package_whatrecommends(args, file) opkg_file_what(args,file,"whatrecommends")
#define opkg_package_whatprovides(args, file) opkg_file_what(args,file,"whatprovides")
#define opkg_package_whatconflicts(args, file) opkg_file_what(args,file,"whatconflicts")
#define opkg_package_whatreplaces(args, file) opkg_file_what(args,file,"whatreplaces")


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


	err = opkg_conf_init (&opkg_conf, &args);
	if (err)
	{
		return err;
	}

	args_deinit (&args);

	opkg_cb_message = default_opkg_message_callback;
	opkg_cb_response = default_opkg_response_callback;
	opkg_cb_status = default_opkg_status_callback;
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
