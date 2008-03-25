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

#ifndef _ATF_ALPHA_INST_H
#define _ATF_ALPHA_INST_H

// ASIM core
#include "asim/syntax.h"

// ASIM private modules
#include "asim/provides/isa.h"

//----------------------------------------------------------------------------
// Trace Instruction interface
//----------------------------------------------------------------------------

typedef class TRACE_INST_CLASS *TRACE_INST;
class TRACE_INST_CLASS
{
  private:
    // consts
    static const UINT64 EofId = UINT64_MAX;

    // variables
    IADDR_CLASS vpc;
    IADDR_CLASS ppc;
    UINT64 vea;
    UINT64 pea;
    UINT64 info;
    UINT32 opcode;
    UINT32 cpu;

  public:
    // constructors / destructors / initializers
    void
    Init(
        IADDR_CLASS vpc,  //< PC (virtual address)
        IADDR_CLASS ppc,  //< PC (physical address)
        UINT64 vea,       //< Effective Address (virtual address)
        UINT64 pea,       //< Effective Address (physical address)
        UINT32 opcode,    //< binary encoded instruction
        UINT32 cpu)       //< cpu this instruction is from
    {
        this->vpc = vpc;
        this->ppc = ppc;
        this->vea = vea;
        this->pea = pea;
        this->info = 0;
        this->opcode = opcode;
        this->cpu = cpu;
    }

    // accessors
    IADDR_CLASS VirtualPc( void ) const { return vpc; }
    IADDR_CLASS PhysicalPc( void ) const { return ppc; }
    UINT64 VirtualEffAddress( void ) const { return vea; }
    UINT64 PhysicalEffAddress( void ) const { return pea; }
    UINT32 Opcode( void ) const { return opcode; }
    UINT32 Cpu( void ) const { return cpu; }
    const bool Wrongpath( void ) const { return false; }
    bool IsWarmUp( void ) const { return false; }
    bool Eof( void ) const { return info == EofId; }
    UINT64 Trap() const
    { 
        if(info < 0x4000) 
        {
            return info;
        }
        return 0;
    }

    // modifiers
    void SetEof( void ) { info = EofId; vpc.SetAddr(UINT64(0)); }
};

#endif // _ATF_ALPHA_INST_H
