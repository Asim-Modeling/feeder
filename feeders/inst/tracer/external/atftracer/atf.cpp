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

/***********************************************************************/
#pragma ident "$Id: atf.cpp 799 2006-10-31 12:27:52Z cjbeckma $"
/***********************************************************************
 *
 *	ABSTRACT:	Arana Trace Format
 *
 *	AUTHOR:		snyder@segsrv.shr.intel.com
 *
 ***********************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//
// If we're working on LINUX machines, then get the include files from
// the atf directory.  Otherwise, get them from the system directories. 
#if defined (HOST_LINUX) || defined (HOST_LINUX_X86) || defined(HOST_FREEBSD) || defined(HOST_FREEBSD_X86)
#include "atom.inst.h"
#else
#include <cmplrs/atom.inst.h>
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/resource.h>

#ifdef EV7
#include "machine.h"
#include "evsim.h"
#endif

#define ATF_H_INTERNALS	1	/* Give me the low level internals also please! */	
#include "atf.h"
#include "disasm.h"

/**********************************************************************/
/* User Changable */

#define ATF_CREATE_DUMMY_PTE  0       /* If true, create dummy PTEs (pa=va) @ ATF_write_instruction() */
#define ATF_DEBUG_PTE_VERBOSE 0       /* If true, verbose PTE checks @ ATF_write_instruction and tlb misses */

#ifndef ATF_PERFORMANCE
#define ATF_PERFORMANCE	0	/* If true, print compression performance info */
#endif

#define	ATF_CACHE

#ifndef ATF_STREAMS_MAX
# ifdef MAX_THREADS
#  define ATF_STREAMS_MAX MAX_THREADS
# else
#  define ATF_STREAMS_MAX 1	/* Number of streams allowed to be open */
# endif
#endif

/* Debugging... Print message if debug level is high enough for this module */
#ifndef ATF_DEBUG
# define ATF_DEBUG 0	/* Module minimum debug level... 2-9, with 4 being the normal value, 0=off */
#endif

/**********************************************************************/
/* Calculated -- Don't hardcode */

/**********************************************************************/
/* PTE Caching */

#define	ATF_TLB_ASSOC		64		/* Associative entries in TLB.. Caller of writer must have <= this number entries, and must be <8192 */
#define ATF_TLB_RECENTS		8		/* Remember last 8 accesses and don't replace them (must be power of 2) */

typedef struct {
    Address	vpage;		/* Virtual page address */
    u64		asn;		/* Address space number */
    u64		pte;		/* Page table entry */
    Address	ppage;		/* Physical page (decoded from pte) */
} Atf_Tlb_Entry;

/**********************************************************************/
/* Per stream storage */

typedef struct {
    Address	vpc;		/* Virtual program counter */
    Address	ppc;		/* Physical program counter */
    u64		local_var[avl_MAX];	/* Value of each variable */

    Atf_Tlb_Entry tlb[ATF_TLB_ASSOC];	/* TLB entries */
    int		tlb_lru;		/* TLB entry to fill next */
    int		tlb_recent_lru;		/* Next recent */
    int		tlb_recent[ATF_TLB_RECENTS];	/* Last N entry numbers that were read */
    } Atf_Cpu;

typedef struct {
    FILE	*fp;		/* File pointer */
    Atf_Cpu	*cpu_ptr;	/* Current cpu */
    Atf_Cpu	*limit_cpu_ptr;	/* Which CPU to limit instructions to, NULL if every CPU */
    Boolean	is_dumping;	/* Am dumping cache */
    uint	instr_left;		/* Number of sequential instructions left to return */

    char	*filename;	/* Current filename */
    Boolean	is_writing;	/* True if writing on this stream */
    Boolean	piping;		/* True if piping read/write output */

    u64		global_var[avg_MAX];	/* Value of each variable */

    Atf_Cpu	cpu[ATF_CPU_MAX];

    } Atf_Stream;

Atf_Stream  atf_stream[ATF_STREAMS_MAX];

/**********************************************************************/
/* Instruction cache structures */

/* Each instruction encountered has a opcode payload passed to any simulator */
/* Also the last EA is stored to aid in compressing loads to constant EAs */
typedef struct {
    Address		ea;		/* Last effective address if this is a load */
    Atf_Instr_Op_Etc	opcode_etc;	/* Last opcode, etc for this PA (passed to simulator) */
} Atf_Instr_Entry;

Atf_Instr_Entry *****atf_instructions;

/**********************************************************************/
/* Performance */

#if ATF_PERFORMANCE
#define IF_ATF_PERFORMANCE(x) x
#else
#define IF_ATF_PERFORMANCE(x)
#endif

#if ATF_PERFORMANCE
typedef struct {
    Counter	token_count_table[256];         /* How often each token was used */
    Counter	command_ext_count_table[256];	/* How often each command extension was used */
    Counter	comments;                       /* Bytes in comments */
    Counter	file_size;                      /* File size */
    Counter	pages[4];                       /* Number of pages added */
} Atf_Stats;
Atf_Stats atf_stats;

int	atf_token_length[256];
int	atf_command_ext_length[256];
#endif /*ATF_PERFORMANCE*/

/**********************************************************************/
/* Forward declarations */


/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/* Generic Utility */

#define ATF_OUT_OF_MEMORY()	fprintf(stderr,"Error!  In %s %ld, ran out of memory\n", __FILE__, __LINE__);

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/* Address translation */

#define IS_PAL(vpc)		(vpc & 0x1)

#define IS_ADDR_DIRECT		((ast_ptr->cpu_ptr->local_var[avl_SUPPAG] & asp_DIRECT))
#define IS_ADDR_SP_EV5(va)	((ast_ptr->cpu_ptr->local_var[avl_SUPPAG] & asp_SP_EV5)   && (((va >> 30) & 0x1fff) == 0x1ffe))
#define IS_ADDR_SP_EV6(va)	((ast_ptr->cpu_ptr->local_var[avl_SUPPAG] & asp_SP_EV6)   && (((va >> 30) & 0x3ffff) == 0x3fffe))
#define IS_ADDR_KSEG_EV5(va)	((ast_ptr->cpu_ptr->local_var[avl_SUPPAG] & asp_KSEG_EV5) && (((va >> 41) & 0x3) == 0x2))
#define IS_ADDR_KSEG_EV6(va)	((ast_ptr->cpu_ptr->local_var[avl_SUPPAG] & asp_KSEG_EV6) && (((va >> 46) & 0x3) == 0x2))
#define IS_ADDR_ASM		((ast_ptr->cpu_ptr->local_var[avl_ASM]))

/* Combination of all of above... maps to super page */
#define IS_ADDR_SUPPAG(va)	(IS_ADDR_DIRECT \
				 || IS_ADDR_SP_EV5(va) \
				 || IS_ADDR_SP_EV6(va) \
				 || IS_ADDR_KSEG_EV5(va) \
				 || IS_ADDR_KSEG_EV6(va) \
				 || IS_ADDR_ASM	)

#if ATF_DEBUG_PTE_VERBOSE
#define DBG_MSG(message, stream_id)             \
{                                               \
    fprintf(stderr, message);                   \
    if (atf_stream[stream_id].is_writing)       \
        ATF_write_comment(stream_id, message);  \
    fflush(stdout); fflush(stderr);             \
}
#else
#define DBG_MSG(message, stream_id)
#endif

/* Cache last 2 translations to avoid TLB lookup all the time */
static Address last_vpage = ~0;
static Address last_vpage2 = ~0;
static Address last_ppage = 0;
static Address last_ppage2 = 0;
static int last_cpu;
static int last_cpu2;
static int last_gh = 0;
static int last_gh2 = 0;
static u64 last_asn = 0;
static u64 last_asn2 = 0;

/*extern CPUWarning(char *text, ...);*/

void ATF_tlb_dump_dbg (Address va)
{
    int ent;
    Atf_Tlb_Entry *tlbe_ptr;

    for (ent=0; ent<ATF_TLB_ASSOC; ent++) {
	tlbe_ptr = &atf_stream[0].cpu_ptr->tlb[ent];
        if (tlbe_ptr->vpage == PAGE_ADDR_PAGE(va))
            /*CPUWarning("Found in ATF TLB ent=%3d vp=%016lx pte=%016lx asn=%lu *************\n",*/
            fprintf(stderr, "Found in ATF TLB ent=%3d vp=%016lx pte=%016lx asn=%lu *************\n",
                            ent, tlbe_ptr->vpage, tlbe_ptr->pte, tlbe_ptr->asn);
    }
}

static Atf_Tlb_Entry *atf_tlb_find (Atf_Stream *ast_ptr, Address va, u64 asn)
    /* Return entry which matches virtual page */
{
    int ent;
    Atf_Tlb_Entry *tlbe_ptr;
    Address vpage = PAGE_ADDR_PAGE(va);

    for (ent=0; ent<ATF_TLB_ASSOC; ent++) {
	tlbe_ptr = &ast_ptr->cpu_ptr->tlb[ent];
	if (   (tlbe_ptr->vpage == vpage)
            && (    tlbe_ptr->asn == asn
                || (tlbe_ptr->pte & 0x10)))		/* ASM is set, asn doesn't matter */
        {
	    ast_ptr->cpu_ptr->tlb_recent_lru++;
	    ast_ptr->cpu_ptr->tlb_recent [ast_ptr->cpu_ptr->tlb_recent_lru & (ATF_TLB_RECENTS-1)] = ent;
	    return (tlbe_ptr);
	}
    }
    return (NULL);
}

static Atf_Tlb_Entry *atf_tlb_find_valid (Atf_Stream *ast_ptr, Address va, u64 asn)
{
    Atf_Tlb_Entry *tlbe_ptr = atf_tlb_find (ast_ptr, va, asn);
    if ((tlbe_ptr != NULL) && (tlbe_ptr->pte & 0x1))	/* if found and VALID */
        return (tlbe_ptr);
    else
        return (NULL);
}

