/*
 * Routines to implement a hash table. Given a 32-bit
 * address, it returns the corresponding user-defined structure.
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

/* Change log:
 *
 * Mar 16, 1993		Jack Veenstra		veenstra@cs.rochester.edu
 *     Created.
 *
 * Apr 24, 1993		Jack Veenstra		veenstra@cs.rochester.edu
 *    Added hash_init().
 * 
 * Mar 12, 1994		Jack Veenstra		veenstra@cs.rochester.edu
 *     Replaced calls to "fatal()" with "fprintf()".
 */

#include <stdio.h>
#include <stdlib.h>

#include "hash.h"
#include "export.h"


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

#define HASH_FUNCTION(addr, mask) ((((addr) >> 14) ^ (((addr) >> 2) & 0xffff)) & mask)

/* hash_init() allocates space for a hash table containing "nentries"
 * number of pointers. The second argument is the size of each user-defined
 * entry. This is used in hash_insert().
 *
 * The number of requested entries is rounded up to a power of two
 * so that the hash index can be computed efficiently.
 *
 * The return value is a pointer to a hash table, which is passed as
 * an argument to the lookup and insert routines.
 */
hash_tab_ptr
hash_init(int nentries, int size)
{
    int exp, tab_size;
    hash_tab_ptr htab;

    /* find power of two that is greater than or equal to "nentries" */
    for (exp = 1; exp < nentries; exp <<= 1)
        ;

    /* There is already one pointer allocated in "table[1]" so subtract 1
     * from exp.
     */
    tab_size = sizeof(struct hash_tab_t) + (exp - 1) * sizeof(hash_entry_ptr);
    htab = (hash_tab_ptr) calloc(1, tab_size);
    if (htab == NULL) {
#ifdef HASH_REPORT
      fatal("hash_init: out of memory, allocating %u pointers\n", exp);
#endif
      return NULL;
    }
    htab->hash_tab_size = exp;
    htab->hash_mask = exp - 1;
    htab->size = size;
    return htab;
}

/* hash_lookup() takes as arguments a hash table (an array of pointers to
 * hash table entries), and an address, and returns a pointer to the
 * entry for that address. If the entry is not found, NULL is returned.
 */
hash_entry_ptr
hash_lookup(hash_tab_ptr htab, unsigned long addr)
{
    int index;
    hash_entry_ptr hptr;

    index = HASH_FUNCTION(addr, htab->hash_mask);
    for (hptr = htab->table[index]; hptr; hptr = hptr->next)
        if (hptr->addr == addr)
            break;
    return hptr;
}

/* hash_insert() is like hash_lookup() except that an entry is inserted if
 * not found.  If the entry is found in the hash table, a pointer to it is
 * returned.  If not found, "size" bytes are allocated, and the entry is
 * inserted into the hash table. The "size" was specified in the call to
 * hash_init().
 */
hash_entry_ptr
hash_insert(hash_tab_ptr htab, unsigned long addr)
{
    int index;
    hash_entry_ptr hptr;

    index = HASH_FUNCTION(addr, htab->hash_mask);
    for (hptr = htab->table[index]; hptr; hptr = hptr->next)
        if (hptr->addr == addr)
            break;
    if (hptr == NULL) {
        NEW_ZITEM(htab->hash_free, htab->size, hptr, "hash_insert");
        hptr->addr = addr;
        hptr->next = htab->table[index];
        htab->table[index] = hptr;
    }
    return hptr;
}

/* hash_insert_size() is like hash_insert() except that the size field is
 * specified instead of using the default size specified at initialization
 * time.  If the entry is found in the hash table, a pointer to it is
 * returned.  If not found, "size" bytes are allocated (with calloc), and
 * the entry is inserted into the hash table.
 */
hash_entry_ptr
hash_insert_size(hash_tab_ptr htab, unsigned long addr, int size)
{
    int index;
    hash_entry_ptr hptr;

    index = HASH_FUNCTION(addr, htab->hash_mask);
    for (hptr = htab->table[index]; hptr; hptr = hptr->next)
        if (hptr->addr == addr)
            break;
    if (hptr == NULL) {
        /* Cannot use NEW_ZITEM() here if the size is non-standard. */
        if (size == htab->size) {
            NEW_ZITEM(htab->hash_free, size, hptr, "hash_insert_size");
        } else {
            hptr = (hash_entry_ptr) calloc(1, size);
            if (hptr == NULL) {
#ifdef HASH_REPORT
                fprintf(Aint_output, "hash_insert_size: out of memory.\n");
#endif
                return NULL;
            }
        }
        hptr->addr = addr;
        hptr->next = htab->table[index];
        htab->table[index] = hptr;
    }
    return hptr;
}

/* hash_insert_buf() is like hash_insert() except that the buffer space to
 * use for the new entry is specified by the "buf" pointer.  If the entry
 * is found in the hash table, a pointer to it is returned.  If not found,
 * the space pointed to by "buf" is assumed to be large enough to hold the
 * new entry and the entry is inserted in the hash table and the buf
 * pointer is returned.
 */
hash_entry_ptr
hash_insert_buf(hash_tab_ptr htab, unsigned long addr, void *buf)
{
    int index;
    hash_entry_ptr hptr;

    index = HASH_FUNCTION(addr, htab->hash_mask);
    for (hptr = htab->table[index]; hptr; hptr = hptr->next)
        if (hptr->addr == addr)
            break;
    if (hptr == NULL) {
        hptr = (hash_entry_ptr) buf;
        if (hptr == NULL) {
#ifdef HASH_REPORT
            fprintf(Aint_output, "hash_insert_buf: buf pointer is NULL.\n");
#endif
            return NULL;
        }
        hptr->addr = addr;
        hptr->next = htab->table[index];
        htab->table[index] = hptr;
    }
    return hptr;
}

