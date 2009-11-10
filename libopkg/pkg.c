/* pkg.c - the opkg package management system

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
#include <ctype.h>
#include <alloca.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include "pkg.h"

#include "pkg_parse.h"
#include "pkg_extract.h"
#include "opkg_message.h"
#include "opkg_utils.h"

#include "libbb/libbb.h"
#include "sprintf_alloc.h"
#include "file_util.h"
#include "str_util.h"
#include "xsystem.h"
#include "opkg_conf.h"

typedef struct enum_map enum_map_t;
struct enum_map
{
     int value;
     char *str;
};

static const enum_map_t pkg_state_want_map[] = {
     { SW_UNKNOWN, "unknown"},
     { SW_INSTALL, "install"},
     { SW_DEINSTALL, "deinstall"},
     { SW_PURGE, "purge"}
};

static const enum_map_t pkg_state_flag_map[] = {
     { SF_OK, "ok"},
     { SF_REINSTREQ, "reinstreq"},
     { SF_HOLD, "hold"},
     { SF_REPLACE, "replace"},
     { SF_NOPRUNE, "noprune"},
     { SF_PREFER, "prefer"},
     { SF_OBSOLETE, "obsolete"},
     { SF_USER, "user"},
};

static const enum_map_t pkg_state_status_map[] = {
     { SS_NOT_INSTALLED, "not-installed" },
     { SS_UNPACKED, "unpacked" },
     { SS_HALF_CONFIGURED, "half-configured" },
     { SS_INSTALLED, "installed" },
     { SS_HALF_INSTALLED, "half-installed" },
     { SS_CONFIG_FILES, "config-files" },
     { SS_POST_INST_FAILED, "post-inst-failed" },
     { SS_REMOVAL_FAILED, "removal-failed" }
};

static int verrevcmp(const char *val, const char *ref);


pkg_t *pkg_new(void)
{
     pkg_t *pkg;

     pkg = xcalloc(1, sizeof(pkg_t));
     pkg_init(pkg);

     return pkg;
}

int pkg_init(pkg_t *pkg)
{
     pkg->name = NULL;
     pkg->epoch = 0;
     pkg->version = NULL;
     pkg->revision = NULL;
     pkg->dest = NULL;
     pkg->src = NULL;
     pkg->architecture = NULL;
     pkg->maintainer = NULL;
     pkg->section = NULL;
     pkg->description = NULL;
     pkg->state_want = SW_UNKNOWN;
     pkg->state_flag = SF_OK;
     pkg->state_status = SS_NOT_INSTALLED;
     pkg->depends_str = NULL;
     pkg->provides_str = NULL;
     pkg->depends_count = 0;
     pkg->depends = NULL;
     pkg->suggests_str = NULL;
     pkg->recommends_str = NULL;
     pkg->suggests_count = 0;
     pkg->recommends_count = 0;
     
     active_list_init(&pkg->list);

     /* Abhaya: added init for conflicts fields */
     pkg->conflicts = NULL;
     pkg->conflicts_count = 0;

     /* added for replaces.  Jamey 7/23/2002 */
     pkg->replaces = NULL;
     pkg->replaces_count = 0;
    
     pkg->pre_depends_count = 0;
     pkg->pre_depends_str = NULL;
     pkg->provides_count = 0;
     pkg->provides = NULL;
     pkg->filename = NULL;
     pkg->local_filename = NULL;
     pkg->tmp_unpack_dir = NULL;
     pkg->md5sum = NULL;
#if defined HAVE_SHA256
     pkg->sha256sum = NULL;
#endif
     pkg->size = NULL;
     pkg->installed_size = NULL;
     pkg->priority = NULL;
     pkg->source = NULL;
     conffile_list_init(&pkg->conffiles);
     pkg->installed_files = NULL;
     pkg->installed_files_ref_cnt = 0;
     pkg->essential = 0;
     pkg->provided_by_hand = 0;

     return 0;
}

void compound_depend_deinit (compound_depend_t *depends)
{
    int i;
    for (i = 0; i < depends->possibility_count; i++)
    {
        depend_t *d;
        d = depends->possibilities[i];
        free (d->version);
        free (d);
    }
    free (depends->possibilities);
}

void pkg_deinit(pkg_t *pkg)
{
	int i;

	if (pkg->name)
		free(pkg->name);
	pkg->name = NULL;

	pkg->epoch = 0;

	if (pkg->version)
		free(pkg->version);
	pkg->version = NULL;
	/* revision shares storage with version, so don't free */
	pkg->revision = NULL;

	/* owned by opkg_conf_t */
	pkg->dest = NULL;
	/* owned by opkg_conf_t */
	pkg->src = NULL;

	if (pkg->architecture)
		free(pkg->architecture);
	pkg->architecture = NULL;

	if (pkg->maintainer)
		free(pkg->maintainer);
	pkg->maintainer = NULL;

	if (pkg->section)
		free(pkg->section);
	pkg->section = NULL;

	if (pkg->description)
		free(pkg->description);
	pkg->description = NULL;
	
	pkg->state_want = SW_UNKNOWN;
	pkg->state_flag = SF_OK;
	pkg->state_status = SS_NOT_INSTALLED;

	active_list_clear(&pkg->list);

	if (pkg->replaces)
		free (pkg->replaces);
	pkg->replaces = NULL;

	for (i = 0; i < pkg->depends_count; i++)
		free (pkg->depends_str[i]);
	free(pkg->depends_str);
	pkg->depends_str = NULL;

	for (i = 0; i < pkg->provides_count-1; i++)
		free (pkg->provides_str[i]);
	free(pkg->provides_str);
	pkg->provides_str = NULL;

	for (i = 0; i < pkg->conflicts_count; i++)
		free (pkg->conflicts_str[i]);
	free(pkg->conflicts_str);
	pkg->conflicts_str = NULL;

	for (i = 0; i < pkg->replaces_count; i++)
		free (pkg->replaces_str[i]);
	free(pkg->replaces_str);
	pkg->replaces_str = NULL;

	for (i = 0; i < pkg->recommends_count; i++)
		free (pkg->recommends_str[i]);
	free(pkg->recommends_str);
	pkg->recommends_str = NULL;

	for (i = 0; i < pkg->suggests_count; i++)
		free (pkg->suggests_str[i]);
	free(pkg->suggests_str);
	pkg->suggests_str = NULL;

	if (pkg->depends) {
		int count = pkg->pre_depends_count
				+ pkg->depends_count
				+ pkg->recommends_count
				+ pkg->suggests_count;

		for (i=0; i<count; i++)
			compound_depend_deinit (&pkg->depends[i]);
		free (pkg->depends);
	}

	if (pkg->conflicts) {
		for (i=0; i<pkg->conflicts_count; i++)
			compound_depend_deinit (&pkg->conflicts[i]);
		free (pkg->conflicts);
	}

	if (pkg->provides)
		free (pkg->provides);

	pkg->pre_depends_count = 0;
	if (pkg->pre_depends_str)
		free(pkg->pre_depends_str);
	pkg->pre_depends_str = NULL;
	
	pkg->provides_count = 0;
	
	if (pkg->filename)
		free(pkg->filename);
	pkg->filename = NULL;
	
	if (pkg->local_filename)
		free(pkg->local_filename);
	pkg->local_filename = NULL;

     /* CLEANUP: It'd be nice to pullin the cleanup function from
	opkg_install.c here. See comment in
	opkg_install.c:cleanup_temporary_files */
	if (pkg->tmp_unpack_dir)
		free(pkg->tmp_unpack_dir);
	pkg->tmp_unpack_dir = NULL;

	if (pkg->md5sum)
		free(pkg->md5sum);
	pkg->md5sum = NULL;

#if defined HAVE_SHA256
	if (pkg->sha256sum)
		free(pkg->sha256sum);
	pkg->sha256sum = NULL;
#endif

	if (pkg->size)
		free(pkg->size);
	pkg->size = NULL;

	if (pkg->installed_size)
		free(pkg->installed_size);
	pkg->installed_size = NULL;

	if (pkg->priority)
		free(pkg->priority);
	pkg->priority = NULL;

	if (pkg->source)
		free(pkg->source);
	pkg->source = NULL;

	conffile_list_deinit(&pkg->conffiles);

	/* XXX: QUESTION: Is forcing this to 1 correct? I suppose so,
	since if they are calling deinit, they should know. Maybe do an
	assertion here instead? */
	pkg->installed_files_ref_cnt = 1;
	pkg_free_installed_files(pkg);
	pkg->essential = 0;

	if (pkg->tags)
		free (pkg->tags);
	pkg->tags = NULL;
}

