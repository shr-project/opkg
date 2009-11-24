/* opkg_conf.h - the opkg package management system

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

#ifndef OPKG_CONF_H
#define OPKG_CONF_H

typedef struct opkg_conf opkg_conf_t;

#include "hash_table.h"
#include "args.h"
#include "pkg.h"
#include "pkg_hash.h"
#include "pkg_src_list.h"
#include "pkg_dest_list.h"
#include "nv_pair_list.h"

#define OPKG_CONF_DEFAULT_TMP_DIR_BASE "/tmp"
#define OPKG_CONF_TMP_DIR_SUFFIX "opkg-XXXXXX"
#define OPKG_CONF_LISTS_DIR  OPKG_STATE_DIR_PREFIX "/lists"

/* In case the config file defines no dest */
#define OPKG_CONF_DEFAULT_DEST_NAME "root"
#define OPKG_CONF_DEFAULT_DEST_ROOT_DIR "/"

#define OPKG_CONF_DEFAULT_HASH_LEN 1024

struct opkg_conf
{
     int lock_fd; /* file descriptor for the lock file */
     pkg_src_list_t pkg_src_list;
     pkg_dest_list_t pkg_dest_list;
     nv_pair_list_t arch_list;

     int restrict_to_default_dest;
     pkg_dest_t *default_dest;

     char *tmp_dir;
     char *lists_dir;

     /* options */
     int autoremove;
     int force_depends;
     int force_defaults;
     int force_maintainer;
     int force_overwrite;
     int force_downgrade;
     int force_reinstall;
     int force_space;
     int force_removal_of_dependent_packages;
     int force_removal_of_essential_packages;
     int check_signature;
     int nodeps; /* do not follow dependences */
     char *offline_root;
     char *offline_root_path;
     char *offline_root_pre_script_cmd;
     char *offline_root_post_script_cmd;
     int query_all;
     int verbosity;
     int noaction;
     char *cache;

#ifdef HAVE_SSLCURL
     /* some options could be used by
      * wget if curl support isn't builtin
      * If someone want to try...
      */
     char *ssl_engine;
     char *ssl_cert;
     char *ssl_cert_type;
     char *ssl_key;
     char *ssl_key_type;
     char *ssl_key_passwd;
     char *ssl_ca_file;
     char *ssl_ca_path;
     int ssl_dont_verify_peer;
#endif
#ifdef HAVE_PATHFINDER
     int check_x509_path;
#endif

     /* proxy options */
     char *http_proxy;
     char *ftp_proxy;
     char *no_proxy;
     char *proxy_user;
     char *proxy_passwd;

     char *signature_ca_file;
     char *signature_ca_path;

     hash_table_t pkg_hash;
     hash_table_t file_hash;
     hash_table_t obs_file_hash;
};

enum opkg_option_type {
     OPKG_OPT_TYPE_BOOL,
     OPKG_OPT_TYPE_INT,
     OPKG_OPT_TYPE_STRING
};
typedef enum opkg_option_type opkg_option_type_t;

typedef struct opkg_option opkg_option_t;
struct opkg_option {
     const char *name;
     const opkg_option_type_t type;
     const void *value;
};

int opkg_conf_init(opkg_conf_t *conf, const args_t *args);
void opkg_conf_deinit(opkg_conf_t *conf);

int opkg_conf_write_status_files(opkg_conf_t *conf);
char *root_filename_alloc(opkg_conf_t *conf, char *filename);


void opkg_init_options_array(const opkg_conf_t *conf, opkg_option_t **options);

#endif
