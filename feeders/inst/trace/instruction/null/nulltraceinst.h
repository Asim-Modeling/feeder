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
 * @brief NULL implementation of trace instruction for trace feeder
 */

#ifndef _NULL_TRACE_INST_H
#define _NULL_TRACE_INST_H

// ASIM core
#include "asim/syntax.h"

// ASIM public modules
#include "asim/provides/isa.h"

//----------------------------------------------------------------------------
// Trace Instruction interface
//----------------------------------------------------------------------------

typedef class TRACE_INST_CLASS *TRACE_INST;
class TRACE_INST_CLASS
{
  public:
    // constructors / destructors / initializers
    // nada

    // accessors
    const IADDR_CLASS VirtualPc( void ) const { return IADDR_CLASS(UINT64(0)); }
    const UINT64 VirtualEffAddress( void ) const { return 0; }
    const UINT32 Cpu( void ) const { return 0; }
    const bool Wrongpath( void ) const { return false; }
    const bool Trap( void ) const { return false; }
    const bool IsWarmUp( void ) const { return false; }
    const bool Eof( void ) const { return false; }
    const UINT64* GetBundleBits() const { return NULL; }
    const IADDR_CLASS Target( void ) const { return IADDR_CLASS(UINT64(0)); }
    const UINT64 CFM( void ) const { return 0; }
    static const UINT64* GetNOP() { return NULL; }

    // modifiers
    void Init( IADDR_CLASS vpc, UINT64 * bb, bool wrongpath, UINT32 cpu ) { }
    void SetTarget( IADDR_CLASS target ) { }
    void SetNonSeqPc( IADDR_CLASS target ) { }
    void SetCFM( UINT64 cfm ) { }
    void SetEof( void ) { }
    void SetVirtEffAddr( UINT64 vea ) { }
};

#endif // _NULL_TRACE_INST_H
