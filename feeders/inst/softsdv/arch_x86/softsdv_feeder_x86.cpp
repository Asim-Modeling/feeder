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
 * @brief x86 specific portion of SoftSDV instruction feeder
 */

#include <string.h>
#include <stdlib.h>
#include <list>

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/trace.h"

// ASIM public modules
#include "asim/provides/instfeeder_interface.h"
#include "asim/provides/instfeeder_implementation.h"


SOFTSDV_MACRO_FEEDER_CLASS::SOFTSDV_MACRO_FEEDER_CLASS(IFEEDER_BASE microFeeder) :
    SOFTSDV_FEEDER_BASE_CLASS(microFeeder)
{
}

void
SOFTSDV_MACRO_FEEDER_CLASS::InitISASpecific(
    UINT32 argc,
    char **argv,
    char **envp)
{
    REGISTER_NAMES_CLASS rNames;
    ASIM_SOFTSDV_REQUEST request;
    UINT32 nMonRegs = 0;

    //
    // Pass the register names and numbers we'll be monitoring to SoftSDV
    //
    UINT32 nRegs = rNames.NumRegs();
    for (UINT32 i = 0; i < nRegs; i++)
    {
        if (rNames.IsArchReg(i) && !rNames.HasParent(i))
        {
            const char *name = rNames.Name(i);
            const char *sdvName = rNames.SoftSDVName(i);

            if (! strcmp(sdvName, "arch.ia32.register.rip") ||
                ! strcmp(sdvName, "arch.ia32.register.eip"))
            {
                // Don't monitor the IP.  We get that through other methods.
            }
            else if (! strncmp(sdvName, "arch.ia32.register.mm", 21))
            {
                // MM registers not available in SoftSDV
            }
            else if (! strcmp(sdvName, "arch.ia32.register.trncr") ||
                     ! strcmp(sdvName, "arch.ia32.register.trnsr"))
            {
                // Transaction execution control and status.  Experimental
                // registers not implemented in SoftSDV.
            }
            else
            {
                request = asimIO->AsimRequestQueue().OpenNext();
                *request = ASIM_SOFTSDV_REQUEST_CLASS(
                    ASIM_REQUEST_REG_MONITOR,
                    sdvName, i, rNames.Size(i));
                asimIO->AsimRequestQueue().Close(request);
                nMonRegs += 1;
            }
        }
    }

    T1("SoftSDV feeder monitoring " << nMonRegs << " x86 architectural registers.");
}


//
// NoteMemoryRefForWarmup --
//     Called while handling warm-up data to fill in details of loads
//     and stores.
//
void
SOFTSDV_MACRO_FEEDER_CLASS::NoteMemoryRefForWarmup(
    UINT32 cpuNum,
    SOFTSDV_LIVE_INSTRUCTION instrHandle,
    WARMUP_INFO warmup)
{
    //
    // Start with loads.  Each reference region may touch multiple pages.
    // Walk through all virtual regions read by the instruction and break
    // them down into contiguous physical regions.
    //
    for (UINT32 i = 0; i < instrHandle->NLoads(); i++)
    {
        const ASIM_SOFTSDV_MEM_ACCESS_CLASS *ref = instrHandle->GetLoad(i);
        MEMORY_VIRTUAL_REFERENCE_CLASS vRegion(ref->GetVA(), ref->GetSize());
        while (vRegion.GetNBytes() != 0)
        {
            MEMORY_VIRTUAL_REFERENCE_CLASS vNext;
            UINT64 pa = ref->PTranslate(vRegion, vNext);
            if (pa != 0)
            {
                warmup->NoteLoad(vRegion.GetVA(), pa,
                                 vRegion.GetNBytes() - vNext.GetNBytes());
            }
            vRegion = vNext;
        }
    }

    for (UINT32 i = 0; i < instrHandle->NStores(); i++)
    {
        const ASIM_SOFTSDV_MEM_ACCESS_CLASS *ref = instrHandle->GetStore(i);
        MEMORY_VIRTUAL_REFERENCE_CLASS vRegion(ref->GetVA(), ref->GetSize());
        while (vRegion.GetNBytes() != 0)
        {
            MEMORY_VIRTUAL_REFERENCE_CLASS vNext;
            UINT64 pa = ref->PTranslate(vRegion, vNext);
            if (pa != 0)
            {
                warmup->NoteStore(vRegion.GetVA(), pa,
                                  vRegion.GetNBytes() - vNext.GetNBytes());
            }
            vRegion = vNext;
        }
    }
}