static int atf_tlb_new (Atf_Stream *ast_ptr)
    /* Return new entry number */
{
    int ent;
    ast_ptr->cpu_ptr->tlb_lru = (ast_ptr->cpu_ptr->tlb_lru + 1) % ATF_TLB_ASSOC;
    while (1) {
	for (ent=0; ent < ATF_TLB_RECENTS; ent++) {
	    if (ast_ptr->cpu_ptr->tlb_recent[ent] == ast_ptr->cpu_ptr->tlb_lru) {
		ast_ptr->cpu_ptr->tlb_lru = (ast_ptr->cpu_ptr->tlb_lru + 1) % ATF_TLB_ASSOC;
		break;
	    }
	}
	if (ent >= ATF_TLB_RECENTS) break;
    }
    /* WARNING... There's a peek into tlb_lru in a caller to determine what entry was loaded */
    return (ast_ptr->cpu_ptr->tlb_lru);
}

static Address atf_tlb_translate (Atf_Stream *ast_ptr, Address va, Boolean is_istream)
    /* Return pa for given va */
{
    Address vpage  = PAGE_ADDR_PAGE(va);
    Address offset = PAGE_ADDR_OFFSET(va);
    int cpunum = ast_ptr->global_var[avg_CPU];
    Atf_Tlb_Entry *tlbe_ptr;
    u64 asn = ast_ptr->cpu_ptr->local_var[avl_ASN];

    if (last_cpu == cpunum && last_vpage == vpage && last_asn == asn) {
	/* Cached ppage! */
        if (last_gh)
            offset = va & MASK(PAGE_SIZE_LOG2 + last_gh * 3);
	return (last_ppage | offset);
    }
    if (last_cpu2 == cpunum && last_vpage2 == vpage && last_asn2 == asn) {
	/* Cached ppage!  Remember last 2, since we often need both I & D translations */
        if (last_gh2)
            offset = va & MASK(PAGE_SIZE_LOG2 + last_gh2 * 3);
	return (last_ppage2 | offset);
    }

    last_cpu2 = last_cpu;
    last_vpage2 = last_vpage;
    last_ppage2 = last_ppage;
    last_gh2 = last_gh;
    last_asn2 = last_asn;
    last_cpu = cpunum;
    last_asn = asn;
    last_vpage = vpage;

    if (is_istream && IS_PAL(va)) {
	/* PAL code -- Clear PAL bit and force direct Imap */
	va = va & ~0x3;
	last_ppage = vpage;
        last_gh = 0;
        offset = PAGE_ADDR_OFFSET(va);
    }
    /* Superpage checks (see atf_tlb_fill also) */
    else if (IS_ADDR_DIRECT) {
        last_ppage = vpage;
        last_gh = 0;
        offset = PAGE_ADDR_OFFSET(va);
    }
    else if (IS_ADDR_SP_EV5(vpage)) {
	last_ppage = vpage & MASK(30);
        last_gh = 0;
        offset = PAGE_ADDR_OFFSET(va);
    }
    else if (IS_ADDR_SP_EV6(vpage)) {
	last_ppage = vpage & MASK(30);
        last_gh = 0;
        offset = PAGE_ADDR_OFFSET(va);
    }
    else if (IS_ADDR_KSEG_EV5(vpage)) {
	last_ppage = vpage & MASK(40);
        last_gh = 0;
        offset = PAGE_ADDR_OFFSET(va);
    }
    else if (IS_ADDR_KSEG_EV6(vpage)) {
	last_ppage = vpage & MASK(46);
	if (vpage & POW2(40))
            last_ppage |= ~MASK(40);    /* Sign extend */
        last_gh = 0;
        offset = PAGE_ADDR_OFFSET(va);
    }
    else if (IS_ADDR_ASM) {
	/* ASM set, direct map */
	last_ppage = vpage;
        last_gh = 0;
        offset = PAGE_ADDR_OFFSET(va);
    }
    else {
	/* PTE lookup/translate */
	tlbe_ptr = atf_tlb_find_valid (ast_ptr, vpage, asn);
	if (tlbe_ptr != NULL) {
	    /*DBG (ATF_DEBUG, 0, msg ("\tpte_hit NEW va %lx ppage %lx PTE %lx\n", va, tlbe_ptr->ppage, tlbe_ptr->pte));*/
            /* Take care of granularity hints !!!! (see atf_tlb_fill_entry() also) */
            last_ppage = tlbe_ptr->ppage;
            last_gh    = (tlbe_ptr->pte >> 5) & 0x3; /* Granularity Hint: xTB_PTE <6:5>   */
            offset     = va & MASK(PAGE_SIZE_LOG2 + last_gh * 3);

            if (last_gh) {
                DBG_MSG(msg("ATF: %016lx **************************************************\n", last_ppage | offset),
                        ast_ptr - &atf_stream[0]);
            }

	} else {
	    last_ppage = vpage;
            last_gh = 0;
            offset = PAGE_ADDR_OFFSET(va);
            fflush(stdout);
            DBG_MSG (msg("Error! atf_tlb_translate had %cTLB miss, Va %#016lx Pg %#016lx ASN %lx\n",
                         is_istream?'I':'D', va, vpage, asn),
                     ast_ptr - &atf_stream[0]);
	}
    }

    return (last_ppage | offset);
}

static void atf_tlb_fill_entry (Atf_Stream *ast_ptr, Address va, u64 pte, int ent)
    /* Fill specific TLB entry */
{
    Address vpage = PAGE_ADDR_PAGE(va);
    Atf_Tlb_Entry *tlbe_ptr;
    u64 asn = ast_ptr->cpu_ptr->local_var[avl_ASN];
    int cpunum = ast_ptr->global_var[avg_CPU];
    int gh;

    tlbe_ptr = &ast_ptr->cpu_ptr->tlb[ ent ];
    tlbe_ptr->vpage = vpage;
    tlbe_ptr->asn = asn;
    tlbe_ptr->pte = pte;
    /* BE WARNED: The current SRM PTE format only supports 44 bit physical addressing */
    /* The below line needs updating when the new 48+ bit format is designed */
    /* Take care of granularity hints in the translation!!!! (see atf_tlb_translate() also) */
    gh = (pte >> 5) & 0x3; /* Granularity Hint: xTB_PTE <6:5>   */
    if (gh == 0)
        tlbe_ptr->ppage = PAGE_ADDR_PAGE(pte >> (32L - PAGE_SIZE_LOG2));
    else
        tlbe_ptr->ppage = ((pte >> 32) & ~MASK(gh * 3)) << PAGE_SIZE_LOG2;
    /*DBG (ATF_DEBUG, 0, msg ("\taft_tlb_fill[%d]  NEW %lx  %lx  %lx\n", ent, va, tlbe_ptr->pte, tlbe_ptr->ppage));*/

    /* Update translation cache */
    if (last_cpu2==cpunum && last_vpage2==vpage && last_asn2==asn) {
        last_gh2 = gh;
        last_ppage2 = tlbe_ptr->ppage;
    }
    else if (last_cpu==cpunum && last_vpage==vpage && last_asn==asn) {
        last_gh = gh;
        last_ppage = tlbe_ptr->ppage;
    }
}

static int atf_tlb_fill (Atf_Stream *ast_ptr, Address va, u64 pte)
    /* Add TLB entry, return -1 if hit, new entry index in TLB if miss */
{
    Address vpage = PAGE_ADDR_PAGE(va);
    Atf_Tlb_Entry *tlbe_ptr;
    u64 asn = ast_ptr->cpu_ptr->local_var[avl_ASN];
    uint ent;

    /* Superpage checks (see atf_tlb_translate also) */
    if ( IS_ADDR_SUPPAG(vpage) ) {
	return (-1);
    }

    /* PTE lookup & check */
    tlbe_ptr = atf_tlb_find (ast_ptr, vpage, asn);
    if (tlbe_ptr != NULL && tlbe_ptr->pte == pte) {
	/* Found, still correct, NOP */
	/*DBG (ATF_DEBUG, 0, msg ("\taft_tlb_fill  HIT %lx  %lx\n", va, pte));*/
	return (-1);
    } else {
	/* Add */
	ent = (tlbe_ptr == NULL) ? atf_tlb_new (ast_ptr) : (tlbe_ptr - &ast_ptr->cpu_ptr->tlb[0]);
        /* ASSERT((ent < ATF_TLB_ASSOC), msg("va=%lx pte=%lx ent=%u max=%u\n", va, pte, ent, ATF_TLB_ASSOC)); */
	atf_tlb_fill_entry (ast_ptr, va, pte, ent);
	return (ent);
    }
}

Address ATF_tlb_translate (int stream_id, Address va, Boolean is_istream)
    /* User interface to TLB translate... Return physical address for given va of current asn */
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];
    return (atf_tlb_translate (ast_ptr, va, is_istream) &
	    (is_istream ? ~0x3 : ~0)	/* clear PAL bit if istream */
	);
}

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/* Instruction caching */

#define MEMORY_TABLE_SHIFT 13

