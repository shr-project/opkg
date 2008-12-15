/* opkg_cmd.c - the itsy package management system

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

#include <string.h>

#include "opkg.h"
#include <libgen.h>
#include <glob.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <dirent.h>

#include "opkg_conf.h"
#include "opkg_cmd.h"
#include "opkg_message.h"
#include "pkg.h"
#include "pkg_dest.h"
#include "pkg_parse.h"
#include "sprintf_alloc.h"
#include "pkg.h"
#include "file_util.h"
#include "str_util.h"
#include "libbb/libbb.h"

#include <fnmatch.h>


#include "opkg_download.h"
#include "opkg_install.h"
#include "opkg_upgrade.h"
#include "opkg_remove.h"
#include "opkg_configure.h"
#include "opkg_message.h"

#ifdef OPKG_LIB
#include "libopkg.h"
static void *p_userdata = NULL;
#endif

static int opkg_update_cmd(opkg_conf_t *conf, int argc, char **argv);
static int opkg_upgrade_cmd(opkg_conf_t *conf, int argc, char **argv);
static int opkg_list_cmd(opkg_conf_t *conf, int argc, char **argv);
static int opkg_info_cmd(opkg_conf_t *conf, int argc, char **argv);
static int opkg_status_cmd(opkg_conf_t *conf, int argc, char **argv);
static int opkg_install_pending_cmd(opkg_conf_t *conf, int argc, char **argv);
static int opkg_install_cmd(opkg_conf_t *conf, int argc, char **argv);
static int opkg_list_installed_cmd(opkg_conf_t *conf, int argc, char **argv);
static int opkg_remove_cmd(opkg_conf_t *conf, int argc, char **argv);
static int opkg_purge_cmd(opkg_conf_t *conf, int argc, char **argv);
static int opkg_flag_cmd(opkg_conf_t *conf, int argc, char **argv);
static int opkg_files_cmd(opkg_conf_t *conf, int argc, char **argv);
static int opkg_search_cmd(opkg_conf_t *conf, int argc, char **argv);
static int opkg_download_cmd(opkg_conf_t *conf, int argc, char **argv);
static int opkg_depends_cmd(opkg_conf_t *conf, int argc, char **argv);
static int opkg_whatdepends_cmd(opkg_conf_t *conf, int argc, char **argv);
static int opkg_whatdepends_recursively_cmd(opkg_conf_t *conf, int argc, char **argv);
static int opkg_whatsuggests_cmd(opkg_conf_t *conf, int argc, char **argv);
static int opkg_whatrecommends_cmd(opkg_conf_t *conf, int argc, char **argv);
static int opkg_whatprovides_cmd(opkg_conf_t *conf, int argc, char **argv);
static int opkg_whatconflicts_cmd(opkg_conf_t *conf, int argc, char **argv);
static int opkg_whatreplaces_cmd(opkg_conf_t *conf, int argc, char **argv);
static int opkg_compare_versions_cmd(opkg_conf_t *conf, int argc, char **argv);
static int opkg_print_architecture_cmd(opkg_conf_t *conf, int argc, char **argv);
static int opkg_configure_cmd(opkg_conf_t *conf, int argc, char **argv);

/* XXX: CLEANUP: The usage strings should be incorporated into this
   array for easier maintenance */
static opkg_cmd_t cmds[] = {
     {"update", 0, (opkg_cmd_fun_t)opkg_update_cmd}, 
     {"upgrade", 0, (opkg_cmd_fun_t)opkg_upgrade_cmd},
     {"list", 0, (opkg_cmd_fun_t)opkg_list_cmd},
     {"list_installed", 0, (opkg_cmd_fun_t)opkg_list_installed_cmd},
     {"info", 0, (opkg_cmd_fun_t)opkg_info_cmd},
     {"flag", 1, (opkg_cmd_fun_t)opkg_flag_cmd},
     {"status", 0, (opkg_cmd_fun_t)opkg_status_cmd},
     {"install_pending", 0, (opkg_cmd_fun_t)opkg_install_pending_cmd},
     {"install", 1, (opkg_cmd_fun_t)opkg_install_cmd},
     {"remove", 1, (opkg_cmd_fun_t)opkg_remove_cmd},
     {"purge", 1, (opkg_cmd_fun_t)opkg_purge_cmd},
     {"configure", 0, (opkg_cmd_fun_t)opkg_configure_cmd},
     {"files", 1, (opkg_cmd_fun_t)opkg_files_cmd},
     {"search", 1, (opkg_cmd_fun_t)opkg_search_cmd},
     {"download", 1, (opkg_cmd_fun_t)opkg_download_cmd},
     {"compare_versions", 1, (opkg_cmd_fun_t)opkg_compare_versions_cmd},
     {"compare-versions", 1, (opkg_cmd_fun_t)opkg_compare_versions_cmd},
     {"print-architecture", 0, (opkg_cmd_fun_t)opkg_print_architecture_cmd},
     {"print_architecture", 0, (opkg_cmd_fun_t)opkg_print_architecture_cmd},
     {"print-installation-architecture", 0, (opkg_cmd_fun_t)opkg_print_architecture_cmd},
     {"print_installation_architecture", 0, (opkg_cmd_fun_t)opkg_print_architecture_cmd},
     {"depends", 1, (opkg_cmd_fun_t)opkg_depends_cmd},
     {"whatdepends", 1, (opkg_cmd_fun_t)opkg_whatdepends_cmd},
     {"whatdependsrec", 1, (opkg_cmd_fun_t)opkg_whatdepends_recursively_cmd},
     {"whatrecommends", 1, (opkg_cmd_fun_t)opkg_whatrecommends_cmd},
     {"whatsuggests", 1, (opkg_cmd_fun_t)opkg_whatsuggests_cmd},
     {"whatprovides", 1, (opkg_cmd_fun_t)opkg_whatprovides_cmd},
     {"whatreplaces", 1, (opkg_cmd_fun_t)opkg_whatreplaces_cmd},
     {"whatconflicts", 1, (opkg_cmd_fun_t)opkg_whatconflicts_cmd},
};

int opkg_state_changed;
static void write_status_files_if_changed(opkg_conf_t *conf)
{
     if (opkg_state_changed && !conf->noaction) {
	  opkg_message(conf, OPKG_INFO,
		       "  writing status file\n");
	  opkg_conf_write_status_files(conf);
	  pkg_write_changed_filelists(conf);
     } else { 
	  opkg_message(conf, OPKG_NOTICE, "Nothing to be done\n");
     }
}


