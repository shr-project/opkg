/* opkg_conf.c - the opkg package management system

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

#include "includes.h"
#include "opkg_conf.h"
#include "opkg_error.h"

#include "xregex.h"
#include "sprintf_alloc.h"
#include "args.h"
#include "opkg_message.h"
#include "file_util.h"
#include "opkg_defines.h"
#include "libbb/libbb.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <glob.h>

static int opkg_conf_parse_file(opkg_conf_t *conf, const char *filename,
				pkg_src_list_t *pkg_src_list,
				nv_pair_list_t *tmp_dest_nv_pair_list);
static int opkg_conf_set_option(const opkg_option_t *options,
				const char *name, const char *value);
static int opkg_conf_set_default_dest(opkg_conf_t *conf,
				      const char *default_dest_name);
static int set_and_load_pkg_src_list(opkg_conf_t *conf,
				     pkg_src_list_t *nv_pair_list);
static int set_and_load_pkg_dest_list(opkg_conf_t *conf,
				      nv_pair_list_t *nv_pair_list);

void opkg_init_options_array(const opkg_conf_t *conf, opkg_option_t **options)
{
     opkg_option_t tmp[] = {
	  { "cache", OPKG_OPT_TYPE_STRING, &conf->cache},
	  { "force_defaults", OPKG_OPT_TYPE_BOOL, &conf->force_defaults },
          { "force_maintainer", OPKG_OPT_TYPE_BOOL, &conf->force_maintainer }, 
	  { "force_depends", OPKG_OPT_TYPE_BOOL, &conf->force_depends },
	  { "force_overwrite", OPKG_OPT_TYPE_BOOL, &conf->force_overwrite },
	  { "force_downgrade", OPKG_OPT_TYPE_BOOL, &conf->force_downgrade },
	  { "force_reinstall", OPKG_OPT_TYPE_BOOL, &conf->force_reinstall },
	  { "force_space", OPKG_OPT_TYPE_BOOL, &conf->force_space },
          { "check_signature", OPKG_OPT_TYPE_INT, &conf->check_signature }, 
	  { "ftp_proxy", OPKG_OPT_TYPE_STRING, &conf->ftp_proxy },
	  { "http_proxy", OPKG_OPT_TYPE_STRING, &conf->http_proxy },
	  { "no_proxy", OPKG_OPT_TYPE_STRING, &conf->no_proxy },
	  { "test", OPKG_OPT_TYPE_INT, &conf->noaction },
	  { "noaction", OPKG_OPT_TYPE_INT, &conf->noaction },
	  { "nodeps", OPKG_OPT_TYPE_BOOL, &conf->nodeps },
	  { "offline_root", OPKG_OPT_TYPE_STRING, &conf->offline_root },
	  { "offline_root_path", OPKG_OPT_TYPE_STRING, &conf->offline_root_path },
	  { "offline_root_post_script_cmd", OPKG_OPT_TYPE_STRING, &conf->offline_root_post_script_cmd },
	  { "offline_root_pre_script_cmd", OPKG_OPT_TYPE_STRING, &conf->offline_root_pre_script_cmd },
	  { "proxy_passwd", OPKG_OPT_TYPE_STRING, &conf->proxy_passwd },
	  { "proxy_user", OPKG_OPT_TYPE_STRING, &conf->proxy_user },
	  { "query-all", OPKG_OPT_TYPE_BOOL, &conf->query_all },
	  { "tmp_dir", OPKG_OPT_TYPE_STRING, &conf->tmp_dir },
	  { "verbosity", OPKG_OPT_TYPE_BOOL, &conf->verbosity },
#if defined(HAVE_OPENSSL)
	  { "signature_ca_file", OPKG_OPT_TYPE_STRING, &conf->signature_ca_file },
	  { "signature_ca_path", OPKG_OPT_TYPE_STRING, &conf->signature_ca_path },
#endif
#if defined(HAVE_PATHFINDER)
          { "check_x509_path", OPKG_OPT_TYPE_INT, &conf->check_x509_path }, 
#endif
#if defined(HAVE_SSLCURL) && defined(HAVE_CURL)
          { "ssl_engine", OPKG_OPT_TYPE_STRING, &conf->ssl_engine },
          { "ssl_cert", OPKG_OPT_TYPE_STRING, &conf->ssl_cert },
          { "ssl_cert_type", OPKG_OPT_TYPE_STRING, &conf->ssl_cert_type },
          { "ssl_key", OPKG_OPT_TYPE_STRING, &conf->ssl_key },
          { "ssl_key_type", OPKG_OPT_TYPE_STRING, &conf->ssl_key_type },
          { "ssl_key_passwd", OPKG_OPT_TYPE_STRING, &conf->ssl_key_passwd },
          { "ssl_ca_file", OPKG_OPT_TYPE_STRING, &conf->ssl_ca_file },
          { "ssl_ca_path", OPKG_OPT_TYPE_STRING, &conf->ssl_ca_path },
          { "ssl_dont_verify_peer", OPKG_OPT_TYPE_BOOL, &conf->ssl_dont_verify_peer },
#endif
	  { NULL }
     };

     *options = xcalloc(1, sizeof(tmp));
     memcpy(*options, tmp, sizeof(tmp));
};

static void opkg_conf_override_string(char **conf_str, char *arg_str) 
{
     if (arg_str) {
	  if (*conf_str) {
	       free(*conf_str);
	  }
	  *conf_str = xstrdup(arg_str);
     }
}

static void opkg_conf_free_string(char **conf_str)
{
     if (*conf_str) {
	  free(*conf_str);
	  *conf_str = NULL;
     }
}

int opkg_conf_init(opkg_conf_t *conf, const args_t *args)
{
     int err;
     int errno_copy;
     char *tmp_dir_base, *tmp2;
     nv_pair_list_t tmp_dest_nv_pair_list;
     char *lock_file = NULL;
     glob_t globbuf;
     char *etc_opkg_conf_pattern;
     char *offline_root = NULL;

     memset(conf, 0, sizeof(opkg_conf_t));

#if defined(HAVE_PATHFINDER)
     conf->check_x509_path = 1;
#endif

     pkg_src_list_init(&conf->pkg_src_list);

     nv_pair_list_init(&tmp_dest_nv_pair_list);
     pkg_dest_list_init(&conf->pkg_dest_list);

     nv_pair_list_init(&conf->arch_list);

     conf->restrict_to_default_dest = 0;
     conf->default_dest = NULL;

     if (args->conf_file) {
	  struct stat stat_buf;
	  err = stat(args->conf_file, &stat_buf);
	  if (err == 0)
	       if (opkg_conf_parse_file(conf, args->conf_file,
				    &conf->pkg_src_list, &tmp_dest_nv_pair_list)<0) {
                   /* Memory leakage from opkg_conf_parse-file */
                   return OPKG_CONF_ERR_PARSE;
               }
     }

     opkg_conf_override_string(&conf->offline_root, args->offline_root);
     offline_root = conf->offline_root;

     if (conf->offline_root)
	  sprintf_alloc(&etc_opkg_conf_pattern, "%s/etc/opkg/*.conf", conf->offline_root);
     else {
	  char *conf_file_dir = getenv("OPKG_CONF_DIR");
	  if (conf_file_dir == NULL)
		  conf_file_dir = ARGS_DEFAULT_CONF_FILE_DIR;
	  sprintf_alloc(&etc_opkg_conf_pattern, "%s/*.conf", conf_file_dir);
     }
     memset(&globbuf, 0, sizeof(globbuf));
     err = glob(etc_opkg_conf_pattern, 0, NULL, &globbuf);
     free (etc_opkg_conf_pattern);
     if (!err) {
	  int i;
	  for (i = 0; i < globbuf.gl_pathc; i++) {
	       if (globbuf.gl_pathv[i]) 
		    if (args->conf_file &&
				!strcmp(args->conf_file, globbuf.gl_pathv[i]))
			    continue;
		    if ( opkg_conf_parse_file(conf, globbuf.gl_pathv[i], 
				         &conf->pkg_src_list, &tmp_dest_nv_pair_list)<0) {
                        /* Memory leakage from opkg_conf_parse-file */
                        return OPKG_CONF_ERR_PARSE;
	            }
                    if (offline_root != conf->offline_root) {
                        opkg_message(conf, OPKG_ERROR,
					"Config file %s, within an offline "
					"root contains option offline_root.\n",
				       globbuf.gl_pathv[i]);
                        return OPKG_CONF_ERR_PARSE;
                    }
	  }
     }
     globfree(&globbuf);

     opkg_conf_override_string(&conf->offline_root_path, 
			       args->offline_root_path);
     opkg_conf_override_string(&conf->offline_root_pre_script_cmd, 
			       args->offline_root_pre_script_cmd);
     opkg_conf_override_string(&conf->offline_root_post_script_cmd, 
			       args->offline_root_post_script_cmd);

     opkg_conf_override_string(&conf->cache, args->cache);
     opkg_conf_override_string(&conf->tmp_dir, args->tmp_dir);

     /* check for lock file */
     if (conf->offline_root)
       sprintf_alloc (&lock_file, "%s/%s/lock", conf->offline_root, OPKG_STATE_DIR_PREFIX);
     else
       sprintf_alloc (&lock_file, "%s/lock", OPKG_STATE_DIR_PREFIX);

     err = conf->lock_fd = creat (lock_file, S_IRUSR | S_IWUSR | S_IRGRP);
     if (err != -1)
       err = lockf (conf->lock_fd, F_TLOCK, 0);
     errno_copy = errno;

     if (err) {
       opkg_message (conf, OPKG_ERROR, "Could not lock %s: %s\n",
                 lock_file, strerror(errno_copy));
       free(lock_file);
       return OPKG_CONF_ERR_LOCK;
     }
     free(lock_file);

     if (conf->tmp_dir)
	  tmp_dir_base = conf->tmp_dir;
     else 
	  tmp_dir_base = getenv("TMPDIR");
     sprintf_alloc(&tmp2, "%s/%s",
		   tmp_dir_base ? tmp_dir_base : OPKG_CONF_DEFAULT_TMP_DIR_BASE,
		   OPKG_CONF_TMP_DIR_SUFFIX);
     if (conf->tmp_dir)
	     free(conf->tmp_dir);
     conf->tmp_dir = mkdtemp(tmp2);
     if (conf->tmp_dir == NULL) {
	  opkg_message(conf, OPKG_ERROR,
			  "%s: Creating temp dir %s failed: %s\n",
			  __FUNCTION__, tmp2, strerror(errno));
	  return OPKG_CONF_ERR_TMP_DIR;
     }

     pkg_hash_init("pkg-hash", &conf->pkg_hash, OPKG_CONF_DEFAULT_HASH_LEN);
     hash_table_init("file-hash", &conf->file_hash, OPKG_CONF_DEFAULT_HASH_LEN);
     hash_table_init("obs-file-hash", &conf->obs_file_hash, OPKG_CONF_DEFAULT_HASH_LEN/16);

     if (conf->lists_dir == NULL)
        conf->lists_dir = xstrdup(OPKG_CONF_LISTS_DIR);

     if (conf->offline_root) {
            char *tmp;
            sprintf_alloc(&tmp, "%s/%s", conf->offline_root, conf->lists_dir);
            free(conf->lists_dir);
            conf->lists_dir = tmp;
     }

     /* if no architectures were defined, then default all, noarch, and host architecture */
     if (nv_pair_list_empty(&conf->arch_list)) {
	  nv_pair_list_append(&conf->arch_list, "all", "1");
	  nv_pair_list_append(&conf->arch_list, "noarch", "1");
	  nv_pair_list_append(&conf->arch_list, HOST_CPU_STR, "10");
     }

     /* Even if there is no conf file, we'll need at least one dest. */
     if (nv_pair_list_empty(&tmp_dest_nv_pair_list)) {
	  nv_pair_list_append(&tmp_dest_nv_pair_list,
			      OPKG_CONF_DEFAULT_DEST_NAME,
			      OPKG_CONF_DEFAULT_DEST_ROOT_DIR);
     }

     /* After parsing the file, set options from command-line, (so that
	command-line arguments take precedence) */
     /* XXX: CLEANUP: The interaction between args.c and opkg_conf.c
	really needs to be cleaned up. There is so much duplication
	right now it is ridiculous. Maybe opkg_conf_t should just save
	a pointer to args_t (which could then not be freed), rather
	than duplicating every field here? */
     if (args->autoremove) {
	  conf->autoremove = 1;
     }
     if (args->force_depends) {
	  conf->force_depends = 1;
     }
     if (args->force_defaults) {
	  conf->force_defaults = 1;
     }
     if (args->force_maintainer) {
          conf->force_maintainer = 1;
     }
     if (args->force_overwrite) {
	  conf->force_overwrite = 1;
     }
     if (args->force_downgrade) {
	  conf->force_downgrade = 1;
     }
     if (args->force_space) {
	  conf->force_space = 1;
     }
     if (args->force_reinstall) {
	  conf->force_reinstall = 1;
     }
     if (args->force_removal_of_dependent_packages) {
	  conf->force_removal_of_dependent_packages = 1;
     }
     if (args->force_removal_of_essential_packages) {
	  conf->force_removal_of_essential_packages = 1;
     }
     if (args->nodeps) {
	  conf->nodeps = 1;
     }
     if (args->noaction) {
	  conf->noaction = 1;
     }
     if (args->query_all) {
	  conf->query_all = 1;
     }
     if (args->verbosity != conf->verbosity) {
	  conf->verbosity = args->verbosity;
     } 