bool
SOFTSDV_MACRO_FEEDER_CLASS::AsimInstFromSoftsdvInst(
    UINT32 cpuNum,
    SOFTSDV_LIVE_INSTRUCTION instrHandle,
    ASIM_MACRO_INST asimInstr,
    bool warmUp,
    UINT64 cycle)
{
    CONST_ASIM_SOFTSDV_INST_INFO sdvInstr = instrHandle->GetSdvInstr();

    asimInstr->Init(sdvInstr->GetInstrBytes(),
                    sdvInstr->GetAddrVirtual(),
                    sdvInstr->GetInstrNBytes());
    asimInstr->SetPhysicalPC(sdvInstr->GetAddrPhysical());
    asimInstr->SetCpuMode(sdvInstr->GetCpuMode());

    if (sdvInstr->IsControl())
    {
        asimInstr->SetControlOp();
        bool isTaken = sdvInstr->GetBranchTaken();
        asimInstr->SetActualTaken(isTaken);
        if (isTaken)
        {
            asimInstr->SetActualTarget(IADDR_CLASS(sdvInstr->GetTargetVA()));
        }
        else
        {
            asimInstr->SetActualTarget(asimInstr->GetVirtualPC().NextMacro());
        }
    }
    else if (sdvInstr->IsRepeat())
    {
        //
        // REPeat instruction.  SoftSDV executes one iteration of the
        // repeat per instruction.  Merge them into a single macro.
        //
        asimInstr->SetRepeatOp();
        MergeREPInstrs(cpuNum, instrHandle, cycle);
    }

    //
    // Most of the code in the feeder that walks the loads and stores uses
    // the instruction handle.  This code must not!  The handle returns
    // details about merged REP instructions.  The instruction itself does
    // not.  However, the REP merge code adjusts the reference in the
    // instruction itself to describe the entire virtual region of the
    // merged reference.  This is different from querying the handle, since
    // the referenes in the handle are limited to 2KB regions due to
    // limitations in the virtual to physical mapping data structure.
    // Since the x86 macro instruction has limited space for storing
    // virtual regions we must store the merged virtual regions in the
    // ASIM_MACRO_INST.
    //
    for (UINT32 i = 0; i < sdvInstr->NLoads(); i++)
    {
        MEMORY_VIRTUAL_REFERENCE_CLASS vRef(sdvInstr->GetLoad(i)->GetVA(),
                                            sdvInstr->GetLoad(i)->GetSize());
        asimInstr->SetLoadRef(i, vRef);

        UINT64 pa = sdvInstr->GetLoad(i)->PTranslate(vRef.GetVA());

        if ((vRef.GetVA() != 0) && (pa == 0))
        {
            cpuStats[cpuNum].nDTranslateUnavailable += 1;
        }
        if (sdvInstr->GetLoad(i)->GetNPages() > 1)
        {
            cpuStats[cpuNum].nDTranslateMultipage += 1;
        }
    }

    for (UINT32 i = 0; i < sdvInstr->NStores(); i++)
    {
        MEMORY_VIRTUAL_REFERENCE_CLASS vRef(sdvInstr->GetStore(i)->GetVA(),
                                            sdvInstr->GetStore(i)->GetSize());
        asimInstr->SetStoreRef(i, vRef);

        UINT64 pa = sdvInstr->GetStore(i)->PTranslate(vRef.GetVA());

        if ((vRef.GetVA() != 0) && (pa == 0))
        {
            cpuStats[cpuNum].nDTranslateUnavailable += 1;
        }
        if (sdvInstr->GetStore(i)->GetNPages() > 1)
        {
            cpuStats[cpuNum].nDTranslateMultipage += 1;
        }
    }

    if (sdvInstr->GetKernelInstr())
    {
        asimInstr->SetKernelInstr();
    }

    if (! warmUp)
    {
        if (sdvInstr->GetIdleInstr())
        {
            asimInstr->SetIdlePauseOp();
            cpuStats[cpuNum].nIdlePauseInstrs += 1;
        }

        if (sdvInstr->GetPauseInstr())
        {
            asimInstr->SetPauseOp();
            if (! sdvInstr->GetIdleInstr())
            {
                cpuStats[cpuNum].nSpinLoopTrips += 1;
            }
        }
    }

    //
    // If the next instruction is an interrupt or new sequence, trigger
    // an interrupt request now.  This is not strictly correct, since some
    // part of the current instruction should probably execute in order
    // to trigger the fault.  This can be fixed but is complicated since
    // delaying the IRQ may cause Archlib to trigger a fault.
    //
    if (! warmUp && (instrHandle->GetNext() != NULL))
    {
        CONST_ASIM_SOFTSDV_INST_INFO nextInstr;
        nextInstr = instrHandle->GetNext()->GetSdvInstr();
        
        if (nextInstr->GetNewSequence() || nextInstr->GetInterrupt())
        {
            //
            // Inject IRQ and make the current instruction commit implicitly
            // in case the model bypasses it.
            //
            instrHandle->SetImplicitCommit(true);

            cpuState[cpuNum].PushSystemEvent(
                SDV_SYSTEM_EVENT_CLASS(asimInstr->GetUid(),
                                       FEEDER_SYSTEM_EVENT_CLASS(
                                           IRQ_FEEDER_SYSTEM_EVENT,
                                           nextInstr->GetAddrVirtual())));

            T1("\tSoftSDV: FEED_Fetch triggering event (m_uid="
               << asimInstr->GetUid() << ") -- IRQ target VA 0x"
               << fmt_x(nextInstr->GetAddrVirtual()));
        }
    }

    return true;
}