static int num_cmds = sizeof(cmds) / sizeof(opkg_cmd_t);

opkg_cmd_t *opkg_cmd_find(const char *name)
{
     int i;
     opkg_cmd_t *cmd;

     for (i=0; i < num_cmds; i++) {
	  cmd = &cmds[i];
	  if (strcmp(name, cmd->name) == 0) {
	       return cmd;
	  }
     }

     return NULL;
}

#ifdef OPKG_LIB
int opkg_cmd_exec(opkg_cmd_t *cmd, opkg_conf_t *conf, int argc, const char **argv, void *userdata)
{
	int result;
	p_userdata = userdata;
      

	result = (cmd->fun)(conf, argc, argv);

        if ( result != 0 ) {
           opkg_message(conf, OPKG_NOTICE, "An error ocurred, return value: %d.\n", result);
        }

        if ( error_list ) {
           reverse_error_list(&error_list);

           opkg_message(conf, OPKG_NOTICE, "Collected errors:\n");
           /* Here we print the errors collected and free the list */
           while (error_list != NULL) {
                 opkg_message(conf, OPKG_NOTICE, "%s",error_list->errmsg);
                 error_list = error_list->next;

           }
           free_error_list(&error_list);

        }
   
	p_userdata = NULL;
	return result;
}
#else
int opkg_cmd_exec(opkg_cmd_t *cmd, opkg_conf_t *conf, int argc, const char **argv)
{
     return (cmd->fun)(conf, argc, argv);
}
#endif

static int opkg_update_cmd(opkg_conf_t *conf, int argc, char **argv)
{
     char *tmp;
     int err;
     int failures;
     char *lists_dir;
     pkg_src_list_elt_t *iter;
     pkg_src_t *src;

 
    sprintf_alloc(&lists_dir, "%s", conf->restrict_to_default_dest ? conf->default_dest->lists_dir : conf->lists_dir);
 
    if (! file_is_dir(lists_dir)) {
	  if (file_exists(lists_dir)) {
	       opkg_message(conf, OPKG_ERROR,
			    "%s: ERROR: %s exists, but is not a directory\n",
			    __FUNCTION__, lists_dir);
	       free(lists_dir);
	       return EINVAL;
	  }
	  err = file_mkdir_hier(lists_dir, 0755);
	  if (err) {
	       opkg_message(conf, OPKG_ERROR,
			    "%s: ERROR: failed to make directory %s: %s\n",
			    __FUNCTION__, lists_dir, strerror(errno));
	       free(lists_dir);
	       return EINVAL;
	  }	
     } 

     failures = 0;


     tmp = strdup ("/tmp/opkg.XXXXXX");

     if (mkdtemp (tmp) == NULL) {
	 perror ("mkdtemp");
	 failures++;
     }


     for (iter = conf->pkg_src_list.head; iter; iter = iter->next) {
	  char *url, *list_file_name;

	  src = iter->data;

	  if (src->extra_data)	/* debian style? */
	      sprintf_alloc(&url, "%s/%s/%s", src->value, src->extra_data, 
			    src->gzip ? "Packages.gz" : "Packages");
	  else
	      sprintf_alloc(&url, "%s/%s", src->value, src->gzip ? "Packages.gz" : "Packages");

	  sprintf_alloc(&list_file_name, "%s/%s", lists_dir, src->name);
	  if (src->gzip) {
	      char *tmp_file_name;
	      FILE *in, *out;
	      
	      sprintf_alloc (&tmp_file_name, "%s/%s.gz", tmp, src->name);
	      err = opkg_download(conf, url, tmp_file_name);
	      if (err == 0) {
		   opkg_message (conf, OPKG_NOTICE, "Inflating %s\n", url);
		   in = fopen (tmp_file_name, "r");
		   out = fopen (list_file_name, "w");
		   if (in && out)
			unzip (in, out);
		   else
			err = 1;
		   if (in)
			fclose (in);
		   if (out)
			fclose (out);
		   unlink (tmp_file_name);
	      }
	  } else
	      err = opkg_download(conf, url, list_file_name);
	  if (err) {
	       failures++;
	  } else {
	       opkg_message(conf, OPKG_NOTICE,
			    "Updated list of available packages in %s\n",
			    list_file_name);
	  }
	  free(url);

	  /* download detached signitures to verify the package lists */
	  /* get the url for the sig file */
	  if (src->extra_data)	/* debian style? */
	      sprintf_alloc(&url, "%s/%s/%s", src->value, src->extra_data,
			    "Packages.sig");
	  else
	      sprintf_alloc(&url, "%s/%s", src->value, "Packages.sig");

	  /* create temporary file for it */
	  char *tmp_file_name;

	  sprintf_alloc (&tmp_file_name, "%s/%s", tmp, "Packages.sig");

	  err = opkg_download(conf, url, tmp_file_name);
	  if (err) {
	    failures++;
		opkg_message (conf, OPKG_NOTICE, "Signature check failed\n");
	  } else {
	    int err;
	    err = opkg_verify_file (list_file_name, tmp_file_name);
	    if (err == 0)
		opkg_message (conf, OPKG_NOTICE, "Signature check passed\n");
	    else
		opkg_message (conf, OPKG_NOTICE, "Signature check failed\n");
	  }
	  unlink (tmp_file_name);
	  free (tmp_file_name);

	  free (url);
	  free(list_file_name);
     }
     rmdir (tmp);
     free (tmp);
     free(lists_dir);

#ifdef CONFIG_CLEAR_SW_INSTALL_FLAG
#warning here
     /* clear SW_INSTALL on any package where state is SS_NOT_INSTALLED.
      * this is a hack to work around poor bookkeeping in old opkg upgrade code 
      * -Jamey 3/1/03
      */
     {
	  int i;
	  int changed = 0;
	  pkg_vec_t *available = pkg_vec_alloc();
	  pkg_hash_fetch_available(&conf->pkg_hash, available);
	  opkg_message(conf, OPKG_DEBUG, "Clearing SW_INSTALL for SS_NOT_INSTALLED packages.\n");
	  for (i = 0; i < available->len; i++) {
	       pkg_t *pkg = available->pkgs[i];
	       if (pkg->state_want == SW_INSTALL && pkg->state_status == SS_NOT_INSTALLED) {
		    opkg_message(conf, OPKG_DEBUG, "Clearing SW_INSTALL on package %s.\n", pkg->name);
		    pkg->state_want = SW_UNKNOWN;
		    changed = 1;
	       }
	  }
	  pkg_vec_free(available);
	  if (changed) {
	       write_status_files_if_changed(conf);
	  }
     }
#endif

     return failures;
}


