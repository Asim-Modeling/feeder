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

/*
 * Definitions of inline macros for queue manipulation and
 * memory management of fixed-size structs using free lists
 */

#ifndef MACROS_H
#define MACROS_H


#include "queue.h"

#define INLINE_INIT_Q(Q) \
((Q)->next = Q, (Q)->prev = Q)

#define INLINE_REMOVE(T) \
((T)->next->prev = (T)->prev, (T)->prev->next = (T)->next)

#define INLINE_ENQUEUE(Q,T) \
((T)->next=(Q), (T)->prev=(Q)->prev, (Q)->prev->next=(T), (Q)->prev=(T))

#define INLINE_INSERT_AFTER(Q,T) \
((T)->next=(Q)->next, (T)->prev=(Q), (Q)->next->prev=(T), (Q)->next=(T))

#define INLINE_INSERT_BEFORE(Q,T) \
((T)->next=(Q), (T)->prev=(Q)->prev, (Q)->prev->next=(T), (Q)->prev=(T))

#define INLINE_DEQUEUE(Q,T) \
((T)=(Q)->next, (T)==(Q)?(T)=NULL:((Q)->next=(T)->next, (T)->next->prev=(Q)))

#ifndef NITEMS
#define NITEMS 100
#endif

#define NEW_ITEM(FREE_ITEM_PTR, ITEM_SIZE, NEW_PTR, NAME) NEW_PTR = (void *) new_item(&(FREE_ITEM_PTR), ITEM_SIZE, NAME)

#define NEW_ZITEM(FREE_ITEM_PTR, ITEM_SIZE, NEW_PTR, NAME) NEW_PTR = (void *) new_zitem(&(FREE_ITEM_PTR), ITEM_SIZE,  NAME)

#define FREE_ITEM(FREE_ITEM_PTR, ITEM_PTR) free_item(&(FREE_ITEM_PTR), ITEM_PTR)

#endif