int pkg_init_from_file(pkg_t *pkg, const char *filename)
{
     int err;
     char **raw, **raw_start;
     FILE *control_file;

     err = pkg_init(pkg);
     if (err) { return err; }

     pkg->local_filename = xstrdup(filename);
    
     control_file = tmpfile();
     err = pkg_extract_control_file_to_stream(pkg, control_file);
     if (err) { return err; }

     rewind(control_file);
     raw = raw_start = read_raw_pkgs_from_stream(control_file);
     pkg_parse_raw(pkg, &raw, NULL, NULL);

     fclose(control_file);

     raw = raw_start;
     while (*raw) {
	  free(*raw++);
     }
     free(raw_start);

     return 0;
}

/* Merge any new information in newpkg into oldpkg */
/* XXX: CLEANUP: This function shouldn't actually modify anything in
   newpkg, but should leave it usable. This rework is so that
   pkg_hash_insert doesn't clobber the pkg that you pass into it. */
/* 
 * uh, i thought that i had originally written this so that it took 
 * two pkgs and returned a new one?  we can do that again... -sma
 */
int pkg_merge(pkg_t *oldpkg, pkg_t *newpkg, int set_status)
{
     if (oldpkg == newpkg) {
	  return 0;
     }

     if (!oldpkg->src)
	  oldpkg->src = newpkg->src;
     if (!oldpkg->dest)
	  oldpkg->dest = newpkg->dest;
     if (!oldpkg->architecture)
	  oldpkg->architecture = xstrdup(newpkg->architecture);
     if (!oldpkg->arch_priority)
	  oldpkg->arch_priority = newpkg->arch_priority;
     if (!oldpkg->section)
	  oldpkg->section = xstrdup(newpkg->section);
     if(!oldpkg->maintainer)
	  oldpkg->maintainer = xstrdup(newpkg->maintainer);
     if(!oldpkg->description)
	  oldpkg->description = xstrdup(newpkg->description);
     if (set_status) {
	  /* merge the state_flags from the new package */
	  oldpkg->state_want = newpkg->state_want;
	  oldpkg->state_status = newpkg->state_status;
	  oldpkg->state_flag = newpkg->state_flag;
     } else {
	  if (oldpkg->state_want == SW_UNKNOWN)
	       oldpkg->state_want = newpkg->state_want;
	  if (oldpkg->state_status == SS_NOT_INSTALLED)
	       oldpkg->state_status = newpkg->state_status;
	  oldpkg->state_flag |= newpkg->state_flag;
     }

     if (!oldpkg->depends_str && !oldpkg->pre_depends_str && !oldpkg->recommends_str && !oldpkg->suggests_str) {
	  oldpkg->depends_str = newpkg->depends_str;
	  newpkg->depends_str = NULL;
	  oldpkg->depends_count = newpkg->depends_count;
	  newpkg->depends_count = 0;

	  oldpkg->depends = newpkg->depends;
	  newpkg->depends = NULL;

	  oldpkg->pre_depends_str = newpkg->pre_depends_str;
	  newpkg->pre_depends_str = NULL;
	  oldpkg->pre_depends_count = newpkg->pre_depends_count;
	  newpkg->pre_depends_count = 0;

	  oldpkg->recommends_str = newpkg->recommends_str;
	  newpkg->recommends_str = NULL;
	  oldpkg->recommends_count = newpkg->recommends_count;
	  newpkg->recommends_count = 0;

	  oldpkg->suggests_str = newpkg->suggests_str;
	  newpkg->suggests_str = NULL;
	  oldpkg->suggests_count = newpkg->suggests_count;
	  newpkg->suggests_count = 0;
     }

     if (!oldpkg->provides_str) {
	  oldpkg->provides_str = newpkg->provides_str;
	  newpkg->provides_str = NULL;
	  oldpkg->provides_count = newpkg->provides_count;
	  newpkg->provides_count = 0;

	  oldpkg->provides = newpkg->provides;
	  newpkg->provides = NULL;
     }

     if (!oldpkg->conflicts_str) {
	  oldpkg->conflicts_str = newpkg->conflicts_str;
	  newpkg->conflicts_str = NULL;
	  oldpkg->conflicts_count = newpkg->conflicts_count;
	  newpkg->conflicts_count = 0;

	  oldpkg->conflicts = newpkg->conflicts;
	  newpkg->conflicts = NULL;
     }

     if (!oldpkg->replaces_str) {
	  oldpkg->replaces_str = newpkg->replaces_str;
	  newpkg->replaces_str = NULL;
	  oldpkg->replaces_count = newpkg->replaces_count;
	  newpkg->replaces_count = 0;

	  oldpkg->replaces = newpkg->replaces;
	  newpkg->replaces = NULL;
     }

     if (!oldpkg->filename)
	  oldpkg->filename = xstrdup(newpkg->filename);
     if (!oldpkg->local_filename)
	  oldpkg->local_filename = xstrdup(newpkg->local_filename);
     if (!oldpkg->tmp_unpack_dir)
	  oldpkg->tmp_unpack_dir = xstrdup(newpkg->tmp_unpack_dir);
     if (!oldpkg->md5sum)
	  oldpkg->md5sum = xstrdup(newpkg->md5sum);
#if defined HAVE_SHA256
     if (!oldpkg->sha256sum)
	  oldpkg->sha256sum = xstrdup(newpkg->sha256sum);
#endif
     if (!oldpkg->size)
	  oldpkg->size = xstrdup(newpkg->size);
     if (!oldpkg->installed_size)
	  oldpkg->installed_size = xstrdup(newpkg->installed_size);
     if (!oldpkg->priority)
	  oldpkg->priority = xstrdup(newpkg->priority);
     if (!oldpkg->source)
	  oldpkg->source = xstrdup(newpkg->source);
     if (nv_pair_list_empty(&oldpkg->conffiles)){
	  list_splice_init(&newpkg->conffiles.head, &oldpkg->conffiles.head);
	  conffile_list_init(&newpkg->conffiles);
     }
     if (!oldpkg->installed_files){
	  oldpkg->installed_files = newpkg->installed_files;
	  oldpkg->installed_files_ref_cnt = newpkg->installed_files_ref_cnt;
	  newpkg->installed_files = NULL;
     }
     if (!oldpkg->essential)
	  oldpkg->essential = newpkg->essential;

     return 0;
}