static Atf_Instr_Entry *atf_cache_location (Address ppc)
    /* Return instr entry pointer for given physical pc address */
    /* Allocate storage as needed */
{
    ulong idx3, idx2, idx1, idx0;
    ulong offset;
    Address page, pageleft;
    Atf_Instr_Entry *****lev0_pptr;
    Atf_Instr_Entry ****lev1_pptr;
    Atf_Instr_Entry ***lev2_pptr;
    static Atf_Instr_Entry **lev3_pptr;		/* Cached so static */
    Atf_Instr_Entry *aie_ptr;
    static Address last_page = ~0;		/* ~0 is in I/O space so is safe */
    
    offset = (PAGE_ADDR_OFFSET(ppc)) >> 2;		/* Instruction in page */
    page = ppc >> PAGE_SIZE_LOG2;

    if (page == last_page) {
	/* lo_pptr was cached correctly */
    } else {
	/* sigh, find a pointer to the page worth of instructions */
	pageleft = page;
	idx3 = pageleft & MASK(MEMORY_TABLE_SHIFT);	/* 25..13   13 bits */
	pageleft = pageleft >> MEMORY_TABLE_SHIFT;
	idx2 = pageleft & MASK(MEMORY_TABLE_SHIFT);	/* 38..26   13 bits*/
	pageleft = pageleft >> MEMORY_TABLE_SHIFT;
	idx1 = pageleft & MASK(MEMORY_TABLE_SHIFT);	/* 51..39   13 bits*/
	pageleft = pageleft >> MEMORY_TABLE_SHIFT;
	idx0 = pageleft & MASK(MEMORY_TABLE_SHIFT);	/* 63..52   13 bits*/
	
	lev0_pptr = &(atf_instructions[idx0]);
	if (*lev0_pptr == NULL)  {
	    *lev0_pptr = (Atf_Instr_Entry ****) UTIL_calloc(POW2(MEMORY_TABLE_SHIFT), sizeof(Atf_Instr_Entry ***));
	    IF_ATF_PERFORMANCE(atf_stats.pages[0]++);
	}

	lev1_pptr = &((*lev0_pptr)[idx1]);
	if (*lev1_pptr == NULL)  {
	    *lev1_pptr = (Atf_Instr_Entry ***) UTIL_calloc(POW2(MEMORY_TABLE_SHIFT), sizeof(Atf_Instr_Entry **));
	    IF_ATF_PERFORMANCE(atf_stats.pages[1]++);
	}

	lev2_pptr = &((*lev1_pptr)[idx2]);
	if (*lev2_pptr == NULL)  {
	    *lev2_pptr = (Atf_Instr_Entry **) UTIL_calloc(POW2(MEMORY_TABLE_SHIFT), sizeof(Atf_Instr_Entry *));
	    IF_ATF_PERFORMANCE(atf_stats.pages[2]++);
	}

	lev3_pptr = &((*lev2_pptr)[idx3]);
	if (*lev3_pptr == NULL)  {
	    /* This last level doesn't need to be calloced since we don't use it as a pointer. (major performance win) */
	    /* Note this means we can't in the future check for all valid instructions.  Oh well. */
            *lev3_pptr = (Atf_Instr_Entry *) UTIL_calloc(POW2(PAGE_SIZE_LOG2), sizeof(Atf_Instr_Entry));
	    IF_ATF_PERFORMANCE(atf_stats.pages[3]++);
	}

	/* Don't do that mess next time */
	last_page = page;
    }

    aie_ptr = &((*lev3_pptr)[offset]);
    return (aie_ptr);
}

static Atf_Instr_Entry *atf_cache_opcode (
    Atf_Stream *ast_ptr, Opcode opcode, Address vea)
    /* Put instruction in the cache of opcodes. */
{
    Atf_Instr_Entry *aie_ptr;
    Address ppc = atf_tlb_translate (ast_ptr, ast_ptr->cpu_ptr->vpc, true/*istream*/);

    aie_ptr = atf_cache_location (ppc);
    aie_ptr->opcode_etc.opcode = opcode;
    aie_ptr->ea = vea;
    return (aie_ptr);
}

static void atf_cache_opcode_last (Atf_Stream *ast_ptr, Opcode opcode, Address vea,
				   Opcode *last_opcode_ptr, Address *last_vea_ptr)
    /* Put instruction in the cache of opcodes. */
    /* Return old value of va and opcode from previous instruction */
{
    Atf_Instr_Entry *aie_ptr;
    Address ppc = atf_tlb_translate (ast_ptr, ast_ptr->cpu_ptr->vpc, true/*istream*/);

    aie_ptr = atf_cache_location (ppc);

    *last_vea_ptr = aie_ptr->ea;
    *last_opcode_ptr = aie_ptr->opcode_etc.opcode;

    aie_ptr->opcode_etc.opcode = opcode;
    aie_ptr->ea = vea;
}

static Atf_Instr_Entry *atf_cache_lookup (
    Atf_Stream *ast_ptr, Address vea,
    Opcode *cur_opcode_ptr, Address *cur_vea_ptr,
    Boolean set_relative_vea,	/* add vea to previous vea */
    Boolean set_abs_vea
    )
    /* Look up the instruction in the cache of opcodes.
     * Put into structures.  Return true if was present, and last va and opcode
     * All readers must have a cache at least as large as the writer's cache;
     * thus this isn't infinite, but rather a fixed 1MB
     */
{
    Atf_Instr_Entry *aie_ptr;
    Address ppc = atf_tlb_translate (ast_ptr, ast_ptr->cpu_ptr->vpc, true/*istream*/);

    aie_ptr = atf_cache_location (ppc);

    if (set_abs_vea) aie_ptr->ea = vea;
    if (set_relative_vea) aie_ptr->ea = ((i64)vea) + ((i64)aie_ptr->ea);

    *cur_vea_ptr = aie_ptr->ea;
    *cur_opcode_ptr = aie_ptr->opcode_etc.opcode;
    return (aie_ptr);
}

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/* Variable reading / setting */

static u64 atf_set_variable (Atf_Stream *ast_ptr, AtfVariable_enum varnum, i64 value)
    /* Set the variable to a new value */
    /* Return the old value */
{
    u64 old_value;

    if (varnum<avl_MAX) {
	/* Is local */
	Atf_Cpu    *cpu_ptr = ast_ptr->cpu_ptr;
	old_value = cpu_ptr->local_var[varnum];
	cpu_ptr->local_var[varnum] = value;
    } else {
	/* Is global */
	old_value = ast_ptr->global_var[varnum];
	ast_ptr->global_var[varnum] = value;

	/* Snoop CPU change */
	if (varnum==avg_CPU) {
	    ast_ptr->cpu_ptr = &ast_ptr->cpu[ ast_ptr->global_var[avg_CPU] ];
	}
    }

    return (old_value);
}

static u64 atf_get_variable (Atf_Stream *ast_ptr, AtfVariable_enum varnum)
    /* Return value of variable */
{
    u64 old_value;
    if (varnum<avl_MAX) {
	/* Is local */
	Atf_Cpu    *cpu_ptr = ast_ptr->cpu_ptr;
	old_value = cpu_ptr->local_var[varnum];
    } else {
	/* Is global */
	old_value = ast_ptr->global_var[varnum];
    }
    return (old_value);
}

static void atf_set_pc (Atf_Stream *ast_ptr, i64 new_vpc, Boolean is_relative)
{
    if (is_relative) 
	ast_ptr->cpu_ptr->vpc += new_vpc;
    else ast_ptr->cpu_ptr->vpc = new_vpc;
}

u64 ATF_get_global (int stream_id, AtfVariable_enum varnum)
    /* User level return value of variable */
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];
    ASSERT (varnum>avl_MAX && varnum<avg_MAX, msg("Bad global variable number %d... Maybe local?", varnum));
    return (ast_ptr->global_var[varnum]);
}
u64 ATF_get_local (int stream_id, AtfVariable_enum varnum)
    /* User level return value of variable */
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];
    ASSERT (varnum>=0 && varnum<avl_MAX, msg("Bad local variable number... Maybe global?", varnum));
    return (ast_ptr->cpu_ptr->local_var[varnum]);
}


/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/* Internal file open/close */

static char *atf_filename_expand (const char *filename, 
				  Atf_Stream *ast_ptr)
    /* Given a filename, parse it */
    /* Trailing :# indicates trace should only accept that cpu #'s instructions */
    /* Remove any $ENV environment variables. */
    /* Return string, NULL if unsuccessful */
{
    char *p;
    char *outstr;
    char *dollar, *end, *env;

    /* Copy */
    outstr = strdup (filename);

    /* replace environment */
    dollar = strchr (outstr, '$');
    if (dollar!=NULL) {
	end = strpbrk (dollar, " \t\n/");
	if (end==NULL) end = &outstr[strlen(outstr)];	/* The null */
	*end = '\0';

	if (NULL == (env = getenv (dollar+1))) {
	    fprintf(stderr,"Error!  In ATF_open, can't parse enviornment var '%s'\n", dollar);
	    return (NULL);	/* Failure */
	}
	
	*dollar = '\0';
	/* Replace dollar sign with new env */
	strcat (outstr, env);
	/* Stuff after dollar (from original, outstr now trashed) */
	strcat (outstr, (end-outstr) + filename);
    }

    /* Trailing :# */
    if ((NULL != (p = strrchr (outstr, ':')))
	&& isdigit (p[1])) {
	/* Pull out CPU number */
	ast_ptr->limit_cpu_ptr = &ast_ptr->cpu[ strtol(p+1, NULL, 0) ];
	*p = '\0';
    } else {
	ast_ptr->limit_cpu_ptr = NULL;
    }

    return (outstr);
}

static char *atf_filename_next (const char *filename)
    /* Given a filename, return the next in sequence order */
    /* Return NULL if unsuccessful */
{
    char *p;
    char *outstr;

    /* Copy */
    outstr = strdup (filename);

    /* Trailing . */
    if (NULL != (p = strrchr (outstr, '.'))) {
	/* Then previous _ just before the dot */
	if (NULL != (p = strrchr (outstr, '_'))) {
	    /* Increment next two letters presuming they exist */
	    if (p[1] && p[2]) {
		if (p[2]=='z' || p[2]=='Z') {
		    p[1]++;
		    p[2] += ('A' - 'Z');  /* Z->A*/
		}
		else p[2]++;
		return (outstr);	/* Success */
	    }
	}
    }
    return (NULL);	/* Failure */
}

static void atf_file_close_single (Atf_Stream *ast_ptr) 
    /* Close a single trace file (out of many making the trace). */
{
    if (ast_ptr->fp) {
#if ATF_PERFORMANCE
        if (!ast_ptr->piping) {
            if ((atf_stats.file_size = (Counter)ftell(ast_ptr->fp)) == (Counter)-1) {
                perror("atf_file_close_single::");
                atf_stats.file_size = (Counter) 0;
            }
        }
        else
            atf_stats.file_size = (Counter) 0;
#endif
	fflush (ast_ptr->fp);
	if (ast_ptr->piping) {
	    pclose (ast_ptr->fp);
	}
	else {
	    fclose (ast_ptr->fp);
	}
	ast_ptr->fp = NULL;
    }
}