/* scan the args passed and cache the local filenames of the packages */
int opkg_multiple_files_scan(opkg_conf_t *conf, int argc, char **argv)
{
     int i;
     int err;
    
     /* 
      * First scan through package names/urls
      * For any urls, download the packages and install in database.
      * For any files, install package info in database.
      */
     for (i = 0; i < argc; i ++) {
	  char *filename = argv [i];
	  //char *tmp = basename (tmp);
	  //int tmplen = strlen (tmp);

	  //if (strcmp (tmp + (tmplen - strlen (OPKG_PKG_EXTENSION)), OPKG_PKG_EXTENSION) != 0)
	  //     continue;
	  //if (strcmp (tmp + (tmplen - strlen (DPKG_PKG_EXTENSION)), DPKG_PKG_EXTENSION) != 0)
	  //     continue;
	
          opkg_message(conf, OPKG_DEBUG2, "Debug mfs: %s  \n",filename );

	  err = opkg_prepare_url_for_install(conf, filename, &argv[i]);
	  if (err)
	    return err;
     }
     return 0;
}

struct opkg_intercept
{
    char *oldpath;
    char *statedir;
};

typedef struct opkg_intercept *opkg_intercept_t;

opkg_intercept_t opkg_prep_intercepts(opkg_conf_t *conf)
{
    opkg_intercept_t ctx;
    char *newpath;
    int gen;

    ctx = malloc (sizeof (*ctx));
    ctx->oldpath = strdup (getenv ("PATH"));

    sprintf_alloc (&newpath, "%s/opkg/intercept:%s", DATADIR, ctx->oldpath);
    setenv ("PATH", newpath, 1);
    free (newpath);
    
    gen = 0;
 retry:
    sprintf_alloc (&ctx->statedir, "/tmp/opkg-intercept-%d-%d", getpid (), gen);
    if (mkdir (ctx->statedir, 0770) < 0) {
	if (errno == EEXIST) {
	    free (ctx->statedir);
	    gen++;
	    goto retry;
	}
	perror (ctx->statedir);
	return NULL;
    }
    setenv ("OPKG_INTERCEPT_DIR", ctx->statedir, 1);
    return ctx;
}

int opkg_finalize_intercepts(opkg_intercept_t ctx)
{
    char *cmd;
    DIR *dir;
    int err = 0;

    setenv ("PATH", ctx->oldpath, 1);
    free (ctx->oldpath);

    dir = opendir (ctx->statedir);
    if (dir) {
	struct dirent *de;
	while (de = readdir (dir), de != NULL) {
	    char *path;
	    
	    if (de->d_name[0] == '.')
		continue;
	    
	    sprintf_alloc (&path, "%s/%s", ctx->statedir, de->d_name);
	    if (access (path, X_OK) == 0) {
		if (system (path)) {
		    err = errno;
		    perror (de->d_name);
		}
	    }
	    free (path);
	}
    } else
	perror (ctx->statedir);
	
    sprintf_alloc (&cmd, "rm -rf %s", ctx->statedir);
    system (cmd);
    free (cmd);

    free (ctx->statedir);
    free (ctx);

    return err;
}

int opkg_configure_packages(opkg_conf_t *conf, char *pkg_name)
{
     pkg_vec_t *all;
     int i;
     pkg_t *pkg;
     opkg_intercept_t ic;
     int r, err = 0;

     opkg_message(conf, OPKG_INFO,
		  "Configuring unpacked packages\n");
     fflush( stdout );

     all = pkg_vec_alloc();
     pkg_hash_fetch_available(&conf->pkg_hash, all);

     ic = opkg_prep_intercepts (conf);
    
     for(i = 0; i < all->len; i++) {
	  pkg = all->pkgs[i];

	  if (pkg_name && fnmatch(pkg_name, pkg->name, 0)) 
	       continue;

	  if (pkg->state_status == SS_UNPACKED) {
	       opkg_message(conf, OPKG_NOTICE,
			    "Configuring %s\n", pkg->name);
	       fflush( stdout );
	       r = opkg_configure(conf, pkg);
	       if (r == 0) {
		    pkg->state_status = SS_INSTALLED;
		    pkg->parent->state_status = SS_INSTALLED;
		    pkg->state_flag &= ~SF_PREFER;
	       } else {
		    if (!err)
			err = r;
	       }
	  }
     }

     r = opkg_finalize_intercepts (ic);
     if (r && !err)
	 err = r;

     pkg_vec_free(all);
     return err;
}

static opkg_conf_t *global_conf;

static void sigint_handler(int sig)
{
     signal(sig, SIG_DFL);
     opkg_message(NULL, OPKG_NOTICE,
		  "opkg: interrupted. writing out status database\n");
     write_status_files_if_changed(global_conf);
     exit(128 + sig);
}

static int opkg_install_cmd(opkg_conf_t *conf, int argc, char **argv)
{
     int i;
     char *arg;
     int err=0;

     global_conf = conf;
     signal(SIGINT, sigint_handler);

     /*
      * Now scan through package names and install
      */
     for (i=0; i < argc; i++) {
	  arg = argv[i];

          opkg_message(conf, OPKG_DEBUG2, "Debug install_cmd: %s  \n",arg );
          err = opkg_prepare_url_for_install(conf, arg, &argv[i]);
          if (err != EINVAL && err != 0)
              return err;
     }
     pkg_info_preinstall_check(conf);

     for (i=0; i < argc; i++) {
	  arg = argv[i];
	  if (conf->multiple_providers)
	       err = opkg_install_multi_by_name(conf, arg);
	  else{
	       err = opkg_install_by_name(conf, arg);
          }
	  if (err == OPKG_PKG_HAS_NO_CANDIDATE) {
	       opkg_message(conf, OPKG_ERROR,
			    "Cannot find package %s.\n"
			    "Check the spelling or perhaps run 'opkg update'\n",
			    arg);
	  }
     }

     /* recheck to verify that all dependences are satisfied */
     if (0) opkg_satisfy_all_dependences(conf);

     opkg_configure_packages(conf, NULL);

     write_status_files_if_changed(conf);

     return err;
}

