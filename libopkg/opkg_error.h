/* opkg_error.h - the opkg package management system

   Copyright (C) 2008 OpenMoko Inc

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/

#ifndef OPKG_ERROR_H
#define OPKG_ERROR_H

enum opkg_error {
    OPKG_SUCCESS = 0,
    OPKG_PKG_DEPS_UNSATISFIED,
    OPKG_PKG_IS_ESSENTIAL,
    OPKG_PKG_HAS_DEPENDENTS,
    OPKG_PKG_HAS_NO_CANDIDATE
};
typedef enum opkg_error opkg_error_t;


struct errlist {
    char * errmsg;
    struct errlist * next;
} ;

struct errlist* error_list;

#endif /* OPKG_ERROR_H */
