/*
 * Copyright (C) 2000-2006 Intel Corporation
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

//----------------------------------------------------------------------------
// Oracle.cpp - AINT implementation of instruction oracle
//
// Author:  Artur Klauser
//
//----------------------------------------------------------------------------

// generic
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

// ASIM local module
extern "C" {
#include "icode.h"
#include "pmint.h"
void informative(char *s, ...);
unsigned long PMINT_get_instr_issue_count();
}

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/trace.h"

// ASIM local module
#include "Oracle.h"

extern thread_ptr Threads;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// the oracle
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//
// create the oracle thread
//
AINT_ORACLE_CLASS::Oracle(UINT32 _tid, IADDR_CLASS startPC)
  : pc(startPC), tid(_tid)
{
  informative("Oracle:: activated\n");
  isWrongpath = false;
  isStalled = false;
  nextSavedInst = NULL;
  lastInst = NULL;
}

//
// skip one instruction in oracle at address pc
//
void
AINT_ORACLE_CLASS::Skip(IADDR_CLASS fetchPc)
{
  //
  // fetch + execute + commit one instruction
  //
  AINT_fetch_issue_commit_next_instr(tid, fetchPc);
}

//
// fetch one oracle instruction at address pc and associate this
// oracle shadow with real instruction inst;
//
// we fetch and execute either a new oracle shadow from AINT or we
// reuse an old oracle shadow that was fetched and executed already
// before but was put on the saved instruction list when it was
// killed;
//
AINT_ORACLE_INST
AINT_ORACLE_CLASS::Fetch(ASIM_INST inst, IADDR_CLASS fetchPc)
{
  AINT_ORACLE_INST oracleInst;
  instid_t instIdOracle;
  instid_t nextSavedId;
  inflight_inst_ptr ifiOracle;
  inflight_inst_ptr ifiNextSaved;
  UINT32 iflagsOracle;
  bool wrongpath;

  // return immediately if oracle is stalled
  if (isStalled) {
    TRACE(Trace_Feeder, printf("\tFEED_OracleFetch stalled, not fetching PC "FMT64X"\n", fetchPc.GetAddr()));
    return (NULL);
  }

  // we are in wrong-path execution mode if we were in wrong-path before
  // or we are starting to fetch a wrong instruction now
  if (! isWrongpath & (pc != fetchPc)) {
    TRACE(Trace_Feeder, printf("\tFEED_OracleFetch transitioning to wrong path PC "FMT64X" should be "FMT64X"\n", fetchPc.GetAddr(), pc.GetAddr()));
  }
  wrongpath = isWrongpath | (pc != fetchPc);
  // set oracle global wrong-path mode to reflect this instruction
  isWrongpath = wrongpath;

  //
  // fetch the oracle shadow instruction
  //
  // if we have already executed instructions waiting to be refetched we
  // hand them out first, otherwise we fetch a new instruction
  oracleInst = nextSavedInst;
  if (oracleInst) {
    // we still have old saved shadows so we use them first
    ASSERT(oracleInst->GetPrev() == NULL, "bogus prev link on nextSavedInst\n");
    ASSERT(! oracleInst->IsWrongpath(), "wrongpath shadows in saved list\n");
    instIdOracle = oracleInst->GetInstId();
    ifiOracle = oracleInst->GetIfi();
    if (wrongpath) {
      // we have correct-path instructions saved, but we are transitioning
      // onto the wrong path now; we just stall the oracle in this case and
      // leave the saved instructions on the queue;
      isStalled = true;
      TRACE(Trace_Feeder, printf("\tFEED_OracleFetch stalling wrongpath with saved shadows present id="FMT64D"\n", instIdOracle));
      return(NULL);
    }

    // we must be on correct path now!
    ASSERT(ifiOracle->picode->addr == fetchPc,
      "pc mismatch on correct-path saved shadow fetch\n");
    // we are going to consume this saved oracle shadow, so advance to
    // next one now and cut this saved shadow lose from list;
    nextSavedInst = oracleInst->GetNext();
    oracleInst->Unlink();

    TRACE(Trace_Feeder, printf("\tFEED_OracleFetch from saved T"FMT32D":"FMT64X" id="FMT64D" wrongpath=%d\n", tid, fetchPc.GetAddr(), instIdOracle, wrongpath));

    // re-associcate new real instruction with the reused oracle shadow
    oracleInst->SetInst(inst);
  } else {
    // get a new instruction and execute it
    instIdOracle = AINT_fetch_next_instr(tid, fetchPc);

    TRACE(Trace_Feeder, printf("\tFEED_OracleFetch T"FMT32D":"FMT64X" id="FMT64D" wrongpath=%d\n", tid, fetchPc.GetAddr(), instIdOracle, wrongpath));

    // if the oracle is not stalled we issue the oracle shadow
    // instruction; the oracle will get stalled if it fetches a
    // non-speculative instruction (eg. a syscall) in wrong-path mode,
    // since it would be incorrect to execute this instruction due to
    // its non-undoable side effects; once the oracle gets stalled, it
    // can't execute instructions until we see a kill of an
    // instruction older than the stall point, i.e.  any kill of an
    // instruction that has an oracle shadow will do;
    if (wrongpath && inst->ForceNonSpec()) {
      isStalled = true;
      TRACE(Trace_Feeder, printf("\tFEED_OracleFetch stalling wrongpath for non-speculative instruction id="FMT64D"\n", instIdOracle));
      // kill this AINT instruction right away since its on the wrongpath,
      // and it will never get associated with a shadow;
      AINT_kill_instrs(tid, instIdOracle, KT_mispredict);
      return (NULL);
    }

    //
    // issue the oracle shadow instruction
    //
    ifiOracle = &(Threads[tid].cbif[instIdOracle]);
    iflagsOracle = ifiOracle->picode->iflags;

    // set up oracle with info to oracle shadow instruction
    oracleInst = new AINT_ORACLE_INST_CLASS(instIdOracle, ifiOracle, inst, wrongpath);

    oracleInst->SaveSrcReg(tid);

    AINT_issue_other(tid, instIdOracle);
    if ( iflagsOracle & (E_READ|E_LD_L)) {
      AINT_do_read(tid, instIdOracle);
    } else if ( iflagsOracle & (E_WRITE|E_ST_C)) {
      AINT_do_spec_write(tid, instIdOracle);
      // if we are on the correct path, we can immediately commit to memory;
      // otherwise we will never commit this write to memory, since it will
      // eventually be killed; in that case only the speculative store buffer
      // holds the write data temporarily until the write is killed;
        if (! wrongpath) {
          AINT_do_write(tid, instIdOracle);
        }
    }

    oracleInst->SaveDestReg(tid);
  }

  // associate oracle shadow with real instruction
  inst->SetOracle(ASIM_ORACLE_INST(oracleInst));

  // update oracle shadow list
  oracleInst->LinkAfter(lastInst);
  lastInst = oracleInst;
  ASSERT(lastInst->GetNext() == NULL, "bogus next link on lastInst\n");

  pc = oracleInst->GetNextPc();

  return (oracleInst);
}

//
// commit the oracle shadow associated with real instruction inst
//
void
AINT_ORACLE_CLASS::Commit(ASIM_INST inst)
{
  AINT_ORACLE_INST oracleInst = AINT_ORACLE_INST(inst->GetOracle());
  // every committed instruction has to have an oracle shadow instruction
  ASSERT(oracleInst, "commiting instruction has no oracle shadow\n");
  instid_t instIdOracle = oracleInst->GetInstId();

  TRACE(Trace_Feeder, printf("\tFEED_OracleCommit id="FMT64D"\n",instIdOracle));

  ASSERT(! oracleInst->IsWrongpath(), "committing wrongpath oracle shadow\n");

  // now we can verify original instruction against oracle shadow instruction
  oracleInst->VerifyStep();

  AINT_commit_instr(tid, instIdOracle);

  // disassociate from real instruction
  ASSERT(oracleInst->GetInst() == inst, "shadow instruction not matching with real instruction\n");
  inst->SetOracle(NULL);
  oracleInst->SetInst(NULL);

  // release oracle shadow instruction
  inst->SetOracle(NULL);
  oracleInst->Unlink();
  if (lastInst == oracleInst) {
    lastInst = NULL;
  }
  oracleInst->DecrRef();
  delete oracleInst;
}

//
// cleanup shadow list by disassociating all shadows from their real
// instructions; if shadow list transits into wrongpath, we completely
// destroy the wrongpath part of the list;
//
void
AINT_ORACLE_CLASS::CleanupShadowList(AINT_ORACLE_INST oracleInst)
{
  AINT_ORACLE_INST nextInst;
  while (oracleInst) {
    nextInst = oracleInst->GetNext();
    // destroy association with real instruction
    if (oracleInst->GetInst()) {
      oracleInst->GetInst()->SetOracle(NULL);
    } else {
      // if we don't have a real instruction anymore, we have cleaned up
      // this part of the list already at an ealier time, so we can stop now;
      break;
    }
    oracleInst->SetInst(NULL);
    // if this shadow or next shadow is on wrong path, cut link
    if (oracleInst->IsWrongpath() || (nextInst && nextInst->IsWrongpath())) {
      oracleInst->CutLinkAfter();
    }
    // if we are about to transition to wrong path, let AINT know that
    // it has to kill the wrong-path instructions;
    if (! oracleInst->IsWrongpath() && nextInst && nextInst->IsWrongpath()) {
      AINT_kill_instrs(tid, nextInst->GetInstId(), KT_mispredict);
    }
    // if this shadow is on wrong path, we are done with it
    if (oracleInst->IsWrongpath()) {
      oracleInst->DecrRef();
      delete oracleInst;
    }
    oracleInst = nextInst;
  }
}

//
// kill the oracle shadow associated with real instruction inst and
// also kill all younger instructions;
//
// if the killed instruction is on the correct (control) path then we
// save the oracle shadows in a saved instruction list and hand them
// out again from there as they are refetched; this is required in
// order to guarantee that AINT execution semantics are not violated
// by avoiding undo/redo of operations that have side effects, like e.g.
// system calls;
void
AINT_ORACLE_CLASS::Kill(ASIM_INST inst, bool mispredict)
{
  AINT_ORACLE_INST oracleInst = AINT_ORACLE_INST(inst->GetOracle());
  if (oracleInst == NULL) {
    // only reason for not having an oracle shadow is an oracle stall
    ASSERT(isStalled,
      "missing oracle shadow for non-stalled oracle kill\n");
    return;
  }

  instid_t instIdOracle = oracleInst->GetInstId();
  inflight_inst_ptr ifiOracle = oracleInst->GetIfi();
  AINT_ORACLE_INST killInst;
  instid_t killId;
  bool wrongpath;

  TRACE(Trace_Feeder, printf("\tFEED_OracleKill id="FMT64D" mp="FMT32D"\n", instIdOracle, mispredict));

  // if we see a kill while we are fetching saved shadows, the kill point must
  // be in the correct path before the next saved shadow; the kill point will
  // become the new next saved shadow; we must attach the saved shadow list
  // to the last shadow, however, otherwise we'd lose the current shadow link
  // in the process;
  if (nextSavedInst) {
    nextSavedInst->JoinAfter(lastInst);
  }

  if (mispredict) {
    pc = oracleInst->GetNextPc();
    lastInst = oracleInst;
    // get the oracle shadow after the branch */
    killInst = oracleInst->GetNext();
    if (killInst) {
      killId = killInst->GetInstId();
      wrongpath = killInst->IsWrongpath();
    } else {
      killId = INSTID_INVALID;
      wrongpath = false;
    }
  } else {
    pc = ifiOracle->picode->addr;
    lastInst = oracleInst->GetPrev();
    killInst = oracleInst;
    killId = instIdOracle;
    wrongpath = oracleInst->IsWrongpath();
  }
  if (lastInst) {
    lastInst->CutLinkAfter();
  }

  if (wrongpath) {
    ASSERT(! nextSavedInst, "wrongpath kill while saved shadows present\n");
    // kill this instruction and all newer instructions
    AINT_kill_instrs(tid, killId, mispredict ? KT_mispredict : KT_trap);
  } else {
    // the backend is killing correct-path instructions due to some
    // structural hazard in the microarchitecture; we can't kill oracle
    // instructions from the correct path, since they have already modified
    // architectural state (e.g. write to memory, syscall side effects);
    // so we have to peel off the oracle shadow instructions from the 
    // real instructions and save them in a queue to be handed out again when
    // we refetch the corresponding real instruction;
    if (killInst) {
      // killInst could be NULL if oracle was stalled and we still have
      // old saved instructions waiting to be refetched;
      nextSavedInst = killInst;
    }
  }
  // reset oracle global wrong-path mode to state of this instruction
  isWrongpath = oracleInst->IsWrongpath();

  // now peel off oracle shadows from real instructions and delete
  // oracle shadows for wrong-path instructions; correct-path shadows
  // stay on the saved list;
  CleanupShadowList(killInst);

  // any kill point that has an oracle shadow associated with it must
  // by definition be older than the stall point (since instructions past
  // the stall point have no oracle shadow), so we can unstall the oracle
  // if we see such a kill, i.e. get here in this kill code flow;
  if (isStalled) {
    isStalled = false;
    TRACE(Trace_Feeder, printf("\tFEED_OracleKill unstalling oracle\n"));
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// oracle shadow instructions
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

AINT_ORACLE_INST_CLASS::aint_oracle_inst_class(
  UINT64 id, inflight_inst_ptr ifi, ASIM_INST i, bool wp)
  : ASIM_ORACLE_INST_CLASS("OracleShadowInst", id),
    instIdOracle(id), ifiOracle(ifi), inst(i), isWrongpath(wp)
{
  next = NULL;
  prev = NULL;
}

//
// save the source register values to a local copy
//
void
AINT_ORACLE_INST_CLASS::SaveSrcReg(UINT32 tid)
{
  MMCHK;

  INT32 logReg;

  logReg = inst->GetSrc1();
  if (logReg >= 0) {
    reg[RegSrc1] = AINT_read_reg(tid, logReg);
  } else {
    reg[RegSrc1] = 0;
  }

  logReg = inst->GetSrc2();
  if (logReg >= 0) {
    reg[RegSrc2] = AINT_read_reg(tid, logReg);
  } else {
    reg[RegSrc2] = 0;
  }

  logReg = inst->GetSrc3();
  if (logReg >= 0) {
    reg[RegSrc3] = AINT_read_reg(tid, logReg);
  } else {
    reg[RegSrc3] = 0;
  }
}

//
// save the destination register value to a local copy
//
void
AINT_ORACLE_INST_CLASS::SaveDestReg(UINT32 tid)
{
  MMCHK;

  INT32 logReg;

  logReg = inst->GetDest();
  if (logReg >= 0) {
    reg[RegDest] = AINT_read_reg(tid, logReg);
  } else {
    reg[RegDest] = 0;
  }
}

//-----------------------------------------------------------------------------
// required ASIM_ORACLE_INST interface
//-----------------------------------------------------------------------------

UINT64
AINT_ORACLE_INST_CLASS::GetRegValue(RegDesc whichReg) const
{
  MMCHK;

  return (reg[whichReg]);
}

IADDR_CLASS
AINT_ORACLE_INST_CLASS::GetNextPc() const
{
  MMCHK;
  if (ifiOracle->nextpc) {
    return (ifiOracle->nextpc);
  } else {
    return (ifiOracle->picode->addr + 4);
  }
}

UINT64
AINT_ORACLE_INST_CLASS::GetVirtualEa() const
{
  MMCHK;
  return (ifiOracle->vaddr);
}

bool
AINT_ORACLE_INST_CLASS::IsTaken() const
{
  MMCHK;
  return (ifiOracle->nextpc != ifiOracle->picode->addr + 4);
}

bool
AINT_ORACLE_INST_CLASS::IsWrongpath() const
{
  MMCHK;
  return (isWrongpath);
}

//-----------------------------------------------------------------------------
// internal functions for instruction verification
//-----------------------------------------------------------------------------

//
// weak verification of Oracle thread with Backend thread; this code was
// ripped out of the Verifier, so read Verifier = Oracle :)
//
void
AINT_ORACLE_INST_CLASS::VerifyStep(void)
{
  MMCHK;

  ASIM_INST instBackend = inst;
  inflight_inst_ptr ifiVerifier = ifiOracle;

  thread_ptr threadBackend = &Threads[0]; // yikes, hardcoded TIDs
  thread_ptr threadVerifier = &Threads[1];
  aint_addr_t PC = ifiVerifier->picode->addr;
  bool failed = false;

  // get backend ifi from inst
  int instidBackend = instBackend->GetIdentifier();
  inflight_inst_ptr ifiBackend = &threadBackend->cbif[instidBackend];

  //
  // verify computed values and addresses
  //

  // get result register
  // verifier
  icode_ptr picodeVerifier = ifiVerifier->picode;
  unsigned int iflagsVerifier = picodeVerifier->iflags;
  Physical_RegNum newDestPhysVerifier;
  if (picodeVerifier->dest < MaxArgs) {
    // This instruction writes a register
    newDestPhysVerifier = ifiVerifier->args[picodeVerifier->dest];
  } else {
    newDestPhysVerifier = ZERO_REGISTER;
  }
  // backend
  icode_ptr picodeBackend = ifiBackend->picode;
  Physical_RegNum newDestPhysBackend;
  if (picodeBackend->dest < MaxArgs) {
    // This instruction writes a register
    newDestPhysBackend = ifiBackend->args[picodeBackend->dest];
  } else {
    newDestPhysBackend = ZERO_REGISTER;
  }

  // check RA, RB, RC
// checking RA,RB,Rc does not work for oracle since we are driving AINT
// across system calls which can re-write a physical destination register
// without renaming it!
//  failed |= verify( failed,
//          (void*) threadVerifier->Reg[ifiVerifier->args[0]].Int64,
//          (void*) threadBackend->Reg[ifiBackend->args[0]].Int64,
//          PC, "RA mismatch");
//  failed |= verify( failed,
//          (void*) threadVerifier->Reg[ifiVerifier->args[1]].Int64,
//          (void*) threadBackend->Reg[ifiBackend->args[1]].Int64,
//          PC, "RB mismatch");
//  failed |= verify( failed,
//          (void*) threadVerifier->Reg[ifiVerifier->args[2]].Int64,
//          (void*) threadBackend->Reg[ifiBackend->args[2]].Int64,
//          PC, "RC mismatch");
  if (iflagsVerifier & E_WRITE) {
    failed |= verify( failed,
            (void*) ifiVerifier->vaddr,
            (void*) ifiBackend->vaddr,
            PC, "wrong store address" );
    failed |= verify( failed,
            (void*) ifiVerifier->data,
            (void*) ifiBackend->data,
            PC, "store data mismatch" );
  } else if (iflagsVerifier & E_READ) {
    failed |= verify( failed,
            (void*) ifiVerifier->vaddr,
            (void*) ifiBackend->vaddr,
            PC, "wrong load address" );
// same problem as with RA,RB,RC (actually, this _is_ RA)
//    failed |= verify( failed,
//            (void*) threadVerifier->Reg[newDestPhysVerifier].Int64,
//            (void*) threadBackend->Reg[newDestPhysBackend].Int64,
//            PC, "load data mismatch");
  } else {
// same problem as with RA,RB,RC (actually, this _is_ RC)
//    failed |= verify( failed,
//            (void*) threadVerifier->Reg[newDestPhysVerifier].Int64,
//            (void*) threadBackend->Reg[newDestPhysBackend].Int64,
//            PC, "writeback data mismatch");

    failed |= verify( failed,
            (void*) reg[RegDest],
            (void*) threadBackend->Reg[newDestPhysBackend].Int64,
            PC, "destination register data mismatch");
  }

  //
  // check if we got the same successor PC
  //
  failed |= verify( failed,
          (void*) ifiVerifier->nextpc,
          (void*) ifiBackend->nextpc,
          PC, "execution stream diverting" );


  if (failed) {
    if (instBackend) {
      cerr << "\t" << (void*) PC << ": "
           << instBackend->GetDisassembly() << endl;
    }
    cerr << endl;

    cerr << "\tOracle:" << endl;
    dumpInsnInfo( ifiVerifier, threadVerifier );
    cerr << endl;

    cerr << "\tBackend:" << endl;
    dumpInsnInfo( ifiBackend, threadBackend );
    cerr << endl;

    cerr << "Oracle::VerifyStep terminating execution" << endl;
    cerr << endl;
    exit(123);
  }
}

//
// verify one data item
//
inline bool
AINT_ORACLE_INST_CLASS::verify (bool failedAlready, void* thisData, void* backendData, aint_addr_t pc, char* errorMsg, int pos)
{
  MMCHK;

  if (thisData != backendData) {
    if (!failedAlready) {
      // print out header for first mismatch
      fflush(stdout);
      fflush(stderr);
      cerr << endl;
      cerr << "Oracle::VerifyStep at cycle " << pmint_get_sim_cycle()
           << " after " << PMINT_get_instr_issue_count()
           << " committed instructions "
           << " at PC " << (void*) pc << endl;
    }
    if ( pos != -1 ) {
     cerr << "\t" << errorMsg << " at element " << pos << endl;
    }
    else {
     cerr << "\t" << errorMsg << endl;
    }
    cerr << "\t" << "Oracle   has " << thisData << endl;
    cerr << "\t" << "Backend  has " << backendData << endl;
    cerr << endl;
    return (true);
  } else {
    return (false);
  }
}

inline void
AINT_ORACLE_INST_CLASS::dumpInsnInfo (inflight_inst_ptr ifi, thread_ptr pthread)
{
  MMCHK;

  fprintf(stderr, "\t [RA]=%016lx\n\t [RB]=%016lx\n\t [RC]=%016lx\n",
          REG(RA), REG(RB), REG(RC));

  if (ifi->picode->iflags & E_MEM_REF) {
    fprintf(stderr, "\n");
    fprintf(stderr, "\t vaddr=%016lx\n\t paddr=%016lx\n",
            ifi->vaddr, ifi->paddr);
    fprintf(stderr, "   Memory    ");
    for (int i = 0; i <= 0x0f; i++) {
      fprintf(stderr, " %02x", i);
    }
    fprintf(stderr, "\n");
    for (uchar* addr = (uchar*) ((ifi->paddr - 0x0f) & ~0xf);
         addr < (uchar*) ((ifi->paddr + 0x1f) & ~0xf);
         addr++)
    {
      if (((unsigned long) addr & 0x0f) == 0) {
        fprintf(stderr, "0x%p: ", addr);
      }
      fprintf(stderr, " %02x", *addr);
      if (((unsigned long) addr & 0x0f) == 0x0f) {
        fprintf(stderr, "\n");
      }
    }
  }
}
