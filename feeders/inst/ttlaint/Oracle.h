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

//----------------------------------------------------------------------------
// Oracle.h - AINT implementation of instruction oracle
//
// Author:  Artur Klauser
//
//----------------------------------------------------------------------------

#ifndef _Oracle_h
#define _Oracle_h


// ASIM core
#include "asim/mm.h"
#include "asim/mmptr.h"

// ASIM public modules
#include "asim/provides/instfeeder_interface.h"

// ASIM local module
#include "aint.h"

// ASIM other modules -- BAD! FIXME illegal dependence
#include "asim/restricted/common.h"

#define INSTID_INVALID UINT64_MAX

typedef long aint_addr_t;

typedef class aint_oracle_inst_class AINT_ORACLE_INST_CLASS, *AINT_ORACLE_INST;
// a (auto ref counted) smart pointer to this class
typedef class mmptr<AINT_ORACLE_INST_CLASS> SMART_PTR_AINT_ORACLE_INST;

//----------------------------------------------------------------------------
// Oracle class
// This class keeps state for the oracle as a whole
//----------------------------------------------------------------------------
typedef class Oracle {
private:
  IADDR_CLASS pc;      // PC that should be fetched next
  UINT32 tid;          // AINT thread ID for oracle thread
  bool isWrongpath;    // true if oracle is in wrong-path mode
  bool isStalled;      // true if oracle is stalled (not executing new insns)
  SMART_PTR_AINT_ORACLE_INST nextSavedInst; // next saved inst for fetching
  SMART_PTR_AINT_ORACLE_INST lastInst;      // last inst fetched

  void CleanupShadowList(AINT_ORACLE_INST oracleInst);

public:
  // Constructor
  Oracle( UINT32 tid, IADDR_CLASS startPC);

  void Skip(IADDR_CLASS pc);
  void SetPc(const IADDR_CLASS _pc) {pc = _pc;}
  AINT_ORACLE_INST Fetch(ASIM_INST inst, IADDR_CLASS fetchPc);
  void Commit(ASIM_INST inst);
  void Kill(ASIM_INST inst, bool mispredict);
} AINT_ORACLE_CLASS, *AINT_ORACLE;

//----------------------------------------------------------------------------
// AINT_ORACLE_INST
// Aint implementation of oracle for instructions.
// This class keeps state of the oracle per instruction.
//----------------------------------------------------------------------------
class aint_oracle_inst_class : public ASIM_ORACLE_INST_CLASS
{
private:
  SMART_PTR_AINT_ORACLE_INST next;
  SMART_PTR_AINT_ORACLE_INST prev;
  UINT64 reg[RegDescMax];
  instid_t instIdOracle;
  inflight_inst_ptr ifiOracle;
  ASIM_INST inst;     // the real ASIM instruction
  bool isWrongpath;
  
public:
  // Constructor
  aint_oracle_inst_class(instid_t id, inflight_inst_ptr _ifi, ASIM_INST i, bool wp);

  // AINT specific interface
  instid_t GetInstId(void) const {return instIdOracle;}
  inflight_inst_ptr GetIfi(void) const {return ifiOracle;}
  ASIM_INST GetInst(void) const {return inst;}
  void SetInst(const ASIM_INST i) {inst = i;}
  void LinkAfter(AINT_ORACLE_INST before);
  void JoinAfter(AINT_ORACLE_INST before);
  void Unlink(void);
  void CutLinkAfter(void);
  AINT_ORACLE_INST GetNext(void) const {return next;}
  AINT_ORACLE_INST GetPrev(void) const {return prev;}
  void SaveSrcReg(UINT32 tid);
  void SaveDestReg(UINT32 tid);

  // internal
  void VerifyStep(void);
  bool verify (bool failedAlready, void* thisData, void* backendData, aint_addr_t pc, char* errorMsg, int pos = -1);
  void dumpInsnInfo (inflight_inst_ptr ifi, thread_ptr pthread);

  //
  // ASIM oracle required interface
  //
  // Accessors
  UINT64 GetRegValue(RegDesc whichReg) const; // value of Register after execution
  IADDR_CLASS GetNextPc() const;     // next correct-path PC
  UINT64 GetVirtualEa() const;  // Virtual Effective Address for memory operations
  bool IsTaken() const;         // is conditional branch taken ?
  bool IsWrongpath() const;     // is this instruction on the wrong control path ?
};


//
// link single inst after the other
//
inline void
AINT_ORACLE_INST_CLASS::LinkAfter(AINT_ORACLE_INST before)
{
  if (before) {
    ASSERT(before->next == NULL, "LinkAfter not at end of list\n");
    before->next = this;
    this->prev = before;
    this->next = NULL;
  } else {
    this->prev = NULL;
    this->next = NULL;
  }
}

//
// link a list of insts after the other
//
inline void
AINT_ORACLE_INST_CLASS::JoinAfter(AINT_ORACLE_INST before)
{
  if (before) {
    before->next = this;
    this->prev = before;
  } else {
    this->prev = NULL;
  }
}

inline void
AINT_ORACLE_INST_CLASS::Unlink(void)
{
  if (prev) {
    prev->next = this->next;
    prev = NULL;
  }
  if (next) {
    next->prev = this->prev;
    next = NULL;
  }
}

inline void
AINT_ORACLE_INST_CLASS::CutLinkAfter(void)
{
  if (next) {
    next->prev = NULL;
    next = NULL;
  }
}

#endif // _Oracle_h
