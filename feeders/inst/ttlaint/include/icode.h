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
 * $Header$
 * $Log$
 * Revision 1.1  2001/11/15 16:09:43  klauser
 * - moving all (instruction) feeders one level down in directory hierarchy
 * - changed atf feeder to more generic trace feeder + atf reader
 *
 * Revision 1.21  2001/07/06 16:29:12  jgago
 * CSN-feeder-6
 *
 * Revision 1.20  2001/06/19 10:24:40  fardanaz
 * CSN-feeder-5
 *
 * Revision 1.19  2001/06/18 15:52:54  fardanaz
 * CSN-feeder-4
 *
 * Revision 1.18  2001/04/19 14:09:41  fardanaz
 * several TTL bugs fixed
 *
 * Revision 1.17  2001/03/27 14:11:51  fardanaz
 * several tarantula imporvments
 *
 * Revision 1.16  2000/11/03 01:27:51  klauser
 * CSN-feeder-149
 *
 * Revision 1.15  2000/10/17 01:57:44  klauser
 * CSN-feeder-142
 *
 * Revision 1.14  2000/09/28 16:15:58  fardanaz
 * CSN-feeder-127
 *
 * Revision 1.13  2000/09/19 19:10:38  fardanaz
 * CSN-feeder-118
 *
 * Revision 1.11  2000/06/01 20:28:39  harish
 * CSN-feeder-93
 *
 * Revision 1.10  2000/05/18 16:24:56  shubu
 * CSN-feeder-91
 *
 * Revision 1.9  1999/11/03 20:48:03  klauser
 * CSN-feeder-58
 *
 * Revision 1.8  1999/10/25 22:45:18  espasa
 * CSN-feeder-53
 *
 * Revision 1.7  1999/10/21 06:24:29  klauser
 * CSN-feeder-48
 *
 * Revision 1.6  1999/10/19 20:50:32  shubu
 * CSN-feeder-47
 *
 * Revision 1.5  1999/09/15 18:52:33  espasa
 * CSN-feeder-34
 *
 * Revision 1.4  1999/08/27 19:14:57  espasa
 * CSN-feeder-30
 *
 * Revision 1.3  1999/08/19 13:25:34  espasa
 * CSN-feeder-28
 *
 * Revision 1.2  1999/07/22 15:07:22  steffan
 * *** empty log message ***
 *
 * Revision 1.1  1999/07/02 00:37:05  espasa
 * *** empty log message ***
 *
 * Revision 1.2  1999/03/05  22:19:59  swallace
 * add gnu BFD include header files to the include directory
 *
 * Revision 1.23  1998/11/03  21:34:33  edwards
 * Fix error reporting for open system call
 *
 * Revision 1.22  1998/08/10  04:44:49  gianos
 * Added support for new MVI and graphics instructions
 *
 * Revision 1.21  1997/10/01 20:15:39  cfj
 * Fixed handling of quiesce (to prevent cases where thread is quiesced when
 * it should not be).  We now assume that a ldg instruction marks the next
 * load as giving the load address we may quiesce on.
 *
 * Revision 1.20  1997/09/03  20:05:47  cfj
 * Changes to AINT to support load_locked & store_conditional
 * See src/exec.c for more detailed comments.
 *
 * Revision 1.19  1997/08/06  12:15:51  jamey
 * added support for verifying that physregs are written before read
 *
 * Revision 1.18  1997/04/21  14:00:16  jamey
 * added snapshot/restore capability
 *
 * Revision 1.17  1997/03/04  21:51:52  jamey
 * added symbol table parsing
 *
 * Revision 1.16  1997/01/10  20:27:35  jamey
 * increased tlb size
 *
 * Revision 1.15  1996/12/18  22:41:17  jamey
 * pthread support for aintpm
 *
 * Revision 1.14  1996/12/04  18:29:52  jamey
 * removed possible SEGV
 *
 * Revision 1.13  1996/11/25  19:23:24  jamey
 * speeding up aint
 *
 * Revision 1.12  1996/10/23  12:59:34  jamey
 * putting aint on a diet
 *
 * Revision 1.11  1996/10/17  19:54:50  cfj
 * Modified BADPATH code so that (most) basic block statistics are
 * collected correctly.
 * (Using basic block info is now the default for Aint.)
 *
 * Revision 1.10  1996/10/07  15:27:33  cfj
 * Changes to AINT (and pmint-evsim.c) which cause AINT not to break when it
 * is given a bad pc.
 *
 * Revision 1.9  1996/09/13  17:55:08  jamey
 * add field for detecting order traps
 *
 * Revision 1.8  1996/09/11  12:39:51  jamey
 * check for issue before commit
 *
 * Revision 1.7  1996/09/06  19:06:12  jamey
 * speculate past SIGFPE during issue
 *
 * Revision 1.6  1996/08/30  12:58:08  jamey
 * modifications for OSF1/4.0
 *
 * Revision 1.5  1996/08/15  18:02:12  jamey
 * ensure syscalls use their own regmap and not a later one
 *
 * Revision 1.4  1996/08/14  22:36:53  jamey
 * increased number of physical registers
 *
 * Revision 1.3  1996/08/12  18:48:51  jamey
 * folded in Vegas storebuf fixes
 *
 */
 /*
 * Definitions of icodes 
 */

