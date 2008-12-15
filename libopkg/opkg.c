/* opkg.c - the opkg  package management system

   Thomas Wood <thomas@openedhand.com>

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

#include <config.h>
#include <fnmatch.h>

#include "opkg.h"
#include "opkg_conf.h"
#include "args.h"

#include "opkg_install.h"
#include "opkg_configure.h"
#include "opkg_download.h"

#include "sprintf_alloc.h"
#include "file_util.h"

#include "libbb.h"

struct _opkg_t
{
  args_t *args;
  opkg_conf_t *conf;
  opkg_option_t *options;
};

/** Private Functions ***/


static int
opkg_configure_packages(opkg_conf_t *conf, char *pkg_name)
{
  pkg_vec_t *all;
  int i;
  pkg_t *pkg;
  int r, err = 0;

  all = pkg_vec_alloc ();
  pkg_hash_fetch_available (&conf->pkg_hash, all);

  for (i = 0; i < all->len; i++)
  {
    pkg = all->pkgs[i];

    if (pkg_name && fnmatch (pkg_name, pkg->name, 0))
      continue;

    if (pkg->state_status == SS_UNPACKED)
    {
      r = opkg_configure (conf, pkg);
      if (r == 0)
      {
        pkg->state_status = SS_INSTALLED;
        pkg->parent->state_status = SS_INSTALLED;
        pkg->state_flag &= ~SF_PREFER;
      }
      else
      {
        if (!err)
          err = r;
      }
    }
  }

  pkg_vec_free (all);
  return err;
}



/*** Public API ***/

opkg_t *
opkg_new ()
{
  opkg_t *opkg;
  opkg = malloc (sizeof (opkg_t));

  opkg->args = malloc (sizeof (args_t));
  args_init (opkg->args);

  opkg->conf = malloc (sizeof (opkg_conf_t));
  opkg_conf_init (opkg->conf, opkg->args);

  opkg_init_options_array (opkg->conf, &opkg->options);
  return opkg;
}

void
opkg_free (opkg_t *opkg)
{
  opkg_conf_deinit (opkg->conf);
  args_deinit (opkg->args);
}

void
opkg_get_option (opkg_t *opkg, char *option, void **value)
{
  int i = 0;
  opkg_option_t *options = opkg->options;

  /* can't store a value in a NULL pointer! */
  if (!value)
    return;

  /* look up the option
   * TODO: this would be much better as a hash table
   */
  while (options[i].name)
  {
    if (strcmp (options[i].name, option) != 0)
    {
      i++;
      continue;
    }
  }

  /* get the option */
  switch (options[i].type)
  {
  case OPKG_OPT_TYPE_BOOL:
    *((int *) value) = *((int *) options[i].value);
    return;

  case OPKG_OPT_TYPE_INT:
    *((int *) value) = *((int *) options[i].value);
    return;

  case OPKG_OPT_TYPE_STRING:
    *((char **)value) = strdup (options[i].value);
    return;
  }

}

void
opkg_set_option (opkg_t *opkg, char *option, void *value)
{
  int i = 0;
  opkg_option_t *options = opkg->options;

  /* NULL values are not defined */
  if (!value)
    return;

  /* look up the option
   * TODO: this would be much better as a hash table
   */
  while (options[i].name)
  {
    if (strcmp (options[i].name, option) == 0)
    {
      break;
    }
    i++;
  }

  /* set the option */
  switch (options[i].type)
  {
  case OPKG_OPT_TYPE_BOOL:
    if (*((int *) value) == 0)
      *((int *)options[i].value) = 0;
    else
      *((int *)options[i].value) = 1;
    return;

  case OPKG_OPT_TYPE_INT:
    *((int *) options[i].value) = *((int *) value);
    return;

  case OPKG_OPT_TYPE_STRING:
    *((char **)options[i].value) = strdup (value);
    return;
  }

}

