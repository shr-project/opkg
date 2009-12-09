/* opkglib.h - the opkg package management system

   Florian Boor <florian.boor@kernelconcepts.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/

#ifndef OPKGLIB_H
#define OPKGLIB_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include "opkg_conf.h"
#include "opkg_message.h"
#include "opkg_download.h"
#include "opkg_utils.h"

#include "args.h"
#include "pkg.h"

extern int opkg_op(int argc, char *argv[]); /* opkglib.c */

#endif