#ifndef ICODE_H
#define ICODE_H

#include <signal.h>
#include <sys/types.h>
#include <malloc.h>

#include "feeder.h"

#include "event.h"
#include "task.h"
#include "aint_malloc.h"
#include "hash.h"
#include "export.h"
#include "bit.h"

/* #include "ipc.h" */

#ifdef ENABLE_TARANTULA_EXTENSIONS
# define NEW_TTL 
#else
# undef NEW_TTL
#endif

#ifdef ENABLE_TLDS_EXTENSIONS
# define NEW_TLDS
#else
# undef NEW_TLDS
#endif

 /*
  * Maximum number of quadwords allowed in a vector register 
  * Federico Ardanaz notes:
  * This cannot be an odd value some code makes /2 and %2 operations on it
  * Even more, if you put a non 64 multiple
  * you MUST rewrite the TTL section in RegType just bellow.
  */
#define MAX_VECTOR_LENGTH	128

typedef struct icode *icode_ptr;
typedef struct process *process_ptr;
typedef struct thread *thread_ptr;
typedef struct inflight_inst *inflight_inst_ptr;

typedef icode_ptr (*PFPI)(inflight_inst_ptr, icode_ptr, thread_ptr);

typedef enum {BB_INNER=0, BB_FIRST=1, BB_LAST=2, BB_SOLE=3} bbcode_t;

typedef enum {RA=0, RB, RC, MaxArgs} Arg;

/* Federico Ardanaz notes:
 * Some TTL instructions use mask and the mask need at least one bit for
 * each vector element, right now our vector size is 128 so we need 128 bits.
 * I now that put this in RegType will grow up a bit the amount of memory used by
 * asim but I think our main problem is CPU time not memory and this is the
 * most natural way to this...
*/
typedef union {
  long Int64;
  unsigned long UInt64;
  double Double;
  float Sp32[2];
  int   Hex32[2];
  
#ifdef NEW_TTL
  /* for TTL masking */
  unsigned long Mask[MAX_VECTOR_LENGTH/64];
#endif  
  
} RegType;

typedef double  FPRegType;
typedef int     Logical_RegNum;
typedef int     Physical_RegNum;


/* Federico Ardanaz notes:
 * -----------------------
 * WARNING, make sure that MAX_ICODE_SIZE >= sizeof(struct icode) 
 * If you change MAX_ICODE_SIZE you can get small performnace differences !
 * This is due to the fact that we use MAX_ICODE_SIZE to dynamically alloc.
 * memory and these virtual addresses are used as physical addresses in the
 * simulated caches (both instructions and data) therefore changing MAX_ICODE_SIZE
 * has the side effect of shift data into the simulated chaches and this produce
 * small performance deviations.
*/
#define MAX_ICODE_SIZE 32*8