/* Pigi: added a flag to disable the checking of structures if the command does not need to 
         read anything from there.
*/
     if (!(args->nocheckfordirorfile)) {

        if (!(args->noreadfeedsfile)) {
           if (set_and_load_pkg_src_list(conf, &conf->pkg_src_list)) {
               nv_pair_list_deinit(&tmp_dest_nv_pair_list);
	       return -1;
	   }
	}
   
        /* Now that we have resolved conf->offline_root, we can commit to
	   the directory names for the dests and load in all the package
	   lists. */
        if (set_and_load_pkg_dest_list(conf, &tmp_dest_nv_pair_list)) {
               nv_pair_list_deinit(&tmp_dest_nv_pair_list);
	       return -1;
	}
   
        if (args->dest) {
	     err = opkg_conf_set_default_dest(conf, args->dest);
	     if (err) {
                  nv_pair_list_deinit(&tmp_dest_nv_pair_list);
	          return OPKG_CONF_ERR_DEFAULT_DEST;
	     }
        }
     }
     nv_pair_list_deinit(&tmp_dest_nv_pair_list);

     return 0;
}

void opkg_conf_deinit(opkg_conf_t *conf)
{
     rm_r(conf->tmp_dir);

     free(conf->tmp_dir);
     free(conf->lists_dir);

     pkg_src_list_deinit(&conf->pkg_src_list);
     pkg_dest_list_deinit(&conf->pkg_dest_list);
     nv_pair_list_deinit(&conf->arch_list);

     opkg_conf_free_string(&conf->cache);

     opkg_conf_free_string(&conf->ftp_proxy);
     opkg_conf_free_string(&conf->http_proxy);
     opkg_conf_free_string(&conf->no_proxy);

     opkg_conf_free_string(&conf->offline_root);
     opkg_conf_free_string(&conf->offline_root_path);
     opkg_conf_free_string(&conf->offline_root_pre_script_cmd);
     opkg_conf_free_string(&conf->offline_root_post_script_cmd);

     opkg_conf_free_string(&conf->proxy_passwd);
     opkg_conf_free_string(&conf->proxy_user);

#if defined(HAVE_OPENSSL)
     opkg_conf_free_string(&conf->signature_ca_file);
     opkg_conf_free_string(&conf->signature_ca_path);
#endif

#if defined(HAVE_SSLCURL)
     opkg_conf_free_string(&conf->ssl_engine);
     opkg_conf_free_string(&conf->ssl_cert);
     opkg_conf_free_string(&conf->ssl_cert_type);
     opkg_conf_free_string(&conf->ssl_key);
     opkg_conf_free_string(&conf->ssl_key_type);
     opkg_conf_free_string(&conf->ssl_key_passwd);
     opkg_conf_free_string(&conf->ssl_ca_file);
     opkg_conf_free_string(&conf->ssl_ca_path);
#endif

     if (conf->verbosity >= OPKG_DEBUG) { 
	hash_print_stats(&conf->pkg_hash);
	hash_print_stats(&conf->file_hash);
	hash_print_stats(&conf->obs_file_hash);
     }

     if (&conf->pkg_hash)
	            pkg_hash_deinit(&conf->pkg_hash);
     if (&conf->file_hash)
	            hash_table_deinit(&conf->file_hash);
     if (&conf->obs_file_hash)
	            hash_table_deinit(&conf->obs_file_hash);

     /* lockf maybe defined with warn_unused_result */
     if(lockf(conf->lock_fd, F_ULOCK, 0) != 0){
              opkg_message(conf, OPKG_DEBUG, "%s: unlock failed: %s\n",
                              __FUNCTION__, 
                              strerror(errno));
     }
     close(conf->lock_fd);
}