static int opkg_upgrade_cmd(opkg_conf_t *conf, int argc, char **argv)
{
     int i;
     pkg_t *pkg;
     int err;

     global_conf = conf;
     signal(SIGINT, sigint_handler);

     if (argc) {
	  for (i=0; i < argc; i++) {
	       char *arg = argv[i];

               err = opkg_prepare_url_for_install(conf, arg, &arg);
               if (err != EINVAL && err != 0)
                   return err;
	  }
	  pkg_info_preinstall_check(conf);

	  for (i=0; i < argc; i++) {
	       char *arg = argv[i];
	       if (conf->restrict_to_default_dest) {
		    pkg = pkg_hash_fetch_installed_by_name_dest(&conf->pkg_hash,
								argv[i],
								conf->default_dest);
		    if (pkg == NULL) {
			 opkg_message(conf, OPKG_NOTICE,
				      "Package %s not installed in %s\n",
				      argv[i], conf->default_dest->name);
			 continue;
		    }
	       } else {
		    pkg = pkg_hash_fetch_installed_by_name(&conf->pkg_hash,
							   argv[i]);
	       }
	       if (pkg)
		    opkg_upgrade_pkg(conf, pkg);
	       else {
		    opkg_install_by_name(conf, arg);
               }
	  }
     } else {
	  pkg_vec_t *installed = pkg_vec_alloc();

	  pkg_info_preinstall_check(conf);

	  pkg_hash_fetch_all_installed(&conf->pkg_hash, installed);
	  for (i = 0; i < installed->len; i++) {
	       pkg = installed->pkgs[i];
	       opkg_upgrade_pkg(conf, pkg);
	  }
	  pkg_vec_free(installed);
     }

     /* recheck to verify that all dependences are satisfied */
     if (0) opkg_satisfy_all_dependences(conf);

     opkg_configure_packages(conf, NULL);

     write_status_files_if_changed(conf);

     return 0;
}

static int opkg_download_cmd(opkg_conf_t *conf, int argc, char **argv)
{
     int i, err;
     char *arg;
     pkg_t *pkg;

     pkg_info_preinstall_check(conf);
     for (i = 0; i < argc; i++) {
	  arg = argv[i];

	  pkg = pkg_hash_fetch_best_installation_candidate_by_name(conf, arg);
	  if (pkg == NULL) {
	       opkg_message(conf, OPKG_ERROR,
			    "Cannot find package %s.\n"
			    "Check the spelling or perhaps run 'opkg update'\n",
			    arg);
	       continue;
	  }

	  err = opkg_download_pkg(conf, pkg, ".");

	  if (err) {
	       opkg_message(conf, OPKG_ERROR,
			    "Failed to download %s\n", pkg->name);
	  } else {
	       opkg_message(conf, OPKG_NOTICE,
			    "Downloaded %s as %s\n",
			    pkg->name, pkg->local_filename);
	  }
     }

     return 0;
}


static int opkg_list_cmd(opkg_conf_t *conf, int argc, char **argv)
{
     int i ;
     pkg_vec_t *available;
     pkg_t *pkg;
     char desc_short[OPKG_LIST_DESCRIPTION_LENGTH];
     char *newline;
     char *pkg_name = NULL;
     char *version_str;

     if (argc > 0) {
	  pkg_name = argv[0];
     }
     available = pkg_vec_alloc();
     pkg_hash_fetch_available(&conf->pkg_hash, available);
     for (i=0; i < available->len; i++) {
	  pkg = available->pkgs[i];
	  /* if we have package name or pattern and pkg does not match, then skip it */
	  if (pkg_name && fnmatch(pkg_name, pkg->name, 0)) 
	       continue;
	  if (pkg->description) {
	       strncpy(desc_short, pkg->description, OPKG_LIST_DESCRIPTION_LENGTH);
	  } else {
	       desc_short[0] = '\0';
	  }
	  desc_short[OPKG_LIST_DESCRIPTION_LENGTH - 1] = '\0';
	  newline = strchr(desc_short, '\n');
	  if (newline) {
	       *newline = '\0';
	  }
#ifndef OPKG_LIB
	  printf("%s - %s\n", pkg->name, desc_short);
#else
	  if (opkg_cb_list) {
	  	version_str = pkg_version_str_alloc(pkg);
	  	opkg_cb_list(pkg->name,desc_short,
		                             version_str,
	                                 pkg->state_status,
	                                 p_userdata);
		free(version_str);
	  }
#endif
     }
     pkg_vec_free(available);

     return 0;
}


static int opkg_list_installed_cmd(opkg_conf_t *conf, int argc, char **argv)
{
     int i ;
     pkg_vec_t *available;
     pkg_t *pkg;
     char desc_short[OPKG_LIST_DESCRIPTION_LENGTH];
     char *newline;
     char *pkg_name = NULL;
     char *version_str;

     if (argc > 0) {
	  pkg_name = argv[0];
     }
     available = pkg_vec_alloc();
     pkg_hash_fetch_all_installed(&conf->pkg_hash, available);
     for (i=0; i < available->len; i++) {
	  pkg = available->pkgs[i];
	  /* if we have package name or pattern and pkg does not match, then skip it */
	  if (pkg_name && fnmatch(pkg_name, pkg->name, 0)) 
	       continue;
	  if (pkg->description) {
	       strncpy(desc_short, pkg->description, OPKG_LIST_DESCRIPTION_LENGTH);
	  } else {
	       desc_short[0] = '\0';
	  }
	  desc_short[OPKG_LIST_DESCRIPTION_LENGTH - 1] = '\0';
	  newline = strchr(desc_short, '\n');
	  if (newline) {
	       *newline = '\0';
	  }
#ifndef OPKG_LIB
	  printf("%s - %s\n", pkg->name, desc_short);
#else
	  if (opkg_cb_list) {
	  	version_str = pkg_version_str_alloc(pkg);
	  	opkg_cb_list(pkg->name,desc_short,
		                             version_str,
	                                 pkg->state_status,
	                                 p_userdata);
		free(version_str);
	  }
#endif
     }

     return 0;
}