abstract_pkg_t *abstract_pkg_new(void)
{
     abstract_pkg_t * ab_pkg;

     ab_pkg = xcalloc(1, sizeof(abstract_pkg_t));

     if (ab_pkg == NULL) {
	  fprintf(stderr, "%s: out of memory\n", __FUNCTION__);
	  return NULL;
     }

     if ( abstract_pkg_init(ab_pkg) < 0 ) 
        return NULL;

     return ab_pkg;
}

int abstract_pkg_init(abstract_pkg_t *ab_pkg)
{
     ab_pkg->provided_by = abstract_pkg_vec_alloc();
     if (ab_pkg->provided_by==NULL){
        return -1;
     }
     ab_pkg->dependencies_checked = 0;
     ab_pkg->state_status = SS_NOT_INSTALLED;

     return 0;
}

void set_flags_from_control(opkg_conf_t *conf, pkg_t *pkg){
     char * temp_str;
     char **raw =NULL;
     char **raw_start=NULL; 

     size_t str_size = strlen(pkg->dest->info_dir)+strlen(pkg->name)+12;
     temp_str = (char *) alloca (str_size);
     memset(temp_str, 0 , str_size);
     
     if (temp_str == NULL ){
        opkg_message(conf, OPKG_INFO, "Out of memory in  %s\n", __FUNCTION__);
        return;
     }
     sprintf( temp_str,"%s/%s.control",pkg->dest->info_dir,pkg->name);
   
     raw = raw_start = read_raw_pkgs_from_file(temp_str);
     if (raw == NULL ){
        opkg_message(conf, OPKG_ERROR, "Unable to open the control file in  %s\n", __FUNCTION__);
        return;
     }

     while(*raw){
        if (!pkg_valorize_other_field(pkg, &raw ) == 0) {
            opkg_message(conf, OPKG_DEBUG, "unable to read control file for %s. May be empty\n", pkg->name);
        }
     }
     raw = raw_start;
     while (*raw) {
        if (raw!=NULL)
          free(*raw++);
     }

     free(raw_start); 

     return ;

}

