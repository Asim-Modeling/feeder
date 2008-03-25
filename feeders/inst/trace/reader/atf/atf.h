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
#pragma ident "$Id: atf.h 799 2006-10-31 12:27:52Z cjbeckma $"
/***********************************************************************
 *
 *	ABSTRACT:	Arana Trace Format Header
 *
 *	AUTHOR:		snyder@segsrv.hlo.dec.com
 *
 ***********************************************************************/


#ifndef _ATF_H
#define _ATF_H

// ASIM local module
#include "portability.h" /* u8 i8 u32 i32 u64 i64 */
#include "atfutil.h"	/* DBG, ASSERT, msg, etc */

#define MAX_THREADS 4

#define	PAGE_SIZE_LOG2	13	/* Size of memory pages... Don't change without looking at atf_cache_location */
#define PAGE_ADDR_OFFSET(adr) ((adr) & MASK(PAGE_SIZE_LOG2))
#define PAGE_ADDR_PAGE(adr)   ((adr) & ~MASK(PAGE_SIZE_LOG2))

#define ATF_CPU_MAX	16	/* Number of CPUs allowed to read from a trace... Probably should be made dynamic */

/**********************************************************************
 * Filenames:
 *	Files start off with a name_aa.atf
 *	Subsequent files are loaded automatically by incrementing aa, to name_ab.atf for example
 *	Trailing colons limit the trace to a single CPU, IE name_aa.atf:0 to only read CPU #0
 */

#define ATF_MAGIC_NUMBER	0x12086900	/* 00 represents version number */

/**********************************************************************
 * Format Definitions
 *
 *	The files are read as a stream of bytes.   A byte is read and
 *	decoded to indicate the format, which indicates the number of
 *	additional bytes to read from the file.
 *
 *  	# of	Byte0 bit		Other bytes	  	    	Description
 *  	Bytes	7 6 5 4 3 2 1 0
 *
 *  	1	0 0 instcount__						Sequential Instructions
 *
 *	1	0 1 d delta____						Branch d=fwd/reverse, delta=# instructions
 *	4	1 1 1 aft_JMP_REL  	Byte1-3=value_offset		Branch, big relative VPC
 *	9	1 1 1 aft_JMP_EXACT	Byte1-7=value			Branch, exact relative VPC
 *
 *	1	1 1 0 0 x x x x						Reserved
 *
 *	1	1 0 d delta____						Execute Load/St, small relative VEA
 *	4	1 1 1 aft_EXEC_EA_REL   Byte1-3=value_offset		Execute Load/St, big relative VEA
 *	9	1 1 1 aft_EXEC_EA_EXACT Byte1-7=vea			Execute Load/St, exact VEA
 *
 *	1	1 1 0 1 cpu_num						Set CPU number variable
 *	1	1 1 1 aft_INC_EPOCH					Increment epoch variable
 *	3	1 1 1 aft_SET_REL   	Byte1=av?_enum,	Byte2=offset	Set Variable, relative change
 *	9	1 1 1 aft_SET_EXACT  	Byte1=av?_enum,	Byte2-9=value	Set Variable, exact number
 *
 *	17	1 1 1 aft_PTE    	Byte1-7=va,	Byte8-16=PTE	Page table entry loading (lsb of va indicates I/D)
 *	13	1 1 1 aft_DMA    	Byte1-7=pa B8-11=len B12=Action	Dma action
 * 	5	1 1 1 aft_EXEC_OP    	Byte1-4=opcode			Execute new instruction
 * 	13	1 1 1 aft_EXEC_OP_EA 	Byte1-4=opcode	Byte5-12=EA	Execute new instruction, with EA
 * 	9	1 1 1 aft_EXEC_OP_SEA 	Byte1-4=opcode	Byte5-9=EA	Execute new instruction, with short EA
 *	4+	1 1 1 aft_COMMAND   	Byte1=acm_enum, Byte2...=?	Extended command
 *					acm_DUMP	  many		  Start Cache Dump (0=end)
 *					acm_COMMENT	  Byte2..=string  Comment string
 *					acm_COMMENT_ROUT  B2-8=stpc ->	  Routine comment, byte2-8=startpc, 9-19=endpc, 20+=string
 *					acm_INTERRUPT			  Interrupt informational
 *					acm_RESYC			  Resync informational
 *                                      acm_BR_ACTION     Byte2=action    Branch action informational
 *
 *                                      acm_PROC_TEXT_INFO                Process text vspace action informational
 *                                      acm_PROC_BSS_INFO                 Process bss/data vspace action informational
 *                                      acm_THREAD_USTACK_ADDR_INFO       Thread user stack vspace (start/end) action informational
 *                                      acm_THREAD_USTACK_GROW_INFO       Thread user stack size/grow action informational
 *                                      acm_THREAD_KSTACK_INFO            Thread kernel stack vspace action informational
 *                                      acm_LIBLOADER_INFO                Library loader vspace action informational
 *                                      acm_LIB_INFO                      Libraries vspace action informational
 *                                      acm_KERNEL_SWITCH                 Addresses producing kernel in/out switches
 *                                      acm_IDLE_THREAD                   Idle thread tracking
 *
 */

