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
 * Routines for the main execution loop, thread queues, and all the event
 * generating functions
 */

#include </usr/include/assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include <machine/fpu.h> /* for FPCR_DYN_PLUS */
#include <machine/hal_sysinfo.h>
#include <sys/sysinfo.h>
#if OSVER >= 40
#include <machine/context.h>
#else
#include <machine/signal.h>
#endif
#include <sys/siginfo.h>
#include <sys/user.h>
#include "icode.h"
#include "globals.h"
#include "opcodes.h"
#include "protos.h"
#include "wheel.h"
#include "pmint.h"
#include "gdbserver.h"



#define EXEC_TRACE 0

#define USE_STOREBUFFER

#ifdef DEBUG_REGISTER_RENAMING
static int debug_register_renaming = 1;
#else
#define debug_register_renaming 0
#endif

#ifdef NEW_TTL
extern int ttl_debug;
#endif

int in_FEED_Skip = 0;


/* icode structs for the terminator functions */
#ifdef DEAD_CODE
static icode_t Idone2;                /* Calls terminate_thr() */
static icode_t Idone3;                /* Calls terminator2() */
static icode_t Idone4;                /* Calls done_thr() */
static icode_t I_pdone2;              /* Calls terminate_pthr() */
static icode_t I_pdone3;              /* Calls pterminator2() */
static icode_t I_pdone4;              /* Calls done_pthr() */
#endif /* DEAD CODE */



void init_all_queues();
void init_thread(thread_ptr pthread, event_ptr pevent);
void init_event(thread_ptr pthread, event_ptr pevent);
void compute_paddr(inflight_inst_ptr ifi, icode_ptr picode, thread_ptr pthread);
void convert_register_value(inflight_inst_ptr ifi, icode_ptr picode, thread_ptr pthread, int regIdx);


int  num_outstanding_instrs;

void 
init_all_queues()
{
  int pid, tid,i;
  Physical_RegNum pr;

  num_outstanding_instrs=0;

  /* init the queue head nodes */
  INLINE_INIT_Q(&Run_q);
  INLINE_INIT_Q(&Done_q);
  INLINE_INIT_Q(&Free_q);
  INLINE_INIT_Q(&Sleep_q);

  INLINE_INIT_Q(&Free_Process_q);
  INLINE_INIT_Q(&Run_Process_q);

  /* Allocate the array of threads */
  Threads = (thread_ptr) calloc(Max_nthreads, sizeof(thread_t));
  if (Threads == NULL) {
    fatal("init_all_queues: cannot allocate 0x%x bytes for threads.\n",
	  Max_nthreads * sizeof(thread_t));
  }

  Processes = (process_ptr) calloc(Max_nprocs, sizeof(process_t));
  if (Processes == NULL) {
      fatal("init_all_queues: cannot allocate 0x%xbytes for processes.\n",
	    Max_nprocs * sizeof(process_t));
  }

  for (pid = 0; pid < Max_nprocs; pid++) {
    Processes[pid].pid = pid;
    INLINE_ENQUEUE(&Free_Process_q, &Processes[pid]);
    Processes[pid].fd = (int *) malloc(MAX_FDNUM * sizeof(int));
    if (Processes[pid].fd == NULL)
      fatal("init_all_queues: cannot allocate 0x%x bytes for thread fds.\n",
	    MAX_FDNUM * sizeof(char));

    {
      int i;
      Processes[pid].num_pages = 0;
      Processes[pid].num_private = 0;
      for (i=0; i<TB_SIZE; i++) Processes[pid].TB[i] = NULL;
    }
    Processes[pid].Shmem_regions = (struct Shm_ds *) NULL;
    Processes[pid].Unsp_Shmat_Current = UNSP_SHMAT_START;

    Processes[pid].sigv = (struct my_sigvec *) malloc(MAX_SIGNALS * 
						    sizeof(struct my_sigvec));
    Processes[pid].signal_mask = 0x0; /* all unblocked */
    Processes[pid].spec_segv_action = SPEC_ACTION_RAISE; /* not blocked */
    Processes[pid].spec_fpe_action = SPEC_ACTION_RAISE;  /* not blocked */

    if (Processes[pid].sigv == NULL)
      fatal("init_all_queues: cannot allocate 0x%x bytes for thread sigv.\n",
	    MAX_SIGNALS * sizeof(struct my_sigvec));
									 
    Processes[pid].segments = (segment_t *) NULL;

  }

  for (tid = 0; tid < Max_nthreads; tid++) {
    thread_ptr pthread = &Threads[tid];
    Logical_RegNum lreg;
    Physical_RegNum preg;

    pthread->tid = tid;
    pthread->process = &Processes[tid];
    
    /* Threads[pid].addr_space = 0; */

    INLINE_ENQUEUE(&Free_q, pthread);

    /* 
     * Initialize the SCALAR register map: integer, FP and vector control.
     * Be careful to skip the $f31 zero register (notice there was a bug
     * in the previous code, since PHYSICAL 63 would have a valid bit set
     * when, in reality, it was not mapped; I don't think this problem would
     * show up in any way, though...)
     */
    for (lreg = 0; lreg < TOTAL_SCALAR_LOGICALS; lreg++) {
      if ( lreg == FP_ZERO_REGISTER ) continue;
      pr = (Physical_RegNum) lreg;
      pthread->RegMap[lreg] = pr;
      pthread->RegValid[pr >> 5] |= (1 << (pr&0x1f));
    }
    pthread->RegMap[FP_ZERO_REGISTER] = ZERO_REGISTER;
    
    /* 
     * Build the scalar free list (remebering to add to it the FP_ZERO_REGISTER
     * that we skipped in the previous loop!)
     */
    pthread->FirstFreePhysicalRegister = -1;
    for (preg= TOTAL_SCALAR_PHYSICALS - 1; preg >= TOTAL_SCALAR_LOGICALS ; preg--) {
      pthread->Reg[preg].Int64 = pthread->FirstFreePhysicalRegister;
      pthread->FirstFreePhysicalRegister = preg;
    }
    pthread->Reg[FP_ZERO_REGISTER].Int64 = pthread->FirstFreePhysicalRegister;
    pthread->FirstFreePhysicalRegister = FP_ZERO_REGISTER;

    preg = pthread->FirstFreePhysicalRegister;
    if (!Quiet)
	fprintf(Aint_output,"Free list = "); 
    for ( i = 0; i < 20; i++ ) {
     if (!Quiet) 
	 fprintf(Aint_output, "%ld,",pthread->FirstFreePhysicalRegister); 
     pthread->FirstFreePhysicalRegister = pthread->Reg[pthread->FirstFreePhysicalRegister].Int64;
    }
    /* fprintf(Aint_output,"\n"); */
    pthread->FirstFreePhysicalRegister = preg;

    /*
     * Now initialize the vector register map
     */
    for (lreg = FIRST_VEC_LOGICAL; lreg < TOTAL_LOGICALS; lreg++) {
     pr = (Physical_RegNum) ((lreg - FIRST_VEC_LOGICAL) * MAX_VECTOR_LENGTH + TOTAL_SCALAR_PHYSICALS);
     pthread->RegMap[lreg] = pr;
     pthread->RegValid[pr >> 5] |= (1 << (pr&0x1f));
    }

    /* 
     * Build the vector free list. Note that the last physical register in use can be easily
     * computed from 'pr'... Just add to it the MAX_VECTOR_LENGTH
     */
    pthread->FirstFreeVectorPhysicalRegister = -1;
    for ( preg = (pr + MAX_VECTOR_LENGTH); preg < TOTAL_PHYSICALS; preg += MAX_VECTOR_LENGTH ) {
     pthread->Reg[preg].Int64 = pthread->FirstFreeVectorPhysicalRegister;
     pthread->FirstFreeVectorPhysicalRegister = preg;
    }
  }

}


void init_process(process_ptr process)
{

  int i;

  process->child_cpu = 0;
  process->process_time = 0;
  process->process_cpu = 0;

  process->is_zombie = 0;
  process->runstate = R_RUN;

  process->youngest = NULL;

  process->thread_count = 0;

  INLINE_ENQUEUE(&Run_Process_q, process);

  /* initialize all file descriptors to "closed" */
  for (i=0; i<MAX_FDNUM; i++)  
    process->fd[i] = -1;

  /* Set stdin, stdout, stderr to default values */
  process->fd[0] = 0; process->fd[1] = 1; process->fd[2] = 2;

}


void
init_thread(thread_ptr pthread, event_ptr pevent)
{

#if 0  
  pthread->reg[ZERO_REGISTER] = 0;
  pthread->fp[ZERO_REGISTER] = 0;
#endif

  pthread->FP = pthread->Reg;

  MapReg(ZERO_REGISTER) = 0;
  MapFP(ZERO_REGISTER) = 0;
  /* pthread->FP[ZERO_REGISTER] = 0; */

  pthread->cpu_time = 0;
  pthread->terrno = 0;
  pthread->ufunc = NULL;
  /* pthread->stall_addr = NULL; */
  pthread->runstate = R_RUN;

  pthread->wthread = NULL;  /* There is initially no thread waiting to join with us */

  pthread->instid = -1;

  /* Use calloc so we dont have to initialize all ifi->picode and ifi->paddr to NULL */
  pthread->cbif = (inflight_inst_ptr) calloc(1, sizeof(cbif_t));
  if (pthread->cbif == NULL) fatal("init_thread: cannot allocate inflight inst circular buffer (%d bytes)\n", sizeof(cbif_t));

  /* Initialize the store queue */
  pthread->StoreQ.next = pthread->StoreQ.prev = &(pthread->StoreQ);
  pthread->StoreQ.addr = 0x0;

  pthread->StoreQ.Updates.next = pthread->StoreQ.Updates.prev = NULL; /* unused */

  pthread->fetch_count = 0;
  /* put the thread on the run queue */
  INLINE_ENQUEUE(&Run_q, pthread);

  /* map in the global errno */
  pthread->perrno = &pthread->terrno;

  /* initialize the event structure associated with this thread */
  init_event(pthread, pevent);

  /* pthread->psave = NULL; */
  pthread->sigpending = 0;

}

void 
init_main_thread(thread_ptr pthread)
{
  int i;
  event_ptr pevent;
  process_ptr process;

  Nprocs++;
  Maxpid = Nprocs-1;

  Nthreads++;
  Maxtid = Nthreads-1;

  process = pthread->process;

  init_process(process);

  process->thread_count = 1;

  pthread->process = process;
  process->threads = process->youngest_thread = pthread;
  pthread->tsibling = pthread->wsibling = NULL;

  NEW_ITEM(Event_free, sizeof(event_t), pevent, "init_main_thread");

  init_thread(pthread, pevent);
  /* pthread->calldepth = 0; */

  /* init the signal handler vectors */
  for (i=0; i<MAX_SIGNALS; i++) {
    process->sigv[i].sv_handler = (sig_handler_t) SIG_DFL;
    process->sigv[i].sv_mask = 0;
    process->sigv[i].sv_flags = 0;
  }
  pthread->sigblocked = 0;

  pthread->time = 0;

  process->parent = NULL;
  process->sibling = NULL;

  pthread->fpcr = FPCR_DYN_PLUS;

#if 0
  pthread->reg[ZERO_REGISTER] = 0;
  pthread->fp[ZERO_REGISTER] = 0;

  pthread->Reg[ZERO_REGISTER] = 0;
  pthread->FP[ZERO_REGISTER] = 0;
#endif

  /* set its picode to the first executable instruction */
  pthread->next_fetch_picode = addr2iphys(process, process->entry_point, NULL);
}

