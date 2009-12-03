/* opkg_install.c - the opkg package management system

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
#include <errno.h>
#include <dirent.h>
#include <glob.h>
#include <time.h>
#include <signal.h>

#include "pkg.h"
#include "pkg_hash.h"
#include "pkg_extract.h"

#include "opkg_install.h"
#include "opkg_configure.h"
#include "opkg_download.h"
#include "opkg_remove.h"

#include "opkg_utils.h"
#include "opkg_message.h"
#include "opkg_cmd.h"
#include "opkg_defines.h"

#include "sprintf_alloc.h"
#include "file_util.h"
#include "xsystem.h"
#include "user.h"
#include "libbb/libbb.h"

static int
satisfy_dependencies_for(opkg_conf_t *conf, pkg_t *pkg)
{
     int i, err;
     pkg_vec_t *depends = pkg_vec_alloc();
     pkg_t *dep;
     char **tmp, **unresolved = NULL;
     int ndepends;

     ndepends = pkg_hash_fetch_unsatisfied_dependencies(conf, 
							pkg, depends, 
							&unresolved);

     if (unresolved) {
	  opkg_message(conf, OPKG_ERROR,
		       "%s: Cannot satisfy the following dependencies for %s:\n\t",
		       conf->force_depends ? "Warning" : "ERROR", pkg->name);
	  tmp = unresolved;
	  while (*unresolved) {
	       opkg_message(conf, OPKG_ERROR, " %s", *unresolved);
	       free(*unresolved);
	       unresolved++;
	  }
	  free(tmp);
	  opkg_message(conf, OPKG_ERROR, "\n");
	  if (! conf->force_depends) {
	       opkg_message(conf, OPKG_INFO,
			    "This could mean that your package list is out of date or that the packages\n"
			    "mentioned above do not yet exist (try 'opkg update'). To proceed in spite\n"
			    "of this problem try again with the '-force-depends' option.\n");
	       pkg_vec_free(depends);
	       return -1;
	  }
     }

     if (ndepends <= 0) {
	  pkg_vec_free(depends);
	  return 0;
     }

     /* Mark packages as to-be-installed */
     for (i=0; i < depends->len; i++) {
	  /* Dependencies should be installed the same place as pkg */
	  if (depends->pkgs[i]->dest == NULL) {
	       depends->pkgs[i]->dest = pkg->dest;
	  }
	  depends->pkgs[i]->state_want = SW_INSTALL;
     }

     for (i = 0; i < depends->len; i++) {
	  dep = depends->pkgs[i];
	  /* The package was uninstalled when we started, but another
	     dep earlier in this loop may have depended on it and pulled
	     it in, so check first. */
	  if ((dep->state_status != SS_INSTALLED)
	      && (dep->state_status != SS_UNPACKED)) {
               opkg_message(conf, OPKG_DEBUG2,"Function: %s calling opkg_install_pkg \n",__FUNCTION__);
	       err = opkg_install_pkg(conf, dep,0);
	       /* mark this package as having been automatically installed to
	        * satisfy a dependancy */
	       dep->auto_installed = 1;
	       if (err) {
		    pkg_vec_free(depends);
		    return err;
	       }
	  }
     }

     pkg_vec_free(depends);

     return 0;
}

static int
check_conflicts_for(opkg_conf_t *conf, pkg_t *pkg)
{
     int i;
     pkg_vec_t *conflicts = NULL;
     int level;
     const char *prefix;
     if (conf->force_depends) {
	  level = OPKG_NOTICE;
	  prefix = "Warning";
     } else {
	  level = OPKG_ERROR;
	  prefix = "ERROR";
     }

     if (!conf->force_depends)
	  conflicts = (pkg_vec_t *)pkg_hash_fetch_conflicts(&conf->pkg_hash, pkg);

     if (conflicts) {
	  opkg_message(conf, level,
		       "%s: The following packages conflict with %s:\n\t", prefix, pkg->name);
	  i = 0;
	  while (i < conflicts->len)
	       opkg_message(conf, level, " %s", conflicts->pkgs[i++]->name);
	  opkg_message(conf, level, "\n");
	  pkg_vec_free(conflicts);
	  return -1;
     }
     return 0;
}

static int
update_file_ownership(opkg_conf_t *conf, pkg_t *new_pkg, pkg_t *old_pkg)
{
     str_list_t *new_list, *old_list;
     str_list_elt_t *iter, *niter;

     new_list = pkg_get_installed_files(conf, new_pkg);
     if (new_list == NULL)
	     return -1;

     for (iter = str_list_first(new_list), niter = str_list_next(new_list, iter); 
             iter; 
             iter = niter, niter = str_list_next(new_list, niter)) {
	  char *new_file = (char *)iter->data;
	  pkg_t *owner = file_hash_get_file_owner(conf, new_file);
	  if (!new_file)
	       opkg_message(conf, OPKG_ERROR, "Null new_file for new_pkg=%s\n", new_pkg->name);
	  if (!owner || (owner == old_pkg))
	       file_hash_set_file_owner(conf, new_file, new_pkg);
     }

     if (old_pkg) {
	  old_list = pkg_get_installed_files(conf, old_pkg);
	  if (old_list == NULL) {
     		  pkg_free_installed_files(new_pkg);
		  return -1;
	  }

	  for (iter = str_list_first(old_list), niter = str_list_next(old_list, iter); 
                  iter; 
                  iter = niter, niter = str_list_next(old_list, niter)) {
	       char *old_file = (char *)iter->data;
	       pkg_t *owner = file_hash_get_file_owner(conf, old_file);
	       if (owner == old_pkg) {
		    /* obsolete */
		    hash_table_insert(&conf->obs_file_hash, old_file, old_pkg);
	       }
	  }
          pkg_free_installed_files(old_pkg);
     }
     pkg_free_installed_files(new_pkg);
     return 0;
}

static int
verify_pkg_installable(opkg_conf_t *conf, pkg_t *pkg)
{
    /* XXX: FEATURE: Anything else needed here? Maybe a check on free space? */

    /* sma 6.20.02:  yup; here's the first bit */
    /* 
     * XXX: BUG easy for cworth
     * 1) please point the call below to the correct current root destination
     * 2) we need to resolve how to check the required space for a pending pkg, 
     *    my diddling with the .opk file size below isn't going to cut it.
     * 3) return a proper error code instead of 1
     */
     int comp_size, blocks_available;
     char *root_dir;
    
     if (!conf->force_space && pkg->installed_size != NULL) {
          root_dir = pkg->dest ? pkg->dest->root_dir : conf->default_dest->root_dir;
	  blocks_available = get_available_blocks(root_dir);

	  comp_size = strtoul(pkg->installed_size, NULL, 0);
	  /* round up a blocks count without doing fancy-but-slow casting jazz */ 
	  comp_size = (int)((comp_size + 1023) / 1024);

	  if (comp_size >= blocks_available) {
	       opkg_message(conf, OPKG_ERROR,
			    "Only have %d available blocks on filesystem %s, pkg %s needs %d\n", 
			    blocks_available, root_dir, pkg->name, comp_size);
	       return ENOSPC;
	  }
     }
     return 0;
}