static Boolean atf_file_open_single (Atf_Stream *ast_ptr) 
    /* Open a single trace file (out of many making the trace). */
    /* Return success/fail */
{
    uint magic;
    char *p;

    if (NULL == ast_ptr->filename) return (false);

    /* Run it through gzip? */
    ast_ptr->piping = ((NULL != (p=strrchr(ast_ptr->filename,'.')))
		       && (!strcmp (p, ".gz")));

    /* Attempt to open file */
    if (ast_ptr->is_writing) {
	if (ast_ptr->piping) {
	    char *pipecmd = (char *) UTIL_malloc (40+strlen (ast_ptr->filename));
	    sprintf (pipecmd, "gzip >%s", ast_ptr->filename);
	    DBG (ATF_DEBUG, 0, msg("Opening output thru pipe '%s'\n", pipecmd));
	    ast_ptr->fp = popen (pipecmd, "w");
	    free (pipecmd);
	} else {
	    ast_ptr->fp = fopen (ast_ptr->filename, "wb");
	}
	if (ast_ptr->fp == NULL) {
            perror("atf_file_open_single::");
            return (false);
        }

	magic = ATF_MAGIC_NUMBER;
	fwrite (&magic, 4, 1, ast_ptr->fp);
    }
    else { /* read */
	/* Check existance before piping */
	ast_ptr->fp = fopen (ast_ptr->filename, "rb");
	if (ast_ptr->fp == NULL) {
            perror("atf_file_open_single::");
            return (false);
        }

	if (ast_ptr->piping) {
	    char *pipecmd = (char *) UTIL_malloc (40+strlen (ast_ptr->filename));
	    sprintf (pipecmd, "gunzip -c %s", ast_ptr->filename);
	    DBG (ATF_DEBUG, 0, msg("Opening input thru pipe '%s'\n", pipecmd));
	    fclose (ast_ptr->fp);
	    ast_ptr->fp = popen (pipecmd, "r");
	    free (pipecmd);
	}
	if (ast_ptr->fp == NULL) {
            perror("atf_file_open_single::");
            return (false);
        }

	/* Check magic number */
	fread (&magic, 4, 1, ast_ptr->fp);
	if (magic != ATF_MAGIC_NUMBER) {
	    fprintf (stderr,"Error! ATF file contains bad magic number\n");
	    fclose (ast_ptr->fp);
	    return (false);
	}
    }

    return (ast_ptr->fp != NULL);
}

static Boolean atf_file_next_single (Atf_Stream *ast_ptr)
    /* Close existing file and attempt to open next */
{
    char *oldfn;

    atf_file_close_single (ast_ptr);

    /* Update filename */
    oldfn = ast_ptr->filename;
    ast_ptr->filename = atf_filename_next (oldfn);
    if (oldfn) free (oldfn);

    if (!ast_ptr->filename) return (false);	/* Couldn't increment */

    return (atf_file_open_single (ast_ptr));
}

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/* User file open/close */

void ATF_file_close (int stream_id) 
    /* Close the trace file. */
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];

    atf_file_close_single (ast_ptr);

    ATF_stats();

    /* Deallocate any storage */
    if (ast_ptr->filename) free (ast_ptr->filename);
}

Boolean ATF_file_open (int stream_id, const char *filename, Boolean do_write)
    /* Open trace file for reading or writing */
    /* Returns:	     	true if the file was opened, false otherwise */
    /* Filenames may contain:	*/
    /*		$ENVVARS which are expanded to environment contents. */
    /*		.gz to pipe input/output through gzip. */
    /*		trailing :# to indicate filtering to a specific CPU. */
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];
    Boolean ok;

    ASSERT (stream_id>=0 && stream_id<ATF_STREAMS_MAX, "Illegal stream id.");

    /* Initialize structures if needed */
    ast_ptr->filename = atf_filename_expand (filename, ast_ptr);
    ast_ptr->is_writing = do_write;

    /* Open first file in the trace */
    ok = atf_file_open_single (ast_ptr);
    if (!ok) return (false);

    if (ast_ptr->is_writing) {
	char cmt[1000];
	/* Dump header comment */
	ATF_write_global  (0, avg_VERSION, 3);
	ATF_write_comment (0, "#Version: $Header$");
	sprintf (cmt, "#Created: %s", UTIL_date_string (0));
	ATF_write_comment (0, cmt);
    }

    return (true);
}

char *ATF_get_filename (int stream_id)
    /* Return the filename of the currently open trace file */
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];
    return (ast_ptr->filename);
}

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/* Writing */

/* Create 5-bit delta value */
#define DELTAT_CODEA(delta) ((((delta) < 0) ? (((-(delta+1)) & 0x1f) | 0x20) : ((delta-1) & 0x1f)))

/* Write elements to file */
#define EMIT_BYTE(ast_ptr, b)  {fputc ((b), ast_ptr->fp);}
#if ATF_PERFORMANCE
# define EMIT_TOKEN_BYTE(ast_ptr,b)  {EMIT_BYTE(ast_ptr,(b)); atf_stats.token_count_table[(b) & 0xff]++; }
#else
# define EMIT_TOKEN_BYTE(ast_ptr,b)  {EMIT_BYTE(ast_ptr,(b));}
#endif
#if ATF_PERFORMANCE
# define EMIT_COMMAND_EXT_BYTE(ast_ptr,b)  {EMIT_BYTE(ast_ptr,(b)); atf_stats.command_ext_count_table[(b) & 0xff]++; }
#else
# define EMIT_COMMAND_EXT_BYTE(ast_ptr,b)  {EMIT_BYTE(ast_ptr,(b));}
#endif
#define EMIT_SKIP(ast_ptr)  {if (ast_ptr->instr_left) { \
                          EMIT_TOKEN_BYTE (ast_ptr, aft_EXEC_SEQ | ((ast_ptr->instr_left - 1) & 0x1f)); \
			  ast_ptr->instr_left = 0; }}

#define EMIT_TOKEN(ast_ptr,b)  {EMIT_SKIP(ast_ptr);  EMIT_TOKEN_BYTE(ast_ptr,(b));}

static void EMIT_DELTA3 (Atf_Stream *ast_ptr, i32 datum)
{
    fwrite (((char*)&datum), 3, 1, ast_ptr->fp);
    /*DBG (ATF_DEBUG, 0, msg("\t\tWrote D3   %06x\n", datum));*/
}
static void EMIT_LW (Atf_Stream *ast_ptr, u32 datum)
{
    fwrite (&datum, 4, 1, ast_ptr->fp);
    /*DBG (ATF_DEBUG, 0, msg("\t\tWrote LW   %08x\n", datum));*/
}
static void EMIT_QW (Atf_Stream *ast_ptr, u64 datum)
{
    fwrite (&datum, 8, 1, ast_ptr->fp);
    /*DBG (ATF_DEBUG, 0, msg("\t\tWrote QW   %016lx\n", datum));*/
}


#define EMIT_STRING(ast_ptr,str, len)  {fwrite ((str), len, 1, ast_ptr->fp);}
#define EMIT_DELTA1(ast_ptr,delta)  {EMIT_BYTE(ast_ptr, ((signed char)(delta)));}

static void atf_write_variable (Atf_Stream *ast_ptr, AtfVariable_enum varnum, u64 value, u64 old_value)
    /* Write a set to the given variable */
{
    i64 delta;

    /* Determine difference and set */
    delta = value - old_value;
    if (delta==0) return;

    if (varnum == avg_CPU && value < 0xf) {
	/* CPU number: Use special compressed format */
	EMIT_TOKEN (ast_ptr, aft_SET_CPU | (value & 0xf));
    }
    else if (varnum == avg_EPOCH && delta==1) {
	/* Epoch increment: Special compressed format */
	EMIT_TOKEN (ast_ptr, aft_INC_EPOCH);
    }
    else {
	/* Generic set */
	if (delta > -0x7f && delta < 0x7f) {
	    EMIT_TOKEN (ast_ptr, aft_SET_REL);
	    EMIT_BYTE (ast_ptr, varnum);
	    EMIT_DELTA1 (ast_ptr, delta);
	} else {
	    EMIT_TOKEN (ast_ptr, aft_SET_EXACT);
	    EMIT_BYTE (ast_ptr, varnum);
	    EMIT_QW (ast_ptr, value);
	}
    }
}

void ATF_write_global (int stream_id, AtfVariable_enum varnum, u64 value)
    /* Write a set to the given global variable */
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];
    u64 old_value;

    ASSERT (varnum>avl_MAX && varnum<avg_MAX, msg("Bad global variable number %d... Maybe local?", varnum));

    /* Determine difference and set */
    old_value = atf_set_variable (ast_ptr, varnum, value);
    atf_write_variable (ast_ptr, varnum, value, old_value);
}

void ATF_write_local (int stream_id, AtfVariable_enum varnum, u64 value)
    /* Write a set to the given local variable */
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];
    u64 old_value;

    ASSERT (varnum>=0 && varnum<avl_MAX, msg("Bad local variable number... Maybe global?", varnum));

    /* Determine difference and set */
    old_value = atf_set_variable (ast_ptr, varnum, value);
    atf_write_variable (ast_ptr, varnum, value, old_value);
}

void ATF_write_pte (int stream_id, Address va, u64 pte)
    /* Write the given PTE entry */
    /* Note the PTE is presumed to be VMS format... OSF is subset of VMS; NT requires conversion! */
    /* BE WARNED: The current SRM PTE format only supports 44 bit physical addressing */
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];
    Address vpage = PAGE_ADDR_PAGE(va);	/* Page address only... Lower bits have ent number */
    int ent;

    if ((ent = atf_tlb_fill (ast_ptr, vpage, pte)) != -1) {
	/* Wasn't in cache, put into file */
	/*DBG (ATF_DEBUG,0,msg("aft_PTE VPC %lx ASN %lx PTE %lx\n", vpage, ast_ptr->cpu_ptr->local_var[avl_ASN], pte));*/
	EMIT_TOKEN (ast_ptr, aft_PTE);
	EMIT_QW (ast_ptr, vpage | ent);	/* Include entry number */
	EMIT_QW (ast_ptr, pte);
    }
}

void ATF_write_comment (int stream_id, const char *comment)
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];
    int len = (int)strlen (comment);

    EMIT_TOKEN (ast_ptr, aft_COMMAND);
    EMIT_COMMAND_EXT_BYTE (ast_ptr, acm_COMMENT);
    EMIT_LW (ast_ptr, len);
    EMIT_STRING (ast_ptr, comment, len);
#if ATF_PERFORMANCE
    atf_stats.comments += len;
#endif
}

void ATF_write_comment_rout (int stream_id, Address ppc_start, Address ppc_end, const char *comment)
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];
    int len = (int)strlen (comment);

    EMIT_TOKEN (ast_ptr, aft_COMMAND);
    EMIT_COMMAND_EXT_BYTE (ast_ptr, acm_COMMENT_ROUT);
    EMIT_QW (ast_ptr, ppc_start);
    EMIT_QW (ast_ptr, ppc_end);
    EMIT_LW (ast_ptr, len);
    EMIT_STRING (ast_ptr, comment, len);
