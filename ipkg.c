/* ipkg.c - the itsy package management system

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

#include "ipkg.h"

#include "args.h"
#include "ipkg_conf.h"
#include "ipkg_cmd.h"

int main(int argc, const char *argv[])
{
    int err, optind;
    args_t args;
    char *cmd_name;
    ipkg_cmd_t *cmd;
    ipkg_conf_t ipkg_conf;

    error_list=NULL;

    args_init(&args);
	
    optind = args_parse(&args, argc, argv);
    if (optind == argc || optind < 0) {
	args_usage("ipkg must have one sub-command argument");
    }

    cmd_name = argv[optind++];

    err = ipkg_conf_init(&ipkg_conf, &args);
    if (err) {
	return err;
    }

    args_deinit(&args);

    cmd = ipkg_cmd_find(cmd_name);
    if (cmd == NULL) {
	fprintf(stderr, "%s: unknown sub-command %s\n", argv[0], cmd_name);
	args_usage(NULL);
    }

    if (cmd->requires_args && optind == argc) {
	fprintf(stderr, "%s: the ``%s'' command requires at least one argument\n",
		__FUNCTION__, cmd_name);
	args_usage(NULL);
    }

    err = ipkg_cmd_exec(cmd, &ipkg_conf, argc - optind, argv + optind);

    if ( err == 0 ) {
       ipkg_message(ipkg_conf, IPKG_NOTICE, "Succesfully done.\n");
    } else {
       ipkg_message(ipkg_conf, IPKG_NOTICE, "Error returned. Return value is %d\n.",err);

}

    }
    /* XXX: FEATURE request: run ldconfig and/or depmod after package needing them are installed or removed */  
    // ipkg_global_postinst();

    ipkg_conf_deinit(&ipkg_conf);

    return err;
}



