#ifndef IA64_TRACE_H
#define IA64_TRACE_H

/*
 * This tool is meant to trace an linux IA64 program.  It depends on
 * libssdis.c/h and libopcodes.c/h (plus other gnutool libs)
 * 
 * Revision 1.4  2001/10/23 17:21:56  wei
 * This version has been debugged with trace.testcase.  Phi emmulation
 * tested, additional improvements made to trace_sample.c and
 * trace_writer.c, and inst.cpp machinery.  Stackreg is fings avg RSE
 * sof size, and utrace3 tests TRAP_BRANCH
 *
 * Revision 1.3  2001/09/27 22:10:29  wei
 * port for x86 portability.  added memory manager.
 *
 * Revision 1.2  2001/06/20 20:26:32  wei
 * forced trace.c and sstr.h
 *
 * Revision 1.3  2001/05/14 23:42:13  wei
 * incorporates trace reader
 *
 * Revision 1.2  2001/05/13 23:27:57  wei
 * better version to be used in sim-scalar
 */

/*
 * Notes:
 * see below about trace and tr64 interfaces in trace.c
 */

// ASIM core
#include "asim/syntax.h"

// ASIM local module
#include "ssdis.h"

typedef unsigned long tr64_reg;

#define TR64_NUM_GR         128
#define TR64_GR_STACK_BASE  32
#define TR64_GR_STACK_TOP   127
#define TR64_GR_STATIC_BASE 1
#define TR64_GR_STATIC_TOP  31


/* single step status codes */
#define TR64_ERROR          0
#define TR64_DONE           1
#define TR64_SINGLE_STEP    2
#define TR64_VALID          3
#define TR64_VALID_NO_DECODE 4
#define TR64_START          5


typedef struct tr64_reg_frame{
    INT32 stack;      /* total number */           
    INT32 local;	    /* number of local regs */   
    INT32 rot;	    /* number of rotating regs */
} tr64_reg_frame;


typedef enum trace_sel_enum {
    TRACE_PTRACE = 1,
    TRACE_GDB    = 2,
    TRACE_TLITE  = 3
} trace_sel_enum;

/* this keeps an approximately accurate picture
 * of the processor state
 * PC, and next PC, are guarenteed to be correct 
 * EA- effective address is not for ptrace, but is for gdb trace
 * various register state are not, particularly gr regs.
 * PR and ar.cfm are guarenteed for ptrace, 
 */
typedef struct tr64_machine_state {
    trace_sel_enum    trace_sel;      /* tracing mechanism switch */
    INT32             status;         /* whether inst got decoded */
    INT32             pid;            /* process id of trace program */
    FILE*             trace;          /* gdb trace read file */
    char*             fname;          /* program name or gdb trace file name */
    UINT64            pc;            
    UINT64            next_pc;
    INT32             br_taken;       /* branch taken or not */
    UINT64            pr;             /* predicate register  */
    UINT64            ea;             /* memory op effective address */
    UINT64            gr[TR64_NUM_GR]; /* general purpose registers */
    ss_ia64_inst*     inst;           /* current inst */
    UINT64            pfs;            /* pfs */
    UINT64            cfm;            /* cfm */
    INT32             init;           /* do we need to be completely initialized? */
} tr64_machine_state;


/* the ptraced machine state, */
extern tr64_machine_state tr64_traced_state;

/* user visible state */
extern tr64_machine_state tr64_user_state;

/* get specific reg */
extern INT32  tr64_get_gr_reg(INT32 regid, UINT64* regval); 

/* cfm was md_addr_t, but changed to tr64_reg */
extern INT32  tr64_compute_rotate(INT32                     ea_id);


/* LORI added to receive information from pipe  */
struct receive
{
  INT32 slot;                        /* slot number within bundle */
  INT32 num_results;                 /* number of results this insn produces */
  INT32 inst_count;                  /* position of instr in instr stream */
  char result1[16];                /* value of 1st result if exists */
  char result2[16];                /* value of 2nd result if exists */
  char ea[16];                     /* effective addr for loads and stores */
  unsigned long instr_address;     /* address of instruction */
};

typedef struct receive tr64_receive;


/*  pipe descriptor */


/* 
 * some internal state for the package
 * for ptrace:
 * this future PC address, future inst decode
 * future ea (need to collect before executed)
 * 
 */