typedef struct icode {
  PFPI func;                /* function that simulates this instr */
  icode_ptr next;
  Logical_RegNum args[MaxArgs];  /* the pre-shifted register args */
  Arg dest;                /* Points into args; can equal RA, RB or RC depending on inst type */
  /*
   * the immed field is used to hold an address, and must
   * be 64 bits
   */
  long immed;           /* the immed/disp/func/hint field */
  unsigned long addr;      /* text address of this instr */
  icode_ptr target;        /* target icode of a branch/bsr instruction */
  unsigned short literal;    /* literal field in operate instr: if hi=0 */
  int instr;               /* undecoded instruction */
  unsigned long extended_instr;
  unsigned int iflags;       /* tells what kind of instruction */
  unsigned short opnum;       /* opnum (operation index) for instruction */
  unsigned long markers;     /* bit-vector of markers set for instruction */
  int is_vector : 1;
  int trace_private : 1;
  int trace_shared : 1;
  int is_masked : 1;
  int is_setvm : 1;
  int is_mvfvp : 1;
  
  char size;        /* For read/write operations */

#ifdef UFUNC_IN_ICODE
  int (*ufunc) ();  /* Sim function corresponding to size */
#endif

  int bbindex;             /* for PM: index of the basic-block containing this instruction */
#define PMINDEX_IN_ICODE   /* reinstated this field in attempt to fix bug with patchworks -Jamey 1/10/97 */
#ifdef PMINDEX_IN_ICODE
  int pmindex;             /* for PM: index of this instruction, e.g. (addr-TextStart)/4 */
#endif
  int meminst_index;

#define USE_FAST_FUNC
#undef USE_FAST_FUNC

#ifdef USE_FAST_FUNC
  /*
   * For dirk speedups
   * op_function is function within an operation group.
   */
  unsigned short op_form; /* immediate or register form */
#endif

} icode_t;

#define picode_cycles(picode_) (((picode_)->iflags&E_SPECIAL)?0:1)

struct StBufEntry {
  ulong Addr;
  ulong Data;
};

#ifndef StoreBuffer_Size
#define StoreBuffer_Size 32
#endif

typedef struct StBufEntry StoreBuffer[StoreBuffer_Size];
typedef StoreBuffer *StoreBuffer_ptr;

typedef short regnum_t;

typedef enum TrapReason {
  TR_notrap,
  TR_memop_order_trap,
  TR_invalid_register_trap,
  TR_max_reason
} TrapReason;

typedef struct inflight_inst {
  icode_ptr picode;
  ulong pc;
  Physical_RegNum	args[MaxArgs];
  Physical_RegNum	OldDest_Phys;
  
  Physical_RegNum vl;
  Physical_RegNum vs;
  Physical_RegNum vm;
  
  uint			pos_in_vreg;
  ulong fetchnum;    /* assigned in increasing order */
  ulong vaddr;
  ulong paddr;
  ulong data;
  ulong bitmask;
  struct stq_entry *stq_update;
  struct stq_entry **stq_update_array;	/* Use for vector stores to hold an array of pointers to stq_update's */
  ulong nextpc; /* for jumps and conditional branches */
  icode_ptr next_picode;
  ulong do_memop_cycle;			/* Cycle do_cache_X was done */
  int type;
  char committed;
  char done_memop;
  char done_spec_write;
  char stc_success;
  char issued;
  char trap_detected;
  char trap_info;
  char runstate; 		/* gets transferred to thread upon commit */
  char signal; 			/* gets transferred to thread upon commit */
  char signal_code; 		/* gets set in Aint_issue() if the instruction raised a signal */
  char *signal_pc; 		/* contains the address of the faulting instruction that raised the signal */

  char veaFlag;
  ulong veaExternal; 
  
} inflight_inst_t;


/* Size of the per-thread circular buffer of inflight-insts.
 * This should be a power of two (for fast macro operations),
 * and must be GREATER than the actual number of inflight insts at any time
 */
#define CBIF_SIZE 1024

/* Circular buffer of in-flight instructions */
typedef struct inflight_inst cbif_t[CBIF_SIZE];

typedef unsigned long instid_t, *instid_ptr;

#define IFI_INSTID(ifi_, pthread_) ((ifi_) - ((pthread_)->cbif))

/* The store buffer is simply a linked-list (or an array), that resembles a per-thread 
 * virtually-addressed cache. For each address, we maintain a sequential list of updates,
 * along with a bit-vector to indicate the bytes at the quadword address that were written
 * to. This information is used by the load instructions to choose bytes correctly from
 * the store-buffer and cache
 */

