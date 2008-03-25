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
 
//
// Authors:  David Goodwin, Steven Wallace, Roger Espasa, Artur Klauser, Harish Patil
//

// generic
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/trace.h"
#include "asim/cmd.h"

// ASIM public modules
#include "asim/provides/instfeeder_interface.h"
#include "asim/provides/isa.h"

// ASIM local module
extern "C"
{
#include "icode.h"
}

#include "Verifier.h"
#include "Oracle.h"
#include "posix_thread.h"
#include "marker.h"

extern int Quiet; 

EXTERNC void informative(char *s, ...);
EXTERNC void informative_ttl(int level,char *s, ...);
EXTERNC void pmint_init(int max_nthreads, int cur_nthreads);
EXTERNC void pmint_thread_begin(int tid);
EXTERNC void pmint_thread_end(int tid);
EXTERNC void pmint_thread_block(int tid);
EXTERNC void pmint_thread_unblock(int tid);
EXTERNC void pmint_quiesce_thread(int tid, void *picode, unsigned long addr);
EXTERNC void pmint_request_begin_skipping (long how_many_to_skip);
EXTERNC void pmint_request_end_skipping ();
EXTERNC void pmint_request_end_simulation ();
EXTERNC void pmint_set_thread_type (int tid, /*thread_type_t*/int type);
EXTERNC int  pmint_get_thread_type (/*thread_type_t*/int type);
EXTERNC void pmint_record_event (int tid, long event, long value);
EXTERNC void pmint_done();
EXTERNC void pmint_usage();
EXTERNC unsigned long pmint_get_sim_cycle();
EXTERNC unsigned long PMINT_get_instr_issue_count();
EXTERNC void pmint_thread_sleep(int tid);
EXTERNC void gdbserver_init();
EXTERNC void gdbserver_done();

EXTERNC void aint_init(int argc, char **argv, char **envp);
EXTERNC void aint_done();
EXTERNC unsigned long AINT_fetch_issue_commit_next_instr(int tid, unsigned long predicted_pc);
EXTERNC unsigned long AINT_fetch_issue_next_instr(int tid, unsigned long predicted_pc);
EXTERNC unsigned long AINT_fetch_next_instr(int tid, unsigned long pc);
EXTERNC unsigned long AINT_issue_branch(int tid, unsigned long instid); 
EXTERNC void AINT_issue_other(int tid, unsigned long instid); 
EXTERNC unsigned long AINT_get_reg_value(int tid, instid_t instid, int reg_arg);

#ifdef NEW_TTL
EXTERNC unsigned long AINT_TTL_get_reg_value(int, instid_t, int, UINT64*, UINT64);
#endif

EXTERNC void AINT_do_read(int tid, unsigned long instid); 
EXTERNC void AINT_do_spec_write(int tid, unsigned long instid); 
EXTERNC void AINT_do_write(int tid, unsigned long instid);
EXTERNC void AINT_stc_failed(int tid, unsigned long instid);
EXTERNC void AINT_commit_instr(int tid, unsigned long instid);
typedef enum {KT_mispredict, KT_trap} Kill_Type;
EXTERNC void AINT_kill_instrs(int tid, unsigned long instid, Kill_Type reason);

EXTERNC thread_ptr Threads;
EXTERNC INT32 ProcessArgc[MAX_NTHREADS];
EXTERNC char **ProcessArgv[MAX_NTHREADS];

static ASIM_THREAD threads[MAX_NTHREADS];
static UINT32 thread_type[MAX_NTHREADS];
static ASIM_INST asim_inst_being_issued[MAX_NTHREADS];
static UINT32 numthreads = 0;

static UINT32 max_nthreads;
static UINT32 cur_nthreads;
static UINT64 thread_mask;
static UINT64 thread_active_mask;
static UINT32 stop_skipping;

// commit verifier
extern int aint_use_verifier_thread;
static Verifier *verifier;

// oracle
extern int aint_use_oracle_thread;
static AINT_ORACLE oracle;

// markers
static AINT_MARKER_CLASS marker;

EXTERNC void find_proc_addrs(char *objfile, char **symbol_names, unsigned long *symbol_addrs);

INT32 debug_level = 0;

EXTERNC INT32 in_FEED_Skip;

#ifdef NEW_TTL
extern int ttl_debug;
#endif
/********************************************************************
 *
 * 'PM' callbacks from AINT
 *    which are intercepted by the feeder
 *
 *******************************************************************/


#define AINT_WRONGPATH_ID  UINT64_MAX


static ALPHA_INSTRUCTION
CreateRegOp (INT32 op, INT32 func, UINT32 rA, UINT32 rB, UINT32 rC)
/*
 *  Generic 3 address operation Ra,Rb,Rc
 */
{
    ALPHA_INSTRUCTION ainst;

    ainst.OpReg.Opcode = op;
    ainst.OpReg.Function = func;
    ainst.OpReg.SBZ = 0;
    ainst.OpReg.RbvType = 0;
    ainst.OpReg.Ra = rA;
    ainst.OpReg.Rb = rB;
    ainst.OpReg.Rc = rC;
    
    return(ainst);
}


