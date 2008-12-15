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
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "file_util.h"
#include "str_util.h"
#include "user.h"

static char *question = NULL;
static int question_len = 255;

opkg_response_callback opkg_cb_response = NULL;

char *get_user_response(const char *format, ...)
{
     int len = question_len;
     va_list ap;
     char *response;
     va_start(ap, format);

     do {
	  if (question == NULL || len > question_len) {
	       question = realloc(question, len + 1);
	       question_len = len;
	  }
	  len = vsnprintf(question,question_len,format,ap);
     } while (len > question_len);
     response = strdup(opkg_cb_response(question));
     str_chomp(response);
     str_tolower(response);

     return response;
}
