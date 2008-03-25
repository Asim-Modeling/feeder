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
 * @brief NULL converter of trace converter for trace feeder.
 */

#ifndef _NULL_TRACE_CONV_H
#define _NULL_TRACE_CONV_H

// ASIM core
#include "asim/syntax.h"

// ASIM public modules
#include "asim/provides/traceinstruction.h"

//----------------------------------------------------------------------------
// Trace Instruction Converter interface
//----------------------------------------------------------------------------

typedef class TRACE_CONVERTER_CLASS *TRACE_CONVERTER;
class TRACE_CONVERTER_CLASS
{
  public:
    // constructors / destructors / initializers
    // ... nada

    // other
    /// create a NoOp ASIM_INST (for wrong-path)
    void
    CreateNoOp (IADDR_CLASS pc, ASIM_INST inst) {}

    /// convert one TRACE_INST into an ASIM_INST
    void
    Convert(ASIM_INST inst, TRACE_INST traceInst, TRACE_INST nextTraceInst) {}
};

#endif // _NULL_TRACE_CONV_H