int
sim_init(int argc, char *argv[])
{
  extern int optind;
  extern int aint_trace_state_changes;
  extern char *job_separator;
  int i;

  if (strcmp(argv[optind], job_separator) == 0) {
    optind++;
    while (strcmp(argv[optind], job_separator) != 0) {
      if (strcmp(argv[optind], "-tc") == 0) {
	aint_trace_state_changes = 1;
	optind += 1;
      }
    }
    optind++;
  }
  return optind;
}

void
informative(char *s, ...)
{
  va_list ap;
  
  /* Federico Ardanaz notes:
   * when debugging trace mode is very usefull but 
   * if you put trace messages to any instruction (skipped or not)
   * the amount of information is too much! so I decided to protect
   * all the TRACE macro uses with an if as follows:
  */
  if (in_FEED_Skip==0)
  {
      TRACE(Trace_Feeder, 
       printf("AINT: ");
       va_start(ap, s);
       vprintf(s, ap);
       va_end(ap);
      ); 
  }
}

void
informative_ttl(int level, char *s, ...)
{
  va_list ap;

#ifdef NEW_TTL
 if ( ttl_debug >= level ) 
 {
      if (in_FEED_Skip==0)
      {
          TRACE(Trace_Feeder, 
           printf("AINT: ");
           va_start(ap, s);
           vprintf(s, ap);
           va_end(ap);
          ); 
      }
 }
#endif
}

void pmint_init(INT32 max_nthreads_, INT32 cur_nthreads_)
{

  int tid;

  max_nthreads = max_nthreads_;
  cur_nthreads = 0;

  thread_mask = 0;
  thread_active_mask = thread_mask;

  if (!Quiet)
  {
      for (tid = 0; tid < cur_nthreads_; tid++) {
	  int argc = ProcessArgc[tid];
	  int i;
	  fprintf(Aint_output, "PMINT-NULL: tid=%d argc=%d ", tid, ProcessArgc[tid]);
	  for (i = 0; i < argc; i++) {
	      fprintf(Aint_output, "%s ", ProcessArgv[tid][i]);
	  }
	  fprintf(Aint_output, "\n");
      }
      
      fprintf(Aint_output, "PMINT_INIT maxthreads=%d ====================================================\n", max_nthreads_);
  }

  //
  // note: verifier and oracle are mutually exclusive, since both of them
  // implicitly run in AINT's thread=1 while the timing simulation is running
  // in AINT's thread=0; we check for this invariant here;
  //
  if (aint_use_verifier_thread && aint_use_oracle_thread) {
    ASIMERROR("Can't run Verifier and Oracle at the same time!\n");
  }

  /* check to see if we have exactly two threads for verifier */
  if (aint_use_verifier_thread) {
    if (cur_nthreads_ != 2) {
      ASIMERROR("Can only run single-threaded in verifier mode!\n");
    }
  }

  /* check to see if we have exactly two threads for oracle */
  if (aint_use_oracle_thread) {
    if (cur_nthreads_ != 2) {
      ASIMERROR("Can only run single-threaded in oracle mode!\n");
    }
  }

  for (tid = 0; tid < cur_nthreads_; tid++) {
    pmint_thread_begin(tid);
  }

  if (!Quiet) 
      fprintf(Aint_output, "PMINT_INIT nthreads=%d ====================================================\n", cur_nthreads_);
}

void pmint_thread_begin(INT32 tid)
{
    UINT32 pid;
    UINT64 vpc;

    thread_ptr pthread;

    pthread = &Threads[tid];
    vpc =  pthread->next_fetch_picode->addr;

    if (aint_use_verifier_thread && tid == 1) {
      /* this is verifier thread - functional simulation only */
      verifier = new Verifier( tid, vpc );

      /* don't create ASIM thread for verifier */
      return;
    }

    if (aint_use_oracle_thread && tid == 1) {
      /* this is oracle thread - functional simulation only */
      oracle = new Oracle( tid, vpc );

      /* don't create ASIM thread for oracle */
      return;
    }

    pid = pthread->process->pid;

    thread_mask |= (1l << tid);
    if (pthread->runstate == R_RUN)
      thread_active_mask |= (1l << tid);
    cur_nthreads++;
    if (cur_nthreads > max_nthreads)
      max_nthreads = cur_nthreads;
    
    //
    // Create the object to represent this thread. Assign
    // a unique number to each new thread.

    ASIM_THREAD thread = new ASIM_THREAD_CLASS(tid, pid, vpc);
    threads[tid] = thread;
    posix_register_aint_thread(pthread); 
    //
    // Send it to the PM...

    if (!Quiet) fprintf(Aint_output, "Starting new thread, tid=%d, pid = "FMT32D", at pc "FMT64X".\n", tid, pid, vpc);

#if PTHREAD_DEBUG
    printf("pmint_thread_begin(%d)\n", tid);
#endif
    CMD_ThreadBegin(thread);
}

