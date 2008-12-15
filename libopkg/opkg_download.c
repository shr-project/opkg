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
#include <curl/curl.h>
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

opkg_download_progress_callback opkg_cb_download_progress = NULL;

int
curl_progress_func (char* url,
		    double t, /* dltotal */
		    double d, /* dlnow */
		    double ultotal,
		    double ulnow)
{
    int i;
    int p = (t) ? d*100/t : 0;

    if (opkg_cb_download_progress)
    {
	static int prev = -1;

	/* don't report the same percentage multiple times
	 * (this can occur due to rounding) */
	if (prev == p)
	    return 0;
	prev = p;

	opkg_cb_download_progress (p, url);
	return 0;
    }

    /* skip progress bar if we haven't done started yet
     * this prevents drawing the progress bar if we receive an error such as
     * file not found */
    if (t == 0)
	return 0;

    printf ("\r%3d%% |", p);
    for (i = 1; i < 73; i++)
    {
	if (i <= p * 0.73)
	    printf ("=");
	else
	    printf ("-");
    }
    printf ("|");
    fflush(stdout);
    return 0;
}

int opkg_download(opkg_conf_t *conf, const char *src, const char *dest_file_name)
{
    int err = 0;

    char *src_basec = strdup(src);
    char *src_base = basename(src_basec);
    char *tmp_file_location;

    opkg_message(conf,OPKG_NOTICE,"Downloading %s\n", src);
	
    fflush(stdout);
    
    if (str_starts_with(src, "file:")) {
	int ret;
	const char *file_src = src + 5;
	opkg_message(conf,OPKG_INFO,"Copying %s to %s...", file_src, dest_file_name);
	ret = file_copy(src + 5, dest_file_name);
	opkg_message(conf,OPKG_INFO,"Done\n");
	return ret;
    }

    sprintf_alloc(&tmp_file_location, "%s/%s", conf->tmp_dir, src_base);
    err = unlink(tmp_file_location);
    if (err && errno != ENOENT) {
	opkg_message(conf,OPKG_ERROR, "%s: ERROR: failed to unlink %s: %s\n",
		__FUNCTION__, tmp_file_location, strerror(errno));
	free(tmp_file_location);
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

    CURL *curl;
    CURLcode res;
    FILE * file = fopen (tmp_file_location, "w");

    curl = curl_easy_init ();
    if (curl)
    {
	curl_easy_setopt (curl, CURLOPT_URL, src);
	curl_easy_setopt (curl, CURLOPT_WRITEDATA, file);
	curl_easy_setopt (curl, CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt (curl, CURLOPT_PROGRESSDATA, src);
	curl_easy_setopt (curl, CURLOPT_PROGRESSFUNCTION, curl_progress_func);
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
	curl_easy_cleanup (curl);
	fclose (file);
	if (res)
	{
	    long error_code;
	    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &error_code);
	    opkg_message(conf, OPKG_ERROR, "Failed to download %s, error %d\n", src, error_code);
	    return res;
	}

    }
    else
	return -1;

    /* if no custom progress handler was set, we need to clear the default progress bar */
    if (!opkg_cb_download_progress)
        printf ("\n");

    err = file_move(tmp_file_location, dest_file_name);

    free(tmp_file_location);
    free(src_basec);

    if (err) {
	return err;
    }

    return 0;
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
       "../../foo.ipk". While this is correct, and exactly what we
       want to use to construct url above, here we actually need to
       use just the filename part, without any directory. */

    stripped_filename = strrchr(pkg->filename, '/');
    if ( ! stripped_filename )
        stripped_filename = pkg->filename;

    sprintf_alloc(&pkg->local_filename, "%s/%s", dir, stripped_filename);

    err = opkg_download(conf, url, pkg->local_filename);
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
	  err = opkg_download(conf, url, tmp_file);
	  if (err)
	       return err;

	  err = pkg_init_from_file(pkg, tmp_file);
	  if (err)
	       return err;
	  pkg->local_filename = strdup(tmp_file);

	  free(tmp_file);
	  free(file_basec);

     } else if (strcmp(&url[strlen(url) - 4], OPKG_PKG_EXTENSION) == 0
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
    int status = -1;
    gpgme_ctx_t ctx;
    gpgme_data_t sig, text;
    gpgme_error_t err = -1;
    gpgme_verify_result_t result;
    gpgme_signature_t s;
    
    err = gpgme_new (&ctx);

    if (err)
	return -1;

    err = gpgme_data_new_from_file (&sig, sig_file, 1); 
    if (err)
	return -1;

    err = gpgme_data_new_from_file (&text, text_file, 1); 
    if (err)
	return -1;

    err = gpgme_op_verify (ctx, sig, text, NULL);

    result = gpgme_op_verify_result (ctx);

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
    opkg_message (conf, OPKG_NOTICE, "Signature check for %s was skipped because GPG support was not enabled in this build\n");
    return 0;
#endif
}
