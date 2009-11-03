/* opkg_message.c - the opkg package management system

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


#include "includes.h"
#include "opkg_conf.h"
#include "opkg_message.h"
#include "opkg_error.h"
#include "opkg_utils.h"

void
opkg_message (opkg_conf_t * conf, message_level_t level, char *fmt, ...)
{
	va_list ap;

	if (conf && (conf->verbosity < level))
		return;

	va_start (ap, fmt);

	if (level == OPKG_ERROR) {
		char msg[256];
		vsnprintf(msg, 256, fmt, ap);
		push_error_list(msg);
	} else
		vprintf(fmt, ap);

	va_end (ap);
}