/* Tarantula changes:
 * 
 * In the previous version of the code, the stq_entry only needed to hold an 'id' for the
 * (store) instruction responsible of that entry. When a load entered the issue
 * stage it went to the Read_StoreBuffer routine, and compared itself to all addresses already
 * present in the store buffer. If there was a match, the load would use the 'id' pointer to
 * reach into the store instruction and read its 'ifi->data' field to get the proper data it
 * wanted. 
 *  
 * For the vector instructions, there are two alternatives. Either we expand the 'data' field in 'ifi'
 * to be able to hold 'MAX_VECTOR_LENGTH' values, or we change the store buffer and let each
 * stq_entry hold its own data. I believe the second option is easier, because when a vector load and
 * a vector store conflict, it's straightforward to find the piece of data that the load actually needs.
 * Meanwhile, if you pursue the first option, once you detect a conflict, now you have to figure what
 * piece of data in the vector register is the one that you actually want... (not that this is undoable,
 * but it's harder...
 *
 * Therefore, I have added a 'value' field to each stq_entry that will hold the 64 (or less) bits being
 * stored. Also, because a store might not be writing the full 64 bits, we keep the corresponding bitmask
 * around.
 *
 * Finally I deleted a 'bytes' field that no one was using.
 *
 * roger.
 */

struct stq_entry {
  struct stq_entry *next;
  struct stq_entry *prev;
  ulong fetchnum;
  ulong vaddr;			/* Exact (unshifted and unmasked) address to where the store is writting */
  ulong	data;			/* actual value being stored */
  ulong	bitmask;		/* indicates which bytes are valid */
  instid_t id;    		/* Whodunnit */
#ifdef NEW_TTL
  uint pos_in_vreg;		/* this field is needed for vector 32 bit stores */
#endif  
};

struct stq_addr {
  struct stq_addr *next;
  struct stq_addr *prev;
  ulong addr;
  struct stq_entry Updates;
};


/* qhead and sqe are pointers; addr is (long) rvalue */
#define QSEARCH(qhead, _addr, _sq) {for(_sq=(qhead)->next; _sq!=(qhead); _sq=_sq->next) if (_sq->addr==(_addr))break;}

#define QWORD_MASK (~(7l))

#define SUCC_CIRC(i, P2) ( (i+1) & (P2-1) )
#define PRED_CIRC(i, P2) ( (i+P2-1) & (P2-1) )

#define NEXT_INSTID(_pthread) SUCC_CIRC(_pthread->instid, CBIF_SIZE)
#define INCR_INSTID(_pthread) ( _pthread->instid = NEXT_INSTID(_pthread), _pthread->instid)

#define PREV_INSTID(_pthread) PRED_CIRC(_pthread->instid, CBIF_SIZE)

/* Each text page's TB_Entry contains a pointer to an array of picodes,
 * i.e., pointers to (struct icode).  A block in memory is allocated to
 * hold all the text instructions in linear sequence, and each ITB page
 * stores pointers to each struct in the block. Thus, given a virtual text
 * address of an instruction, its real (aint-space) address is found by
 * taking its offset from the vaddr of the first instruction of the page,
 * and dividing the offset by 4, to get the index into the Itext page,
 * where the picode is stored.  This is handled by addr2iphys().
 *
 * To convert a real address into the text address, we simply look at the 
 * picode->addr field! To ensure validity, each picode inserted into Itext
 * must have the correct address (or shouldn't be converted).
 */

#define IR2V(RADDR) ((icode_ptr)(RADDR))->addr
#define IV2R(PROCESS, VADDR) ((icode_ptr)addr2iphys(PROCESS, VADDR, NULL))

/* addr2iphys(process, textaddr, paccess) returns the (struct icode *) 
 * pointer corresponding to textaddr.
 */
icode_ptr addr2iphys(process_ptr process, long addr, long *paccess);

/* This checks to see if an address points to code space.
 */
int points_to_code_space(process_ptr process, long addr);


/* addr2iphys(process, textaddr, paccess) returns the (struct icode *) 
 * pointer corresponding to textaddr.
 */


/* mnemonics for the register indices into the args[] array */
#if 0 /* see enum above */
#define RA 0
#define RB 1
#define RC 2
/* FP register indices into the args[] array */
#define FA 0
#define FB 1
#define FC 2
#endif