void
SOFTSDV_MACRO_FEEDER_CLASS::MergeREPInstrs(
    UINT32 cpuNum,
    SOFTSDV_LIVE_INSTRUCTION instrHandle,
    UINT64 cycle)
{
    enum constants
    {
        MAX_LOADS = 2,
        MAX_STORES = 1,
        MAX_REFS = MAX_LOADS > MAX_STORES ? MAX_LOADS : MAX_STORES,
        REF_REGION_CHUNK = 2048
    };

    CONST_ASIM_SOFTSDV_INST_INFO baseInstr = instrHandle->GetSdvInstr();
    SOFTSDV_LIVE_INSTRUCTION nextSdvHandle = instrHandle->GetNext();
    
    CONST_ASIM_SOFTSDV_INST_INFO nextSdvInstr = nextSdvHandle->GetSdvInstr();

    if ((nextSdvHandle == NULL) || nextSdvInstr->GetWarmUp() ||
        (baseInstr->GetAddrVirtual() != nextSdvInstr->GetAddrVirtual()))
    {
        // Nothing to merge
        return;
    }

    //
    // Determine number of references to track per instruction, sizes,
    // and direction.
    //

    UINT32 nRefs[2];        // Load count in index 0, stores in 1
    UINT32 refSize = 0;

    // How many loads and stores?  References specified in the instruction are
    // first.
    for (unsigned int ld_st = 0; ld_st < 2; ld_st++)
    {
        bool getLoads = (ld_st == 0);

        nRefs[ld_st] = 0;

        while ((nRefs[ld_st] < baseInstr->NRefs(getLoads)) &&
               (baseInstr->GetRef(getLoads, nRefs[ld_st])->GetRefSource() == SDV_MEM_REF_INSTRUCTION))
        {
            UINT32 size = baseInstr->GetRef(getLoads, nRefs[ld_st])->GetSize();
            if (refSize == 0)
            {
                refSize = size;
            }
            else
            {
                ASSERTX(refSize == size);
            }

            nRefs[ld_st] += 1;
        }
    }

    ASSERTX((nRefs[0] + nRefs[1] > 0) && (nRefs[0] <= MAX_LOADS) && (nRefs[1] <= MAX_STORES));
    // Size must be a power of two <= 8
    ASSERTX((refSize > 0) && (refSize <= 8) && (((refSize - 1) & refSize) == 0));

    // Direction and use values?
    bool moveForward;
    bool useValues;

    // Look at the first load or store (if no loads) to compute direction
    moveForward = baseInstr->GetRef(nRefs[0], 0)->GetVA() < nextSdvInstr->GetRef(nRefs[0], 0)->GetVA();
    useValues = baseInstr->GetRef(nRefs[0], 0)->HasValue();

    //
    // These will hold new descriptors for merged regions.  Loads in index
    // 0, stores in 1.
    //
    ASIM_SOFTSDV_MEM_ACCESS newRefs[2][MAX_REFS];
            
    //
    // A single REP instruction may touch a lot of memory, but the
    // ASIM_SOFTSDV_MEM_ACCESS_CLASS is incapable of describing the crossing
    // of more than one page boundary.  Instead of coming up with a data
    // structure just for REP, store the references for the REP in multiple
    // instances of the reference class.  Each instance will represent at
    // most REP_REGION_CHUNK bytes.  Full, merged regions are stored on
    // these lists:
    //
    typedef list<ASIM_SOFTSDV_MEM_ACCESS> MEM_ACCESS_LIST;
    MEM_ACCESS_LIST newRegions[2];
    UINT32 listElements = 0;

    for (unsigned int ld_st = 0; ld_st < 2; ld_st++)
    {
        for (unsigned int i = 0; i < nRefs[ld_st]; i++)
        {
            newRefs[ld_st][i] =
                InitREPRegion(baseInstr->GetRef(ld_st == 0, i), refSize,
                              moveForward, useValues, REF_REGION_CHUNK);
        }
    }

    UINT32 newRegionSize = refSize;

    //
    // Walk through all the REP instructions that follow and merge the
    // reference regions.
    //
    while ((nextSdvHandle != NULL) &&
           (baseInstr->GetAddrVirtual() == nextSdvInstr->GetAddrVirtual()) &&
           ! nextSdvInstr->GetWarmUp())
    {
        ASSERTX(newRegionSize <= REF_REGION_CHUNK);

        if (newRegionSize == REF_REGION_CHUNK)
        {
            // Merged regions are full.  Push the full ones on the region
            // lists and allocate new merge regions.
            listElements += 1;
            for (unsigned int ld_st = 0; ld_st < 2; ld_st++)
            {
                for (unsigned int i = 0; i < nRefs[ld_st]; i++)
                {
                    newRegions[ld_st].push_front(newRefs[ld_st][i]);
                    newRefs[ld_st][i] =
                        InitREPRegion(nextSdvInstr->GetRef(ld_st == 0, i),
                                      refSize, moveForward, useValues,
                                      REF_REGION_CHUNK);
                }
            }

            newRegionSize = refSize;
        }
        else
        {
            // Merge new instruction into the current buffer
            for (unsigned int ld_st = 0; ld_st < 2; ld_st++)
            {
                for (unsigned int i = 0; i < nRefs[ld_st]; i++)
                {
                    MergeREPRegion(newRefs[ld_st][i],
                                   nextSdvInstr->GetRef(ld_st == 0, i),
                                   refSize, moveForward, useValues,
                                   REF_REGION_CHUNK);
                }
            }

            newRegionSize += refSize;
        }

        // All information extracted.  Remove this instruction.
        nextSdvHandle->DeleteInstr();

        // Refill the incoming instruction buffer
        FetchIncomingInstructions(cpuNum, SOFTSDV_EXECUTE_CHUNK_SIZE, cycle);
        nextSdvHandle = instrHandle->GetNext();
        nextSdvInstr = nextSdvHandle->GetSdvInstr();
    }

    //
    // For REP going backward through memory, the values in the buffer
    // are at the wrong end.  Shift them to the start of the buffer.
    //
    if (useValues && ! moveForward && (newRegionSize != REF_REGION_CHUNK))
    {
        for (unsigned int ld_st = 0; ld_st < 2; ld_st++)
        {
            for (unsigned int i = 0; i < nRefs[ld_st]; i++)
            {
                memmove(newRefs[ld_st][i]->GetValue(),
                        newRefs[ld_st][i]->GetValue() + REF_REGION_CHUNK - newRegionSize,
                        newRegionSize);
            }
        }
    }

    //
    // Update the merged instruction with the true virtual regions.
    // Only the code in AsimInstFromSoftsdvInst will see these regions.
    // See the long comment there.
    //
    for (unsigned int ld_st = 0; ld_st < 2; ld_st++)
    {
        for (unsigned int i = 0; i < nRefs[ld_st]; i++)
        {
            UINT64 size = newRegionSize + listElements * REF_REGION_CHUNK;
            UINT64 va;
            if (moveForward)
            {
                va = baseInstr->GetRef(ld_st == 0, i)->GetVA();
            }
            else
            {
                va = newRefs[ld_st][i]->GetVA();
            }
            
            MEMORY_VIRTUAL_REFERENCE_CLASS vRef(va, size);
            if (ld_st == 0)
            {
                instrHandle->UpdateLoadVirtualRegion(i, vRef);
            }
            else
            {
                instrHandle->UpdateStoreVirtualRegion(i, vRef);
            }
        }
    }

    //
    // Build the final REP descriptor with the new regions and attach
    // it to the instruction in the queue.
    //
    SOFTSDV_REP_MEMORY_REGIONS regions;
    regions = new SOFTSDV_REP_MEMORY_REGIONS_CLASS(nRefs[0] * (1 + listElements),
                                                   nRefs[1] * (1 + listElements));
    UINT32 idx = 0;
    while (idx < nRefs[0])
    {
        regions->SetLoad(idx, newRefs[0][idx]);
        idx += 1;
    }

    MEM_ACCESS_LIST::iterator refs = newRegions[0].begin();
    while (refs != newRegions[0].end())
    {
        regions->SetLoad(idx, *refs++);
        idx += 1;
    }

    idx = 0;
    while (idx < nRefs[1])
    {
        regions->SetStore(idx, newRefs[1][idx]);
        idx += 1;
    }

    refs = newRegions[1].begin();
    while (refs != newRegions[1].end())
    {
        regions->SetStore(idx, *refs++);
        idx += 1;
    }

    instrHandle->SetREPMemRegions(regions);
}


