/* pkg_vec.c - the opkg package management system

   Steven M. Ayer
   
   Copyright (C) 2002 Compaq Computer Corporation

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
#include <fnmatch.h>
#include "xregex.h"
#include "pkg.h"
#include "opkg_message.h"
#include "libbb/libbb.h"

pkg_vec_t * pkg_vec_alloc(void)
{
    pkg_vec_t * vec = xcalloc(1, sizeof(pkg_vec_t));
    vec->pkgs = NULL;
    vec->len = 0;

    return vec;
}

void pkg_vec_free(pkg_vec_t *vec)
{
    if (!vec)
      return;

    if (vec->pkgs)
      free(vec->pkgs);

    free(vec);
}

/*
 * assumption: all names in a vector are identical
 * assumption: all version strings are trimmed,
 *             so identical versions have identical version strings,
 *             implying identical packages; let's marry these
 */
pkg_t *pkg_vec_insert_merge(pkg_vec_t *vec, pkg_t *pkg, int set_status,opkg_conf_t *conf)
{
     int i;
     int found = 0;

     /* look for a duplicate pkg by name, version, and architecture */
     for (i = 0; i < vec->len; i++){
         opkg_message(conf, OPKG_DEBUG2, "Function: %s. Found pkg=%s version=%s arch=%s cmp=%s version=%s arch=%s \n", 
                      __FUNCTION__, pkg->name, pkg->version, pkg->architecture, 
                       vec->pkgs[i]->name, vec->pkgs[i]->version,vec->pkgs[i]->architecture );
	  if ((strcmp(pkg->name, vec->pkgs[i]->name) == 0)
	      && (pkg_compare_versions(pkg, vec->pkgs[i]) == 0)
	      && (strcmp(pkg->architecture, vec->pkgs[i]->architecture) == 0)) {
	       found  = 1;
               opkg_message(conf, OPKG_DEBUG2, "Function: %s. Found duplicate for pkg=%s version=%s arch=%s\n",
                             __FUNCTION__, pkg->name, pkg->version, pkg->architecture);
	       break;
	  }
     }

     /* we didn't find one, add it */
     if (!found){   
          opkg_message(conf, OPKG_DEBUG2, "Function: %s. Adding new pkg=%s version=%s arch=%s\n",
                      __FUNCTION__, pkg->name, pkg->version, pkg->architecture);
          pkg_vec_insert(vec, pkg);
	  return pkg;
     }
     /* update the one that we have */
     else {
          opkg_message(conf, OPKG_DEBUG2, "Function: %s. calling pkg_merge for pkg=%s version=%s arch=%s",
                        __FUNCTION__, pkg->name, pkg->version, pkg->architecture);
	  if (set_status) {
	       /* this is from the status file, so need to merge with existing database */
               opkg_message(conf, OPKG_DEBUG2, " with set_status\n");
	       pkg_merge(vec->pkgs[i], pkg, set_status);
	       /* XXX: CLEANUP: It's not so polite to free something here
		  that was passed in from above. */
	       pkg_deinit(pkg);
	       free(pkg);
	  } else {
               opkg_message(conf, OPKG_DEBUG2, " WITHOUT set_status\n");
	       /* just overwrite the old one */
	       pkg_deinit(vec->pkgs[i]);
	       free(vec->pkgs[i]);
	       vec->pkgs[i] = pkg;
	  }
	  return vec->pkgs[i];
     }
}

void pkg_vec_insert(pkg_vec_t *vec, const pkg_t *pkg)
{
    vec->pkgs = xrealloc(vec->pkgs, (vec->len + 1) * sizeof(pkg_t *));
    vec->pkgs[vec->len] = (pkg_t *)pkg;
    vec->len++;
}

int pkg_vec_contains(pkg_vec_t *vec, pkg_t *apkg)
{
     int i;
     for (i = 0; i < vec->len; i++)
	  if (vec->pkgs[i] == apkg)
	       return 1;
     return 0;
}

void pkg_vec_sort(pkg_vec_t *vec, compare_fcn_t compar)
{
     qsort(vec->pkgs, vec->len, sizeof(pkg_t *), compar);
}

int pkg_vec_clear_marks(pkg_vec_t *vec)
{
     int npkgs = vec->len;
     int i;
     for (i = 0; i < npkgs; i++) {
	  pkg_t *pkg = vec->pkgs[i];
	  pkg->state_flag &= ~SF_MARKED;
     }
     return 0;
}

int pkg_vec_mark_if_matches(pkg_vec_t *vec, const char *pattern)
{
     int matching_count = 0;
     pkg_t **pkgs = vec->pkgs;
     int npkgs = vec->len;
     int i;
     for (i = 0; i < npkgs; i++) {
	  pkg_t *pkg = pkgs[i];
	  if (fnmatch(pattern, pkg->name, 0)==0) {
	       pkg->state_flag |= SF_MARKED;
	       matching_count++;
	  }
     }
     return matching_count;
}


abstract_pkg_vec_t * abstract_pkg_vec_alloc(void)
{
    abstract_pkg_vec_t * vec ; 
    vec = xcalloc(1, sizeof(abstract_pkg_vec_t));
    vec->pkgs = NULL;
    vec->len = 0;

    return vec;
}

void abstract_pkg_vec_free(abstract_pkg_vec_t *vec)
{
    if (!vec)
      return;
    free(vec->pkgs);
    free(vec);
}

/*
 * assumption: all names in a vector are unique
 */
void abstract_pkg_vec_insert(abstract_pkg_vec_t *vec, abstract_pkg_t *pkg)
{
    vec->pkgs = xrealloc(vec->pkgs, (vec->len + 1) * sizeof(abstract_pkg_t *));
    vec->pkgs[vec->len] = pkg;
    vec->len++;
}

abstract_pkg_t * abstract_pkg_vec_get(abstract_pkg_vec_t *vec, int i)
{
    if (vec->len > i) 
	return vec->pkgs[i];
    else
	return NULL;
}

int abstract_pkg_vec_contains(abstract_pkg_vec_t *vec, abstract_pkg_t *apkg)
{
     int i;
     for (i = 0; i < vec->len; i++)
	  if (vec->pkgs[i] == apkg)
	       return 1;
     return 0;
}

void abstract_pkg_vec_sort(pkg_vec_t *vec, compare_fcn_t compar)
{
     qsort(vec->pkgs, vec->len, sizeof(pkg_t *), compar);
}

int pkg_compare_names(const void *p1, const void *p2)
{
  const pkg_t *pkg1 = *(const pkg_t **)p1;
  const pkg_t *pkg2 = *(const pkg_t **)p2;
  if (pkg1->name == NULL)
    return 1;
  if (pkg2->name == NULL)
    return -1;
  return(strcmp(pkg1->name, pkg2->name));
}

