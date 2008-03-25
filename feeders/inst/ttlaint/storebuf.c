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

/* storebuf.c - routines for manipulating the store buffer */

#include <stdio.h>
#include <stdlib.h>

#include "icode.h"
#include "globals.h"
#include "opcodes.h"
#include "protos.h"

/* 
 * This routine updates the store buffer entry using the vaddr of
 * the store. 
 *
 * Tarantula Changes:
 *
 * The actual updates and byte masks are now in the stq_entry structure, to facilitate
 * execution of vector instructions.
 *
 * pthread->StoreQ is the head of the address-entry doubly-linked list.
 * Each sqe_addr contains the head of the updates list
 * The updates list is sequential; updates are inserted in fetch order
 */

void Update_StoreBuffer(thread_ptr pthread, inflight_inst_ptr ifi)
{

  struct stq_addr *sq;
  struct stq_entry *sqe, *update;

  /* The address is in ifi->vaddr */

  if (0) informative("Update_StoreBuffer: fetchnum %lu vaddr %lx paddr %lx data %lu bitmask %lu ",
	      ifi->fetchnum, ifi->vaddr, ifi->paddr, ifi->data, ifi->bitmask); 

  if (ifi->stq_update != NULL) {
    /* 
     * Repeated calls to AINT_issue_memop() get us here. However, our caller has
     * checked for consistency and Cleaned up the STQ state if necessary. Therefore,
     * simply return.
     */
    informative("no update needed\n"); 
    return;
  }

  update = ifi->stq_update =  (struct stq_entry *) malloc(sizeof(struct stq_entry));
  if (update == NULL) fatal("Update StoreBuffer: Cannot allocate update struct\n");
#ifdef NEW_TTL
  if ( ifi->picode->is_vector) informative_ttl(10,"allocating stq_update[%d] = %lx\n",ifi->pos_in_vreg,update);
#endif

  update->fetchnum = ifi->fetchnum; /* To ensure fetch-ordering of updates */
  update->id = ifi - pthread->cbif;

  /*
   * Tarantula: also store the address, data and bitmask in the update struct
   */
  update->vaddr = ifi->vaddr;
  update->data = ifi->data;
  
  update->bitmask = ifi->bitmask;
#ifdef NEW_TTL
  update->pos_in_vreg = ifi->pos_in_vreg;	
#endif

  /* Look up the storeq for existing updates to this address */
  QSEARCH(&pthread->StoreQ, (ifi->vaddr & QWORD_MASK), sq);

  if (sq == &(pthread->StoreQ)) {
    /* no pending updates to this address */
    sq = malloc(sizeof(struct stq_addr));
    if (sq == NULL) fatal("Update_StoreBuffer: cannot allocate storeq address entry\n");
    sq->addr = (ifi->vaddr & QWORD_MASK);
    /* Insert update into list for this addr */
    sq->Updates.prev = sq->Updates.next = update;
    update->prev = update->next = &(sq->Updates);
    
    sq->Updates.fetchnum = 0;
    INLINE_INSERT_AFTER(&(pthread->StoreQ), sq); /* Insert addr into storeq list */
    return;
  }
  
  /* Federico Ardanaz notes:
   * ----------------------------------------------------------------------------- 
   * Well we have a little tricky problem with vector stores of 32 bits cause
   * the difernece in the address of two consecutive elements in a vector are 
   * lost in the AND QWORD_MASK just above.At the same time we will enter here lots of
   * times with the (aparently) same address and the same fetchnum (there is only
   * one vector store!) The original scalar code will think at this point that  we are 
   * replying a store and this is not the case, so we need look at pos_in_vreg
   * to be sure if we are dealing with a true reply or just another element of a vector.
   */

  /* We have an existing storeq address entry. Insert this one in fetch order */
  for (sqe=sq->Updates.next; (sqe != &(sq->Updates)); sqe=sqe->next) {
	register int same_fetch, younger, same_pos = 0;
#ifdef NEW_TTL
  	same_pos 	= sqe->pos_in_vreg 	== ifi->pos_in_vreg;
#endif	
	same_fetch 	= sqe->fetchnum 	== ifi->fetchnum;
	younger     = sqe->fetchnum 	>  ifi->fetchnum;
		
    if (younger || (same_fetch && !same_pos)) {
      /* insert ourself before sqe since they come after us */
      INLINE_INSERT_BEFORE(sqe, update);
      return;
    } else if (same_fetch && same_pos) {
      /* Overwrite our own prior update */
      INLINE_INSERT_BEFORE(sqe, update);
      INLINE_REMOVE(sqe);
      free(sqe);
      return;
    }
  }


  /* We reached the end of the addr list */
  if (sq->Updates.next == &(sq->Updates)) fatal("Update_StoreBuffer: found empty update list\n");
      
  /* Enqueue us here since we are the highest fetchnum */
  INLINE_ENQUEUE(&(sq->Updates), update);
}