void pkg_formatted_field(FILE *fp, pkg_t *pkg, const char *field)
{
     int i;
     int flag_provide_false = 0;

     if (strlen(field) < PKG_MINIMUM_FIELD_NAME_LEN) {
	  goto UNKNOWN_FMT_FIELD;
     }

     switch (field[0])
     {
     case 'a':
     case 'A':
	  if (strcasecmp(field, "Architecture") == 0) {
	       if (pkg->architecture) {
                   fprintf(fp, "Architecture: %s\n", pkg->architecture);
	       }
	  } else if (strcasecmp(field, "Auto-Installed") == 0) {
		if (pkg->auto_installed)
		    fprintf(fp, "Auto-Installed: yes\n");
	  } else {
	       goto UNKNOWN_FMT_FIELD;
	  }
	  break;
     case 'c':
     case 'C':
	  if (strcasecmp(field, "Conffiles") == 0) {
	       conffile_list_elt_t *iter;

	       if (nv_pair_list_empty(&pkg->conffiles))
		    return;

               fprintf(fp, "Conffiles:\n");
	       for (iter = nv_pair_list_first(&pkg->conffiles); iter; iter = nv_pair_list_next(&pkg->conffiles, iter)) {
		    if (((conffile_t *)iter->data)->name && ((conffile_t *)iter->data)->value) {
                         fprintf(fp, "%s %s\n", 
                                 ((conffile_t *)iter->data)->name, 
                                 ((conffile_t *)iter->data)->value);
		    }
	       }
	  } else if (strcasecmp(field, "Conflicts") == 0) {
	       if (pkg->conflicts_count) {
                    fprintf(fp, "Conflicts:");
		    for(i = 0; i < pkg->conflicts_count; i++) {
                        fprintf(fp, "%s %s", i == 0 ? "" : ",", pkg->conflicts_str[i]);
                    }
                    fprintf(fp, "\n");
	       }
	  } else {
	       goto UNKNOWN_FMT_FIELD;
	  }
	  break;
     case 'd':
     case 'D':
	  if (strcasecmp(field, "Depends") == 0) {
	       if (pkg->depends_count) {
                    fprintf(fp, "Depends:");
		    for(i = 0; i < pkg->depends_count; i++) {
                        fprintf(fp, "%s %s", i == 0 ? "" : ",", pkg->depends_str[i]);
                    }
		    fprintf(fp, "\n");
	       }
	  } else if (strcasecmp(field, "Description") == 0) {
	       if (pkg->description) {
                   fprintf(fp, "Description: %s\n", pkg->description);
	       }
	  } else {
	       goto UNKNOWN_FMT_FIELD;
	  }
          break;
     case 'e':
     case 'E':
	  if (pkg->essential) {
              fprintf(fp, "Essential: yes\n");
	  }
	  break;
     case 'f':
     case 'F':
	  if (pkg->filename) {
              fprintf(fp, "Filename: %s\n", pkg->filename);
	  }
	  break;
     case 'i':
     case 'I':
	  if (strcasecmp(field, "Installed-Size") == 0) {
               fprintf(fp, "Installed-Size: %s\n", pkg->installed_size);
	  } else if (strcasecmp(field, "Installed-Time") == 0 && pkg->installed_time) {
               fprintf(fp, "Installed-Time: %lu\n", pkg->installed_time);
	  }
	  break;
     case 'm':
     case 'M':
	  if (strcasecmp(field, "Maintainer") == 0) {
	       if (pkg->maintainer) {
                   fprintf(fp, "maintainer: %s\n", pkg->maintainer);
	       }
	  } else if (strcasecmp(field, "MD5sum") == 0) {
	       if (pkg->md5sum) {
                   fprintf(fp, "MD5Sum: %s\n", pkg->md5sum);
	       }
	  } else {
	       goto UNKNOWN_FMT_FIELD;
	  }
	  break;
     case 'p':
     case 'P':
	  if (strcasecmp(field, "Package") == 0) {
               fprintf(fp, "Package: %s\n", pkg->name);
	  } else if (strcasecmp(field, "Priority") == 0) {
               fprintf(fp, "Priority: %s\n", pkg->priority);
	  } else if (strcasecmp(field, "Provides") == 0) {
	       if (pkg->provides_count) {
               /* Here we check if the opkg_internal_use_only is used, and we discard it.*/
                  for ( i=0; i < pkg->provides_count; i++ ){
	              if (strstr(pkg->provides_str[i],"opkg_internal_use_only")!=NULL) {
                         memset (pkg->provides_str[i],'\x0',strlen(pkg->provides_str[i])); /* Pigi clear my trick flag, just in case */
                         flag_provide_false = 1;
                      }
                  }
                  if ( !flag_provide_false ||                                             /* Pigi there is not my trick flag */
                     ((flag_provide_false) &&  (pkg->provides_count > 1))){             /* Pigi There is, but we also have others Provides */
                     fprintf(fp, "Provides:");
		     for(i = 0; i < pkg->provides_count; i++) {
                         if (strlen(pkg->provides_str[i])>0) {
                            fprintf(fp, "%s %s", i == 1 ? "" : ",", pkg->provides_str[i]);
                         }
                     }
                     fprintf(fp, "\n");
                  }
               }
	  } else {
	       goto UNKNOWN_FMT_FIELD;
	  }
	  break;
     case 'r':
     case 'R':
	  if (strcasecmp (field, "Replaces") == 0) {
	       if (pkg->replaces_count) {
                    fprintf(fp, "Replaces:");
		    for (i = 0; i < pkg->replaces_count; i++) {
                        fprintf(fp, "%s %s", i == 0 ? "" : ",", pkg->replaces_str[i]);
                    }
                    fprintf(fp, "\n");
	       }
	  } else if (strcasecmp (field, "Recommends") == 0) {
	       if (pkg->recommends_count) {
                    fprintf(fp, "Recommends:");
		    for(i = 0; i < pkg->recommends_count; i++) {
                        fprintf(fp, "%s %s", i == 0 ? "" : ",", pkg->recommends_str[i]);
                    }
                    fprintf(fp, "\n");
	       }
	  } else {
	       goto UNKNOWN_FMT_FIELD;
	  }
	  break;
     case 's':
     case 'S':
	  if (strcasecmp(field, "Section") == 0) {
	       if (pkg->section) {
                   fprintf(fp, "Section: %s\n", pkg->section);
	       }
#if defined HAVE_SHA256
	  } else if (strcasecmp(field, "SHA256sum") == 0) {
	       if (pkg->sha256sum) {
                   fprintf(fp, "SHA256sum: %s\n", pkg->sha256sum);
	       }
#endif
	  } else if (strcasecmp(field, "Size") == 0) {
	       if (pkg->size) {
                   fprintf(fp, "Size: %s\n", pkg->size);
	       }
	  } else if (strcasecmp(field, "Source") == 0) {
	       if (pkg->source) {
                   fprintf(fp, "Source: %s\n", pkg->source);
               }
	  } else if (strcasecmp(field, "Status") == 0) {
               char *pflag = pkg_state_flag_to_str(pkg->state_flag);
               char *pstat = pkg_state_status_to_str(pkg->state_status);
               char *pwant = pkg_state_want_to_str(pkg->state_want);

	       if (pflag == NULL || pstat == NULL || pwant == NULL)
		       return;

               fprintf(fp, "Status: %s %s %s\n", pwant, pflag, pstat);

               free(pflag);
               free(pwant);
               free(pstat);
	  } else if (strcasecmp(field, "Suggests") == 0) {
	       if (pkg->suggests_count) {
                    fprintf(fp, "Suggests:");
		    for(i = 0; i < pkg->suggests_count; i++) {
                        fprintf(fp, "%s %s", i == 0 ? "" : ",", pkg->suggests_str[i]);
                    }
                    fprintf(fp, "\n");
	       }
	  } else {
	       goto UNKNOWN_FMT_FIELD;
	  }
	  break;
     case 't':
     case 'T':
	  if (strcasecmp(field, "Tags") == 0) {
	       if (pkg->tags) {
                   fprintf(fp, "Tags: %s\n", pkg->tags);
	       }
	  }
	  break;
     case 'v':
     case 'V':
	  {
	       char *version = pkg_version_str_alloc(pkg);
	       if (version == NULL)
	            return;
               fprintf(fp, "Version: %s\n", version);
	       free(version);
          }
	  break;
     default:
	  goto UNKNOWN_FMT_FIELD;
     }

     return;

UNKNOWN_FMT_FIELD:
     fprintf(stderr, "%s: ERROR: Unknown field name: %s\n", __FUNCTION__, field);
}

