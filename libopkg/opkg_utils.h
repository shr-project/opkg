/* opkg_utils.h - the opkg package management system

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

#ifndef OPKG_UTILS_H
#define OPKG_UTILS_H

#include "pkg.h"
#include "opkg_error.h"

void push_error_list(char * msg);
void free_error_list(void);
void print_error_list(void);

long unsigned int get_available_blocks(char * filesystem);
char *trim_alloc(const char *line);
int line_is_blank(const char *line);

#endif