static int opkg_conf_set_default_dest(opkg_conf_t *conf,
				      const char *default_dest_name)
{
     pkg_dest_list_elt_t *iter;
     pkg_dest_t *dest;

     for (iter = void_list_first(&conf->pkg_dest_list); iter; iter = void_list_next(&conf->pkg_dest_list, iter)) {
	  dest = (pkg_dest_t *)iter->data;
	  if (strcmp(dest->name, default_dest_name) == 0) {
	       conf->default_dest = dest;
	       conf->restrict_to_default_dest = 1;
	       return 0;
	  }
     }

     fprintf(stderr, "ERROR: Unknown dest name: `%s'\n", default_dest_name);

     return 1;
}

static int
set_and_load_pkg_src_list(opkg_conf_t *conf, pkg_src_list_t *pkg_src_list)
{
     pkg_src_list_elt_t *iter;
     pkg_src_t *src;
     char *list_file;

     for (iter = void_list_first(pkg_src_list); iter; iter = void_list_next(pkg_src_list, iter)) {
          src = (pkg_src_t *)iter->data;
	  if (src == NULL) {
	       continue;
	  }

	  sprintf_alloc(&list_file, "%s/%s", 
			  conf->restrict_to_default_dest ? conf->default_dest->lists_dir : conf->lists_dir, 
			  src->name);

	  if (file_exists(list_file)) {
	       if (pkg_hash_add_from_file(conf, list_file, src, NULL, 0)) {
		    free(list_file);
		    return -1;
	       }
	  }
	  free(list_file);
     }

     return 0;
}

