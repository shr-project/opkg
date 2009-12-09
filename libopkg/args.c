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
#include "opkg_conf.h"

static void print_version(void);

enum long_args_opt
{
     ARGS_OPT_FORCE_MAINTAINER = 129, 
     ARGS_OPT_FORCE_DEPENDS,
     ARGS_OPT_FORCE_OVERWRITE,
     ARGS_OPT_FORCE_DOWNGRADE,
     ARGS_OPT_FORCE_REINSTALL,
     ARGS_OPT_FORCE_REMOVAL_OF_DEPENDENT_PACKAGES,
     ARGS_OPT_FORCE_REMOVAL_OF_ESSENTIAL_PACKAGES,
     ARGS_OPT_FORCE_SPACE,
     ARGS_OPT_NOACTION,
     ARGS_OPT_NODEPS,
     ARGS_OPT_AUTOREMOVE,
     ARGS_OPT_CACHE,
};

void args_init(args_t *args)
{
     memset(args, 0, sizeof(args_t));

     args->dest = ARGS_DEFAULT_DEST;

     sprintf_alloc(&args->conf_file, "%s/%s", OPKGETCDIR,
		   ARGS_DEFAULT_CONF_FILE_NAME);
     conf->verbosity = ARGS_DEFAULT_VERBOSITY;
}

void args_deinit(args_t *args)
{
     free (args->dest);
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
	  {"noaction", 0, 0, ARGS_OPT_NOACTION},
	  {"nodeps", 0, 0, ARGS_OPT_NODEPS},
	  {"offline", 1, 0, 'o'},
	  {"offline-root", 1, 0, 'o'},
	  {"test", 0, 0, ARGS_OPT_NOACTION},
	  {"tmp-dir", 1, 0, 't'},
	  {"tmp_dir", 1, 0, 't'},
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
	       conf->query_all = 1;
	       break;
	  case 'd':
	       args->dest = xstrdup(optarg);
	       break;
	  case 'f':
	       free(args->conf_file);
	       args->conf_file = xstrdup(optarg);
	       break;
	  case 'o':
	       conf->offline_root = xstrdup(optarg);
	       break;
	  case 't':
	       conf->tmp_dir = xstrdup(optarg);
	       break;
	  case 'v':
	       print_version();
	       exit(0);
	  case 'V':
	       conf->verbosity = atoi(optarg);
	       break;
	  case ARGS_OPT_AUTOREMOVE:
	       conf->autoremove = 1;
	       break;
	  case ARGS_OPT_CACHE:
	       free(conf->cache);
	       conf->cache = xstrdup(optarg);
	       break;
          case ARGS_OPT_FORCE_MAINTAINER:
               conf->force_maintainer = 1;
               break;
	  case ARGS_OPT_FORCE_DEPENDS:
	       conf->force_depends = 1;
	       break;
	  case ARGS_OPT_FORCE_OVERWRITE:
	       conf->force_overwrite = 1;
	       break;
	  case ARGS_OPT_FORCE_DOWNGRADE:
	       conf->force_downgrade = 1;
	       break;
	  case ARGS_OPT_FORCE_REINSTALL:
	       conf->force_reinstall = 1;
	       break;
	  case ARGS_OPT_FORCE_REMOVAL_OF_ESSENTIAL_PACKAGES:
	       conf->force_removal_of_essential_packages = 1;
	       break;
	  case ARGS_OPT_FORCE_REMOVAL_OF_DEPENDENT_PACKAGES:
	       conf->force_removal_of_dependent_packages = 1;
	       break;
	  case ARGS_OPT_FORCE_SPACE:
	       conf->force_space = 1;
	       break;
	  case ARGS_OPT_NODEPS:
	       conf->nodeps = 1;
	       break;
	  case ARGS_OPT_NOACTION:
	       conf->noaction = 1;
	       break;
	  case ':':
	       parse_err = -1;
	       break;
	  case '?':
	       parse_err = -1;
	       break;
	  default:
	       printf("Confusion: getopt_long returned %d\n", c);
	  }
     }
    
     if (parse_err) {
	  return parse_err;
     } else {
	  return optind;
     }
}

