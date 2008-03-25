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

/* Memory allocation routines */

#include <stdio.h>

#include "icode.h"
#include "opcodes.h"
#include "globals.h"

static int Max_bucket;

#define ALLOC_HASH_SIZE 64
static avail_ptr Avail_free;

void 
malloc_init(thread_ptr pthread, unsigned long base, unsigned long size)
{
  
  unsigned long newbase;
  unsigned long shift;
  int i, bucket;
  avail_ptr availp;

  /* round base to a oct? word boundary */
  newbase = BASE2ROUNDUP(base, 16);
  size -= newbase - base;
  base = newbase;

  /* find bucket such that 2^bucket <= size */
  for (shift = size, bucket = -1; shift; ++bucket)
    shift >>= 1;

  Max_bucket = bucket;
  pthread->avail = (avail_ptr) calloc(bucket+1, sizeof(avail_t));
  if (pthread->avail == NULL)
    fatal("malloc_init: out of memory.\n");

  /* initialize the head nodes for all the queues */
  for (i = 0; i<= Max_bucket; i++) {
    pthread->avail[i].address = 0;
    pthread->avail[i].size = 0;
    INLINE_INIT_Q(&pthread->avail[i]);
  }

  pthread->alloc = hash_init(ALLOC_HASH_SIZE, A_ENTRY_SIZE);

  NEW_ITEM(Avail_free, sizeof(struct avail), availp, "malloc_init");
  availp -> address = base;
  availp->size = size;
  INLINE_ENQUEUE(&pthread->avail[bucket], availp);
}