static int
set_and_load_pkg_dest_list(opkg_conf_t *conf, nv_pair_list_t *nv_pair_list)
{
     nv_pair_list_elt_t *iter;
     nv_pair_t *nv_pair;
     pkg_dest_t *dest;
     char *root_dir;

     for (iter = nv_pair_list_first(nv_pair_list); iter; iter = nv_pair_list_next(nv_pair_list, iter)) {
	  nv_pair = (nv_pair_t *)iter->data;

	  if (conf->offline_root) {
	       sprintf_alloc(&root_dir, "%s%s", conf->offline_root, nv_pair->value);
	  } else {
	       root_dir = xstrdup(nv_pair->value);
	  }
	  dest = pkg_dest_list_append(&conf->pkg_dest_list, nv_pair->name, root_dir, conf->lists_dir);
	  free(root_dir);
	  if (dest == NULL) {
	       continue;
	  }
	  if (conf->default_dest == NULL) {
	       conf->default_dest = dest;
	  }
	  if (file_exists(dest->status_file_name)) {
	       if (pkg_hash_add_from_file(conf, dest->status_file_name,
				      NULL, dest, 1))
		       return -1;
	  }
     }

     return 0;
}

static int opkg_conf_parse_file(opkg_conf_t *conf, const char *filename,
				pkg_src_list_t *pkg_src_list,
				nv_pair_list_t *tmp_dest_nv_pair_list)
{
     int err;
     opkg_option_t * options;
     FILE *file;
     regex_t valid_line_re, comment_re;
#define regmatch_size 12
     regmatch_t regmatch[regmatch_size];

     opkg_init_options_array(conf, &options);

     file = fopen(filename, "r");
     if (file == NULL) {
	  fprintf(stderr, "%s: failed to open %s: %s\n",
		  __FUNCTION__, filename, strerror(errno));
	  free(options);
	  return -1;
     }
     opkg_message(conf, OPKG_NOTICE, "loading conf file %s\n", filename);

     err = xregcomp(&comment_re, 
		    "^[[:space:]]*(#.*|[[:space:]]*)$",
		    REG_EXTENDED);
     if (err) {
	  free(options);
	  return -1;
     }
     err = xregcomp(&valid_line_re, "^[[:space:]]*(\"([^\"]*)\"|([^[:space:]]*))[[:space:]]*(\"([^\"]*)\"|([^[:space:]]*))[[:space:]]*(\"([^\"]*)\"|([^[:space:]]*))([[:space:]]+([^[:space:]]+))?[[:space:]]*$", REG_EXTENDED);
     if (err) {
	  free(options);
	  return -1;
     }

     while(1) {
	  int line_num = 0;
	  char *line;
	  char *type, *name, *value, *extra;

	  line = file_read_line_alloc(file);
	  line_num++;
	  if (line == NULL) {
	       break;
	  }

	  if (regexec(&comment_re, line, 0, 0, 0) == 0) {
	       goto NEXT_LINE;
	  }

	  if (regexec(&valid_line_re, line, regmatch_size, regmatch, 0) == REG_NOMATCH) {
	       fprintf(stderr, "%s:%d: Ignoring invalid line: `%s'\n",
		       filename, line_num, line);
	       goto NEXT_LINE;
	  }

	  /* This has to be so ugly to deal with optional quotation marks */
	  if (regmatch[2].rm_so > 0) {
	       type = xstrndup(line + regmatch[2].rm_so,
			      regmatch[2].rm_eo - regmatch[2].rm_so);
	  } else {
	       type = xstrndup(line + regmatch[3].rm_so,
			      regmatch[3].rm_eo - regmatch[3].rm_so);
	  }
	  if (regmatch[5].rm_so > 0) {
	       name = xstrndup(line + regmatch[5].rm_so,
			      regmatch[5].rm_eo - regmatch[5].rm_so);
	  } else {
	       name = xstrndup(line + regmatch[6].rm_so,
			      regmatch[6].rm_eo - regmatch[6].rm_so);
	  }
	  if (regmatch[8].rm_so > 0) {
	       value = xstrndup(line + regmatch[8].rm_so,
			       regmatch[8].rm_eo - regmatch[8].rm_so);
	  } else {
	       value = xstrndup(line + regmatch[9].rm_so,
			       regmatch[9].rm_eo - regmatch[9].rm_so);
	  }
	  extra = NULL;
	  if (regmatch[11].rm_so > 0) {
	       extra = xstrndup (line + regmatch[11].rm_so,
				regmatch[11].rm_eo - regmatch[11].rm_so);
	  }

	  /* We use the tmp_dest_nv_pair_list below instead of
	     conf->pkg_dest_list because we might encounter an
	     offline_root option later and that would invalidate the
	     directories we would have computed in
	     pkg_dest_list_init. (We do a similar thing with
	     tmp_src_nv_pair_list for sake of symmetry.) */
	  if (strcmp(type, "option") == 0) {
	       opkg_conf_set_option(options, name, value);
	  } else if (strcmp(type, "src") == 0) {
	       if (!nv_pair_list_find((nv_pair_list_t*) pkg_src_list, name)) {
		    pkg_src_list_append (pkg_src_list, name, value, extra, 0);
	       } else {
		    opkg_message(conf, OPKG_ERROR, "ERROR: duplicate src declaration.  Skipping:\n\t src %s %s\n",
				 name, value);
	       }
	  } else if (strcmp(type, "src/gz") == 0) {
	       if (!nv_pair_list_find((nv_pair_list_t*) pkg_src_list, name)) {
		    pkg_src_list_append (pkg_src_list, name, value, extra, 1);
	       } else {
		    opkg_message(conf, OPKG_ERROR, "ERROR: duplicate src declaration.  Skipping:\n\t src %s %s\n",
				 name, value);
	       }
	  } else if (strcmp(type, "dest") == 0) {
	       nv_pair_list_append(tmp_dest_nv_pair_list, name, value);
	  } else if (strcmp(type, "lists_dir") == 0) {
	       conf->lists_dir = xstrdup(value);
	  } else if (strcmp(type, "arch") == 0) {
	       opkg_message(conf, OPKG_INFO, "supported arch %s priority (%s)\n", name, value);
	       if (!value) {
		    opkg_message(conf, OPKG_NOTICE, "defaulting architecture %s priority to 10\n", name);
		    value = xstrdup("10");
	       }
	       nv_pair_list_append(&conf->arch_list, name, value);
	  } else {
	       fprintf(stderr, "WARNING: Ignoring unknown configuration "
		       "parameter: %s %s %s\n", type, name, value);
	       free(options);
	       return -1;
	  }

	  free(type);
	  free(name);
	  free(value);
	  if (extra)
	       free (extra);

     NEXT_LINE:
	  free(line);
     }

     free(options);
     regfree(&comment_re);
     regfree(&valid_line_re);
     fclose(file);

     return 0;
}

