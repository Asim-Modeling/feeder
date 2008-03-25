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
 * Prototype declarations for functions used in several source files
 */

#ifndef PROTOS_H
#define PROTOS_H

#include <malloc.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <standards.h>
#include "/usr/include/assert.h"

#include "globals.h"
#include "sim.h"
#include "icode.h"

/* avoid a name clash with EVSIM */
#ifndef error
#define error _aint_error
#endif

extern void myassert __((unsigned int bool, const char *, ...));
extern void fatal __((const char *, ...));
extern void error __((const char *, ...));
extern void warning __((const char *, ...));
extern void notice __((const char *, ...));
extern void informative __((const char *, ...));
extern void informative_ttl __((int level,const char *, ...));

extern void wakeup_thr(thread_ptr pthread);
extern void sleep_thr(thread_ptr pthread);
extern void aint_cvt_fp_mem(unsigned int opnum, double fpreg, long *memloc);
ulong tlb_miss(process_ptr process, ulong addr, long *paccess,
	       long vpage, long tbkey, long tbtag);

void *allocate_pages(long nbytes);
void *allocate_fixed(long addr, long nbytes);


double get_fpcr();
void put_fpcr(double fpcr);




extern void subst_init();

extern void init_main_thread(thread_ptr pthread);
void init_thread(thread_ptr pthread, event_ptr pevent);

void init_process(process_ptr process);
void print_reg_state(int tid);

segment_t *process_add_segment(process_ptr process, ulong start, ulong size, int prot, int flags);
segment_t *process_find_segment(process_ptr process, ulong addr);

int segment_contains(segment_t *segment, ulong addr);

void memcpy_from_object_space(process_ptr, char *aint_ptr, ulong object_ptr, size_t n);
void memcpy_to_object_space(process_ptr process, ulong object_ptr, const void *aint_vptr, size_t size);
void strcpy_from_object_space(process_ptr, char *aint_ptr, ulong object_ptr);
void strcpy_to_object_space(process_ptr process, ulong object_ptr, const char *aint_ptr);

size_t strlen_from_object_space(process_ptr, ulong object_ptr);
char *strdup_from_object_space(process_ptr, ulong object_ptr);


void copy_addr_space(thread_ptr parent, thread_ptr child);
void init_thread_addr_space(thread_ptr parent, thread_ptr child);

/* in symtab.c */
void read_hdrs(const char *objfile);

/* in loader.c */
void load_object_file(process_t *process, const char *exename);
void decode_instr(process_ptr process, ulong textaddr, unsigned instr, 
		  icode_ptr picode, icode_ptr next_picode);
void decode_text_region(process_ptr process, ulong start, size_t size);

void object_file_info(const char *objfile_name, 
		      ulong *pstack_end, 
		      int *pdynamic,
                      ulong *pbrk);
unsigned int picode_instr(thread_ptr pthread, icode_ptr picode);
const char *aint_lookup_symbol_name(process_ptr process, ulong addr);

int AINT_kill_thread (thread_ptr pthread, int sig);
int handle_signals_for_issue(int sig, void *vinfo, void *vctxt);
void AINT_regmap_snapshot(Physical_RegNum *snapshot, thread_ptr pthread, instid_t current_instid);

char *read_snapshot_objname(const char *);
void snapshot_process(process_ptr, const char *filename);
void restore_process(process_ptr, const char *filename);

void snapshot_subst(FILE *out);
void restore_subst(FILE *out);

/* make inlining optional, for better gprof or hiprof */
#ifndef DO_INLINING
#define DO_INLINING
#endif

ulong addr2phys(process_ptr process, long addr, long *paccess);


/* make inlining optional, for better gprof or hiprof */
int addr2tbeflags(process_ptr process, long addr);

#define BASE2ROUNDDOWN(v_, size_) ((v_) & ~((size_) - 1))
#define BASE2ROUNDUP(v_, size_) (((v_) + (size_)-1) & ~((size_) - 1))


/* Storebuffer manipulation */
void Update_StoreBuffer(thread_ptr pthread, inflight_inst_ptr ifi);
void Cleanup_StoreBuffer(thread_ptr pthread, inflight_inst_ptr ifi, instid_t instid);
ulong Read_StoreBuffer(thread_ptr pthread, inflight_inst_ptr ifi, ulong bitmask);

#ifndef DEBUG_AINT
#define DEBUG_AINT 0
#endif

#endif

