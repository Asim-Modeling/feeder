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
 * @brief SoftSDV IPF Instruction Feeder
 */

// ASIM private modules
#include "asim/provides/softsdv_feeder.h"
#include "asim/provides/decode_cache.h"

class SOFTSDV_FEEDER_IPF_CLASS : public SOFTSDV_FEEDER_BASE_CLASS
{
  public:
    SOFTSDV_FEEDER_IPF_CLASS(void);
    ~SOFTSDV_FEEDER_IPF_CLASS() {};

    //
    // IPF specific register value code.  ISA neutral input and output value
    // functions are defined in SOFTSDV_FEEDER_BASE_CLASS.
    //
    // Predicate register numbering is logical (virtual), not physical.
    //
    bool GetPredicateRegVal(ASIM_MACRO_INST inst, UINT32 slot, ARCH_REGISTER reg);

  private:
    class IPF_CPU_STATISTICS_CLASS
    {
      public:
        IPF_CPU_STATISTICS_CLASS() :
            nInjectedXInstrs(0)
        {};

        UINT64 nInjectedXInstrs;
    };

    //
    //Decode cache for speeding up asim
    //
    IPF_DECODE_CACHE_CLASS decode_cache;

    DYNAMIC_ARRAY_CLASS<IPF_CPU_STATISTICS_CLASS> ipfStats;

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

    //
    // Cache some IPF raw bundles for commonly synthesized instructions.
    //
    IPF_RAW_BUNDLE bundleNop;
    IPF_RAW_BUNDLE bundleBranch;
    IPF_RAW_BUNDLE bundleXor;
};

typedef SOFTSDV_FEEDER_IPF_CLASS *SOFTSDV_FEEDER_IPF;