static int opkg_conf_set_option(const opkg_option_t *options,
				const char *name, const char *value)
{
     int i = 0;
     while (options[i].name) {
	  if (strcmp(options[i].name, name) == 0) {
	       switch (options[i].type) {
	       case OPKG_OPT_TYPE_BOOL:
		    *((int *)options[i].value) = 1;
		    return 0;
	       case OPKG_OPT_TYPE_INT:
		    if (value) {
			 *((int *)options[i].value) = atoi(value);
			 return 0;
		    } else {
			 printf("%s: Option %s need an argument\n",
				__FUNCTION__, name);
			 return EINVAL;
		    }		    
	       case OPKG_OPT_TYPE_STRING:
		    if (value) {
			 *((char **)options[i].value) = xstrdup(value);
			 return 0;
		    } else {
			 printf("%s: Option %s need an argument\n",
				__FUNCTION__, name);
			 return EINVAL;
		    }
	       }
	  }
	  i++;
     }
    
     fprintf(stderr, "%s: Unrecognized option: %s=%s\n",
	     __FUNCTION__, name, value);
     return EINVAL;
}

int opkg_conf_write_status_files(opkg_conf_t *conf)
{
     pkg_dest_list_elt_t *iter;
     pkg_dest_t *dest;
     pkg_vec_t *all;
     pkg_t *pkg;
     int i, ret = 0;

     if (conf->noaction)
	  return 0;

     list_for_each_entry(iter, &conf->pkg_dest_list.head, node) {
          dest = (pkg_dest_t *)iter->data;

          dest->status_fp = fopen(dest->status_file_name, "w");
          if (dest->status_fp == NULL) {
               fprintf(stderr, "%s: Can't open status file: %s: %s\n",
                    __FUNCTION__, dest->status_file_name, strerror(errno));
               ret = -1;
          }
     }

     all = pkg_vec_alloc();
     pkg_hash_fetch_available(&conf->pkg_hash, all);

     for(i = 0; i < all->len; i++) {
	  pkg = all->pkgs[i];
	  /* We don't need most uninstalled packages in the status file */
	  if (pkg->state_status == SS_NOT_INSTALLED
	      && (pkg->state_want == SW_UNKNOWN
		  || pkg->state_want == SW_DEINSTALL
		  || pkg->state_want == SW_PURGE)) {
	       continue;
	  }
	  if (pkg->dest == NULL) {
	       fprintf(stderr, "%s: ERROR: Can't write status for "
		       "package %s since it has a NULL dest\n",
		       __FUNCTION__, pkg->name);
	       continue;
	  }
	  if (pkg->dest->status_fp)
	       pkg_print_status(pkg, pkg->dest->status_fp);
     }

     pkg_vec_free(all);

     list_for_each_entry(iter, &conf->pkg_dest_list.head, node) {
          dest = (pkg_dest_t *)iter->data;
          fclose(dest->status_fp);
     }

     return ret;
}


char *root_filename_alloc(opkg_conf_t *conf, char *filename)
{
     char *root_filename;
     sprintf_alloc(&root_filename, "%s%s", (conf->offline_root ? conf->offline_root : ""), filename);
     return root_filename;
}
