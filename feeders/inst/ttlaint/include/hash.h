/*
 * Header file for hash table data structure.
 *
 * Copyright (C) 1993 by Jack E. Veenstra (veenstra@cs.rochester.edu)
 * 
 * This file is part of MINT, a MIPS code interpreter and event generator
 * for parallel programs.
 * 
 * MINT is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 * 
 * MINT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MINT; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef HASH_H
#define HASH_H

/* #include "common.h" */

/*
 * Mar 16, 1993		Jack Veenstra		veenstra@cs.rochester.edu
 *    Created.
 *
 * Apr 24, 1993		Jack Veenstra		veenstra@cs.rochester.edu
 *    Added hash_init().
 *
 * Nov 3, 1993		Jack Veenstra		veenstra@cs.rochester.edu
 *    Moved the static global variables into a structure so that multiple
 * instances of the hash table can be used within the same program. This
 * changes the interface in an unavoidably incompatible way.
 */

/* These routines implement a hash table. Each routine takes a pointer to
 * a hash table object as the first argument. The routines hash_lookup()
 * and hash_insert() take a 32-bit address as the second argument and
 * return a pointer to a user-defined structure if found. If not
 * found, hash_lookup() returns NULL, while hash_insert() allocates
 * space for a new entry and returns a pointer to it. hash_remove()
 * removes an entry.
 *
 * The program using the hash table routines should define a structure
 * for containing information about each entry. The structure must
 * include as its first field an unsigned long field to store the address.
 * The second field must be a pointer so that this structure can be linked
 * on a list. Other fields are optional. The size of the structure
 * must be passed as the second argument to hash_init().
 *
 * Example:
 *
 * struct h_entry {
 *     unsigned long addr;
 *     void *next;
 *     unsigned long accessed;
 *     unsigned long written;
 * };
 * #define ENTRY_SIZE (sizeof(struct h_entry))
 *
 * The program should declare a variable of type "hash_tab_ptr" to
 * point to the hash table object and call hash_init() with the number of
 * entries as the first argument. The number of entries will be rounded up
 * to a power of two. The size of each user-defined entry is passed in as
 * the second argument.
 *     Example:
 *
 *     hash_tab_ptr Addrs;
 *     Addrs = hash_init(64 * 1024, ENTRY_SIZE);
 *
 * Then the call to lookup an address and insert it if not found
 * would look like:
 *
 *     hashp = (struct h_entry *) hash_insert(Addrs, addr);
 *
 * To lookup an address without inserting it if it's not found:
 *
 *     hashp = (struct h_entry *) hash_lookup(Addrs, addr);
 */

typedef int (*PFI)();
typedef void (*PFV)();

/* Basic structure of a hash_t (hash table entry). The user-defined hentry
 * may contain any number of fields but the first field must be the
 * address, and the second field must be a pointer.
 */
typedef struct hash_entry_t {
    unsigned long addr;
    struct hash_entry_t *next;
} hash_entry_t, *hash_entry_ptr;

typedef struct hash_tab_t {
    hash_entry_ptr hash_free;
    unsigned long hash_mask;
    int hash_tab_size;
    int size;
    hash_entry_ptr table[1];		/* Allocated space is larger
                                         * than it appears. :-) */
} hash_tab_t, *hash_tab_ptr;

#ifdef __cplusplus
extern "C" {
#endif

hash_tab_ptr hash_init(int nentries, int size);
hash_entry_ptr hash_insert(hash_tab_ptr htab, unsigned long addr);
hash_entry_ptr hash_insert_size(hash_tab_ptr htab, unsigned long addr, int size);
hash_entry_ptr hash_insert_buf(hash_tab_ptr htab, unsigned long addr,
                                void *buf);
hash_entry_ptr hash_lookup(hash_tab_ptr htab, unsigned long addr);
void hash_apply(hash_tab_ptr htab, PFI func);
void hash_apply1(hash_tab_ptr htab, PFI func, void *vp1);
void hash_apply2(hash_tab_ptr htab, PFI func, void *vp1, void *vp2);
int hash_free(hash_tab_ptr htab);
int hash_space(hash_tab_ptr htab);
hash_entry_ptr hash_remove(hash_tab_ptr htab, unsigned long addr);
hash_entry_ptr hash_free_entry(hash_tab_ptr htab, unsigned long addr);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* !__hash_h */