#if ATF_PERFORMANCE
    atf_stats.comments += len;
#endif
}

void ATF_write_interrupt (int stream_id)
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];
    EMIT_TOKEN (ast_ptr, aft_COMMAND);
    EMIT_COMMAND_EXT_BYTE (ast_ptr, acm_INTERRUPT);
}

void ATF_write_resync (int stream_id)
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];
    EMIT_TOKEN (ast_ptr, aft_COMMAND);
    EMIT_COMMAND_EXT_BYTE (ast_ptr, acm_RESYNC);
}

void ATF_write_branch_action (int stream_id, unsigned char action)
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];
    EMIT_TOKEN (ast_ptr, aft_COMMAND);
    EMIT_COMMAND_EXT_BYTE (ast_ptr, acm_BR_ACTION);
    EMIT_BYTE (ast_ptr, action);
}

/**********************************************************************/
/* informationals regarding process/thread vm space breakdown */

void ATF_write_proc_text_info (int stream_id, u64 pid, Address text_start, Address text_end, const char *comment)
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];
    int len = (int)strlen (comment);

    EMIT_TOKEN (ast_ptr, aft_COMMAND);
    EMIT_COMMAND_EXT_BYTE (ast_ptr, acm_PROC_TEXT_INFO);
    EMIT_QW (ast_ptr, pid);
    EMIT_QW (ast_ptr, text_start);
    EMIT_QW (ast_ptr, text_end);
    EMIT_LW (ast_ptr, len);
    EMIT_STRING (ast_ptr, comment, len);
#if ATF_PERFORMANCE
    atf_stats.comments += len;
#endif
}

void ATF_write_proc_bss_info (int stream_id, u64 pid, Address bss_start, Address bss_end)
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];

    EMIT_TOKEN (ast_ptr, aft_COMMAND);
    EMIT_COMMAND_EXT_BYTE (ast_ptr, acm_PROC_BSS_INFO);
    EMIT_QW (ast_ptr, pid);
    EMIT_QW (ast_ptr, bss_start);
    EMIT_QW (ast_ptr, bss_end);
}

void ATF_write_thread_ustack_addr_info (int stream_id, u64 pid, u64 tid, Address ustack_start, Address ustack_end)
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];

    EMIT_TOKEN (ast_ptr, aft_COMMAND);
    EMIT_COMMAND_EXT_BYTE (ast_ptr, acm_THREAD_USTACK_ADDR_INFO);
    EMIT_QW (ast_ptr, pid);
    EMIT_QW (ast_ptr, tid);
    EMIT_QW (ast_ptr, ustack_start);
    EMIT_QW (ast_ptr, ustack_end);
}

void ATF_write_thread_ustack_grow_info (int stream_id, u64 pid, u64 tid, u64 ustack_curr_size, Boolean ustack_grow_up)
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];

    EMIT_TOKEN (ast_ptr, aft_COMMAND);
    EMIT_COMMAND_EXT_BYTE (ast_ptr, acm_THREAD_USTACK_GROW_INFO);
    EMIT_QW (ast_ptr, pid);
    EMIT_QW (ast_ptr, tid);
    EMIT_QW (ast_ptr, ustack_curr_size);
    EMIT_BYTE (ast_ptr, ustack_grow_up);
}

void ATF_write_thread_kstack_info (int stream_id, u64 pid, u64 tid, Address kstack_start, Address kstack_end)
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];

    EMIT_TOKEN (ast_ptr, aft_COMMAND);
    EMIT_COMMAND_EXT_BYTE (ast_ptr, acm_THREAD_KSTACK_INFO);
    EMIT_QW (ast_ptr, pid);
    EMIT_QW (ast_ptr, tid);
    EMIT_QW (ast_ptr, kstack_start);
    EMIT_QW (ast_ptr, kstack_end);
}

void ATF_write_libloader_info (int stream_id, Address text_start, Address text_end, Address bss_start, Address bss_end)
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];

    EMIT_TOKEN (ast_ptr, aft_COMMAND);
    EMIT_COMMAND_EXT_BYTE (ast_ptr, acm_LIBLOADER_INFO);
    EMIT_QW (ast_ptr, text_start);
    EMIT_QW (ast_ptr, text_end);
    EMIT_QW (ast_ptr, bss_start);
    EMIT_QW (ast_ptr, bss_end);
}

void ATF_write_lib_info (int stream_id, Address text_start, Address text_end, Address bss_start, Address bss_end)
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];

    EMIT_TOKEN (ast_ptr, aft_COMMAND);
    EMIT_COMMAND_EXT_BYTE (ast_ptr, acm_LIB_INFO);
    EMIT_QW (ast_ptr, text_start);
    EMIT_QW (ast_ptr, text_end);
    EMIT_QW (ast_ptr, bss_start);
    EMIT_QW (ast_ptr, bss_end);
}

void ATF_write_kernel_switch (int stream_id, Boolean in, Address addr, const char *comment)
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];
    int len = (int)strlen (comment);

    EMIT_TOKEN (ast_ptr, aft_COMMAND);
    EMIT_COMMAND_EXT_BYTE (ast_ptr, acm_KERNEL_SWITCH);
    EMIT_BYTE (ast_ptr, in);
    EMIT_QW (ast_ptr, addr);
    EMIT_LW (ast_ptr, len);
    EMIT_STRING (ast_ptr, comment, len);
#if ATF_PERFORMANCE
    atf_stats.comments += len;
#endif
}

void ATF_write_idle_thread (int stream_id, Boolean idle_start)
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];

    EMIT_TOKEN (ast_ptr, aft_COMMAND);
    EMIT_COMMAND_EXT_BYTE (ast_ptr, acm_IDLE_THREAD);
    EMIT_BYTE (ast_ptr, idle_start);
}

/**********************************************************************/
/* DMA/Cache dumping */

void ATF_write_dma (int stream_id, Address pa, uint length, Boolean isDMAwrite)
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];
    EMIT_TOKEN (ast_ptr, aft_DMA);
    EMIT_QW (ast_ptr, pa);
    EMIT_LW (ast_ptr, length);
    EMIT_BYTE (ast_ptr, isDMAwrite);    /* DMA action */
}

void ATF_write_dump_start (int stream_id)
    /* Write start of cache dump */
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];
    ast_ptr->is_dumping = true;
    EMIT_TOKEN (ast_ptr, aft_COMMAND);
    EMIT_COMMAND_EXT_BYTE (ast_ptr, acm_DUMP);
}

void ATF_write_dump (int stream_id, Address pa, Address va, Boolean is_dstream, AtfDumpState_enum state)
    /* Write cache dump entry.  Pass 0 for an address that doesn't matter */
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];
    u32	pa_lower = (u32)(pa & ~0x3f);	/* Lower LW of pa, limited to cache line */
    u32	pa_upper = (u32)(pa>>32);

    /* Add I/D and OSDE bits to within-cache line bits of pa */
    pa_lower |= (is_dstream?0x1:0) | ( (state & 0x3) << 3);

    /* If bits2:1=00 are ever used, add check for a all zero LW; else it will falsely indicate end of dump */
    if (va==0) {
	if (pa_upper == 0) {
	    EMIT_LW (ast_ptr, pa_lower | 0x2);
	} else {
	    EMIT_LW (ast_ptr, pa_lower | 0x4);
	    EMIT_LW (ast_ptr, pa_upper);
	}
    } else {
	EMIT_LW (ast_ptr, pa_lower | 0x6);
	EMIT_LW (ast_ptr, pa_upper);
	EMIT_QW (ast_ptr, (va & ~0x3f));	/* va, limited to cache line */
    }
}

void ATF_write_dump_end (int stream_id)
    /* Write end of cache dump */
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];
    ast_ptr->is_dumping = false;
    EMIT_LW (ast_ptr, 0);
}

/**********************************************************************/
/* Instructions */

#if ATF_CREATE_DUMMY_PTE
#define CREATE_DUMMY_PTE_OR_RET(stream_id, va, is_istream)                                                         \
{                                                                                                                  \
    u64 pte =   ((PAGE_ADDR_PAGE(va)) << (32-PAGE_SIZE_LOG2))  /* PFN */                                           \
              | 0x00000000FFFF000E                             /* FOE/FOW/FOR set, xRE/xWE clear, SW bits set */   \
              | 0x0000000000000001;                            /* make it valid */                                 \
    ATF_write_pte(stream_id, va, pte);                                                                             \
}
#else
#define CREATE_DUMMY_PTE_OR_RET(stream_id, va, itlb_missed)  \
{                                                            \
    if     (itlb_missed)   return -1;                        \
    else /* dtlb_missed */ return -2;                        \
}
#endif /* ATF_CREATE_DUMMY_PTE */

/*
 * ATF_write_instruction() abstract:
 *
 *   Purpose:
 *         dump instructions to trace file
 *
 *   Return values:
 *         If successful return 1
 *
 *         Negative return values mean that tlb_lookup failed for vpc or vea.
 *         if lookup failed for vpc, then return value is -1
 *         if lookup failed for vea, then return value is -2
 *
 *         In this case the external writer (virtual machine) must generate
 *         a valid PTE entry for the missing page, or ATF should be compiled
 *         with ATF_CREATE_DUMMY_PTE=1 to create dummy PTE's (ppage=vpage).
 *
 *         ****** WARNING ******* :
 *         When the return value is not 1, the external writer MUST call 
 *         ATF_write_instruction() again for the SAME instruction.
 *         Negative return values mean that ATF failed to write the instruction
 *         in the trace file!!!
 *
 *         Note that the external writer (virtual machine) should check for
 *         loops while @ debugging stage: ATF misses in tlb, virtual machine
 *         fails to produce a valid PTE and attempts to call ATF_write_instruction()
 *         for the same instruction, the call fails again, and so forth.
 */