#if ATF_H_INTERNALS
typedef enum {
    /* 0x00 .. 0x3f */
    aft_JMP_SHORT=0x00,		/* JMP short distance */
    /* 0x40 .. 0x7f */
    aft_EXEC_EA_SHORT=0x40,	/* Load, short EA delta */
    /* 0x80 .. 0x9f */
    aft_EXEC_SEQ=0x80,		/* Sequential execution */
    /* 0xa0 .. 0xbf */
    /* reserved */
    /* 0xc0 .. 0xcf */
    /* reserved */
    /* 0xd0 .. 0xdf */
    aft_SET_CPU=0xd0,		/* Set CPU number to 0..16 */
    /* 0xe0 .. 0xff */
    aft_JMP_REL=0xe0,		/* Branch, big relative VPC */
    aft_JMP_EXACT=0xe1,		/* Branch, exact relative VPC */
    aft_EXEC_EA_REL=0xe2,	/* Load, big relative VEA */
    aft_EXEC_EA_EXACT=0xe3,	/* Load, exact VEA */
    aft_EXEC_EA_LAST=0xe7,	/* Load, same relative value as last EXEC_EA_REL */
    aft_EXEC_OP=0xe4,		/* New instruction */
    aft_EXEC_OP_EA=0xe5,	/* New instruction (load/store), with EA */
    aft_EXEC_OP_SEA=0xe6,	/* New instruction (load/store), with short EA */
    aft_SET_REL=0xe8,		/* Set Variable, relative change */
    aft_SET_EXACT=0xe9,		/* Set Variable, exact number */
    aft_INC_EPOCH=0xea,		/* Set Variable, relative change */
    aft_PTE=0xeb,		/* Load Page table entry */
    aft_DMA=0xec,		/* Dma action */
    aft_COMMAND=0xff	/* Extended command */
} AtfFormatType_enum;
#endif /*ATF_H_INTERNALS*/

/***********************************************************************
 * Variables
 *
 *	Generic longwords which indicate information about the file
 *	on a *per cpu or global* basis are loaded using variables.
 *	This method allows simple expansion without special casing every new variable.
 */

typedef enum {
    /* avg_OS variable: operating systems */
    aos_OSF=0,		/* OSF Unix */
    aos_NT=1,		/* NT */
    aos_VMS=2		/* VMS */
} AtfOS_enum;