ASIM_SOFTSDV_MEM_ACCESS
SOFTSDV_MACRO_FEEDER_CLASS::InitREPRegion(
    const ASIM_SOFTSDV_MEM_ACCESS_CLASS *oldRef,
    UINT32 refSize,
    bool moveForward,
    bool useValues,
    UINT32 valueMaxSize)
{
    ASIM_SOFTSDV_MEM_ACCESS newRef = new ASIM_SOFTSDV_MEM_ACCESS_CLASS();

    newRef->SetVA(oldRef->GetVA());
    newRef->SetPA(oldRef->GetPA(0), 0);

    // Does the reference span two pages?
    if (oldRef->GetNPages() > 1)
    {
        UINT64 lastPA = oldRef->PTranslate(oldRef->GetVA() + refSize - 1);
        if ((lastPA & oldRef->PageMask()) != oldRef->GetPA(0))
        {
            newRef->SetPA(lastPA, 1);
        }
    }

    ASSERTX(oldRef->GetRefSource() == SDV_MEM_REF_INSTRUCTION);
    newRef->SetRefSource(SDV_MEM_REF_INSTRUCTION);

    ASSERTX(oldRef->GetSize() == refSize);
    newRef->SetSize(refSize);

    if (useValues)
    {
        unsigned char *value = (unsigned char *)malloc(valueMaxSize);
        VERIFY(value != NULL, "Out of memory");
        bzero(value, valueMaxSize);
        newRef->AllocValueStorage(value);

        // Hack to make the descriptor believe that the value is set
        UINT8 zero = 0;
        newRef->SetValue(&zero, 1, 0);

        // Put the value in the right place
        unsigned char *dst = value;
        if (! moveForward)
        {
            dst += valueMaxSize - refSize;
        }
        memcpy(dst, oldRef->GetValue(), refSize);
    }

    return newRef;
}


