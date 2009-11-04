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

#include "includes.h"

#include "config.h"
#include "args.h"
#include "sprintf_alloc.h"
#include "libbb/libbb.h"

static void print_version(void);

enum long_args_opt
{
     ARGS_OPT_FORCE_DEFAULTS = 129,
     ARGS_OPT_FORCE_MAINTAINER, 
     ARGS_OPT_FORCE_DEPENDS,
     ARGS_OPT_FORCE_OVERWRITE,
     ARGS_OPT_FORCE_DOWNGRADE,
     ARGS_OPT_FORCE_REINSTALL,
     ARGS_OPT_FORCE_REMOVAL_OF_DEPENDENT_PACKAGES,
     ARGS_OPT_FORCE_REMOVAL_OF_ESSENTIAL_PACKAGES,
     ARGS_OPT_FORCE_SPACE,
     ARGS_OPT_NOACTION,
     ARGS_OPT_NODEPS,
     ARGS_OPT_VERBOSITY,
     ARGS_OPT_MULTIPLE_PROVIDERS,
     ARGS_OPT_AUTOREMOVE,
     ARGS_OPT_CACHE,
};

char *conf_file_dir;

int args_init(args_t *args)
{
     if (!args) {
	  return EFAULT;
     }
     memset(args, 0, sizeof(args_t));

     args->dest = ARGS_DEFAULT_DEST;

     conf_file_dir = getenv("OPKG_CONF_DIR");
     if (conf_file_dir == NULL || conf_file_dir[0] == '\0') {
	  conf_file_dir = ARGS_DEFAULT_CONF_FILE_DIR;
     }
     sprintf_alloc(&args->conf_file, "%s/%s", OPKGETCDIR,
		   ARGS_DEFAULT_CONF_FILE_NAME);

     args->force_defaults = ARGS_DEFAULT_FORCE_DEFAULTS;
     args->force_maintainer = ARGS_DEFAULT_FORCE_MAINTAINER;
     args->force_depends = ARGS_DEFAULT_FORCE_DEPENDS;
     args->force_overwrite = ARGS_DEFAULT_FORCE_OVERWRITE;
     args->force_downgrade = ARGS_DEFAULT_FORCE_DOWNGRADE;
     args->force_reinstall = ARGS_DEFAULT_FORCE_REINSTALL;
     args->force_removal_of_dependent_packages = ARGS_DEFAULT_FORCE_REMOVAL_OF_DEPENDENT_PACKAGES;
     args->force_removal_of_essential_packages = ARGS_DEFAULT_FORCE_REMOVAL_OF_ESSENTIAL_PACKAGES;
     args->autoremove = ARGS_DEFAULT_AUTOREMOVE;
     args->noaction = ARGS_DEFAULT_NOACTION;
     args->nodeps = ARGS_DEFAULT_NODEPS;
     args->verbosity = ARGS_DEFAULT_VERBOSITY;
     args->offline_root = ARGS_DEFAULT_OFFLINE_ROOT;
     args->offline_root_path = ARGS_DEFAULT_OFFLINE_ROOT_PATH;
     args->offline_root_pre_script_cmd = ARGS_DEFAULT_OFFLINE_ROOT_PRE_SCRIPT_CMD;
     args->offline_root_post_script_cmd = ARGS_DEFAULT_OFFLINE_ROOT_POST_SCRIPT_CMD;
     args->multiple_providers = 0;
     args->nocheckfordirorfile = 0;
     args->noreadfeedsfile = 0;

     return 0;
}