typedef enum {
  R_RUN = 0,
  R_SLEEP,
  R_STALL,
  R_BLOCKED,
  R_WAIT,
  R_ZOMBIE,
  R_DONE,
  R_FREE,
  R_STOPPED, /* by ptrace() equivalent */
  R_SIGNAL,
  R_SINGLESTEP,
  R_MAX
} state_t;

extern const char *runstate_names[R_MAX];

#define WAKEUP(T) ((T)->runstate = R_RUN)
#define SLEEP(T) ((T)->runstate = R_SLEEP)
#define STALL(T) ((T)->runstate = R_STALL)

/* Max number of file descriptors/thread */
#define MAX_FDNUM 64

/* Constants and macros for the per-process Translation Buffer */
#define TB_OFFSET_LENGTH 16
#define TB_PAGESIZE (1 << TB_OFFSET_LENGTH)
#define TB_LISTIDX_LENGTH 8			/* increased this to improve lookup speed -Jamey 1/10/97 */
#define TB_SIZE (1 << TB_LISTIDX_LENGTH)
#define TB_VPAGE(vaddr) (vaddr >> TB_OFFSET_LENGTH)
#define TB_OFFSET(vaddr) (vaddr & (TB_PAGESIZE-1))
#define TB_KEY(vpage) (vpage & (TB_SIZE - 1))
#define TB_TAG(vpage) (vpage >> TB_LISTIDX_LENGTH)

/* returns instruction number */
#define ITB_OFFSET(vaddr) ((vaddr & (TB_PAGESIZE-1)) >> 2)

/* This is not essential unless we need explicit call-stack information
 * Not used in this version of AINT
 */
#define MAX_CALLS 100

/* number of signals supported (0 not used) */
#define MAX_SIGNALS 33

#define SETSIG(sigvec, signum)    (sigvec |= (1 << ((signum)-1)))
#define CLRSIG(sigvec, signum)    (sigvec &= ~(1<<((signum)-1)))
#define ISSETSIG(sigvec, signum) (sigvec & (1 <<((signum)-1)))

typedef void (*sig_handler_t)(int, ...);
struct my_sigvec {
  sig_handler_t sv_handler;
  int sv_mask;
  int sv_flags;
};

/* Page-Table entry for the per-process TB */
struct TB_Entry {
  struct TB_Entry *next;
  void *page;
  icode_ptr textpage; /* for text pages, contains decoded text */
  struct TB_Entry *lookaside; /* Used only at the head of the queue;
				 also pads up to power of 2 */
  long tag;
  int flags;                  /* Holds MAP_xxx flags, including MAP_PRIVATE/MAP_SHARED flag */
  int prot;                   /* Holds PROT_xxx flags */
};

/* Can have copy-on-write stuff some day */

/* Per-process shared-region descriptors */
struct Shm_ds {
  struct Shm_ds *next;
  long shmid;    /* Index into the global (ipc) shmem struct */
  long size;
  ulong addr;    /* Base address to which mapped */
};


#define ZERO_REGISTER 31
#define FP_ZERO_REGISTER (ZERO_REGISTER + 32)
#define ZERO_REGISTER_REDIRECT 32
#define REALLY_ZERO_ZERO
#ifndef REALLY_ZERO_ZERO
# define ZERO_REDIRECT(X) ((X) == 31 ? 32 : (X))
# define ZERO_ZERO_REG
# define ZERO_ZERO_FP
#else
# define ZERO_REDIRECT(X) (X)
# define ZERO_ZERO_REG (pthread->Reg[31].Int64=0)
# define ZERO_ZERO_FP (pthread->FP[31].Double=0)
#endif

#define GP 29
#define SP 30
#define A0 16
#define A1 17
#define A2 18
#define A3 19
#define A4 20
#define A5 21
#define RET_VAL_REG 0
#define ERROR_REG 19 /* from syscalls */
#define CHILD_FLAG_REG 20
#define RA_REG 26
#define PROCEDURE_VALUE_REG 27

/* corresponds to an object file section or to a region allocated by shmat or mmap */
typedef struct segment {
  ulong start;     /* in object space */
  ulong end;       /* inclusive: end = (start + size-1) */
  ulong size;
  int prot;        /* use mmap() PROT_xxxx flags */
  int flags;       /* use mmap() MAP_xxxx flags */
} segment_t;

typedef struct aint_symbol_s {
  ulong base;
  char *name;
} aint_symbol_t;

