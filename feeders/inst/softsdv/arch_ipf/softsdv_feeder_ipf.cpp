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
 * @brief IPF specific portion of SoftSDV instruction feeder
 */


// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/trace.h"

// ASIM public modules
#include "asim/provides/instfeeder_interface.h"
#include "asim/provides/instfeeder_implementation.h"

SOFTSDV_FEEDER_IPF_CLASS::SOFTSDV_FEEDER_IPF_CLASS(void) :
    SOFTSDV_FEEDER_BASE_CLASS()
{
    ASSERTX(SYLLABLES_PER_BUNDLE == 3);

    IPF_RAW_SYLLABLE syllable[SYLLABLES_PER_BUNDLE];

    //
    // Build nop.m, nop.i, nop.i
    //
    syllable[0].Reset();
    syllable[0].SetNopMunit();
    syllable[1].Reset();
    syllable[1].SetNopIunit();
    syllable[2].Reset();
    syllable[2].SetNopIunit();
    bundleNop.SetTemplate(IPF_TEMPLATE_MII);
    bundleNop.SetSlots(syllable);

    //
    // Build br.none, br.none, br.none
    //
    syllable[0].Reset();
    syllable[0].SetIndirectBranch(0, 1);
    syllable[1].Reset();
    syllable[1].SetIndirectBranch(0, 2);
    syllable[2].Reset();
    syllable[2].SetIndirectBranch(0, 3);
    bundleBranch.SetTemplate(IPF_TEMPLATE_BBB);
    bundleBranch.SetSlots(syllable);

    syllable[0].Reset();
    syllable[0].SetXOR(1, 2, 3);
    syllable[1].Reset();
    syllable[1].SetXOR(4, 5, 6);
    syllable[2].Reset();
    syllable[2].SetXOR(7, 8, 9);
    bundleXor.SetTemplate(IPF_TEMPLATE_MII);
    bundleXor.SetSlots(syllable);

}


void
SOFTSDV_FEEDER_IPF_CLASS::InitISASpecific(
    UINT32 argc,
    char **argv,
    char **envp)
{
    int nCpus = asimIO->NCpus();
    ipfStats.Init(nCpus);
}


//
// NoteMemoryRefForWarmup --
//     Called while handling warm-up data to fill in details of loads
//     and stores.
//
void
SOFTSDV_FEEDER_IPF_CLASS::NoteMemoryRefForWarmup(
    UINT32 cpuNum,
    SOFTSDV_LIVE_INSTRUCTION instrHandle,
    WARMUP_INFO warmup)
{
    ASIM_MACRO_INST inst = NULL;

    //
    // We must decode the instruction to find out whether it is predicated.
    //
    if (warmup->IsAsimInstValid())
    {
        // Decode already cached in the warm-up data
        inst = warmup->GetAsimInst();
    }
    else
    {
        inst = warmup->InitAsimInst();
        AsimInstFromSoftsdvInst(cpuNum, instrHandle, inst, true, 0);
        inst->SetTraceID((PTR_SIZED_UINT)instrHandle);
    }

    if (inst->GetQP() && inst->HasEffAddress())
    {
        UINT32 accessSize = 8;
        if (inst->IsLoad() || inst->IsStore())
        {
            accessSize = inst->GetAccessSize();
        }

        if (inst->IsStore() && instrHandle->NStores())
        {
            UINT64 va = instrHandle->GetStore(0)->GetVA();
            UINT64 pa = instrHandle->GetStore(0)->PTranslate(va);
            warmup->NoteStore(va, pa, accessSize);
        }
        else if (instrHandle->NLoads())
        {
            UINT64 va = instrHandle->GetLoad(0)->GetVA();
            UINT64 pa = instrHandle->GetLoad(0)->PTranslate(va);
            warmup->NoteLoad(va, pa, accessSize);
        }
    }
}