void init_event(thread_ptr pthread, event_ptr pevent)
{
  pthread->pevent = pevent;
  pevent->cpu_time = &pthread->cpu_time;
  pevent->tid = pthread->tid;
  pevent->next = NULL;
}


/* For debug purposes: display values in mapped regs */
void dump_regs(thread_ptr pthread)
{
  Logical_RegNum r;
  printf("{");
  for (r=0; r< TOTAL_LOGICALS; r++) {
    printf("%ld ", MapReg(r));
  }
  printf("\n");
}


/* Allocates a new instid entry, and marks it with a non-zero picode */
#ifndef __GNUC__
#pragma inline (new_instid)
#else
inline
#endif
instid_t new_instid(thread_ptr pthread, icode_ptr picode) {
  instid_t instid = SUCC_CIRC(pthread->instid, CBIF_SIZE);
  inflight_inst_t *ifi = &pthread->cbif[instid];
  if (ifi->picode) {
    /* We have bit our own tail! */
    fatal("new_instid: buffer overflow \n\t"
	  "{tid=%d pc=%lx instr=%x extinstr=%lx opc=%s \n\t"
	  " instid=%ld fetchnum=%d type=0x%x issued=%d"
	  " committed=%d done_memop=%d trapped=%d}"
	  " at cycle %ld\n", 
	  pthread->tid, 
	  ifi->picode->addr, ifi->picode->instr, ifi->picode->extended_instr, desc_table[ifi->picode->opnum].opname,
	  instid, ifi->fetchnum, ifi->type,
	  (int)ifi->issued,(int)ifi->committed, (int)ifi->done_memop, (int)ifi->trap_detected,
	  (ulong)pthread->time);
  }
  /* All we need for now */
  pthread->instid = instid; /* INCR_INSTID has this side-effect already - NOT! */

  return instid;
}

void print_reg_state(int tid);

#define CHECK_VREG_FREE(a)    \
          ASSERT(pthread->FirstFreeVectorPhysicalRegister == -1 ||     \
                 (pthread->FirstFreeVectorPhysicalRegister >= FIRST_VECTOR_PHYSICAL &&         \
                  pthread->FirstFreeVectorPhysicalRegister < TOTAL_PHYSICALS &&        \
                  (((pthread->FirstFreeVectorPhysicalRegister - FIRST_VECTOR_PHYSICAL) % MAX_VECTOR_LENGTH) == 0)),    \
                    a ": FirstFreeVectorPhysicalRegister %d is out of bounds\n",pthread->FirstFreeVectorPhysicalRegister);

#define CHECK_REG_FREE(a)     \
         ASSERT(pthread->FirstFreePhysicalRegister == -1 ||   \
                (pthread->FirstFreePhysicalRegister >= 0 &&   \
                 pthread->FirstFreePhysicalRegister < FIRST_VECTOR_PHYSICAL), \
                   a ": FirstFreePhysicalRegister %d is out of bounds %d\n",pthread->FirstFreePhysicalRegister);


#ifdef __GNUC__
inline
#else
#pragma inline(ReMap)
#endif
static Physical_RegNum ReMap(thread_ptr pthread, Logical_RegNum reg)
{
  Physical_RegNum pr, *free;
  /* Map zero register to zero */
  if ((reg == ZERO_REGISTER) || (reg == FP_ZERO_REGISTER)) {
    return ZERO_REGISTER;
  }

  /*
   * Distinguish between free lists, depending on whether this is a
   * vector or a scalar register
   */
  free = (reg >= FIRST_VEC_LOGICAL) ? 
		(&pthread->FirstFreeVectorPhysicalRegister) : 
		(&pthread->FirstFreePhysicalRegister);

  pr = *free;
  if (pr != -1) {
    *free = pthread->Reg[pr].Int64;
    if (debug_register_renaming) {
      informative("\nReMap: tid=%d lr=%d opr=%d npr=%d\n", 
		  pthread->tid, reg, pthread->RegMap[reg], pr);
      print_reg_state(pthread->tid);
    }
    pthread->RegMap[reg] = pr;
    pthread->RegValid[pr >> 5] &= ~(1 << (pr&0x1f));
    return pr;
  } else {
    fatal("ReMap: cannot remap register in thread %d\n", pthread->tid);
    return -1;
  }
}


void print_reg_state(int tid){

  thread_ptr pthread = &(Threads[tid]);
  int lr,preg;
  int num_in_use=0;

  informative("Current reg map:  ");
  for (lr = 0; lr < TOTAL_LOGICALS; lr++) {
    int pr = pthread->RegMap[lr];
    if ((lr%4)==0) informative("\n  ");
    informative("%2d/p%02d=%s%lx    ", 
		lr, pr, 
		((pthread->RegValid[pr >> 5] & (1 << (pr&0x1f))) ? "V" : "I"),
		pthread->Reg[pr]);
  }
  informative("\n");
}

void check_reg_state(int tid, char *str){

  thread_ptr pthread = &(Threads[tid]);
  int lr,preg;
  int num_in_use=0;

  num_in_use = TOTAL_PHYSICALS;
  for (preg = pthread->FirstFreePhysicalRegister; preg >= 0; preg = pthread->Reg[preg].Int64)
    num_in_use--;
  if (num_in_use>(num_outstanding_instrs+TOTAL_LOGICALS))
    warning("Num regs in use:%d  num_out_instrs=%d (%s)", num_in_use,num_outstanding_instrs,str); 
}

/* LDL STC 
 *
 * ifi->stc_success is used to keep track of which store_conditional's succeed.
 *  In this implementation, the PM determines success/failure
 *
 *  - when an stc is fetched stc_success is set to 1.
 *  - when an stc is issued, if stc_success is still 1 , it is assumed the stc will succeed.
 *  - If PMINT/AINT_stc_fail is called, stc_success is set to 0
 *
 *
 *  at issue time (if stc_success)
 *    - update store buffer (as in normal write)
 *    - set store data (as in normal write)
 *    - set result reg to 1
 *
 *  at issue time (if !stc_success)
 *    - set result reg to 0
 *    - clear out store info (?needed?)
 *
 *  at fail time
 *    - set success to be 0
 *    - remove it from the store buffer (if it's there)
 *    - (If stc has been issued, PM should this and all following instrs)
 *    
 *  at commit time
 *    - write mem if it succeeded
 *
 */

void AINT_stc_failed(int tid, instid_t instid){
  thread_ptr pthread = &Threads[tid];
  inflight_inst_ptr ifi = &pthread->cbif[instid];
    
  ifi->stc_success=0;
  Cleanup_StoreBuffer(pthread, ifi, instid);
}


icode_ptr AINT_prefetch_instr(int tid, ulong predicted_pc)
{
  thread_ptr pthread = &Threads[tid];
  icode_ptr picode = pthread->next_fetch_picode;
  long flags;

  /* save a call to addr2iphys() if we can */
  if (   picode && picode->addr == predicted_pc) {
  }
  else {
    picode = addr2iphys(pthread->process, predicted_pc, &flags);
  }

  return picode;
}

instid_t AINT_fetch_next_instr(int tid, ulong predicted_pc)
{
  thread_ptr pthread = &Threads[tid];
  icode_ptr picode = pthread->next_fetch_picode;
  instid_t instid;
  inflight_inst_ptr ifi;
  long flags;


  num_outstanding_instrs++;


  /* picode = pthread->next_fetch_picode; */
  /* save a call to addr2iphys() if we can */
  if (   picode && picode->addr == predicted_pc) {
  }
  else {
    picode = addr2iphys(pthread->process, predicted_pc, &flags);
  }
  if(picode == 0) {
#if 0
    if(!pthread->next_fetch_picode)
      picode = pthread->next_fetch_picode;
    else
#endif
      return -1;
  }
  pthread->next_fetch_picode = picode->next;

  instid = new_instid(pthread, picode);
  ifi = &pthread->cbif[instid];

  /* zero fields out to avoid errors: */
  bzero((void*)ifi, sizeof(struct inflight_inst));

  ifi->pc = picode->addr;
  ifi->picode = picode;
  ifi->fetchnum = pthread->fetch_count++;
  ifi->type = picode->iflags;

  /* Assume st_c's will succeed. */
  if (ifi->picode->iflags & E_ST_C)
    ifi->stc_success=1;
    
#if EXEC_TRACE
  informative("Aint_fetch: p%d.%d 0x%lx: %08x\n", pthread->process->pid, tid, picode->addr, picode_instr(pthread, picode));
#endif
  /* do register renaming */

#ifdef DO_RENAMING
  {
    Physical_RegNum *ifi_args = ifi->args;
    Physical_RegNum *regmap = pthread->RegMap;
    Logical_RegNum *picode_args = picode->args;

    ifi_args[RA] = regmap[picode_args[RA]];
    ifi_args[RB] = regmap[picode_args[RB]];
    ifi_args[RC] = regmap[picode_args[RC]];
#ifdef NEW_TTL
    ifi->vl      = regmap[VL_REG];
    ifi->vs      = regmap[VS_REG];
    ifi->vm      = regmap[VM_REG];
#endif

    if (picode->dest < MaxArgs) {
      Arg picode_dest = picode->dest;

      /* This instruction writes a register. Rename that register */
      ifi->OldDest_Phys = ifi_args[picode_dest];
	  
	  /* equivalent? */
	  /*ifi->OldDest_Phys = regmap[picode_args[picode_dest]];*/

      ifi_args[picode_dest] = ReMap(pthread,  picode_args[picode_dest]);
	  
      if (debug_register_renaming)
	informative("AINT_fetch_next_inst: (predpc=0x%lx) picode->addr=0x%lx, tid=%d instid=%d defining dstnum=%d/p%02d (was %d/p%02d)\n",
		    predicted_pc, picode->addr,
		    tid, instid, 
		    picode_args[picode_dest], ifi_args[picode_dest],
		    picode_args[picode_dest], ifi->OldDest_Phys);

    } else {
      ifi->OldDest_Phys = ZERO_REGISTER; /* This is never released */
    }
  }
#endif
  return instid;
}

#ifndef CHECK_ORDER_TRAPS
#define CHECK_ORDER_TRAPS 0
#endif