static int
unpack_pkg_control_files(opkg_conf_t *conf, pkg_t *pkg)
{
     int err;
     char *conffiles_file_name;
     char *root_dir;
     FILE *conffiles_file;

     sprintf_alloc(&pkg->tmp_unpack_dir, "%s/%s-XXXXXX", conf->tmp_dir, pkg->name);

     pkg->tmp_unpack_dir = mkdtemp(pkg->tmp_unpack_dir);
     if (pkg->tmp_unpack_dir == NULL) {
	  opkg_message(conf, OPKG_ERROR,
		       "%s: Failed to create temporary directory '%s': %s\n",
		       __FUNCTION__, pkg->tmp_unpack_dir, strerror(errno));
	  return -1;
     }

     err = pkg_extract_control_files_to_dir(pkg, pkg->tmp_unpack_dir);
     if (err) {
	  return err;
     }

     /* XXX: CLEANUP: There might be a cleaner place to read in the
	conffiles. Seems like I should be able to get everything to go
	through pkg_init_from_file. If so, maybe it would make sense to
	move all of unpack_pkg_control_files to that function. */

     /* Don't need to re-read conffiles if we already have it */
     if (!nv_pair_list_empty(&pkg->conffiles)) {
	  return 0;
     }

     sprintf_alloc(&conffiles_file_name, "%s/conffiles", pkg->tmp_unpack_dir);
     if (! file_exists(conffiles_file_name)) {
	  free(conffiles_file_name);
	  return 0;
     }
    
     conffiles_file = fopen(conffiles_file_name, "r");
     if (conffiles_file == NULL) {
	  fprintf(stderr, "%s: failed to open %s: %s\n",
		  __FUNCTION__, conffiles_file_name, strerror(errno));
	  free(conffiles_file_name);
	  return -1;
     }
     free(conffiles_file_name);

     while (1) {
	  char *cf_name;
	  char *cf_name_in_dest;

	  cf_name = file_read_line_alloc(conffiles_file);
	  if (cf_name == NULL) {
	       break;
	  }
	  if (cf_name[0] == '\0') {
	       continue;
	  }

	  /* Prepend dest->root_dir to conffile name.
	     Take pains to avoid multiple slashes. */
	  root_dir = pkg->dest->root_dir;
	  if (conf->offline_root)
	       /* skip the offline_root prefix */
	       root_dir = pkg->dest->root_dir + strlen(conf->offline_root);
	  sprintf_alloc(&cf_name_in_dest, "%s%s", root_dir,
			cf_name[0] == '/' ? (cf_name + 1) : cf_name);

	  /* Can't get an md5sum now, (file isn't extracted yet).
	     We'll wait until resolve_conffiles */
	  conffile_list_append(&pkg->conffiles, cf_name_in_dest, NULL);

	  free(cf_name);
	  free(cf_name_in_dest);
     }

     fclose(conffiles_file);

     return 0;
}

/*
 * Remove packages which were auto_installed due to a dependency by old_pkg,
 * which are no longer a dependency in the new (upgraded) pkg.
 */
static int
pkg_remove_orphan_dependent(opkg_conf_t *conf, pkg_t *pkg, pkg_t *old_pkg) 
{
	int i, j, k, l, found;
	int n_deps;
	pkg_t *p;
	struct compound_depend *cd0, *cd1;
        abstract_pkg_t **dependents;

	int count0 = old_pkg->pre_depends_count +
				old_pkg->depends_count +
				old_pkg->recommends_count +
				old_pkg->suggests_count;
	int count1 = pkg->pre_depends_count +
				pkg->depends_count +
				pkg->recommends_count +
				pkg->suggests_count;

	for (i=0; i<count0; i++) {
		cd0 = &old_pkg->depends[i];
		if (cd0->type != DEPEND)
			continue;
		for (j=0; j<cd0->possibility_count; j++) {

			found = 0;

			for (k=0; k<count1; k++) {
				cd1 = &pkg->depends[i];
				if (cd1->type != DEPEND)
					continue;
				for (l=0; l<cd1->possibility_count; l++) {
					if (cd0->possibilities[j]
					 == cd1->possibilities[l]) {
						found = 1;
						break;
					}
				}
				if (found)
					break;
			}

			if (found)
				continue;

			/*
			 * old_pkg has a dependency that pkg does not.
			 */
			p = pkg_hash_fetch_installed_by_name (&conf->pkg_hash,
					cd0->possibilities[j]->pkg->name);

			if (!p)
				continue;

			if (!p->auto_installed)
				continue;

			n_deps = pkg_has_installed_dependents(conf, NULL, p,
					&dependents);
			n_deps--; /* don't count old_pkg */

			if (n_deps == 0) {
				opkg_message (conf, OPKG_NOTICE,
						"%s was autoinstalled and is "
						"now orphaned, removing.\n",
						p->name);

				/* p has one installed dependency (old_pkg),
				 * which we need to ignore during removal. */
				p->state_flag |= SF_REPLACE;

				opkg_remove_pkg(conf, p, 0);
			} else 
				opkg_message(conf, OPKG_INFO,
						"%s was autoinstalled and is "
						"still required by %d "
						"installed packages.\n",
						p->name, n_deps);

		}
	}

	return 0;
}

/* returns number of installed replacees */
static int
pkg_get_installed_replacees(opkg_conf_t *conf, pkg_t *pkg, pkg_vec_t *installed_replacees)
{
     abstract_pkg_t **replaces = pkg->replaces;
     int replaces_count = pkg->replaces_count;
     int i, j;
     for (i = 0; i < replaces_count; i++) {
	  abstract_pkg_t *ab_pkg = replaces[i];
	  pkg_vec_t *pkg_vec = ab_pkg->pkgs;
	  if (pkg_vec) {
	       for (j = 0; j < pkg_vec->len; j++) {
		    pkg_t *replacee = pkg_vec->pkgs[j];
		    if (!pkg_conflicts(pkg, replacee))
			 continue;
		    if (replacee->state_status == SS_INSTALLED) {
			 pkg_vec_insert(installed_replacees, replacee);
		    }
	       }
	  }
     }
     return installed_replacees->len;
}

