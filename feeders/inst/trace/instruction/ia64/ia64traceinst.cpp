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
 * @brief IA64 implementation of trace instruction for trace feeder
 */

// ASIM public modules
#include "asim/provides/traceinstruction.h"

/// [MII] nop 0; nop 0; nop 0;
const UINT64 TRACE_INST_CLASS::NOP_BUNDLE[2] =
{
    // @#$$! little endian
    UINT64_CONST(0x0000000100000000), // low bits
    UINT64_CONST(0x0004000000000200)  // high bits
};

/// [MII] or r1=r1,r1; or r1=r1,r1; or r1=r1,r1
const UINT64 TRACE_INST_CLASS::OR_BUNDLE[2] =
{
    UINT64_CONST(0x0810200e02040800), // low bits
    UINT64_CONST(0x8038081020401c04)  // high bits

};

/// [MII] nop 0; nop 0; nop 0;
const UINT64 TRACE_INST_CLASS::STARTUP_BRANCH_BUNDLE[2] =
{
    // @#$$! little endian
    // change this to reflect an unconditional long imm. (probably) branch
    UINT64_CONST(0x0000002000000016), // low bits
    UINT64_CONST(0x4000000000100000)  // high bits
};
