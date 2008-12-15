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
#include <string.h>
#include <stdlib.h>


void active_list_init(struct active_list *ptr) {
    INIT_LIST_HEAD(&ptr->node);
    INIT_LIST_HEAD(&ptr->depend);
    ptr->depended = NULL;
}

/**
 */ 
struct active_list * active_list_next(struct active_list *head, struct active_list *ptr) {
    struct active_list *next=NULL;
    if ( !head ) {
        fprintf(stderr, "active_list_next head = %p, ptr = %p invalid value!!\n", head, ptr);
        return NULL;
    }
    if ( !ptr )
        ptr = head;
    next = list_entry(ptr->node.next, struct active_list, node);
    if ( next == head ) {
        return NULL;
    }
    if ( ptr->depended && &ptr->depended->depend == ptr->node.next ) {
        return ptr->depended;
    }
    while ( next->depend.next != &next->depend ) {
        next = list_entry(next->depend.next, struct active_list, node); 
    }
    return next;
}


struct active_list * active_list_prev(struct active_list *head, struct active_list *ptr) {
    struct active_list *prev=NULL;
    if ( !head ) {
        fprintf(stderr, "active_list_prev head = %p, ptr = %p invalid value!!\n", head, ptr);
        return NULL;
    }
    if ( !ptr )
        ptr = head;
    if ( ptr->depend.prev != &ptr->depend ) {
        prev = list_entry(ptr->depend.prev, struct active_list, node);
        return prev;
    } 
    if ( ptr->depended  && ptr->depended != head && &ptr->depended->depend == ptr->node.prev ) {
        prev = list_entry(ptr->depended->node.prev, struct active_list, node);
    } else 
        prev = list_entry(ptr->node.prev, struct active_list, node);
    if ( prev == head )
        return NULL;
    return prev;
}

static void list_head_clear (struct list_head *head) {
    struct active_list *next;
    struct list_head *n, *ptr;
    if (!head)
        return;
    list_for_each_safe(ptr, n , head) {
        next = list_entry(ptr, struct active_list, node);
        if (next->depend.next != &next->depend) {
            list_head_clear(&next->depend);
        }
        active_list_init(next);
    }
}
void active_list_clear(struct active_list *head) {
    list_head_clear(&head->node);
    if (head->depend.next != &head->depend) {
        list_head_clear(&head->depend);
    }
    active_list_init(head);
}

void active_list_add_depend(struct active_list *node, struct active_list *depend) {
    list_del_init(&depend->node);
    list_add_tail(&depend->node, &node->depend);
    depend->depended  = node;
}

void active_list_add(struct active_list *head, struct active_list *node) {
    list_del_init(&node->node);
    list_add_tail(&node->node, &head->node);
    node->depended  = head;
}

struct active_list * active_list_head_new() {
    struct active_list * head = calloc(1, sizeof(struct active_list));
    active_list_init(head);
    return head;
}

void active_list_head_delete(struct active_list *head) {
    active_list_clear(head);
    free(head);
}