/* avl_SUPPAG variable: superpage bitmasks */
#define asp_SP_EV5 	(1<<0)	/* EV5 SPE<0>  VA<42:30>=1ffe  -> PA<39:30>=0 */
#define asp_SP_EV6 	(1<<1)	/* EV6 SPE<0>  VA<47:30>=3fffe -> PA<43:30>=0 */
#define asp_KSEG_EV5	(1<<2)	/* EV5 SPE<1>  VA<42:41>=2     -> PA<40>=signextend */
#define asp_KSEG_EV6	(1<<3)	/* EV6 SPE<1>  VA<47:46>=2     -> PA<47:46>=0 */
#define asp_DIRECT 	(1<<8)	/* Direct map virtual->physical */

typedef enum {
    /* Local variables */
    avl_TID,		/* Thread ID */
    avl_PID,		/* Process ID */
    avl_ASN,		/* Process's Address Space Number */
    avl_ASM,		/* Process's Address Space Match */
    avl_SUPPAG,		/* Superpage enable bitmask (see asp_*) */
    avl_MAX,		/*END*/
    /* Global variables */
    avg_CPU,		/* Next CPU number command is for */
    avg_EPOCH,		/* Epoch number */
    avg_VERSION,	/* File format */
    avg_OS,		/* Operating system (see AtfOS_enum) */
    avg_MAX		/*END*/
    /* Note only encoding space for 256 total */
} AtfVariable_enum;

/*
 * VERSIONS:
 *            1: act_INSTR_OLD, act_INSTR_new, act_SET, act_PTE, act_DUMP, act_COMMENT actions.
 *            2: DMA, INTERRUPT, BR_ACTION actions added.
 *            3: BR_ACTION debugged. Reader has to figure out if branch is taken by
 *               looking at the target/fall-through/next instructions. Only branches
 *               whose path is interrupted just after the branch use the BR_ACTION!
 *               act_PROC_TEXT_INFO, act_PROC_BSS_INFO, act_THREAD_USTACK_ADDR_INFO,
 *               act_THREAD_USTACK_GROW_INFO, act_THREAD_KSTACK_INFO, act_LIBLOADER_INFO,
 *               act_LIB_INFO, act_KERNEL_SWITCH, act_IDLE_THREAD actions added.
 *
 */

#define ATF_VAR_IS_GLOBAL(varnum) (varnum & 0x80)	/* Is this variable a global number */
#define ATF_GLOBAL_VAR(varnum) (varnum & 0x7f)		/* Give me 12383129837198237198237Is this variable a global number */

/**********************************************************************
 * Extended Commands
 *
 *	Less used commands are encoded into a second byte.
 *	These commands may have variable length payloads such as strings.
 */

#if ATF_H_INTERNALS
typedef enum {
    acm_DUMP=0,		/* Start cache dump */
    acm_COMMENT=1,	/* Generic comment */
    acm_COMMENT_ROUT=2,	/* Routine comment */
    acm_INTERRUPT=3,	/* Interrupt informational */
    acm_RESYNC=4,	/* Resync informational */
    acm_BR_ACTION=5,	/* Branch action informational */

    acm_PROC_TEXT_INFO=6,          /* Process text vspace action informational */
    acm_PROC_BSS_INFO=7,           /* Process bss/data vspace action informational */
    acm_THREAD_USTACK_ADDR_INFO=8, /* Thread user stack vspace (start/end) action informational */
    acm_THREAD_USTACK_GROW_INFO=9, /* Thread user stack size/grow action informational */
    acm_THREAD_KSTACK_INFO=10,     /* Thread kernel stack vspace action informational */
    acm_LIBLOADER_INFO=11,         /* Library loader vspace action informational */
    acm_LIB_INFO=12,               /* Library vspace action informational */
    acm_KERNEL_SWITCH=13,          /* Addresses producing kernel in/out switches */
    acm_IDLE_THREAD=14,            /* Idle thread tracking */

    acm_MAX=255         /* END */
    /* Note only encoding space for 256 total */
} AtfCommand_enum;
#endif /*ATF_H_INTERNALS*/