typedef struct process {
    process_ptr next;
    process_ptr prev;

    int pid;    /* process id assigned by aint */

    /* The following fields concerning address-space
     * implementation are substantially different from
     * the previous (segmented) implementation.
     */
    struct TB_Entry *TB[TB_SIZE];    /* The per-process translation buffer */
    int num_pages;        /* Number of page-table entries allocated - hack for speeding up fork */
    int num_private;        /* Number of actual pages allocated as private */
    struct Shm_ds *Shmem_regions;    /* List of shared regions attached by process */
    unsigned long Unsp_Shmat_Current; /* Address to use in shmat called with NULL addr */

    process_ptr parent;    /* parent process */
    process_ptr youngest;  /* youngest child process */
    process_ptr sibling;   /* next older process with same parent */

    thread_ptr threads;    /* list of threads running in this process. Main is first */
    thread_ptr youngest_thread;  /* Last in the list of threads - for fast append */
    int thread_count;

    /* thread_ptr waiting_threads;       * list of threads waiting on this process */
    int *fd;    /* file descriptors; */

    struct my_sigvec *sigv;    /* for signal handlers */
    sigset_t signal_mask;
    int spec_segv_action;      /* set_speculative() action for SEGV */
    int spec_fpe_action;       /* set_speculative() action for FPE */

    aint_time_t process_time;    /* simulated time for this process */
    aint_time_t process_cpu;    /* accumulated cpu time for this process */
    aint_time_t child_cpu;        /* accumulated cpu time for finished children */

    state_t runstate;
    int is_zombie;        /* =1 if process is almost dead */

    segment_t *segments; /* memory segments used by this process */
    int n_segments;
    int n_instructions;
    ulong entry_point;   /* entry point for this process (virtual address) */
    ulong initial_gp;    /* GP value for this process upon entry */
    ulong stack_end;
    ulong brk;
    
    aint_symbol_t *symtab;
    long nsymbols;

} process_t;


/*
 * Changes to the thread structure required for Tarantula:
 *
 * Now the 'logical' register state is extended to handle the new 32 vector registers
 * plus 3 control registers (VL, VS and VM). Note how, if 
 * the TTL extensions are disbled, the total size of the register map defaults to
 * what the original AINT version was.
 *
 * For the physical register state, you would probably define the vector register state as
 * V_Reg[N_PHYSICAL_V_REGS][MAX_VECTOR_LENGTH]. However, this has the problem that
 * we would have 2 different places where registers live (i.e., field 'Reg' and field
 * 'V_Reg'. Now, when trying to use the 'args' field in an ICODE struct, how would we
 * distinguish between the two fields ? (yes, I know, there are several ways to do this).
 * 
 * The one I think might work best is to simply flatten all vector registers and stick them
 * at the end of the 'Reg' array. Of course, this will require some relatively complex indexing
 * functions when remapping a vector logical into a physical... but I am sure you can handle that :-)
 *
 */


/*
 * Logical State associated with each thread. 
 */
#define	N_INT_LOGICALS		32
#define	N_FP_LOGICALS		32
#define	N_VCTRL_LOGICALS	4
#define	N_VEC_LOGICALS		32

#define TOTAL_SCALAR_LOGICALS	(N_INT_LOGICALS + N_FP_LOGICALS + N_VCTRL_LOGICALS)
#define TOTAL_LOGICALS		(N_INT_LOGICALS + N_FP_LOGICALS + N_VCTRL_LOGICALS + N_VEC_LOGICALS)

#define FIRST_FP_LOGICAL	(N_INT_LOGICALS)
#define FIRST_VCTRL_LOGICAL	(N_INT_LOGICALS + N_FP_LOGICALS)
#define FIRST_VEC_LOGICAL	(N_INT_LOGICALS + N_FP_LOGICALS + N_VCTRL_LOGICALS)

#define HWVL_REG		(FIRST_VCTRL_LOGICAL + 0)
#define VL_REG			(FIRST_VCTRL_LOGICAL + 1)
#define VS_REG			(FIRST_VCTRL_LOGICAL + 2)
#define VM_REG			(FIRST_VCTRL_LOGICAL + 3)


/*
 * Physical Registers. AINT seemed to have a ratio of logicals-to-physicals set
 * at 16, so I've kept that despite it will eat up lots of resources for the
 * vector register file
 */