bool
SOFTSDV_FEEDER_IPF_CLASS::AsimInstFromSoftsdvInst(
    UINT32 cpuNum,
    SOFTSDV_LIVE_INSTRUCTION instrHandle,
    ASIM_MACRO_INST asimInstr,
    bool warmUp,
    UINT64 cycle)
{
    CONST_ASIM_SOFTSDV_INST_INFO sdvInstr = instrHandle->GetSdvInstr();

    IADDR_CLASS instrIPVirtual(sdvInstr->GetAddrVirtual(), sdvInstr->GetAddrVirtual());
   
    decode_cache.Init(asimInstr, sdvInstr->GetBundle64(), instrIPVirtual);

    asimInstr->SetPhysicalPC( sdvInstr->GetAddrPhysical() );

    asimInstr->SetCFM(sdvInstr->GetCfmBefore(), sdvInstr->GetCfmAfter());
    asimInstr->SetBSP(sdvInstr->GetBspBefore(), sdvInstr->GetBspAfter());
    asimInstr->SetPRF(sdvInstr->GetPredsBefore_Physical(),
                      sdvInstr->GetPredsAfter_Physical());

    if (asimInstr->IsControlOp())
    {
        if (sdvInstr->GetType() != ASIM_SOFTSDV_INST_INFO_CLASS::CONTROL)
        {
            if (asimInstr->IsBreak())
            {
                //
                // break instructions in application mode seem not to be
                // tagged as control.  No big deal.
                //
            }
            else if (asimInstr->GetQP())
            {
                cerr << "SoftSDV missed a control instruction:  ("
                     << asimInstr->GetVirtualPC() << "):  "
                     << asimInstr->GetDisassembly() << endl;
            }
            
            asimInstr->SetActualTaken(false);
            asimInstr->SetActualTarget(instrIPVirtual.Next());
        }
        else
        {
            asimInstr->SetActualTaken(sdvInstr->GetBranchTaken());
            if (sdvInstr->GetBranchTaken())
            {
                IADDR_CLASS targetVA(sdvInstr->GetTargetVA(),
                                     sdvInstr->GetTargetVA());
                asimInstr->SetActualTarget(targetVA);
            }
            else
            {
                asimInstr->SetActualTarget(instrIPVirtual.Next());
            }
        }
    }
    else if (asimInstr->HasEffAddress())
    {
        if (sdvInstr->GetType() != ASIM_SOFTSDV_INST_INFO_CLASS::LOAD &&
            sdvInstr->GetType() != ASIM_SOFTSDV_INST_INFO_CLASS::STORE)
        {
            if (asimInstr->GetQP())
            {
                //
                // This can actually happen in a few cases.  The most common
                // is an FP load where the register target isn't an active
                // register.
                //
                cerr << "SoftSDV missed mem instr:  ("
                     << asimInstr->GetVirtualPC() << "):  "
                     << asimInstr->GetDisassembly() << endl;
            }

            asimInstr->SetVirtualEffAddress(0);
        }
        else
        {
            UINT64 va = 0;
            UINT64 pa = 0;

            if ((sdvInstr->GetType() == ASIM_SOFTSDV_INST_INFO_CLASS::LOAD) &&
                instrHandle->NLoads())
            {
                va = instrHandle->GetLoad(0)->GetVA();
                pa = instrHandle->GetLoad(0)->PTranslate(va);
            }
            else if (instrHandle->NStores())
            {
                va = instrHandle->GetStore(0)->GetVA();
                pa = instrHandle->GetStore(0)->PTranslate(va);
            }

            asimInstr->SetVirtualEffAddress(va);

            if ((va != 0) && (pa == 0) && (asimInstr->GetQP()))
            {
                cpuStats[cpuNum].nDTranslateUnavailable += 1;
            }
            else
            {
                asimInstr->SetPhysicalEffAddress(pa);
            }
        }
    }

    if (asimInstr->IsNOP() || asimInstr->IsTypeL())
    {
        instrHandle->SetImplicitCommit(true);
    }

    if (sdvInstr->GetKernelInstr())
    {
        asimInstr->SetKernelInstr(true);
    }

    if (sdvInstr->GetIdleInstr())
    {
        asimInstr->SetIdlePauseOp(true);
        if (! warmUp)
        {
            cpuStats[cpuNum].nIdlePauseInstrs += 1;
        }
    }

    return true;
}


SOFTSDV_LIVE_INSTRUCTION
SOFTSDV_FEEDER_IPF_CLASS::NoteNewIncomingInstr(
    UINT32 cpuNum,
    ASIM_SOFTSDV_INST_INFO sdvInstr)
{
    int ipf_template = sdvInstr->GetBundle8()[0] & 31;
    if (((sdvInstr->GetAddrVirtual() & 3) == 1) &&
        (ipf_template == 4 || ipf_template == 5))
    {
        ASIM_SOFTSDV_INST_INFO_CLASS newInstr = *sdvInstr;

        newInstr.SetAddrVirtual(newInstr.GetAddrVirtual()+1);
        newInstr.SetAddrPhysical(newInstr.GetAddrPhysical()+1);
        ipfStats[cpuNum].nInjectedXInstrs += 1;
        return cpuState[cpuNum].liveInstrs.AddInstruction(&newInstr);
    }

    return NULL;
}


void
SOFTSDV_FEEDER_IPF_CLASS::InjectNOP(
    UINT32 cpuNum,
    const IADDR_CLASS ipVA,
    const IADDR_CLASS ipPA,
    ASIM_MACRO_INST asimInstr)
{
    decode_cache.Init(asimInstr,&bundleNop, ipVA);
//    asimInstr->Init(&bundleNop, ipVA);
    asimInstr->SetTraceID(traceArtificialInstId);
}


