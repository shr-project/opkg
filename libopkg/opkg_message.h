/* opkg_message.h - the opkg package management system

   Copyright (C) 2003 Daniele Nicolodi <daniele@grinta.net>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/

#ifndef _OPKG_MESSAGE_H_
#define _OPKG_MESSAGE_H_

#include "opkg_conf.h"

typedef enum {
     OPKG_ERROR,	/* error conditions */
     OPKG_NOTICE,	/* normal but significant condition */
     OPKG_INFO,		/* informational message */
     OPKG_DEBUG,	/* debug level message */
     OPKG_DEBUG2,	/* more debug level message */
} message_level_t;

extern void opkg_message(opkg_conf_t *conf, message_level_t level, char *fmt, ...);

#endif /* _OPKG_MESSAGE_H_ */
