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
 * Definitions of global variables used in more than one file
 */

#ifndef GLOBALS_H
#define GLOBALS_H


#include "ipc.h"

#ifdef MAIN
#define EXTERN
#else
#define EXTERN extern
#endif

/* C-style BOOLs */
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

/* Align the simulated address space on an 8k boundary */
#define M_ALIGN (8 * 1024)
#define PAGE_NUMBER_MASK (~(M_ALIGN-1))
#define PAGE_OFFSET_MASK (M_ALIGN-1)

/* have aint be verbose about what's going on (e.g., loading) */
EXTERN int Verbose;
/* have aint be extra quiet */
EXTERN int Quiet;
EXTERN char *job_separator;
EXTERN int aint_trace_state_changes;
EXTERN int aint_use_verifier_thread;
EXTERN int aint_use_oracle_thread;

/* the number of existing processes */
EXTERN int Nprocs;
EXTERN int Nthreads;

/* name of object file being traced */
EXTERN char *Objname;

/* Aint output file */
EXTERN char *Aint_output_filename;
EXTERN FILE *Aint_output;

/* marker statistics output file */
EXTERN char *Aint_marker_filename;
EXTERN FILE *Aint_marker_output;

EXTERN int aint_recognize_unop;

EXTERN char *Debugger_tty_name;
EXTERN int AINT_in_issue;
EXTERN process_ptr AINT_issue_process;

/* Name of the aint executable */
EXTERN char *Simname;
/* All the xxx_start variables are addresses  in the object's memory space
 */

EXTERN ulong Data_start;     /* start of data section in memory */
EXTERN ulong Data_end;       /* end of all private memory */
EXTERN ulong Shmem_start;
EXTERN ulong Shmem_end;

EXTERN ulong Sp_shmat_start;    /* Lowest addr where shmat can be requested */
EXTERN ulong Sp_shmat_size;    /* Total size of attached segments in a process */
EXTERN ulong Sp_shmat_end;    /* Highest addr (+size) where shmat can be requested */


EXTERN long Stack_size;     /* in bytes */
EXTERN long Heap_size;       /* in bytes (for private malloc */
EXTERN long Mem_size;       /* data+bss+heap+stack in bytes */
EXTERN long Shmem_size;

EXTERN long Text_seek;      /* seek offset in file for text section */
EXTERN long Pdata_seek;
EXTERN long Rdata_seek;
EXTERN long Data_seek;
EXTERN long Sdata_seek;
EXTERN long Rconst_seek;

extern void find_proc_addrs(char *objname, const char **symbolnames, unsigned long *symboladdrs);
EXTERN unsigned long sim_user_addr;

EXTERN int zap_bogus_ldgp_after_sqrtt;
EXTERN int replace_sqrt_function_calls;

EXTERN unsigned long sqrt_function_addr;
EXTERN unsigned long sqrtf_function_addr;
EXTERN unsigned long f_sqrt_function_addr; 
EXTERN unsigned long f_sqrtf_function_addr; 
EXTERN unsigned long f_sqrt4_function_addr; 
EXTERN unsigned long sqrt4_function_addr; 

/* Hook Value for object breakpoint */
EXTERN ulong ObjHook;
extern int ObjHookProc(icode_ptr picode, thread_ptr pthread);

/* Default sizes for memory segments */
#ifndef SHMEM_SIZE
#define SHMEM_SIZE (1024 * 1024 * 1024)
#endif

/* Size (in terms of # of lsb's aligned) of lock-range for ldx_l and stx_c */
#ifndef LOCK_RANGE_SZ
#define LOCK_RANGE_SZ 5 
/* 32 bytes */
#endif

#ifndef SHARED_START
#define SHARED_START ((ulong)(0x00010000L))
#endif

#ifndef SP_SHMAT_START
#define SP_SHMAT_START ((ulong)(0x00010000L))
#endif

#ifndef SP_SHMAT_END
#define SP_SHMAT_END ((ulong) (SP_SHMAT_START + SHMEM_SIZE))
#endif

#ifndef UNSP_SHMAT_START
#define UNSP_SHMAT_START ((ulong)(0x00010000))
#endif

#ifndef UNSP_SHMAT_END
#define UNSP_SHMAT_END ((ulong)(UNSP_SHMAT_START + SHMEM_SIZE))
#endif