void args_usage(const char *complaint)
{
     if (complaint) {
	  printf("opkg: %s\n", complaint);
     }
     print_version();
     printf("usage: opkg [options...] sub-command [arguments...]\n");
     printf("where sub-command is one of:\n");
    
     printf("\nPackage Manipulation:\n");
     printf("\tupdate			Update list of available packages\n");
     printf("\tupgrade			Upgrade installed packages\n");
     printf("\tinstall <pkgs>		Install package(s)\n");
     printf("\tconfigure <pkgs>	Configure unpacked package(s)\n");
     printf("\tremove <pkgs|regexp>	Remove package(s)\n");
     printf("\tflag <flag> <pkgs>	Flag package(s)\n");
     printf("\t <flag>=hold|noprune|user|ok|installed|unpacked (one per invocation)\n");
    
     printf("\nInformational Commands:\n");
     printf("\tlist			List available packages\n");
     printf("\tlist-installed		List installed packages\n");
     printf("\tlist-upgradable		List installed and upgradable packages\n");
     printf("\tfiles <pkg>		List files belonging to <pkg>\n");
     printf("\tsearch <file|regexp>	List package providing <file>\n");
     printf("\tinfo [pkg|regexp]	Display all info for <pkg>\n");
     printf("\tstatus [pkg|regexp]	Display all status for <pkg>\n");
     printf("\tdownload <pkg>		Download <pkg> to current directory\n");
     printf("\tcompare-versions <v1> <op> <v2>\n");
     printf("\t                    compare versions using <= < > >= = << >>\n");
     printf("\tprint-architecture	List installable package architectures\n");
     printf("\twhatdepends [-A] [pkgname|pat]+\n");
     printf("\twhatdependsrec [-A] [pkgname|pat]+\n");
     printf("\twhatprovides [-A] [pkgname|pat]+\n");
     printf("\twhatconflicts [-A] [pkgname|pat]+\n");
     printf("\twhatreplaces [-A] [pkgname|pat]+\n");

     printf("\nOptions:\n");
     printf("\t-A			Query all packages not just those installed\n"); 
     printf("\t-V <level>		Set verbosity level to <level>.\n");
     printf("\t--verbosity <level>	Verbosity levels:\n");
     printf("\t				0 errors only\n");
     printf("\t				1 normal messages (default)\n");
     printf("\t				2 informative messages\n");
     printf("\t				3 debug\n");
     printf("\t				4 debug level 2\n");
     printf("\t-f <conf_file>		Use <conf_file> as the opkg configuration file\n");
     printf("\t--conf <conf_file>	Default configuration file location\n");
     printf("				is %s/%s\n", ARGS_DEFAULT_CONF_FILE_DIR, ARGS_DEFAULT_CONF_FILE_NAME);
     printf("\t--cache <directory>	Use a package cache\n");
     printf("\t-d <dest_name>		Use <dest_name> as the the root directory for\n");
     printf("\t--dest <dest_name>	package installation, removal, upgrading.\n");
     printf("				<dest_name> should be a defined dest name from\n");
     printf("				the configuration file, (but can also be a\n");
     printf("				directory name in a pinch).\n");
     printf("\t-o <dir>		Use <dir> as the root directory for\n");
     printf("\t--offline-root <dir>	offline installation of packages.\n");

     printf("\nForce Options:\n");
     printf("\t--force-depends		Install/remove despite failed dependences\n");
     printf("\t--force-maintainer	Overwrite preexisting config files\n");
     printf("\t--force-reinstall	Reinstall package(s)\n");
     printf("\t--force-overwrite	Overwrite files from other package(s)\n");
     printf("\t--force-downgrade	Allow opkg to downgrade packages\n");
     printf("\t--force-space		Disable free space checks\n");
     printf("\t--noaction		No action -- test only\n");
     printf("\t--nodeps		Do not follow dependences\n");
     printf("\t--force-removal-of-dependent-packages\n");
     printf("\t			Remove package and all dependencies\n");
     printf("\t--autoremove		Remove packages that were installed\n");
     printf("\t			automatically to satisfy dependencies\n");
     printf("\t-t			Specify tmp-dir.\n");
     printf("\t--tmp-dir		Specify tmp-dir.\n");

     printf("\n");

     printf(" regexp could be something like 'pkgname*' '*file*' or similar\n");
     printf(" e.g. opkg info 'libstd*' or opkg search '*libop*' or opkg remove 'libncur*'\n");
     /* -force-removal-of-essential-packages	Let opkg remove essential packages. 
	Using this option is almost guaranteed to break your system, hence this option
	is not even advertised in the usage statement. */
     exit(1);
}

static void print_version(void)
{
     printf("opkg version %s\n", VERSION);
}