void pkg_formatted_info(FILE *fp, pkg_t *pkg)
{
	pkg_formatted_field(fp, pkg, "Package");
	pkg_formatted_field(fp, pkg, "Version");
	pkg_formatted_field(fp, pkg, "Depends");
	pkg_formatted_field(fp, pkg, "Recommends");
	pkg_formatted_field(fp, pkg, "Suggests");
	pkg_formatted_field(fp, pkg, "Provides");
	pkg_formatted_field(fp, pkg, "Replaces");
	pkg_formatted_field(fp, pkg, "Conflicts");
	pkg_formatted_field(fp, pkg, "Status");
	pkg_formatted_field(fp, pkg, "Section");
	pkg_formatted_field(fp, pkg, "Essential");
	pkg_formatted_field(fp, pkg, "Architecture");
	pkg_formatted_field(fp, pkg, "Maintainer");
	pkg_formatted_field(fp, pkg, "MD5sum");
	pkg_formatted_field(fp, pkg, "Size");
	pkg_formatted_field(fp, pkg, "Filename");
	pkg_formatted_field(fp, pkg, "Conffiles");
	pkg_formatted_field(fp, pkg, "Source");
	pkg_formatted_field(fp, pkg, "Description");
	pkg_formatted_field(fp, pkg, "Installed-Time");
	pkg_formatted_field(fp, pkg, "Tags");
	fputs("\n", fp);
}

void pkg_print_status(pkg_t * pkg, FILE * file)
{
     if (pkg == NULL) {
	  return;
     }

     /* XXX: QUESTION: Do we actually want more fields here? The
	original idea was to save space by installing only what was
	needed for actual computation, (package, version, status,
	essential, conffiles). The assumption is that all other fields
	can be found in th available file.

	But, someone proposed the idea to make it possible to
	reconstruct a .opk from an installed package, (ie. for beaming
	from one handheld to another). So, maybe we actually want a few
	more fields here, (depends, suggests, etc.), so that that would
	be guaranteed to work even in the absence of more information
	from the available file.

	28-MAR-03: kergoth and I discussed this yesterday.  We think
	the essential info needs to be here for all installed packages
	because they may not appear in the Packages files on various
	feeds.  Furthermore, one should be able to install from URL or
	local storage without requiring a Packages file from any feed.
	-Jamey
     */
     pkg_formatted_field(file, pkg, "Package");
     pkg_formatted_field(file, pkg, "Version");
     pkg_formatted_field(file, pkg, "Depends");
     pkg_formatted_field(file, pkg, "Recommends");
     pkg_formatted_field(file, pkg, "Suggests");
     pkg_formatted_field(file, pkg, "Provides");
     pkg_formatted_field(file, pkg, "Replaces");
     pkg_formatted_field(file, pkg, "Conflicts");
     pkg_formatted_field(file, pkg, "Status");
     pkg_formatted_field(file, pkg, "Essential");
     pkg_formatted_field(file, pkg, "Architecture");
     pkg_formatted_field(file, pkg, "Conffiles");
     pkg_formatted_field(file, pkg, "Installed-Time");
     pkg_formatted_field(file, pkg, "Auto-Installed");
     fputs("\n", file);
}

/*
 * libdpkg - Debian packaging suite library routines
 * vercmp.c - comparison of version numbers
 *
 * Copyright (C) 1995 Ian Jackson <iwj10@cus.cam.ac.uk>
 */
int pkg_compare_versions(const pkg_t *pkg, const pkg_t *ref_pkg)
{
     int r;

     if (pkg->epoch > ref_pkg->epoch) {
	  return 1;
     }

     if (pkg->epoch < ref_pkg->epoch) {
	  return -1;
     }

     r = verrevcmp(pkg->version, ref_pkg->version);
     if (r) {
	  return r;
     }

     r = verrevcmp(pkg->revision, ref_pkg->revision);
     if (r) {
	  return r;
     }

     return r;
}

/* assume ascii; warning: evaluates x multiple times! */
#define order(x) ((x) == '~' ? -1 \
		: isdigit((x)) ? 0 \
		: !(x) ? 0 \
		: isalpha((x)) ? (x) \
		: (x) + 256)

static int verrevcmp(const char *val, const char *ref) {
  if (!val) val= "";
  if (!ref) ref= "";

  while (*val || *ref) {
    int first_diff= 0;

    while ( (*val && !isdigit(*val)) || (*ref && !isdigit(*ref)) ) {
      int vc= order(*val), rc= order(*ref);
      if (vc != rc) return vc - rc;
      val++; ref++;
    }

    while ( *val == '0' ) val++;
    while ( *ref == '0' ) ref++;
    while (isdigit(*val) && isdigit(*ref)) {
      if (!first_diff) first_diff= *val - *ref;
      val++; ref++;
    }
    if (isdigit(*val)) return 1;
    if (isdigit(*ref)) return -1;
    if (first_diff) return first_diff;
  }
  return 0;
}

int pkg_version_satisfied(pkg_t *it, pkg_t *ref, const char *op)
{
     int r;

     r = pkg_compare_versions(it, ref);

     if (strcmp(op, "<=") == 0 || strcmp(op, "<") == 0) {
	  return r <= 0;
     }

     if (strcmp(op, ">=") == 0 || strcmp(op, ">") == 0) {
	  return r >= 0;
     }

     if (strcmp(op, "<<") == 0) {
	  return r < 0;
     }

     if (strcmp(op, ">>") == 0) {
	  return r > 0;
     }

     if (strcmp(op, "=") == 0) {
	  return r == 0;
     }

     fprintf(stderr, "unknown operator: %s", op);
     return 0;
}

int pkg_name_version_and_architecture_compare(const void *p1, const void *p2)
{
     const pkg_t *a = *(const pkg_t**) p1;
     const pkg_t *b = *(const pkg_t**) p2;
     int namecmp;
     int vercmp;
     if (!a->name || !b->name) {
       fprintf(stderr, "pkg_name_version_and_architecture_compare: a=%p a->name=%p b=%p b->name=%p\n",
	       a, a->name, b, b->name);
       return 0;
     }
       
     namecmp = strcmp(a->name, b->name);
     if (namecmp)
	  return namecmp;
     vercmp = pkg_compare_versions(a, b);
     if (vercmp)
	  return vercmp;
     if (!a->arch_priority || !b->arch_priority) {
       fprintf(stderr, "pkg_name_version_and_architecture_compare: a=%p a->arch_priority=%i b=%p b->arch_priority=%i\n",
	       a, a->arch_priority, b, b->arch_priority);
       return 0;
     }
     if (a->arch_priority > b->arch_priority)
	  return 1;
     if (a->arch_priority < b->arch_priority)
	  return -1;
     return 0;
}

