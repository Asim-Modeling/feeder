/*
 * *****************************************************************
 * *                                                               *
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

#ifndef _IA64_TRACE_CONV_H
#define _IA64_TRACE_CONV_H

// ASIM core
#include "asim/syntax.h"

// ASIM public modules
#include "asim/provides/traceinstruction.h"
#include "asim/provides/decode_cache.h"

//----------------------------------------------------------------------------
// Trace Instruction Converter interface
//----------------------------------------------------------------------------

typedef class TRACE_CONVERTER_CLASS *TRACE_CONVERTER;
class TRACE_CONVERTER_CLASS : public TRACEABLE_CLASS
{
  public:
    // constructors / destructors / initializers
    TRACE_CONVERTER_CLASS() 
    {
        SetTraceableName("TRACE_CONVERTER_CLASS");
    }
    // ... nada
    IPF_DECODE_CACHE_CLASS decode_cache;
    // other
    /// create an off path ASIM_INST (for wrong-path)
    void
    CreateOffPathInst (const IADDR_CLASS pc, ASIM_INST inst);

    /// create a Branch ASIM_INST with the given target (used for thread startup)
    void
    CreateStartupBranch(const IADDR_CLASS pc, IADDR_CLASS targetPc, 
                        ASIM_INST inst, TRACE_INST traceInst);

    /// convert one TRACE_INST into an ASIM_INST
    void
    Convert(ASIM_INST inst, TRACE_INST traceInst, TRACE_INST nextTraceInst);
};

#endif // _IA64_TRACE_CONV_H
