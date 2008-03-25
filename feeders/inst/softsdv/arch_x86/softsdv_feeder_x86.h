/**************************************************************************
 * Copyright (C) 2004-2006 Intel Corporation
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
 **************************************************************************/

/**
 * @file
 * @author Michael Adler
 * @brief SoftSDV x86 Instruction Feeder
 */

// ASIM private modules
#include "asim/provides/softsdv_feeder.h"


class SOFTSDV_MACRO_FEEDER_CLASS : public SOFTSDV_FEEDER_BASE_CLASS
{
  public:
    SOFTSDV_MACRO_FEEDER_CLASS(IFEEDER_BASE microFeeder = NULL);
    ~SOFTSDV_MACRO_FEEDER_CLASS() {};

  private:

    //----------------------------------------------------------------
    //
    // Virtual functions required by SOFTSDV_FEEDER_BASE_CLASS...
    //
    //----------------------------------------------------------------

    void InitISASpecific(UINT32 argc, char **argv, char **envp);

    void
    NoteMemoryRefForWarmup(
        UINT32 cpuNum,
        SOFTSDV_LIVE_INSTRUCTION instrHandle,
        WARMUP_INFO warmup);

    //
    // Convert an instruction from the SoftSDV representation to ASIM_MACRO_INST.
    //
    bool
    AsimInstFromSoftsdvInst(
        UINT32 cpuNum,
        SOFTSDV_LIVE_INSTRUCTION instrHandle,
        ASIM_MACRO_INST asimInstr,
        bool warmUp,
        UINT64 cycle);

    void
    MergeREPInstrs(
        UINT32 cpuNum,
        SOFTSDV_LIVE_INSTRUCTION instrHandle,
        UINT64 cycle);

    ASIM_SOFTSDV_MEM_ACCESS
    InitREPRegion(
        const ASIM_SOFTSDV_MEM_ACCESS_CLASS *oldRef,
        UINT32 refSize,
        bool moveForward,
        bool useValues,
        UINT32 valueMaxSize);

    void
    MergeREPRegion(
        ASIM_SOFTSDV_MEM_ACCESS mergeRef,
        const ASIM_SOFTSDV_MEM_ACCESS_CLASS *oldRef,
        UINT32 refSize,
        bool moveForward,
        bool useValues,
        UINT32 valueMaxSize);

    SOFTSDV_LIVE_INSTRUCTION
    NoteNewIncomingInstr(
        UINT32 cpuNum,
        ASIM_SOFTSDV_INST_INFO sdvInstr);

    void
    InjectNOP(
        UINT32 cpuNum,
        const IADDR_CLASS ipVA,
        const IADDR_CLASS ipPA,
        ASIM_MACRO_INST asimInstr);

    void
    InjectBadPathInstr(
        UINT32 cpuNum,
        const IADDR_CLASS ipVA,
        const IADDR_CLASS ipPA,
        ASIM_MACRO_INST asimInstr);

    void
    InjectJMP(
        UINT32 cpuNum,
        const IADDR_CLASS ipVA,
        const IADDR_CLASS ipPA,
        SOFTSDV_LIVE_INSTRUCTION targetHandle,
        ASIM_MACRO_INST asimInstr);

    bool
    GetInputPredicateValue(
        UINT32 cpuNum,
        SOFTSDV_LIVE_INSTRUCTION instrHandle,
        INT32 regNum,
        ARCH_REGISTER reg);

    bool
    GetOutputPredicateValue(
        UINT32 cpuNum,
        SOFTSDV_LIVE_INSTRUCTION instrHandle,
        INT32 regNum,
        ARCH_REGISTER reg);

    void DumpISAPerCPUStats(STATE_OUT state_out, UINT32 cpuNum);
};

typedef SOFTSDV_MACRO_FEEDER_CLASS *SOFTSDV_MACRO_FEEDER;

//
// Right now we only support one macro feeder at a time and it must be
// named MACRO_FEEDER.
//
typedef SOFTSDV_MACRO_FEEDER_CLASS MACRO_FEEDER_CLASS;
typedef MACRO_FEEDER_CLASS *MACRO_FEEDER;
