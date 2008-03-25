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

#ifndef __symtab_h
#define __symtab_h

#ifndef FUNC_HASH_SIZE
#define FUNC_HASH_SIZE 128
#endif

typedef struct func_name_t {
    unsigned long addr;
    struct func_name_t *next;
    char *fname;
} func_name_t, *func_name_ptr;

#define FUNC_ENTRY_SIZE (sizeof(struct func_name_t))

typedef struct namelist {
    char *n_name;
    int n_type;
    unsigned long n_value;
} namelist_t, *namelist_ptr;

int namelist(char *objname, namelist_ptr pnlist);

struct file_info {
    long addr;
    char *fname;
    int linelow;
    unsigned char *lptr;
};

extern void initJSRTargets(char *objfile);
extern ulong findJSRTargetPC(ulong pc);


#endif /* __symtab_h */
