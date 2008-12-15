/* active_list.h - the opkg package management system

   Tick Chen <tick@openmoko.com>

   Copyright (C) 2008 Openmoko Inc. 

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/


#include "active_list.h"
#include <stdio.h>


void active_list_init(struct active_list *ptr) {
    INIT_LIST_HEAD(&ptr->node);
    INIT_LIST_HEAD(&ptr->depend);
    ptr->walked=0;
    ptr->depended = NULL;
}

/**
 */ 
struct active_list * active_list_next(struct active_list *head, struct active_list *ptr) {
    struct active_list *next=NULL;
    if (!head || !ptr) {
        fprintf(stderr, "active_list_prev head = %p, ptr = %p invalid value!!\n", head, ptr);
        return NULL;
    }
    if (ptr == head) {
        head->walked = !head->walked;
        next = list_entry(ptr->node.next, struct active_list, node);
        if (next == head)
            return NULL;
        return active_list_next(head, next);
    }
    if (ptr->depend.next != &ptr->depend) {
        next = list_entry(ptr->depend.next, struct active_list, node);
        if (head->walked != next->walked) {
            ptr->walked = head->walked;
            return active_list_next(head, next);
        }
    } 
    if (ptr->walked != head->walked) {
        ptr->walked = head->walked;
        return ptr;
    }

    if (ptr->depended && ptr->node.next == &ptr->depended->depend ) {
        return ptr->depended;
    }

    if (ptr->node.next != &head->node) {
        next = list_entry(ptr->node.next, struct active_list, node);
        return active_list_next(head, next);
    } 
    return NULL;
}


void active_list_clear(struct active_list *head) {
}

void active_list_add_depend(struct active_list *node, struct active_list *depend) {
    list_del_init(&depend->node);
    list_add_tail(&depend->node, &node->depend);
    depend->depended  = node;
}

void active_list_add(struct active_list *head, struct active_list *node) {
    list_del_init(&node->node);
    list_add_tail(&node->node, &head->node);
}