void AINT_do_spec_write(int tid, instid_t instid)
{
  thread_ptr pthread = &(Threads[tid]);
  inflight_inst_ptr ifi = &(pthread->cbif[instid]);
  int i,vl,vs,needs_cleanup;
  int isAScatter;
  int regIdx;


  ifi->done_memop = 0;
  isAScatter = (ifi->picode->iflags & E_SCATTER);

  /* 
   * The address for load/store has already been computed in the past and is
   * kept in ifi.
   */
  if (0) informative("AINT_do_spec_write: tid=%d, instid=%d, ea=%lx\n", tid, instid, pthread->vaddr);

  /*
   * If we are here, WE MUST BE DEALING with a store
   */
  if ( ! (ifi->picode->iflags & (E_WRITE|E_ST_C))) {
   fatal("AINT_do_spec_write: not working on a store!");
  }

  /* 
   * roger 30/Aug/99
   *
   * For stores, however, we need to update the store buffer using ifi->data and ifi->size.
   * For vector stores, we have to repearedly call Update_StoreBuffer for each and every piece
   * of data being written to memory.
   *
   * The first complication, however, is that we might have been doing repeated AINT_do_spec_write()
   * calls for the same store (due to speculation on the performance model side). Thus, before
   * creating any new state in the StoreQueue we must check that this particular store
   * does not already have state associated with it. If it does, then clean it up before 
   * creating new stuff. To be more precise, the cleaning up should only happen if the vaddr has 
   * changed from the previous time we invoked AINT_do_spec_write(). 
   *
   * We start with the Scalar case:
   */


  needs_cleanup = (ifi->picode->is_vector == 0) && (ifi->stq_update != NULL) && 
		  ( (ifi->stq_update->vaddr != ifi->vaddr) || (ifi->stq_update->data != ifi->data) );
   
  /* 
   * Vector case: Notice how we don't bother optimizing away the case were we 
   * are called twice for the same vector store but the <base,vl,stride> have NOT
   * changed. It's too much work and it's safer to just always clean up before
   * working on a vector store.
   */ 
  needs_cleanup |= ifi->picode->is_vector && ifi->stq_update_array != NULL;

  /*
   * Do the cleanup: This is a little bit tricky, because 'Cleanup_StoreBuffer' uses 'ifi->vaddr' to
   * do its work. Thus, we can not simply invoke it with the current ifi->vaddr, because that one is
   * the new vaddr we want to work on. We must temporarily save the new vaddr, set 'ifi' to point to
   * the old 'vaddr' and then restore vaddr back.
   */
  if ( needs_cleanup ) {
   /* 
    * Save current vaddr 
    */
   ulong newvaddr = ifi->vaddr;

   /* 
    * Put back the old addr so that Cleanup can do its work on the old addr (this line only
    * executed for scalar stores
    */
   if ( ifi->stq_update != NULL ) ifi->vaddr = ifi->stq_update->vaddr;

   /* 
    * Do Cleanup 
    */
   Cleanup_StoreBuffer(pthread, ifi, instid);

   /* 
    * Restore the proper vaddr 
    */
   ifi->vaddr = newvaddr;
  }

  if ( ifi->picode->is_vector == 0 ) {
   if ( ifi->picode->iflags & E_ST_C ) {
    /* 
     * Store conditionals write the store buffer if they 
     * have not yet failed.  
     */
     if ( ifi->stc_success ) Update_StoreBuffer(pthread, ifi);
   }
   else {
    /*
     * Normal stores go here.  Simply call the function that does the work
     */
    Update_StoreBuffer(pthread, ifi);
   }
  }
  else {
   /*
    * Vector stores go here. Start by computing
    * appropriate vector length
    */
   vl = VL; 
   vs = VS; 

   /*
    * Allocate a vector of pointers to stq_update entries (see explanation below)
    */
   ifi->stq_update_array = (struct stq_entry **) malloc(vl * sizeof(struct stq_entry *));
#ifdef NEW_TTL
   informative_ttl(10,"allocating stq_update_array =%lx\n",ifi->stq_update_array);
#endif
   if ( ifi->stq_update_array == NULL ) 
     fatal("AINT_do_spec_write: cannot allocate %d bytes for stq_update_array.\n", vl * sizeof(struct stq_entry *));

   if (isAScatter)
   	regIdx = RC;
   else
   	regIdx = RA;
		
   for ( i = 0; i < vl; i++ ) {

    /*
     * 1.- We need to set this field here for proper debugging of allocation/deallocation of the StoreBuffer
     * structures. Certainly, you could move this statement down to (3) below and eveything would be fine.
     */
    ifi->pos_in_vreg = i;

	/* before we compute physical address we must to verify if we are dealing with a scatter */
	if (isAScatter)
	{
		ifi->vaddr = VELEMINT (RA,i) + REG(RB);
	}
	
    /*
     * 2.- recompute the paddr. This function call ONLY uses ifi->vaddr as input. Note that the call
     *     to AINT_issue_other above has already computed this for position 0. Therefore, we might
     *      as well skip this when i == 0. 
     */
    compute_paddr(ifi, ifi->picode, pthread);

    /*
     * 3.- increment the counter indicating which position inside the vector register we want to store
     *     and convert the register value into acceptable memory format. Note that convert_register_value
     *     detects that we are dealing with a vector instruction and uses 'pos_in_vreg' accordingly.
     */
	 
    convert_register_value(ifi, ifi->picode, pthread,regIdx);

    /*
     * 4.- Do the real work :-)
     */
    Update_StoreBuffer(pthread, ifi);

    /*
     * 5.- Keep a copy of the ifi->stq_update pointer and delete it from 'ifi'. This is necessary for
     *     the following reasons:
     *
     *     (a) In pure-scalar-aint, every 'ifi' was supposed to update the store buffer only once. If,
     *		a certain 'ifi' happened to enter the Udate_StoreBuffer code more than once, a safety check 
     *		would be triggered and nothing (bad) would happen. Note that this situation might be fairly
     *		common due to the fact that the performance model may do multiple calls	to AINT_do_spec_write 
     *         for the same 'ifi', 
     *
     *	    (b) In pure-scalar-aint, an 'ifi' only writes AT MOST ONE QUADWORD to memory. Therefore, just one
     *		stq_update pointer is needed per 'ifi' (this pointer is used later in CleanupStoreBuffer, whenever
     *		the store happens to commit, to release the previously malloc'ed stq_entry).
     *       
     *    For vector stores, we need several changes. First, we want to call Update_StoreBuffer repeatedly, yet
     *    we want each of this calls to behave normally, that is, to create its own stq_entry structure and
     *    store it in the store buffer. Second, we need some way to link together all the different stq_entry
     *    structures related to the SAME 'ifi'. Therefore, we store the current ifi->stq_update pointer
     *    in the array of pointers and then clear it to get a new stq_entry structure in the next loop iteration
     */
    ifi->stq_update_array[ifi->pos_in_vreg] = ifi->stq_update;
    ifi->stq_update = NULL;

    /*
     * 6.- increment the vaddr by the stride, to prepare for next iteration
     */
    if (!isAScatter) 
	{
		ifi->vaddr += vs;
	}

   }
  }

#if CHECK_ORDER_TRAPS
    { 
      instid_t j;
      int needs_reissue = 0;
      long vaddrbase = (ifi->vaddr & ~7L);
      long bytemask = ((1 << ifi->picode->size) - 1) << (ifi->vaddr & 7);
      for (j = SUCC_CIRC(instid, CBIF_SIZE); pthread->cbif[j].picode; 
	   j = SUCC_CIRC(j, CBIF_SIZE)) {
	inflight_inst_ptr jifi = &pthread->cbif[j];
	long jiflags = jifi->picode->iflags;
	if (jifi->done_memop && (jiflags & (E_READ|E_LD_L)) && !(jiflags & E_PREFETCH)) {
	  long jvaddrbase = (jifi->vaddr & ~7L);
	  long jbytemask = ((1 << jifi->picode->size) - 1) << (jifi->vaddr & 7);
	  if (   (jvaddrbase == vaddrbase)
	      && (jbytemask & bytemask) != 0) {
	    if (0)
	      informative("AINT_do_spec_write: tid=%d \n\t"
			  "writer: instid=%ld pc=%lx eaw=%lx|%dB \n\t"
			  "reader: instid=%ld pc=%lx ear=%lx|%dB needs redo\n",
			  pthread->tid, 
			  instid, ifi->picode->addr, 
			  ifi->vaddr, ifi->picode->size,
			  IFI_INSTID(jifi, pthread), jifi->picode->addr,
			  jifi->vaddr, jifi->picode->size);

	    needs_reissue = 1;
	    jifi->done_memop = 0;
	  }
	}
	if (needs_reissue && jifi->issued) {
	  jifi->trap_detected = TR_memop_order_trap;
	  if (0) fprintf(Aint_output, " %d", j);
	}
      }
      if (needs_reissue)
	if (0) fprintf(Aint_output, "\n");
    }
#endif /* CHECK_ORDER_TRAPS */


  ifi->done_spec_write = 1;

}

ulong AINT_issue_branch(int tid, instid_t instid)
{
  thread_ptr pthread = &(Threads[tid]);
  inflight_inst_ptr ifi = &(pthread->cbif[instid]);

  AINT_issue_other(tid, instid);
  return ifi->nextpc;
}


static jmp_buf issue_restart_jmp_buf;

#if !defined (_XOPEN_SOURCE_EXTENDED) || defined (_OSF_SOURCE)
#define SIGCONTEXT struct sigcontext
#else
#define SIGCONTEXT struct _sigcontext
#endif

extern double get_fpcr();

/*
 * Roger:
 * The following is a global variables used to pass information from a
 * generated signal to the AINT_issue() subroutine, so that we can store
 * the exact siginfo code and the exact faulting pc inside the IFI and 
 * print it out if the user does a FEED_Commit on the faulting instruction.
 */
siginfo_t siginfo;

void dump_siginfo(char *p,int n)
{
 int i,j;
 siginfo_t *s;

 s = (siginfo_t *)p;
 printf("------ DUMP -------\n");
 for ( i = 0; i < n / 4; i++ ) {
  printf("@0x%p :",p);
  for (j = 0; j < 4; j++ ) {
   printf("0x%8x,",(unsigned int)*p);
   p++;
  }
  printf("\n");
 }
 printf("-------AGAIN, INTERPRETING THE FIELDS:\n");
 printf("si_signo = %d @ (0x%lx, size=%d)\n",s->si_signo,&(s->si_signo),sizeof(s->si_signo));
 printf("si_errno = %d @ (0x%lx, size=%d)\n",s->si_errno,&(s->si_errno),sizeof(s->si_errno));
 printf("si_code  = %d @ (0x%lx, size=%d)\n",s->si_code,&(s->si_code),sizeof(s->si_code));
 printf("si_addr  = %p @ (0x%lx, size=%d)\n",s->si_addr,&(s->si_addr),sizeof(s->si_addr));
}

