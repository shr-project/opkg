/* user.c - the opkg package management system

   Jamey Hicks

   Copyright (C) 2002 Hewlett Packard Company
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
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include "file_util.h"
#include "str_util.h"

char *get_user_response(const char *format, ...)
{
	va_list ap;
	char *response;

	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);

	if (!isatty(fileno(stdin)))
		return NULL;

	response = (char *)file_read_line_alloc(stdin);
	if (response == NULL)
		return NULL;

	str_tolower(response);

	return response;
}
