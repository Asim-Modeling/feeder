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
 * @brief IA64 converter of trace converter for trace feeder.
 */

// generic
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ostream>

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/trace.h"
#include "asim/cmd.h"
#include "asim/ioformat.h"

// ASIM public modules
#include "asim/provides/instfeeder_interface.h"
#include "asim/provides/traceinstruction.h"
#include "asim/provides/traceconverter.h"

namespace iof = IoFormat;

using namespace iof;
using namespace std;

void
TRACE_CONVERTER_CLASS::Convert (
    ASIM_INST inst,
    TRACE_INST traceInst,
    TRACE_INST nextTraceInst)
{
    IADDR_CLASS pc = traceInst->VirtualPc();  // Get pc of subsequent instr

    const UINT64* bundle_bits = traceInst->GetBundleBits();

    decode_cache.Init(inst,bundle_bits, pc);
//    inst->Init(bundle_bits, pc);
    
    if (inst->HasEffAddress())
    {
        inst->SetVirtualEffAddress(traceInst->VirtualEffAddress());
    }

    // ugly!  We're using the ASIM_INST code to decipher if this is a nop or
    // not, then turning around and setting the TRACE_INST nop flag!  
    // We use this flag to decide if the trace inst needs to be committed or not
    if (inst->IsNOP())
    {
        traceInst->SetNop();
    }

    if (inst->IsTypeL())
    {
        traceInst->SetLongImm();
    }

    /// trap means we have non-sequential control flow on an
    /// operation that is not a controlOp
    bool trap = traceInst->Trap();

    IADDR_CLASS nextpc = nextTraceInst->VirtualPc();  // Get pc of next instr

    // regular synchronous control flow changes
    if (inst->IsControlOp())
    {
        if (traceInst->Target().GetAddr() == 0) {
            // sometimes control ops don't set up their sequential successor,
            // so we have to do it ourselves here;
            traceInst->SetTarget(pc.Next());
        }
        // get the actual target of a control instruction
        //ASSERT(traceInst->Target().GetAddr() != 0,
        //    "missing branch target info in trace");
        inst->SetActualTarget(traceInst->Target());
        T1("\tActual Target = " << traceInst->Target() << endl
              << "\tNext trace IP = " << nextpc); 

        inst->SetActualTaken(traceInst->Taken());
        
    }

    // ansynchronous control flow changes
    if(trap)
    {
        inst->SetNonSequentialPc(traceInst->Target());
        T1("\tTRAP Target = " << traceInst->Target() << endl
              << "\tNext trace IP = " << nextpc); 
    }

    // set CFM regardless of instruction type
    inst->SetCFM(traceInst->OldCFM(), traceInst->NewCFM());
    T1("\tOldCFM = " << fmt_x(traceInst->OldCFM()) << endl
          << "\tNewCFM = " << fmt_x(traceInst->NewCFM()));

    // set the predicate register file
    inst->SetPRF(traceInst->OldPRF(), traceInst->NewPRF());
    T1("\tOldPRF = " << fmt_x(traceInst->OldPRF()) << endl
          << "\tNewPRF = " << fmt_x(traceInst->NewPRF()));
}


void
TRACE_CONVERTER_CLASS::CreateOffPathInst (
    const IADDR_CLASS pc,
    ASIM_INST inst)
{
    decode_cache.Init(inst,TRACE_INST_CLASS::GetOffPathInst(), pc);
    //inst->Init(TRACE_INST_CLASS::GetOffPathInst(), pc);
}

void
TRACE_CONVERTER_CLASS::CreateStartupBranch (
    const IADDR_CLASS pc,
    IADDR_CLASS targetPc,
    ASIM_INST inst,
    TRACE_INST traceInst)
{
    UINT64 branchBundle[2];

    inst->Init(TRACE_INST_CLASS::GetStartupBranch(), pc);

    ASSERTX(inst->IsControlOp());

    inst->SetActualTarget(targetPc);

    inst->SetCFM(UINT32_MAX, UINT32_MAX);

    T1("\tActual Target = " << targetPc);
    
    inst->SetActualTaken(true);

    // set CFM regardless of instruction type
    inst->SetCFM(traceInst->OldCFM(), traceInst->OldCFM());
    T1("\tOldCFM = " << fmt_x(traceInst->OldCFM()) << endl
          << "\tNewCFM = " << fmt_x(traceInst->NewCFM()));    
}

