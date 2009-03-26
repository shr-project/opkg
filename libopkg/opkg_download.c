/* vi: set noexpandtab sw=4 sts=4: */
/* opkg_download.c - the opkg package management system

   Carl D. Worth

   Copyright (C) 2001 University of Southern California
   Copyright (C) 2008 OpenMoko Inc

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/
#include "config.h"
#ifdef HAVE_CURL
#include <curl/curl.h>
#endif
#ifdef HAVE_GPGME
#include <gpgme.h>
#endif

#include "includes.h"
#include "opkg_download.h"
#include "opkg_message.h"
#include "opkg_state.h"

#include "sprintf_alloc.h"
#include "xsystem.h"
#include "file_util.h"
#include "str_util.h"
#include "opkg_defines.h"

int opkg_download(opkg_conf_t *conf, const char *src,
  const char *dest_file_name, curl_progress_func cb, void *data)
{
    int err = 0;

    char *src_basec = strdup(src);
    char *src_base = basename(src_basec);
    char *tmp_file_location;

    opkg_message(conf,OPKG_NOTICE,"Downloading %s\n", src);
	
    if (str_starts_with(src, "file:")) {
	int ret;
	const char *file_src = src + 5;
	opkg_message(conf,OPKG_INFO,"Copying %s to %s...", file_src, dest_file_name);
	ret = file_copy(src + 5, dest_file_name);
	opkg_message(conf,OPKG_INFO,"Done\n");
        free(src_basec);
	return ret;
    }

    sprintf_alloc(&tmp_file_location, "%s/%s", conf->tmp_dir, src_base);
    err = unlink(tmp_file_location);
    if (err && errno != ENOENT) {
	opkg_message(conf,OPKG_ERROR, "%s: ERROR: failed to unlink %s: %s\n",
		__FUNCTION__, tmp_file_location, strerror(errno));
	free(tmp_file_location);
        free(src_basec);
	return errno;
    }

    if (conf->http_proxy) {
	opkg_message(conf,OPKG_DEBUG,"Setting environment variable: http_proxy = %s\n", conf->http_proxy);
	setenv("http_proxy", conf->http_proxy, 1);
    }
    if (conf->ftp_proxy) {
	opkg_message(conf,OPKG_DEBUG,"Setting environment variable: ftp_proxy = %s\n", conf->ftp_proxy);
	setenv("ftp_proxy", conf->ftp_proxy, 1);
    }
    if (conf->no_proxy) {
	opkg_message(conf,OPKG_DEBUG,"Setting environment variable: no_proxy = %s\n", conf->no_proxy);
	setenv("no_proxy", conf->no_proxy, 1);
    }

#ifdef HAVE_CURL
    CURL *curl;
    CURLcode res;
    FILE * file = fopen (tmp_file_location, "w");

    curl = curl_easy_init ();
    if (curl)
    {
	curl_easy_setopt (curl, CURLOPT_URL, src);
	curl_easy_setopt (curl, CURLOPT_WRITEDATA, file);
	curl_easy_setopt (curl, CURLOPT_NOPROGRESS, (cb == NULL));
	if (cb)
	{
		curl_easy_setopt (curl, CURLOPT_PROGRESSDATA, data);
		curl_easy_setopt (curl, CURLOPT_PROGRESSFUNCTION, cb);
	}
	curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt (curl, CURLOPT_FAILONERROR, 1);
	if (conf->http_proxy || conf->ftp_proxy)
	{
	    char *userpwd;
	    sprintf_alloc (&userpwd, "%s:%s", conf->proxy_user,
		    conf->proxy_passwd);
	    curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, userpwd);
	    free (userpwd);
	}
	res = curl_easy_perform (curl);
	fclose (file);
	if (res)
	{
	    long error_code;
	    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &error_code);
	    opkg_message(conf, OPKG_ERROR, "Failed to download %s. \nerror detail: %s\n", src, curl_easy_strerror(res));
	    free(tmp_file_location);
            free(src_basec);
	    curl_easy_cleanup (curl);
	    return res;
	}
	curl_easy_cleanup (curl);

    }
    else
    {
	free(tmp_file_location);
        free(src_basec);
	return -1;
    }
#else
    {
      int res;
      char *wgetcmd;
      char *wgetopts;
      wgetopts = getenv("OPKG_WGETOPTS");
      sprintf_alloc(&wgetcmd, "wget -q %s%s -O \"%s\" \"%s\"",
		    (conf->http_proxy || conf->ftp_proxy) ? "-Y on " : "",
		    (wgetopts!=NULL) ? wgetopts : "",
		    tmp_file_location, src);
      opkg_message(conf, OPKG_INFO, "Executing: %s\n", wgetcmd);
      res = xsystem(wgetcmd);
      free(wgetcmd);
      if (res) {
	opkg_message(conf, OPKG_ERROR, "Failed to download %s, error %d\n", src, res);
	free(tmp_file_location);
        free(src_basec);
	return res;
      }
    }
#endif

    err = file_move(tmp_file_location, dest_file_name);

    free(tmp_file_location);
    free(src_basec);

    if (err) {
	return err;
    }

    return 0;
}

static int opkg_download_cache(opkg_conf_t *conf, const char *src,
  const char *dest_file_name, curl_progress_func cb, void *data)
{
    char *cache_name = strdup(src);
    char *cache_location, *p;
    int err = 0;

    if (!conf->cache || str_starts_with(src, "file:")) {
	err = opkg_download(conf, src, dest_file_name, cb, data);
	goto out1;
    }

    for (p = cache_name; *p; p++)
	if (*p == '/')
	    *p = ',';	/* looks nicer than | or # */

    sprintf_alloc(&cache_location, "%s/%s", conf->cache, cache_name);
    if (file_exists(cache_location))
	opkg_message(conf, OPKG_NOTICE, "Copying %s\n", cache_location);
    else {
	err = opkg_download(conf, src, cache_location, cb, data);
	if (err) {
	    (void) unlink(cache_location);
	    goto out2;
	}
    }

    err = file_copy(cache_location, dest_file_name);


