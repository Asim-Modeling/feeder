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
 * @brief IA64 implementation of trace instruction for trace feeder
 */

#ifndef _IA64_TRACE_INST_H
#define _IA64_TRACE_INST_H

// ASIM core
#include "asim/syntax.h"

// ASIM public modules
#include "asim/provides/iaddr.h"
#include "asim/arch_register.h"
#include "asim/provides/isa.h"
//----------------------------------------------------------------------------
// Trace Instruction interface
//----------------------------------------------------------------------------

typedef class TRACE_INST_CLASS *TRACE_INST;
class TRACE_INST_CLASS
{
    static const UINT64 NOP_BUNDLE[2];
    static const UINT64 OR_BUNDLE[2];
    static const UINT64 STARTUP_BRANCH_BUNDLE[2];
    
    // variables
    IADDR_CLASS vpc;    ///< virtual PC
    IADDR_CLASS target; ///< target (ie. next PC)
    bool taken;         ///< branch taken?
    UINT64 oldcfm;      ///< current frame marker (before execution)
    UINT64 newcfm;      ///< current frame marker (after execution)
    UINT64 vea;         ///< virtual effective address
    UINT64 bits[2];     ///< bundle bits - little endian
    UINT64 oldPRF;      ///< old predicate RF state
    UINT64 newPRF;      ///< new predicate RF state
    bool wrongpath;     ///< embedded wrongpath orphan syllable in trace
    bool trap;          ///< trap on this inst (to target)
    bool longImm;       ///< syllable is just data bits for long immediate
    bool nop;           ///< inst is nop, no need to commit
    bool eof;           ///< end of file reached; no syllable available
    bool isWarmUp;      ///< inst in warm-up (not sampling) region
    UINT64 oldbsp;      ///<the bsp before the instruction executed
    UINT64 newbsp;      ///<the bsp after the instruction executed
    UINT64 uid;         ///<the Uid of the last asim inst that was derived from
                        ///this inst
        /* Register values for the traces that contain them */
    ARCH_REGISTER_CLASS src[NUM_SRC_GP_REGS];
    ARCH_REGISTER_CLASS pred[NUM_SRC_PRED_REGS];
    ARCH_REGISTER_CLASS dst[NUM_DST_REGS];   

  public:
    // constructors / destructors / initializers
    void
    Init(
        IADDR_CLASS vpc,    ///< PC (virtual address)
        UINT64 * bb,        ///< pointer to bundle bits [2] - little endian
        bool wrongpath)     ///< is this instruction on the wrongpath
    {
        Clear();

        this->vpc = vpc;
        this->bits[0] = bb[0];
        this->bits[1] = bb[1];
        this->wrongpath = wrongpath;
    }
    void Clear(void) { memset (this, 0, sizeof(*this)); }
    
    // accessors
    static const UINT64* GetOffPathInst() { return OR_BUNDLE; }
    static const UINT64* GetStartupBranch() { return STARTUP_BRANCH_BUNDLE; }
    
    IADDR_CLASS VirtualPc( void ) const { return vpc; }
    IADDR_CLASS Target( void ) const { return target; }
    bool Taken( void ) const { return taken; }
    UINT64 OldCFM( void ) const { return oldcfm; }
    UINT64 NewCFM( void ) const { return newcfm; }
    UINT64 VirtualEffAddress( void ) const { return vea; }
    UINT64 OldPRF( void ) const { return oldPRF; }
    UINT64 NewPRF( void ) const { return newPRF; }
    UINT64 Uid( void ) const { return uid; }
    bool Wrongpath( void ) const { return wrongpath; }
    bool Trap( void ) const { return trap; }
    bool LongImm( void ) const { return longImm; }
    bool Nop( void ) const { return nop; }
    bool Eof( void ) const { return eof; }
    bool IsWarmUp( void ) const { return isWarmUp; }
    const UINT64* GetBundleBits() const { return & bits[0]; }

    // modifiers
    void SetUid(UINT64 Uid) { this->uid = Uid;}
    void SetEof( void ) { eof = true; vpc = 0; }
    void SetIsWarmUp( void ) { isWarmUp = true; }
    void SetVirtEffAddr( UINT64 vea ) { this->vea = vea; }
    void SetTarget( IADDR_CLASS target ) { this->target = target; }
    void SetTaken( bool taken ) { this->taken = taken; }
    void SetTrap( bool trap ) { this->trap = trap; }
    void SetNonSeqPc( IADDR_CLASS target )
    {
        this->target = target;
        this->trap = true;
    }
    void SetCFM( UINT64 oldcfm, UINT64 newcfm )
    {
        this->oldcfm = oldcfm;
        this->newcfm = newcfm;
    }
    void SetBSP( UINT64 oldbsp, UINT64 newbsp)
        {
            this->oldbsp=oldbsp;
            this->newbsp=newbsp;
        }

    void SetLongImm( void ) { this->longImm = true; }
    void SetNop( void ) { this->nop = true; }
    void SetPredRF( UINT64 oldPRF, UINT64 newPRF )
        {
            this->oldPRF = oldPRF;
            this->newPRF = newPRF;
        }
    void SetInputDep(ARCH_REGISTER_CLASS src[NUM_SRC_GP_REGS+NUM_SRC_PRED_REGS])
        {
            for (int i = 0; i < (NUM_SRC_GP_REGS+NUM_SRC_PRED_REGS); i++)
            {
                this->src[i] = src[i];
            }
 
        }

    void SetOutputDep( ARCH_REGISTER_CLASS dst[NUM_DST_REGS])
        {
            for(int i = 0; i < NUM_DST_REGS; i++)
            {
                this->dst[i] =dst[i];
            }            
        }
    ARCH_REGISTER getdest(UINT32 slot) 
        {
            ASSERTX(slot<NUM_DST_REGS);
            return &dst[slot];
        }
    ARCH_REGISTER getsrc(UINT32 slot) 
        {
            ASSERTX(slot<NUM_SRC_GP_REGS);
            return &src[slot];
        }
    ARCH_REGISTER getpred(UINT32 slot) 
        {
            ASSERTX(slot<NUM_SRC_PRED_REGS);
            return &pred[slot];
        }
    void getdest(UINT32 slot,ARCH_REGISTER reg) 
        {
            ASSERTX(slot<NUM_DST_REGS);
            *reg=dst[slot];
        }
    void getsrc(UINT32 slot,ARCH_REGISTER reg) 
        {
            ASSERTX(slot<NUM_SRC_GP_REGS);
            *reg=src[slot];
        }
    void getpred(UINT32 slot,ARCH_REGISTER reg) 
        {
            ASSERTX(slot<NUM_SRC_PRED_REGS);
            *reg=pred[slot];
        }


};

#endif // _IA64_TRACE_INST_H

