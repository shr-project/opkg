/* opkg_state.c - the opkg package management system

   Thomas Wood <thomas@openedhand.com>

   Copyright (C) 2008 by OpenMoko Inc

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/

#include "opkg_state.h"


static char *state_strings[] =
{
  "None",
  "Downloading Package",
  "Installing Package",
  "Configuring Package",
  "Upgrading Package",
  "Removing Package",
  "Downloading Repository",
  "Verifying Repository Signature"
};



opkg_state_changed_callback opkg_cb_state_changed = NULL;

static opkg_state_t opkg_state = 0;
static char *opkg_state_data = NULL;

void
opkg_set_current_state (opkg_conf_t *conf, opkg_state_t state, const char *data)
{
  if (opkg_state_data)
    free (opkg_state_data);
  if (data)
  {
    opkg_state_data = strdup (data);
  }
  else
  {
    opkg_state_data = NULL;
  }

  opkg_state = state;

  if (opkg_cb_state_changed)
  {
    opkg_cb_state_changed (opkg_state, opkg_state_data);
  }


  if (data == NULL)
    opkg_message (conf, OPKG_INFO, "opkg state set to %s\n", state_strings[state]);
  else
    opkg_message (conf, OPKG_INFO, "opkg state set to %s: %s\n", state_strings[state], data);
}

void
opkg_get_current_state (opkg_state_t *state, const char **data)
{
  *state = opkg_state;
  *data = opkg_state_data;
}