void pmint_thread_end(INT32 tid)
{
    if (aint_use_verifier_thread && tid == 1) {
      /* this is verifier thread - functional simulation only */
      /* don't destroy ASIM thread for verifier */
      return;
    }

    if (aint_use_oracle_thread && tid == 1) {
      /* this is oracle thread - functional simulation only */
      /* don't destroy ASIM thread for oracle */
      return;
    }

    thread_mask &= ~(1l << tid);
    thread_active_mask &= ~(1l << tid);
    cur_nthreads--;

#if PTHREAD_DEBUG
    printf("pmint_thread_end(%d)\n", tid);
#endif
    CMD_ThreadEnd(threads[tid]);
}

void pmint_thread_block(INT32 tid)
{
    ASIM_INST inst;

    inst = asim_inst_being_issued[tid];
    ASSERTX(inst);
    ASSERTX(inst->ForceNonSpec());
    ASSERTX(inst->IsCallSys());
    thread_mask &= ~(1l << tid);
    // set pc to continue at
#if PTHREAD_DEBUG
    printf("pmint_thread_block(%d):inst->VirtualPc + 4 = %lx\n",tid, inst->GetVirtualPC() + 4);
#endif
    threads[tid]->SetVirtualPc(inst->GetVirtualPC() + 4);
    CMD_ThreadBlock(threads[tid], inst);
}

void pmint_thread_unblock(INT32 tid)
{
    thread_mask |= (1l << tid);
#if PTHREAD_DEBUG
    printf("pmint_thread_unblock(%d)\n", tid);
#endif
    CMD_ThreadUnblock(threads[tid]);
}

unsigned long pmint_get_sim_cycle()
{
  return asimSystem->SYS_Cycle();
}

unsigned long PMINT_get_instr_issue_count()
{
    return asimSystem->SYS_GlobalCommittedInsts(); 
}

void pmint_set_thread_type (int tid, int type)
{
  thread_type[tid] = type;
}

int pmint_get_thread_type (int tid)
{
   return thread_type[tid];
}

void pmint_quiesce_thread(int tid, void *picode, unsigned long addr) {}
void pmint_request_begin_skipping (long how_many_to_skip) {}
void pmint_request_end_skipping () { stop_skipping = 1; }
void pmint_request_end_simulation () {}
void pmint_record_event (int tid, long event, long value) {}

void pmint_done()  {}
void pmint_usage() {}
void sim_done(double a, double b) {}

void gdbserver_init() {}
void gdbserver_done() {}

void
FEED_Verify (ASIM_INST inst, UINT32 tid, UINT64 instid)
/*
 * run verifier step for to-be commited instruction instid
 * Note: This has to be called _before_ inst is committed in order to make
 * sure that all of the information that inst references is still alive!
 */
{
    thread_ptr pthread = &Threads[tid];
    inflight_inst_ptr ifi = &pthread->cbif[instid];

    if (inst) {
      verifier->Step( inst, ifi, VerifierCheck );
    } else {
      verifier->Step( inst, ifi, VerifierSkip );
    }
}

inline void
FEED_VerifySkip (UINT32 tid, UINT64 instid)
{
    FEED_Verify( NULL, tid, instid );
}


/********************************************************************
 *
 *                       AINT oracle interface
 *
 * each instruction can be ornamented with an oracle shadow; the oracle
 * is able to answer questions about the results and behavior of the
 * instruction before the instruction itself has executed. E.g. the oracle
 * is used to get branch results, effective address, and register values
 * that will be produced by the instruction.
 ********************************************************************/

//
// skip one instruction in the oracle at address pc
//
inline void
FEED_OracleSkip(UINT64 pc)
{
  oracle->Skip(pc);
}

//
// fetch one instruction in oracle at address pc and associate this
// oracle shadow with real instruction inst
//
inline AINT_ORACLE_INST
FEED_OracleFetch(ASIM_INST inst, UINT64 pc)
{
  return (oracle->Fetch(inst, pc));
}

//
// commit the oracle shadow associated with real instruction inst
//
inline void
FEED_OracleCommit(ASIM_INST inst)
{
  oracle->Commit(inst);
}

//
// kill the oracle shadow associated with real instruction inst; this
// also kills all younger instructions;
//
inline void
FEED_OracleKill(ASIM_INST inst, bool mispredict)
{
  oracle->Kill(inst, mispredict);
}


/********************************************************************
 * FEED_Marker
 *
 * Marker manipulation: set / clear
 * A marker is a binary flag on a static instruction.
 * We support up to 64 markers in this feeder. Each marker ID maps to
 * a unique bit in a bit vector (UINT16). We store one 64-bit vector
 * per static instruction.
 *******************************************************************/
