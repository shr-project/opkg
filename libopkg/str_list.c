/* str_list.c - the opkg package management system

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

#include "includes.h"

#include "str_list.h"

int str_list_elt_init(str_list_elt_t *elt, char *data)
{
    return void_list_elt_init((void_list_elt_t *) elt, data);
}

void str_list_elt_deinit(str_list_elt_t *elt)
{
    void_list_elt_deinit((void_list_elt_t *) elt);
}

str_list_t *str_list_alloc()
{
     str_list_t *list = (str_list_t *)calloc(1, sizeof(str_list_t));
     if (list)
	  str_list_init(list);
     return list;
}

int str_list_init(str_list_t *list)
{
    return void_list_init((void_list_t *) list);
}

void str_list_deinit(str_list_t *list)
{
    void_list_deinit((void_list_t *) list);
}

int str_list_append(str_list_t *list, char *data)
{
    return void_list_append((void_list_t *) list, data);
}

int str_list_push(str_list_t *list, char *data)
{
    return void_list_push((void_list_t *) list, data);
}

str_list_elt_t *str_list_pop(str_list_t *list)
{
    return (str_list_elt_t *) void_list_pop((void_list_t *) list);
}

str_list_elt_t *str_list_remove(str_list_t *list, str_list_elt_t **iter)
{
    return (str_list_elt_t *) void_list_remove((void_list_t *) list,
					       (void_list_elt_t **) iter);
}

char *str_list_remove_elt(str_list_t *list, const char *target_str)
{
     return (char *)void_list_remove_elt((void_list_t *) list,
					 (void *)target_str,
					 (void_list_cmp_t)strcmp);
}

str_list_elt_t *str_list_first(str_list_t *list) {
    return (str_list_elt_t * )void_list_first((void_list_t *) list);
}

str_list_elt_t *str_list_prev(str_list_t *list, str_list_elt_t *node) {
    return (str_list_elt_t * )void_list_prev((void_list_t *) list, (void_list_elt_t *)node);
}

str_list_elt_t *str_list_next(str_list_t *list, str_list_elt_t *node) {
    return (str_list_elt_t * )void_list_next((void_list_t *) list, (void_list_elt_t *)node);
}

str_list_elt_t *str_list_last(str_list_t *list) {
    return (str_list_elt_t * )void_list_last((void_list_t *) list);
}


void str_list_purge(str_list_t *list) {
    str_list_elt_t *elt;
    while (!void_list_empty(list)) {
        elt = str_list_first(list);
        if (!elt)
            return;
        list_del_init(&elt->node);
        free(elt->data);
        elt->data=NULL;
        free(elt);
    }
}