out2:
    free(cache_location);
out1:
    free(cache_name);
    return err;
}

int opkg_download_pkg(opkg_conf_t *conf, pkg_t *pkg, const char *dir)
{
    int err;
    char *url;
    char *pkgid;
    char *stripped_filename;

    if (pkg->src == NULL) {
	opkg_message(conf,OPKG_ERROR, "ERROR: Package %s (parent %s) is not available from any configured src.\n",
		pkg->name, pkg->parent->name);
	return -1;
    }

    sprintf_alloc (&pkgid, "%s;%s;%s;", pkg->name, pkg->version, pkg->architecture);
    opkg_set_current_state (conf, OPKG_STATE_DOWNLOADING_PKG, pkgid);
    free (pkgid);

    sprintf_alloc(&url, "%s/%s", pkg->src->value, pkg->filename);

    /* XXX: BUG: The pkg->filename might be something like
       "../../foo.opk". While this is correct, and exactly what we
       want to use to construct url above, here we actually need to
       use just the filename part, without any directory. */

    stripped_filename = strrchr(pkg->filename, '/');
    if ( ! stripped_filename )
        stripped_filename = pkg->filename;

    sprintf_alloc(&pkg->local_filename, "%s/%s", dir, stripped_filename);

    err = opkg_download_cache(conf, url, pkg->local_filename, NULL, NULL);
    free(url);

    opkg_set_current_state (conf, OPKG_STATE_NONE, NULL);
    return err;
}

/*
 * Downloads file from url, installs in package database, return package name. 
 */