/***********************************************************************
 * Cache dump format
 *		
 *	The cache dump consists of a LW or QW per cached location.
 *	
 *	Bit Number
 *	1....
 *	2....9 9....6 6....3 3....0 
 *	7....6 5....4 3....2 1....6   5   4   3   2   1   0
 *	       	             pa....   rsv  OSDE   0   1   D/I
 *	              pa...........   rsv  OSDE   1   0   D/I
 *	va........... pa...........   rsv  OSDE   1   1   D/I
 *
 *	Bit 0, D/I is set to indicate dstream, clear for istream.
 *		If both, include two LW/QWs, one for each.
 *	Bit 1 & 2 indicate if a 32 bit sign extended physical address,
 *		a 64 bit pa, or a 64 bit VA and 64 bit PA are provided.
 *	Bit 3 & 4 indicate cache state, 3=DirtyShared, 2=Shared, 1=Dirty, 0=Excl
 *		don't include a entry if a line is invalid
 *	Bit 5 is reserved
 *
 *	31:0=0 indicates end of cache dump, make sure that's not written by accident!
 *
 *	Arana recommends keeping 16 MB of data (sort 16 assoc to 64K by 64 assoc)
 *	worth of addresses and using the va/pa form.
 *	Send the most recent data last, as cache simulators work on the fly.
 *	Preprocessors and simulators for this subformat already exist, contact snyder@segsrv.hlo.dec.com.
 *
 **********************************************************************/

typedef enum {
    ads_EXCLUSIVE=0,
    ads_DIRTY=1,
    ads_SHARED=2,
    ads_DIRTYSHARED=3
} AtfDumpState_enum;

/***********************************************************************
 * Per-instruction structure
 *
 * A pointer to this structure is returned with act_INSTR_* when a instruction is executed
 * Everything other then the opcode may be modified by the application and if so the modified
 * data will be passed next time that instruction reoccurs.  This is useful for decoding, etc.
 * Be careful though, the entry may later be reused by a new opcode at the same address.  (If so act_INSTR_new is passed)
 */

#ifndef ATF_INSTR_OP_ETC_DEFINED	/* Predefine if you want different information here */
#define ATF_INSTR_OP_ETC_DEFINED
typedef struct {
    Opcode opcode;		/* 32 bit instruction word ... required field  */
    /* Additional stuff like decoded field may go here */
} Atf_Instr_Op_Etc;
#endif

/***********************************************************************
 * Overall functions
 */

extern void ATF_init (void);
extern void ATF_file_close (int stream_id);
extern Boolean ATF_file_open (int stream_id, const char *filename, Boolean do_write);
extern void ATF_stats (void);

/***********************************************************************
 * TLB/Utility functions
 */

extern Address ATF_tlb_translate (int stream_id, Address va, Boolean is_istream);
extern char *ATF_get_filename (int stream_id);
extern u64 ATF_get_local (int stream_id, AtfVariable_enum varnum);
extern u64 ATF_get_global (int stream_id, AtfVariable_enum varnum);
extern void ATF_tlb_dump_dbg (Address va);

/***********************************************************************
 * WRITER functions
 */

extern void ATF_write_global (int stream_id, AtfVariable_enum varnum, u64 value);
extern void ATF_write_local (int stream_id, AtfVariable_enum varnum, u64 value);
extern void ATF_write_pte (int stream_id, Address va, u64 pte);
extern void ATF_write_dma (int stream_id, Address pa, uint length, Boolean isDMAwrite);
extern void ATF_write_comment (int stream_id, const char *comment);
extern void ATF_write_comment_rout (int stream_id, Address ppc_start, Address ppc_end, const char *comment);
extern void ATF_write_interrupt (int stream_id);
extern void ATF_write_resync (int stream_id);
extern void ATF_write_branch_action (int stream_id, unsigned char action);

