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

// generic
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/trace.h"

// ASIM public modules
#include "asim/provides/instfeeder_interface.h"

// ASIM local module
#include "Oracle.h"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// the oracle
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//
// create the oracle thread
//
TRACE_ORACLE_CLASS::TRACE_ORACLE_CLASS()
{
    lastInst = NULL;
    TRACE(Trace_Feeder, cout << "Oracle:: activated" << endl);
}

//
// set oracle shadow for real instruction inst
//

TRACE_ORACLE_INST
TRACE_ORACLE_CLASS::Fetch (
    ASIM_INST inst,
    const TRACE_INST traceInst,
    const IADDR_CLASS nextpc)
{
    TRACE_ORACLE_INST oracleInst = 
        new TRACE_ORACLE_INST_CLASS(inst, traceInst, nextpc);
    ASSERT(oracleInst->GetMMRefCount() == 1, "RefCount of new object != 1\n");
    inst->SetOracle(oracleInst);

    // update oracle shadow list
    oracleInst->LinkAfter(lastInst);
    lastInst = oracleInst;
    ASSERT(lastInst->GetNext() == NULL, "bogus next link on lastInst\n");

    TRACE(Trace_Feeder,
        cout << "\tFEED_OracleFetch id=" << inst->GetTraceID() << endl);

    return oracleInst;
}

//
// commit the oracle shadow associated with real instruction inst
//
void
TRACE_ORACLE_CLASS::Commit (
    ASIM_INST inst)
{
    TRACE_ORACLE_INST oracleInst = TRACE_ORACLE_INST(&*inst->GetOracle());
    // every committed instruction has to have an oracle shadow instruction

    // r2r: this hack goes with the problem of being lied to in Kill()
    if (! oracleInst) {
        return;
    }
    ASSERT(oracleInst, "commiting instruction has no oracle shadow\n");

    TRACE(Trace_Feeder,
        cout << "\tFEED_OracleCommit id=" << inst->GetTraceID() << endl);

    // disassociate from real instruction
    ASSERT(oracleInst->GetInst() == inst,
        "shadow instruction not matching with real instruction\n");
    inst->SetOracle(NULL);
    oracleInst->SetInst(NULL);
    oracleInst->SetTraceInst(NULL);

    // release oracle shadow instruction
    inst->SetOracle(NULL);
    oracleInst->SetInst(NULL);
    oracleInst->Unlink();
    if (lastInst == oracleInst) {
        lastInst = NULL;
    }
//    oracleInst->DecrRef();
//    oracleInst=NULL;
//    delete oracleInst;
}

//
// cleanup shadow list by disassociating all shadows from their real
// instructions;
//
void
TRACE_ORACLE_CLASS::CleanupShadowList (
    TRACE_ORACLE_INST oracleInst)
{
    TRACE_ORACLE_INST nextInst;

    while (oracleInst) {
        nextInst = oracleInst->GetNext();
        // destroy association with real instruction
        ASSERT(oracleInst->GetInst(), "oracle shadow without real instruction\n");
        TRACE(Trace_Feeder,
            cout << "\tFEED_OracleKill id="
                 << oracleInst->GetInst()->GetTraceID() << endl);

        oracleInst->GetInst()->SetOracle(NULL);
        oracleInst->SetTraceInst(NULL);
        oracleInst->SetInst(NULL);

        // we're done with this oracle shadow
        oracleInst->Unlink();
//        oracleInst->DecrRef();
//        oracleInst=NULL;
//        delete oracleInst;

        oracleInst = nextInst;
    }
}

//
// kill the oracle shadow associated with real instruction inst and
// also kill all younger instructions;
//
void
TRACE_ORACLE_CLASS::Kill (
    ASIM_INST inst,
    const bool killMe)
{
    TRACE_ORACLE_INST oracleInst = TRACE_ORACLE_INST(&*inst->GetOracle());
    ASSERT(oracleInst, "missing oracle shadow for kill point\n");
    TRACE_ORACLE_INST killInst;

    TRACE(Trace_Feeder,
        cout << "\tFEED_OracleKill id=" << inst->GetTraceID()
             << " killMe=" << killMe << endl);

    if (killMe) {
        killInst = oracleInst;
        lastInst = oracleInst->GetPrev();
    } else {
        // get the oracle shadow after the branch */
        killInst = oracleInst->GetNext();
        lastInst = oracleInst;
    }
    if (lastInst) {
        lastInst->CutLinkAfter();
    }

    // delete oracle shadows
    CleanupShadowList(killInst);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// oracle shadow instructions
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

TRACE_ORACLE_INST_CLASS::TRACE_ORACLE_INST_CLASS(
    ASIM_INST asim_i,
    const TRACE_INST trace_i,
    const IADDR_CLASS npc)
    : ASIM_ORACLE_INST_CLASS("OracleShadowInst",
      0 /*id*/),
      inst(asim_i),
      traceInst(trace_i),
      nextPc(npc)
{
    // nada
}

//-----------------------------------------------------------------------------
// required ASIM_ORACLE_INST interface
//-----------------------------------------------------------------------------

UINT64
TRACE_ORACLE_INST_CLASS::GetRegValue (
    RegDesc whichReg) const
{
    MMCHK;

    ASSERT(false, "Oracle::GetRegValue not supported on traces\n");
    return 0;
}

IADDR_CLASS
TRACE_ORACLE_INST_CLASS::GetNextPc(void) const
{
    MMCHK;
    return (nextPc);
}

UINT64
TRACE_ORACLE_INST_CLASS::GetVirtualEa(void) const
{
    MMCHK;
    return (traceInst->VirtualEffAddress());
}

bool
TRACE_ORACLE_INST_CLASS::IsTaken(void) const
{
    MMCHK;
    return (nextPc != traceInst->VirtualPc().Next());
}

bool
TRACE_ORACLE_INST_CLASS::IsWrongpath(void) const
{
    MMCHK;
    // all trace oracle shadows are on the correct path; wrongpath trace
    // instructions do not have oracle shadows;
    return (false);
}