static int
pkg_remove_installed_replacees(opkg_conf_t *conf, pkg_vec_t *replacees)
{
     int i;
     int replaces_count = replacees->len;
     for (i = 0; i < replaces_count; i++) {
	  pkg_t *replacee = replacees->pkgs[i];
	  int err;
	  replacee->state_flag |= SF_REPLACE; /* flag it so remove won't complain */
	  err = opkg_remove_pkg(conf, replacee,0);
	  if (err)
	       return err;
     }
     return 0;
}

/* to unwind the removal: make sure they are installed */
static int
pkg_remove_installed_replacees_unwind(opkg_conf_t *conf, pkg_vec_t *replacees)
{
     int i, err;
     int replaces_count = replacees->len;
     for (i = 0; i < replaces_count; i++) {
	  pkg_t *replacee = replacees->pkgs[i];
	  if (replacee->state_status != SS_INSTALLED) {
               opkg_message(conf, OPKG_DEBUG2,"Function: %s calling opkg_install_pkg \n",__FUNCTION__);
	       err = opkg_install_pkg(conf, replacee,0);
	       if (err)
		    return err;
	  }
     }
     return 0;
}

/* compares versions of pkg and old_pkg, returns 0 if OK to proceed with installation of pkg, 1 otherwise */
static int
opkg_install_check_downgrade(opkg_conf_t *conf, pkg_t *pkg, pkg_t *old_pkg, int message)
{	  
     if (old_pkg) {
          char message_out[15];
	  char *old_version = pkg_version_str_alloc(old_pkg);
	  char *new_version = pkg_version_str_alloc(pkg);
	  int cmp = pkg_compare_versions(old_pkg, pkg);
	  int rc = 0;

          memset(message_out,'\x0',15);
          strncpy (message_out,"Upgrading ",strlen("Upgrading ")); 
          if ( (conf->force_downgrade==1) && (cmp > 0) ){     /* We've been asked to allow downgrade  and version is precedent */
             cmp = -1 ;                                       /* then we force opkg to downgrade */ 
             strncpy (message_out,"Downgrading ",strlen("Downgrading "));         /* We need to use a value < 0 because in the 0 case we are asking to */
                                                              /* reinstall, and some check could fail asking the "force-reinstall" option */
          } 

	  if (cmp > 0) {
	       opkg_message(conf, OPKG_NOTICE,
			    "Not downgrading package %s on %s from %s to %s.\n",
			    old_pkg->name, old_pkg->dest->name, old_version, new_version);
	       rc = 1;
	  } else if (cmp < 0) {
	       opkg_message(conf, OPKG_NOTICE,
			    "%s%s on %s from %s to %s...\n",
			    message_out, pkg->name, old_pkg->dest->name, old_version, new_version);
	       pkg->dest = old_pkg->dest;
	       rc = 0;
	  } else /* cmp == 0 */ {
	       if (conf->force_reinstall) {
		    opkg_message(conf, OPKG_NOTICE,
				 "Reinstalling %s (%s) on %s...\n",
				 pkg->name, new_version, old_pkg->dest->name);
		    pkg->dest = old_pkg->dest;
		    rc = 0;
	       } else {
		    opkg_message(conf, OPKG_NOTICE,
				 "Not installing %s (%s) on %s -- already installed.\n",
				 pkg->name, new_version, old_pkg->dest->name);
		    rc = 1;
	       }
	  } 
	  free(old_version);
	  free(new_version);
	  return rc;
     } else {
      char message_out[15] ;
      memset(message_out,'\x0',15);
      if ( message ) 
          strncpy( message_out,"Upgrading ",strlen("Upgrading ") );
      else
          strncpy( message_out,"Installing ",strlen("Installing ") );
	  char *version = pkg_version_str_alloc(pkg);
      
	  opkg_message(conf, OPKG_NOTICE,
		       "%s%s (%s) to %s...\n", message_out,
		       pkg->name, version, pkg->dest->name);
	  free(version);
	  return 0;
     }
}


static int
prerm_upgrade_old_pkg(opkg_conf_t *conf, pkg_t *pkg, pkg_t *old_pkg)
{
     /* DPKG_INCOMPATIBILITY:
	dpkg does some things here that we don't do yet. Do we care?
	
	1. If a version of the package is already installed, call
	   old-prerm upgrade new-version
	2. If the script runs but exits with a non-zero exit status
	   new-prerm failed-upgrade old-version
	   Error unwind, for both the above cases:
	   old-postinst abort-upgrade new-version
     */
     return 0;
}

static int
prerm_upgrade_old_pkg_unwind(opkg_conf_t *conf, pkg_t *pkg, pkg_t *old_pkg)
{
     /* DPKG_INCOMPATIBILITY:
	dpkg does some things here that we don't do yet. Do we care?
	(See prerm_upgrade_old_package for details)
     */
     return 0;
}

static int
prerm_deconfigure_conflictors(opkg_conf_t *conf, pkg_t *pkg, pkg_vec_t *conflictors)
{
     /* DPKG_INCOMPATIBILITY:
	dpkg does some things here that we don't do yet. Do we care?
	2. If a 'conflicting' package is being removed at the same time:
		1. If any packages depended on that conflicting package and
		   --auto-deconfigure is specified, call, for each such package:
		   deconfigured's-prerm deconfigure \
		   in-favour package-being-installed version \
		   removing conflicting-package version
		Error unwind:
		   deconfigured's-postinst abort-deconfigure \
		   in-favour package-being-installed-but-failed version \
		   removing conflicting-package version

		   The deconfigured packages are marked as requiring
		   configuration, so that if --install is used they will be
		   configured again if possible.
		2. To prepare for removal of the conflicting package, call:
		   conflictor's-prerm remove in-favour package new-version
		Error unwind:
		   conflictor's-postinst abort-remove in-favour package new-version
     */
     return 0;
}

static int
prerm_deconfigure_conflictors_unwind(opkg_conf_t *conf, pkg_t *pkg, pkg_vec_t *conflictors)
{
     /* DPKG_INCOMPATIBILITY: dpkg does some things here that we don't
	do yet. Do we care?  (See prerm_deconfigure_conflictors for
	details) */
     return 0;
}

static int
preinst_configure(opkg_conf_t *conf, pkg_t *pkg, pkg_t *old_pkg)
{
     int err;
     char *preinst_args;

     if (old_pkg) {
	  char *old_version = pkg_version_str_alloc(old_pkg);
	  sprintf_alloc(&preinst_args, "upgrade %s", old_version);
	  free(old_version);
     } else if (pkg->state_status == SS_CONFIG_FILES) {
	  char *pkg_version = pkg_version_str_alloc(pkg);
	  sprintf_alloc(&preinst_args, "install %s", pkg_version);
	  free(pkg_version);
     } else {
	  preinst_args = xstrdup("install");
     }

     err = pkg_run_script(conf, pkg, "preinst", preinst_args);
     if (err) {
	  opkg_message(conf, OPKG_ERROR,
		       "Aborting installation of %s\n", pkg->name);
	  return 1;
     }

     free(preinst_args);

     return 0;
}

