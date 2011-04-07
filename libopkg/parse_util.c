/* parse_util.c - the opkg package management system

   Copyright (C) 2009 Ubiq Technologies <graham.gower@gmail.com>

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

#include <ctype.h>

#include "opkg_utils.h"
#include "libbb/libbb.h"

#include "parse_util.h"

int
is_field(const char *type, const char *line)
{
	if (!strncmp(line, type, strlen(type)))
		return 1;
	return 0;
}

char *
parse_simple(const char *type, const char *line)
{
	return trim_xstrdup(line + strlen(type) + 1);
}

/*
 * Parse a comma separated string into an array.
 */
char **
parse_list(const char *raw, unsigned int *count, const char sep, int skip_field)
{
	char **depends = NULL;
	const char *start, *end;
	int line_count = 0;

	/* skip past the "Field:" marker */
	if (!skip_field) {
	while (*raw && *raw != ':')
		raw++;
	raw++;
	}

	if (line_is_blank(raw)) {
		*count = line_count;
		return NULL;
	}

	while (*raw) {
		depends = xrealloc(depends, sizeof(char *) * (line_count + 1));
	
		while (isspace(*raw))
			raw++;

		start = raw;
		while (*raw != sep && *raw)
			raw++;
		end = raw;

		while (end > start && isspace(*end))
			end--;

		if (sep == ' ')
			end++;

		depends[line_count] = xstrndup(start, end-start);

        	line_count++;
		if (*raw == sep)
		    raw++;
	}

	*count = line_count;
	return depends;
}
