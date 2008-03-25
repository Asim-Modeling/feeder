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

#ifndef _ATFTRACECONV_H
#define _ATFTRACECONV_H

// ASIM core
#include "asim/syntax.h"
#include "asim/alphaops.h"

// ASIM public modules
#include "asim/provides/traceinstruction.h"

//----------------------------------------------------------------------------
// Trace Instruction Converter interface
//----------------------------------------------------------------------------

typedef class TRACE_CONVERTER_CLASS *TRACE_CONVERTER;
class TRACE_CONVERTER_CLASS
{
  private:
    /// Generic 3-register operation Ra,Rb,Rc
    ALPHA_INSTRUCTION
    CreateRegOp ( INT32 op, INT32 func, UINT32 rA, UINT32 rB, UINT32 rC);

    /// determine if we are in PAL code space
    bool IsPalCodeInst ( UINT64 addr) { return ((addr & 0x01) == 0x01); }

  public:
    // constructors / destructors / initializers
    // ... nada

    // other
    /// create a NoOp ASIM_INST (for wrong-path)
    void
    CreateNoOp (IADDR_CLASS pc, ASIM_INST inst);

    /// convert one TRACE_INST into an ASIM_INST
    void
    Convert(ASIM_INST inst, TRACE_INST traceInst, TRACE_INST nextTraceInst);
};

/**
 * Create a 3-register Alpha instruction from its component portions.
 */
inline ALPHA_INSTRUCTION
TRACE_CONVERTER_CLASS::CreateRegOp (
    INT32 op,    ///< opcode portion of instruction
    INT32 func,  ///< function code portion of instruction
    UINT32 rA,   ///< register_A portion of instruction
    UINT32 rB,   ///< register_B portion of instruction
    UINT32 rC)   ///< register_C portion of instruction
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

#endif // _ATFTRACECONV_H