int opkg_prepare_url_for_install(opkg_conf_t *conf, const char *url, char **namep)
{
     int err = 0;
     pkg_t *pkg;
     pkg = pkg_new();
     if (pkg == NULL)
	  return ENOMEM;

     if (str_starts_with(url, "http://")
	 || str_starts_with(url, "ftp://")) {
	  char *tmp_file;
	  char *file_basec = strdup(url);
	  char *file_base = basename(file_basec);

	  sprintf_alloc(&tmp_file, "%s/%s", conf->tmp_dir, file_base);
	  err = opkg_download(conf, url, tmp_file, NULL, NULL);
	  if (err)
	       return err;

	  err = pkg_init_from_file(pkg, tmp_file);
	  if (err)
	       return err;
	  pkg->local_filename = strdup(tmp_file);

	  free(tmp_file);
	  free(file_basec);

     } else if (strcmp(&url[strlen(url) - 4], OPKG_PKG_EXTENSION) == 0
                || strcmp(&url[strlen(url) - 4], IPKG_PKG_EXTENSION) == 0
		|| strcmp(&url[strlen(url) - 4], DPKG_PKG_EXTENSION) == 0) {

	  err = pkg_init_from_file(pkg, url);
	  if (err)
	       return err;
	  pkg->local_filename = strdup(url);
	  opkg_message(conf, OPKG_DEBUG2, "Package %s provided by hand (%s).\n", pkg->name,pkg->local_filename);
          pkg->provided_by_hand = 1;

     } else {
       pkg_deinit(pkg);
       free(pkg);
       return 0;
     }

     if (!pkg->architecture) {
	  opkg_message(conf, OPKG_ERROR, "Package %s has no Architecture defined.\n", pkg->name);
	  return -EINVAL;
     }

     pkg->dest = conf->default_dest;
     pkg->state_want = SW_INSTALL;
     pkg->state_flag |= SF_PREFER;
     pkg = hash_insert_pkg(&conf->pkg_hash, pkg, 1,conf);  
     if ( pkg == NULL ){
        fprintf(stderr, "%s : This should never happen. Report this Bug in bugzilla please \n ",__FUNCTION__);
        return 0;
     }
     if (namep) {
	  *namep = strdup(pkg->name);
     }
     return 0;
}

int
opkg_verify_file (opkg_conf_t *conf, char *text_file, char *sig_file)
{
#ifdef HAVE_GPGME
    if (conf->check_signature == 0 )
        return 0;
    int status = -1;
    gpgme_ctx_t ctx;
    gpgme_data_t sig, text, key;
    gpgme_error_t err = -1;
    gpgme_verify_result_t result;
    gpgme_signature_t s;
    char *trusted_path = NULL;
    
    err = gpgme_new (&ctx);

    if (err)
	return -1;

    sprintf_alloc(&trusted_path, "%s/%s", conf->offline_root, "/etc/opkg/trusted.gpg");
    err = gpgme_data_new_from_file (&key, trusted_path, 1); 
    free (trusted_path);
    if (err)
    {
      return -1;
    }
    err = gpgme_op_import (ctx, key);
    if (err)
    {
      gpgme_data_release (key);
      return -1;
    }
    gpgme_data_release (key);

    err = gpgme_data_new_from_file (&sig, sig_file, 1); 
    if (err)
    {
	gpgme_release (ctx);
	return -1;
    }

    err = gpgme_data_new_from_file (&text, text_file, 1); 
    if (err)
    {
        gpgme_data_release (sig);
	gpgme_release (ctx);
	return -1;
    }

    err = gpgme_op_verify (ctx, sig, text, NULL);

    result = gpgme_op_verify_result (ctx);
    if (!result)
	return -1;

    /* see if any of the signitures matched */
    s = result->signatures;
    while (s)
    {
	status = gpg_err_code (s->status);
	if (status == GPG_ERR_NO_ERROR)
	    break;
	s = s->next;
    }


    gpgme_data_release (sig);
    gpgme_data_release (text);
    gpgme_release (ctx);

    return status;
#else
    return 0;
#endif
}
