/* args.h - parse command-line args

  Carl D. Worth

  Copyright 2001 University of Southern California
 
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

#ifndef ARGS_H
#define ARGS_H

struct args
{
    char *conf_file;
    char *dest;

    int nocheckfordirorfile;
    int noreadfeedsfile;
};
typedef struct args args_t;

#define ARGS_DEFAULT_CONF_FILE_DIR OPKGETCDIR"/opkg"
#define ARGS_DEFAULT_CONF_FILE_NAME "opkg.conf"
#define ARGS_DEFAULT_DEST NULL
#define ARGS_DEFAULT_VERBOSITY 1

void args_init(args_t *args);
void args_deinit(args_t *args);
int args_parse(args_t *args, int argc, char *argv[]);
void args_usage(const char *complaint);

#endif