int abstract_pkg_name_compare(const void *p1, const void *p2)
{
     const abstract_pkg_t *a = *(const abstract_pkg_t **)p1;
     const abstract_pkg_t *b = *(const abstract_pkg_t **)p2;
     if (!a->name || !b->name) {
       fprintf(stderr, "abstract_pkg_name_compare: a=%p a->name=%p b=%p b->name=%p\n",
	       a, a->name, b, b->name);
       return 0;
     }
     return strcmp(a->name, b->name);
}


char *pkg_version_str_alloc(pkg_t *pkg)
{
     char *complete_version;
     char *epoch_str;
     char *revision_str;

     if (pkg->epoch) {
	  sprintf_alloc(&epoch_str, "%d:", pkg->epoch);
     } else {
	  epoch_str = xstrdup("");
     }

     if (pkg->revision && strlen(pkg->revision)) {
	  sprintf_alloc(&revision_str, "-%s", pkg->revision);
     } else {
	  revision_str = xstrdup("");
     }


     sprintf_alloc(&complete_version, "%s%s%s",
		   epoch_str, pkg->version, revision_str);

     free(epoch_str);
     free(revision_str);

     return complete_version;
}

str_list_t *pkg_get_installed_files(pkg_t *pkg)
{
     int err;
     char *list_file_name = NULL;
     FILE *list_file = NULL;
     char *line;
     char *installed_file_name;
     int rootdirlen;

     pkg->installed_files_ref_cnt++;

     if (pkg->installed_files) {
	  return pkg->installed_files;
     }

     pkg->installed_files = str_list_alloc();

     /* For uninstalled packages, get the file list directly from the package.
	For installed packages, look at the package.list file in the database.
     */
     if (pkg->state_status == SS_NOT_INSTALLED || pkg->dest == NULL) {
	  if (pkg->local_filename == NULL) {
	       return pkg->installed_files;
	  }
	  /* XXX: CLEANUP: Maybe rewrite this to avoid using a temporary
	     file. In other words, change deb_extract so that it can
	     simply return the file list as a char *[] rather than
	     insisting on writing in to a FILE * as it does now. */
	  list_file = tmpfile();
	  err = pkg_extract_data_file_names_to_stream(pkg, list_file);
	  if (err) {
	       fclose(list_file);
	       fprintf(stderr, "%s: Error extracting file list from %s: %s\n",
		       __FUNCTION__, pkg->local_filename, strerror(err));
	       return pkg->installed_files;
	  }
	  rewind(list_file);
     } else {
	  sprintf_alloc(&list_file_name, "%s/%s.list",
			pkg->dest->info_dir, pkg->name);
	  if (! file_exists(list_file_name)) {
	       free(list_file_name);
	       return pkg->installed_files;
	  }

	  list_file = fopen(list_file_name, "r");
	  if (list_file == NULL) {
	       fprintf(stderr, "WARNING: Cannot open %s: %s\n",
		       list_file_name, strerror(errno));
	       free(list_file_name);
	       return pkg->installed_files;
	  }
	  free(list_file_name);
     }

     rootdirlen = strlen( pkg->dest->root_dir );
     while (1) {
	  char *file_name;
	
	  line = file_read_line_alloc(list_file);
	  if (line == NULL) {
	       break;
	  }
	  str_chomp(line);
	  file_name = line;

	  /* Take pains to avoid uglies like "/./" in the middle of file_name. */
	  if( strncmp( pkg->dest->root_dir, 
		       file_name, 
		       rootdirlen ) ) {
	       if (*file_name == '.') {
		    file_name++;
	       }
	       if (*file_name == '/') {
		    file_name++;
	       }

	       /* Freed in pkg_free_installed_files */
	       sprintf_alloc(&installed_file_name, "%s%s", pkg->dest->root_dir, file_name);
	  } else {
	       // already contains root_dir as header -> ABSOLUTE
	       sprintf_alloc(&installed_file_name, "%s", file_name);
	  }
	  str_list_append(pkg->installed_files, installed_file_name);
          free(installed_file_name);
	  free(line);
     }

     fclose(list_file);

     return pkg->installed_files;
}

/* XXX: CLEANUP: This function and it's counterpart,
   (pkg_get_installed_files), do not match our init/deinit naming
   convention. Nor the alloc/free convention. But, then again, neither
   of these conventions currrently fit the way these two functions
   work. */
int pkg_free_installed_files(pkg_t *pkg)
{
     pkg->installed_files_ref_cnt--;

     if (pkg->installed_files_ref_cnt > 0)
	  return 0;

     if (pkg->installed_files) {
         str_list_purge(pkg->installed_files);
     }

     pkg->installed_files = NULL;

     return 0;
}

int pkg_remove_installed_files_list(opkg_conf_t *conf, pkg_t *pkg)
{
     int err;
     char *list_file_name;

     //I don't think pkg_free_installed_files should be called here. Jamey
     //pkg_free_installed_files(pkg);

     sprintf_alloc(&list_file_name, "%s/%s.list",
		   pkg->dest->info_dir, pkg->name);
     if (!conf->noaction) {
	  err = unlink(list_file_name);
	  free(list_file_name);

	  if (err) {
	       return errno;
	  }
     }
     return 0;
}

conffile_t *pkg_get_conffile(pkg_t *pkg, const char *file_name)
{
     conffile_list_elt_t *iter;
     conffile_t *conffile;

     if (pkg == NULL) {
	  return NULL;
     }

     for (iter = nv_pair_list_first(&pkg->conffiles); iter; iter = nv_pair_list_next(&pkg->conffiles, iter)) {
	  conffile = (conffile_t *)iter->data;

	  if (strcmp(conffile->name, file_name) == 0) {
	       return conffile;
	  }
     }

     return NULL;
}