#ifndef STACK_SIZE
#define STACK_SIZE ((ulong) 2*1024*1024*1024)
#endif

#ifndef HEAP_SIZE
#define HEAP_SIZE ((ulong) 2*1024*1024*1024)
#endif

#ifndef MAX_NPROCS
#define MAX_NPROCS 4

#ifndef MAX_NTHREADS
#define MAX_NTHREADS 4
#endif

/* arguments of initial processes being traced */
EXTERN int ProcessArgc[MAX_NTHREADS];
EXTERN char **ProcessArgv[MAX_NTHREADS];
EXTERN char *ProcessFilename[MAX_NTHREADS];

/* zero if all top-level processes exited with zero, -1 otherwise */
EXTERN int aint_exit_code;

#endif

/* Type of trace being executed */
/* WARNING: Same table in evsim.h */
#ifndef _THREAD_TYPE_DECL
#define _THREAD_TYPE_DECL
typedef enum {
  TT_ATOM=0,		/* Atom execution/ trace */
  TT_PW,		/* Aint or trace7 patchworks */
  TT_ATF,		/* Aint or trace7 atf */
  TT_AINT		/* Aint -p process or Aint -s snapshot */
} thread_type_t;
extern thread_type_t EVSIM_thread_type[MAX_NTHREADS];
#endif

/*
 * Number of instructions in current picode
 */
extern int insn_count;
/*
 * Number of instructions actually allocated
 */
extern int insn_alloc;

EXTERN int aint_parse_basic_blocks;

/* Ckpoint_freq controls the frequency of calls to sim_checkpoint().
 * This can be set on the command line as well as changed by the user while
 * the program is running. If set to zero, no further calls to sim_checkpoint
 * will occur.
 */
#define DEFAULT_CKPOINT_FREQ 10000000
EXTERN unsigned int Ckpoint_freq;

EXTERN int Recycle_threads;

/* The time of the first fork, or zero if no forks yet */
EXTERN aint_time_t First_fork_elapsed;

/* The accumulated cpu time of processor 0 at the time of the first fork */
EXTERN aint_time_t First_fork_cpu;


/* All queues are circularly doubly linked lists with a head node.
 * When a thread exits, it goes on the done_q until the parent waits on
 * it. If a parent exits before its children, it goes to sleep. When the
 * child exits, it wakes up the parent. The parent moves finished children
 * from the done_q to the free_q
 */

EXTERN thread_t Run_q;
EXTERN thread_t Free_q;
EXTERN thread_t Done_q;
EXTERN thread_t Sleep_q;

EXTERN process_t Free_Process_q;
EXTERN process_t Run_Process_q;

/* the array of threads, indexed by tids */
EXTERN thread_ptr Threads;

/* The array of processes (akin to Mach tasks); indexed by pids */
EXTERN process_ptr Processes;

EXTERN int Maxpid;

EXTERN int Maxtid;

/* Free list pointer for events. One event is allocated for each thread */
EXTERN event_ptr Event_free;

/* The max number of processes that can be simulated */
EXTERN int Max_nprocs;

/* Max # of threads that can be active. Since each thread runs on a separate
 * processor, this is also the max # of processors in the system.
 */

EXTERN int Max_nthreads;

/* Struct used by shmem syscalls */
EXTERN struct sysv_shm Shmseg[MAX_SHMSEG];

/* Flag telling read_text what events are flagged */
EXTERN int Trace_option;

/* Possible values for Trace_option */
#define TRACE_NONE    0
#define TRACE_INST    1
#define TRACE_PRIVATE    2
#define TRACE_SHARED     4
#define TRACE_REFS        (TRACE_PRIVATE | TRACE_SHARED)
#define TRACE_SYNC    8    /* LDx_L, STx_C, MB */
#define TRACE_DEFAULT     (TRACE_REFS | TRACE_SYNC)

/* Function prototypes */
void block_thr(qnode_ptr q, thread_ptr pthread);
thread_ptr unblock_thr(thread_ptr q);
event_ptr event_unblock(thread_ptr pthread, thread_ptr pthr);
void event_terminate(thread_ptr pthread);
void event_exit(thread_ptr pthread);

int wait_for_children(thread_ptr pthread);
int wait_for_child(thread_ptr pthread);


#endif