static int opkg_info_status_cmd(opkg_conf_t *conf, int argc, char **argv, int installed_only)
{
     int i;
     pkg_vec_t *available;
     pkg_t *pkg;
     char *pkg_name = NULL;
     char **pkg_fields = NULL;
     int n_fields = 0;
     char *buff ; // = (char *)malloc(1);

     if (argc > 0) {
	  pkg_name = argv[0];
     }
     if (argc > 1) {
	  pkg_fields = &argv[1];
	  n_fields = argc - 1;
     }

     available = pkg_vec_alloc();
     if (installed_only)
	  pkg_hash_fetch_all_installed(&conf->pkg_hash, available);
     else
	  pkg_hash_fetch_available(&conf->pkg_hash, available);
     for (i=0; i < available->len; i++) {
	  pkg = available->pkgs[i];
	  if (pkg_name && fnmatch(pkg_name, pkg->name, 0)) {
	       continue;
	  }
#ifndef OPKG_LIB
	  if (n_fields) {
	       for (j = 0; j < n_fields; j++)
		    pkg_print_field(pkg, stdout, pkg_fields[j]);
	  } else {
	       pkg_print_info(pkg, stdout);
	  }
#else

	  buff = pkg_formatted_info(pkg);
          if ( buff ) {
	       if (opkg_cb_status) opkg_cb_status(pkg->name,
						  pkg->state_status,
						  buff,
						  p_userdata);
/* 
   We should not forget that actually the pointer is allocated. 
   We need to free it :)  ( Thanks florian for seeing the error )
*/
               free(buff);
          }
#endif
	  if (conf->verbosity > 1) {
	       conffile_list_elt_t *iter;
	       for (iter = pkg->conffiles.head; iter; iter = iter->next) {
		    conffile_t *cf = iter->data;
		    int modified = conffile_has_been_modified(conf, cf);
		    opkg_message(conf, OPKG_NOTICE, "conffile=%s md5sum=%s modified=%d\n",
				 cf->name, cf->value, modified);
	       }
	  }
     }
#ifndef OPKG_LIB
     if (buff)
	  free(buff);
#endif
     pkg_vec_free(available);

     return 0;
}

static int opkg_info_cmd(opkg_conf_t *conf, int argc, char **argv)
{
     return opkg_info_status_cmd(conf, argc, argv, 0);
}

static int opkg_status_cmd(opkg_conf_t *conf, int argc, char **argv)
{
     return opkg_info_status_cmd(conf, argc, argv, 1);
}

static int opkg_configure_cmd(opkg_conf_t *conf, int argc, char **argv)
{
     
     int err;
     if (argc > 0) {
	  char *pkg_name = NULL;

	  pkg_name = argv[0];

	  err = opkg_configure_packages (conf, pkg_name);
     
     } else {
	  err = opkg_configure_packages (conf, NULL);
     }

     write_status_files_if_changed(conf);

     return err;
}

static int opkg_install_pending_cmd(opkg_conf_t *conf, int argc, char **argv)
{
     int i, err;
     char *globpattern;
     glob_t globbuf;
    
     sprintf_alloc(&globpattern, "%s/*" OPKG_PKG_EXTENSION, conf->pending_dir);
     err = glob(globpattern, 0, NULL, &globbuf);
     free(globpattern);
     if (err) {
	  return 0;
     }

     opkg_message(conf, OPKG_NOTICE,
		  "The following packages in %s will now be installed.\n",
		  conf->pending_dir);
     for (i = 0; i < globbuf.gl_pathc; i++) {
	  opkg_message(conf, OPKG_NOTICE,
		       "%s%s", i == 0 ? "" : " ", globbuf.gl_pathv[i]);
     }
     opkg_message(conf, OPKG_NOTICE, "\n");
     for (i = 0; i < globbuf.gl_pathc; i++) {
	  err = opkg_install_from_file(conf, globbuf.gl_pathv[i]);
	  if (err == 0) {
	       err = unlink(globbuf.gl_pathv[i]);
	       if (err) {
		    opkg_message(conf, OPKG_ERROR,
				 "%s: ERROR: failed to unlink %s: %s\n",
				 __FUNCTION__, globbuf.gl_pathv[i], strerror(err));
		    return err;
	       }
	  }
     }
     globfree(&globbuf);

     return err;
}

static int opkg_remove_cmd(opkg_conf_t *conf, int argc, char **argv)
{
     int i,a,done;
     pkg_t *pkg;
     pkg_t *pkg_to_remove;
     pkg_vec_t *available;
     char *pkg_name = NULL;
     global_conf = conf;
     signal(SIGINT, sigint_handler);

// ENH: Add the "no pkg removed" just in case.

    done = 0;

     available = pkg_vec_alloc();
     pkg_info_preinstall_check(conf);
     if ( argc > 0 ) {
        pkg_hash_fetch_all_installed(&conf->pkg_hash, available);
        for (i=0; i < argc; i++) {
           pkg_name = malloc(strlen(argv[i])+2);
           strcpy(pkg_name,argv[i]);
           for (a=0; a < available->len; a++) {
               pkg = available->pkgs[a];
	       if (pkg_name && fnmatch(pkg_name, pkg->name, 0)) {
                  continue;
               }
               if (conf->restrict_to_default_dest) {
	            pkg_to_remove = pkg_hash_fetch_installed_by_name_dest(&conf->pkg_hash,
							        pkg->name,
							        conf->default_dest);
               } else {
	            pkg_to_remove = pkg_hash_fetch_installed_by_name(&conf->pkg_hash, pkg->name );
               }
        
               if (pkg == NULL) {
	            opkg_message(conf, OPKG_ERROR, "Package %s is not installed.\n", pkg->name);
	            continue;
               }
               if (pkg->state_status == SS_NOT_INSTALLED) {    // Added the control, so every already removed package could be skipped
	            opkg_message(conf, OPKG_ERROR, "Package seems to be %s not installed (STATUS = NOT_INSTALLED).\n", pkg->name);
                    continue;
               }
               opkg_remove_pkg(conf, pkg_to_remove,0);
               done = 1;
           }
           free (pkg_name);
        }
        pkg_vec_free(available);
     } else {
	  pkg_vec_t *installed_pkgs = pkg_vec_alloc();
	  int i;
	  int flagged_pkg_count = 0;
	  int removed;

	  pkg_hash_fetch_all_installed(&conf->pkg_hash, installed_pkgs);

	  for (i = 0; i < installed_pkgs->len; i++) {
	       pkg_t *pkg = installed_pkgs->pkgs[i];
	       if (pkg->state_flag & SF_USER) {
		    flagged_pkg_count++;
	       } else {
		    if (!pkg_has_installed_dependents(conf, pkg->parent, pkg, NULL))
			 opkg_message(conf, OPKG_NOTICE, "Non-user leaf package: %s\n", pkg->name);
	       }
	  }
	  if (!flagged_pkg_count) {
	       opkg_message(conf, OPKG_NOTICE, "No packages flagged as installed by user, \n"
			    "so refusing to uninstall unflagged non-leaf packages\n");
	       return 0;
	  }

	  /* find packages not flagged SF_USER (i.e., installed to
	   * satisfy a dependence) and not having any dependents, and
	   * remove them */
	  do {
	       removed = 0;
	       for (i = 0; i < installed_pkgs->len; i++) {
		    pkg_t *pkg = installed_pkgs->pkgs[i];
		    if (!(pkg->state_flag & SF_USER)
			&& !pkg_has_installed_dependents(conf, pkg->parent, pkg, NULL)) {
			 removed++;
			 opkg_message(conf, OPKG_NOTICE, "Removing non-user leaf package %s\n");
			 opkg_remove_pkg(conf, pkg,0);
                         done = 1;
		    }
	       }
	  } while (removed);
	  pkg_vec_free(installed_pkgs);
     }

     if ( done == 0 ) 
        opkg_message(conf, OPKG_NOTICE, "No packages removed.\n");

     write_status_files_if_changed(conf);
     return 0;
}

