/* args.c - parse command-line args
 
  Carl D. Worth

  Copyright 2001 University of Southern California
 
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 */

#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ipkg.h"

#include "config.h"
#include "args.h"
#include "sprintf_alloc.h"

static void print_version(void);

enum long_args_opt
{
     ARGS_OPT_FORCE_DEFAULTS = 129,
     ARGS_OPT_FORCE_DEPENDS,
     ARGS_OPT_FORCE_OVERWRITE,
     ARGS_OPT_FORCE_DOWNGRADE,
     ARGS_OPT_FORCE_REINSTALL,
     ARGS_OPT_FORCE_REMOVAL_OF_DEPENDENT_PACKAGES,
     ARGS_OPT_FORCE_REMOVAL_OF_ESSENTIAL_PACKAGES,
     ARGS_OPT_FORCE_SPACE,
     ARGS_OPT_NOACTION,
     ARGS_OPT_NODEPS,
     ARGS_OPT_VERBOSE_WGET,
     ARGS_OPT_VERBOSITY,
     ARGS_OPT_MULTIPLE_PROVIDERS
};

int args_init(args_t *args)
{
     char *conf_file_dir;

     memset(args, 0, sizeof(args_t));

     args->dest = ARGS_DEFAULT_DEST;

     conf_file_dir = getenv("IPKG_CONF_DIR");
     if (conf_file_dir == NULL || conf_file_dir[0] == '\0') {
	  conf_file_dir = ARGS_DEFAULT_CONF_FILE_DIR;
     }
     sprintf_alloc(&args->conf_file, "%s/%s", conf_file_dir,
		   ARGS_DEFAULT_CONF_FILE_NAME);

     args->force_defaults = ARGS_DEFAULT_FORCE_DEFAULTS;
     args->force_depends = ARGS_DEFAULT_FORCE_DEPENDS;
     args->force_overwrite = ARGS_DEFAULT_FORCE_OVERWRITE;
     args->force_downgrade = ARGS_DEFAULT_FORCE_DOWNGRADE;
     args->force_reinstall = ARGS_DEFAULT_FORCE_REINSTALL;
     args->force_removal_of_dependent_packages = ARGS_DEFAULT_FORCE_REMOVAL_OF_DEPENDENT_PACKAGES;
     args->force_removal_of_essential_packages = ARGS_DEFAULT_FORCE_REMOVAL_OF_ESSENTIAL_PACKAGES;
     args->noaction = ARGS_DEFAULT_NOACTION;
     args->nodeps = ARGS_DEFAULT_NODEPS;
     args->verbose_wget = ARGS_DEFAULT_VERBOSE_WGET;
     args->verbosity = ARGS_DEFAULT_VERBOSITY;
     args->offline_root = ARGS_DEFAULT_OFFLINE_ROOT;
     args->offline_root_pre_script_cmd = ARGS_DEFAULT_OFFLINE_ROOT_PRE_SCRIPT_CMD;
     args->offline_root_post_script_cmd = ARGS_DEFAULT_OFFLINE_ROOT_POST_SCRIPT_CMD;
     args->multiple_providers = 0;
     args->nocheckfordirorfile = 0;
     args->noreadfeedsfile = 0;

     return 1;
}

void args_deinit(args_t *args)
{
     free(args->conf_file);
     args->conf_file = NULL;
}

