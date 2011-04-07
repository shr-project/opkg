/* release_parse.c - the opkg package management system

   Copyright (C) 2010,2011 Javier Palacios

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

#include <stdio.h>

#include "release.h"
#include "release_parse.h"
#include "libbb/libbb.h"
#include "parse_util.h"

static int
release_parse_line(release_t *release, const char *line)
{
	int ret = 0;
	unsigned int count = 0;
	char **list = 0;
	static int reading_md5sums = 0;
#ifdef HAVE_SHA256
	static int reading_sha256sums = 0;
#endif

	switch (*line) {
	case 'A':
		if (is_field("Architectures", line)) {
			release->architectures = parse_list(line, &release->architectures_count, ' ', 0);
		}
		break;

	case 'C':
		if (is_field("Codename", line)) {
			release->name = parse_simple("Codename", line);
	    	}
		else if (is_field("Components", line)) {
			release->components = parse_list(line, &release->components_count, ' ', 0);
	    	}
		break;

	case 'D':
		if (is_field("Date", line)) {
			release->datestring = parse_simple("Date", line);
		}
		break;

	case 'M':
		if (is_field("MD5sum", line)) {
			reading_md5sums = 1;
			if (release->md5sums == NULL) {
			     release->md5sums = xcalloc(1, sizeof(cksum_list_t));
			     cksum_list_init(release->md5sums);
			}
			goto dont_reset_flags;
	    	}
		break;

#ifdef HAVE_SHA256
	case 'S':
		if (is_field("SHA256", line)) {
			reading_sha256sums = 1;
			if (release->sha256sums == NULL) {
			     release->sha256sums = xcalloc(1, sizeof(cksum_list_t));
			     cksum_list_init(release->sha256sums);
			}
			goto dont_reset_flags;
	    	}
		break;
#endif

	case ' ':
		if (reading_md5sums) {
			list = parse_list(line, &count, ' ', 1);
			cksum_list_append(release->md5sums, list);
			goto dont_reset_flags;
		}
#ifdef HAVE_SHA256
		else if (reading_sha256sums) {
			list = parse_list(line, &count, ' ', 1);
			cksum_list_append(release->sha256sums, list);
			goto dont_reset_flags;
		}
#endif
		break;

	default:
		ret = 1;
	}

	reading_md5sums = 0;
#ifdef HAVE_SHA256
	reading_sha256sums = 0;
#endif

dont_reset_flags:

	return ret;
}

int
release_parse_from_stream(release_t *release, FILE *fp)
{
	int ret = 0;
	char *buf = NULL;
	size_t buflen, nread;

	nread = getline(&buf, &buflen, fp);
	while ( nread != -1 ) {
		if (buf[nread-1] == '\n') buf[nread-1] = '\0';
		if (release_parse_line(release, buf))
                        opkg_msg(DEBUG, "Failed to parse release line for %s:\n\t%s\n",
					release->name, buf);
		nread = getline(&buf, &buflen, fp);
	}

	if (!feof(fp)) {
		opkg_perror(ERROR, "Problems reading Release file for %sd\n", release->name);
		ret = -1;
	}

	free(buf);
	return ret;
}