void
FEED_Marker (ASIM_MARKER_CMD cmd, ASIM_THREAD thread,
             UINT32 markerID, IADDR_CLASS markerPC,
             UINT32 instBits, UINT32 instMask)
{
  ASSERT (markerID < MAX_MARKER, "marker ID outside valid range");
  UINT32 tid = thread->Uid();

  switch (cmd) {
    case MARKER_CLEAR_ALL:
      marker.ClearAll (tid, markerID);
      printf("Clearing marker %d on thread %d at all addresses\n",
        markerID, tid);
      break;
    case MARKER_CLEAR_PC:
      marker.ClearPc (tid, markerID, markerPC);
      printf("Clearing marker %d on thread %d at pc 0x%lx\n",
        markerID, tid, markerPC.GetAddr());
      break;
    case MARKER_SET_PC:
      marker.SetPc (tid, markerID, markerPC);
      printf("Setting marker %d on thread %d at pc 0x%lx\n",
        markerID, tid, markerPC.GetAddr());
      break;
    case MARKER_SET_INST:
      marker.SetInst (tid, markerID, instBits, instMask);
      printf("Setting marker %d on thread %d for all insts matching"
             " (0x%08x 0x%08x)\n", markerID, tid, instBits, instMask);
      break;
    default:
      ASIMERROR("FEED_Marker: invalid marker command <" << cmd << ">\n");
  }
}


/********************************************************************
 * FEED_Symbol
 *
 * Symbol table query
 *******************************************************************/
UINT64
FEED_Symbol (ASIM_THREAD thread, char* name)
{
  UINT32 tid = thread->Uid();
  return(FEED_Symbol(tid, name));
}

UINT64
FEED_Symbol (UINT32 tid, char* name)
{
  char *names[2];
  UINT64 addrs[2] = { 0, 0 };
  thread_ptr pthread;

  //
  // Obtain the Object file name for this thread
  //
  pthread = &Threads[tid];
  ASSERT(pthread->Objname != NULL,"FEED_Symbol invalid thread\n");

  //
  // Build array with desired subroutine name
  //
  names[0] = name;
  names[1] = NULL;
  find_proc_addrs(pthread->Objname,names,addrs);

  if ( addrs[0] == (UINT64)-1 ) {
      addrs[0] = 0;
  }

  return(addrs[0]);
}


/********************************************************************
 *
 * FEEDer callbacks
 *
 *******************************************************************/

UINT64
FEED_Skip (ASIM_THREAD thread, UINT64 n, INT32 markerID)
{
    UINT64 instructions = 0;
    UINT32 tid, startid, maxid;
    UINT64 nextpc[MAX_NTHREADS];
    thread_ptr pthread;
    inflight_inst_ptr ifi;
    extern int aint_trace_state_changes;

    in_FEED_Skip = 1;

    if(thread != NULL)
      startid = maxid = thread->Uid();
    else
      startid = 0, maxid = max_nthreads - 1;

    if(!n) n = UINT64_MAX;

    // determine starting pc for each thread
    for(tid = startid; tid <= maxid; tid++) {
      if(thread_active_mask & (1 << tid)) {
        pthread = &Threads[tid];
        ifi = &pthread->cbif[pthread->instid];

        nextpc[tid] = ifi->nextpc;
        if((INT64)pthread->instid < 0) {
          nextpc[tid] = threads[tid]->StartVirtualPc();
          pthread->instid = 0;
        }
        // print what we are doing
        printf("Start Skipping: thread %d at pc 0x%lx",tid,nextpc[tid]);
        if (n != UINT64_MAX) {
          printf(" for %lu instructions", n);
          if (markerID >= 0) {
            printf(" or");
          }
        }
        if (markerID >= 0) {
          printf(" until marker %d", markerID);
        }
        printf("\n");
      }
    }

    stop_skipping = 0;

    // fast fetch instructions until all requested instructions are fetched
    // or we get a request to stop skipping
    while (thread_active_mask != 0 && instructions < n && !stop_skipping) {
      // round-robin the threads
      for(tid = startid; tid <= maxid; tid++) {
        if (thread_mask & (1 << tid)) {
          UINT64 predicted_pc;
          UINT64 instid;

          instructions++;  // completed another instruction

          // Fetch
          predicted_pc = nextpc[tid];
	  
	  // this trace statement is commented out because it prints WAY too
	  // much info in a standard trace file
	  //          TRACE(Trace_Feeder, printf("\tFEED_Skip: thread "FMT32D" skips pc="FMT64X" bad path\n",tid,  predicted_pc));

          if (aint_use_verifier_thread) {
            instid = AINT_fetch_issue_next_instr(tid, predicted_pc);
            FEED_VerifySkip(tid, instid);
            AINT_commit_instr(tid, instid);
          } else {
            instid = AINT_fetch_issue_commit_next_instr(tid, predicted_pc);
            if (aint_use_oracle_thread) {
              FEED_OracleSkip(predicted_pc);
            }
          }

          pthread = &Threads[tid];
          ifi = &pthread->cbif[instid];
          nextpc[tid] = ifi->nextpc;

          // stop if we hit a marker;
          // putting this at the end of the loop makes sure we step over
          // this marker if we enter FEED_Skip again with the PC still at
          // the marker, i.e. the first insn is always executed!
          if (markerID >= 0 && marker.IsSet (tid, markerID, nextpc[tid])) {
            stop_skipping = TRUE;
          }
        }
      } // for
    } // while

    // Fixup code so PM and AINT can continue correctly
    for(tid = startid; tid <= maxid; tid++) {
      if(thread_active_mask & (1 << tid)) {
        pthread = &Threads[tid];

        // Mark all pregs mapped to current logical regs as valid
        for (INT32 lreg=0; lreg < TOTAL_LOGICALS; lreg++) {
          INT32 pr = pthread->RegMap[lreg];
          pthread->RegValid[pr >> 5] |= (1 << (pr&0x1f));
        }

        // set pc to continue at
        threads[tid]->SetVirtualPc(nextpc[tid]);
        if (in_FEED_Skip==0)
            TRACE(Trace_Feeder, printf("\tFEED_Skip: thread "FMT32D" SetVirtualPc to pc="FMT64X" at the end of skipping\n",tid,  nextpc[tid]));
        if (aint_use_oracle_thread) {
          oracle->SetPc(nextpc[tid]);
        }
        // print what we are doing
        printf("End Skipping: thread %d",tid);
        if (markerID >= 0) {
          printf(" when hitting marker %d", markerID);
        }
        printf(" at pc 0x%lx\n",nextpc[tid]);
      }
    }

    in_FEED_Skip = 0;

    printf("End Skipping: %lu instructions have been skipped.\n",instructions);

    //aint_trace_state_changes = 1;
    return instructions;
}