int args_parse(args_t *args, int argc, char *argv[])
{
     int c;
     int option_index = 0;
     int parse_err = 0;
     static struct option long_options[] = {
	  {"query-all", 0, 0, 'A'},
	  {"conf-file", 1, 0, 'f'},
	  {"conf", 1, 0, 'f'},
	  {"dest", 1, 0, 'd'},
	  {"force-defaults", 0, 0, ARGS_OPT_FORCE_DEFAULTS},
	  {"force_defaults", 0, 0, ARGS_OPT_FORCE_DEFAULTS},
	  {"force-depends", 0, 0, ARGS_OPT_FORCE_DEPENDS},
	  {"force_depends", 0, 0, ARGS_OPT_FORCE_DEPENDS},
	  {"force-overwrite", 0, 0, ARGS_OPT_FORCE_OVERWRITE},
	  {"force_overwrite", 0, 0, ARGS_OPT_FORCE_OVERWRITE},
	  {"force_downgrade", 0, 0, ARGS_OPT_FORCE_DOWNGRADE},
	  {"force-downgrade", 0, 0, ARGS_OPT_FORCE_DOWNGRADE},
	  {"force-reinstall", 0, 0, ARGS_OPT_FORCE_REINSTALL},
	  {"force_reinstall", 0, 0, ARGS_OPT_FORCE_REINSTALL},
	  {"force-space", 0, 0, ARGS_OPT_FORCE_SPACE},
	  {"force_space", 0, 0, ARGS_OPT_FORCE_SPACE},
	  {"recursive", 0, 0,
	   ARGS_OPT_FORCE_REMOVAL_OF_DEPENDENT_PACKAGES},
	  {"force-removal-of-dependent-packages", 0, 0,
	   ARGS_OPT_FORCE_REMOVAL_OF_DEPENDENT_PACKAGES},
	  {"force_removal_of_dependent_packages", 0, 0,
	   ARGS_OPT_FORCE_REMOVAL_OF_DEPENDENT_PACKAGES},
	  {"force-removal-of-essential-packages", 0, 0,
	   ARGS_OPT_FORCE_REMOVAL_OF_ESSENTIAL_PACKAGES},
	  {"force_removal_of_essential_packages", 0, 0,
	   ARGS_OPT_FORCE_REMOVAL_OF_ESSENTIAL_PACKAGES},
	  {"multiple-providers", 0, 0, ARGS_OPT_MULTIPLE_PROVIDERS},
	  {"multiple_providers", 0, 0, ARGS_OPT_MULTIPLE_PROVIDERS},
	  {"noaction", 0, 0, ARGS_OPT_NOACTION},
	  {"nodeps", 0, 0, ARGS_OPT_NODEPS},
	  {"offline", 1, 0, 'o'},
	  {"offline-root", 1, 0, 'o'},
	  {"test", 0, 0, ARGS_OPT_NOACTION},
	  {"tmp-dir", 1, 0, 't'},
	  {"verbose-wget", 0, 0, ARGS_OPT_VERBOSE_WGET},
	  {"verbose_wget", 0, 0, ARGS_OPT_VERBOSE_WGET},
	  {"verbosity", 2, 0, 'V'},
	  {"version", 0, 0, 'v'},
	  {0, 0, 0, 0}
     };

     while (1) {
	  c = getopt_long_only(argc, argv, "Ad:f:no:t:vV:", long_options, &option_index);
	  if (c == -1)
	       break;

	  switch (c) {
	  case 'A':
	       args->query_all = 1;
	       break;
	  case 'd':
	       args->dest = optarg;
	       break;
	  case 'f':
	       free(args->conf_file);
	       args->conf_file = strdup(optarg);
	       break;
	  case 'o':
	       args->offline_root = optarg;
	       break;
	  case 'n':
	       args->noaction = 1;
	       break;
	  case 't':
	       args->tmp_dir = strdup(optarg);
	       break;
	  case 'v':
	       print_version();
	       exit(0);
	  case 'V':
	  case ARGS_OPT_VERBOSITY:
	       if (optarg)
		    args->verbosity = atoi(optarg);
	       else
		    args->verbosity += 1;
	       break;
	  case ARGS_OPT_FORCE_DEFAULTS:
	       args->force_defaults = 1;
	       break;
	  case ARGS_OPT_FORCE_DEPENDS:
	       args->force_depends = 1;
	       break;
	  case ARGS_OPT_FORCE_OVERWRITE:
	       args->force_overwrite = 1;
	       break;
	  case ARGS_OPT_FORCE_DOWNGRADE:
	       args->force_downgrade = 1;
	       break;
	  case ARGS_OPT_FORCE_REINSTALL:
	       args->force_reinstall = 1;
	       break;
	  case ARGS_OPT_FORCE_REMOVAL_OF_ESSENTIAL_PACKAGES:
	       args->force_removal_of_essential_packages = 1;
	       break;
	  case ARGS_OPT_FORCE_REMOVAL_OF_DEPENDENT_PACKAGES:
	       args->force_removal_of_dependent_packages = 1;
	       break;
	  case ARGS_OPT_FORCE_SPACE:
	       args->force_space = 1;
	       break;
	  case ARGS_OPT_VERBOSE_WGET:
	       args->verbose_wget = 1;
	       break;
	  case ARGS_OPT_MULTIPLE_PROVIDERS:
	       args->multiple_providers = 1;
	       break;
	  case ARGS_OPT_NODEPS:
	       args->nodeps = 1;
	       break;
	  case ARGS_OPT_NOACTION:
	       args->noaction = 1;
	       break;
	  case ':':
	       parse_err++;
	       break;
	  case '?':
	       parse_err++;
	       break;
	  default:
	       printf("Confusion: getopt_long returned %d\n", c);
	  }
     }
    
     if (parse_err) {
	  return -parse_err;
     } else {
	  return optind;
     }
}