static int
preinst_configure_unwind(opkg_conf_t *conf, pkg_t *pkg, pkg_t *old_pkg)
{
     /* DPKG_INCOMPATIBILITY:
	dpkg does the following error unwind, should we?
	pkg->postrm abort-upgrade old-version
	OR pkg->postrm abort-install old-version
	OR pkg->postrm abort-install
     */
     return 0;
}

static char *
backup_filename_alloc(const char *file_name)
{
     char *backup;

     sprintf_alloc(&backup, "%s%s", file_name, OPKG_BACKUP_SUFFIX);

     return backup;
}


static int
backup_make_backup(opkg_conf_t *conf, const char *file_name)
{
     int err;
     char *backup;
    
     backup = backup_filename_alloc(file_name);
     err = file_copy(file_name, backup);
     if (err) {
	  opkg_message(conf, OPKG_ERROR,
		       "%s: Failed to copy %s to %s\n",
		       __FUNCTION__, file_name, backup);
     }

     free(backup);

     return err;
}

static int
backup_exists_for(const char *file_name)
{
     int ret;
     char *backup;

     backup = backup_filename_alloc(file_name);

     ret = file_exists(backup);

     free(backup);

     return ret;
}

static int
backup_remove(const char *file_name)
{
     char *backup;

     backup = backup_filename_alloc(file_name);
     unlink(backup);
     free(backup);

     return 0;
}

static int
backup_modified_conffiles(opkg_conf_t *conf, pkg_t *pkg, pkg_t *old_pkg)
{
     int err;
     conffile_list_elt_t *iter;
     conffile_t *cf;

     if (conf->noaction) return 0;

     /* Backup all modified conffiles */
     if (old_pkg) {
	  for (iter = nv_pair_list_first(&old_pkg->conffiles); iter; iter = nv_pair_list_next(&old_pkg->conffiles, iter)) {
	       char *cf_name;
	       
	       cf = iter->data;
	       cf_name = root_filename_alloc(conf, cf->name);

	       /* Don't worry if the conffile is just plain gone */
	       if (file_exists(cf_name) && conffile_has_been_modified(conf, cf)) {
		    err = backup_make_backup(conf, cf_name);
		    if (err) {
			 return err;
		    }
	       }
	       free(cf_name);
	  }
     }

     /* Backup all conffiles that were not conffiles in old_pkg */
     for (iter = nv_pair_list_first(&pkg->conffiles); iter; iter = nv_pair_list_next(&pkg->conffiles, iter)) {
	  char *cf_name;
	  cf = (conffile_t *)iter->data;
	  cf_name = root_filename_alloc(conf, cf->name);
	  /* Ignore if this was a conffile in old_pkg as well */
	  if (pkg_get_conffile(old_pkg, cf->name)) {
	       continue;
	  }

	  if (file_exists(cf_name) && (! backup_exists_for(cf_name))) {
	       err = backup_make_backup(conf, cf_name);
	       if (err) {
		    return err;
	       }
	  }
	  free(cf_name);
     }

     return 0;
}

static int
backup_modified_conffiles_unwind(opkg_conf_t *conf, pkg_t *pkg, pkg_t *old_pkg)
{
     conffile_list_elt_t *iter;

     if (old_pkg) {
	  for (iter = nv_pair_list_first(&old_pkg->conffiles); iter; iter = nv_pair_list_next(&old_pkg->conffiles, iter)) {
	       backup_remove(((nv_pair_t *)iter->data)->name);
	  }
     }

     for (iter = nv_pair_list_first(&pkg->conffiles); iter; iter = nv_pair_list_next(&pkg->conffiles, iter)) {
	  backup_remove(((nv_pair_t *)iter->data)->name);
     }

     return 0;
}


static int
check_data_file_clashes(opkg_conf_t *conf, pkg_t *pkg, pkg_t *old_pkg)
{
     /* DPKG_INCOMPATIBILITY:
	opkg takes a slightly different approach than dpkg at this
	point.  dpkg installs each file in the new package while
	creating a backup for any file that is replaced, (so that it
	can unwind if necessary).  To avoid complexity and redundant
	storage, opkg doesn't do any installation until later, (at the
	point at which dpkg removes the backups.
	
	But, we do have to check for data file clashes, since after
	installing a package with a file clash, removing either of the
	packages involved in the clash has the potential to break the
	other package.
     */
     str_list_t *files_list;
     str_list_elt_t *iter, *niter;

     int clashes = 0;

     files_list = pkg_get_installed_files(conf, pkg);
     if (files_list == NULL)
	     return -1;

     for (iter = str_list_first(files_list), niter = str_list_next(files_list, iter); 
             iter; 
             iter = niter, niter = str_list_next(files_list, iter)) {
	  char *root_filename;
	  char *filename = (char *) iter->data;
	  root_filename = root_filename_alloc(conf, filename);
	  if (file_exists(root_filename) && (! file_is_dir(root_filename))) {
	       pkg_t *owner;
	       pkg_t *obs;
	       /* Pre-existing conffiles are OK */
	       /* @@@@ should have way to check that it is a conffile -Jamey */
	       if (backup_exists_for(root_filename)) {
		    continue;
	       }

	       /* Pre-existing files are OK if force-overwrite was asserted. */ 
	       if (conf->force_overwrite) {
		    /* but we need to change who owns this file */
		    file_hash_set_file_owner(conf, filename, pkg);
		    continue;
	       }

	       owner = file_hash_get_file_owner(conf, filename);

	       /* Pre-existing files are OK if owned by the pkg being upgraded. */
	       if (owner && old_pkg) {
		    if (strcmp(owner->name, old_pkg->name) == 0) {
			 continue;
		    }
	       }

	       /* Pre-existing files are OK if owned by a package replaced by new pkg. */
	       if (owner) {
                    opkg_message(conf, OPKG_DEBUG2, "Checking for replaces for %s in package %s\n", filename, owner->name);
		    if (pkg_replaces(pkg, owner)) {
			 continue;
		    }
/* If the file that would be installed is owned by the same package, ( as per a reinstall or similar )
   then it's ok to overwrite. */
                    if (strcmp(owner->name,pkg->name)==0){
			 opkg_message(conf, OPKG_INFO, "Replacing pre-existing file %s owned by package %s\n", filename, owner->name);
			 continue;
                    }
	       }

	       /* Pre-existing files are OK if they are obsolete */
	       obs = hash_table_get(&conf->obs_file_hash, filename);
	       if (obs) {
		    opkg_message(conf, OPKG_INFO, "Pre-exiting file %s is obsolete.  obs_pkg=%s\n", filename, obs->name);
		    continue;
	       }

	       /* We have found a clash. */
	       opkg_message(conf, OPKG_ERROR,
			    "Package %s wants to install file %s\n"
			    "\tBut that file is already provided by package ",
			    pkg->name, filename);
	       if (owner) {
		    opkg_message(conf, OPKG_ERROR,
				 "%s\n", owner->name);
	       } else {
		    opkg_message(conf, OPKG_ERROR,
				 "<no package>\nPlease move this file out of the way and try again.\n");
	       }
	       clashes++;
	  }
	  free(root_filename);
     }
     pkg_free_installed_files(pkg);

     return clashes;
}

