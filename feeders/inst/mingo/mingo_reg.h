/*
 * Copyright (C) 2002-2006 Intel Corporation
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
 * @file mingo_reg.h
 * @author Michael Adler
 * @date March 13, 2002
 *
 * Register names passed from instrumented program to Mingo feeder.
 * 
 */

#ifndef _MINGO_REG_
#define _MINGO_REG_

typedef enum
{
    MINGO_REG_NONE     = 0,

    //
    // IA64 register names.  Actual register names are register number + the base
    // from the following classes:
    //
    MINGO_REG_IBASE     = 1,                        // 128 general registers
    MINGO_REG_FBASE     = MINGO_REG_IBASE + 128,    // 128 floating point registers
    MINGO_REG_PBASE     = MINGO_REG_FBASE + 128,    //  64 predicate registers
    MINGO_REG_BBASE     = MINGO_REG_PBASE + 64,     //   8 branch registers

    //
    // IA32 mappings into the IA64 name space.
    //
    // Notes:
    //  - MINGO and the simulator do not understand that AH and EAX are overlayed.
    //    If you call MINGO_Memory_Read() into AH and the first reader is in EAX
    //    you must pass AH to MINGO_Dependent_Read().
    //
    //  - Similarly, if AH and AL are written by separate loads and then consumed
    //    together in one dependent AX, you must call MINGO_Dependent_Read() twice
    //    before the use of AX or EAX.  Call once with AH and once with AL just
    //    before the combined register is read.
    //
    //  - Perfect accuracy is not required in deciding whether to use integer or
    //    floating point semantics for XMM registers.  (Namely whether you use
    //    IXMMBASE or FXMMBASE.)  Obviously make sure the memory read and dependent
    //    read are consistent.  The choice of int or fp will be used by the
    //    performance simulator in generating its synthetic workload.
    //
    //  - The current naming does not provide for SIMD granularity in accesses
    //    to MMX and XMM registers.
    //
    MINGO_REG_EAX       = MINGO_REG_IBASE + 1,      // Skip IA64 zero register
    MINGO_REG_AH        = MINGO_REG_IBASE + 2,
    MINGO_REG_AL        = MINGO_REG_IBASE + 3,
    MINGO_REG_EBX       = MINGO_REG_IBASE + 4,
    MINGO_REG_BH        = MINGO_REG_IBASE + 5,
    MINGO_REG_BL        = MINGO_REG_IBASE + 6,
    MINGO_REG_ECX       = MINGO_REG_IBASE + 7,
    MINGO_REG_CH        = MINGO_REG_IBASE + 8,
    MINGO_REG_CL        = MINGO_REG_IBASE + 9,
    MINGO_REG_EDX       = MINGO_REG_IBASE + 10,
    MINGO_REG_DH        = MINGO_REG_IBASE + 11,
    MINGO_REG_DL        = MINGO_REG_IBASE + 12,
    MINGO_REG_EBP       = MINGO_REG_IBASE + 13,
    MINGO_REG_ESI       = MINGO_REG_IBASE + 14,
    MINGO_REG_EDI       = MINGO_REG_IBASE + 15,
    MINGO_REG_ES        = MINGO_REG_IBASE + 16,

    MINGO_REG_MMXBASE   = MINGO_REG_IBASE + 17,     // 8 MMX registers
    MINGO_REG_IXMMBASE  = MINGO_REG_MMXBASE + 8,    // 8 XMM registers (with int semantics)

    MINGO_REG_STBASE    = MINGO_REG_FBASE + 2,      // 8 x87 FPU registers (skip IA64 0 and 1)
    MINGO_REG_FXMMBASE  = MINGO_REG_STBASE + 8      // 8 XMM registers (with FP semantics)
}
MINGO_REG;

#endif      // _MINGO_REG_