//
// This routine "manufactures" a fake physical PC based on the Aint page frame address (translated from the target
// virtual page address) + the virtual page offset. 
// 
IADDR_CLASS
FEED_ITranslate(ASIM_THREAD thread, IADDR_CLASS vpc)
{
    thread_ptr pthread = &Threads[thread->Uid()]; 

    // This typecast is ok because this gives the physical page frame in aint's address space
    // corresponding to the virtual page in the target address space. 
    UINT64 ppc = (UINT64)addr2iphys(pthread->process, (long)(vpc & ~((long)(TB_PAGESIZE - 1))), (long *)NULL); 
    ASSERTX((ppc & ((UINT64)(TB_PAGESIZE - 1))) == 0); 
    ppc |= TB_OFFSET(vpc); 
    return ppc; 
}


void
FEED_Fetch (ASIM_THREAD thread, IADDR_CLASS predicted_pc, ASIM_INST inst)
/*
 * Fetch next instruction from 'thread'.
 */
{
    UINT64 instid;
    UINT32 tid = thread->Uid();

    thread_ptr pthread = &Threads[tid];
    inflight_inst_ptr ifi;
    icode_ptr picode;      
    UINT64 iflags;
    UINT64 ppc; 

    inst->SetThread(thread);

    instid = AINT_fetch_next_instr(tid, predicted_pc);
    if(instid == AINT_WRONGPATH_ID) {
      // fetching to unknown address so must use NO-OP to fake bad path
      TRACE(in_FEED_Skip == 0 && Trace_Feeder, 
	    printf("\tFEED_Fetch down unknown address "FMT64X" bad path\n", predicted_pc.GetAddr()));

      inst->SetInstrNoCache(predicted_pc, FEED_ITranslate(thread, predicted_pc), 
			    CreateRegOp(BIT_OP, BIS_FUNC, ZERO_REG, ZERO_REG, ZERO_REG));
      inst->SetIdentifier(AINT_WRONGPATH_ID);
      return;
    }

    TRACE(in_FEED_Skip == 0 && Trace_Feeder, 
	  printf("\tFEED_Fetch T"FMT32D":"FMT64X" index "FMT64D"\n", tid,
		 predicted_pc.GetAddr(), instid)); 
          
    ifi = &pthread->cbif[instid];
    picode = ifi->picode;      
    iflags = picode->iflags;

    // the PM is still on trace, so grab the next instruction.
    //
    //r2r inst->SetInstr(predicted_pc, picode->instr, picode->extended_instr);
    inst->SetInstrNoCache(predicted_pc, FEED_ITranslate(thread, predicted_pc), picode->instr, picode->extended_instr);
    inst->SetIdentifier(instid);

    //
    // At this point, we do not know the true actual target. The ActualTarget only
    // becomes meaningful after the ASIM model has called 'FEED_Issue' (see below).
    // However, leaving the 'ActualTarget' field uninitialized causes problems when
    // diffing two cycle-by-cycle trace dumps from ASIM (because the variable is uninitialized,
    // different executables return different flavours of garbage).
    // Therefore, to make the output of ASIM more deterministic we will set the ActualTarget to -1
    // at this point
    //
    if (inst->IsBranch() || inst->IsJump()) inst->SetActualTarget(0xffffffffffffffff);

    if (inst->IsCallPal() || inst->IsRpcc()) {

	if (in_FEED_Skip==0)
        TRACE(Trace_Feeder, printf("\tFEED_Fetch NonSpec T"FMT32D":"FMT64X" index "FMT64D" %s\n", tid,
				   predicted_pc.GetAddr(), instid, inst->GetDisassembly())); 

	inst->SetForceNonSpec(TRUE);
	inst->SetForceInOrder(TRUE);
    }

    if (aint_use_oracle_thread) {
      FEED_OracleFetch(inst, predicted_pc);
    }
}

