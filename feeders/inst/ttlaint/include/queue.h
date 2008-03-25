/*
 * Copyright (C) 2001-2006 Intel Corporation
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>

/* Define queue node types. All queues are assumed to have a head node.
 * An empty queue must be initialized so that its next and prev
 * point to the head node
 */

typedef struct qnode_t {
  struct qnode_t *next;
  struct qnode_t *prev;
} qnode_t, *qnode_ptr;

/* Memory management function prototypes */
qnode_ptr new_item(void *free_item_ptr, size_t item_size, char *name);
qnode_ptr new_zitem(void *free_item_ptr, size_t item_size, char *name);
void free_item(void *free_item_ptr, void *item_ptr);

#endif