int ATF_write_instruction (int stream_id, Address vpc, Opcode opcode, Address vea)
    /* Write the given instruction to the file. */
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];
    Atf_Cpu    *cpu_ptr = ast_ptr->cpu_ptr;
    Address	last_vea;
    Opcode	last_opcode;
    i64 	delta, lwdelta;
    static i64  last_ea_delta = ~0;

    /* Check that VPC->PPC translation exists */
    if (   !(IS_PAL(vpc))
        && !(IS_ADDR_SUPPAG(vpc))
        && (NULL == atf_tlb_find_valid (ast_ptr, vpc, ast_ptr->cpu_ptr->local_var[avl_ASN])))
    {
	    DBG_MSG (msg("Error! ATF %d: Couldn't find VPC %016lx ASN %lx in ITLB (modes:%s%s%s%s%s%s%s)\n",
                         stream_id,
                         vpc,
                         ast_ptr->cpu_ptr->local_var[avl_ASN],
                         (IS_PAL(vpc)) ? " PAL" : "",
                         (ast_ptr->cpu_ptr->local_var[avl_SUPPAG] & asp_DIRECT) ? " DIRECT" : "",
                         (ast_ptr->cpu_ptr->local_var[avl_SUPPAG] & asp_SP_EV5) ? " SP_EV5" : "",
                         (ast_ptr->cpu_ptr->local_var[avl_SUPPAG] & asp_SP_EV6) ? " SP_EV6" : "",
                         (ast_ptr->cpu_ptr->local_var[avl_SUPPAG] & asp_KSEG_EV5) ? " KSEG_EV5" : "",
                         (ast_ptr->cpu_ptr->local_var[avl_SUPPAG] & asp_KSEG_EV6) ? " KSEG_EV6" : "",
                         (ast_ptr->cpu_ptr->local_var[avl_ASM]) ? " ASM" : ""),
                     stream_id);
            CREATE_DUMMY_PTE_OR_RET(stream_id, vpc, 1 /* itlb_missed */);
    }

    /* Check that VEA->PEA translation exists */
    if (   IsLoadStore (opcode)
        && !(IS_ADDR_SUPPAG(vea))
        && (NULL == atf_tlb_find_valid (ast_ptr, vea, ast_ptr->cpu_ptr->local_var[avl_ASN])))
    {
	    DBG_MSG (msg("Error! ATF %d: Couldn't find VEA %016lx ASN %lx in DTLB (modes:%s%s%s%s%s%s)\n",
                         stream_id,
                         vea, 
                         ast_ptr->cpu_ptr->local_var[avl_ASN],
                         (ast_ptr->cpu_ptr->local_var[avl_SUPPAG] & asp_DIRECT) ? " DIRECT" : "",
                         (ast_ptr->cpu_ptr->local_var[avl_SUPPAG] & asp_SP_EV5) ? " SP_EV5" : "",
                         (ast_ptr->cpu_ptr->local_var[avl_SUPPAG] & asp_SP_EV6) ? " SP_EV6" : "",
                         (ast_ptr->cpu_ptr->local_var[avl_SUPPAG] & asp_KSEG_EV5) ? " KSEG_EV5" : "",
                         (ast_ptr->cpu_ptr->local_var[avl_SUPPAG] & asp_KSEG_EV6) ? " KSEG_EV6" : "",
                         (ast_ptr->cpu_ptr->local_var[avl_ASM]) ? " ASM" : ""),
                     stream_id);
            CREATE_DUMMY_PTE_OR_RET(stream_id, vea, 0 /* dtlb_missed */);
    }

    /* Create the new VPC for the instruction */
    delta = (long)(vpc) - (long)(cpu_ptr->vpc);
    atf_set_pc (ast_ptr, vpc, false);
    if (delta == 0) {
	/* NOP */
    } else if ((delta > -0x80 && delta < 0x80) && ((delta & 3)==0)) {
	/* Note only usable if moving power of 4 distance (increment of 1 instruction) */
	EMIT_TOKEN (ast_ptr, aft_JMP_SHORT | DELTAT_CODEA((delta >> 2)));
    } else if (delta > -((1L<<23) - 1) && delta < ((1L<<23) - 1)) {
	EMIT_TOKEN (ast_ptr, aft_JMP_REL);
	EMIT_DELTA3 (ast_ptr, delta);
    } else {
	EMIT_TOKEN (ast_ptr, aft_JMP_EXACT);
	EMIT_QW (ast_ptr, vpc);
    }

    /* Dump instruction opcode if not already correctly cached */
    atf_cache_opcode_last (ast_ptr, opcode, vea, &last_opcode, &last_vea);
    delta = (long)(vea) - (long)(last_vea);
    lwdelta = (long)(vea>>2) - (long)(last_vea>>2);
    if (last_opcode != opcode) {
	if (IsLoadStore (opcode)) {
	    if ( (vea & UINT64_CONST(0xffffffff00000000) ) != 0) {
		EMIT_TOKEN (ast_ptr, aft_EXEC_OP_EA);
		EMIT_LW (ast_ptr, opcode);
		EMIT_QW (ast_ptr, vea);
	    } else {
		EMIT_TOKEN (ast_ptr, aft_EXEC_OP_SEA);
		EMIT_LW (ast_ptr, opcode);
		EMIT_LW (ast_ptr, vea);
	    }
	} else {
	    EMIT_TOKEN (ast_ptr, aft_EXEC_OP);
	    EMIT_LW (ast_ptr, opcode);
	}
    }
    else if (IsLoadStore (opcode) && (delta != 0)) {
	/* Have right opcode, but effective address has changed */
	if (((delta & 0x3) == 0) && (lwdelta > -0x20) && (lwdelta < 0x20)) {
	    EMIT_TOKEN (ast_ptr, aft_EXEC_EA_SHORT | DELTAT_CODEA(lwdelta));
	    last_ea_delta = delta;
	} else if (delta == last_ea_delta) {
	    EMIT_TOKEN (ast_ptr, aft_EXEC_EA_LAST);
	} else if (delta > -((1L<<23) - 1) && delta < ((1L<<23) - 1)) {
	    EMIT_TOKEN (ast_ptr, aft_EXEC_EA_REL);
	    EMIT_DELTA3 (ast_ptr, delta);
	    last_ea_delta = delta;
	} else {
	    EMIT_TOKEN (ast_ptr, aft_EXEC_EA_EXACT);
	    EMIT_QW (ast_ptr, vea);
	}
    }
    else {
	/* Same instruction, same effective address, just do it as a skip */
	ast_ptr->instr_left ++;
	if (ast_ptr->instr_left > 0x1f) {
	    EMIT_SKIP (ast_ptr);
	}
    }

    /* Always move on to next instruction */
    atf_set_pc (ast_ptr, 0x4, true/*relative*/);

    return 1;
}

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/* Reading */

/* Decode 5-bit delta value */
#define VALUE_DELTAT(value) (((value) & 0x20) ? (-((value) & 0x1f)-1) : (((value) & 0x1f)+1))
/* Create 1-byte delta value */
#define VALUE_DELTA1(value) ((signed char)(value))

#define get_token(ast_ptr) (get_byte(ast_ptr))
static u8 get_byte (Atf_Stream *ast_ptr)
{
    unsigned char datum;
    datum = fgetc(ast_ptr->fp) & 0xff;
    DBG (ATF_DEBUG, 0, msg("\t\tGot Byte %02x\n", datum));
    return (datum);
}

static int get_delta3 (Atf_Stream *ast_ptr)
{
    int datum;
    datum = (fgetc(ast_ptr->fp)) | (fgetc(ast_ptr->fp)<<8) | (fgetc(ast_ptr->fp)<<16);
    if (datum & 0x800000) datum = datum | 0xff000000;	/* Sign extend */
    DBG (ATF_DEBUG, 0, msg("\t\tGot Delta3  %06x\n", datum));
    return (datum);
}

static u32 get_lw (Atf_Stream *ast_ptr)
{
    u32 datum;
    datum = (fgetc(ast_ptr->fp)) | (fgetc(ast_ptr->fp)<<8) | (fgetc(ast_ptr->fp)<<16) | (fgetc(ast_ptr->fp)<<24);
    DBG (ATF_DEBUG, 0, msg("\t\tGot LW   %08x\n", datum));
    return (datum);
}

static u64 get_qw (Atf_Stream *ast_ptr)
{
    u64 datum;
    u32 datuml,datumh;
    /* Don't optimize more or sign extension can get problematic */
    datuml = (fgetc(ast_ptr->fp)) | (fgetc(ast_ptr->fp)<<8) | (fgetc(ast_ptr->fp)<<16) | (fgetc(ast_ptr->fp)<<24);
    datumh = (fgetc(ast_ptr->fp)) | (fgetc(ast_ptr->fp)<<8) | (fgetc(ast_ptr->fp)<<16) | (fgetc(ast_ptr->fp)<<24);
    datum = datuml | (((u64)datumh)<<32L);
    DBG (ATF_DEBUG, 0, msg("\t\tGot QW   %016lx\n", datum));
    return (datum);
}

static char *atf_get_string (Atf_Stream *ast_ptr)
    /* Get a string from the file and return pointer */
{
    static int string_size = 0;
    static char *string = NULL;
    int size;

    /* Get the size */
    size = get_lw (ast_ptr);

    /* Get storage to fit the string */
    /* This is inefficient if we do it a lot... Presumably there aren't many strings */
    if (string == NULL) {
	string_size = size + 2;
	string = (char *) UTIL_malloc (string_size);
    } else if (string_size < size + 2) {
	string_size = size + 2;
	string = (char *) UTIL_realloc (string, string_size);
    }

    /* Read string */
    fread (string, size, 1, ast_ptr->fp);
    /* Null */
    string[size] = '\0';
#if ATF_PERFORMANCE
    atf_stats.comments += size;
#endif
    return (string);
}

/* Return the action if we are on the correct CPU number */
#define RETURN_OR_CONTINUE(action) if (ast_ptr->limit_cpu_ptr == NULL || ast_ptr->limit_cpu_ptr == cpu_ptr) return (action); else continue;