int pkg_run_script(opkg_conf_t *conf, pkg_t *pkg,
		   const char *script, const char *args)
{
     int err;
     char *path;
     char *cmd;

     if (conf->noaction)
	     return 0;

     /* XXX: CLEANUP: There must be a better way to handle maintainer
	scripts when running with offline_root mode and/or a dest other
	than '/'. I've been playing around with some clever chroot
	tricks and I might come up with something workable. */
     /*
      * Attempt to provide a restricted environment for offline operation
      * Need the following set as a minimum:
      * OPKG_OFFLINE_ROOT = absolute path to root dir
      * D                 = absolute path to root dir (for OE generated postinst)
      * PATH              = something safe (a restricted set of utilities)
      */

     if (conf->offline_root) {
          if (conf->offline_root_path) {
            setenv("PATH", conf->offline_root_path, 1);
          } else {
            opkg_message(conf, OPKG_NOTICE, 
	    	"(offline root mode: not running %s.%s)\n", pkg->name, script);
	    return 0;
          }
	  setenv("OPKG_OFFLINE_ROOT", conf->offline_root, 1);
	  setenv("D", conf->offline_root, 1);
     }

     /* XXX: FEATURE: When conf->offline_root is set, we should run the
	maintainer script within a chroot environment. */

     /* Installed packages have scripts in pkg->dest->info_dir, uninstalled packages
	have scripts in pkg->tmp_unpack_dir. */
     if (pkg->state_status == SS_INSTALLED || pkg->state_status == SS_UNPACKED) {
	  if (pkg->dest == NULL) {
	       fprintf(stderr, "%s: ERROR: installed package %s has a NULL dest\n",
		       __FUNCTION__, pkg->name);
	       return EINVAL;
	  }
	  sprintf_alloc(&path, "%s/%s.%s", pkg->dest->info_dir, pkg->name, script);
     } else {
	  if (pkg->tmp_unpack_dir == NULL) {
	       fprintf(stderr, "%s: ERROR: uninstalled package %s has a NULL tmp_unpack_dir\n",
		       __FUNCTION__, pkg->name);
	       return EINVAL;
	  }
	  sprintf_alloc(&path, "%s/%s", pkg->tmp_unpack_dir, script);
     }

     opkg_message(conf, OPKG_INFO, "Running script %s\n", path);

     setenv("PKG_ROOT",
	    pkg->dest ? pkg->dest->root_dir : conf->default_dest->root_dir, 1);

     if (! file_exists(path)) {
	  free(path);
	  return 0;
     }

     sprintf_alloc(&cmd, "%s %s", path, args);
     free(path);

     err = xsystem(cmd);
     free(cmd);

     if (err) {
	  fprintf(stderr, "%s script returned status %d\n", script, err);
	  return err;
     }

     return 0;
}

char *pkg_state_want_to_str(pkg_state_want_t sw)
{
     int i;

     for (i=0; i < ARRAY_SIZE(pkg_state_want_map); i++) {
	  if (pkg_state_want_map[i].value == sw) {
	       return xstrdup(pkg_state_want_map[i].str);
	  }
     }

     fprintf(stderr, "%s: ERROR: Illegal value for state_want: %d\n",
	     __FUNCTION__, sw);
     return xstrdup("<STATE_WANT_UNKNOWN>");
}

pkg_state_want_t pkg_state_want_from_str(char *str)
{
     int i;

     for (i=0; i < ARRAY_SIZE(pkg_state_want_map); i++) {
	  if (strcmp(str, pkg_state_want_map[i].str) == 0) {
	       return pkg_state_want_map[i].value;
	  }
     }

     fprintf(stderr, "%s: ERROR: Illegal value for state_want string: %s\n",
	     __FUNCTION__, str);
     return SW_UNKNOWN;
}

char *pkg_state_flag_to_str(pkg_state_flag_t sf)
{
     int i;
     int len = 3; /* ok\000 is minimum */
     char *str = NULL;

     /* clear the temporary flags before converting to string */
     sf &= SF_NONVOLATILE_FLAGS;

     if (sf == 0) {
	  return xstrdup("ok");
     } else {

	  for (i=0; i < ARRAY_SIZE(pkg_state_flag_map); i++) {
	       if (sf & pkg_state_flag_map[i].value) {
		    len += strlen(pkg_state_flag_map[i].str) + 1;
	       }
	  }
	  str = xmalloc(len);
	  str[0] = 0;
	  for (i=0; i < ARRAY_SIZE(pkg_state_flag_map); i++) {
	       if (sf & pkg_state_flag_map[i].value) {
		    strcat(str, pkg_state_flag_map[i].str);
		    strcat(str, ",");
	       }
	  }
	  len = strlen(str);
	  str[len-1] = 0; /* squash last comma */
	  return str;
     }
}

pkg_state_flag_t pkg_state_flag_from_str(const char *str)
{
     int i;
     int sf = SF_OK;

     if (strcmp(str, "ok") == 0) {
	  return SF_OK;
     }
     for (i=0; i < ARRAY_SIZE(pkg_state_flag_map); i++) {
	  const char *sfname = pkg_state_flag_map[i].str;
	  int sfname_len = strlen(sfname);
	  if (strncmp(str, sfname, sfname_len) == 0) {
	       sf |= pkg_state_flag_map[i].value;
	       str += sfname_len;
	       if (str[0] == ',') {
		    str++;
	       } else {
		    break;
	       }
	  }
     }

     return sf;
}

char *pkg_state_status_to_str(pkg_state_status_t ss)
{
     int i;

     for (i=0; i < ARRAY_SIZE(pkg_state_status_map); i++) {
	  if (pkg_state_status_map[i].value == ss) {
	       return xstrdup(pkg_state_status_map[i].str);
	  }
     }

     fprintf(stderr, "%s: ERROR: Illegal value for state_status: %d\n",
	     __FUNCTION__, ss);
     return xstrdup("<STATE_STATUS_UNKNOWN>");
}

pkg_state_status_t pkg_state_status_from_str(const char *str)
{
     int i;

     for (i=0; i < ARRAY_SIZE(pkg_state_status_map); i++) {
	  if (strcmp(str, pkg_state_status_map[i].str) == 0) {
	       return pkg_state_status_map[i].value;
	  }
     }

     fprintf(stderr, "%s: ERROR: Illegal value for state_status string: %s\n",
	     __FUNCTION__, str);
     return SS_NOT_INSTALLED;
}

int pkg_arch_supported(opkg_conf_t *conf, pkg_t *pkg)
{
     nv_pair_list_elt_t *l;

     if (!pkg->architecture)
	  return 1;

     list_for_each_entry(l , &conf->arch_list.head, node) {
	  nv_pair_t *nv = (nv_pair_t *)l->data;
	  if (strcmp(nv->name, pkg->architecture) == 0) {
	       opkg_message(conf, OPKG_DEBUG, "arch %s (priority %s) supported for pkg %s\n", nv->name, nv->value, pkg->name);
	       return 1;
	  }
     }

     opkg_message(conf, OPKG_DEBUG, "arch %s unsupported for pkg %s\n", pkg->architecture, pkg->name);
     return 0;
}