static int opkg_purge_cmd(opkg_conf_t *conf, int argc, char **argv)
{
     int i;
     pkg_t *pkg;

     global_conf = conf;
     signal(SIGINT, sigint_handler);

     pkg_info_preinstall_check(conf);

     for (i=0; i < argc; i++) {
	  if (conf->restrict_to_default_dest) {
	       pkg = pkg_hash_fetch_installed_by_name_dest(&conf->pkg_hash,
							   argv[i],
							   conf->default_dest);
	  } else {
	       pkg = pkg_hash_fetch_installed_by_name(&conf->pkg_hash, argv[i]);
	  }

	  if (pkg == NULL) {
	       opkg_message(conf, OPKG_ERROR,
			    "Package %s is not installed.\n", argv[i]);
	       continue;
	  }
	  opkg_purge_pkg(conf, pkg);
     }

     write_status_files_if_changed(conf);
     return 0;
}

static int opkg_flag_cmd(opkg_conf_t *conf, int argc, char **argv)
{
     int i;
     pkg_t *pkg;
     const char *flags = argv[0];
    
     global_conf = conf;
     signal(SIGINT, sigint_handler);

     for (i=1; i < argc; i++) {
	  if (conf->restrict_to_default_dest) {
	       pkg = pkg_hash_fetch_installed_by_name_dest(&conf->pkg_hash,
							   argv[i],
							   conf->default_dest);
	  } else {
	       pkg = pkg_hash_fetch_installed_by_name(&conf->pkg_hash, argv[i]);
	  }

	  if (pkg == NULL) {
	       opkg_message(conf, OPKG_ERROR,
			    "Package %s is not installed.\n", argv[i]);
	       continue;
	  }
          if (( strcmp(flags,"hold")==0)||( strcmp(flags,"noprune")==0)||
              ( strcmp(flags,"user")==0)||( strcmp(flags,"ok")==0)) {
	      pkg->state_flag = pkg_state_flag_from_str(flags);
          }
/* pb_ asked this feature 03292004 */
/* Actually I will use only this two, but this is an open for various status */
          if (( strcmp(flags,"installed")==0)||( strcmp(flags,"unpacked")==0)){
	      pkg->state_status = pkg_state_status_from_str(flags);
          }
	  opkg_state_changed++;
	  opkg_message(conf, OPKG_NOTICE,
		       "Setting flags for package %s to %s\n",
		       pkg->name, flags);
     }

     write_status_files_if_changed(conf);
     return 0;
}

static int opkg_files_cmd(opkg_conf_t *conf, int argc, char **argv)
{
     pkg_t *pkg;
     str_list_t *installed_files;
     str_list_elt_t *iter;
     char *pkg_version;
     size_t buff_len = 8192;
     size_t used_len;
     char *buff ;

     buff = (char *)malloc(buff_len);
     if ( buff == NULL ) {
        fprintf( stderr,"%s: Unable to allocate memory \n",__FUNCTION__);
        return ENOMEM;
     }
 
     if (argc < 1) {
	  return EINVAL;
     }

     pkg = pkg_hash_fetch_installed_by_name(&conf->pkg_hash,
					    argv[0]);
     if (pkg == NULL) {
	  opkg_message(conf, OPKG_ERROR,
		       "Package %s not installed.\n", argv[0]);
	  return 0;
     }

     installed_files = pkg_get_installed_files(pkg);
     pkg_version = pkg_version_str_alloc(pkg);

#ifndef OPKG_LIB
     printf("Package %s (%s) is installed on %s and has the following files:\n",
	    pkg->name, pkg_version, pkg->dest->name);
     for (iter = installed_files->head; iter; iter = iter->next) {
	  puts(iter->data);
     }
#else
     if (buff) {
     try_again:
	  used_len = snprintf(buff, buff_len, "Package %s (%s) is installed on %s and has the following files:\n",
			      pkg->name, pkg_version, pkg->dest->name) + 1;
	  if (used_len > buff_len) {
	       buff_len *= 2;
	       buff = realloc (buff, buff_len);
	       goto try_again;
	  }
	  for (iter = installed_files->head; iter; iter = iter->next) {
	       used_len += strlen (iter->data) + 1;
	       while (buff_len <= used_len) {
		    buff_len *= 2;
		    buff = realloc (buff, buff_len);
	       }
	       strncat(buff, iter->data, buff_len);
	       strncat(buff, "\n", buff_len);
	  } 
	  if (opkg_cb_list) opkg_cb_list(pkg->name,
					 buff,
					 pkg_version_str_alloc(pkg),
					 pkg->state_status,
					 p_userdata);
	  free(buff);
     }
#endif

     free(pkg_version);
     pkg_free_installed_files(pkg);

     return 0;
}