/* hash_remove() removes an element from the hash table. It does NOT
 * put the element on the free list. It returns a pointer to the entry.
 * This function should be used to remove elements of non-standard
 * size before freeing the hash table object.
 */
hash_entry_ptr
hash_remove(hash_tab_ptr htab, unsigned long addr)
{
    int index;
    hash_entry_ptr hptr, prev;

    index = HASH_FUNCTION(addr, htab->hash_mask);
    prev = NULL;
    for (hptr = htab->table[index]; hptr; prev = hptr, hptr = hptr->next)
        if (hptr->addr == addr)
            break;
    if (hptr == NULL) {
#ifdef HASH_REPORT
        fprintf(Aint_output, "hash_remove: entry for 0x%x not found.\n", addr);
#endif
        return NULL;
    }
    if (prev == NULL)
        htab->table[index] = hptr->next;
    else
        prev->next = hptr->next;
    return hptr;
}

/* hash_free_entry() removes an element from the hash table and puts
 * the element on the free list for subsequent use.  This function should
 * be called only for elements whose size is the same as the size passed in
 * the hash_init() call. This routine returns a pointer to the freed
 * entry. The first field (the address) is used to link the entry on
 * a free list so that field should not be accessed but the other fields
 * are still valid even though the entry is on the free list. This supports
 * an optimization where the entry can be looked up and removed in one
 * call.
 */
hash_entry_ptr
hash_free_entry(hash_tab_ptr htab, unsigned long addr)
{
    int index;
    hash_entry_ptr hptr, prev;

    index = HASH_FUNCTION(addr, htab->hash_mask);
    prev = NULL;
    for (hptr = htab->table[index]; hptr; prev = hptr, hptr = hptr->next)
        if (hptr->addr == addr)
            break;
    if (hptr == NULL) {
#ifdef HASH_REPORT
        fprintf(Aint_output, "hash_free_entry: entry for 0x%x not found.\n", addr);
#endif
        return NULL;
    }
    if (prev == NULL)
        htab->table[index] = hptr->next;
    else
        prev->next = hptr->next;
    FREE_ITEM(htab->hash_free, hptr);
    return hptr;
}

/*
 * hash_apply() calls func(), a user-supplied function, for every entry
 * in the hash table. A pointer to a hash table entry is passed to the
 * function. The user function should return TRUE (1) in order
 * to continue processing entries. To "break out of the loop" and
 * discontinue processing entries, the user function should return
 * FALSE (0).
 *    The entries are passed to the user function in the order in which
 * they occur in the hash table. It might be useful to be able to pass
 * the entries in increasing address order, but this is not supported yet.
 */
void
hash_apply(hash_tab_ptr htab, PFI func)
{
    int index;
    hash_entry_ptr hptr;

    for (index = 0; index < htab->hash_tab_size; index++)
        for (hptr = htab->table[index]; hptr; hptr = hptr->next)
            if ((*func)(hptr) == 0)
                return;
}

/* 1 extra argument is passed to the user-provided "func" function */
void
hash_apply1(hash_tab_ptr htab, PFI func, void *vp1)
{
    int index;
    hash_entry_ptr hptr;

    for (index = 0; index < htab->hash_tab_size; index++)
        for (hptr = htab->table[index]; hptr; hptr = hptr->next)
            if ((*func)(hptr, vp1) == 0)
                return;
}

/* 2 extra arguments are passed to the user-provided "func" function */
void
hash_apply2(hash_tab_ptr htab, PFI func, void *vp1, void *vp2)
{
    int index;
    hash_entry_ptr hptr;

    for (index = 0; index < htab->hash_tab_size; index++)
        for (hptr = htab->table[index]; hptr; hptr = hptr->next)
            if ((*func)(hptr, vp1, vp2) == 0)
                return;
}

/*
 * hash_free() frees the space used by the hash table entries and returns
 * the amount of space used.
 */
int
hash_free(hash_tab_ptr htab)
{
    int index, space;
    hash_entry_ptr hptr, next;

    space = htab->hash_tab_size * sizeof(hash_entry_ptr);
    for (index = 0; index < htab->hash_tab_size; index++)
        for (hptr = htab->table[index]; hptr; hptr = next) {
            next = hptr->next;
            FREE_ITEM(htab->hash_free, hptr);
            space += htab->size;
        }
    return space;
}

/*
 * hash_space() returns the amount of space used in the hash table.
 */
int
hash_space(hash_tab_ptr htab)
{
    int index, space;
    hash_entry_ptr hptr, next;

    space = htab->hash_tab_size * sizeof(hash_entry_ptr);
    for (index = 0; index < htab->hash_tab_size; index++)
        for (hptr = htab->table[index]; hptr; hptr = next) {
            next = hptr->next;
            space += htab->size;
        }
    return space;
}

/* for debugging */
void
hash_print_table(hash_tab_ptr htab)
{
    int index;
    hash_entry_ptr hptr;

    for (index = 0; index < htab->hash_tab_size; index++) {
        printf("[%d]:", index);
        for (hptr = htab->table[index]; hptr; hptr = hptr->next)
            printf(" 0x%lx", hptr->addr);
        printf("\n");
    }
}