extern void ATF_write_proc_text_info (int stream_id, u64 pid, Address text_start, Address text_end, const char *comment);
extern void ATF_write_proc_bss_info (int stream_id, u64 pid, Address bss_start, Address bss_end);
extern void ATF_write_thread_ustack_addr_info (int stream_id, u64 pid, u64 tid, Address ustack_start, Address ustack_end);
extern void ATF_write_thread_ustack_grow_info (int stream_id, u64 pid, u64 tid, u64 ustack_curr_size, Boolean ustack_grow_up);
extern void ATF_write_thread_kstack_info (int stream_id, u64 pid, u64 tid, Address kstack_start, Address kstack_end);
extern void ATF_write_libloader_info (int stream_id, Address text_start, Address text_end, Address bss_start, Address bss_end);
extern void ATF_write_lib_info (int stream_id, Address text_start, Address text_end, Address bss_start, Address bss_end);

extern void ATF_write_kernel_switch (int stream_id, Boolean in, Address addr, const char *comment);
extern void ATF_write_idle_thread (int stream_id, Boolean idle_start);

extern void ATF_write_dump_start (int stream_id);
extern void ATF_write_dump (int stream_id, Address pa, Address va, Boolean is_dstream, AtfDumpState_enum state);
extern void ATF_write_dump_end (int stream_id);

extern int  ATF_write_instruction (int stream_id, Address vpc, Opcode opcode, Address vea);

/***********************************************************************
 * READER functions
 *		
 * Actions
 *	The generic reader function reads a token from the file and returns
 *	a action.  This action indicates what happened during that call.
 *
 */

typedef enum {
    atf_br_act_TAKEN=0,       /* Branch taken */
    atf_br_act_NOT_TAKEN=1,   /* Branch not taken */

    atf_br_act_MAX=255        /* END */
    /* Note only encoding space for 256 total */
} AtfBranchAction_enum;

typedef enum {
    act_INSTR_OLD,	/* Instruction execution, old opcode */
    act_INSTR_new,	/* Instruction execution, new opcode */
    act_SET,		/* Variable changed */
    act_PTE,		/* PTE changed */
    act_DMA,		/* Dma action */
    act_DUMP,		/* Start cache dump */
    act_COMMENT,	/* Generic comment */
    act_BR_ACTION,	/* Branch action informational */
    act_INTERRUPT,	/* Interrupt informational */

    act_PROC_TEXT_INFO,          /* Process text vspace action informational */
    act_PROC_BSS_INFO,           /* Process bss/data vspace action informational */
    act_THREAD_USTACK_ADDR_INFO, /* Thread user stack vspace (start/end) action informational */
    act_THREAD_USTACK_GROW_INFO, /* Thread user stack size/grow action informational */
    act_THREAD_KSTACK_INFO,      /* Thread kernel stack vspace action informational */
    act_LIBLOADER_INFO,          /* Library loader vspace action informational */
    act_LIB_INFO,                /* Library vspace action informational */
    act_KERNEL_SWITCH,           /* Addresses producing kernel in/out switches */
    act_IDLE_THREAD,             /* Idle thread tracking */

    act_EOF		/* End of file */
} AtfAction_enum;

/*
 * ugly generic parameters that can be anything
 * but compiler still throws warning if explicit typecasts do not
 * maintain bit-width size between int and ptr, so we try to please
 * everybody here;
 */
typedef union generic_param {
  UINT64 uint64;
  void*  ptr;
} AtfGenParam;

extern AtfAction_enum ATF_read_action (int stream_id, AtfGenParam *value1_ptr, AtfGenParam *value2_ptr, AtfGenParam *value3_ptr, AtfGenParam *value4_ptr);


/***********************************************************************
 * Instruction classification functions
 *		
 */

extern INT64 IsLoadStore(UINT64 inst);
extern INT64 IsLoad(UINT64 inst);
extern INT64 IsLlSc(UINT64 inst);
extern INT64 IsStore(UINT64 inst);
extern char rwchar(UINT64 inst);

/***********************************************************************/
#endif /*_ATF_H*/