int handle_signals_for_issue(int sig, void *vinfo, void *vctxt)
{
  siginfo_t *info = (siginfo_t *)vinfo;
  SIGCONTEXT *context = (SIGCONTEXT *)vctxt;
  int sig_ignore;

  if (AINT_in_issue) {
    sig_ignore = FALSE;
    if (sig == SIGSEGV || sig == SIGBUS) {
      switch(AINT_issue_process->spec_segv_action) {
        case(SPEC_ACTION_RAISE):
          break;
        case(SPEC_ACTION_IGNORE):
          sig_ignore = TRUE;
          break;
        case(SPEC_ACTION_CALL_HANDLER):
          fprintf(Aint_output, "AINT: SPEC_ACTION_CALL_HANDLER not supported\n");
          break;
        default:
          fprintf(Aint_output, "AINT: spec_segv_action (%d) unknown type\n",
            AINT_issue_process->spec_segv_action);
      }
    } else if (sig == SIGFPE) {
      switch(AINT_issue_process->spec_fpe_action) {
        case(SPEC_ACTION_RAISE):
          break;
        case(SPEC_ACTION_IGNORE):
          sig_ignore = TRUE;
          break;
        case(SPEC_ACTION_CALL_HANDLER):
          fprintf(Aint_output, "AINT: SPEC_ACTION_CALL_HANDLER not supported\n");
          break;
        default:
          fprintf(Aint_output, "AINT: spec_fpe_action (%d) unknown type\n",
            AINT_issue_process->spec_segv_action);
      }
    }

    if (Verbose) {
      long ieee_fp_control;
      long ieee_state_at_signal[3];
      getsysinfo(GSI_IEEE_FP_CONTROL, (caddr_t)&ieee_fp_control, sizeof(ieee_fp_control), 0, 0, 0);
      getsysinfo(GSI_IEEE_STATE_AT_SIGNAL, 
		 (caddr_t)ieee_state_at_signal, sizeof(ieee_state_at_signal), 0, 0, 0);

      fprintf(Aint_output, "handle_signals_for_issue!\n");
      fprintf(Aint_output, "    - ieee_fp_control = %lx\n", ieee_fp_control);
      fprintf(Aint_output, "    - ieee_state_at_signal = [%lx, %lx, %lx]\n", 
	      ieee_state_at_signal[0],
	      ieee_state_at_signal[1],
	      ieee_state_at_signal[2]);
      fprintf(Aint_output, "    - fpcr=%lx\n", get_fpcr());

      if (sig_ignore) {
        fprintf(Aint_output, "    - ignoring signal by set_speculative() rule\n");
      }
    }

    if (sig_ignore) {
      /* do not raise the signal to AINT */
      return 0;
    }

    /*
     * Copy the signal information so that AINT_issue can record the specific signal code
     * inside the ifi
     */
    siginfo = *info;

    /* 
     * Roger: I got really confused here... On Oct/25/1999, on a OSF1 V4.0F machine, the OS
     * seems to be returning an incorrect layout for the siginfo_t data structure. That is,
     * in the 'addr' field there is a 0... and 60 bytes later (still inside siginfo) we get
     * the real address of the faulting instruction. This is driving me crazy, so the solution
     * will be as follows: The SIGCONTEXT structure happens to also contain the faulting instruction
     * pc. Thus I will manually fix (oh god! :-)) the siginfo variable "as if" the OS had
     * returned the right thing. 
     */
    siginfo.si_addr = (void*)(context->__XSE(sc_pc));

    /*
     * You can use the dump_siginfo routine above to see if you can fix this horrible kludge!
     *
     * dump_siginfo(info,sizeof(siginfo_t));
     * dump_siginfo(&siginfo,sizeof(siginfo_t));
     * printf("sc_pc = %p\n",context->__XSE(sc_pc));
     * printf("sc_fp_trap_pc = %p\n",context->__XSE(sc_fp_trap_pc));
     * printf("sc_fp_trigger_inst = %p\n",context->__XSE(sc_fp_trigger_inst));
     */

    /*
     * Return to AINT_issue with a longjmp
     */
    longjmp(issue_restart_jmp_buf, sig);
  } else {
    /* oops -- simulator itself caught a signal -- fatal! */
    fatal("handle_signals_for_issue: caught signal %d si_errno=%d si_code=%d at pc=%lx ra=%lx si_addr=%lx\n", 
	  sig, info->si_errno, info->si_code,
	  context->sc_pc, context->sc_regs[26],
	  info->si_addr);
  }
  return 0;
}

#ifdef E_WAS_LDQ_U
long was_ldq_u_count = 0;
#endif /* E_WAS_LDQ_U */

void AINT_issue_other(int tid, instid_t instid)	/* in other words,, issue a non branch instruction */
{
  thread_ptr pthread = &Threads[tid];
  inflight_inst_ptr ifi = &(pthread->cbif[instid]);
  icode_ptr picode = ifi->picode;
  icode_ptr new_picode;
  int sig;

  AINT_in_issue = 1;
  AINT_issue_process = pthread->process;
  ifi->done_memop = 0;
  ifi->done_spec_write = 0;
  if ((sig = setjmp(issue_restart_jmp_buf)) == 0) {

    if (picode) {
      ifi->trap_detected = 0;

      if(0)
      informative("AINT_issue: tid=%d, instid=%d, opc=%s R(29)=%lx\n", tid, instid, desc_table[picode->opnum],REG(29));

      /* This is the time to copy the old destination register value into the new one */
#ifdef DO_RENAMING
      { 
	int argnum;
	for (argnum = 0; argnum < MaxArgs; argnum++) {
	  int pr = ifi->args[argnum];
	  if (argnum == picode->dest) continue;
	  /*
	   * roger 13/10/2000 ADDENDUM
	   * 
	   * Yet another special case appears with gather/scatters: They look like arithmetic operations
	   * in that they have three register specifiers Ra, Rb, Rc. The use of the specifiers is
	   * as follows:
	   *
	   *           |  Ra   |  Rb   |  Rc       |
	   *           +-------+-------+-----------+
	   *  gather   | @vect | @scal | Dest vect |
	   *           +-------+-------+-----------+
	   *  scatter  | @vect | @scal | Data vect |
	   *           +-------+-------+-----------+
	   *
	   * Here in AINT_issue_other, for gat/scat instructions,  we should ONLY check for the validity of RB because:
	   *
	   *  - for gather, RC is a destination anyway. RA should not be checked either because it will
	   *    be first used when we get to AINT_DoRead() and thus, checking its validity now is premature.
	   *
	   *  - for scatter, RC will be checked when we get to AINT_DoSpecWrite(), which is the place
	   *    where we first use (vector) register RC containing the data to be sent to memory. Similarly
	   *    for RA: it will be used to compute addresses in AINT_DoSpecWrite() ! So its pointless to
	   *    look at its validity now.
	   */
          if ( (picode->iflags & E_GATSCAT) && argnum != RB ) continue;

	  /* 
	   * roger 9/9/99 
	   * An exception must be made when checking for validity of registers. For store instructions,
	   * the source DATA register (Ra) should not be checked for validity. You do have to check, however,
	   * the validity of the base address register. This allows executing code on a processor that
	   * uses the idea of split-stores: first issue the address part (as soon as the address register
	   * is ready) and, some time later, issue the data part (when the data becomes ready and 
	   * environment conditions allow the store to proceed).
	   *
	   * The validity of the source DATA register has to be checked inside AINT_do_write(), which is the
	   * point where the register is really being used/read.
	   * This applies to scalar stores, and we have to guard it against confusion with SCATTERS
	   */
          if ( (picode->iflags & E_WRITE) && !(picode->iflags & E_SCATTER) && argnum == RA ) continue;
	
	
		/*
		    Fede 17/04/2001:
			the instruction mvfvp must not be checked here in the standard way, the TTL section
			bellow will take care of this.
		*/
#ifdef NEW_TTL		
		if (picode->is_mvfvp) continue;
#endif		
				
	  if ( ISREGVALID(pr) == 0 ) {
	    ifi->trap_detected = TR_invalid_register_trap;
	    ifi->trap_info = argnum;
	  }
	}
      }
#ifdef NEW_TTL
      /* 
       * For vector instructions, check VL/VS/VM where appropriate; i.e. VL must be checked always, VS for vector
       * operations and VM for operations under mask (to be done...)
       */
      if ( picode->is_vector ) {

       /*
        * Start Checking VL for every vector instruction
        */
       if ( ISREGVALID(ifi->vl) == 0 ) {
        ifi->trap_detected = TR_invalid_register_trap;
        ifi->trap_info = 4;
       }

       /*
	    * checking VS only for vector memory instructions
        */
       if ( (picode->iflags & (E_READ|E_WRITE)) && ISREGVALID(ifi->vs) == 0 ) {
        ifi->trap_detected = TR_invalid_register_trap;
        ifi->trap_info = 5;
       }

       /*
        * Check for VM 
		* fede: only when doing a mvfvp !
        */
		if ((picode->is_mvfvp) && (ISREGVALID(ifi->vm)==0) )
		{
        	ifi->trap_detected = TR_invalid_register_trap;
        	ifi->trap_info = 6;
		}
		
		
      }
#endif
      if (picode->dest < MaxArgs) {
	int pr = ifi->args[picode->dest];
	pthread->Reg[pr].Int64 = pthread->Reg[ifi->OldDest_Phys].Int64;
	pthread->RegValid[pr >> 5] |= (1 << (pr&0x1f));
      }
#endif /* DO_RENAMING */
      ifi->runstate = R_RUN;
      ifi->signal = 0;
  
      if (picode->func == NULL) { 
	/*if (0)*/
	warning("AINT_issue_other: tid=%d instid=%d picode=%p pc=%lx opc=%s -- NULL func\n",
		tid, instid, picode, picode->addr, desc_table[picode->opnum]);

	goto done;
      }
	  
      /* set flag before execution, so its correct in case we hit a signal */
      ifi->issued = 1;

      new_picode = picode->func(ifi, picode, pthread);
      if (new_picode) {
	ifi->nextpc = new_picode->addr;
	ifi->next_picode = new_picode;
      } else {
	ifi->nextpc = 0;
	ifi->next_picode = NULL;
	if (0)
	warning("new_picode is null for tid=%d pc=%lx opc=%s\n", 
		tid, picode->addr, desc_table[picode->opnum].opname);
      }
      { 
	int rdest = -1, pdest=-1;
	ulong dval=0;
	if (picode->dest < MaxArgs) {
	  rdest = picode->args[picode->dest];
	  pdest = ifi->args[picode->dest];
	  dval = REG(picode->dest);
	} 
	if (debug_register_renaming) {
	  informative("AINT_issue: tid=%d instid=%ld pc=%lx dstnum=%d/p%02d dstval=%lx RA=%d/p%02d RB=%d/p%02d RC=%d/p%02d\n",
		      tid, instid, picode->addr, 
		      rdest, ifi->args[picode->dest], dval,
		      picode->args[RA], ifi->args[RA], 
		      picode->args[RB], ifi->args[RB], 
		      picode->args[RC], ifi->args[RC]);
	  informative("AINT_issue: tid=%d instid=%ld [opc=%s ra=%lx rb=%lx rc=%lx immed=%lx lit=%lx]\n",
		      tid, instid, 
		      desc_table[picode->opnum].opname,
		      REG(RA), REG(RB), REG(RC), picode->immed, picode->literal);
	}
      }
      if (ifi->runstate == R_SIGNAL) {
	goto done;
      } else {
	picode = new_picode;
      }
    } else {
      /* NULL picode */
    }
  } else {
    /* arrived via longjmp */
    ifi->runstate = R_SIGNAL;
    ifi->signal = sig;
    ifi->signal_code = siginfo.si_code;
    ifi->signal_pc = (char *)siginfo.si_addr;
  }
 done:
  AINT_in_issue = 0;
  AINT_issue_process = NULL;
}


unsigned long AINT_get_reg_value(int tid, instid_t instid, int reg_arg)
{
  thread_ptr pthread = &Threads[tid];
  inflight_inst_ptr ifi = &(pthread->cbif[instid]);
  icode_ptr picode = ifi->picode;
  icode_ptr new_picode;
  int sig;

  return REG(reg_arg);
}



#ifdef NEW_TTL

#ifndef UINT64
#define UINT64 unsigned long int
#endif
void AINT_TTL_get_reg_value(int tid, instid_t instid, int reg_arg, UINT64 *varray, UINT64 fillVL)
{
  thread_ptr pthread = &Threads[tid];
  inflight_inst_ptr ifi = &(pthread->cbif[instid]);
  icode_ptr picode = ifi->picode;
  icode_ptr new_picode;
  int sig,i;

  for (i=0; i<fillVL; i++)
  	varray[i] = VELEMINT(reg_arg,i);
  
}
#endif



unsigned long AINT_read_reg(int tid, int log_reg)
{
  thread_ptr pthread = &Threads[tid];

  return MapReg(log_reg);
}