static int opkg_depends_cmd(opkg_conf_t *conf, int argc, char **argv)
{

     if (argc > 0) {
	  pkg_vec_t *available_pkgs = pkg_vec_alloc();
	  const char *rel_str = "depends on";
	  int i;
     
	  pkg_info_preinstall_check(conf);

	  if (conf->query_all)
	       pkg_hash_fetch_available(&conf->pkg_hash, available_pkgs);
	  else
	       pkg_hash_fetch_all_installed(&conf->pkg_hash, available_pkgs);
	  for (i = 0; i < argc; i++) {
	       const char *target = argv[i];
	       int j;

	       opkg_message(conf, OPKG_ERROR, "target=%s\n", target);

	       for (j = 0; j < available_pkgs->len; j++) {
		    pkg_t *pkg = available_pkgs->pkgs[j];
		    if (fnmatch(target, pkg->name, 0) == 0) {
			 int k;
			 int count = pkg->depends_count + pkg->pre_depends_count;
			 opkg_message(conf, OPKG_ERROR, "What %s (arch=%s) %s\n",
				      target, pkg->architecture, rel_str);
			 for (k = 0; k < count; k++) {
			      compound_depend_t *cdepend = &pkg->depends[k];
			      int l;
			      for (l = 0; l < cdepend->possibility_count; l++) {
				   depend_t *possibility = cdepend->possibilities[l];
				   opkg_message(conf, OPKG_ERROR, "    %s", possibility->pkg->name);
				   if (conf->verbosity > 0) {
					// char *ver = abstract_pkg_version_str_alloc(possibility->pkg); 
					opkg_message(conf, OPKG_NOTICE, " %s", possibility->version);
					if (possibility->version) {
					     char *typestr = NULL;
					     switch (possibility->constraint) {
					     case NONE: typestr = "none"; break;
					     case EARLIER: typestr = "<"; break;
					     case EARLIER_EQUAL: typestr = "<="; break;
					     case EQUAL: typestr = "="; break;
					     case LATER_EQUAL: typestr = ">="; break;
					     case LATER: typestr = ">"; break;
					     }
					     opkg_message(conf, OPKG_NOTICE, " (%s %s)", typestr, possibility->version);
					}
					// free(ver);
				   }
				   opkg_message(conf, OPKG_ERROR, "\n");
			      }
			 }
		    }
	       }
	  }
	  pkg_vec_free(available_pkgs);
     }
     return 0;
}

enum what_field_type {
  WHATDEPENDS,
  WHATCONFLICTS,
  WHATPROVIDES,
  WHATREPLACES,
  WHATRECOMMENDS,
  WHATSUGGESTS
};

static int opkg_what_depends_conflicts_cmd(opkg_conf_t *conf, enum what_field_type what_field_type, int recursive, int argc, char **argv)
{

     if (argc > 0) {
	  pkg_vec_t *available_pkgs = pkg_vec_alloc();
	  const char *rel_str = NULL;
	  int i;
	  int changed;

	  switch (what_field_type) {
	  case WHATDEPENDS: rel_str = "depends on"; break;
	  case WHATCONFLICTS: rel_str = "conflicts with"; break;
	  case WHATSUGGESTS: rel_str = "suggests"; break;
	  case WHATRECOMMENDS: rel_str = "recommends"; break;
	  case WHATPROVIDES: rel_str = "provides"; break;
	  case WHATREPLACES: rel_str = "replaces"; break;
	  }
     
	  if (conf->query_all)
	       pkg_hash_fetch_available(&conf->pkg_hash, available_pkgs);
	  else
	       pkg_hash_fetch_all_installed(&conf->pkg_hash, available_pkgs);

	  /* mark the root set */
	  pkg_vec_clear_marks(available_pkgs);
	  opkg_message(conf, OPKG_NOTICE, "Root set:\n");
	  for (i = 0; i < argc; i++) {
	       const char *dependee_pattern = argv[i];
	       pkg_vec_mark_if_matches(available_pkgs, dependee_pattern);
	  }
	  for (i = 0; i < available_pkgs->len; i++) {
	       pkg_t *pkg = available_pkgs->pkgs[i];
	       if (pkg->state_flag & SF_MARKED) {
		    /* mark the parent (abstract) package */
		    pkg_mark_provides(pkg);
		    opkg_message(conf, OPKG_NOTICE, "  %s\n", pkg->name);
	       }
	  }

	  opkg_message(conf, OPKG_NOTICE, "What %s root set\n", rel_str);
	  do {
	       int j;
	       changed = 0;

	       for (j = 0; j < available_pkgs->len; j++) {
		    pkg_t *pkg = available_pkgs->pkgs[j];
		    int k;
		    int count = ((what_field_type == WHATCONFLICTS)
				 ? pkg->conflicts_count
				 : pkg->pre_depends_count + pkg->depends_count + pkg->recommends_count + pkg->suggests_count);
		    /* skip this package if it is already marked */
		    if (pkg->parent->state_flag & SF_MARKED) {
			 continue;
		    }
		    for (k = 0; k < count; k++) {
			 compound_depend_t *cdepend = 
			      (what_field_type == WHATCONFLICTS) ? &pkg->conflicts[k] : &pkg->depends[k];
			 int l;
			 for (l = 0; l < cdepend->possibility_count; l++) {
			      depend_t *possibility = cdepend->possibilities[l];
			      if (possibility->pkg->state_flag & SF_MARKED) {
				   /* mark the depending package so we won't visit it again */
				   pkg->state_flag |= SF_MARKED;
				   pkg_mark_provides(pkg);
				   changed++;

				   opkg_message(conf, OPKG_NOTICE, "    %s", pkg->name);
				   if (conf->verbosity > 0) {
					char *ver = pkg_version_str_alloc(pkg); 
					opkg_message(conf, OPKG_NOTICE, " %s", ver);
					opkg_message(conf, OPKG_NOTICE, "\t%s %s", rel_str, possibility->pkg->name);
					if (possibility->version) {
					     char *typestr = NULL;
					     switch (possibility->constraint) {
					     case NONE: typestr = "none"; break;
					     case EARLIER: typestr = "<"; break;
					     case EARLIER_EQUAL: typestr = "<="; break;
					     case EQUAL: typestr = "="; break;
					     case LATER_EQUAL: typestr = ">="; break;
					     case LATER: typestr = ">"; break;
					     }
					     opkg_message(conf, OPKG_NOTICE, " (%s %s)", typestr, possibility->version);
					}
					free(ver);
					if (!pkg_dependence_satisfiable(conf, possibility))
					     opkg_message(conf, OPKG_NOTICE, " unsatisfiable");
				   }
				   opkg_message(conf, OPKG_NOTICE, "\n");
				   goto next_package;
			      }
			 }
		    }
	       next_package:
		    ;
	       }
	  } while (changed && recursive);
	  pkg_vec_free(available_pkgs);
     }

     return 0;
}