UINT64
FEED_GetRegValue (ASIM_INST inst, RegOps regOp)
{
  UINT32 tid = inst->GetThread()->Uid();
  UINT64 instid = inst->GetIdentifier();
  Arg reg_arg;
  
  if (inst->IsLoad())
    switch (regOp)
    {
      case Src1: reg_arg = RB; break;
      case Dest:  reg_arg = RA; break;
      default: ASSERTX(0);
    }

  else if (inst->IsStore())
    switch (regOp)
    {
      case Src1: reg_arg = RA; break;
      case Src2: reg_arg = RB; break;
      default: ASSERTX(0);
    }
  else if (inst->IsOperate() || inst->IsFPOperate())
    switch (regOp)
    {
      case Src1: reg_arg = RA; break;
      case Src2: reg_arg = RB; break;
      case Dest:  reg_arg = RC; break;
    }
  else if (inst->IsJsr())
    switch (regOp)
    {
      case Src1: reg_arg = RA; break;
      case Src2: reg_arg = RB; break;
      default: ASSERTX(0);
    }
  else
  {
    printf("FEED_GetRegValue() not implemented for this inst type yet\n");
    ASSERTX(0);
  }
  
  return AINT_get_reg_value(tid, instid, reg_arg);

}

#ifdef NEW_TTL
// Federico Ardanaz notes:
// Just a vector version that overloads the previous one, varray must an UINT64 vector where
// we put all the data. fillVL is length of the needed vector. We return a boolean indicating success
//
bool FEED_GetRegValue (ASIM_INST inst, RegOps regOp, UINT64* varray, UINT64 fillVL)
{
  UINT32 tid = inst->GetThread()->Uid();
  UINT64 instid = inst->GetIdentifier();
  Arg reg_arg;
  int isGathScatt;

  if (! inst->IsVector()) return (false);
  
  isGathScatt = inst->IsVectorGather()||inst->IsVectorScatter();
  
  if (inst->IsVectorLoad())
    switch (regOp)
    {
      case Src1: reg_arg = RB; break;
      case Dest:  reg_arg = RA; break;
      default: ASSERTX(0);
    }

  else if (inst->IsVectorStore())
    switch (regOp)
    {
      case Src1: reg_arg = RA; break;
      case Src2: reg_arg = RB; break;
      default: ASSERTX(0);
    }
  else if (inst->IsVectorOperate() && (!isGathScatt))
    switch (regOp)
    {
      case Src1: reg_arg = RA; break;
      case Src2: reg_arg = RB; break;
      case Dest:  reg_arg = RC; break;
      default: ASSERTX(0);
    }
  else if (inst->IsVectorGather())
  {
    switch (regOp)
    {
      case Src1: reg_arg = RA; break;
      case Src2: reg_arg = RB; break;
      case Dest: reg_arg = RC; break;
      default: ASSERTX(0);
    }
  }
  else if (inst->IsVectorScatter())
  {
    switch (regOp)
    {
      case Src1: reg_arg = RA; break;
      case Src2: reg_arg = RB; break;
      case Src3: reg_arg = RC; break;
      default: ASSERTX(0);
    }
  }
  else if (inst->IsMVTVP())
  {
    switch (regOp)
	{
      case Src1: reg_arg = RA; break;
      case Dest: reg_arg = RC; break;
      default: ASSERTX(0);
	}
    varray[0] = AINT_get_reg_value(tid, instid, reg_arg); 
	return (true);
  }
  else
  {
    printf("FEED_GetRegValue() (vector version) not implemented for this inst type yet\n");
    ASSERTX(0);
  }
  
  AINT_TTL_get_reg_value(tid, instid, reg_arg, varray, fillVL);
  return (true);

}

// direct getting
UINT64
FEED_GetDirectRegValue (ASIM_INST inst, int Pos)
{
  UINT32 tid = inst->GetThread()->Uid();
  UINT64 instid = inst->GetIdentifier();

  // pos must be RA/RB/RC
  return AINT_get_reg_value(tid, instid, Pos);	
}

#endif

void
FEED_SetEA(ASIM_INST inst, char vf, UINT64 vea)
{
    UINT32 tid = inst->GetThread()->Uid();
    UINT64 instid = inst->GetIdentifier();
    thread_ptr pthread = &Threads[tid];
    inflight_inst_ptr ifi = &pthread->cbif[instid];
    
    ifi->veaFlag = vf; 
    ifi->veaExternal = vea; 
}