void
SOFTSDV_FEEDER_IPF_CLASS::InjectBadPathInstr(
    UINT32 cpuNum,
    const IADDR_CLASS ipVA,
    const IADDR_CLASS ipPA,
    ASIM_MACRO_INST asimInstr)
{
    decode_cache.Init(asimInstr,&bundleXor, ipVA);
//    asimInstr->Init(&bundleXor, ipVA);
    asimInstr->SetTraceID(traceWrongPathId);
}


void
SOFTSDV_FEEDER_IPF_CLASS::InjectJMP(
    UINT32 cpuNum,
    const IADDR_CLASS ipVA,
    const IADDR_CLASS ipPA,
    SOFTSDV_LIVE_INSTRUCTION targetHandle,
    ASIM_MACRO_INST asimInstr)
{
    asimInstr->Init(&bundleBranch, ipVA);

    CONST_ASIM_SOFTSDV_INST_INFO targetInstr = targetHandle->GetSdvInstr();

    asimInstr->SetActualTaken(true);
    IADDR_CLASS targetVA(targetInstr->GetAddrVirtual(), targetInstr->GetAddrVirtual());
    asimInstr->SetActualTarget(targetVA);

    asimInstr->SetCFM(targetInstr->GetCfmBefore(), targetInstr->GetCfmBefore());
    asimInstr->SetBSP(targetInstr->GetBspBefore(), targetInstr->GetBspBefore());
    asimInstr->SetPRF(targetInstr->GetPredsBefore_Physical(),
                      targetInstr->GetPredsBefore_Physical());

    SOFTSDV_LIVE_INSTRUCTION jmpHandle;
    jmpHandle = cpuState[cpuNum].liveInstrs.AddArtificialPlaceHolder(targetHandle);
    asimInstr->SetTraceID((PTR_SIZED_UINT)jmpHandle);
}


//
// Controlling predicate value.  Logical (not physical) register numbering.
//
bool
SOFTSDV_FEEDER_IPF_CLASS::GetPredicateRegVal(
    ASIM_MACRO_INST inst,
    UINT32 slot,
    ARCH_REGISTER reg)
{
    if ((slot > 0) || ! inst->TakesPredicate())
    {
        return false;
    }

    UINT64 value = inst->GetQP() ? 1 : 0;

    *reg = ARCH_REGISTER_CLASS(REG_TYPE_PREDICATE,
                               inst->GetLogicalPredicateNum(),
                               value);

    return true;
}


//
// Any predicate input.  Logical register numbering.
//
bool
SOFTSDV_FEEDER_IPF_CLASS::GetInputPredicateValue(
    UINT32 cpuNum,
    SOFTSDV_LIVE_INSTRUCTION instrHandle,
    INT32 regNum,
    ARCH_REGISTER reg)
{
    CONST_ASIM_SOFTSDV_INST_INFO sdvInstr = instrHandle->GetSdvInstr();

    *reg = ARCH_REGISTER_CLASS(REG_TYPE_PREDICATE,
                               regNum,
                               (sdvInstr->GetPredsBefore_Logical() >> regNum) & 1);
    return true;
}


//
// Any predicate output.  Logical register numbering.
//
bool
SOFTSDV_FEEDER_IPF_CLASS::GetOutputPredicateValue(
    UINT32 cpuNum,
    SOFTSDV_LIVE_INSTRUCTION instrHandle,
    INT32 regNum,
    ARCH_REGISTER reg)
{
    CONST_ASIM_SOFTSDV_INST_INFO sdvInstr = instrHandle->GetSdvInstr();

    *reg = ARCH_REGISTER_CLASS(REG_TYPE_PREDICATE,
                               regNum,
                               (sdvInstr->GetPredsAfter_Logical() >> regNum) & 1);
    return true;
}

//----------------------------------------------------------------
// Statistics
//----------------------------------------------------------------

void
SOFTSDV_FEEDER_IPF_CLASS::DumpISAPerCPUStats(
    STATE_OUT state_out,
    UINT32 cpuNum
    )
{
    state_out->AddScalar("uint",
                         "instrs_x_immediate",
                         "Total 64-bit immediate instruction placeholders added",
                         ipfStats[cpuNum].nInjectedXInstrs);
}

//----------------------------------------------------------------
// Global feeder control functions
//----------------------------------------------------------------

IFEEDER_BASE
IFEEDER_New(void)
{
    return new SOFTSDV_FEEDER_IPF_CLASS();
}


void
IFEEDER_Usage(FILE *file)
{
}