void aint_print_changed_state(thread_ptr pthread, inflight_inst_ptr ifi)
{
  icode_ptr picode = ifi->picode;
  fprintf(stdout, "COMMIT: %d %lx %s", 
	  pthread->tid, picode->addr, desc_table[picode->opnum].opname);
  if (picode->iflags & (E_WRITE|E_ST_C)) {
    long *paddr = (long *)((ulong)ifi->paddr&0xFFFFFFFFFFFFFFF8L);
    if (paddr != 0) 
      fprintf(stdout, " ea=%lx v=%lx *ea=%lx\n", ifi->vaddr, REG(RA), *(long *)paddr);
    else
      fprintf(stdout, " ea=%lx v=%lx *ea=??\n", ifi->vaddr, REG(RA));
  } else if (picode->iflags & (E_READ|E_LD_L)) {
    fprintf(stdout, " ea=%lx v=%lx\n", 
	    ifi->vaddr, REG(picode->dest));
  } else {
    fprintf(stdout, " %lx\n", REG(picode->dest));
  }
  fflush(stdout);
}

void AINT_do_read(int tid, instid_t instid)
{
  thread_ptr pthread = &(Threads[tid]);
  inflight_inst_ptr ifi = &(pthread->cbif[instid]);
  ulong bitmask = ifi->bitmask;
  icode_ptr picode = ifi->picode;
  int size = picode->size;
  int i,vl,vs;
  icode_ptr next_picode = NULL;
  int isAGather;
  
  isAGather = (picode->iflags & E_GATSCAT);

  if ( picode->is_vector ) { vl = VL; vs = VS; }
  else                     { vl =  1; vs =  0; }

  
  /* 
   * We treat all reads in a uniform way, so that scalar reads simply
   * become a particular case of vector reads. Recall that we have already
   * computed the first VADDR and PADDR in a previous call to 'event_read'.
   * Therefore, in the first pass through this loop, we simply use the
   * previously computed values. Only if VL happens to be > 1 we have to
   * recompue the addresses so that eveything works fine.
   */
   
  for ( i = 0; i < vl; i++ ) {
   ifi->data = 0l;

	/* Before we try to read on we must be sure we are not dealing with a gather! */
	
	if (isAGather)
	{
		ifi->vaddr = VELEMINT(RA,i) + REG(RB);
		compute_paddr(ifi, picode, pthread);
	}

#ifdef NEW_TTL
   informative_ttl(10,"AINT_do_read: %s fetchnum %lu vaddr %lx paddr %lx \n", picode->is_vector ? "VECTOR" : "SCALAR", ifi->fetchnum, ifi->vaddr, ifi->paddr); 
   informative("(0) AINT_do_read: Element %d vaddr %lx paddr %lx bitmask %lx\n", i, ifi->vaddr, ifi->paddr, ifi->bitmask); 
#endif

	
   /* 
    * Read into register from storebuf and/or from cache 
    */
   bitmask = Read_StoreBuffer(pthread, ifi, ifi->bitmask);

   if (bitmask) {
     /* 
      * Read the rest of the word from the cache 
      */
     ifi->data &= ~bitmask;
     if (ifi->paddr) ifi->data |= (* (long *) (ifi->paddr & QWORD_MASK) & bitmask);
   } 

#ifdef NEW_TTL
    informative_ttl(10,"(1) AINT_do_read: bitmask %lx data %lx \n", bitmask, ifi->data); 
#endif

   /* At this point, we have the quad-word over the given address. We need to
    * shift it to ensure the required data starts at byte 0
    */
   if (size < 8) {
     int	shift = ((ifi->vaddr & 0x7) * 8);
     long	mask = (1 << (size * 8)) - 1;
     int	signpos = 1 << (size * 8 - 1);
     ifi->data = (ifi->data >> shift);
     /* now sign extend */
     if (0 &&ifi->data & signpos) ifi->data |= ~mask;
   }

#ifdef NEW_TTL
    informative_ttl(10,"(2) AINT_do_read: bitmask %lx data %lx \n", bitmask, ifi->data); 
#endif

   /*
    * The following call will put the data just read in the appropriate register.
    * However, for vector operations, the register ID is not enough... we need to
    * tell this routine to which element inside the vector register the data must
    * be written to. We do this by setting ifi->pos_in_vreg to the current loop 
    * iteration
    */
   ifi->pos_in_vreg = i;
 
   (desc_table[picode->opnum].func)(ifi, picode, pthread);
   { 
    int rdest = -1, pdest=-1;
    ulong dval=0;
    if (picode->dest < MaxArgs) {
      rdest = picode->args[picode->dest];
      pdest = ifi->args[picode->dest] + i;
      dval = VELEMINT(picode->dest,i);
    } 
#ifdef NEW_TTL
    informative_ttl(10,"(3) AINT_do_read: dstnum=%d/p%02d destregval=%lx ea=%lx\n", rdest, pdest, dval, ifi->vaddr);
#endif
   }

   /*
    * At this point, we are done for scalar memory references. For vector memory references, however,
    * now it's time to loop back again and fetch the following address, which happens to be in 
    * ifi->vaddr + stride.
    */
    if (( vl > 1 ) && (!isAGather) ) {
     ifi->vaddr += vs;
     compute_paddr(ifi, picode, pthread);
    }
  }

  /* Set ifi->done_memop to true */
  ifi->done_memop = 1;

  /* If inst committed, set ifi->picode to null */
  if (ifi->committed) {
    printf("WARNING: completing memop (read) after commit\n"); 

    if (aint_trace_state_changes)
      aint_print_changed_state(pthread, ifi);
    ifi->picode = NULL;
  }

}


void AINT_do_write(int tid, instid_t instid)
{
  thread_ptr pthread = &(Threads[tid]);
  inflight_inst_ptr ifi = &(pthread->cbif[instid]);
  icode_ptr picode = ifi->picode;
  int i,vl,vs,pr;

#ifdef DO_RENAMING
  /* 
   * roger 9/9/99 
   * The validity of the source DATA register has to be checked here, since in AINT_issue() we did not 
   * check for it.
   */
  pr = ifi->args[RA];
  if ( ISREGVALID(pr) == 0 ) {
   ifi->trap_detected = TR_invalid_register_trap;
   ifi->trap_info = RA;
  }
#endif

  /*
   * It is compulsory to do first a AINT_do_spec_write before actually doing a do_write. We
   * check for that here.
   */
  if ( ! ifi->done_spec_write ) {
      fatal("AINT_do_write: store instruction without do_spec_write \n\t"
            "{tid=%d pc=%lx instr=%x extinstr=%lx opc=%s \n\t"
            " instid=%ld fetchnum=%d type=0x%x issued=%d"
            " committed=%d done_spec_write=%d trapped=%d}"
            " at cycle %ld\n", 
            pthread->tid, 
            ifi->picode->addr, ifi->picode->instr, ifi->picode->extended_instr, 
	    desc_table[ifi->picode->opnum].opname,
            instid, ifi->fetchnum, ifi->type,
            (int)ifi->issued,(int)ifi->committed, (int)ifi->done_spec_write, (int)ifi->trap_detected,
            (ulong)pthread->time);
   }

  /* 
   * For scalar stores, all the information required is already in 'ifi'. We simply
   * loop once around the vector loop and finish.
   */
  if ( ! picode->is_vector ) { 
   /* 
    * Write through to memory 
    */
   (desc_table[picode->opnum].func)(ifi, picode, pthread);
  }
  else {
   /*
    * For vector stores, we have already (when we went through AINT_do_spec_write):
    *  - computed every vaddr for each vector element being stored
    *  - computed every bitmask for each vector element being stored
    *  - converted every piece of data in the register file to its appropriate
    *    memory format.
    *
    * We stored all this information into stq_entry structures and we kept a pointer
    * to each one of those in the stq_update_array[] array. Therefore, to speed up
    * a little bit the task at hand, we will simply walk this array, copy back the
    * desired information into 'ifi' and then call the low level function that 
    * will actually perform the write.
    * 
    */
   vl = VL; 
   for ( i = 0; i < vl; i++ ) {
	
    ifi->vaddr = ifi->stq_update_array[i]->vaddr;
    ifi->data  = ifi->stq_update_array[i]->data;
    compute_paddr(ifi, picode, pthread);
    /* 
     * Write through to memory 
     */
    ifi->pos_in_vreg = i; /* currently, only needed for debugging */ 
    (desc_table[picode->opnum].func)(ifi, picode, pthread);
   }
  }

  /* Set ifi->done_memop to true */
  ifi->done_memop = 1;

  /* If inst committed, set ifi->picode to null; cleanup storeq */
  if (ifi->committed) {
    printf("WARNING: completing memop (store) after commit\n"); 

    if (aint_trace_state_changes)
      aint_print_changed_state(pthread, ifi);
	  
    if (0)
    informative("AINT_do_write: committed store instid=%ld, pc=%lx, fetchnum=%ld, vaddr=%lx\n",
		instid, ifi->picode->addr, ifi->fetchnum, ifi->vaddr);

    Cleanup_StoreBuffer(pthread, ifi, instid);
    ifi->picode = NULL;
  }
}

static char *trap_name[] = {
  "unissued",
  "memop-order-trapped",
  "invalid-register-trap"
};

/* Commit instruction: release old destination register, and dequeue if
 * read/write not pending
 */

