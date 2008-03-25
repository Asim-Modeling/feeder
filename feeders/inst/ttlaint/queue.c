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
 * Routines for generic queue manipulation.
 */


#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "protos.h"
#include "export.h"
/* These routines are generic in the sense that they manipulate
 * arbitrary structures. The only requirement is that the first
 * two fields in a structure correspond to the next and prev
 * fields. This implies that a structure can be on at most one
 * queue at any time.
 *
 * For more general (and more efficient) manipulation of queues, use
 * the inline queue macros.
 *
 * Each queue has a head node which must be initialized with init_q().
 */

/* Initialize the head node for a queue. */
void
init_q(qnode_ptr qhead)
{
    qhead->next = qhead;
    qhead->prev = qhead;
}

/* returns true if the node is on the queue */
int
onqueue_q(qnode_ptr qhead, qnode_ptr pnode)
{
    qnode_ptr pnext;

    for (pnext = qhead->next; pnext != qhead; pnext = pnext->next)
        if (pnext == pnode)
            return 1;
    return 0;
}

/* add a node to the tail of a queue */
void
enqueue_q(qnode_ptr qhead, qnode_ptr pnew)
{
    /* link it into the list */
    pnew->next = qhead;
    pnew->prev = qhead->prev;
    qhead->prev->next = pnew;
    qhead->prev = pnew;
}

/* remove and return a node from the head of the queue */
qnode_ptr
dequeue_q(qnode_ptr qhead)
{
    qnode_ptr pnode;

    pnode = qhead->next;
    if (pnode != qhead) {
        qhead->next = pnode->next;
        pnode->next->prev = qhead;
        return pnode;
    }
    return NULL;
}

/* remove a node from a queue */
void
remove_q(qnode_ptr pnode)
{
    pnode->next->prev = pnode->prev;
    pnode->prev->next = pnode->next;
}

void
q_debug(qnode_ptr q)
{
    qnode_ptr pnode;

    pnode = q;
    printf("0x%p", pnode);
    for (pnode = pnode->next; pnode != q; pnode = pnode->next)
        printf(" --next--> 0x%p", pnode);
    printf("\n");

    pnode = q;
    printf("0x%p", pnode);
    for (pnode = pnode->prev; pnode != q; pnode = pnode->prev)
        printf(" --prev--> 0x%p", pnode);
    printf("\n");
}

/* new_item() and free_item() are routines for managing memory allocation.
 * Any type of struct may be allocated and freed with these routines.
 * To use these routines, define a variable to serve as the "free list"
 * pointer. Inline macro versions of these functions are faster but
 * confuse dbx. If the macros are used and "DEBUG" is also defined,
 * then the macros use these functions instead of inlining the code.
 * This allows the macros to be used with debugging.
 *
 * new_item() uses the first field of the "item" (an arbitrary struct)
 * to store the pointer to the next free item when it is on the free
 * list. This field may be used for anything when it is not on the
 * free list. So the item must be large enough to store a pointer.
 *
 * new_item() takes 3 arguments:
 *    free_item_ptr   is the address of the "free list" pointer, initially NULL
 *    item_size       is the size in bytes of an item
 *    name            is the name of the calling routine
 * new_item() does not zero the item. To allocate an item whose fields are
 * initialized to zero, use new_zitem().
 *
 * free_item() puts an item back on the free list, so that it can be
 * reused later without having to call malloc(). Items are never "freed"
 * in the sense of returning them via "free()". Once an item is allocated,
 * it is never returned to the free pool used by malloc().
 *
 * Example:
 *
 * void *Foo_free;
 * #define FOO_SIZE (sizeof(struct foo))
 * bar()
 * {
 *     struct foo *pfoo;
 *     pfoo = (struct foo *) new_item(&Foo_free, FOO_SIZE, "bar");
 *     free_item(&Foo_free, pfoo);
 * }
 * 
 * To use the macro versions (notice the different calling sequence):
 *
 * bar()
 * {
 *     struct foo *pfoo;
 *     NEW_ITEM(Foo_free, FOO_SIZE, pfoo, "bar");
 *     FREE_ITEM(Foo_free, pfoo);
 * }
 */
qnode_ptr
new_item(void *free_item_ptr, size_t item_size, char *name)
{
    qnode_ptr new_ptr;

#ifdef SLOW_MEMORY
    new_ptr = (qnode_ptr)malloc(item_size);
    if (new_ptr == NULL)
      fatal("new_item failed to allocate %d bytes\n", item_size);
#else

    /* reduce calls to malloc and make more efficient use of space
     * by allocating several structs at once
     */
    if (*(long **)free_item_ptr == NULL) {
        int i;
        qnode_ptr ptmp;
        new_ptr = (qnode_ptr) malloc(NITEMS * item_size);
        if (new_ptr == NULL) {
            fatal("%s: out of memory\n", name);
        }

        /* link all the free structs using the "next" pointer */
        ptmp = new_ptr;
        for (i = 0; i < NITEMS - 1; i++) {
            ptmp->next = (qnode_ptr) ((char *) new_ptr +
                                            ((i + 1) * item_size));
            ptmp = ptmp->next;
        }
        ptmp->next = NULL;
        *(long **)free_item_ptr = (long *)new_ptr;
    }

    /* remove a free item from the front of the list */
    new_ptr = *(qnode_ptr *)free_item_ptr;
    *(long *)free_item_ptr = **(long **)free_item_ptr;
#endif

    return new_ptr;
}

/* Define a macro for allocating a new structure, initialized to zero.
 * Zero the item only when it is about to be used, not when allocating
 * the array. This saves real memory when large items are allocated
 * and not used, and saves time since the item must be zeroed anyway
 * just before it is returned (since it may have been used and then freed).
 */
qnode_ptr
new_zitem(void *free_item_ptr, size_t item_size, char *name)
{
    qnode_ptr new_ptr;

#ifdef SLOW_MEMORY
    new_ptr = (qnode_ptr)malloc(item_size);
    if (new_ptr == NULL)
      fatal("new_zitem failed to allocate %d bytes\n", item_size);
#else
    {
      int i;
      qnode_ptr ptmp;
      /* reduce calls to malloc and make more efficient use of space
       * by allocating several structs at once
       */
      if (*(long **)free_item_ptr == NULL) {
        new_ptr = (qnode_ptr) malloc(NITEMS * item_size);
        if (new_ptr == NULL) {
	  fatal("%s: out of memory\n", name);
        }

        /* link all the free structs using the "next" pointer */
        ptmp = new_ptr;
        for (i = 0; i < NITEMS - 1; i++) {
	  ptmp->next = (qnode_ptr) ((char *) new_ptr +
				    ((i + 1) * item_size));
	  ptmp = ptmp->next;
        }
        ptmp->next = NULL;
        *(long **)free_item_ptr = (long *)new_ptr;
      }

      /* remove a free item from the front of the list */
      new_ptr = *(qnode_ptr *)free_item_ptr;
      *(long *)free_item_ptr = **(long **)free_item_ptr;
      memset(new_ptr, 0, item_size);
    }
#endif
    return new_ptr;
}

/* Free an item by putting it on the free list so it can be reused. */
void
free_item(void *free_item_ptr, void *item_ptr)
{
#ifdef SLOW_MEMORY
  if (item_ptr != NULL)
    free(item_ptr);
#else
    /* put a free item on the front of the list */
    *(long *)item_ptr = *(long *)free_item_ptr;
    *(long **)free_item_ptr = (long *)item_ptr;
#endif
}