void
FEED_Issue (ASIM_INST inst)
/*
 * Issue 'inst'...
 */
{
    UINT32 tid = inst->GetThread()->Uid();
    UINT64 instid = inst->GetIdentifier();
    thread_ptr pthread = &Threads[tid];
    inflight_inst_ptr ifi = &pthread->cbif[instid];

    if(instid == AINT_WRONGPATH_ID) {
      if (in_FEED_Skip==0)
        TRACE(Trace_Feeder, printf("\tFEED_Issue T"FMT32D" wrongpath instruction\n", tid)); 
      return;
    }
    asim_inst_being_issued[tid] = inst;

#ifdef NEW_TLDS
    if (inst->IsTLDSArm() || inst->IsTLDSEpochNumber()){
      thread_ptr pthread = &Threads[tid];
      inflight_inst_ptr ifi = &(pthread->cbif[instid]);
      int pr = ifi->args[ifi->picode->args[RA]];
      inst->SetVirtualEffAddress(REG(RA));
    }
#endif
    if (in_FEED_Skip==0)
        TRACE(Trace_Feeder, printf("\tFEED_Issue T"FMT32D": index "FMT64D"\n", tid, instid)); 
#ifdef NEW_TTL
    if(inst->IsMemRead() || inst->IsMemWrite()) {
#else
    if(inst->IsLoad() || inst->IsStore()) {
#endif
      //
      // Roger: In this new version, FEED_Issue really does nothing but read the register
      //        file and compute the effective address. WE ARE NOT entering the store into
      //        the store buffer (that will happen much later, under the direction of the PM,
      //        when  FEED_SpecStore() is invoked
      //
      AINT_issue_other(tid, instid);
      inst->SetVirtualEffAddress(Threads[tid].vaddr);
	  
      // we take the effective address in AINT address space as physical
      // address in object address space - just to get something that could
      // be a physical address

      //
      // Julio Gago @ BSSAD, 07/13/2001
      //
      // TLB translation for absurd addresses (not valid in the address
      // space of the process, generated in the bad path) results in '0' as
      // the physical address. However, we MUST return a physical address
      // with the same low log2(PGSIZE) bits of the virtual address in order
      // to keep VEA/PEA pairs consistent.
      //
      // inst->SetPhysicalEffAddress(Threads[tid].paddr ? Threads[tid].paddr : TB_OFFSET(Threads[tid].vaddr));
	  
	  // Federico Ardanaz, 2001.07.19
	  // Use the feeder virtual address as the physical address in the model is not a good idea.
	  // When running vector code we need a coherent translation mechanism among scalar and
	  // vector sides. Right now the MBOX is using 64Kb pages and the VBOX 512Mb ones...
	  // TODO: Anyway we must unify the page size some point in the future.
	  
	  // We use 2 bits as a TPU designator and "or" it to the virtual address.
	  // we use the last (high order) 2 bits of the "offset" in the PA (bits 0 to 36).
	  // keep in mind that bits above position 36 are used to designate procesor number and
	  // so on.
	  
	  UINT64 tpu_mask = (UINT64)inst->GetThread()->Tpu();
	  tpu_mask<<=35;
	  inst->SetPhysicalEffAddress(Threads[tid].vaddr|tpu_mask);
	  
      
      if (in_FEED_Skip==0)
        TRACE(Trace_Feeder, printf("\tFEED_Issue : l/s vaddr=0x%lx\n", Threads[tid].vaddr)); 
    }
    else if (inst->IsBranch() || inst->IsJump()) {
      UINT64 target, predicted_pc, real_pc;
      thread_ptr pthread = &Threads[tid];
      inflight_inst_ptr ifi;

      ifi = &pthread->cbif[instid];
      real_pc = ifi->pc;
      predicted_pc = inst->GetVirtualPC();

      target = AINT_issue_branch(tid, instid);
      if(predicted_pc == real_pc)
          inst->SetActualTarget(target);
      else {
          if (in_FEED_Skip==0)
            TRACE(Trace_Feeder, printf("\tFEED_Issue fix: predpc="FMT64X" realpc="FMT64X" target="FMT64X" fixedtarg="FMT64X"\n", predicted_pc, real_pc, target, predicted_pc - real_pc + target)); 
          inst->SetActualTarget(predicted_pc - real_pc + target);
      }
      if (in_FEED_Skip==0)
        TRACE(Trace_Feeder, printf("\tFEED_Issue : branch targ="FMT64X"\n", target)); 
    }
    else {
      AINT_issue_other(tid, instid);
    }

#ifdef NEW_TTL
    //
    // Note that ALL instructions (whether vector or not) are getting VL and VS. This is convenient when 
    // computing stats, since we can simply ask for the inst->GetVl() and we get a '1' for scalar instructions
    // and something else for vector instructions.
    //
    if ( inst->IsVector() ) informative_ttl(1,"\tPASSING VL=%lu from pr %d VS=%ld from pr %d\n",VL, ifi->vl,VS,ifi->vs);
    if ( inst->IsVectorMem() || inst->IsVectorOperate() ) {
     inst->SetVl(VL);
     inst->SetVs((INT64)VS);
    }
    else {
     inst->SetVl(1);
     inst->SetVs(0);
    }
	
    if ( inst->IsVector() ) informative_ttl(1,
	 "\tPASSING VMH=%lx, VML=%lx from pr %d\n",VMH,VML, ifi->vm);
	 
	inst->SetVML(VML);
	inst->SetVMH(VMH);
	
#endif
    asim_inst_being_issued[tid] = NULL;

}

void
FEED_DoSpecWrite (ASIM_INST inst)
/*
 * 'inst' is a store entering the STQ in the performance model. Here in AINT we
 * must update out internal store buffer.
 */
{
    UINT32 tid = inst->GetThread()->Uid();
    UINT64 instid = inst->GetIdentifier();

    if (in_FEED_Skip==0)
        TRACE(Trace_Feeder, printf("\tFEED_DoSpecStore T"FMT32D": index "FMT64D"\n", tid, instid)); 
        
    AINT_do_spec_write(tid, instid);    
}

void
FEED_DoRead (ASIM_INST inst)
/*
 * 'inst' is a load accessing the cache
 */
{
    UINT32 tid = inst->GetThread()->Uid();
    UINT64 instid = inst->GetIdentifier();

    if (in_FEED_Skip==0)
        TRACE(Trace_Feeder, printf("\tFEED_DoRead T"FMT32D": index "FMT64D"\n", tid, instid)); 
        
    AINT_do_read(tid, instid);    
}


void
FEED_DoWrite (ASIM_INST inst)
/*
 * Inst is a store making the stored value non-speculative
 */
{
    UINT32 tid = inst->GetThread()->Uid();
    UINT64 instid = inst->GetIdentifier();

    if (in_FEED_Skip==0)
        TRACE(Trace_Feeder, printf("\tFEED_DoWrite T"FMT32D": index "FMT64D"\n", tid, instid)); 
        
    AINT_do_write(tid, instid);
}


void
FEED_Commit (ASIM_INST inst)
/*
 * 'inst' is committed...
 */
{
    UINT32 tid = inst->GetThread()->Uid();
    UINT64 instid = inst->GetIdentifier();


    if (in_FEED_Skip==0)
        TRACE(Trace_Feeder, cout << "\tFEED_Commit id=" << instid << " pc=" << inst->GetVirtualPC() << endl);
    if (instid == AINT_WRONGPATH_ID) {
        ASIMERROR("FEED_Commit: Attempt to commit wrongpath instruction " << inst->GetVirtualPC() << ": " << inst->GetDisassembly() << endl);
        return;
    }
    
    /*
     * check commited instruction against the verifier
     */
    if (verifier) {
        FEED_Verify(inst, tid, instid);
    }

    /*
     * commit oracle instruction
     */
    if (aint_use_oracle_thread) {
        FEED_OracleCommit(inst);
    }

    AINT_commit_instr(tid, instid);

    INT32 markerID = asimSystem->SYS_CommitWatchMarker();
    if (markerID >= 0 && marker.IsSet (tid, markerID, inst->GetVirtualPC())) {
        asimSystem->SYS_CommittedMarkers()++;
#ifdef NEW_TTL		
	/* This is a little hack to ease tarantula studies, we dump on a specific
	 * file the following register each time we pass throughout a marker:
	 * < markerID, VirtualPC, Cycle, CommitedInstrucctions>
	 */
	 fprintf(Aint_marker_output,
	 "MarkerID:%d\t\tPC:%lx\t\tCycle:%ld\t\tCommited inst:%ld\n",
	 markerID,inst->GetVirtualPC(),asimSystem->SYS_Cycle(),asimSystem->SYS_GlobalCommittedInsts());  
	 fflush(Aint_marker_output);
#endif
    }
}


void
FEED_Kill (ASIM_INST inst, bool mispredict, bool killMe)
/*
 * kill 'inst'...
 */
{

    UINT32 tid = inst->GetThread()->Uid();
    UINT64 instid = inst->GetIdentifier();
    thread_ptr pthread = &Threads[tid];
    inflight_inst_ptr ifi = &pthread->cbif[instid];

/*
    icode_ptr picode = ifi->picode;
    UINT64 iflags = picode->iflags;
*/
    if (in_FEED_Skip==0)
        TRACE(Trace_Feeder, printf("\tFEED_Kill id="FMT64D" mp="FMT32D"  tid="FMT32D"\n", instid, mispredict, tid)); 
    if(mispredict)
      // give AINT the next instruction id if branch */
      instid = SUCC_CIRC(instid, CBIF_SIZE);
    AINT_kill_instrs(tid, instid, mispredict ? KT_mispredict : KT_trap);
    // problem with flag mispredict -- AINT wants the next instruction after
    // branch, but we are giving it the branch itself, even though it tries
    // to backstep to the branch

    /*
     * kill oracle instruction
     */
    if (aint_use_oracle_thread) {
        FEED_OracleKill(inst, mispredict);
    }
}


bool
FEED_Init (UINT32 argc, char **argv, char **envp)
/*
 * Main program for aint simulator-based performance
 * models. Communication between this simulator and
 * the performance model is through feeder.h
 * described interface.
 */
{
    aint_init(argc, argv, envp);
    return(TRUE);
}


void
FEED_Done (void)
/*
 * Cleanup..
 */
{
    for(UINT32 t = 0; t < numthreads; t++)
        ;

    aint_done();
}

void
FEED_Usage (FILE *file)
{
    fprintf(file, "Please look at aint_init.c\n");
}

