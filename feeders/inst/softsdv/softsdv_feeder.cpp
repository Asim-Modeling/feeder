/**************************************************************************
 * Copyright (C) 2003-2006 Intel Corporation
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
 * @brief SoftSDV instruction feeder
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/trace.h"

// ASIM public modules
#include "asim/provides/instfeeder_interface.h"
#include "asim/provides/instfeeder_implementation.h"



//----------------------------------------------------------------
// Public interface
//----------------------------------------------------------------

SOFTSDV_FEEDER_BASE_CLASS::SOFTSDV_FEEDER_BASE_CLASS(IFEEDER_BASE parentFeeder) :
    IFEEDER_BASE_CLASS("SoftSDV_Feeder", parentFeeder, IFEED_TYPE_MACRO),
    asimIO(NULL),
    nSpinLoopSamples(0),
    nSpinLoopCycles(0),
    addrTrans(14, 0x1000000000ULL),     // 16k pages, 64 GB physical offset
    warmUpClientInfoCalled(false),
    simComplete(false)
{
    if (SOFTSDV_REGISTER_VALUES)
    {
        SetCapable(IFEED_REGISTER_VALUES);
    }

    if (SOFTSDV_MEMORY_VALUES)
    {
        SetCapable(IFEED_MEMORY_VALUES);
    }
    SetCapable(IFEED_V2P_TRANSLATION);

    SetMacroType(IFEED_MACRO_SOFTSDV);
};


bool
SOFTSDV_FEEDER_BASE_CLASS::Init(
    UINT32 argc,
    char **argv,
    char **envp)
{
    ASSERTX(asimIO == NULL);
    
    //
    // Parse Asim-only arguments
    //
    char **argvOut = (char **)malloc(sizeof(char *) * (argc + 1));
    UINT32 argcOut = 0;
    for (UINT32 i = 0; i < argc; i++)
    {
        if (! strcmp(argv[i], "-group"))
        {
            ASSERT(i + 1 <= argc, "-group takes an argument");
            globalStats.workloadGroup = argv[i+1];
            i += 1;
        }
        else if (! strcmp(argv[i], "-workload"))
        {
            ASSERT(i + 1 <= argc, "-workload takes an argument");
            globalStats.workloadName = argv[i+1];
            i += 1;
        }
        else
        {
            argvOut[argcOut++] = argv[i];
        }
    }
    argvOut[argcOut] = NULL;

    asimIO = new SOFTSDV_IO_ASIM_SIDE_CLASS(argcOut, argvOut, envp,
                                            SOFTSDV_REGISTER_VALUES,
                                            SOFTSDV_MEMORY_VALUES,
                                            SOFTSDV_MONITOR_DMA_TRAFFIC);

    free(argvOut);


    //
    // Send initialization packets to SoftSDV
    //
    ASIM_SOFTSDV_REQUEST request = asimIO->AsimRequestQueue().OpenNext();
    *request = ASIM_SOFTSDV_REQUEST_CLASS(ASIM_REQUEST_INIT);
    asimIO->AsimRequestQueue().Close(request);

    InitISASpecific(argc, argv, envp);

    request = asimIO->AsimRequestQueue().OpenNext();
    *request = ASIM_SOFTSDV_REQUEST_CLASS(ASIM_REQUEST_INIT_DONE);
    asimIO->AsimRequestQueue().Close(request);

    // Signal queue write
    char c = 0;
    write(asimIO->pipeOut, &c, 1);

    // Initialize dynamically sized storage
    int nCpus = asimIO->NCpus();
    cpuState.Init(nCpus);
    cpuStats.Init(nCpus);
    threads.Init(nCpus);

    //
    // Start Asim threads, one for each SoftSDV CPU
    ///
    for (int cpu = 0; cpu < nCpus; cpu++)
    {
        T1("SoftSDV: Starting thread, cpu = " << cpu); 

        threads[cpu] = new IFEEDER_THREAD_CLASS(this, STREAM_HANDLE(cpu), 0);
    }

    //
    // Perform one translation so that addrTrans will allocate physical
    // page 0 to virtual page 0.
    //
    UINT64 pa;
    addrTrans.ITranslate(0, 0, pa);

    return true;
};

void
SOFTSDV_FEEDER_BASE_CLASS::Done(void)
{};


void
SOFTSDV_FEEDER_BASE_CLASS::ForceThreadExit(
    IFEEDER_STREAM_HANDLE stream)
{
    UINT32 cpu = STREAM_HANDLE(stream);

    //
    // End of simulation
    //
    simComplete = true;

    if (threads[cpu] != NULL)
    {
        T1("SoftSDV: Ending thread, cpu = " << cpu);
        threads[cpu]->ThreadEnd();
        threads[cpu] = NULL;
    }
};


SOFTSDV_LIVE_INSTRUCTION
SOFTSDV_FEEDER_BASE_CLASS::BufferIncomingInstruction(
    UINT32 cpu,
    ASIM_SOFTSDV_INST_INFO instr,
    UINT64 cycle)
{
    SOFTSDV_LIVE_INSTRUCTION instrHandle;

    //
    // Store the instruction in the replay buffer
    //
    instrHandle = cpuState[cpu].liveInstrs.AddInstruction(instr);

    if (! instr->GetWarmUp())
    {
        cpuStats[cpu].instrsFromSoftsdv += 1;
        if (instr->GetInterrupt())
        {
            cpuStats[cpu].nInterrupts += 1;
        }

        if (instr->GetInstructionIsTagged())
        {
            INT32 tag = instr->GetInstrTag();
            cpuStats[cpu].taggedInstrs.NoteTag(tag);
            globalStats.taggedInstrs.NoteTag(tag);
        }

        if (instr->GetKernelInstr())
        {
            cpuStats[cpu].nKernelInstrs += 1;
        }

        if (instr->GetTimerInterrupt())
        {
            cpuStats[cpu].nTimerInterrupts += 1;
        }
    }

    //
    // Does the ISA require a special sequence of instructions following
    // the new instruction?  (E.g. extended immediate on IPF)
    //
    NoteNewIncomingInstr(cpu, instr);

    //
    // For control instructions, add the target to the virtual to physical
    // translation.
    //
    if (instr->IsControl() && (instr->GetTargetPA() != 0))
    {
        if (! cpuState[cpu].iTranslate.AddEntry(instr->GetTargetVA(),
                                                instr->GetTargetPA(),
                                                instr->GetUid()))
        {
            globalStats.nITranslateEntriesAdded += 1;
        }
    }

    //
    // Add the instruction virtual to physical mapping to the translation engine
    //
    if (! cpuState[cpu].iTranslate.AddEntry(instr->GetAddrVirtual(),
                                            instr->GetAddrPhysical(),
                                            instr->GetUid()))
    {
        globalStats.nITranslateEntriesAdded += 1;
    }

    if (cpuState[cpu].iTranslate.IsEntryArtificial(instr->GetAddrVirtual()))
    {
        globalStats.nITranslateArtificialPageInstrs += 1;
    }

    //
    // Compute statistics incoming values
    //
    if (SOFTSDV_REGISTER_VALUES)
    {
        for (UINT32 i = 0; i < instr->NInputRegisters(); i++)
        {
            if (! instr->InputRegister(i).HasKnownValue())
            {
                globalStats.nUnknownRegisterValues += 1;
            }
        }
    }

    return instrHandle;
}


//
// Read the separate register values ring and store the incoming values
// in the live instruction buffer.
//
void
SOFTSDV_FEEDER_BASE_CLASS::CheckForIncomingRegValues(void)
{
    while (asimIO->RegValues().NReadSlotsAvailable() != 0)
    {
        ASIM_SOFTSDV_REG_INFO regValues;
        regValues = asimIO->RegValues().OpenNext();
        UINT32 regCpu = regValues->GetCpuNum();
        cpuState[regCpu].liveInstrs.PushRegisterValues(*regValues);
        asimIO->RegValues().Close(regValues);
        cpuStats[regCpu].nRegValueMessages += 1;
    }
}


//
// Read in up to 32 prebuffered instructions to the replay buffer.
// 32 is merely a limit to prevent it from allowing any one simulated
// CPU to get too far ahead.  We need prebuffered instructions to
// predict instruction virtual to physical address translation.
//
// Return true if at least one instruction was fetched.
//
UINT32
SOFTSDV_FEEDER_BASE_CLASS::FetchIncomingInstructions(
    UINT32 cpu,
    UINT32 fetchChunkSize,
    UINT64 cycle)
{
    ASIM_SOFTSDV_INST_INFO newInstr;

    //
    // Don't fetch if there are already enough instructions in the local
    // buffer.
    //
    if (cpuState[cpu].liveInstrs.NUnfetchedQueuedInstrs() > fetchChunkSize / 2)
    {
        return 0;
    }

    for (UINT32 i = 0; i < fetchChunkSize; i++)
    {
        CheckForIncomingRegValues();

        // Get the next instruction
        newInstr = asimIO->InstrRing(cpu).OpenNext();

        // Check again for register values in case the new instruction
        // depends on registers and we missed the registers before the
        // instruction.  Must check both places since the register slots
        // being full could block all instruction progress.
        CheckForIncomingRegValues();

        if (newInstr == NULL)
        {
            //
            // End of stream
            //
            return i;
        }

        BufferIncomingInstruction(cpu, newInstr, cycle);
        asimIO->InstrRing(cpu).Close(newInstr);
    }

    //
    // Generate a new fetch request to replace the instructions we just loaded.
    //
    ASIM_SOFTSDV_REQUEST request = asimIO->AsimRequestQueue().OpenNext();
    if (request != NULL)
    {
        *request = ASIM_SOFTSDV_REQUEST_CLASS(ASIM_REQUEST_FETCH, cpu, fetchChunkSize);
        asimIO->AsimRequestQueue().Close(request);
    }

    // Signal the queue write on the control pipe
    char c = 0;
    write(asimIO->pipeOut, &c, 1);

    return fetchChunkSize;
}


void
SOFTSDV_FEEDER_BASE_CLASS::WarmUpClientInfo(
    const WARMUP_CLIENTS clientInfo)
{
    if (warmUpClientInfoCalled)
    {
        // SoftSDV feeder doesn't permit changes in requested warm-up information
        ASSERT(warmUpClients == *clientInfo, "Warm-up requests changed.");
        return;
    }
    
    warmUpClientInfoCalled = true;
    warmUpClients = *clientInfo;

    // Tell SoftSDV
    ASIM_SOFTSDV_WARMUP_METHOD method;
    if (warmUpClients.MonitorInstrs() ||
        warmUpClients.MonitorICache() ||
        warmUpClients.MonitorDCache())
    {
        method = ASIM_SDV_WARMUP_ON;
    }
    else
    {
        method = ASIM_SDV_WARMUP_OFF;
    }

    ASIM_SOFTSDV_REQUEST request = asimIO->AsimRequestQueue().OpenNext();
    *request = ASIM_SOFTSDV_REQUEST_CLASS(ASIM_REQUEST_WARMUP, method);
    asimIO->AsimRequestQueue().Close(request);

    //
    // Now that warm-up is known, make initial fetch requests to fill the
    // incoming instruction queues.
    //
    int nCpus = asimIO->NCpus();
    for (int cpu = 0; cpu < nCpus; cpu++)
    {
        request = asimIO->AsimRequestQueue().OpenNext();
        *request = ASIM_SOFTSDV_REQUEST_CLASS(ASIM_REQUEST_FETCH, cpu, SOFTSDV_WARMUP_CHUNK_SIZE);
        asimIO->AsimRequestQueue().Close(request);
    }

    // Signal queue write
    char c = 0;
    write(asimIO->pipeOut, &c, 1);
}


bool
SOFTSDV_FEEDER_BASE_CLASS::WarmUp(
    IFEEDER_STREAM_HANDLE stream,
    WARMUP_INFO warmup)
{
    UINT32 cpu = STREAM_HANDLE(stream);

    T1("\tSoftSDV: FEED_Warmup from stream cpu=" << cpu);

    if (! warmUpClientInfoCalled)
    {
        //
        // The model must not have a warm-up manager.  SoftSDV is expecting
        // a message about warm-up before fetch can begin.
        //
        WARMUP_CLIENTS_CLASS wClients = WARMUP_CLIENTS_CLASS();
        WarmUpClientInfo(&wClients);
    }

    //
    // Start by releasing the last warm-up instruction.  It was kept active
    // in case the warm-up code probed properties of the instruction (e.g.
    // cache line value).
    //
    if (cpuState[cpu].lastWarmup != NULL)
    {
        INT32 tag = -1;
        cpuState[cpu].lastWarmup->CommitInstr(&tag);
        cpuState[cpu].lastWarmup = NULL;
    }

    SOFTSDV_LIVE_INSTRUCTION fetchHandle;

    FetchIncomingInstructions(cpu, SOFTSDV_WARMUP_CHUNK_SIZE, 0);
    fetchHandle = cpuState[cpu].liveInstrs.GetNextRealFetchHandle();

    if ((fetchHandle == NULL) || simComplete)
    {
        ForceThreadExit(stream);
        return false;
    }

    CONST_ASIM_SOFTSDV_INST_INFO sdvInstr = fetchHandle->GetSdvInstr();
    if (sdvInstr->GetWarmUp())
    {
        //
        // Instruction is warm-up
        //
        ASIM_MACRO_INST mInst = NULL;

        if (warmUpClients.MonitorICache())
        {
            warmup->NoteIFetch(sdvInstr->GetAddrVirtual(), sdvInstr->GetAddrPhysical());
        }

        if (warmUpClients.MonitorInstrs() && sdvInstr->IsControl())
        {
            mInst = warmup->InitAsimInst();

            AsimInstFromSoftsdvInst(cpu, fetchHandle, mInst, true, 0);
            mInst->SetTraceID((PTR_SIZED_UINT)fetchHandle);

            if (mInst->IsControlOp())
            {
                warmup->NoteCtrlTransfer();
            }
        }

        if (warmUpClients.MonitorDCache() && sdvInstr->IsMemRef())
        {
            NoteMemoryRefForWarmup(cpu, fetchHandle, warmup);
        }

        cpuState[cpu].lastWarmup = fetchHandle;
        return true;
    }

    // Not in warm-up mode
    cpuState[cpu].liveInstrs.SetNextFetch(fetchHandle);
    return false;
}


bool
SOFTSDV_FEEDER_BASE_CLASS::Fetch(
    IFEEDER_STREAM_HANDLE stream,
    IADDR_CLASS pc,
    ASIM_INST inst,
    UINT64 cycle)
{
    UINT32 cpu = STREAM_HANDLE(stream);

    if (! pc.GetSOF())
    {
        if (! inst->GetMacroInst()->IsRepeatOp())
        {
            ASSERT(cycle < cpuState[cpu].lastCommitCycle + 1000,
                   "1000 cycles with no macro instruction commit!");
        }

        return true;
    }

    inst->CreateNewMacroOp(pc.GetMacro());
    ASIM_MACRO_INST mInst = inst->GetMacroInst();

    //
    // If the context switch flag is set, then all the pm is asking for is for
    // the feeder to create a nop inst.  So, check for this first, and if
    // found, return the nop.
    //
    // Shouldn't this be in the software context code so each feeder doesn't
    // have to do the same thing?
    //
    if (inst->GetAsimSchedulerContextSwitch() == true)
    {
        T1("SoftSDV: FEED_Fetch: Context switch requested.  Creating a NOP to represent a context switch.");
        InjectNOP(cpu, pc, pc, mInst);
        cpuStats[cpu].nNOPsForContextSwitch += 1;

        ASSERT(cycle < cpuState[cpu].lastCommitCycle + 1000,
               "1000 cycles with no macro instruction commit!");

        return true;
    }

    return FetchMacro(stream, pc, mInst, cycle);
}


bool
SOFTSDV_FEEDER_BASE_CLASS::FetchMacro(
    IFEEDER_STREAM_HANDLE stream,
    IADDR_CLASS pc,
    ASIM_MACRO_INST mInst,
    UINT64 cycle)
{
    bool result;
    UINT32 cpu = STREAM_HANDLE(stream);

    if (! warmUpClientInfoCalled)
    {
        //
        // The model must not have a warm-up manager.  SoftSDV is expecting
        // a message about warm-up before fetch can begin.
        //
        WARMUP_CLIENTS_CLASS wClients = WARMUP_CLIENTS_CLASS();
        WarmUpClientInfo(&wClients);
    }

    T1("\tSoftSDV: FEED_Fetch from stream cpu=" << cpu << ", ip=" << pc);

    ASSERT(cpuState[cpu].CheckHWCNum(mInst->GetSWC()->GetHWCNum()),
           "Asim context scheduler moved a SoftSDV CPU");

    cpuState[cpu].lastFetchCycle = cycle;

    SOFTSDV_LIVE_INSTRUCTION fetchHandle;

    FetchIncomingInstructions(cpu, SOFTSDV_EXECUTE_CHUNK_SIZE, cycle);
    fetchHandle = cpuState[cpu].liveInstrs.GetNextRealFetchHandle();

    if ((fetchHandle == NULL) || simComplete)
    {
        ForceThreadExit(stream);

        InjectNOP(cpu, pc, pc, mInst);
        cpuStats[cpu].nNOPsAtBenchmarkEnd += 1;
        return false;
    }

    cpuStats[cpu].nFetched += 1;

    CONST_ASIM_SOFTSDV_INST_INFO newInstr = fetchHandle->GetSdvInstr();

    UINT64 newAddr = newInstr->GetAddrVirtual();

    //
    // Initialize the process information on the fist pass the feeder.
    // Once we are really running context switches will be detected on
    // commit.
    //
    if (newInstr->GetContextSwitch() && (mInst->GetSWC()->GetPid() == -1))
    {
        SW_CONTEXT swc = mInst->GetSWC();
        swc->SetPid(newInstr->GetPid());
        if (newInstr->GetProcessName()[0])
        {
            swc->SetProcessName(newInstr->GetProcessName());
        }
        else
        {
            char numAsName[64];
            sprintf(numAsName, "%d", newInstr->GetPid());
            swc->SetProcessName(numAsName);
        }
    }

    //
    // Is an interrupt request pending?  If yes, just emit bad path to
    // minimize confusion.
    //
    const SDV_SYSTEM_EVENT_CLASS *nextEvent = cpuState[cpu].GetNextSystemEvent();
    bool interruptPending = nextEvent &&
                            (nextEvent->GetEventType() == IRQ_FEEDER_SYSTEM_EVENT);

    //
    // Need a JMP to a new sequence?  Keep inserting JMP instructions until
    // Asim takes the hint.
    //
    if (pc.GetUniqueAddr() != newAddr &&
        ! cpuState[cpu].OnBadPath() &&
        ! interruptPending &&
        (newInstr->GetNewSequence() ||
         newInstr->GetInterrupt()))
    {
        T1("\t\tInject JMP for interrupt from " << pc << " to " << hex << newAddr << dec);

        InjectJMP(cpu, pc, pc, fetchHandle, mInst);
        cpuState[cpu].liveInstrs.SetNextFetch(fetchHandle);
        cpuStats[cpu].nJMPsInjected += 1;
        return true;
    }

    //
    // On bad path due to branch mispredict?  Insert NOPs.
    //
    if ((pc.GetUniqueAddr() != newAddr) ||
        cpuState[cpu].OnBadPath() ||
        interruptPending)
    {
        T1("\t\tInject bad path NOP at " << pc << ", next real op at " << hex << newAddr << dec);

        InjectBadPathInstr(cpu, pc, pc, mInst);
        cpuState[cpu].SetOnBadPath();
        cpuState[cpu].liveInstrs.SetNextFetch(fetchHandle);
        cpuStats[cpu].nBadPath += 1;
        return true;
    }

    //
    // Fetching along correct path...
    //

    //
    // First make sure the incoming instruction buffer is full.  The
    // code the fills in ASIM_INSTs may want to know the next instruction.
    //
    FetchIncomingInstructions(cpu, SOFTSDV_EXECUTE_CHUNK_SIZE, cycle);

    result = AsimInstFromSoftsdvInst(cpu, fetchHandle, mInst, false, cycle);
    mInst->SetTraceID((PTR_SIZED_UINT)fetchHandle);

    T1("\t\tFetch (Uid=" << mInst->GetUid() << ") " << hex << newAddr << " / "
       << newInstr->GetAddrPhysical() << dec
       << ":  " << mInst->GetDisassembly());
    if (mInst->IsControlOp())
    {
        T1("\t\tBranch target:  " << mInst->GetActualTarget());
    }

    if (SOFTSDV_REGISTER_VALUES && TRACING(1))
    {
        for (UINT32 i = 0; i < newInstr->NInputRegisters(); i++)
        {
            T1("\t\tInput reg:  " << newInstr->InputRegister(i));
        }

        for (UINT32 i = 0; i < newInstr->NOutputRegisters(); i++)
        {
            T1("\t\tOutput reg: " << newInstr->OutputRegister(i));
        }
    }

    return result;
}


void
SOFTSDV_FEEDER_BASE_CLASS::Issue(
    ASIM_INST inst)
{
    if (inst->HadFault() || ! inst->IsTrueEndOfMacro()) return;

    UINT32 cpu = STREAM_HANDLE(inst->GetSWC()->GetFeederStreamHandle());
    cpuStats[cpu].nIssued += 1;
};


void
SOFTSDV_FEEDER_BASE_CLASS::Commit(
    ASIM_INST inst)
{
    if (inst->HadFault() || ! inst->IsTrueEndOfMacro()) return;

    UINT32 cpu = STREAM_HANDLE(inst->GetSWC()->GetFeederStreamHandle());

    UINT32 totalCommit = 0;
    
    ASIM_MACRO_INST mInst = inst->GetMacroInst();

    if (mInst->GetTraceID() == traceWrongPathId)
    {
        //
        // Committing bad path instruction.  Shouldn't happen!
        //
        ASIMERROR("SOFTSDV_FEEDER::Commit - Attempt to commit wrong path instruction\n"
                  << "   PC " << mInst->GetVirtualPC()
                  << "  Uid " << mInst->GetUid() << endl);
        return;
    }
    else if (mInst->GetTraceID() == traceArtificialInstId)
    {
        //
        // Committing an artificial instruction.  Nothing to do.
        //
        cpuStats[cpu].nArtificialCommitted += 1;
        totalCommit += 1;
    }
    else
    {
        SOFTSDV_LIVE_INSTRUCTION instrHandle = SdvHandleFromMacroInstr(mInst);
        CONST_ASIM_SOFTSDV_INST_INFO sdvInstr = instrHandle->GetSdvInstr();

        SW_CONTEXT swc = inst->GetSWC();

        // Record approx. cycle.  Used only for detecting likely errors.
        cpuState[cpu].lastCommitCycle = cpuState[cpu].lastFetchCycle;

        //
        // Note timer interrupts
        //
        if (sdvInstr->GetTimerInterrupt())
        {
            swc->TimerInterruptEnter();
            cpuState[cpu].SetInTimerInterrupt();
        }
        else if (cpuState[cpu].InTimerInterrupt() && sdvInstr->GetReturnFromInterrupt())
        {
            swc->TimerInterruptExit();
            cpuState[cpu].ClearInTimerInterrupt();
        }

        //
        // Invalidate the artificial translation for a JMP injected as the
        // vector to an I-stream TLB miss.  The JMP was injected at the
        // right VA but an aritifical virtual to physical translation may
        // have been added by the timing to handle the JMP.
        //
        if (sdvInstr->GetInterrupt())
        {
            UINT64 va = mInst->GetVirtualPC().GetUniqueAddr();
            if (cpuState[cpu].iTranslate.IsEntryArtificial(va))
            {
                T1("\tSoftSDV: FEED_Commit (Dropping artificial ITranslate from VA "
                   << mInst->GetVirtualPC() << ")");

                cpuState[cpu].iTranslate.DropTranslation(va);
            }
        }

        //
        // Detect context switches in the simulated OS on commit.
        //
        if (sdvInstr->GetContextSwitch())
        {
            cpuStats[cpu].nContextSwitches += 1;

            swc->SetPid(sdvInstr->GetPid());
            if (sdvInstr->GetProcessName()[0])
            {
                swc->SetProcessName(sdvInstr->GetProcessName());
            }
            else
            {
                char numAsName[64];
                sprintf(numAsName, "%d", sdvInstr->GetPid());
                swc->SetProcessName(numAsName);
            }

            cpuState[cpu].iTranslate.DropOldUserSpaceTranslations(sdvInstr->GetUid());
        }

        //
        // Keep track of whether we are in user space or in the kernel
        //
        if (sdvInstr->GetKernelInstr())
        {
            cpuState[cpu].SetInKernel();
        }
        else
        {
            cpuState[cpu].ClearInKernel();
        }

        INT32 tag = -1;
        UINT64 nCommitted = instrHandle->CommitInstr(&tag);
        cpuStats[cpu].nImplicitCommitted += nCommitted;
        cpuStats[cpu].nCommitted += 1;
        totalCommit += (1 + nCommitted);

        //
        // Spin locks may be tagged with values:
        //      125 - Entry loop
        //      126 - Loop trip
        //      127 - Exit loop
        //
        if (tag == 126)
        {
            UINT64 curCycle = cpuState[cpu].lastFetchCycle;
            //
            // Try to find the average spin loop length.  Assume it
            // must be shorter than 100 cycles.
            //
            if (curCycle - cpuState[cpu].lastSpinCycle < 100)
            {
                nSpinLoopSamples += 1;
                nSpinLoopCycles += (curCycle - cpuState[cpu].lastSpinCycle);
            }
            cpuStats[cpu].nSpinLoopTrips += 1;
            cpuState[cpu].lastSpinCycle = curCycle;
        }
    }

    T1("\tSoftSDV: FEED_Commit (Uid=" << mInst->GetUid() << ") "
       << mInst->GetVirtualPC() << ":  "
       << mInst->GetDisassembly());
}


void
SOFTSDV_FEEDER_BASE_CLASS::Kill(
    ASIM_INST inst,
    bool fetchNext,
    bool killMe)
{
    //
    // Parameters are silly.  Exactly one bool must be true.
    //
    ASSERTX((fetchNext == true) ^ (killMe == true));

    // Ignore kills for all but the first instruction in a macro sequence
    if (killMe && ! inst->IsStartOfMacro())
    {
        killMe = false;
        fetchNext = true;

        //return;
    }

    ASIM_MACRO_INST mInst = inst->GetMacroInst();

    ASSERTX(mInst);

    UINT32 cpu = STREAM_HANDLE(inst->GetSWC()->GetFeederStreamHandle());

    cpuStats[cpu].nKilled += 1;

    if (mInst->GetTraceID() != traceWrongPathId)
    {
        T1("\tSoftSDV: FEED_Kill Me=" << (killMe ? "Y" : "N")
           << " (Uid=" << mInst->GetUid() << ") "
           << mInst->GetVirtualPC() << "):  "
           << mInst->GetDisassembly());

        if (mInst->GetTraceID() != traceArtificialInstId)
        {
            SOFTSDV_LIVE_INSTRUCTION instrHandle = SdvHandleFromMacroInstr(mInst);
            UINT64 nKilled = cpuState[cpu].liveInstrs.SetNextFetch(instrHandle);
            if (fetchNext)
            {
                cpuState[cpu].liveInstrs.GetNextFetchHandle();
                nKilled -= 1;
            }

            cpuStats[cpu].nKilledInstrs += nKilled;
        }

        cpuState[cpu].ClearOnBadPath();

        if (killMe)
        {
            //
            // Kill any system events pending for this instruction
            //
            FEEDER_SYSTEM_EVENT_TYPES event;
            while ((event = GetNextSystemEventType(inst)) != NULL_FEEDER_SYSTEM_EVENT)
            {
                HandleSystemEvent(inst, event);
            }
        }

        //
        // Kill all system events pending for future instructions.
        //
        const SDV_SYSTEM_EVENT_CLASS *nextEvent;
        nextEvent = cpuState[cpu].GetNextSystemEvent();
        while ((nextEvent != NULL) &&
               (nextEvent->GetMacroUid() > mInst->GetUid()))
        {
            cpuState[cpu].PopSystemEvent();
            nextEvent = cpuState[cpu].GetNextSystemEvent();
        }
    }
}


bool
SOFTSDV_FEEDER_BASE_CLASS::ITranslate(
    IFEEDER_STREAM_HANDLE stream,
    UINT32 hwcNum,
    UINT64 va,
    UINT64& pa)
{
    MEMORY_VIRTUAL_REFERENCE_CLASS r(va, 1);
    MEMORY_VIRTUAL_REFERENCE_CLASS rNext(0, 0);

    return ITranslate(stream, r, pa, NULL, rNext);
};


bool
SOFTSDV_FEEDER_BASE_CLASS::ITranslate(
    IFEEDER_STREAM_HANDLE stream,
    const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
    UINT64& pa,
    PAGE_TABLE_INFO pt_info,
    MEMORY_VIRTUAL_REFERENCE_CLASS& vNextRegion)
{
    if (asimIO->OSMode())
    {
        //
        // In OS mode use the SoftSDV translations.  Asim is fetching ahead.
        // Look in a cache of recent translations to guess at the new one.
        // The cache actually includes a number of instructions received
        // from SoftSDV but not yet fetched, so the translation ought to be
        // quite accurate.
        //
        UINT32 cpu = STREAM_HANDLE(stream);

        UINT32 lookupSteps = cpuState[cpu].iTranslate.Lookup(vRegion.GetVA(), pa);

        if (lookupSteps == 0)
        {
            //
            // Make sure we've prefetched incoming instructions far enough ahead
            // to know future translations.
            //
            FetchIncomingInstructions(cpu, SOFTSDV_EXECUTE_CHUNK_SIZE, cpuState[cpu].lastFetchCycle);
            lookupSteps = cpuState[cpu].iTranslate.Lookup(vRegion.GetVA(), pa);
        }

        if (lookupSteps != 0)
        {
            T1("\tSoftSDV: Itranslate va " << fmt_x(vRegion.GetVA()) << " to pa: " <<  fmt_x(pa));

            globalStats.nITranslateHits += 1;
            globalStats.sumITranslateSteps += lookupSteps;
            if (lookupSteps > globalStats.maxITranslateSteps)
            {
                globalStats.maxITranslateSteps = lookupSteps;
            }

            vNextRegion = MEMORY_VIRTUAL_REFERENCE_CLASS(0, 0);
            return true;
        }
        else
        {
            //
            // No translation.  Fall back to our own allocation.
            //
            globalStats.nITranslateMisses += 1;
            bool pageHit = addrTrans.ITranslate(STREAM_HANDLE(stream), vRegion.GetVA(), pa);
            if (! pageHit)
            {
                globalStats.nITranslateMissPages += 1;
            }

            //
            // Add the translation to the SoftSDV table too, so that it gets
            // the same answer.
            //
            if (! cpuState[cpu].iTranslate.AddEntry(vRegion.GetVA(), pa))
            {
                globalStats.nITranslateEntriesAdded += 1;
            }

            vNextRegion = MEMORY_VIRTUAL_REFERENCE_CLASS(0, 0);
            return pageHit;
        }
    }
    else
    {
        vNextRegion = MEMORY_VIRTUAL_REFERENCE_CLASS(0, 0);
        return addrTrans.ITranslate(STREAM_HANDLE(stream), vRegion.GetVA(), pa);
    }
};


bool
SOFTSDV_FEEDER_BASE_CLASS::DTranslate(
    ASIM_INST inst,
    UINT64 va,
    UINT64& pa)
{
    MEMORY_VIRTUAL_REFERENCE_CLASS r(va, 1);
    MEMORY_VIRTUAL_REFERENCE_CLASS rNext(0, 0);

    return DTranslate(inst, r, pa, NULL, rNext);
};


bool
SOFTSDV_FEEDER_BASE_CLASS::DTranslate(
    ASIM_INST inst,
    const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
    UINT64& pa,
    PAGE_TABLE_INFO pt_info,
    MEMORY_VIRTUAL_REFERENCE_CLASS& vNextRegion)
{
    ASSERTX(&vRegion != &vNextRegion);
    ASIM_MACRO_INST mInst = inst->GetMacroInst();

    if (asimIO->OSMode() &&
        (! mInst->IsInjectedInst()) &&
        (mInst->GetTraceID() != traceWrongPathId) &&
        (mInst->GetTraceID() != traceArtificialInstId))
    {
        //
        // In OS mode just use the translation provided by SoftSDV
        //
        UINT32 cpu = STREAM_HANDLE(inst->GetSWC()->GetFeederStreamHandle());

        SOFTSDV_LIVE_INSTRUCTION instrHandle = SdvHandleFromMacroInstr(mInst);

        pa = 0;
        bool foundTranslation = false;

        for (UINT32 i = 0; i < instrHandle->NLoads(); i++)
        {
            if (instrHandle->GetLoad(i)->ContainsVA(vRegion))
            {
                pa = instrHandle->GetLoad(i)->PTranslate(vRegion, vNextRegion);
                foundTranslation = true;
                break;
            }
        }

        if (! foundTranslation)
        {
            for (UINT32 i = 0; i < instrHandle->NStores(); i++)
            {
                if (instrHandle->GetStore(i)->ContainsVA(vRegion))
                {
                    pa = instrHandle->GetStore(i)->PTranslate(vRegion, vNextRegion);
                    foundTranslation = true;
                    break;
                }
            }
        }

        if (foundTranslation && ((pa != 0) || (mInst->IsTaccess())))
        {
            globalStats.nDTranslateExpectedVA += 1;

            if (SOFTSDV_NO_DATA_SHARING && (pa != 0))
            {
                //
                // Disable data sharing by merging the CPU number as a
                // high bit component of the physical address.
                //
                pa = pa ^ ((UINT64)cpu << 20);
            }

            if (mInst->IsTaccess())
            {
                //
                // SoftSDV stores tags, hash entries, etc. in the PA for
                // these functions.  Just return page 0.  Asim should probably
                // be smarter about these instructions.
                //
                pa = 0;
            }

            //
            // Probably about to take an interrupt due to a missing TLB entry.
            // Translate to something on page 0 to keep Asim happy.  Too bad
            // the page size isn't global.  Guess 4k.
            //
            if (pa == 0)
            {
                pa = vRegion.GetVA() & 4095;
                vNextRegion = MEMORY_VIRTUAL_REFERENCE_CLASS(0, 0);
            }

            T1("\tSoftSDV: FEED_DTranslate (Uid=" << mInst->GetUid() << ") "
               << "VA 0x" << fmt_x(vRegion.GetVA())
               << " -> PA 0x" << fmt_x(pa));
        }
        else
        {
            //
            // Virtual address doesn't match the instruction's virtual address.
            // The model can do this for a variety of reasons, usually when
            // the instruction will be replayed later with the correct virtual
            // address.  Make up a mapping.
            //
            if (instrHandle->GetSdvInstr()->IsMemRef())
            {
                globalStats.nDTranslateUnexpectedVA += 1;
            }
            vNextRegion = MEMORY_VIRTUAL_REFERENCE_CLASS(0, 0);
            return addrTrans.DTranslate(inst, vRegion.GetVA(), pa);
        }

        return true;
    }
    else
    {
        vNextRegion = MEMORY_VIRTUAL_REFERENCE_CLASS(0, 0);
        return addrTrans.DTranslate(inst, vRegion.GetVA(), pa);
    }
};


//----------------------------------------------------------------
// Register values
//----------------------------------------------------------------

//
// Return a specific input register value.
// Logical (not physical) register numbering.
//
// ***** This function does nothing for x86 since it only looks for *****
// ***** register values in the instruction, not the separate       *****
// ***** register value class.  Obviously it could be fixed.        *****
//
bool
SOFTSDV_FEEDER_BASE_CLASS::GetInputRegisterValue(
    ASIM_INST inst,
    ARCH_REGISTER_TYPE rType,
    INT32 regNum,
    ARCH_REGISTER reg)
{
    ASSERTX(SOFTSDV_REGISTER_VALUES);

    UINT32 cpu = STREAM_HANDLE(inst->GetSWC()->GetFeederStreamHandle());

    ASIM_MACRO_INST mInst = inst->GetMacroInst();

    if (rType == REG_TYPE_PREDICATE)
    {
        return GetInputPredicateValue(cpu, SdvHandleFromMacroInstr(mInst),
                                      regNum, reg);
    }

    CONST_ASIM_SOFTSDV_INST_INFO sdvInstr;
    sdvInstr = SdvHandleFromMacroInstr(mInst)->GetSdvInstr();

    for (unsigned int i = 0; i < sdvInstr->NInputRegisters(); i++)
    {
        if ((sdvInstr->InputRegister(i).GetType() == rType) &&
            (sdvInstr->InputRegister(i).GetNum() == regNum))
        {
            *reg = sdvInstr->InputRegister(i);
            return true;
        }
    }

    *reg = ARCH_REGISTER_CLASS(rType, regNum);
    return false;
}


//
// Enumerate all input register values.  Logical register numbering.
//
bool
SOFTSDV_FEEDER_BASE_CLASS::GetInputRegVal(
    ASIM_INST inst,
    UINT32 slot,
    ARCH_REGISTER reg)
{
    ASSERTX(SOFTSDV_REGISTER_VALUES);

    ASIM_MACRO_INST mInst = inst->GetMacroInst();

    if ((mInst->GetTraceID() == traceWrongPathId) ||
        (mInst->GetTraceID() == traceArtificialInstId))
    {
        return false;
    }

    UINT32 cpu = STREAM_HANDLE(inst->GetSWC()->GetFeederStreamHandle());

    SOFTSDV_LIVE_INSTRUCTION instrHandle = SdvHandleFromMacroInstr(mInst);

    //
    // First test -- does the instruction have input registers in its
    // data structure?  This is how IPF passes values.
    //
    CONST_ASIM_SOFTSDV_INST_INFO sdvInstr = instrHandle->GetSdvInstr();
    if ((sdvInstr != NULL) && (slot < sdvInstr->NInputRegisters()))
    {
        // Value was passed as part of the instruction
        *reg = sdvInstr->InputRegister(slot);
        return true;
    }

    //
    // Second test -- is there an input register state passed along with
    // the instruction?
    //
    const ASIM_SOFTSDV_REG_INFO_CLASS* rInfo;
    rInfo = instrHandle->GetInputRegValues();

    if ((rInfo != NULL) && (slot < rInfo->NRegisters()))
    {
        *reg = *rInfo->GetRegister(slot);
        return true;
    }

    return false;
}


//
// Return a specific output register value.  Logical register numbering.
//
// ***** This function does nothing for x86 since the x86 SoftSDV   *****
// ***** stub only passes input register values.                    *****
//
bool
SOFTSDV_FEEDER_BASE_CLASS::GetOutputRegisterValue(
    ASIM_INST inst,
    ARCH_REGISTER_TYPE rType,
    INT32 regNum,
    ARCH_REGISTER reg)
{
    ASSERTX(SOFTSDV_REGISTER_VALUES);

    ASIM_MACRO_INST mInst = inst->GetMacroInst();

    UINT32 cpu = STREAM_HANDLE(inst->GetSWC()->GetFeederStreamHandle());

    if (rType == REG_TYPE_PREDICATE)
    {
        return GetOutputPredicateValue(cpu, SdvHandleFromMacroInstr(mInst),
                                       regNum, reg);
    }

    CONST_ASIM_SOFTSDV_INST_INFO sdvInstr;
    sdvInstr = SdvHandleFromMacroInstr(mInst)->GetSdvInstr();

    for (unsigned int i = 0; i < sdvInstr->NOutputRegisters(); i++)
    {
        if ((sdvInstr->OutputRegister(i).GetType() == rType) &&
            (sdvInstr->OutputRegister(i).GetNum() == regNum))
        {
            *reg = sdvInstr->OutputRegister(i);
            return true;
        }
    }

    *reg = ARCH_REGISTER_CLASS(rType, regNum);
    return false;
}


//
// Enumerate all output register values.  Logical register numbering.
//
// ***** This function does nothing for x86 since the x86 SoftSDV   *****
// ***** stub only passes input register values.                    *****
//
bool
SOFTSDV_FEEDER_BASE_CLASS::GetOutputRegVal(
    ASIM_INST inst,
    UINT32 slot,
    ARCH_REGISTER reg)
{
    ASSERTX(SOFTSDV_REGISTER_VALUES);

    ASIM_MACRO_INST mInst = inst->GetMacroInst();

    UINT32 cpu = STREAM_HANDLE(inst->GetSWC()->GetFeederStreamHandle());
    CONST_ASIM_SOFTSDV_INST_INFO sdvInstr;
    sdvInstr = SdvHandleFromMacroInstr(mInst)->GetSdvInstr();

    if (slot >= sdvInstr->NOutputRegisters())
    {
        return false;
    }

    *reg = sdvInstr->OutputRegister(slot);

    return true;
}

//
// ReadMemory --
//    Get the value read from memory by the instruction.
//
bool
SOFTSDV_FEEDER_BASE_CLASS::ReadMemory(
    ASIM_INST inst,
    void *buffer,
    UINT32 size,
    UINT64 pAddr)
{
    ASSERTX(SOFTSDV_MEMORY_VALUES);

    ASIM_MACRO_INST mInst = inst->GetMacroInst();

    T1("\tSoftSDV: FEED_ReadMemory (Uid=" << mInst->GetUid() << ") "
       << "PA 0x" << fmt_x(pAddr) << "/");

    UINT32 cpu = STREAM_HANDLE(inst->GetSWC()->GetFeederStreamHandle());
    SOFTSDV_LIVE_INSTRUCTION instrHandle = SdvHandleFromMacroInstr(mInst);

    if ((mInst->GetTraceID() == traceWrongPathId) ||
        (mInst->GetTraceID() == traceArtificialInstId))
    {
        T1("\t\t-- failed (bad path)");
        cpuStats[cpu].nFailedMemReads += 1;
        return false;
    }

    for (UINT32 i = 0; i < instrHandle->NLoads(); i++)
    {
        const ASIM_SOFTSDV_MEM_ACCESS_CLASS *ld = instrHandle->GetLoad(i);
        if (ld->HasValue())
        {
            MEMORY_VIRTUAL_REFERENCE_CLASS vRef = ld->VTranslate(pAddr, size);
            if (vRef.GetNBytes())
            {
                unsigned char *v = (unsigned char *)ld->GetValue();
                // address may be in the middle of the region
                v += (vRef.GetVA() - ld->GetVA());
                memcpy(buffer, v, size);

                cpuStats[cpu].nSuccessfulMemReads += 1;

                T1("\t\t-- ok");
                return true;
            }
        }
    }

    //
    // Perhaps the reference crosses a 2k boundary and was split artificially
    // into multiple pages.  Try reading one byte at a time.
    //
    if (size > 1)
    {
        T1("\t\t-- failed, trying 1 byte reads");
        char *b = (char *)buffer;
        for (UINT32 i = 0; i < size; i++)
        {
            if (! ReadMemory(inst, &b[i], 1, pAddr + i))
            {
                return false;
            }
        }

        return true;
    }

    T1("\t\t-- failed");
    cpuStats[cpu].nFailedMemReads += 1;
    return false;
}


//
// ReadIOPort --
//     Read data from an I/O port.
//
bool
SOFTSDV_FEEDER_BASE_CLASS::ReadIOPort(
    ASIM_INST inst,
    void *buffer,
    UINT32 size,
    UINT32 port)
{
    ASSERTX(SOFTSDV_MEMORY_VALUES);

    ASIM_MACRO_INST mInst = inst->GetMacroInst();

    T1("\tSoftSDV: FEED_ReadIOPort (Uid=" << mInst->GetUid() << ") "
       << "Port " << fmt_x(port) << "/" << size);

    UINT32 cpu = STREAM_HANDLE(inst->GetSWC()->GetFeederStreamHandle());
    CONST_ASIM_SOFTSDV_INST_INFO sdvInstr;
    sdvInstr = SdvHandleFromMacroInstr(mInst)->GetSdvInstr();

    ASSERTX(port == sdvInstr->GetIOPort());
    ASSERTX(size == sdvInstr->GetIOSize());

    UINT64 data = sdvInstr->GetIOData();

    switch (size)
    {
      case 1:
        *(UINT8*)buffer = data;
        break;
      case 2:
        *(UINT16*)buffer = data;
        break;
      case 4:
        *(UINT32*)buffer = data;
        break;
      case 8:
        *(UINT64*)buffer = data;
        break;
      default:
        ASIMERROR("Illegal I/O size");
        break;
    }

    return true;
}


//----------------------------------------------------------------
// System events
//----------------------------------------------------------------

FEEDER_SYSTEM_EVENT_CLASS
SOFTSDV_FEEDER_BASE_CLASS::HandleSystemEvent(
    ASIM_INST inst,
    FEEDER_SYSTEM_EVENT_TYPES type)
{
    ASIM_MACRO_INST mInst = inst->GetMacroInst();
    UINT32 cpu = STREAM_HANDLE(inst->GetSWC()->GetFeederStreamHandle());

    const SDV_SYSTEM_EVENT_CLASS *nextEvent = cpuState[cpu].GetNextSystemEvent();

    ASSERTX((nextEvent->GetEventType() == type) &&
            (nextEvent->GetMacroUid() == mInst->GetUid()));

    T1("\tSoftSDV: FEED_HandleSystemEvent (m_uid=" << mInst->GetUid() << ") -- " << type);

    FEEDER_SYSTEM_EVENT_CLASS event = nextEvent->GetEvent();
    cpuState[cpu].PopSystemEvent();
    cpuStats[cpu].nSysEvents += 1;

    return event;
}


FEEDER_SYSTEM_EVENT_TYPES
SOFTSDV_FEEDER_BASE_CLASS::GetNextSystemEventType(ASIM_INST inst)
{
    ASIM_MACRO_INST mInst = inst->GetMacroInst();
    UINT32 cpu = STREAM_HANDLE(inst->GetSWC()->GetFeederStreamHandle());

    const SDV_SYSTEM_EVENT_CLASS *nextEvent = cpuState[cpu].GetNextSystemEvent();
    FEEDER_SYSTEM_EVENT_TYPES event = NULL_FEEDER_SYSTEM_EVENT;

    if (nextEvent && (nextEvent->GetMacroUid() == mInst->GetUid()))
    {
        event = nextEvent->GetEventType();
    }

    T1("\tSoftSDV: FEED_GetNextSystemEventType (m_uid=" << mInst->GetUid() << ") -- " << event);

    return event;
}


//----------------------------------------------------------------
// Statistics
//----------------------------------------------------------------

void 
SOFTSDV_FEEDER_BASE_CLASS::DumpStats(STATE_OUT state_out)
{
    int nCpus = asimIO->NCpus();
    char name[128];

    for(int i = 0; i < nCpus; i++)
    {
        sprintf(name, "CPU_%d", cpuState[i].HWCNum());
        state_out->AddCompound("info", name);

        state_out->AddScalar("uint",
                             "softsdv_cpu",
                             "SoftSDV CPU number",
                             i);
        state_out->AddScalar("uint",
                             "instrs_fetched",
                             "Total instructions fetched (including replay and bad path)",
                             cpuStats[i].nFetched);
        state_out->AddScalar("uint",
                             "instrs_issued",
                             "Total instructions issued (including replay and bad path)",
                             cpuStats[i].nIssued);
        state_out->AddScalar("uint",
                             "instrs_killed",
                             "Total instructions killed",
                             cpuStats[i].nKilledInstrs);
        state_out->AddScalar("uint",
                             "instrs_committed",
                             "Number of calls to commit method",
                             cpuStats[i].nCommitted);
        state_out->AddScalar("uint",
                             "instrs_implicitly_committed",
                             "Number of insts that were implicitly committed",
                             cpuStats[i].nImplicitCommitted);
        state_out->AddScalar("uint",
                             "instrs_artificial_committed",
                             "Number of artificial insts that were committed",
                             cpuStats[i].nArtificialCommitted);

        state_out->AddScalar("uint",
                             "instrs_from_softsdv",
                             "Instructions arriving from SoftSDV",
                             cpuStats[i].instrsFromSoftsdv);
        state_out->AddScalar("uint",
                             "user_instrs_from_softsdv",
                             "User-mode instructions arriving from SoftSDV",
                             cpuStats[i].instrsFromSoftsdv - cpuStats[i].nKernelInstrs);
        state_out->AddScalar("uint",
                             "kernel_instrs_from_softsdv",
                             "Kernel-mode instructions arriving from SoftSDV",
                             cpuStats[i].nKernelInstrs);
        state_out->AddScalar("uint",
                             "reg_value_messages",
                             "Number of register value messages from SoftSDV",
                             cpuStats[i].nRegValueMessages);
        state_out->AddScalar("uint",
                             "interrupts",
                             "Number of interrupts taken in SoftSDV simulation",
                             cpuStats[i].nInterrupts);
        state_out->AddScalar("uint",
                             "sysevents",
                             "Number of system events injected from SoftSDV",
                             cpuStats[i].nSysEvents);
        state_out->AddScalar("uint",
                             "timer_interrupts",
                             "Number of timer interrupts handled in SoftSDV simulation",
                             cpuStats[i].nTimerInterrupts);
        state_out->AddScalar("uint",
                             "idle_pause_instrs",
                             "Number of PAUSE instructions tagged in the kernel's idle loop",
                             cpuStats[i].nIdlePauseInstrs);
        state_out->AddScalar("uint",
                             "spin_loop_trips",
                             "Number of trips through tagged spin lock loops",
                             cpuStats[i].nSpinLoopTrips);
        state_out->AddScalar("uint",
                             "context_switches",
                             "Number of OS context switches in SoftSDV simulation",
                             cpuStats[i].nContextSwitches);

        state_out->AddScalar("uint",
                             "instrs_badpath",
                             "Total bad path instructions fetched",
                             cpuStats[i].nBadPath);
        state_out->AddScalar("uint",
                             "instrs_nop_context_switch",
                             "Total NOPs injected for Asim context switches",
                             cpuStats[i].nNOPsForContextSwitch);
        state_out->AddScalar("uint",
                             "instrs_nop_benchmark_end",
                             "Total NOPs injected after the benchmark ended",
                             cpuStats[i].nNOPsAtBenchmarkEnd);
        state_out->AddScalar("uint",
                             "instrs_jmp",
                             "Total JMPs injected to redirect flow",
                             cpuStats[i].nJMPsInjected);
        state_out->AddScalar("uint",
                             "kill_calls",
                             "Number of calls to kill method",
                             cpuStats[i].nKilled);

        state_out->AddScalar("uint",
                             "dtranslate_missing_pa",
                             "Number of memory instructions from SoftSDV missing a physical address",
                             cpuStats[i].nDTranslateUnavailable);
        state_out->AddScalar("uint",
                             "dtranslate_multiple_pages",
                             "Number of memory references touching multiple pages in SoftSDV",
                             cpuStats[i].nDTranslateMultipage);

        state_out->AddScalar("uint",
                             "mem_read_value_ok",
                             "Number of successful attempts to read a memory value",
                             cpuStats[i].nSuccessfulMemReads);
        state_out->AddScalar("uint",
                             "mem_read_value_failed",
                             "Number of failed attempts to read a memory value",
                             cpuStats[i].nFailedMemReads);

        DumpISAPerCPUStats(state_out, i);
        cpuStats[i].taggedInstrs.DumpStats(state_out, "instrs_tagged",
                                           "Count of marked instructions seen from feeder");

        state_out->CloseCompound();
    }

    state_out->AddScalar("info",
                         "group",
                         "Workload group",
                         globalStats.workloadGroup.c_str());
    state_out->AddScalar("info",
                         "workload",
                         "Workload name",
                         globalStats.workloadName.c_str());
    state_out->AddScalar("uint",
                         "dtranslate_expected_va",
                         "Number of calls to DTranslate with a VA matching the instruction",
                         globalStats.nDTranslateExpectedVA);
    state_out->AddScalar("uint",
                         "dtranslate_unexpected_va",
                         "Number of calls to DTranslate with a VA not matching the instruction",
                         globalStats.nDTranslateUnexpectedVA);
    state_out->AddScalar("uint",
                         "itranslate_hits",
                         "Number of hits in local ITranslation engine",
                         globalStats.nITranslateHits);
    state_out->AddScalar("uint",
                         "itranslate_misses",
                         "Number of misses in local ITranslation engine",
                         globalStats.nITranslateMisses);
    state_out->AddScalar("uint",
                         "itranslate_miss_pages",
                         "Number of pages allocated due to misses in local ITranslation engine",
                         globalStats.nITranslateMissPages);
    state_out->AddScalar("uint",
                         "itranslate_miss_page_instrs",
                         "Number of instructions received from SoftSDV on pages with artificial virtual to physical translation due to ITranslation misses",
                         globalStats.nITranslateArtificialPageInstrs);
    state_out->AddScalar("uint",
                         "itranslate_new_entries",
                         "Number of new entries added to local ITranslation engine",
                         globalStats.nITranslateEntriesAdded);
    state_out->AddScalar("double",
                         "itranslate_mean_steps",
                         "Mean linked list traversal depth resulting in a successful lookup",
                         (double) globalStats.sumITranslateSteps / (double) globalStats.nITranslateHits);
    state_out->AddScalar("uint",
                         "itranslate_max_steps",
                         "Maximum linked list traversal depth resulting in a successful lookup",
                         globalStats.maxITranslateSteps);
    state_out->AddScalar("uint",
                         "unknown_register_values",
                         "Number of unknown input register values from SoftSDV",
                         globalStats.nUnknownRegisterValues);
    state_out->AddScalar("double",
                         "spin_loop_length",
                         "Mean length of tagged spin loops",
                         nSpinLoopSamples == 0 ?
                             0.0 :
                             (double) nSpinLoopCycles / (double) nSpinLoopSamples);

    DumpISAGlobalStats(state_out);
    globalStats.taggedInstrs.DumpStats(state_out, "instrs_tagged_total",
                                       "Count of marked instructions seen from feeder across all CPUs");
}
