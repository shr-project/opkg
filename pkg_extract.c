/* pkg_extract.c - the itsy package management system

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

#include "ipkg.h"
#include <errno.h>

#include "pkg_extract.h"

#include "libbb/libbb.h"
#include "file_util.h"
#include "sprintf_alloc.h"

/* assuage libb functions */
const char *applet_name = "ipkg";

int pkg_extract_control_file_to_stream(pkg_t *pkg, FILE *stream)
{
    char *buffer = deb_extract(pkg->local_filename, stderr,
			       extract_control_tar_gz
			       | extract_one_to_buffer,
			       NULL, "./control");
    if (buffer == NULL) {
	return EINVAL;
    }

    /* XXX: QUESTION: Is there a way to do this directly with deb_extract now? */
    fputs(buffer, stream);
    free(buffer);

    return 0;
}

int pkg_extract_control_files_to_dir(pkg_t *pkg, const char *dir)
{
    return pkg_extract_control_files_to_dir_with_prefix(pkg, dir, "");
}

int pkg_extract_control_files_to_dir_with_prefix(pkg_t *pkg,
						 const char *dir,
						 const char *prefix)
{
    char *dir_with_prefix;

    sprintf_alloc(&dir_with_prefix, "%s/%s", dir, prefix);

    deb_extract(pkg->local_filename, stderr,
		extract_control_tar_gz
                | extract_all_to_fs| extract_preserve_date
		| extract_unconditional,
		dir_with_prefix, NULL);

    free(dir_with_prefix);

    /* XXX: BUG: how do we know if deb_extract worked or not? This is
       a defect in the current deb_extract from what I can tell.

       Once this is fixed, audit all calls to deb_extract. */
    return 0;
}

int pkg_extract_data_files_to_dir(pkg_t *pkg, const char *dir)
{
    deb_extract(pkg->local_filename, stderr,
		extract_data_tar_gz
                | extract_all_to_fs| extract_preserve_date
		| extract_unconditional,
		dir, NULL);

    /* BUG: How do we know if deb_extract worked or not? This is a
       defect in the current deb_extract from what I can tell. */
    return 0;
}

int pkg_extract_data_file_names_to_file(pkg_t *pkg, const char *file_name)
{
     int err=0;
     char *line, *data_file;
     FILE *file;
     FILE *tmp;

     file = fopen(file_name, "w");
     if (file == NULL) {
	  fprintf(stderr, "%s: ERROR: Failed to open %s for writing.\n",
		  __FUNCTION__, file_name);
	  return EINVAL;
     }

     tmp = tmpfile();
     if (pkg->installed_files) {
	  str_list_elt_t *elt;
	  for (elt = pkg->installed_files->head; elt; elt = elt->next) {
	       fprintf(file, "%s\n", elt->data);
	  }
     } else {
	  err = pkg_extract_data_file_names_to_stream(pkg, tmp);
	  if (err) {
	       fclose(file);
	       fclose(tmp);
	       return err;
	  }

	  /* Fixup data file names by removing the initial '.' */
	  rewind(tmp);
	  while (1) {
	       line = file_read_line_alloc(tmp);
	       if (line == NULL) {
		    break;
	       }

	       data_file = line;
	       if (*data_file == '.') {
		    data_file++;
	       }

	       if (*data_file != '/') {
		    fputs("/", file);
	       }

	       /* I have no idea why, but this is what dpkg does */
	       if (strcmp(data_file, "/\n") == 0) {
		    fputs("/.\n", file);
	       } else {
		    fputs(data_file, file);
	       }
	  }
     }
     fclose(tmp);
     fclose(file);

     return err;
}

int pkg_extract_data_file_names_to_stream(pkg_t *pkg, FILE *file)
{
    /* XXX: DPKG_INCOMPATIBILITY: deb_extract will extract all of the
       data file names with a '.' as the first character. I've taught
       ipkg how to cope with the presence or absence of the '.', but
       this may trip up dpkg.

       For all I know, this could actually be a bug in ipkg-build. So,
       I'll have to try installing some .debs and comparing the *.list
       files.

       If we wanted to, we could workaround the deb_extract behavior
       right here, by writing to a tmpfile, then munging things as we
       wrote to the actual stream. */
     deb_extract(pkg->local_filename, file,
		 extract_quiet | extract_data_tar_gz | extract_list,
		 NULL, NULL);

    /* BUG: How do we know if deb_extract worked or not? This is a
       defect in the current deb_extract from what I can tell. */
    return 0;
}
