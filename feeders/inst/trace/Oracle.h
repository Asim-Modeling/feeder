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

/**
 * @file
 * @author Artur Klauser
 * @brief Trace Feeder implementation of instruction oracle.
 */

#ifndef _Oracle_h
#define _Oracle_h

// ASIM core
#include "asim/mm.h"

// ASIM public modules
#include "asim/provides/instfeeder_interface.h"
#include "asim/provides/isa.h"
#include "asim/provides/traceinstruction.h"

typedef class TRACE_ORACLE_INST_CLASS *TRACE_ORACLE_INST;
// a (auto ref counted) smart pointer to this class
typedef class mmptr<TRACE_ORACLE_INST_CLASS> SMART_PTR_TRACE_ORACLE_INST;


/**
 * @brief This class keeps state for the oracle as a whole.
 */
typedef class TRACE_ORACLE_CLASS *TRACE_ORACLE;
class TRACE_ORACLE_CLASS
{
  private:
    SMART_PTR_TRACE_ORACLE_INST lastInst;  ///< last inst fetched

    void
    CleanupShadowList (
        TRACE_ORACLE_INST oracleInst);

  public:
    // constructors / destructors / initializers
    TRACE_ORACLE_CLASS();

    /// Fetch next oracle instruction associated with PM instruction.
    TRACE_ORACLE_INST
    Fetch (
        ASIM_INST inst,
        const TRACE_INST aftInst,
        const IADDR_CLASS nextpc);

    ///  Commit oracle instruction associated with PM instruction.
    void Commit (ASIM_INST inst); 

    /// Kill oracle instruction associated with PM instruction.
    void Kill (ASIM_INST inst, const bool mispredict);
};


/**
 * @brief Trace Feeder implementation of oracle for instructions.
 *        This class keeps state of the oracle per instruction.
 */
class TRACE_ORACLE_INST_CLASS : public ASIM_ORACLE_INST_CLASS
{
  private:
    ASIM_INST inst;       ///< the real ASIM instruction
    TRACE_INST traceInst; ///< Trace Feeder internal instruction representation
    IADDR_CLASS nextPc;   ///< next PC
    SMART_PTR_TRACE_ORACLE_INST next;  ///< next oracle instruction
    SMART_PTR_TRACE_ORACLE_INST prev;  ///< previous oracle instruction
    
  public:
    // Constructor
    TRACE_ORACLE_INST_CLASS (
        ASIM_INST asim_i,
        const TRACE_INST trace_i,
        const IADDR_CLASS npc);

    // Trace Feeder specific interface
    // accessors
    ASIM_INST GetInst (void) const {return inst;}
    TRACE_INST GetTraceInst(void) const {return traceInst;}
    TRACE_ORACLE_INST GetNext(void) const {return next;}
    TRACE_ORACLE_INST GetPrev(void) const {return prev;}

    // modifiers
    void SetInst ( ASIM_INST i) {inst = i;}
    void SetTraceInst(const TRACE_INST i) {traceInst = i;}
    void LinkAfter(const TRACE_ORACLE_INST before);
    void Unlink(void);
    void CutLinkAfter(void);

    //
    // ASIM oracle required interface
    //
    // Accessors
    // value of Register after execution
    UINT64 GetRegValue(RegDesc whichReg) const;
    // next correct-path PC
    IADDR_CLASS GetNextPc(void) const;
    // Virtual Effective Address for memory operations
    UINT64 GetVirtualEa(void) const;
    // is conditional branch taken ?
    bool IsTaken(void) const;
    // is this instruction on the wrong control path ?
    bool IsWrongpath(void) const;
};

//
// link single inst after the other
//
inline void
TRACE_ORACLE_INST_CLASS::LinkAfter (
    const TRACE_ORACLE_INST before)
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

inline void
TRACE_ORACLE_INST_CLASS::Unlink(void)
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
TRACE_ORACLE_INST_CLASS::CutLinkAfter(void)
{
    if (next) {
        next->prev = NULL;
        next = NULL;
    }
}

#endif // _Oracle_h