void AINT_commit_instr(int tid, instid_t instid)
{
  thread_ptr pthread = &Threads[tid];
  inflight_inst_ptr ifi = &(pthread->cbif[instid]);
  icode_ptr picode = ifi->picode;
  Arg picode_dest = picode->dest;

  num_outstanding_instrs--;

#ifdef E_WAS_LDQ_U
  if (picode->iflags & E_WAS_LDQ_U)
    was_ldq_u_count++;
#endif /* E_WAS_LDQ_U */

  /* make sure the previous instruction was committed */
  if (ifi->fetchnum > 0) {
    instid_t previnstid = PRED_CIRC(instid, CBIF_SIZE);
    if(!(pthread->cbif[previnstid].committed))
      fatal("AINT_commit_instr: tid=%d instid=%ld fetchnum=%ld previnstid=%ld previous instruction not committed.\n",
	    tid, instid, ifi->fetchnum, previnstid);
  }

  if (!ifi->veaFlag && (!ifi->issued || ifi->trap_detected)) { 
    if (ifi->trap_detected == TR_invalid_register_trap) {
      int argnum = ifi->trap_info;
      int lr,pr;

#ifdef NEW_TTL
      if      ( argnum == 4 ) { lr = VL_REG; pr = ifi->vl; }
      else if ( argnum == 5 ) { lr = VS_REG; pr = ifi->vs; }
	  else if ( argnum == 6 ) { lr = VM_REG; pr = ifi->vm; }
      else                    { lr = picode->args[argnum]; pr = ifi->args[argnum]; }
#else
      lr = picode->args[argnum]; 
      pr = ifi->args[argnum];
#endif
      fatal("AINT_commit: committing instruction that used unwritten physical register: "
	    "tid=%d pc=%lx fetchnum=%lx instid=%ld lreg=%d preg=%d; %s\n",
	    tid, ifi->pc, ifi->fetchnum, instid,
	    lr, pr, desc_table[picode->opnum]);
    } else {
      error("Committed %s instruction tid=%d instid=%d pc=%lx opc=%s dstreg=%d at cycle %ld\n",
	    trap_name[ifi->trap_detected],
	    tid, instid, picode->addr, desc_table[picode->opnum].opname,
	    picode->args[picode_dest],
	    (ulong)pthread->time);
    }
  }

  pthread->runstate = ifi->runstate;
  pthread->signal = ifi->signal;

#ifdef GDBSERVER
  if (Debugger_tty_name != NULL) {
    gdbserver_cycle();
    /* can be updated by gdbserver_cycle() */
    ifi->runstate = pthread->runstate;
    ifi->signal = pthread->signal;
  }
#endif

  if (ifi->runstate == R_SIGNAL) {
    process_ptr process = pthread->process;
    if (picode->iflags & E_FLOAT) {
      fprintf(Aint_output, "\n\n\t\t\tINSTRUCTION CAUGHT/RAISED A SIGNAL (MOST LIKELY SIG_FPE (8))\n");
      fprintf(Aint_output, "  R_SIGNAL opc=%s \n\t [RA]=%016lx \n\t [RB]=%016lx\n\t [RC]=%016lx\n",
	      desc_table[picode->opnum].opname,
	      FP(RA),
	      FP(RB),
	      FP(RC)
	      );
      fprintf(Aint_output, "\t [FPCR]=%016lx\n", pthread->fpcr);
      fprintf(Aint_output, "\t signal=%d \n\t sv_handler=%d\n",
	      pthread->signal, 
	      pthread->process->sigv[pthread->signal].sv_handler);
    }
    fprintf(Aint_output,"\nSIGNAL information:\n\tsigno %d\n\tsi_code %d\n\tsi_addr %lx"
	    " (this pc is probably the PC+4 of the faulting address or\n\tthe PC of the first TRAPB"
	    " after the faulting address)\n",
	    (int)ifi->signal, (int)ifi->signal_code, ifi->signal_pc
	   );
    fatal("AINT_commit_instr: thread %d:%d runstate=R_SIGNAL, signal=%d for "
	  "instruction pc=%lx,addr=%x,opc=%s\n",
	  tid, process->pid, 
	  pthread->signal,
	  picode->addr,
	  picode_instr(pthread, picode),
	  desc_table[picode->opnum].opname
	  );
  }

  /* Release the old destination register if we renamed it */
  if (picode_dest < MaxArgs) {
    int olddestphys = ifi->OldDest_Phys;
    int rdest = picode->args[picode_dest];
    int pdest = ifi->args[picode_dest];
    if (debug_register_renaming)
      informative("AINT_commit_instr: tid=%d instid=%ld freeing dstnum=%d/p%02d (now %d/p%02d)\n",
		  tid, instid, rdest, ifi->OldDest_Phys,
		  rdest, pdest);
    if (olddestphys != ZERO_REGISTER){
      /* 
       * Distinguish between vector and scalar registers 
       */
      if ( olddestphys >= FIRST_VECTOR_PHYSICAL ) {
       pthread->Reg[olddestphys].Int64 = pthread->FirstFreeVectorPhysicalRegister;
       pthread->FirstFreeVectorPhysicalRegister = olddestphys;
      }
      else {
       pthread->Reg[olddestphys].Int64 = pthread->FirstFreePhysicalRegister;
       pthread->FirstFreePhysicalRegister = olddestphys;
      }
      pthread->RegValid[olddestphys >> 5] &= ~(1 << (olddestphys&0x1f));
    }
  }

  /* If this was a store, 
   * dequeue and delete the stq update should happen at
   * do_write, when we go to the cache.
   */
    
  if (ifi->committed || picode == NULL) {
    error("Instruction instid=%ld is already committed\n", instid);
    return;
  }

  ifi->committed = 1;
  /* Release the instid ONLY if there is no pending read/write */
  if ((picode->iflags & (E_MEM_REF|E_LOCK))&& !(picode->iflags & E_PREFETCH) ) {
    if (!ifi->done_memop) {
      /* r2r:
       * AINT supports the notion of first doing a commit and later doing
       * a do_read/write. This is supported to allow data sharing accross
       * threads to occur at a point that is independent of commiting the 
       * corresponding instruction, i.e. the new data is not visible to
       * other threads until it has left the write/store/merge buffer, which
       * can be many cycles after the instruction commits.
       *
       * I'm disabling this ability here in order to have an earlier warning
       * indication for debugging the non-multithreaded case.
       * When we try to run multi-threaded applications, we will have to
       * augment AINT's memory system to support the notion of sharing
       * at different levels.
       */
      /*  return; */
      fatal("AINT_commit: committing memory instruction before do_write/read \n\t"
            "{tid=%d pc=%lx instr=%x extinstr=%lx opc=%s \n\t"
            " instid=%ld fetchnum=%d type=0x%x issued=%d"
            " committed=%d done_memop=%d trapped=%d}"
            " at cycle %ld\n", 
            pthread->tid, 
            ifi->picode->addr, ifi->picode->instr, ifi->picode->extended_instr, desc_table[ifi->picode->opnum].opname,
            instid, ifi->fetchnum, ifi->type,
            (int)ifi->issued,(int)ifi->committed, (int)ifi->done_memop, (int)ifi->trap_detected,
            (ulong)pthread->time);
    }
  }
  if (aint_trace_state_changes)
    aint_print_changed_state(pthread, ifi);

  Cleanup_StoreBuffer(pthread, ifi, instid);
  ifi->picode = NULL;
}

#if 0
void AINT_kill_instrs(int tid, instid_t instid, Kill_Type kill_reason)
{
  /* For now, ignore the kill reason.
   * Start at the newest inst in the pipeline, working
   * backwards until we kill instid i
   */

  thread_ptr pthread = &(Threads[tid]);

  if (num_outstanding_instrs == 0){
    return; /* nothing to kill */
  }

  do { /* use do-while loop instead of while loop to capture the case
          that the buffer is full. Note that we should already
          return if the buffer is empty. In both cases, 
          pthread->instid == PRED_CIRC(instid, CBIF_SIZE)
       */
    /* Kill j, release dest reg, rollback regmap */
    inflight_inst_ptr ifi = &(pthread->cbif[pthread->instid]);
    icode_ptr picode = ifi->picode;

    num_outstanding_instrs--;

    if (!picode) {
      error("AINT_kill_instrs: tid=%d instid=%d already killed\n", tid, instid);
      return;
    }
      
    if (ifi->done_memop && (picode->iflags&(E_WRITE|E_ST_C)))
      error("killing write after AINT_do_write\n");

    if (picode->dest < MaxArgs) {
      Physical_RegNum pr = ifi->args[picode->dest];
      Physical_RegNum *free;

      if (pr != ZERO_REGISTER) {
        if (debug_register_renaming)
          informative("AINT_kill_instrs: tid=%d instid=%d freeing dstnum=%d/p%02d (now %d/p%02d)\n",
                      tid, pthread->instid, 
                      picode->args[picode->dest], pr,
                      picode->args[picode->dest], ifi->OldDest_Phys);
	/*
	 * Are we releasing a vector or a scalar register ? Choose the appropriate free
	 * list accordingly
	 */
        free = (pr >= FIRST_VECTOR_PHYSICAL) ? 
          (&pthread->FirstFreeVectorPhysicalRegister) : 
          (&pthread->FirstFreePhysicalRegister);

        pthread->Reg[pr].Int64 = *free;
        *free = pr;
        pthread->RegValid[pr >> 5] &= ~(1 << (pr&0x1f));
        pthread->RegMap[picode->args[picode->dest]] = ifi->OldDest_Phys;
      }
    }

    /* check_reg_state(tid,"at end of AINT_kill_instrs"); */

    /* If this was a store, dequeue and delete storeq update */
#ifdef USE_STOREBUFFER
    Cleanup_StoreBuffer(pthread, ifi, instid);
#endif /* USE_STOREBUFFER */

    ifi->picode = NULL;

    pthread->instid = PREV_INSTID(pthread);
  } while (pthread->instid != PRED_CIRC(instid, CBIF_SIZE));
}
#endif

void AINT_kill_instrs(int tid, instid_t instid, Kill_Type kill_reason)
{
  /* For now, ignore the kill reason.
   * Start at the newest inst in the pipeline, working
   * backwards until we kill instid i
   */

  thread_ptr pthread = &(Threads[tid]);


  num_outstanding_instrs--;

  while(pthread->instid != PRED_CIRC(instid, CBIF_SIZE)) {

    /* Kill j, release dest reg, rollback regmap */
    inflight_inst_ptr ifi = &(pthread->cbif[pthread->instid]);
    icode_ptr picode = ifi->picode;

    if (!picode) {
      error("AINT_kill_instrs: tid=%d instid=%d already killed\n", tid, instid);
      return;
    }
      
    if (ifi->done_memop && (picode->iflags&(E_WRITE|E_ST_C)))
      error("killing write after AINT_do_write\n");

    if (picode->dest < MaxArgs) {
      Physical_RegNum pr = ifi->args[picode->dest];
      Physical_RegNum *free;

      if (pr != ZERO_REGISTER) {
	if (debug_register_renaming)
	  informative("AINT_kill_instrs: tid=%d instid=%d freeing dstnum=%d/p%02d (now %d/p%02d)\n",
		      tid, pthread->instid, 
		      picode->args[picode->dest], pr,
		      picode->args[picode->dest], ifi->OldDest_Phys);
	/*
	 * Are we releasing a vector or a scalar register ? Choose the appropriate free
	 * list accordingly
	 */
	free = (pr >= FIRST_VECTOR_PHYSICAL) ? 
		   (&pthread->FirstFreeVectorPhysicalRegister) : 
		   (&pthread->FirstFreePhysicalRegister);

	pthread->Reg[pr].Int64 = *free;
	*free = pr;
	pthread->RegValid[pr >> 5] &= ~(1 << (pr&0x1f));
	pthread->RegMap[picode->args[picode->dest]] = ifi->OldDest_Phys;
      }
    }

    /* check_reg_state(tid,"at end of AINT_kill_instrs"); */

    /* If this was a store, dequeue and delete storeq update */
#ifdef USE_STOREBUFFER
    Cleanup_StoreBuffer(pthread, ifi, instid);
#endif /* USE_STOREBUFFER */

    ifi->picode = NULL;

    pthread->instid = PREV_INSTID(pthread);
  }
}


void AINT_regmap_snapshot(Physical_RegNum *snapshot, thread_ptr pthread, instid_t current_instid)
{
  instid_t instid = pthread->instid;
  
  /* take a snapshot of the latest RegMap */
  memcpy(snapshot, pthread->RegMap, sizeof(pthread->RegMap));

  while(instid != current_instid) {
    /* rollback regmap */
    inflight_inst_ptr ifi = &(pthread->cbif[instid]);
    icode_ptr picode = ifi->picode;

    if (picode->dest < MaxArgs) {
      Physical_RegNum pr = ifi->args[picode->dest];
      if (pr != ZERO_REGISTER) {
	snapshot[picode->args[picode->dest]] = ifi->OldDest_Phys;
      }
    }

    instid = PRED_CIRC(instid, CBIF_SIZE);
  }
}

void compute_paddr(inflight_inst_ptr ifi, icode_ptr picode, thread_ptr pthread)
{
 unsigned long vaddr, paddr;

  /*
   * We have been called either from
   *  - event_read (below) or
   *  - AINT_Doread (in this case, we are dealing with a vector memory insn)
   *
   * Our starting address is stored in ifi->vaddr
   */
  vaddr = ifi->vaddr;

  /*
   * Translate address and store it in appropriate fields
   */
  paddr = PMAP(vaddr);
  pthread->paddr = paddr;
  ifi->paddr = paddr;

  
  /* 
   * Calculate the byte-mask 
   */
  switch (picode->size) {
  case 0: /* wh64 has no data */
    ifi->bitmask = 0;
    break;
  case 1:
  case 2:
  case 4:
    ifi->bitmask = (1L << (8 * picode->size)) - 1;
    /*printf(">> compute_paddr: %12lx: vaddr=%lx, size=%d, bitmask1=%lx\n",picode->addr, ifi->vaddr, picode->size, ifi->bitmask);*/
    ifi->bitmask = ifi->bitmask << ((ifi->vaddr & 0x7)<<3);
    /*printf(">> compute_paddr: bitmask2=%lx\n", ifi->bitmask);*/
    break;
  case 8:
    ifi->bitmask = -1L;
    break;
  default:
      fatal("p%d.%d: 0x%lx: invalid data size (%d) for memop\n", pthread->process->pid, pthread->tid, picode->addr, picode->size);
  }

  return;
}