AtfAction_enum ATF_read_action (int stream_id,
				ulong *value1_ptr, ulong *value2_ptr, ulong *value3_ptr, ulong *value4_ptr)
    /* Read the next token(s) from the file, and return what it did */
{
    Atf_Stream *ast_ptr = &atf_stream[stream_id];
    Atf_Cpu    *cpu_ptr = ast_ptr->cpu_ptr;
    unsigned char token;
    int 	command;
    char	*string;
    char	byte;
    i64		delta;
    static i64	last_ea_delta;
    Opcode	opcode;
    Address	last_vea;
    Atf_Instr_Entry *aie_ptr;

    /* Keep looping till we have a new action to return */
    while (1) {
	/* CPU pointer may have changed from a SET command */
	cpu_ptr = ast_ptr->cpu_ptr;

	if (ast_ptr->instr_left) {
	    DBG (ATF_DEBUG, 0, msg("\tInstrs left to skip %d\n", ast_ptr->instr_left));
	    ast_ptr->instr_left --;
	    aie_ptr=atf_cache_lookup (ast_ptr, 0L, &opcode, &last_vea, false/*rel*/, false/*abs*/);
	    /* Common entry point for returning pre-existing instruction information */
	    /* Sorry this is a goto, we need common code to do a return or continue, so this is the fastest way */
	return_instr_old:
	    *value1_ptr = cpu_ptr->vpc;
	    *value2_ptr = (ulong)&(aie_ptr->opcode_etc);
	    *value3_ptr = last_vea;
	    atf_set_pc (ast_ptr, 0x4, true/*relative*/);
	    RETURN_OR_CONTINUE (act_INSTR_OLD);
	}
	
	if (ast_ptr->is_dumping) {
	    Address pa, va=0;
	    pa = get_lw (ast_ptr);
	    if (pa != 0) {
		if (pa & 0x4)  pa |= (((Address)get_lw (ast_ptr))<<32);
		if ((pa & 0x6) == 0x6)  va = get_qw (ast_ptr);
		*value1_ptr = pa & ~0x3f;
		*value2_ptr = va;
		*value3_ptr = pa & 0x1;
		*value4_ptr = (pa >> 3) & 0x3;
		return (act_DUMP);
	    } else {
		/* End dump */
		ast_ptr->is_dumping = false;
		DBG (ATF_DEBUG, 0, msg ("\tacm_DUMP_END\n"));
		continue;
	    }
	}

	token = get_token(ast_ptr);
	if (feof (ast_ptr->fp)) return (act_EOF);

#if ATF_PERFORMANCE
	atf_stats.token_count_table[token & 0xff]++;
#endif
	
	if ((token & 0xe0) == aft_EXEC_SEQ) {
	    delta = (token & 0x1f) + 1;
	    DBG (ATF_DEBUG, 0, msg("\taft_EXEC_SEQ %d\n", delta));
	    ast_ptr->instr_left += delta;
	    /* Back around loop */
	}
	else if ((token & 0xc0) == aft_JMP_SHORT) {
	    delta = VALUE_DELTAT(token) << 2;
	    atf_set_pc (ast_ptr, delta, true/*relative*/);
	    DBG (ATF_DEBUG, 0, msg ("\taft_JMP_SHORT %x\n", delta));
	}
	else if ((token & 0xc0) == aft_EXEC_EA_SHORT) {
	    delta = VALUE_DELTAT(token) << 2;
	    last_ea_delta = delta;
	    aie_ptr=atf_cache_lookup (ast_ptr, (u64)delta, &opcode, &last_vea, true/*rel*/, false/*abs*/);
	    DBG (ATF_DEBUG, 0, msg ("\taft_EXEC_EA_SHORT %x\n", delta));
	    goto return_instr_old;
	}
	else if ((token & 0xf0) == aft_SET_CPU) {
	    *value1_ptr = avg_CPU;
	    *value2_ptr = token & 0x0f;
	    atf_set_variable (ast_ptr, (AtfVariable_enum)*value1_ptr, *value2_ptr);
	    DBG (ATF_DEBUG, 0, msg ("\taft_SET_CPU %d\n", *value2_ptr));
	    return (act_SET);
	}
	else if ((token & 0xe0) == 0xe0) {
	    switch (token) {
	    case aft_JMP_REL:
		delta = get_delta3 (ast_ptr);
		atf_set_pc (ast_ptr, delta, true/*relative*/);
		DBG (ATF_DEBUG, 0, msg ("\taft_JMP_REL %x\n", delta));
		break;
		
	    case aft_JMP_EXACT:
		delta = get_qw (ast_ptr);
		atf_set_pc (ast_ptr, delta, false/*absolute*/);
		DBG (ATF_DEBUG, 0, msg ("\taft_JMP_EXACT %lx\n", delta));
		break;
		
	    case aft_EXEC_EA_LAST:
		delta = last_ea_delta;
		aie_ptr=atf_cache_lookup (ast_ptr, (u64)delta, &opcode, &last_vea, true/*rel*/, false/*abs*/);
		DBG (ATF_DEBUG, 0, msg ("\taft_EXEC_EA_LAST\n"));
		goto return_instr_old;

	    case aft_EXEC_EA_REL:
		delta = get_delta3 (ast_ptr);
		last_ea_delta = delta;
		aie_ptr=atf_cache_lookup (ast_ptr, (u64)delta, &opcode, &last_vea, true/*rel*/, false/*abs*/);
		DBG (ATF_DEBUG, 0, msg ("\taft_EXEC_EA_REL\n"));
		goto return_instr_old;

	    case aft_EXEC_EA_EXACT:
		last_vea = get_qw (ast_ptr);
		aie_ptr=atf_cache_lookup (ast_ptr, last_vea, &opcode, &last_vea, false/*rel*/, true/*abs*/);
		DBG (ATF_DEBUG, 0, msg ("\taft_EXEC_EA_EXACT\n"));
		goto return_instr_old;
		
	    case aft_EXEC_OP:
		opcode = get_lw(ast_ptr);
		last_vea = 0L;
		DBG (ATF_DEBUG, 0, msg ("\taft_EXEC_OP\n"));
		/* Common entry point for returning NEW instruction information */
		/* Sorry this is a goto, we need common code to do a return or continue, so this is the fastest way */
	    return_instr_new:
		aie_ptr = atf_cache_opcode (ast_ptr, opcode, last_vea);
		*value1_ptr = cpu_ptr->vpc;
		*value2_ptr = (ulong)&(aie_ptr->opcode_etc);
		*value3_ptr = last_vea;
		atf_set_pc (ast_ptr, 0x4, true/*relative*/);
		RETURN_OR_CONTINUE (act_INSTR_NEW);

	    case aft_EXEC_OP_EA:
		opcode = get_lw(ast_ptr);
		last_vea = get_qw(ast_ptr);
		DBG (ATF_DEBUG, 0, msg ("\taft_EXEC_OP_EA\n"));
		goto return_instr_new;

	    case aft_EXEC_OP_SEA:
		opcode = get_lw(ast_ptr);
		last_vea = get_lw(ast_ptr);
		DBG (ATF_DEBUG, 0, msg ("\taft_EXEC_OP_SEA\n"));
		goto return_instr_new;
		
	    case aft_SET_REL:
		*value1_ptr = get_byte(ast_ptr);
		byte = get_byte(ast_ptr);
		*value2_ptr = VALUE_DELTA1 (byte) + atf_get_variable (ast_ptr, (AtfVariable_enum)*value1_ptr);
		atf_set_variable (ast_ptr, (AtfVariable_enum)*value1_ptr, *value2_ptr);
		DBG (ATF_DEBUG, 0, msg ("\taft_SET_REL %lx %lx\n", *value1_ptr, *value2_ptr));
		return (act_SET);
		
	    case aft_SET_EXACT:
		*value1_ptr = get_byte(ast_ptr);
		*value2_ptr = get_qw(ast_ptr);
		atf_set_variable (ast_ptr, (AtfVariable_enum)*value1_ptr, *value2_ptr);
		DBG (ATF_DEBUG, 0, msg ("\taft_SET_EXACT %lx %lx\n", *value1_ptr, *value2_ptr));
		return (act_SET);
		
	    case aft_INC_EPOCH:
		*value1_ptr = avg_EPOCH;
		*value2_ptr = ast_ptr->global_var[ avg_EPOCH ] + 1;
		atf_set_variable (ast_ptr, (AtfVariable_enum)*value1_ptr, *value2_ptr);
		DBG (ATF_DEBUG, 0, msg ("\taft_EPOCH %lx %lx\n", *value1_ptr, *value2_ptr));
		return (act_SET);
		
	    case aft_PTE:
		delta = get_qw(ast_ptr);
		*value1_ptr = PAGE_ADDR_PAGE(delta);
		*value2_ptr = get_qw(ast_ptr);
		atf_tlb_fill_entry (ast_ptr, *value1_ptr, *value2_ptr, PAGE_ADDR_OFFSET(delta));
		DBG (ATF_DEBUG, 0, msg ("\taft_PTE %lx %lx\n", *value1_ptr, *value2_ptr));
		RETURN_OR_CONTINUE (act_PTE);
		
	    case aft_DMA:
		*value1_ptr = get_qw(ast_ptr);
		*value2_ptr = get_lw(ast_ptr);
		*value3_ptr = get_byte(ast_ptr);
		DBG (ATF_DEBUG, 0, msg ("\taft_DMA %lx %lx %lx\n", *value1_ptr, *value2_ptr, *value3_ptr));
		RETURN_OR_CONTINUE (act_DMA);
		
	    case aft_COMMAND: 
		command = get_byte(ast_ptr);
		DBG (ATF_DEBUG, 0, msg ("\tcmd %lx\n", command));

#if ATF_PERFORMANCE
	        atf_stats.command_ext_count_table[command & 0xff]++;
#endif
	
		switch (command) {
		case acm_DUMP:
		    ast_ptr->is_dumping = true;
		    DBG (ATF_DEBUG, 0, msg ("\tacm_DUMP\n"));
		    break;
		    
		case acm_COMMENT:
		    string = atf_get_string (ast_ptr);
		    DBG (ATF_DEBUG, 0, msg ("\tacm_COMMENT %s\n", string));
		    *value1_ptr = (ulong)string;
		    return (act_COMMENT);
		    
		case acm_COMMENT_ROUT:
		    DBG (ATF_DEBUG, 0, msg ("\tacm_COMMENT_ROUT\n"));
		    *value1_ptr = get_qw (ast_ptr);
		    *value2_ptr = get_qw (ast_ptr);
		    string = atf_get_string (ast_ptr);
		    *value3_ptr = (ulong)string;
		    /* Not passed outside yet */
		    break;
		    
		case acm_INTERRUPT:
		    DBG (ATF_DEBUG, 0, msg ("\tacm_INTERRUPT\n"));
		    return (act_INTERRUPT);

		case acm_RESYNC:
		    DBG (ATF_DEBUG, 0, msg ("\tacm_RESYNC\n"));
		    /* Not passed outside yet */
		    break;
		case acm_BR_ACTION:
		    DBG (ATF_DEBUG, 0, msg ("\tacm_BR_ACTION\n"));
		    *value1_ptr = get_byte (ast_ptr);
		    return (act_BR_ACTION);
		    
		case acm_PROC_TEXT_INFO:
		    DBG (ATF_DEBUG, 0, msg ("\tacm_PROC_TEXT_INFO\n"));
		    *value1_ptr = get_qw (ast_ptr); /* pid */
		    *value2_ptr = get_qw (ast_ptr); /* start */
		    *value3_ptr = get_qw (ast_ptr); /* end */
		    string = atf_get_string (ast_ptr);
		    *value4_ptr = (ulong)string;    /* uu_comm */
		    return(act_PROC_TEXT_INFO);

		case acm_PROC_BSS_INFO:
		    DBG (ATF_DEBUG, 0, msg ("\tacm_PROC_BSS_INFO\n"));
		    *value1_ptr = get_qw (ast_ptr); /* pid */
		    *value2_ptr = get_qw (ast_ptr); /* start */
		    *value3_ptr = get_qw (ast_ptr); /* end */
		    return(act_PROC_BSS_INFO);

		case acm_THREAD_USTACK_ADDR_INFO:
		    DBG (ATF_DEBUG, 0, msg ("\tacm_THREAD_USTACK_ADDR_INFO\n"));
		    *value1_ptr = get_qw (ast_ptr); /* pid */
		    *value2_ptr = get_qw (ast_ptr); /* tid */
		    *value3_ptr = get_qw (ast_ptr); /* start */
		    *value4_ptr = get_qw (ast_ptr); /* end */
		    return(act_THREAD_USTACK_ADDR_INFO);

		case acm_THREAD_USTACK_GROW_INFO:
		    DBG (ATF_DEBUG, 0, msg ("\tacm_THREAD_USTACK_GROW_INFO\n"));
		    *value1_ptr = get_qw (ast_ptr); /* pid */
		    *value2_ptr = get_qw (ast_ptr); /* tid */
		    *value3_ptr = get_qw (ast_ptr); /* size */
		    *value4_ptr = get_byte (ast_ptr); /* uu_grow_up */
		    return(act_THREAD_USTACK_GROW_INFO);

		case acm_THREAD_KSTACK_INFO:
		    DBG (ATF_DEBUG, 0, msg ("\tacm_THREAD_KSTACK_INFO\n"));
		    *value1_ptr = get_qw (ast_ptr); /* pid */
		    *value2_ptr = get_qw (ast_ptr); /* tid */
		    *value3_ptr = get_qw (ast_ptr); /* start */
		    *value4_ptr = get_qw (ast_ptr); /* end */
		    return(act_THREAD_KSTACK_INFO);

		case acm_LIBLOADER_INFO:
		    DBG (ATF_DEBUG, 0, msg ("\tacm_LIBLOADER_INFO\n"));
		    *value1_ptr = get_qw (ast_ptr); /* text start */
		    *value2_ptr = get_qw (ast_ptr); /* text end */
		    *value3_ptr = get_qw (ast_ptr); /* bss start */
		    *value4_ptr = get_qw (ast_ptr); /* bss end */
		    return(act_LIBLOADER_INFO);

		case acm_LIB_INFO:
		    DBG (ATF_DEBUG, 0, msg ("\tacm_LIB_INFO\n"));
		    *value1_ptr = get_qw (ast_ptr); /* text start */
		    *value2_ptr = get_qw (ast_ptr); /* text end */
		    *value3_ptr = get_qw (ast_ptr); /* bss start */
		    *value4_ptr = get_qw (ast_ptr); /* bss end */
		    return(act_LIB_INFO);

		case acm_KERNEL_SWITCH:
		    DBG (ATF_DEBUG, 0, msg ("\tacm_KERNEL_SWITCH\n"));
		    *value1_ptr = get_byte (ast_ptr); /* in */
		    *value2_ptr = get_qw (ast_ptr);   /* addr */
		    string = atf_get_string (ast_ptr);
		    *value3_ptr = (ulong)string;      /* comment */
		    return(act_KERNEL_SWITCH);

		case acm_IDLE_THREAD:
		    DBG (ATF_DEBUG, 0, msg ("\tacm_IDLE_THREAD\n"));
		    *value1_ptr = get_byte (ast_ptr); /* start */
		    return(act_IDLE_THREAD);

		default:
		    DBG (ATF_DEBUG, 0, msg ("Bad command %02x\n", command));
		    break;
		} /* switch command */
		break;
		
	    default:
		DBG_ERROR (msg ("Error! ATF %d: Bad format %02x\n", stream_id, token));
		break;
	    } /* switch format */
	} /* switch token */
	else {
	    DBG_ERROR (msg ("Error! ATF %d: Bad token %02x\n", stream_id, token));
	}

    } /* forever */
}
    
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/* Init */