void args_deinit(args_t *args)
{
     free (args->offline_root);
     free (args->offline_root_path);
     free (args->offline_root_pre_script_cmd);
     free (args->offline_root_post_script_cmd);

     free (args->dest);
     free (args->tmp_dir);
     free (args->cache);
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
	  {"autoremove", 0, 0, ARGS_OPT_AUTOREMOVE},
	  {"cache", 1, 0, ARGS_OPT_CACHE},
	  {"conf-file", 1, 0, 'f'},
	  {"conf", 1, 0, 'f'},
	  {"dest", 1, 0, 'd'},
	  {"force-defaults", 0, 0, ARGS_OPT_FORCE_DEFAULTS},
	  {"force_defaults", 0, 0, ARGS_OPT_FORCE_DEFAULTS},
          {"force-maintainer", 0, 0, ARGS_OPT_FORCE_MAINTAINER}, 
          {"force_maintainer", 0, 0, ARGS_OPT_FORCE_MAINTAINER}, 
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
	  {"offline-path", 1, 0, 'p'},
	  {"offline-root-path", 1, 0, 'p'},
	  {"test", 0, 0, ARGS_OPT_NOACTION},
	  {"tmp-dir", 1, 0, 't'},
	  {"verbosity", 2, 0, 'V'},
	  {"version", 0, 0, 'v'},
	  {0, 0, 0, 0}
     };

     while (1) {
	  c = getopt_long_only(argc, argv, "Ad:f:no:p:t:vV:", long_options, &option_index);
	  if (c == -1)
	       break;

	  switch (c) {
	  case 'A':
	       args->query_all = 1;
	       break;
	  case 'd':
	       args->dest = xstrdup(optarg);
	       break;
	  case 'f':
	       free(args->conf_file);
	       args->conf_file = xstrdup(optarg);
	       break;
	  case 'o':
	       args->offline_root = xstrdup(optarg);
	       break;
	  case 'p':
	       args->offline_root_path = xstrdup(optarg);
	       break;
	  case 'n':
	       args->noaction = 1;
	       break;
	  case 't':
	       args->tmp_dir = xstrdup(optarg);
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
	  case ARGS_OPT_AUTOREMOVE:
	       args->autoremove = 1;
	       break;
	  case ARGS_OPT_CACHE:
	       free(args->cache);
	       args->cache = xstrdup(optarg);
	       break;
	  case ARGS_OPT_FORCE_DEFAULTS:
	       args->force_defaults = 1;
	       break;
          case ARGS_OPT_FORCE_MAINTAINER:
               args->force_maintainer = 1;
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
	  printf("opkg: %s\n", complaint);
     }
     print_version();
     printf("usage: opkg [options...] sub-command [arguments...]\n");
     printf("where sub-command is one of:\n");
    
     printf("\nPackage Manipulation:\n");
     printf("\tupdate  		Update list of available packages\n");
     printf("\tupgrade			Upgrade all installed packages to latest version\n");
     printf("\tinstall <pkg>		Download and install <pkg> (and dependencies)\n");
     printf("\tinstall <file.opk>	Install package <file.opk>\n");
     printf("\tconfigure [<pkg>]	Configure unpacked packages\n");
     printf("\tremove <pkg|regexp>	Remove package <pkg|packages following regexp>\n");
     printf("\tflag <flag> <pkg> ...	Flag package(s) <pkg>\n");
     printf("\t <flag>=hold|noprune|user|ok|installed|unpacked (one per invocation)	\n");
    
     printf("\nInformational Commands:\n");
     printf("\tlist    		List available packages and descriptions\n");
     printf("\tlist_installed		List all and only the installed packages and description \n");
     printf("\tlist_upgradable		List all the installed and upgradable packages\n");
     printf("\tfiles <pkg>		List all files belonging to <pkg>\n");
     printf("\tsearch <file|regexp>		Search for a package providing <file>\n");
     printf("\tinfo [pkg|regexp]		Display all info for <pkg>\n");
     printf("\tstatus [pkg|regexp]		Display all status for <pkg>\n");
     printf("\tdownload <pkg>		Download <pkg> to current directory.\n");
     printf("\tcompare_versions <v1> <op> <v2>\n");
     printf("\t                          compare versions using <= < > >= = << >>\n");
     printf("\tprint_architecture      prints the architecture.\n");
     printf("\tprint_installation_architecture\n");
     printf("\twhatdepends [-A] [pkgname|pat]+\n");
     printf("\twhatdependsrec [-A] [pkgname|pat]+\n");
     printf("\twhatprovides [-A] [pkgname|pat]+\n");
     printf("\twhatconflicts [-A] [pkgname|pat]+\n");
     printf("\twhatreplaces [-A] [pkgname|pat]+\n");
     printf("\t                        prints the installation architecture.\n");    
     printf("\nOptions:\n");
     printf("\t-A                      Query all packages with whatdepends, whatprovides, whatreplaces, whatconflicts\n"); 
     printf("\t-V <level>               Set verbosity level to <level>. If no value is\n");
     printf("\t--verbosity <level>      provided increase verbosity by one. Verbosity levels:\n");
     printf("\t                         0 errors only\n");
     printf("\t                         1 normal messages (default)\n");
     printf("\t                         2 informative messages\n");
     printf("\t                         3 debug output\n");
     printf("\t-f <conf_file>		Use <conf_file> as the opkg configuration file\n");
     printf("\t--cache <directory>	Use a package cache\n");
     printf("\t-conf <conf_file>	Default configuration file location\n");
     printf("				is %s/%s\n", ARGS_DEFAULT_CONF_FILE_DIR, ARGS_DEFAULT_CONF_FILE_NAME);
     printf("\t-d <dest_name>		Use <dest_name> as the the root directory for\n");
     printf("\t-dest <dest_name>	package installation, removal, upgrading.\n");
     printf("				<dest_name> should be a defined dest name from\n");
     printf("				the configuration file, (but can also be a\n");
     printf("				directory name in a pinch).\n");
     printf("\t-o <offline_root>	Use <offline_root> as the root directory for\n");
     printf("\t-offline <offline_root>	offline installation of packages.\n");
     printf("\t-p <path>		Path to utilities for runing postinst\n");
     printf("\t-offline-path <path>	script in offline mode.\n");

     printf("\nForce Options (use when opkg is too smart for its own good):\n");
     printf("\t-force-depends		Make dependency checks warnings instead of errors\n");
     printf("\t				Install/remove package in spite of failed dependences\n");
     printf("\t-force-defaults		Use default options for questions asked by opkg.\n");
     printf("				(no prompts). Note that this will not prevent\n");
     printf("				package installation scripts from prompting.\n");
     printf("\t-force-reinstall 	Allow opkg to reinstall a package.\n");
     printf("\t-force-overwrite 	Allow opkg to overwrite files from another package during an install.\n");
     printf("\t-force-downgrade 	Allow opkg to downgrade packages.\n");
     printf("\t-force_space            Install even if there does not seem to be enough space.\n");
     printf("\t-noaction               No action -- test only\n");
     printf("\t-nodeps                 Do not follow dependences\n");
     printf("\t-force-removal-of-dependent-packages\n");
     printf("\t-recursive	 	Allow opkg to remove package and all that depend on it.\n");
     printf("\t-autoremove	 	Allow opkg to remove packages that where installed automatically to satisfy dependencies.\n");
     printf("\t-test                   No action -- test only\n");
     printf("\t-t	 	        Specify tmp-dir.\n");
     printf("\t--tmp-dir 	        Specify tmp-dir.\n");
     printf("\n");
     printf("\tregexp could be something like 'pkgname*' '*file*' or similar\n");
     printf("\teg: opkg info 'libstd*'  or opkg search '*libop*' or opkg remove 'libncur*'\n");
     /* -force-removal-of-essential-packages	Let opkg remove essential packages. 
	Using this option is almost guaranteed to break your system, hence this option
	is not even advertised in the usage statement. */
     exit(1);
}

static void print_version(void)
{
     printf("opkg version %s\n", VERSION);
}