typedef struct tr64_lookahead {
    /* all future info (current pc+1) */
    ss_ia64_inst* inst;
    INT32         out_cnt;
    INT32         out1_id;
    UINT64        out1;
    UINT64        pr;
    UINT64        psr;
    UINT64        cfm;
    UINT64        pfs;
    UINT64        pc;
    UINT64        ea;         /* base adr read by inst */
    INT32         ea_id;    
    INT32         status;     /* whether inst got decoded */
    INT32         sof;

    /* all guess info (current pc+2) */
    ss_ia64_inst* guess_inst;
    UINT64        guess_ea;  
    UINT64        guess_pc;
    INT32         guess_status;
} tr64_machine_lookahead;


extern tr64_machine_lookahead tr64_traced_lookahead;

/* 
 * discussion on how this works.
 * for ptrace:
 * we have a 3 stage pipeline
 * guess (current +2)
 *   estimated pc, decode inst, read ea
 * future (current+1)
 *   actual pc is here, check if need new inst decode
 *   get reg write values
 * current
 *   report to program 
 *   PC, and NPC (future+1 PC)
 *   update reg state to reflect state for current
 *
 * for gdb trace
 * we have 2 stage pipeline, since lori already gets ea
 * future (current+1 )
 *   actual pc is here, decode isnt
 *   get all state from trace file
 * current
 *   report to program 
 *   PC, and NPC (future+1 PC)
 *   update reg state to reflect state for current
 *
 * note that there is a separate interfaces to work 
 * with trace state and another to read "current" 
 * state
 */

/* initialize ptrace package, and spawn an executable called
 * fname 
 */
extern INT32        tr64_init_ptrace(char*  fname,    
				     char** argv,
				     char** envp
);

/* this is for the newew light weight trace file format */
extern INT32        tr64_init_tlite(char* tname);

extern INT32        tr64_finish_tlite();


/* running a program is a two step process
 * first we need to single step to get new PC address, this allows
 * us to decode the instruction, and get any register values before
 * writeback of the current instruction.  second, we update registers
 * to reflect the execution of the current instruction.
 */

/* single step-   available externallly for debugging
 * other wise only should be used by tr64 internal procedures
 * Either inst or compute_ea must be set.  Inst passes back an instruction
 * compute_ea asks function to update lookahead structure
 */
extern UINT64 tr64_step_pc(INT32* status, ss_ia64_inst* inst, bool compute_ea);
/* similary should only be used for debug, 
 * just minimally zaps state to expected places
 */
extern void       tr64_step_update();

/* print state */
extern void       tr64_print_state(FILE* f,INT32 verbose);

/* pass debug level to library */
extern void       tr64_set_debug(INT32 level);

/* print guess_ea stats to FILE* f */
extern void       tr64_print_ea_stats(FILE* f);


/* this is the top level trace interface.
 * can get the "current" state from here
 * also initializes the underlying machinery from here
 * also handles allocating and freeing ss_ia64_inst structures
 * 
 * this probably will not execute the very last instruction in the trace 
 * due to current limitations, but there is not a problem with the
 * current simulator framework
 */

/*
 * call this after setting the method of tracing
 * return TR64 define status (see above)
 */
extern  INT32       trace_initialize(UINT64*);

/*
 * virtual single step, return TR64 define status 
 * (see above)
 * also passes back decoded instruction
 * which has to be freed
 */
extern  INT32       trace_single_step(
    UINT64*         pc,
    UINT64*         next_pc,
    INT32*          br_taken,      /* if branch, is taken? */
    UINT64*         ea,            /* if memory op, set base adr */
    UINT64*         pr,            /* current pr */
    UINT64*         cfm_pfs,       /* current cfm or pfs depending on inst */
    ss_ia64_inst**  inst           /* pc's current inst */
    );
    

extern  INT32       trace_single_step_pr(
    UINT64*         pc,
    UINT64*         next_pc,
    INT32*          br_taken,      /* if branch, is taken? */
    UINT64*         ea,            /* if memory op, set base adr */
    UINT64*         pr,            /* current pr */
    UINT64*         next_pr,       /* lookahead pr */
    UINT64*         cfm_pfs,       /* current cfm or pfs depending on inst */
    ss_ia64_inst**  inst           /* pc's current inst */
    );
    


/* given a pc, this simulates the behavior of decoding/executing that
 * insturction.  currently the behavior is superficially implemented
 */
extern  INT32       trace_simulate_step(
    UINT64          pc,
    UINT64*         ea,            /* if memory op, set base adr */
    UINT64*         pr,            /* current pr */
    UINT64*         cfm_pfs,       /* simulated cfm */
    ss_ia64_inst**  inst          /* pc's current inst */
    );

extern void  trace_get_pc(UINT64* pc, UINT64 *npc);

#endif /* IA64_TRACE_H */