int
opkg_install_package (opkg_t *opkg, char *package_name)
{
  int err;

  pkg_info_preinstall_check (opkg->conf);

  if (opkg->conf->multiple_providers)
  {
    err = opkg_install_multi_by_name (opkg->conf, package_name);
  }
  else
  {
    err = opkg_install_by_name (opkg->conf, package_name);
  }

  err = opkg_configure_packages (opkg->conf, NULL);

  if (opkg->conf->noaction)
    return err;

  opkg_conf_write_status_files (opkg->conf);
  pkg_write_changed_filelists (opkg->conf);

  return err;
}

int
opkg_remove_package (opkg_t *opkg, char *package_name)
{
  return 1;
}

int
opkg_upgrade_package (opkg_t *opkg, char *package_name)
{
  return 1;
}

int
opkg_upgrade_all (opkg_t *opkg)
{
  return 1;
}

int
opkg_update_package_lists (opkg_t *opkg)
{
  char *tmp;
  int err;
  char *lists_dir;
  pkg_src_list_elt_t *iter;
  pkg_src_t *src;


  sprintf_alloc (&lists_dir, "%s",
                 (opkg->conf->restrict_to_default_dest)
                 ? opkg->conf->default_dest->lists_dir
                 : opkg->conf->lists_dir);

  if (!file_is_dir (lists_dir))
  {
    if (file_exists (lists_dir))
    {
      /* XXX: Error: file exists but is not a directory */
      free (lists_dir);
      return 1;
    }

    err = file_mkdir_hier (lists_dir, 0755);
    if (err)
    {
      /* XXX: Error: failed to create directory */
      free (lists_dir);
      return 1;
    }
  }

  tmp = strdup ("/tmp/opkg.XXXXXX");

  if (mkdtemp (tmp) == NULL)
  {
    /* XXX: Error: could not create temporary file name */
    free (lists_dir);
    free (tmp);
    return 1;
  }


  for (iter = opkg->conf->pkg_src_list.head; iter; iter = iter->next)
  {
    char *url, *list_file_name;

    src = iter->data;

    if (src->extra_data)  /* debian style? */
      sprintf_alloc (&url, "%s/%s/%s", src->value, src->extra_data,
                     src->gzip ? "Packages.gz" : "Packages");
    else
      sprintf_alloc (&url, "%s/%s", src->value, src->gzip ? "Packages.gz" : "Packages");

    sprintf_alloc (&list_file_name, "%s/%s", lists_dir, src->name);
    if (src->gzip)
    {
      char *tmp_file_name;
      FILE *in, *out;

      sprintf_alloc (&tmp_file_name, "%s/%s.gz", tmp, src->name);

      /* XXX: Note: downloading url */
      err = opkg_download (opkg->conf, url, tmp_file_name);

      if (err == 0)
      {
        /* XXX: Note: Inflating downloaded file */
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
    }
    else
      err = opkg_download (opkg->conf, url, list_file_name);

    if (err)
    {
      /* XXX: Error: download error */
    }
    free (url);

#ifdef HAVE_GPGME
    /* download detached signitures to verify the package lists */
    /* get the url for the sig file */
    if (src->extra_data)  /* debian style? */
      sprintf_alloc (&url, "%s/%s/%s", src->value, src->extra_data,
                     "Packages.sig");
    else
      sprintf_alloc (&url, "%s/%s", src->value, "Packages.sig");

    /* create temporary file for it */
    char *tmp_file_name;

    sprintf_alloc (&tmp_file_name, "%s/%s", tmp, "Packages.sig");

    err = opkg_download (opkg->conf, url, tmp_file_name);
    if (err)
    {
      /* XXX: Warning: Download failed */
    }
    else
    {
      int err;
      err = opkg_verify_file (opkg->conf, list_file_name, tmp_file_name);
      if (err == 0)
      {
        /* XXX: Notice: Signature check passed */
      }
      else
      {
        /* XXX: Warning: Signature check failed */
      }
    }
    unlink (tmp_file_name);
    free (tmp_file_name);
    free (url);
#else
    /* XXX: Note: Signiture check for %s skipped because GPG support was not
     * enabled in this build
     */
#endif
    free (list_file_name);
  }
  rmdir (tmp);
  free (tmp);
  free (lists_dir);

  return 0;
}