void
SOFTSDV_MACRO_FEEDER_CLASS::MergeREPRegion(
    ASIM_SOFTSDV_MEM_ACCESS mergeRef,
    const ASIM_SOFTSDV_MEM_ACCESS_CLASS *oldRef,
    UINT32 refSize,
    bool moveForward,
    bool useValues,
    UINT32 valueMaxSize)
{
    ASSERTX(oldRef->GetRefSource() == SDV_MEM_REF_INSTRUCTION);
    VERIFYX(oldRef->GetSize() == refSize);

    //
    // Combine the address regions
    //
    if (moveForward)
    {
        VERIFYX(mergeRef->GetVA() + mergeRef->GetSize() == oldRef->GetVA());
    }
    else
    {
        VERIFYX(mergeRef->GetVA() == oldRef->GetVA() + refSize);
        mergeRef->SetVA(mergeRef->GetVA() - refSize);
    }
    mergeRef->SetSize(mergeRef->GetSize() + refSize);

    //
    // Does the reference touch a new page?
    //
    ASSERTX(mergeRef->GetNPages() > 0);
    if (moveForward)
    {
        UINT32 pIdx = mergeRef->GetNPages() - 1;
        UINT64 oldPA = oldRef->PTranslate(oldRef->GetVA() + refSize - 1) &
                       oldRef->PageMask();

        if (mergeRef->GetPA(pIdx) != oldPA)
        {
            // Add the new page to the end
            mergeRef->SetPA(oldPA, pIdx + 1);
        }
    }
    else
    {
        UINT64 oldPA = oldRef->PTranslate(oldRef->GetVA()) & oldRef->PageMask();
        if (mergeRef->GetPA(0) != oldPA)
        {
            //
            // Move all pages up in the list and add the new one at the beginning
            //
            UINT32 nPages = mergeRef->GetNPages();
            for (unsigned int i = nPages; i > 0; i--)
            {
                mergeRef->SetPA(mergeRef->GetPA(i-1), i);
            }
            mergeRef->SetPA(oldPA, 0);
        }
    }

    //
    // Combine values
    //
    ASSERTX(useValues == oldRef->HasValue());
    if (useValues)
    {
        unsigned char *dst;
        if (moveForward)
        {
            dst = mergeRef->GetValue() + mergeRef->GetSize() - refSize;
        }
        else
        {
            dst = mergeRef->GetValue() + valueMaxSize - mergeRef->GetSize();
        }
        memcpy(dst, oldRef->GetValue(), refSize);
    }
}


