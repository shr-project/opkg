/* opkg_utils.c - the opkg package management system

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
#include <errno.h>
#include <ctype.h>
#include <sys/vfs.h>

#include "opkg_utils.h"
#include "pkg.h"
#include "pkg_hash.h"
#include "libbb/libbb.h"

void print_pkg_status(pkg_t * pkg, FILE * file);

long unsigned int get_available_blocks(char * filesystem)
{
    struct statfs sfs;

    if(statfs(filesystem, &sfs)){
        fprintf(stderr, "bad statfs\n");
        return 0;
    }
    /*    fprintf(stderr, "reported fs type %x\n", sfs.f_type); */

    // Actually ((sfs.f_bavail * sfs.f_bsize) / 1024) 
    // and here we try to avoid overflow. 
    if (sfs.f_bsize >= 1024) 
        return (sfs.f_bavail * (sfs.f_bsize / 1024));
    else if (sfs.f_bsize > 0)
        return sfs.f_bavail / (1024 / sfs.f_bsize);
    fprintf(stderr, "bad statfs f_bsize == 0\n");
    return 0;
}

char **read_raw_pkgs_from_file(const char *file_name)
{
     FILE *fp; 
     char **ret;
    
     if(!(fp = fopen(file_name, "r"))){
	  fprintf(stderr, "can't get %s open for read\n", file_name);
	  return NULL;
     }

     ret = read_raw_pkgs_from_stream(fp);

     fclose(fp);

     return ret;
}

char **read_raw_pkgs_from_stream(FILE *fp)
{    
     char **raw = NULL, *buf, *scout;
     int count = 0;
     size_t size = 512;
     
     buf = xcalloc(1, size);

     while (fgets(buf, size, fp)) {
	  while (strlen (buf) == (size - 1)
		 && buf[size-2] != '\n') {
	       size_t o = size - 1;
	       size *= 2;
	       buf = xrealloc (buf, size);
	       if (fgets (buf + o, size - o, fp) == NULL)
		    break;
	  }
	  
	  if(!(count % 50))
	       raw = xrealloc(raw, (count + 50) * sizeof(char *));
	
	  if((scout = strchr(buf, '\n')))
	       *scout = '\0';

	  raw[count++] = xstrdup(buf);
     }
    
     raw = xrealloc(raw, (count + 1) * sizeof(char *));
     raw[count] = NULL;

     free (buf);
    
     return raw;
}

/* something to remove whitespace, a hash pooper */
char *trim_alloc(const char *src)
{
     const char *end;

     /* remove it from the front */    
     while(src && 
	   isspace(*src) &&
	   *src)
	  src++;

     end = src + (strlen(src) - 1);

     /* and now from the back */
     while((end > src) &&
	   isspace(*end))
	  end--;

     end++;

     /* xstrndup will NULL terminate for us */
     return xstrndup(src, end-src);
}

int line_is_blank(const char *line)
{
     const char *s;

     for (s = line; *s; s++) {
	  if (!isspace(*s))
	       return 0;
     }
     return 1;
}

static struct errlist *error_list_head, *error_list_tail;

/*
 * XXX: this function should not allocate memory as it may be called to
 *      print an error because we are out of memory.
 */
void push_error_list(char * msg)
{
	struct errlist *e;

	e = xcalloc(1,  sizeof(struct errlist));
	e->errmsg = xstrdup(msg);
	e->next = NULL;

	if (error_list_head) {
		error_list_tail->next = e;
		error_list_tail = e;
	} else {
		error_list_head = error_list_tail = e;
	}
}

void free_error_list(void)
{
	struct errlist *err, *err_tmp;

	err = error_list_head;
	while (err != NULL) {
		free(err->errmsg);
		err_tmp = err;
		err = err->next;
		free(err_tmp);
	}
}

void print_error_list (void)
{
	struct errlist *err = error_list_head;

	if (err) {
		printf ("Collected errors:\n");
		/* Here we print the errors collected and free the list */
		while (err != NULL) {
			printf (" * %s", err->errmsg);
			err = err->next;
		}
	}
}