/*
 * XXX: This function sucks, as does the below comment.
 */
static int
check_data_file_clashes_change(opkg_conf_t *conf, pkg_t *pkg, pkg_t *old_pkg)
{
    /* Basically that's the worst hack I could do to be able to change ownership of
       file list, but, being that we have no way to unwind the mods, due to structure
       of hash table, probably is the quickest hack too, whishing it would not slow-up thing too much.
       What we do here is change the ownership of file in hash if a replace ( or similar events
       happens )
       Only the action that are needed to change name should be considered.
       @@@ To change after 1.0 release.
     */
     str_list_t *files_list;
     str_list_elt_t *iter, *niter;

     char *root_filename = NULL;

     files_list = pkg_get_installed_files(conf, pkg);
     if (files_list == NULL)
	     return -1;

     for (iter = str_list_first(files_list), niter = str_list_next(files_list, iter); 
             iter; 
             iter = niter, niter = str_list_next(files_list, niter)) {
	  char *filename = (char *) iter->data;
          if (root_filename) {
              free(root_filename);
              root_filename = NULL;
          }
	  root_filename = root_filename_alloc(conf, filename);
	  if (file_exists(root_filename) && (! file_is_dir(root_filename))) {
	       pkg_t *owner;

	       owner = file_hash_get_file_owner(conf, filename);

	       if (conf->force_overwrite) {
		    /* but we need to change who owns this file */
		    file_hash_set_file_owner(conf, filename, pkg);
		    continue;
	       }


	       /* Pre-existing files are OK if owned by a package replaced by new pkg. */
	       if (owner) {
		    if (pkg_replaces(pkg, owner)) {
/* It's now time to change the owner of that file. 
   It has been "replaced" from the new "Replaces", then I need to inform lists file about that.  */
			 opkg_message(conf, OPKG_INFO, "Replacing pre-existing file %s owned by package %s\n", filename, owner->name);
		         file_hash_set_file_owner(conf, filename, pkg);
			 continue;
		    }
	       }

	  }
     }
     if (root_filename) {
         free(root_filename);
         root_filename = NULL;
     }
     pkg_free_installed_files(pkg);

     return 0;
}

static int
check_data_file_clashes_unwind(opkg_conf_t *conf, pkg_t *pkg, pkg_t *old_pkg)
{
     /* Nothing to do since check_data_file_clashes doesn't change state */
     return 0;
}

static int
postrm_upgrade_old_pkg(opkg_conf_t *conf, pkg_t *pkg, pkg_t *old_pkg)
{
     /* DPKG_INCOMPATIBILITY: dpkg does the following here, should we?
	1. If the package is being upgraded, call
	   old-postrm upgrade new-version
	2. If this fails, attempt:
	   new-postrm failed-upgrade old-version
	Error unwind, for both cases:
	   old-preinst abort-upgrade new-version    */
     return 0;
}

static int
postrm_upgrade_old_pkg_unwind(opkg_conf_t *conf, pkg_t *pkg, pkg_t *old_pkg)
{
     /* DPKG_INCOMPATIBILITY:
	dpkg does some things here that we don't do yet. Do we care?
	(See postrm_upgrade_old_pkg for details)
     */
    return 0;
}

static int
remove_obsolesced_files(opkg_conf_t *conf, pkg_t *pkg, pkg_t *old_pkg)
{
     int err;
     str_list_t *old_files;
     str_list_elt_t *of;
     str_list_t *new_files;
     str_list_elt_t *nf;
     hash_table_t new_files_table;

     old_files = pkg_get_installed_files(conf, old_pkg);
     if (old_files == NULL)
	  return -1;

     new_files = pkg_get_installed_files(conf, pkg);
     if (new_files == NULL) {
          pkg_free_installed_files(old_pkg);
	  return -1;
     }

     new_files_table.entries = NULL;
     hash_table_init("new_files" , &new_files_table, 20);
     for (nf = str_list_first(new_files); nf; nf = str_list_next(new_files, nf)) {
         if (nf && nf->data)
            hash_table_insert(&new_files_table, nf->data, nf->data);
     }

     for (of = str_list_first(old_files); of; of = str_list_next(old_files, of)) {
	  pkg_t *owner;
	  char *old, *new;
	  old = (char *)of->data;
          new = (char *) hash_table_get (&new_files_table, old);
          if (new)
               continue;

	  if (file_is_dir(old)) {
	       continue;
	  }
	  owner = file_hash_get_file_owner(conf, old);
	  if (owner != old_pkg) {
	       /* in case obsolete file no longer belongs to old_pkg */
	       continue;
	  }
 
	  /* old file is obsolete */
	  opkg_message(conf, OPKG_INFO,
		       "    removing obsolete file %s\n", old);
	  if (!conf->noaction) {
	       err = unlink(old);
	       if (err) {
		    opkg_message(conf, OPKG_ERROR, "    Warning: remove %s failed: %s\n", old,
				 strerror(errno));
	       }
	  }
     }

     hash_table_deinit(&new_files_table);
     pkg_free_installed_files(old_pkg);
     pkg_free_installed_files(pkg);

     return 0;
}

static int
install_maintainer_scripts(opkg_conf_t *conf, pkg_t *pkg, pkg_t *old_pkg)
{
     int ret;
     char *prefix;

     sprintf_alloc(&prefix, "%s.", pkg->name);
     ret = pkg_extract_control_files_to_dir_with_prefix(pkg,
							pkg->dest->info_dir,
							prefix);
     free(prefix);
     return ret;
}

static int
remove_disappeared(opkg_conf_t *conf, pkg_t *pkg)
{
     /* DPKG_INCOMPATIBILITY:
	This is a fairly sophisticated dpkg operation. Shall we
	skip it? */
     
     /* Any packages all of whose files have been overwritten during the
	installation, and which aren't required for dependencies, are
	considered to have been removed. For each such package
	1. disappearer's-postrm disappear overwriter overwriter-version
	2. The package's maintainer scripts are removed
	3. It is noted in the status database as being in a sane state,
           namely not installed (any conffiles it may have are ignored,
	   rather than being removed by dpkg). Note that disappearing
	   packages do not have their prerm called, because dpkg doesn't
	   know in advance that the package is going to vanish.
     */
     return 0;
}