void args_usage(char *complaint)
{
     if (complaint) {
	  fprintf(stderr, "ipkg: %s\n", complaint);
     }
     print_version();
     fprintf(stderr, "usage: ipkg [options...] sub-command [arguments...]\n");
     fprintf(stderr, "where sub-command is one of:\n");
    
     fprintf(stderr, "\nPackage Manipulation:\n");
     fprintf(stderr, "\tupdate  		Update list of available packages\n");
     fprintf(stderr, "\tupgrade			Upgrade all installed packages to latest version\n");
     fprintf(stderr, "\tinstall <pkg>		Download and install <pkg> (and dependencies)\n");
     fprintf(stderr, "\tinstall <file.ipk>	Install package <file.ipk>\n");
     fprintf(stderr, "\tconfigure [<pkg>]	Configure unpacked packages\n");
     fprintf(stderr, "\tremove <pkg|regexp>	Remove package <pkg|packages following regexp>\n");
     fprintf(stderr, "\tflag <flag> <pkg> ...	Flag package(s) <pkg>\n");
     fprintf(stderr, "\t <flag>=hold|noprune|user|ok|installed|unpacked (one per invocation)	\n");
    
     fprintf(stderr, "\nInformational Commands:\n");
     fprintf(stderr, "\tlist    		List available packages and descriptions\n");
     fprintf(stderr, "\tlist_installed		List all and only the installed packages and description \n");
     fprintf(stderr, "\tfiles <pkg>		List all files belonging to <pkg>\n");
     fprintf(stderr, "\tsearch <file|regexp>		Search for a package providing <file>\n");
#ifndef IPKG_LIB
     fprintf(stderr, "\tinfo [pkg|regexp [<field>]]	Display all/some info fields for <pkg> or all\n");
     fprintf(stderr, "\tstatus [pkg|regexp [<field>]]	Display all/some status fields for <pkg> or all\n");
#else
     fprintf(stderr, "\tinfo [pkg|regexp]		Display all info for <pkg>\n");
     fprintf(stderr, "\tstatus [pkg|regexp]		Display all status for <pkg>\n");
#endif
     fprintf(stderr, "\tdownload <pkg>		Download <pkg> to current directory.\n");
     fprintf(stderr, "\tcompare_versions <v1> <op> <v2>\n");
     fprintf(stderr, "\t                          compare versions using <= < > >= = << >>\n");
     fprintf(stderr, "\tprint_architecture      prints the architecture.\n");
     fprintf(stderr, "\tprint_installation_architecture\n");
     fprintf(stderr, "\twhatdepends [-A] [pkgname|pat]+\n");
     fprintf(stderr, "\twhatdependsrec [-A] [pkgname|pat]+\n");
     fprintf(stderr, "\twhatprovides [-A] [pkgname|pat]+\n");
     fprintf(stderr, "\twhatconflicts [-A] [pkgname|pat]+\n");
     fprintf(stderr, "\twhatreplaces [-A] [pkgname|pat]+\n");
     fprintf(stderr, "\t                        prints the installation architecture.\n");    
     fprintf(stderr, "\nOptions:\n");
     fprintf(stderr, "\t-A                      Query all packages with whatdepends, whatprovides, whatreplaces, whatconflicts\n"); 
     fprintf(stderr, "\t-V <level>               Set verbosity level to <level>. If no value is\n");
     fprintf(stderr, "\t--verbosity <level>      provided increase verbosity by one. Verbosity levels:\n");
     fprintf(stderr, "\t                         0 errors only\n");
     fprintf(stderr, "\t                         1 normal messages (default)\n");
     fprintf(stderr, "\t                         2 informative messages\n");
     fprintf(stderr, "\t                         3 debug output\n");
     fprintf(stderr, "\t-f <conf_file>		Use <conf_file> as the ipkg configuration file\n");
     fprintf(stderr, "\t-conf <conf_file>	Default configuration file location\n");
     fprintf(stderr, "				is %s/%s\n", ARGS_DEFAULT_CONF_FILE_DIR, ARGS_DEFAULT_CONF_FILE_NAME);
     fprintf(stderr, "\t-d <dest_name>		Use <dest_name> as the the root directory for\n");
     fprintf(stderr, "\t-dest <dest_name>	package installation, removal, upgrading.\n");
     fprintf(stderr, "				<dest_name> should be a defined dest name from\n");
     fprintf(stderr, "				the configuration file, (but can also be a\n");
     fprintf(stderr, "				directory name in a pinch).\n");
     fprintf(stderr, "\t-o <offline_root>	Use <offline_root> as the root directory for\n");
     fprintf(stderr, "\t-offline <offline_root>	offline installation of packages.\n");
     fprintf(stderr, "\t-verbose_wget		more wget messages\n");
    
     fprintf(stderr, "\tForce Options (use when ipkg is too smart for its own good):\n");
     fprintf(stderr, "\t-force-depends		Make dependency checks warnings instead of errors\n");
     fprintf(stderr, "\t				Install/remove package in spite of failed dependences\n");
     fprintf(stderr, "\t-force-defaults		Use default options for questions asked by ipkg.\n");
     fprintf(stderr, "				(no prompts). Note that this will not prevent\n");
     fprintf(stderr, "				package installation scripts from prompting.\n");
     fprintf(stderr, "\t-force-reinstall 	Allow ipkg to reinstall a package.\n");
     fprintf(stderr, "\t-force-overwrite 	Allow ipkg to overwrite files from another package during an install.\n");
     fprintf(stderr, "\t-force-downgrade 	Allow ipkg to downgrade packages.\n");
     fprintf(stderr, "\t-force_space            Install even if there does not seem to be enough space.\n");
     fprintf(stderr, "\t-noaction               No action -- test only\n");
     fprintf(stderr, "\t-nodeps                 Do not follow dependences\n");
     fprintf(stderr, "\t-force-removal-of-dependent-packages\n");
     fprintf(stderr, "\t-recursive	 	Allow ipkg to remove package and all that depend on it.\n");
     fprintf(stderr, "\t-test                   No action -- test only\n");
     fprintf(stderr, "\t-t	 	        Specify tmp-dir.\n");
     fprintf(stderr, "\t--tmp-dir 	        Specify tmp-dir.\n");
     fprintf(stderr, "\n");
     fprintf(stderr, "\tregexp could be something like 'pkgname*' '*file*' or similar\n");
     fprintf(stderr, "\teg: ipkg info 'libstd*'  or ipkg search '*libop*' or ipkg remove 'libncur*'\n");
     /* -force-removal-of-essential-packages	Let ipkg remove essential packages. 
	Using this option is almost guaranteed to break your system, hence this option
	is not even advertised in the usage statement. */
     exit(1);
}

static void print_version(void)
{
     fprintf(stderr, "ipkg version %s\n", VERSION);
}