/* For store insts, remove pending updates from store queue */
void Cleanup_StoreBuffer(thread_ptr pthread, inflight_inst_ptr ifi, instid_t instid)
{
 int i,vl;

 vl =  ifi->picode->is_vector ? VL : 1;
 
 for ( i = 0; i < vl ; i++ ) {

  /*
   * This 'if' will only be true for VECTOR MEMORY instructions.
   */
  if ( ifi->stq_update_array != NULL ) {
   ifi->stq_update = ifi->stq_update_array[i];
   if ( ifi->stq_update == NULL ) fatal("Cleanup_StoreBuffer: vector stq_update is null\n");
   ifi->vaddr = ifi->stq_update->vaddr;
   
  }

  if (ifi->stq_update) {
#ifdef NEW_TTL
    if ( ifi->picode->is_vector ) informative_ttl(10,"freeing    stq_update[%d] = %lx\n",i,ifi->stq_update);
#endif
    if (ifi->stq_update->next == NULL) 
      warning("(ifi->stq_update->next == NULL)");
    if (ifi->stq_update->prev == NULL) 
      warning("(ifi->stq_update->prev == NULL)");
    INLINE_REMOVE(ifi->stq_update); /* From update list */
    if (ifi->stq_update->next == ifi->stq_update->prev) {
      /* This was the last remaining update to this address */
      struct stq_addr *sq;
      /* Find the stq addr entry for this update */
      QSEARCH(&pthread->StoreQ, (ifi->vaddr & QWORD_MASK), sq);
      if (sq == &pthread->StoreQ) {
       fatal("Cleanup_StoreBuffer: address 0x%lx for instid %ld (pc 0x%lx) not found in store-buffer\n", 
	      ifi->vaddr, instid, ifi->pc);
      }
      INLINE_REMOVE(sq);
      free(sq);
    }
    free(ifi->stq_update);
    ifi->stq_update = NULL;
  }
 }
 if ( ifi->stq_update_array != NULL ) {
#ifdef NEW_TTL
  informative_ttl(10,"freeing stq_update_array = %lx\n",ifi->stq_update_array);
#endif
  free(ifi->stq_update_array);
  ifi->stq_update_array = NULL;
 }
}


/* 
 * Search the storeq for most recent updates 
 *
 * This function is called from AINT_do_read() and gets executed for every load whenever the 
 * performance model decides it's time to 'execute' the load instruction (the PM does so by calling
 * FEED_DoRead which calls AINT_do_read).
 *
 * The goal of this function is to determine if there are pieces of data required by the load
 * that have been written by an older, yet-uncommited store. If so, we get the data from the StoreBuffer.
 * Note that it might happen that the load is only partially fullfilled by the store data. In this
 * case, the 'bitmask' at the end of the loop will be non-zero and when we return to AINT_do_read,
 * it will take care of this fact by reading from real memory the missing pieces of data.
 *
 */
ulong  Read_StoreBuffer(thread_ptr pthread, inflight_inst_ptr ifi, ulong bitmask)
{
  struct stq_addr *sq;
  struct stq_entry *sqe;
  ulong tempmask;
  inflight_inst_ptr u_ifi;

  QSEARCH(&pthread->StoreQ, (ifi->vaddr & QWORD_MASK), sq);
  if (sq == &pthread->StoreQ) return bitmask;

  for (sqe = sq->Updates.prev; sqe != &(sq->Updates); sqe = sqe->prev) {
    /* Do not consider updates from newer stores ? */
    if (sqe->fetchnum > ifi->fetchnum) continue;
    
    /* This is an update from an older store */
    tempmask = bitmask & (sqe->bitmask);
    /* tempmask contains the valid bits from this update */

    /* sqe->data is the actual (unshifted) word written at the actual (un-realigned) address.
     * We need to convert it to a qword-aligned data word
     */
    ifi->data &= (~tempmask);
    ifi->data |= (tempmask & (sqe->data << ((sqe->vaddr & 0x7)<<3)));

    bitmask &= (~tempmask);

    if (bitmask == 0) return 0;
  }
  return bitmask;
}