int pkg_mark_provides(pkg_t *pkg)
{
     int provides_count = pkg->provides_count;
     abstract_pkg_t **provides = pkg->provides;
     int i;
     pkg->parent->state_flag |= SF_MARKED;
     for (i = 0; i < provides_count; i++) {
	  provides[i]->state_flag |= SF_MARKED;
     }
     return 0;
}

static int opkg_whatdepends_recursively_cmd(opkg_conf_t *conf, int argc, char **argv)
{
     return opkg_what_depends_conflicts_cmd(conf, WHATDEPENDS, 1, argc, argv);
}
static int opkg_whatdepends_cmd(opkg_conf_t *conf, int argc, char **argv)
{
     return opkg_what_depends_conflicts_cmd(conf, WHATDEPENDS, 0, argc, argv);
}

static int opkg_whatsuggests_cmd(opkg_conf_t *conf, int argc, char **argv)
{
     return opkg_what_depends_conflicts_cmd(conf, WHATSUGGESTS, 0, argc, argv);
}

static int opkg_whatrecommends_cmd(opkg_conf_t *conf, int argc, char **argv)
{
     return opkg_what_depends_conflicts_cmd(conf, WHATRECOMMENDS, 0, argc, argv);
}

static int opkg_whatconflicts_cmd(opkg_conf_t *conf, int argc, char **argv)
{
     return opkg_what_depends_conflicts_cmd(conf, WHATCONFLICTS, 0, argc, argv);
}

static int opkg_what_provides_replaces_cmd(opkg_conf_t *conf, enum what_field_type what_field_type, int argc, char **argv)
{

     if (argc > 0) {
	  pkg_vec_t *available_pkgs = pkg_vec_alloc();
	  const char *rel_str = (what_field_type == WHATPROVIDES ? "provides" : "replaces");
	  int i;
     
	  pkg_info_preinstall_check(conf);

	  if (conf->query_all)
	       pkg_hash_fetch_available(&conf->pkg_hash, available_pkgs);
	  else
	       pkg_hash_fetch_all_installed(&conf->pkg_hash, available_pkgs);
	  for (i = 0; i < argc; i++) {
	       const char *target = argv[i];
	       int j;

	       opkg_message(conf, OPKG_ERROR, "What %s %s\n",
			    rel_str, target);
	       for (j = 0; j < available_pkgs->len; j++) {
		    pkg_t *pkg = available_pkgs->pkgs[j];
		    int k;
		    int count = (what_field_type == WHATPROVIDES) ? pkg->provides_count : pkg->replaces_count;
		    for (k = 0; k < count; k++) {
			 abstract_pkg_t *apkg = 
			      ((what_field_type == WHATPROVIDES) 
			       ? pkg->provides[k]
			       : pkg->replaces[k]);
			 if (fnmatch(target, apkg->name, 0) == 0) {
			      opkg_message(conf, OPKG_ERROR, "    %s", pkg->name);
			      if (strcmp(target, apkg->name) != 0)
				   opkg_message(conf, OPKG_ERROR, "\t%s %s\n", rel_str, apkg->name);
			      opkg_message(conf, OPKG_ERROR, "\n");
			 }
		    }
	       }
	  }
	  pkg_vec_free(available_pkgs);
     }
     return 0;
}

static int opkg_whatprovides_cmd(opkg_conf_t *conf, int argc, char **argv)
{
     return opkg_what_provides_replaces_cmd(conf, WHATPROVIDES, argc, argv);
}

static int opkg_whatreplaces_cmd(opkg_conf_t *conf, int argc, char **argv)
{
     return opkg_what_provides_replaces_cmd(conf, WHATREPLACES, argc, argv);
}

static int opkg_search_cmd(opkg_conf_t *conf, int argc, char **argv)
{
     int i;

     pkg_vec_t *installed;
     pkg_t *pkg;
     str_list_t *installed_files;
     str_list_elt_t *iter;
     char *installed_file;

     if (argc < 1) {
	  return EINVAL;
     }
 
     installed = pkg_vec_alloc();
     pkg_hash_fetch_all_installed(&conf->pkg_hash, installed);

     for (i=0; i < installed->len; i++) {
	  pkg = installed->pkgs[i];

	  installed_files = pkg_get_installed_files(pkg);

	  for (iter = installed_files->head; iter; iter = iter->next) {
	       installed_file = iter->data;
	       if (fnmatch(argv[0], installed_file, 0)==0)  {
#ifndef OPKG_LIB
		    printf("%s: %s\n", pkg->name, installed_file);
#else
			if (opkg_cb_list) opkg_cb_list(pkg->name, 
						       installed_file, 
			                               pkg_version_str_alloc(pkg), 
			                               pkg->state_status, p_userdata);
#endif			   
	       }		
	  }

	  pkg_free_installed_files(pkg);
     }

     /* XXX: CLEANUP: It's not obvious from the name of
	pkg_hash_fetch_all_installed that we need to call
	pkg_vec_free to avoid a memory leak. */
     pkg_vec_free(installed);

     return 0;
}

static int opkg_compare_versions_cmd(opkg_conf_t *conf, int argc, char **argv)
{
     if (argc == 3) {
	  /* this is a bit gross */
	  struct pkg p1, p2;
	  parseVersion(&p1, argv[0]); 
	  parseVersion(&p2, argv[2]); 
	  return pkg_version_satisfied(&p1, &p2, argv[1]);
     } else {
	  opkg_message(conf, OPKG_ERROR,
		       "opkg compare_versions <v1> <op> <v2>\n"
		       "<op> is one of <= >= << >> =\n");
	  return -1;
     }
}

#ifndef HOST_CPU_STR
#define HOST_CPU_STR__(X) #X
#define HOST_CPU_STR_(X) HOST_CPU_STR__(X)
#define HOST_CPU_STR HOST_CPU_STR_(HOST_CPU_FOO)
#endif

static int opkg_print_architecture_cmd(opkg_conf_t *conf, int argc, char **argv)
{
     nv_pair_list_elt_t *l;

     l = conf->arch_list.head;
     while (l) {
	  nv_pair_t *nv = l->data;
	  printf("arch %s %s\n", nv->name, nv->value);
	  l = l->next;
     }
     return 0;
}