void convert_register_value(inflight_inst_ptr ifi, icode_ptr picode, thread_ptr pthread, int regIdx)
{
 double fa;

 if ( picode->iflags & E_FLOAT ) {
  fa = picode->is_vector ? VELEMFP(regIdx,ifi->pos_in_vreg) :  FP(regIdx);
  aint_cvt_fp_mem (picode->opnum, fa, (long*)&ifi->data);
 }
 else {
  ifi->data = picode->is_vector ? VELEMINT(regIdx,ifi->pos_in_vreg) : REG(regIdx);
 }
}

OP(event_read)
{
  event_ptr pevent = pthread->pevent;
  unsigned long vaddr, paddr;
  int is_shared;

  ifi->type = picode->iflags;

  /* The vaddr calculated is the address passed on by the instruction decoder.
   * The actual address referenced may be different, as in case of unaligned
   * loads (and stores)
   */
   
  if (!ifi->veaFlag)
  {
      vaddr = REG(RB) + picode->immed;
  }
  else
  {
      /* Aint gets this from the performance model, set via FEED_SetEA */ 
      vaddr = ifi->veaExternal; 
  }

  if (picode->opnum == ldq_u_opn) vaddr = BASE2ROUNDDOWN(vaddr, 8);

  /*
   * Store the address in the corresponding fields, and call 'compute_paddr' to
   * do the rest of the job (by the way, compute_paddr will also compute the
   * necessary bitmask
   */
  pthread->vaddr = vaddr;
  ifi->vaddr = vaddr;

  /*
   * This will compute the physical address and the bitmask and leave them in 'ifi'
   */
  compute_paddr(ifi, picode, pthread);

  return picode->next;
}




OP(event_write)
{
  event_ptr pevent;
  long vaddr, paddr;
  int is_shared;

  ifi->type = picode->iflags;

  /* The vaddr calculated is the address passed on by the instruction decoder.
   * The actual address referenced may be different, as in case of unaligned
   * loads (and stores)
   */

  vaddr = REG(RB) + picode->immed;

  if (picode->opnum == stq_u_opn) vaddr = BASE2ROUNDDOWN(vaddr, 8);

  /*
   * Store the address in the corresponding fields, and call 'compute_paddr' to
   * do the rest of the job (by the way, compute_paddr will also compute the
   * necessary bitmask
   */
  pthread->vaddr = vaddr;
  ifi->vaddr = vaddr;

  /*
   * This will compute the physical address and the bitmask and leave them in 'ifi'
   */
  /*printf (">>event write: vaddr=%lx\n",vaddr);*/
  compute_paddr(ifi, picode, pthread);

  /* 
   * Convert register value to memory word. Here we have to be careful for vector writes,
   * since we may be called repeteadly to compute the different data items inside a vector
   * register. This will leave the desired data into ifi->data.
   */

  convert_register_value(ifi, picode, pthread,RA);

  return picode->next;
}

OP(event_wh64)
{
  event_ptr pevent;
  long vaddr, paddr;
  int is_shared;

  ifi->type = picode->iflags;

  /* The vaddr calculated is the address passed on by the instruction decoder.
   * The actual address referenced may be different, as in case of unaligned
   * loads (and stores)
   */
  vaddr = REG(RB);
  /* 64-byte aligned */
  vaddr = BASE2ROUNDDOWN(vaddr, 64);

  /*
   * Store the address in the corresponding fields, and call 'compute_paddr' to
   * do the rest of the job (by the way, compute_paddr will also compute the
   * necessary bitmask
   */
  pthread->vaddr = vaddr;
  ifi->vaddr = vaddr;

  /*
   * This will compute the physical address and the bitmask and leave them in 'ifi'
   */
  compute_paddr(ifi, picode, pthread);

  return picode->next;
}

OP(event_store_conditional)
{
  event_write(ifi, picode, pthread); 

  /* Now update result register */
  REG(RA) = ifi->stc_success; ZERO_ZERO_REG;
  return  picode->next;
} 

OP(event_load_locked)
{
  return event_read(ifi, picode, pthread);
}



OP(event_memory_barrier)
{
  /* has no effect in aintpm -Jamey 1/7/97 */ 
  return picode->next;
}

OP(event_sim_user)
{
  /* unsupported in aintpm -Jamey 1/9/97 */ 
  return picode->next;
}

OP(event_inst)
{
  event_ptr pevent;

  pevent = pthread->pevent;
  pevent->iaddr = picode->addr;
  pevent->picode = picode;
  pevent->type = picode->iflags;

  pthread->ufunc = NULL;

  /* Force a reschedule here since we want sim_inst to execute */
  SLEEP(pthread);
  return(picode->next);
}

instid_t OLD_AINT_fetch_issue_commit_next_instr(int tid, ulong predicted_pc,
					    ulong *ea, int *taken)
{
  thread_ptr pthread = &Threads[tid];
  icode_ptr picode = pthread->next_fetch_picode;
  instid_t instid;
  inflight_inst_ptr ifi;
  long flags;
  long iflags;

  if (0)
  fprintf(Aint_output, "AINT_fetch_issue_commit_next_instr: tid=%d predicted_pc=%lx\n",
	  tid, predicted_pc);
  /* save a call to addr2iphys() if we can */
  if (   picode
      && picode->addr == predicted_pc) {
    pthread->next_fetch_picode = picode->next;
  } else {
    picode = addr2iphys(pthread->process, predicted_pc, &flags);
    pthread->next_fetch_picode = picode->next;
  }

  if (0) fprintf(Aint_output, "fetch ... ");


  /* just use the current instid */
  instid = pthread->instid;
  ifi = &pthread->cbif[instid];

  /* zero fields out to avoid errors: */
  bzero((void *)ifi, sizeof(struct inflight_inst));

  iflags = picode->iflags;
  ifi->pc = picode->addr;
  ifi->picode = picode;
  ifi->fetchnum = pthread->fetch_count++;
  ifi->type = iflags;
  
  /* Assume st_c's will succeed. */
  if (ifi->picode->iflags & E_ST_C)
    ifi->stc_success=1;

  /* do register renaming */

#ifdef DO_RENAMING
  {
    Physical_RegNum *ifi_args = ifi->args;
    Physical_RegNum *regmap = pthread->RegMap;
    Logical_RegNum *picode_args = picode->args;

    ifi_args[RA] = regmap[picode_args[RA]];
    ifi_args[RB] = regmap[picode_args[RB]];
    ifi_args[RC] = regmap[picode_args[RC]];

    if (picode->dest < MaxArgs) {
      Arg picode_dest = picode->dest;

      /* This instruction writes a register. Rename that register */
      ifi->OldDest_Phys = ifi_args[picode_dest];

      ifi_args[picode_dest] = ReMap(pthread,  picode_args[picode_dest]);
      if (debug_register_renaming)
	informative("AINT_fetch_next_inst: (predpc=0x%lx) picode->addr=0x%lx, tid=%d instid=%d defining dstnum=%d/p%02d (was %d/p%02d)\n",
		    predicted_pc, picode->addr,
		    tid, instid, 
		    picode_args[picode_dest], ifi_args[picode_dest],
		    picode_args[picode_dest], ifi->OldDest_Phys);

    } else {
      ifi->OldDest_Phys = ZERO_REGISTER; /* This is never released */
    }
  }
#endif

  if (0) fprintf(Aint_output, "issue ... ");
  /* now issue */
  {
    icode_ptr new_picode;

    /* This is the time to copy the old destination register value into the new one */
#ifdef DO_RENAMING
    if (picode->dest < MaxArgs) {
      pthread->Reg[ifi->args[picode->dest]].Int64 = pthread->Reg[ifi->OldDest_Phys].Int64;
	  /*
      pthread->Reg[ifi->args[picode->dest]] = pthread->Reg[ifi->OldDest_Phys];
	  */
	  
    }
#endif /* DO_RENAMING */
    ifi->runstate = R_RUN;
    ifi->signal = 0;
  
    new_picode = picode->func(ifi, picode, pthread);
    if (new_picode)
      ifi->nextpc = new_picode->addr;
    else
      ifi->nextpc = 0;
    ifi->issued = 1;
    ifi->trap_detected = 0;
    { 
      int rdest = -1, pdest=-1;
      ulong dval=0;
      if (picode->dest < MaxArgs) {
	rdest = picode->args[picode->dest];
	pdest = ifi->args[picode->dest];
	dval = REG(picode->dest);
      } 
    }
  }
  /* special issue actions */
  if (iflags & (E_CONDBR)) {
    if (taken != NULL)
      *taken = ifi->nextpc;
  } else if (iflags & (E_MEM_REF|E_LOCK)) {
    if (ea != NULL)
      *ea = pthread->vaddr;
  }

  if (0) fprintf(Aint_output, "do memop ... ");

  /* do memop */
  if (iflags & (E_READ|E_LD_L)) {
    ulong bitmask = ifi->bitmask;
    int size = picode->size;
    icode_ptr next_picode = NULL;

    ifi->data = 0l;
    /* Read into register from storebuf and/or from cache */
    /* don't need to use the storebuf */
    if (bitmask) {
      /* Read the rest of the word from the cache */
      ifi->data &= ~bitmask;
      if (ifi->paddr) ifi->data |= (* (long *) (ifi->paddr & QWORD_MASK) & bitmask);
    } else {
      fatal("bitmask = 0 for pc=%lx\n", ifi->picode->addr);
    }

    /* At this point, we have the quad-word over the given address. We need to
     * shift it to ensure the required data starts at byte 0
     */
    if (size < 8) {
      int shift = ((ifi->vaddr & 0x7) * 8);
      long mask = (1 << (size * 8)) - 1;
      int signpos = 1 << (size * 8 - 1);
      ifi->data = (ifi->data >> shift);
      /* now sign extend */
      if (0 &&ifi->data & signpos)
	ifi->data |= ~mask;
    }

    (desc_table[picode->opnum].func)(ifi, picode, pthread);
    next_picode = picode;
    { 
      int rdest = -1, pdest=-1;
      ulong dval=0;
      if (next_picode->dest < MaxArgs) {
	rdest = next_picode->args[next_picode->dest];
	pdest = ifi->args[next_picode->dest];
	dval = REG(next_picode->dest);
      } 
    }

    /* Set ifi->done_memop to true */
    ifi->done_memop = 1;
  } else if (iflags & (E_WRITE|E_ST_C)) {
    /* At this point, we have the quad-word over the given address. We need to
     * shift it to ensure the required data starts at byte 0
     */
    
    /* Write through to memory */
    (desc_table[picode->opnum].func)(ifi, picode, pthread);
    /* Set ifi->done_memop to true */
    ifi->done_memop = 1;
  }

  if (0) fprintf(Aint_output, "commit\n");
  /* commit */
  {
    Arg picode_dest = picode->dest;

#ifdef E_WAS_LDQ_U
    if (iflags & E_WAS_LDQ_U)
      was_ldq_u_count++;
#endif /* E_WAS_LDQ_U */

    if (ifi->trap_detected) {
      error("Committed %s instruction tid=%d instid=%d pc=%lx opc=%s dstreg=%d at cycle %ld\n",
	    "trapped",
	    tid, instid, picode->addr, desc_table[picode->opnum].opname,
	    picode->args[picode_dest],
	    (ulong)pthread->time);
    }

    pthread->runstate = ifi->runstate;
    pthread->signal = ifi->signal;

#ifdef GDBSERVER
    if (Debugger_tty_name != NULL) {
      gdbserver_cycle();
      /* can be updated by gdbserver_cycle() */
      ifi->runstate = pthread->runstate;
      ifi->signal = pthread->signal;
    }
#endif

    if (ifi->runstate == R_SIGNAL) {
      process_ptr process = pthread->process;
      if (iflags & E_FLOAT) {
	fprintf(Aint_output, "  R_SIGNAL opc=%s \n\t [RA]=%016lx \n\t [RB]=%016lx\n\t [RC]=%016lx\n",
		desc_table[picode->opnum].opname,
		FP(RA),
		FP(RB),
		FP(RC)
		);
	fprintf(Aint_output, "\t [FPCR]=%016lx\n", pthread->fpcr);
	fprintf(Aint_output, "\t signal=%d \n\t sv_handler=%d\n",
		pthread->signal, 
		pthread->process->sigv[pthread->signal].sv_handler);
      }
      fatal("AINT_commit_instr: thread %d:%d runstate=R_SIGNAL, signal=%d for "
	    "instruction pc=%lx,addr=%x,opc=%s\n",
	    tid, process->pid, 
	    pthread->signal,
	    picode->addr,
	    picode_instr(pthread, picode),
	    desc_table[picode->opnum].opname
	    );
    }

    /* Release the old destination register if we renamed it */
    if (picode_dest < MaxArgs) {
      int olddestphys = ifi->OldDest_Phys;
      int rdest = picode->args[picode_dest];
      int pdest = ifi->args[picode_dest];
      if (debug_register_renaming)
	informative("AINT_commit_instr: tid=%d instid=%ld freeing dstnum=%d/p%02d (now %d/p%02d)\n",
		    tid, instid, rdest, ifi->OldDest_Phys,
		    rdest, pdest);
      if (olddestphys != ZERO_REGISTER){
	pthread->Reg[olddestphys].Int64 = pthread->FirstFreePhysicalRegister;
	pthread->FirstFreePhysicalRegister = olddestphys;
	pthread->RegValid[olddestphys >> 5] &= ~(1 << (olddestphys&0x1f));
      }
    }

    /* If this was a store, 
     * dequeue and delete the stq update should happen at
     * do_write, when we go to the cache.
     */
    ifi->committed = 1;

#ifdef USE_STOREBUFFER
    Cleanup_StoreBuffer(pthread, ifi, instid);
#endif /* USE_STOREBUFFER */
    ifi->picode = NULL;
  }
  return instid;
}