void ATF_init (void) 
    /* Initialize structures. */
{
    int i;
    struct rlimit rlimit;

    bzero ((char *)atf_stream, sizeof (atf_stream));
    for (i=0; i<ATF_STREAMS_MAX; i++) {
	Atf_Stream *ast_ptr = &atf_stream[i];
	ast_ptr->cpu_ptr = &ast_ptr->cpu[ ast_ptr->global_var[avg_CPU] ];
    }
    if( getrlimit(RLIMIT_DATA, &rlimit) != 0) {
        fprintf(stderr,"Error!  In ATF_init, can't increase RLIMIT_DATA\n");
    }
    /* rlimit.rlim_cur *= 2; */
    rlimit.rlim_cur = rlimit.rlim_max;
    if( setrlimit(RLIMIT_DATA, &rlimit) != 0) {
        fprintf(stderr,"Error!  In ATF_init, can't increase RLIMIT_DATA\n");
    }

    atf_instructions = (Atf_Instr_Entry *****) UTIL_calloc(MEMORY_TABLE_SHIFT, sizeof(Atf_Instr_Entry *****));

#if ATF_PERFORMANCE
    bzero ((char *)&atf_stats, sizeof (atf_stats));
    for (i=0x00; i<=0xff; i++)	    atf_token_length[i] = 1;
    for (i=0x00; i<=0xff; i++)	    atf_command_ext_length[i] = 1;
    atf_token_length[aft_JMP_REL]	= 1+3;
    atf_token_length[aft_JMP_EXACT]	= 1+8;
    atf_token_length[aft_EXEC_EA_REL]	= 1+3;
    atf_token_length[aft_EXEC_EA_EXACT]	= 1+8;
    atf_token_length[aft_EXEC_OP]	= 1+4;
    atf_token_length[aft_EXEC_OP_EA]	= 1+4+8;
    atf_token_length[aft_SET_REL]	= 1+1+1;
    atf_token_length[aft_SET_EXACT]	= 1+8;
    atf_token_length[aft_INC_EPOCH]	= 1;
    atf_token_length[aft_PTE]		= 1+8+8;
    atf_token_length[aft_DMA]		= 1+8+4+1;
    atf_token_length[aft_COMMAND]	= 0;  /* dummy length; actual is extension# * extension_size; see below */

    atf_command_ext_length[acm_DUMP]                     = 1+1;
    atf_command_ext_length[acm_COMMENT]                  = 1+1+4;
    atf_command_ext_length[acm_COMMENT_ROUT]             = 1+1+8+8+4;
    atf_command_ext_length[acm_INTERRUPT]                = 1+1;
    atf_command_ext_length[acm_RESYNC]                   = 1+1;
    atf_command_ext_length[acm_BR_ACTION]                = 1+1+1;
    atf_command_ext_length[acm_PROC_TEXT_INFO]           = 1+1+8+8+8+4;
    atf_command_ext_length[acm_PROC_BSS_INFO]            = 1+1+8+8+8;
    atf_command_ext_length[acm_THREAD_USTACK_ADDR_INFO]  = 1+1+8+8+8+8;
    atf_command_ext_length[acm_THREAD_USTACK_GROW_INFO]  = 1+1+8+8+8+1;
    atf_command_ext_length[acm_THREAD_KSTACK_INFO]       = 1+1+8+8+8+8;
    atf_command_ext_length[acm_LIB_INFO]                 = 1+1+8+8+8+8;
    atf_command_ext_length[acm_LIBLOADER_INFO]           = 1+1+8+8+8+8;
    atf_command_ext_length[acm_KERNEL_SWITCH]            = 1+1+1+8+4;
    atf_command_ext_length[acm_IDLE_THREAD]              = 1+1+1;

#endif
}

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/* Stats */

void ATF_stats (void) 
    /* Print statistics. */
{
#if ATF_PERFORMANCE
    int i;
    Counter size, totsize;

    printf ("\nATF File statistics\n");

    printf ("\tEntry\t\t\t#\t\tBytes\t%% of file\n");
    printf ("\tComment Text \t\t\t%9d\t%5.2f\n",
	    atf_stats.comments, DIVPERCENT (atf_stats.comments, atf_stats.file_size));
    for (i=0x00; i<=0xff; i++) {
	size = atf_stats.token_count_table[i] * atf_token_length[i];
	if (size && i!=aft_COMMAND) printf ("\tToken %02x\t%9d\t%9d\t%5.2f\n",
			  i, atf_stats.token_count_table[i],
			  size, DIVPERCENT (size, atf_stats.file_size));
    }
    /* aft_COMMAND printout */
    size = 0;
    for (i=0x00; i<=0xff; i++)
	size += atf_stats.command_ext_count_table[i] * atf_command_ext_length[i];
    if (size) printf ("\tToken %02x\t%9d\t%9d\t%5.2f\n",
			  aft_COMMAND, atf_stats.token_count_table[aft_COMMAND],
			  size, DIVPERCENT (size, atf_stats.file_size));

    for (i=0x00; i<=0xff; i++) {
	size = atf_stats.command_ext_count_table[i] * atf_command_ext_length[i];
	if (size) printf ("\tCommand Extension %02x\t%9d\t%9d\t%5.2f\n",
			  i, atf_stats.command_ext_count_table[i],
			  size, DIVPERCENT (size, atf_stats.file_size));
    }
    printf ("\n\n");

    for (i=0; i<4; i++) {
	printf ("\tLevel %d tables created\t%9d\n", i, atf_stats.pages[i]);
    }
    printf ("\n\n");
#endif
}

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
