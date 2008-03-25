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
// Author: Artur Klauser
//

// generic
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/trace.h"
#include "asim/cmd.h"
#include "asim/ioformat.h"

// ASIM public modules
#include "asim/provides/instfeeder_interface.h"
#include "asim/provides/traceinstruction.h"

// ASIM local modules
#include "traceconverter.h"

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

    //r2r inst->SetInstr(pc, traceInst.Opcode());
    inst->SetInstrNoCache(pc.GetAddr(), 0, traceInst->Opcode());

    if (inst->IsLoad() || inst->IsStore())
    {
        // inst->SetPhysicalEffAddress(TRACE_tlb_translate(sid, vea, FALSE));
        inst->SetVirtualEffAddress(traceInst->VirtualEffAddress());
        inst->SetPhysicalEffAddress(traceInst->PhysicalEffAddress());
    }

    UINT32 trap = traceInst->Trap();         // Get trap from current inst
    IADDR_CLASS nextpc = nextTraceInst->VirtualPc(); // Get pc of next instr

    if(!trap && !IsPalCodeInst(pc) && IsPalCodeInst(nextpc))
    {
        // start of PAL-code?
        // warning:  if PALcode makes a call, the return back to palcode
        // area will be detected as a trap
        trap = nextpc.GetAddr() & 0x3fff; // extract pal-base offset from PC
    }

    if (inst->IsBranch() || inst->IsJump())
    {
        // here we need to look at the next instruction in the trace
        // to get the actual target pc of the jump...
        inst->SetActualTarget(nextpc.GetAddr());
        TRACE(Trace_Feeder,
            cout << "\tActual Target = " << nextpc << endl); 
    }
    else if (!trap && nextpc != pc.Next().GetAddr())
    {
        inst->SetNonSequentialPc(nextpc.GetAddr(), trap);
        TRACE(Trace_Feeder,
            cout << "\tUnknown NonSequential PC from " << pc 
                 << " to " << nextpc << endl); 
    }
    if(trap)
    {
        inst->SetNonSequentialPc(nextpc.GetAddr(), trap);
        TRACE(Trace_Feeder,
            cout << "\tTRAP = " << fmt_p(trap)
                 << " TTARGET = " << nextpc << endl); 
    }
}


void
TRACE_CONVERTER_CLASS::CreateNoOp (
    IADDR_CLASS pc,
    ASIM_INST inst)
{
    inst->SetInstrNoCache(pc.GetAddr(), 0, 
        CreateRegOp(BIT_OP, BIS_FUNC, ZERO_REG, ZERO_REG, ZERO_REG));
}
