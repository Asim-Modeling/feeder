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
 * @author Artur Klauser and Steven Wallace
 * @brief IA64 bundle type definition for Ciardi trace reader
 */

#ifndef _IA64_CIARDI_BUNDLE_H
#define _IA64_CIARDI_BUNDLE_H

// generic
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// ASIM core
#include "asim/syntax.h"

// ASIM public modules
#include "asim/provides/isa.h"
#include "asim/provides/traceinstruction.h"

/// additional info for a syllable
typedef class IA64_CIARDI_SYLLABLE_CLASS *IA64_CIARDI_SYLLABLE;
class IA64_CIARDI_SYLLABLE_CLASS
{
  public:
    UINT64 addr;   ///< virtual memory address
    UINT64 cfm;    ///< branch's CFM
    UINT64 bsp;
    bool taken;    ///< branch taken
    bool trap;     ///< trap occurred
    UINT64 hint;   ///< branch hint info
    UINT64 tag;    ///< branch hint tag info
    INT8 vpr[2];   ///< predicate virtual destination reg (<0 invalid)
    bool value[2]; ///< predicate value
    bool movpr;    ///< pregs value is valid
    UINT64 pregs;  ///< full predicate register state
    IADDR_CLASS target;    ///< branch target address (can't be in union)
    bool wrongpath;        ///< is this a wrongpath instruction
        /* Register values for the traces that contain them */
        /* not arch_registers yet because we don't know the type */
    ARCH_REGISTER_CLASS input_registers[NUM_SRC_PRED_REGS+NUM_SRC_GP_REGS];
    ARCH_REGISTER_CLASS output_registers[NUM_DST_REGS];
//    UINT64 input_regval[ NUM_SRC_PRED_REGS+NUM_SRC_GP_REGS][2];
//    UINT64 output_regval[NUM_DST_REGS][2];
};

/// information for a whole bundle
typedef class IA64_CIARDI_BUNDLE_CLASS *IA64_CIARDI_BUNDLE;
class IA64_CIARDI_BUNDLE_CLASS
{
  public:
    // consts
    static const int NumSyllables = 3;

    // variables
    UINT64 bits[2];  ///< encoded instruction bits - little endian
    UINT64 vpc;      ///< virtual PC (IP)
    /// additional syllable info
    IA64_CIARDI_SYLLABLE_CLASS syllable[NumSyllables];

#if 0 /* CIARDI_DUMP */
    char buffers[10][1024];
    UINT32 lines;
#endif

    // initializer
    void Clear (void) { memset(this, 0, sizeof(*this));}

    void ClearRegisters (void) 
        {
            for(int j=0; j<NumSyllables; j++)
            {
                for(UINT32 i=0; i< NUM_SRC_PRED_REGS+NUM_SRC_GP_REGS; i++)
                {
                    syllable[j].input_registers[i]=ARCH_REGISTER_CLASS();
                }
                for(UINT32 i=0; i< NUM_DST_REGS; i++)
                {
                    syllable[j].output_registers[i]=ARCH_REGISTER_CLASS();
                }
            }

        }

};

#endif // _IA64_CIARDI_BUNDLE_H