SOFTSDV_LIVE_INSTRUCTION
SOFTSDV_MACRO_FEEDER_CLASS::NoteNewIncomingInstr(
    UINT32 cpuNum,
    ASIM_SOFTSDV_INST_INFO sdvInstr)
{
    return NULL;
}


void
SOFTSDV_MACRO_FEEDER_CLASS::InjectNOP(
    UINT32 cpuNum,
    const IADDR_CLASS ipVA,
    const IADDR_CLASS ipPA,
    ASIM_MACRO_INST asimInstr)
{
    static UINT8 nopOpcode[] = { 0x90 };

    asimInstr->Init(nopOpcode, ipVA.GetMacro(), sizeof(nopOpcode));
    asimInstr->SetTraceID(traceArtificialInstId);
    asimInstr->SetDis("NOP");
}


void
SOFTSDV_MACRO_FEEDER_CLASS::InjectBadPathInstr(
    UINT32 cpuNum,
    const IADDR_CLASS ipVA,
    const IADDR_CLASS ipPA,
    ASIM_MACRO_INST asimInstr)
{
    static UINT8 xorOpcode[] = { 0x31, 0xc1 };

    asimInstr->Init(xorOpcode, ipVA.GetMacro(), sizeof(xorOpcode));
    asimInstr->SetTraceID(traceWrongPathId);
    asimInstr->SetDis("XOR (bad path)");
}


void
SOFTSDV_MACRO_FEEDER_CLASS::InjectJMP(
    UINT32 cpuNum,
    const IADDR_CLASS ipVA,
    const IADDR_CLASS ipPA,
    SOFTSDV_LIVE_INSTRUCTION targetHandle,
    ASIM_MACRO_INST asimInstr)
{
    //
    // An interrupt will be signalled to set the next fetch location.
    // Just inject a NOP and add it to the linked list of instructions.
    // The instruction must be in the list for the interrupt request to
    // be raised.
    //
    static UINT8 nopOpcode[] = { 0x90 };

    asimInstr->Init(nopOpcode, ipVA.GetMacro(), sizeof(nopOpcode));
    asimInstr->SetDis("NOP <IRQ trigger>");
    asimInstr->SetTraceID(traceArtificialInstId);

    UINT64 targetVA = targetHandle->GetSdvInstr()->GetAddrVirtual();

    // Trigger an interrupt request.
    T1("\tSoftSDV: FEED_Fetch triggering event (m_uid="
       << asimInstr->GetUid() << ") -- IRQ target VA 0x"
       << fmt_x(targetVA));

    cpuState[cpuNum].PushSystemEvent(
        SDV_SYSTEM_EVENT_CLASS(asimInstr->GetUid(),
                               FEEDER_SYSTEM_EVENT_CLASS(
                                   IRQ_FEEDER_SYSTEM_EVENT,
                                   targetVA)));
}


//----------------------------------------------------------------
// Predicate register values -- these calls are defined for IPF
//    but x86 has no predicates.
//----------------------------------------------------------------

//
// Any predicate input.
//
bool
SOFTSDV_MACRO_FEEDER_CLASS::GetInputPredicateValue(
    UINT32 cpuNum,
    SOFTSDV_LIVE_INSTRUCTION instrHandle,
    INT32 regNum,
    ARCH_REGISTER reg)
{
    return false;
}


//
// Any predicate output.
//
bool
SOFTSDV_MACRO_FEEDER_CLASS::GetOutputPredicateValue(
    UINT32 cpuNum,
    SOFTSDV_LIVE_INSTRUCTION instrHandle,
    INT32 regNum,
    ARCH_REGISTER reg)
{
    return false;
}

//----------------------------------------------------------------
// Statistics
//----------------------------------------------------------------

void
SOFTSDV_MACRO_FEEDER_CLASS::DumpISAPerCPUStats(
    STATE_OUT state_out,
    UINT32 cpuNum
    )
{
}