static int
install_data_files(opkg_conf_t *conf, pkg_t *pkg)
{
     int err;

     /* opkg takes a slightly different approach to data file backups
	than dpkg. Rather than removing backups at this point, we
	actually do the data file installation now. See comments in
	check_data_file_clashes() for more details. */
    
     opkg_message(conf, OPKG_INFO,
		  "    extracting data files to %s\n", pkg->dest->root_dir);
     err = pkg_extract_data_files_to_dir(pkg, pkg->dest->root_dir);
     if (err) {
	  return err;
     }

     /* XXX: BUG or FEATURE : We are actually loosing the Essential flag,
        so we can't save ourself from removing important packages
        At this point we (should) have extracted the .control file, so it
        would be a good idea to reload the data in it, and set the Essential 
        state in *pkg. From now on the Essential is back in status file and
        we can protect again.
        We should operate this way:
        fopen the file ( pkg->dest->root_dir/pkg->name.control )
        check for "Essential" in it 
        set the value in pkg->essential.
        This new routine could be useful also for every other flag
        Pigi: 16/03/2004 */
     set_flags_from_control(conf, pkg) ;
     
     opkg_message(conf, OPKG_DEBUG, "    Calling pkg_write_filelist from %s\n", __FUNCTION__);
     err = pkg_write_filelist(conf, pkg);
     if (err)
	  return err;

     /* XXX: FEATURE: opkg should identify any files which existed
	before installation and which were overwritten, (see
	check_data_file_clashes()). What it must do is remove any such
	files from the filelist of the old package which provided the
	file. Otherwise, if the old package were removed at some point
	it would break the new package. Removing the new package will
	also break the old one, but this cannot be helped since the old
	package's file has already been deleted. This is the importance
	of check_data_file_clashes(), and only allowing opkg to install
	a clashing package with a user force. */

     return 0;
}

static int
user_prefers_old_conffile(const char *file_name, const char *backup)
{
     char *response;
     const char *short_file_name;

     short_file_name = strrchr(file_name, '/');
     if (short_file_name) {
	  short_file_name++;
     } else {
	  short_file_name = file_name;
     }

     while (1) {
	  response = get_user_response("    Configuration file '%s'\n"
				       "    ==> File on system created by you or by a script.\n"
				       "    ==> File also in package provided by package maintainer.\n"
				       "       What would you like to do about it ?  Your options are:\n"
				       "        Y or I  : install the package maintainer's version\n"
				       "        N or O  : keep your currently-installed version\n"
				       "          D     : show the differences between the versions (if diff is installed)\n"
				       "     The default action is to keep your current version.\n"
				       "    *** %s (Y/I/N/O/D) [default=N] ? ", file_name, short_file_name);

	  if (response == NULL)
		  return 1;

	  if (strcmp(response, "y") == 0
	      || strcmp(response, "i") == 0
	      || strcmp(response, "yes") == 0) {
	       free(response);
	       return 0;
	  }

	  if (strcmp(response, "d") == 0) {
	       const char *argv[] = {"diff", "-u", backup, file_name, NULL};
	       xsystem(argv);
	       printf("    [Press ENTER to continue]\n");
	       response = file_read_line_alloc(stdin);
	       free(response);
	       continue;
	  }

	  free(response);
	  return 1;
     }
}

static int
resolve_conffiles(opkg_conf_t *conf, pkg_t *pkg)
{
     conffile_list_elt_t *iter;
     conffile_t *cf;
     char *cf_backup;
     char *md5sum;

     if (conf->noaction) return 0;

     for (iter = nv_pair_list_first(&pkg->conffiles); iter; iter = nv_pair_list_next(&pkg->conffiles, iter)) {
	  char *root_filename;
	  cf = (conffile_t *)iter->data;
	  root_filename = root_filename_alloc(conf, cf->name);

	  /* Might need to initialize the md5sum for each conffile */
	  if (cf->value == NULL) {
	       cf->value = file_md5sum_alloc(root_filename);
	  }

	  if (!file_exists(root_filename)) {
	       free(root_filename);
	       continue;
	  }

	  cf_backup = backup_filename_alloc(root_filename);


          if (file_exists(cf_backup)) {
              /* Let's compute md5 to test if files are changed */
              md5sum = file_md5sum_alloc(cf_backup);
              if (md5sum && cf->value && strcmp(cf->value,md5sum) != 0 ) {
                  if (conf->force_maintainer) {
                      opkg_message(conf, OPKG_NOTICE, "Conffile %s using maintainer's setting.\n", cf_backup);
                  } else if (conf->force_defaults
                          || user_prefers_old_conffile(root_filename, cf_backup) ) {
                      rename(cf_backup, root_filename);
                  }
              }
              unlink(cf_backup);
	      if (md5sum)
                  free(md5sum);
          }

	  free(cf_backup);
	  free(root_filename);
     }

     return 0;
}


int
opkg_install_by_name(opkg_conf_t *conf, const char *pkg_name)
{
     int cmp;
     pkg_t *old, *new;
     char *old_version, *new_version;

     old = pkg_hash_fetch_installed_by_name(&conf->pkg_hash, pkg_name);
     if (old)
        opkg_message(conf, OPKG_DEBUG2, "Old versions from pkg_hash_fetch %s \n",  old->version);
    
     new = pkg_hash_fetch_best_installation_candidate_by_name(conf, pkg_name);
     if (new == NULL)
	return -1;

     opkg_message(conf, OPKG_DEBUG2, "Versions from pkg_hash_fetch in %s ", __FUNCTION__);
     if ( old ) 
        opkg_message(conf, OPKG_DEBUG2, " old %s ", old->version);
     opkg_message(conf, OPKG_DEBUG2, " new %s\n", new->version);

     new->state_flag |= SF_USER;
     if (old) {
	  old_version = pkg_version_str_alloc(old);
	  new_version = pkg_version_str_alloc(new);

	  cmp = pkg_compare_versions(old, new);
          if ( (conf->force_downgrade==1) && (cmp > 0) ){     /* We've been asked to allow downgrade  and version is precedent */
	     opkg_message(conf, OPKG_DEBUG, " Forcing downgrade \n");
             cmp = -1 ;                                       /* then we force opkg to downgrade */ 
                                                              /* We need to use a value < 0 because in the 0 case we are asking to */
                                                              /* reinstall, and some check could fail asking the "force-reinstall" option */
          } 
	  opkg_message(conf, OPKG_DEBUG, 
		       "Comparing visible versions of pkg %s:"
		       "\n\t%s is installed "
		       "\n\t%s is available "
		       "\n\t%d was comparison result\n",
		       pkg_name, old_version, new_version, cmp);
	  if (cmp == 0 && !conf->force_reinstall) {
	       opkg_message(conf, OPKG_NOTICE,
			    "Package %s (%s) installed in %s is up to date.\n",
			    old->name, old_version, old->dest->name);
	       free(old_version);
	       free(new_version);
	       return 0;
	  } else if (cmp > 0) {
	       opkg_message(conf, OPKG_NOTICE,
			    "Not downgrading package %s on %s from %s to %s.\n",
			    old->name, old->dest->name, old_version, new_version);
	       free(old_version);
	       free(new_version);
	       return 0;
	  } else if (cmp < 0) {
	       new->dest = old->dest;
	       old->state_want = SW_DEINSTALL;    /* Here probably the problem for bug 1277 */
	  }
	  free(old_version);
	  free(new_version);
     }

     opkg_message(conf, OPKG_DEBUG2,"%s: calling opkg_install_pkg \n",__FUNCTION__);
     return opkg_install_pkg(conf, new,0);
}

