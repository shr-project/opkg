/* opkg.c - the itsy package management system

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

#include "opkg.h"

#include "args.h"
#include "opkg_conf.h"
#include "opkg_cmd.h"

int main(int argc, const char *argv[])
{
    int err, optind;
    args_t args;
    char *cmd_name;
    opkg_cmd_t *cmd;
    opkg_conf_t opkg_conf;

    error_list=NULL;

    args_init(&args);
	
    optind = args_parse(&args, argc, argv);
    if (optind == argc || optind < 0) {
	args_usage("opkg must have one sub-command argument");
    }

    cmd_name = argv[optind++];

    err = opkg_conf_init(&opkg_conf, &args);
    if (err) {
	return err;
    }

    args_deinit(&args);

    cmd = opkg_cmd_find(cmd_name);
    if (cmd == NULL) {
	fprintf(stderr, "%s: unknown sub-command %s\n", argv[0], cmd_name);
	args_usage(NULL);
    }

    if (cmd->requires_args && optind == argc) {
	fprintf(stderr, "%s: the ``%s'' command requires at least one argument\n",
		__FUNCTION__, cmd_name);
	args_usage(NULL);
    }

    err = opkg_cmd_exec(cmd, &opkg_conf, argc - optind, argv + optind);

    if ( err == 0 ) {
       opkg_message(opkg_conf, OPKG_NOTICE, "Succesfully done.\n");
    } else {
       opkg_message(opkg_conf, OPKG_NOTICE, "Error returned. Return value is %d\n.",err);

}

    }
    /* XXX: FEATURE request: run ldconfig and/or depmod after package needing them are installed or removed */  
    // opkg_global_postinst();

    opkg_conf_deinit(&opkg_conf);

    return err;
}