instid_t AINT_fetch_issue_commit_next_instr(int tid, ulong nextpc)
{
 instid_t instid;
 thread_ptr pthread = &Threads[tid];
 inflight_inst_ptr ifi;
 long iflags;

 /*
  * Fetch, issue and commit the instruction found at 'nextpc', using
  * 'nextpc' as the predicted pc. 
  */
 instid = AINT_fetch_next_instr(tid, nextpc);

 /*
  * Get the instruction flags to direct Issue decisions
  */
 ifi = &pthread->cbif[pthread->instid];
 iflags = ifi->picode->iflags;

 /*
  * Issue instruction
  */
 if ( iflags & E_MEM_REF ) {
  /*
   * Mem refs first 'issue' and then actually perform the read/write
   */
  AINT_issue_other(tid, instid);
  if      ( iflags & (E_READ|E_LD_L))  AINT_do_read(tid, instid);    
  else if ( iflags & (E_WRITE|E_ST_C)) {
   AINT_do_spec_write(tid, instid);
   AINT_do_write(tid, instid);
  }
 }
 else {
  /*
   * Although the FEED_Issue() function would invoke 'AINT_issue_branch()' for
   * branch instructions, we do not do that because:
   *  - AINT_issue_branch() and AINT_issue_other() are exactly equivalent except
   *    for the fact that AINT_issue_branch() returns the target address of the
   *    branch
   *  - We do not need the target of the branch because we are not passing that
   *    information up to the performance model
   *
   * Therefore, we end up calling 'AINT_issue_other' for everything that is not
   * a memory reference.
   */
  AINT_issue_other(tid, instid);
 }

 /*
  * Commit Instruction
  */
 AINT_commit_instr(tid, instid);

 return instid;
}


instid_t AINT_fetch_issue_next_instr(int tid, ulong nextpc)
{
 instid_t instid;
 thread_ptr pthread = &Threads[tid];
 inflight_inst_ptr ifi;
 long iflags;

 /*
  * Fetch, issue and commit the instruction found at 'nextpc', using
  * 'nextpc' as the predicted pc. 
  */
 instid = AINT_fetch_next_instr(tid, nextpc);

 /*
  * Get the instruction flags to direct Issue decisions
  */
 ifi = &pthread->cbif[pthread->instid];
 iflags = ifi->picode->iflags;

 /*
  * Issue instruction
  */
 if ( iflags & E_MEM_REF ) {
  /*
   * Mem refs first 'issue' and then actually perform the read/write
   */
  AINT_issue_other(tid, instid);
  if      ( iflags & (E_READ|E_LD_L))  AINT_do_read(tid, instid);    
  else if ( iflags & (E_WRITE|E_ST_C)) {
   AINT_do_spec_write(tid, instid);
   AINT_do_write(tid, instid);
  }
 }
 else {
  /*
   * Although the FEED_Issue() function would invoke 'AINT_issue_branch()' for
   * branch instructions, we do not do that because:
   *  - AINT_issue_branch() and AINT_issue_other() are exactly equivalent except
   *    for the fact that AINT_issue_branch() returns the target address of the
   *    branch
   *  - We do not need the target of the branch because we are not passing that
   *    information up to the performance model
   *
   * Therefore, we end up calling 'AINT_issue_other' for everything that is not
   * a memory reference.
   */
  AINT_issue_other(tid, instid);
 }

 return instid;
}



int AINT_kill_thread(thread_ptr pthread, int sig)
{
  fprintf(Aint_output, "AINT_kill_thread: %d.%d sig=%d\n", 
	  pthread->tid, pthread->process->pid, sig);
  pthread->runstate = R_SIGNAL;
  if (pthread->signal == 0)
    pthread->signal = sig;
  else
    pthread->sigpending |= (1 << sig);
  return 0;
}



/* This routine is called when a process terminates in orer to wait for
 * all its children to complete. This guarantees that the elapsed time 
 * of the parent is at least as great as all of  its children.
 *
 * Return values:
 *   -1 if all children have finished
 *    0 if some child has not finished
 */

int
wait_for_children(thread_ptr pthread)
{
  int childpid;

  while ((childpid = wait_for_child(pthread)) > 0)
    ;

  return childpid;
}

/* This routine is called by wait to wait for one child. If there is
 * more than one terminated child, the child that terminated first
 * is the one that is waited on.
 *
 * Return values:
 *   -1   no children exist
 *    0   no child has terminated yet
 *    pid the child pid of the earliest terminated child process
 */

int
wait_for_child(thread_ptr pthread)
{
    process_ptr child;

    process_ptr prev;
    aint_time_t min_time;

    child = pthread->process->youngest;

    /* if there are no children, return -1 */
    if (child == NULL)
        return -1;

    min_time = MAXTIME;
    /* search the list of children for one that has finished */
    for ( ; child; child = child->sibling) {
        if (child->runstate == R_DONE || child->is_zombie) {
            /* find the earliest terminated child */
            if (min_time > child->threads->time)
                min_time = child->threads->time;
        }
    }

    /* if no child has terminated, then return 0 */
    if (min_time == MAXTIME)
        return 0;

    prev = pthread->process;

    /* search the list of children again for the earliest one */
    for (child = pthread->process->youngest; child; child = child->sibling) {
        if (child->threads->time == min_time
            && (child->runstate == R_DONE || child->is_zombie)) {
            /* disown child */
            if (prev == pthread->process)
                pthread->process->youngest = child->sibling;
            else
                prev->sibling = child->sibling;

            /* collect cpu time of child */
            pthread->process->child_cpu += child->threads->cpu_time + child->child_cpu;

            /* update parent's elapsed time to that of the child */
            if (pthread->time < child->threads->time)
                pthread->time = child->threads->time;

            /* Set child's parent pointer to NULL so that the child knows
             * the parent waited on it already, and it can put itself
             * on the free queue.
             */
            child->parent = NULL;

            /* If child is not a zombie, then move it to the free pool.
             * We can't move a zombie child to the free pool, since it
             * still has to send an E_DONE event, so it is not yet
             * free to be reallocated. A zombie child will move itself
             * to the free pool if the parent has waited on it already.
             */
            if (child->runstate == R_DONE) {
                /* move child to free pool */
                INLINE_REMOVE(child);
                if (Recycle_threads) {
                    INLINE_INSERT_AFTER(&Free_q, child->threads);
		    INLINE_INSERT_AFTER(&Free_Process_q, child);
		}
                child->runstate = R_FREE;
		child->threads->runstate = R_FREE;
            }

            return child->pid;
        } else
            prev = child;
    }

    /* if we get here, it's an error */
    fatal("wait_for_child: can't find earliest dead child\n");
    return 0;
}

void
sleep_thr(thread_ptr pthread)
{
    INLINE_REMOVE(pthread);
    INLINE_ENQUEUE(&Sleep_q, pthread);
    pthread->runstate = R_SLEEP;
}

void
wakeup_thr(thread_ptr pthread)
{
    INLINE_REMOVE(pthread);
    INLINE_ENQUEUE(&Run_q, pthread);
    pthread->runstate = R_RUN;
}




/* This is the last function called by a thread. It moves the thread
 * to the free queue if the parent has already waited on it.
 */
/* ARGSUSED */
OP(done_thr)
{
    /* If parent has already waited on this thread, then we can move
     * this thread to the free queue. Otherwise, move this thread to
     * the done queue and the parent will move
     * this thread to the free queue when it waits on this thread.
     */

    pthread->process->thread_count--;

    if ( (pthread->process->parent == NULL)) {

        INLINE_REMOVE(pthread);
        if (Recycle_threads)
            INLINE_INSERT_AFTER(&Free_q, pthread);
        pthread->runstate = R_FREE;
	if (pthread->process->thread_count == 0) pthread->process->runstate = R_FREE;
    } else {
        INLINE_REMOVE(pthread);
        INLINE_ENQUEUE(&Done_q, pthread);
        pthread->runstate = R_DONE;
	if (pthread->process->thread_count == 0) pthread->process->runstate = R_DONE;
    }

    return NULL;	/* return value not used */
}

/* Block the thread and put it on the specified wait queue.
 */
void
block_thr(qnode_ptr q, thread_ptr pthread)
{
    INLINE_REMOVE(pthread);
    INLINE_ENQUEUE(q, (qnode_ptr) pthread);
    pthread->runstate = R_BLOCKED;
}

/* dequeue the first waiting thread and make it runnable */
thread_ptr
unblock_thr(thread_ptr q)
{
    thread_ptr pthread;

    INLINE_DEQUEUE(q, pthread);
    if (pthread) {
        INLINE_ENQUEUE(&Run_q, pthread);
        pthread->runstate = R_RUN;
	pthread->process->runstate = R_RUN;
    }
    return pthread;
}