int pkg_get_arch_priority(opkg_conf_t *conf, const char *archname)
{
     nv_pair_list_elt_t *l;

     list_for_each_entry(l , &conf->arch_list.head, node) {
	  nv_pair_t *nv = (nv_pair_t *)l->data;
	  if (strcmp(nv->name, archname) == 0) {
	       int priority = strtol(nv->value, NULL, 0);
	       return priority;
	  }
     }
     return 0;
}

int pkg_info_preinstall_check(opkg_conf_t *conf)
{
     int i;
     hash_table_t *pkg_hash = &conf->pkg_hash;
     pkg_vec_t *available_pkgs = pkg_vec_alloc();
     pkg_vec_t *installed_pkgs = pkg_vec_alloc();

     opkg_message(conf, OPKG_INFO, "pkg_info_preinstall_check: updating arch priority for each package\n");
     pkg_hash_fetch_available(pkg_hash, available_pkgs);
     /* update arch_priority for each package */
     for (i = 0; i < available_pkgs->len; i++) {
	  pkg_t *pkg = available_pkgs->pkgs[i];
	  int arch_priority = 1;
	  if (!pkg)
	       continue;
	  // opkg_message(conf, OPKG_DEBUG2, " package %s version=%s arch=%p:", pkg->name, pkg->version, pkg->architecture);
	  if (pkg->architecture) 
	       arch_priority = pkg_get_arch_priority(conf, pkg->architecture);
	  else 
	       opkg_message(conf, OPKG_ERROR, "pkg_info_preinstall_check: no architecture for package %s\n", pkg->name);
	  // opkg_message(conf, OPKG_DEBUG2, "%s arch_priority=%d\n", pkg->architecture, arch_priority);
	  pkg->arch_priority = arch_priority;
     }

     for (i = 0; i < available_pkgs->len; i++) {
	  pkg_t *pkg = available_pkgs->pkgs[i];
	  if (!pkg->arch_priority && (pkg->state_flag || (pkg->state_want != SW_UNKNOWN))) {
	       /* clear flags and want for any uninstallable package */
	       opkg_message(conf, OPKG_DEBUG, "Clearing state_want and state_flag for pkg=%s (arch_priority=%d flag=%d want=%d)\n", 
			    pkg->name, pkg->arch_priority, pkg->state_flag, pkg->state_want);
	       pkg->state_want = SW_UNKNOWN;
	       pkg->state_flag = 0;
	  }
     }
     pkg_vec_free(available_pkgs);

     /* update the file owner data structure */
     opkg_message(conf, OPKG_INFO, "pkg_info_preinstall_check: update file owner list\n");
     pkg_hash_fetch_all_installed(pkg_hash, installed_pkgs);
     for (i = 0; i < installed_pkgs->len; i++) {
	  pkg_t *pkg = installed_pkgs->pkgs[i];
	  str_list_t *installed_files = pkg_get_installed_files(pkg); /* this causes installed_files to be cached */
	  str_list_elt_t *iter, *niter;
	  if (installed_files == NULL) {
	       opkg_message(conf, OPKG_ERROR, "No installed files for pkg %s\n", pkg->name);
	       break;
	  }
	  for (iter = str_list_first(installed_files), niter = str_list_next(installed_files, iter); 
                  iter; 
                  iter = niter, niter = str_list_next(installed_files, iter)) {
	       char *installed_file = (char *) iter->data;
	       // opkg_message(conf, OPKG_DEBUG2, "pkg %s: file=%s\n", pkg->name, installed_file);
	       file_hash_set_file_owner(conf, installed_file, pkg);
	  }
	  pkg_free_installed_files(pkg);
     }
     pkg_vec_free(installed_pkgs);

     return 0;
}

struct pkg_write_filelist_data {
     opkg_conf_t *conf;
     pkg_t *pkg;
     FILE *stream;
};

void pkg_write_filelist_helper(const char *key, void *entry_, void *data_)
{
     struct pkg_write_filelist_data *data = data_;
     pkg_t *entry = entry_;
     if (entry == data->pkg) {
	  fprintf(data->stream, "%s\n", key);
     }
}

int pkg_write_filelist(opkg_conf_t *conf, pkg_t *pkg)
{
     struct pkg_write_filelist_data data;
     char *list_file_name = NULL;
     int err = 0;

     if (!pkg) {
	  opkg_message(conf, OPKG_ERROR, "Null pkg\n");
	  return -EINVAL;
     }
     opkg_message(conf, OPKG_INFO,
		  "    creating %s.list file\n", pkg->name);
     sprintf_alloc(&list_file_name, "%s/%s.list", pkg->dest->info_dir, pkg->name);
     if (!list_file_name) {
	  opkg_message(conf, OPKG_ERROR, "Failed to alloc list_file_name\n");
	  return -ENOMEM;
     }
     opkg_message(conf, OPKG_INFO,
		  "    creating %s file for pkg %s\n", list_file_name, pkg->name);
     data.stream = fopen(list_file_name, "w");
     if (!data.stream) {
	  opkg_message(conf, OPKG_ERROR, "Could not open %s for writing: %s\n",
		       list_file_name, strerror(errno));
		       return errno;
     }
     data.pkg = pkg;
     data.conf = conf;
     hash_table_foreach(&conf->file_hash, pkg_write_filelist_helper, &data);
     fclose(data.stream);
     free(list_file_name);

     pkg->state_flag &= ~SF_FILELIST_CHANGED;

     return err;
}

int pkg_write_changed_filelists(opkg_conf_t *conf)
{
     pkg_vec_t *installed_pkgs = pkg_vec_alloc();
     hash_table_t *pkg_hash = &conf->pkg_hash;
     int i;
     int err;
     if (conf->noaction)
	  return 0;

     opkg_message(conf, OPKG_INFO, "%s: saving changed filelists\n", __FUNCTION__);
     pkg_hash_fetch_all_installed(pkg_hash, installed_pkgs);
     for (i = 0; i < installed_pkgs->len; i++) {
	  pkg_t *pkg = installed_pkgs->pkgs[i];
	  if (pkg->state_flag & SF_FILELIST_CHANGED) {
               opkg_message(conf, OPKG_DEBUG, "Calling pkg_write_filelist for pkg=%s from %s\n", pkg->name, __FUNCTION__);
	       err = pkg_write_filelist(conf, pkg);
	       if (err)
		    opkg_message(conf, OPKG_NOTICE, "pkg_write_filelist pkg=%s returned %d\n", pkg->name, err);
	  }
     }
     pkg_vec_free (installed_pkgs);
     return 0;
}