#define	LOG_TO_PHY_RATIO	16
#define	N_INT_PHYSICALS		N_INT_LOGICALS * LOG_TO_PHY_RATIO
#define	N_FP_PHYSICALS		N_FP_LOGICALS  * LOG_TO_PHY_RATIO
/* we need a lot of VM renaming => to keep a little of margin: x8 */
#define	N_VCTRL_PHYSICALS	N_VCTRL_LOGICALS * LOG_TO_PHY_RATIO	* 8

#define	N_VEC_PHYSICALS		N_VEC_LOGICALS * MAX_VECTOR_LENGTH * LOG_TO_PHY_RATIO

#define TOTAL_SCALAR_PHYSICALS	(N_INT_PHYSICALS + N_FP_PHYSICALS + N_VCTRL_PHYSICALS)
#define FIRST_VECTOR_PHYSICAL	(N_INT_PHYSICALS + N_FP_PHYSICALS + N_VCTRL_PHYSICALS)
#define TOTAL_PHYSICALS 	(N_INT_PHYSICALS + N_FP_PHYSICALS + N_VCTRL_PHYSICALS + N_VEC_PHYSICALS )


typedef struct thread {
  thread_ptr next;
  thread_ptr prev;

  icode_ptr next_fetch_picode; /* Pointer to the next instruction (the pc) */
  event_ptr pevent;    /* pointer to an event structure */
  state_t runstate;    /* status of thread (runnable, sleeping etc. */

  int tid;             /* thread id assigned by aint, used to index into Threads[] */

  struct inflight_inst *cbif;    /* Points to the beginning of a circular
				  * buffer of inflight-insts.
				  */

  instid_t instid;

  ulong fetch_count;    /* Incremented for each fetch */

  struct stq_addr StoreQ;

  long paddr;          /* phys addr computed by an event function */
  long vaddr;

  char *Objname;	/* Pointer to a malloc'ed string containing the Obect file name
			 * from where this thread was launched
			 */

  aint_time_t time;    /*simulated time for this thread */
  aint_time_t cpu_time;    /* accumulated cpu time for this thread */
    /* Stack stuff should go here. */

  int (*ufunc)();      /* user defined or event-specific function to call */
  int exitcode;
  int semval;        /* waiting for this semaphore value */
  int *perrno;
  int terrno;           /* most recent */
  long *calls;         /* save the value of return addresses */
  int calldepth;       /* number of entries in calls */

  int signal;      
  int sigblocked;    /* bit i=1 if signal i currently blocked */
  int sigpending;    /* bit i=1 if signal i currently pending */
  /* context_ptr psave; ptr to thread save area (for signal support) */

  long unique; /* process unique value used by rduniq and wruniq */
  long prio;   /* used for getprio/setprio syscalls */

  process_ptr process;    /* owning process */
  thread_ptr tsibling;    /* next older thread owned by same process */
  thread_ptr wsibling;    /* next older thread waiting on the same process */
  thread_ptr wthread;     /* thread waiting on us */
  thread_ptr child_thread;  /* child thread (fork() or thread_create()) */

  /* long reg[33];    * An extra reg for writes to r32 (redirected r31) */
  /* double fp[33]; */

  long			fpcr;       /* The Floating point control register */
  RegType 		*FP;    /* This should point to the Reg[] array */
  Physical_RegNum	FirstFreePhysicalRegister; /* free list linked through Reg array */
  Physical_RegNum	FirstFreeVectorPhysicalRegister; /* free list linked through Reg array */
  RegType		Reg[TOTAL_PHYSICALS];
  unsigned int		RegValid[TOTAL_PHYSICALS/32];
  Physical_RegNum	RegMap[TOTAL_LOGICALS];

} thread_t;





/* The context of a thread struct (see below) that must be saved 
 * when executing a signal handler... Not implemented yet
 */

typedef struct context {
  thread_ptr next;
  thread_ptr prev;
  long reg[33];
  long fpcr;
  float fp[32];
  icode_ptr picode;
  event_ptr pevent;
  state_t runstate;
  long paddr;
  icode_ptr target;
  int (*ufunc)();
  int *stall_addr;
  int exitcode;
  int semval;    /* waiting for this semaphore value */
  int sigblocked;
  struct context *psave;
  int errcode;
} context_t, *context_ptr;
/* The thread structure */

