/* opkg.h - the itsy package management system

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

#ifndef OPKG_H
#define OPKG_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if 0
#define OPKG_DEBUG_NO_TMP_CLEANUP
#endif

#include "includes.h"
#include "opkg_conf.h"
#include "opkg_message.h"

#define OPKG_PKG_EXTENSION ".ipk"
#define DPKG_PKG_EXTENSION ".deb"

#define OPKG_LEGAL_PKG_NAME_CHARS "abcdefghijklmnopqrstuvwxyz0123456789.+-"
#define OPKG_PKG_VERSION_SEP_CHAR '_'

#define OPKG_STATE_DIR_PREFIX OPKGLIBDIR"/opkg"
#define OPKG_LISTS_DIR_SUFFIX "lists"
#define OPKG_INFO_DIR_SUFFIX "info"
#define OPKG_STATUS_FILE_SUFFIX "status"

#define OPKG_BACKUP_SUFFIX "-opkg.backup"

#define OPKG_LIST_DESCRIPTION_LENGTH 128

enum opkg_error {
    OPKG_SUCCESS = 0,
    OPKG_PKG_DEPS_UNSATISFIED,
    OPKG_PKG_IS_ESSENTIAL,
    OPKG_PKG_HAS_DEPENDENTS,
    OPKG_PKG_HAS_NO_CANDIDATE
};
typedef enum opkg_error opkg_error_t;

extern int opkg_state_changed;


struct errlist {
    char * errmsg;
    struct errlist * next;
} ;

struct errlist* error_list;


#endif
