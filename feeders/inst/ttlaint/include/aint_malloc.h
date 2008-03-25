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

#ifndef MALLOC_H
#define MALLOC_H

/* Structure for information about allocated blocks. Stored in a hash
 * table indexed by the address
 */
typedef struct a_entry {
  unsigned long address;
  struct a_entry *next;
  unsigned int size;
  int bucket;
} a_entry_t, *a_entry_ptr;

#define A_ENTRY_SIZE (sizeof(struct a_entry))

/* Structure for free blocks. Each entry is stored
 * in a linked list. There is a separate linked list for each different 
 * size block. The sizes are powers of two
 */
typedef struct avail {
  struct avail *next;
  struct avail *prev;
  unsigned long address;
  int size;
} avail_t, *avail_ptr;



#endif