#define DO_RENAMING

#ifdef DO_RENAMING
/* These macros describe the expression for deriving the physical registers to which
 * the instruction's logical register arguments are mapped.
 *
 * RegArg is of the type Arg, and is one of RA, RB, RC
 * ifi is a pointer to struct inflight_inst; ifi->args is an array of Physical_RegNum
 * pthread->Reg is the per-thread array of physical registers.
 * The renaming is performed at fetch time, which translates the icode->args array
 * (of type Logical_RegNum), into the ifi->args array, using the pthread->RegMap
 * and pthread->Users tables.
 */
#define REG(RegArg)		(pthread->Reg[ifi->args[RegArg]].Int64)
#define FP(RegArg)		(pthread->FP[ifi->args[RegArg]].Double)
#define VELEMFP(RegArg,i)	(pthread->Reg[ifi->args[RegArg]+i].Double)
#define VELEMINT(RegArg,i)	(pthread->Reg[ifi->args[RegArg]+i].Int64)

#define VL		(pthread->Reg[ifi->vl].Int64)
#define VS		(pthread->Reg[ifi->vs].Int64)

/* Federico Ardanaz notes:
 * eps, VM is a 128 bit field so we need a bit more complex handling
 * We expect that VM must be in RC!
*/
#define VMH         (pthread->Reg[ifi->vm].Mask[1])
#define VML         (pthread->Reg[ifi->vm].Mask[0])
#define GETVM(i)    (ISVALID_L(pthread->Reg[ifi->vm].Mask[i/(MAX_VECTOR_LENGTH/2)], i%(MAX_VECTOR_LENGTH/2)) !=0 )
#define SETVM(i)    (SETVALID_L( pthread->Reg[ifi->args[RC]].Mask[i/(MAX_VECTOR_LENGTH/2)], i%(MAX_VECTOR_LENGTH/2) ))
#define CLEARVM(i)  (CLRVALID_L( pthread->Reg[ifi->args[RC]].Mask[i/(MAX_VECTOR_LENGTH/2)], i%(MAX_VECTOR_LENGTH/2) ))


#define MapReg(RegNum)		(pthread->Reg[pthread->RegMap[RegNum]].Int64)
#define MapFP(RegNum)		(pthread->Reg[pthread->RegMap[RegNum + N_INT_LOGICALS]].Double)
#define MapVEC_PTR(RegNum)	(&(pthread->Reg[pthread->RegMap[RegNum + N_INT_LOGICALS + N_FP_LOGICALS]]))
#define MapVCTRL(RegNum)	(pthread->Reg[pthread->RegMap[RegNum + N_INT_LOGICALS + N_FP_LOGICALS + N_VEC_LOGICALS]].Int64)
#define CurMap(LogReg)		(pthread->RegMap[LogReg])

#define MapReg_thr(pthread, RegNum) ((pthread)->Reg[(pthread)->RegMap[RegNum]].Int64)
#define MapFP_thr(pthread, RegNum) ((pthread)->Reg[(pthread)->RegMap[RegNum+N_INT_LOGICALS]].Double)
#define Reg_log2phys(pthread, regnum) ((pthread)->RegMap[regnum])

#define ISREGVALID(pr) ((pthread->RegValid[(pr) >> 5]) & (1 << ((pr)&0x1f)))

#else
#define REG(Rn)    pthread->Reg[picode->args[Rn]]
#define FP(Fn)    (pthread->Reg[picode->args[Fn]+32])
#define MapReg(RegNum) pthread->Reg[RegNum]
#define MapFP(RegNum) (pthread->Reg[RegNum+32])
#define MapReg_thr(pthread, RegNum) (pthread)->Reg[RegNum]
#define MapFP_thr(pthread, RegNum) MapReg_thr(pthread, RegNum+32)
#endif

#define MAP(ADDR) addr2phys(pthread->process, ADDR, NULL)
#define PMAP(ADDR) addr2phys(pthread->process, ADDR, NULL)
#define SMAP(ADDR) addr2phys(pthread->process, ADDR, NULL)

#define IS_SHARED(ADDR) (addr2tbeflags(pthread->process, ADDR) & MAP_SHARED) 

/* Dont remove this endif */
#endif
