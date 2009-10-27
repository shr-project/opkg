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
  OPKG_ERR_UNKNOWN = -1,
  OPKG_ERR_NONE = 0,

  OPKG_CONF_ERR_DEFAULT_DEST,  /* could not set default dest */
  OPKG_CONF_ERR_PARSE,         /* error parsing config file */
  OPKG_CONF_ERR_TMP_DIR,       /* could not create temporary directory */
  OPKG_CONF_ERR_LOCK,          /* could not get opkg lock */

  OPKG_PKG_DEPS_UNSATISFIED,
  OPKG_PKG_IS_ESSENTIAL,
  OPKG_PKG_HAS_DEPENDENTS,
  OPKG_PKG_HAS_NO_CANDIDATE,
  OPKG_PKG_HAS_NO_AVAILABLE_ARCH,

  OPKG_INSTALL_ERR_NOT_TRUSTED,
  OPKG_INSTALL_ERR_DOWNLOAD,
  OPKG_INSTALL_ERR_CONFLICTS,
  OPKG_INSTALL_ERR_ALREADY_INSTALLED,
  OPKG_INSTALL_ERR_DEPENDENCIES,
  OPKG_INSTALL_ERR_NO_DOWNGRADE,
  OPKG_INSTALL_ERR_NO_SPACE,
  OPKG_INSTALL_ERR_SIGNATURE,
  OPKG_INSTALL_ERR_MD5,
  OPKG_INSTALL_ERR_INTERNAL,
  OPKG_INSTALL_ERR_SHA256,

};
typedef enum opkg_error opkg_error_t;


struct errlist {
    char * errmsg;
    struct errlist * next;
} ;

extern struct errlist* error_list;

#endif /* OPKG_ERROR_H */