/**
 *  @brief Really install a pkg_t 
 */
int
opkg_install_pkg(opkg_conf_t *conf, pkg_t *pkg, int from_upgrade)
{
     int err = 0;
     int message = 0;
     pkg_t *old_pkg = NULL;
     pkg_vec_t *replacees;
     abstract_pkg_t *ab_pkg = NULL;
     int old_state_flag;
     char* file_md5;
#ifdef HAVE_SHA256
     char* file_sha256;
#endif
     sigset_t newset, oldset;

     if ( from_upgrade ) 
        message = 1;            /* Coming from an upgrade, and should change the output message */

     if (!pkg) {
	  opkg_message(conf, OPKG_ERROR,
		       "INTERNAL ERROR: null pkg passed to opkg_install_pkg\n");
	  return -1;
     }

     opkg_message(conf, OPKG_DEBUG2, "Function: %s calling pkg_arch_supported %s \n", __FUNCTION__, __FUNCTION__);

     if (!pkg_arch_supported(conf, pkg)) {
	  opkg_message(conf, OPKG_ERROR, "INTERNAL ERROR: architecture %s for pkg %s is unsupported.\n",
		       pkg->architecture, pkg->name);
	  return -1;
     }
     if (pkg->state_status == SS_INSTALLED && conf->force_reinstall == 0 && conf->nodeps == 0) {
	  err = satisfy_dependencies_for(conf, pkg);
	  if (err)
		  return -1;

	  opkg_message(conf, OPKG_NOTICE,
		       "Package %s is already installed in %s.\n", 
		       pkg->name, pkg->dest->name);
	  return 0;
     }

     if (pkg->dest == NULL) {
	  pkg->dest = conf->default_dest;
     }

     old_pkg = pkg_hash_fetch_installed_by_name(&conf->pkg_hash, pkg->name);

     err = opkg_install_check_downgrade(conf, pkg, old_pkg, message);
     if (err)
	     return -1;

     pkg->state_want = SW_INSTALL;
     if (old_pkg){                          
         old_pkg->state_want = SW_DEINSTALL; /* needed for check_data_file_clashes of dependences */
     }

     err = check_conflicts_for(conf, pkg);
     if (err)
	     return -1;
    
     /* this setup is to remove the upgrade scenario in the end when
	installing pkg A, A deps B & B deps on A. So both B and A are
	installed. Then A's installation is started resulting in an
	uncecessary upgrade */ 
     if (pkg->state_status == SS_INSTALLED && conf->force_reinstall == 0)
	     return 0;
    
     err = verify_pkg_installable(conf, pkg);
     if (err)
	     return -1;

     if (pkg->local_filename == NULL) {
	  err = opkg_download_pkg(conf, pkg, conf->tmp_dir);
	  if (err) {
	       opkg_message(conf, OPKG_ERROR,
			    "Failed to download %s. Perhaps you need to run 'opkg update'?\n",
			    pkg->name);
	       return -1;
	  }
     }

     /* check that the repository is valid */
     #if defined(HAVE_GPGME) || defined(HAVE_OPENSSL)
     char *list_file_name, *sig_file_name, *lists_dir;

     /* check to ensure the package has come from a repository */
     if (conf->check_signature && pkg->src)
     {
       sprintf_alloc (&lists_dir, "%s",
                     (conf->restrict_to_default_dest)
                      ? conf->default_dest->lists_dir
                      : conf->lists_dir);
       sprintf_alloc (&list_file_name, "%s/%s", lists_dir, pkg->src->name);
       sprintf_alloc (&sig_file_name, "%s/%s.sig", lists_dir, pkg->src->name);

       if (file_exists (sig_file_name))
       {
         if (opkg_verify_file (conf, list_file_name, sig_file_name)){
           opkg_message(conf, OPKG_ERROR, "Failed to verify the signature of: %s\n",
                           list_file_name);
           return -1;
         }
       }else{
         opkg_message(conf, OPKG_ERROR, "Signature file is missing. "
                         "Perhaps you need to run 'opkg update'?\n");
         return -1;
       }

       free (lists_dir);
       free (list_file_name);
       free (sig_file_name);
     }
     #endif

     /* Check for md5 values */
     if (pkg->md5sum)
     {
         file_md5 = file_md5sum_alloc(pkg->local_filename);
         if (file_md5 && strcmp(file_md5, pkg->md5sum))
         {
              opkg_message(conf, OPKG_ERROR,
                           "Package %s md5sum mismatch. Either the opkg or the package index are corrupt. Try 'opkg update'.\n",
                           pkg->name);
              free(file_md5);
              return -1;
         }
	 if (file_md5)
              free(file_md5);
     }

#ifdef HAVE_SHA256
     /* Check for sha256 value */
     if(pkg->sha256sum)
     {
         file_sha256 = file_sha256sum_alloc(pkg->local_filename);
         if (file_sha256 && strcmp(file_sha256, pkg->sha256sum))
         {
              opkg_message(conf, OPKG_ERROR,
                           "Package %s sha256sum mismatch. Either the opkg or the package index are corrupt. Try 'opkg update'.\n",
                           pkg->name);
              free(file_sha256);
              return -1;
         }
	 if (file_sha256)
              free(file_sha256);
     }
#endif

     if (pkg->tmp_unpack_dir == NULL) {
	  if (unpack_pkg_control_files(conf, pkg) == -1) {
	       opkg_message(conf, OPKG_ERROR, "Failed to unpack control"
			      " files from %s.\n", pkg->local_filename);
	       return -1;
	  }
     }

     /* We should update the filelist here, so that upgrades of packages that split will not fail. -Jamey 27-MAR-03 */
/* Pigi: check if it will pass from here when replacing. It seems to fail */
/* That's rather strange that files don't change owner. Investigate !!!!!!*/
     err = update_file_ownership(conf, pkg, old_pkg);
     if (err)
	     return -1;

     if (conf->nodeps == 0) {
	  err = satisfy_dependencies_for(conf, pkg);
	  if (err)
		return -1;
          if (pkg->state_status == SS_UNPACKED)
               /* Circular dependency has installed it for us. */
		return 0;
     }

     replacees = pkg_vec_alloc();
     pkg_get_installed_replacees(conf, pkg, replacees);

     /* this next section we do with SIGINT blocked to prevent inconsistency between opkg database and filesystem */

	  sigemptyset(&newset);
	  sigaddset(&newset, SIGINT);
	  sigprocmask(SIG_BLOCK, &newset, &oldset);

	  opkg_state_changed++;
	  pkg->state_flag |= SF_FILELIST_CHANGED;

	  if (old_pkg)
               pkg_remove_orphan_dependent(conf, pkg, old_pkg);

	  /* XXX: BUG: we really should treat replacement more like an upgrade
	   *      Instead, we're going to remove the replacees 
	   */
	  err = pkg_remove_installed_replacees(conf, replacees);
	  if (err)
		  goto UNWIND_REMOVE_INSTALLED_REPLACEES;

	  err = prerm_upgrade_old_pkg(conf, pkg, old_pkg);
	  if (err)
		  goto UNWIND_PRERM_UPGRADE_OLD_PKG;

	  err = prerm_deconfigure_conflictors(conf, pkg, replacees);
	  if (err)
		  goto UNWIND_PRERM_DECONFIGURE_CONFLICTORS;

	  err = preinst_configure(conf, pkg, old_pkg);
	  if (err)
		  goto UNWIND_PREINST_CONFIGURE;

	  err = backup_modified_conffiles(conf, pkg, old_pkg);
	  if (err)
		  goto UNWIND_BACKUP_MODIFIED_CONFFILES;

	  err = check_data_file_clashes(conf, pkg, old_pkg);
	  if (err)
		  goto UNWIND_CHECK_DATA_FILE_CLASHES;

	  err = postrm_upgrade_old_pkg(conf, pkg, old_pkg);
	  if (err)
		  goto UNWIND_POSTRM_UPGRADE_OLD_PKG;

	  if (conf->noaction)
		  return 0;

	  /* point of no return: no unwinding after this */
	  if (old_pkg && !conf->force_reinstall) {
	       old_pkg->state_want = SW_DEINSTALL;

	       if (old_pkg->state_flag & SF_NOPRUNE) {
		    opkg_message(conf, OPKG_INFO,
				 "  not removing obsolesced files because package marked noprune\n");
	       } else {
		    opkg_message(conf, OPKG_INFO,
				 "  removing obsolesced files\n");
		    if (remove_obsolesced_files(conf, pkg, old_pkg)) {
			opkg_message(conf, OPKG_ERROR, "Failed to determine "
					"obsolete files from previously "
					"installed %s\n", old_pkg->name);
		    }
	       }

               /* removing files from old package, to avoid ghost files */ 
               remove_data_files_and_list(conf, old_pkg);
               remove_maintainer_scripts(conf, old_pkg);
	  }


	  opkg_message(conf, OPKG_INFO,
		       "  installing maintainer scripts\n");
	  if (install_maintainer_scripts(conf, pkg, old_pkg)) {
		opkg_message(conf, OPKG_ERROR, "Failed to extract maintainer"
			       " scripts for %s. Package debris may remain!\n",
			       pkg->name);
		goto pkg_is_hosed;
	  }

	  /* the following just returns 0 */
	  remove_disappeared(conf, pkg);

	  opkg_message(conf, OPKG_INFO,
		       "  installing data files\n");

	  if (install_data_files(conf, pkg)) {
		opkg_message(conf, OPKG_ERROR, "Failed to extract data files "
			       "for %s. Package debris may remain!\n",
			       pkg->name);
		goto pkg_is_hosed;
	  }

	  err = check_data_file_clashes_change(conf, pkg, old_pkg);
	  if (err) {
		opkg_message(conf, OPKG_ERROR,
				"check_data_file_clashes_change() failed for "
			       "for files belonging to %s.\n",
			       pkg->name);
	  }

	  opkg_message(conf, OPKG_INFO,
		       "  resolving conf files\n");
	  resolve_conffiles(conf, pkg);

	  pkg->state_status = SS_UNPACKED;
	  old_state_flag = pkg->state_flag;
	  pkg->state_flag &= ~SF_PREFER;
	  opkg_message(conf, OPKG_DEBUG, "   pkg=%s old_state_flag=%x state_flag=%x\n", pkg->name, old_state_flag, pkg->state_flag);

	  if (old_pkg && !conf->force_reinstall) {
	       old_pkg->state_status = SS_NOT_INSTALLED;
	  }

	  time(&pkg->installed_time);

	  ab_pkg = pkg->parent;
	  if (ab_pkg)
	       ab_pkg->state_status = pkg->state_status;

	  opkg_message(conf, OPKG_INFO, "Done.\n");

	  sigprocmask(SIG_UNBLOCK, &newset, &oldset);
          pkg_vec_free (replacees);
	  return 0;
     

     UNWIND_POSTRM_UPGRADE_OLD_PKG:
	  postrm_upgrade_old_pkg_unwind(conf, pkg, old_pkg);
     UNWIND_CHECK_DATA_FILE_CLASHES:
	  check_data_file_clashes_unwind(conf, pkg, old_pkg);
     UNWIND_BACKUP_MODIFIED_CONFFILES:
	  backup_modified_conffiles_unwind(conf, pkg, old_pkg);
     UNWIND_PREINST_CONFIGURE:
	  preinst_configure_unwind(conf, pkg, old_pkg);
     UNWIND_PRERM_DECONFIGURE_CONFLICTORS:
	  prerm_deconfigure_conflictors_unwind(conf, pkg, replacees);
     UNWIND_PRERM_UPGRADE_OLD_PKG:
	  prerm_upgrade_old_pkg_unwind(conf, pkg, old_pkg);
     UNWIND_REMOVE_INSTALLED_REPLACEES:
	  pkg_remove_installed_replacees_unwind(conf, replacees);

pkg_is_hosed:
	  opkg_message(conf, OPKG_INFO,
		       "Failed.\n");

	  sigprocmask(SIG_UNBLOCK, &newset, &oldset);

          pkg_vec_free (replacees);
	  return -1;
}
