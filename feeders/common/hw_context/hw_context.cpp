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
 */


/**
 * @file
 * @author Judy Hall
 * @brief HW_CONTEXT_CLASS
 *
 */

#include <sstream>

// Includes
#include "asim/trace.h"
#include "asim/mesg.h"
#include "asim/ioformat.h"
#include "asim/state.h"
#include "asim/cmd.h"
//#include "asim/provides/isa.h"
//#include "asim/provides/software_context.h"

#include "asim/atomic.h"

#include "asim/provides/hardware_context.h"


UID_GEN32 HW_CONTEXT_CLASS::uniqueHwCxtId = 0;
// what is this for?  It doesn't appear to be used.  Eric
//HW_CONTEXT HW_CONTEXT_CLASS::allHWCs[maxHWCs];



/* ----------------------- HW_CONTEXT_CLASS -------------------*/

// Constructor

HW_CONTEXT_CLASS::HW_CONTEXT_CLASS(
    ASIM_REGISTRY reg,
    const UINT32 cpu,
    const UINT32 tpu,
    const INT32 priority
    )
    : uid(uniqueHwCxtId++),
      cpu(cpu), 
      tpu(tpu),
      runningSWC (ASIM_SWC_NONE),
      lastCommitCycle(0),
      myPriority(priority),
      processHistory(PROCESS_HISTORY_BUCKETS, 6, 1, true, 1, false, true),
      curProcessIdx(-1),
      maxProcessIdx(-1),
      pid(0),
      nTimerInterrupts(0),
      timerInterruptCycles(0),
      timerInterruptStartCycle(0),
      inTimerInterrupt(false),
      userSpinLoopTrips(0),
      userSpinLoopCycles(0),
      kernelSpinLoopTrips(0),
      kernelSpinLoopCycles(0),
      idleSpinLoopTrips(0),
      idleSpinLoopCycles(0),
      spinLoopStartCycle(0),
      instrsSinceLastSpinTag(0),
      inSpinLoop(false),
      hwcCacheOwnerId(cpu)
{
    ASSERTX(uid < maxHWCs);
    ASSERTX(PROCESS_HISTORY_BUCKETS > 0);
//    allHWCs[uid] = this;

    ostringstream hwc_name;

    static const char *colNames[7] = {
        "pid",
        "start_cycle",
        "instrs_fetched",
        "instrs_issued",
        "instrs_killed",
        "instrs_committed",
        "LAST"
    };
    processHistory.ColNames(colNames);
    processHistory.RowNamesInit();

    //this defaults to true unless explicitly turned off
    watchDogTimerOn=true;

    hwc_name.str("");   // Clear
    hwc_name << "HWC_" << tpu << "_PROCESS_HISTORY";
    reg->RegisterState(&processHistory,
                       hwc_name.str().c_str(),
                       "Simulated process history");

    hwc_name.str("");   // Clear
    hwc_name << "HWC_" << tpu << "_n_timer_interrupts";
    reg->RegisterState(&nTimerInterrupts, hwc_name.str().c_str(),
                       "Number of timer interrupts handled");

    hwc_name.str("");   // Clear
    hwc_name << "HWC_" << tpu << "_timer_interrupt_cycles";
    reg->RegisterState(&timerInterruptCycles, hwc_name.str().c_str(),
                       "Cycles spent in timer interrupts");

    hwc_name.str("");   // Clear
    hwc_name << "HWC_" << tpu << "_user_spin_loop_trips";
    reg->RegisterState(&userSpinLoopTrips, hwc_name.str().c_str(),
                       "Number of trips through spin loops");

    hwc_name.str("");   // Clear
    hwc_name << "HWC_" << tpu << "_user_spin_loop_cycles";
    reg->RegisterState(&userSpinLoopCycles, hwc_name.str().c_str(),
                       "Cycles spent in spin loops");

    hwc_name.str("");   // Clear
    hwc_name << "HWC_" << tpu << "_kernel_spin_loop_trips";
    reg->RegisterState(&kernelSpinLoopTrips, hwc_name.str().c_str(),
                       "Number of trips through kernel spin loops loops excluding idle loop");

    hwc_name.str("");   // Clear
    hwc_name << "HWC_" << tpu << "_kernel_spin_loop_cycles";
    reg->RegisterState(&kernelSpinLoopCycles, hwc_name.str().c_str(),
                       "Cycles spent in kernel spin loops excluding idle loop");

    hwc_name.str("");   // Clear
    hwc_name << "HWC_" << tpu << "_idle_spin_loop_trips";
    reg->RegisterState(&idleSpinLoopTrips, hwc_name.str().c_str(),
                       "Number of trips through kernel idle spin loops");

    hwc_name.str("");   // Clear
    hwc_name << "HWC_" << tpu << "_idle_spin_loop_cycles";
    reg->RegisterState(&idleSpinLoopCycles, hwc_name.str().c_str(),
                       "Cycles spent in kernel idle spin loops");

    hwc_name.str("");   // Clear
    hwc_name << "HWC_" << tpu;
    reg->RegisterState(&uid, hwc_name.str().c_str(), "Hardware context");

    SetTraceableName("HW_CONTEXT_CLASS");
    T1("Created HWC #" << uid);
}


HW_CONTEXT_CLASS::~HW_CONTEXT_CLASS ()
{
//    allHWCs[uid] = NULL;
}


/*------------------- IDToString --------------------------------------*/

/** 
 * Routine to print useful identifying information about a hardware
 * context. Call this instead of writing your own code. That way, as
 * we evolve the way of identifying a hardware context, only this
 * routine will have to change.
 *
 * For tracing, do this: 
 * T1(hwc);
 * where xxx is the trace flag for the calling component. Substitute
 * the name of the variable that contains a pointer to an object of
 * type HW_CONTEXT_CLASS.
 */

HW_CONTEXT_CLASS::operator string()
{
    return(IDToString());
}

string
HW_CONTEXT_CLASS::IDToString()
{
    std::ostringstream buf;
    buf << "Hardware context: CPU = " << cpu
         << " TPU = " << tpu;
    return(buf.str());
}
    
/*------------------------ StateToString -------------------------------*/

/** Routine to print the state of a hardware context. Call this
 *  instead of writing your own.
 */

string
HW_CONTEXT_CLASS::StateToString()
{
    std::ostringstream buf;

    if (runningSWC != ASIM_SWC_NONE)
    {
        buf << "Running ";
        //buf << runningSWC->IDToString();
    }
    else
    {
        buf << "Not running a software context";
    }
    return(buf.str());
}

string
HW_CONTEXT_CLASS::TraceToString()
{
    std::ostringstream buf;
    buf << "\tHWC:"
        << " uid=" << uid
        << ", cpu=" << cpu
        << ", tpu=" << tpu
        << ", pri=" << myPriority
        << ", swc=" << (runningSWC == ASIM_SWC_NONE ? -1 : static_cast<INT32>(runningSWC->GetUniqueID()));
    return(buf.str());
}

/* ------------------------- WarmUp ---------------------------*/

void
HW_CONTEXT_CLASS::WarmUpClientInfo(
    const WARMUP_CLIENTS clientInfo)
{
    if (runningSWC != ASIM_SWC_NONE)
    {
        runningSWC->WarmUpClientInfo(clientInfo);
    }
}


bool
HW_CONTEXT_CLASS::WarmUp(
    WARMUP_INFO warmup)
{
    if (runningSWC != ASIM_SWC_NONE)
    {
        return runningSWC->WarmUp(warmup);
    }
    else
    {
        return false;
    }
}

/* ------------------------- Fetch ----------------------------*/

ASIM_INST
HW_CONTEXT_CLASS::Fetch (
    UINT64 cycle,
    IADDR_CLASS ip)
{
    if (runningSWC != ASIM_SWC_NONE)
    {
        ASIM_INST instr = runningSWC->Fetch(cycle, ip);

        //
        // If this is the first instruction we need to find out about the
        // running process.
        //
        if (curProcessIdx == -1)
        {
            curProcessIdx = 0;
            maxProcessIdx = 0;
            processHistory.RowName(runningSWC->GetProcessName(), 0);
            pid = runningSWC->GetPid();
            processHistory.AddEvent(0, procHistPid, pid);
        }

        processHistory.AddEvent(curProcessIdx, procHistFetched, 1);

        return instr;
    }
    else
    {
        T1("\tNo mapped software context for this hardware context.");
        return NULL;
    }
        
}

ASIM_MACRO_INST
HW_CONTEXT_CLASS::FetchMacro (
    UINT64 cycle,
    IADDR_CLASS ip)
{
    if (runningSWC != ASIM_SWC_NONE)
    {
        ASIM_MACRO_INST instr = runningSWC->FetchMacro(cycle, ip);
        return instr;
    }
    else
    {
        T1("\tNo mapped software context for this hardware context.");
        return NULL;
    }
        
}

ASIM_INST
HW_CONTEXT_CLASS::FetchMicro (
    UINT64 cycle,
    ASIM_MACRO_INST macro,
    IADDR_CLASS ip)
{
    if (runningSWC != ASIM_SWC_NONE)
    {
        ASIM_INST instr = runningSWC->FetchMicro(cycle,macro,ip);

        //
        // If this is the first instruction we need to find out about the
        // running process.
        //
        if (curProcessIdx == -1)
        {
            curProcessIdx = 0;
            maxProcessIdx = 0;
            processHistory.RowName(runningSWC->GetProcessName(), 0);
            pid = runningSWC->GetPid();
            processHistory.AddEvent(0, procHistPid, pid);
        }

        processHistory.AddEvent(curProcessIdx, procHistFetched, 1);

        return instr;
    }
    else
    {
        T1("\tNo mapped software context for this hardware context.");
        return NULL;
    }
        
}


void
HW_CONTEXT_CLASS::Commit(
    UINT64 cycle, 
    ASIM_INST ainst, 
    UINT64 nCommitted)
{
    lastCommitCycle = cycle;
    T1("\tCommitting at cycle"<<cycle);
 
    if (curProcessIdx >= 0)
    {
        processHistory.AddEvent(curProcessIdx, procHistCommitted, nCommitted);
    }
    
    ainst->GetSWC()->Commit(ainst);

    if ((runningSWC != NULL) && (runningSWC->GetPid() != pid))
    {
        //
        // Process id in feeder or SWC changed.  Add a new line in the histogram.
        // We do this only on commit to limit the number of times the pid
        // changes.  If we changed every time we saw a fetch or kill there
        // would be much more noise in this histogram during context switches.
        //
        curProcessIdx += 1;
        maxProcessIdx += 1;
        if (curProcessIdx >= (INT32)PROCESS_HISTORY_BUCKETS)
        {
            //
            // Table is full
            //
            curProcessIdx = 0; // roll over and start at the beginning again.
            ASIMWARNING("Too many simulated context switches.  Raise PROCESS_HISTORY_BUCKETS to " << maxProcessIdx << endl);
            pid = runningSWC->GetPid();
        }
        else
        {
            processHistory.RowName(runningSWC->GetProcessName(), curProcessIdx);
            pid = runningSWC->GetPid();
            processHistory.AddEvent(curProcessIdx, procHistPid, pid);
            processHistory.AddEvent(curProcessIdx, procHistStartCycle, cycle);
        }
    }

    if ((runningSWC != NULL) && (runningSWC->InTimerInterrupt() != inTimerInterrupt))
    {
        if (inTimerInterrupt)
        {
            //
            // We were in a timer interrupt.  Update the statistics.
            //
            inTimerInterrupt = false;
            nTimerInterrupts += 1;
            timerInterruptCycles += (cycle - timerInterruptStartCycle);
        }
        else
        {
            //
            // Just entered interrupt
            //
            inTimerInterrupt = true;
            timerInterruptStartCycle = cycle;
        }
    }

    //
    // Spin loops.  Spin loops aren't tagged explicitly.  We use heuristics
    // to detect them.
    //
    ASIM_MACRO_INST mInst = ainst->GetMacroInst();
    if (mInst->IsDynamicEndOfMacro())
    {
        if (inSpinLoop)
        {
            instrsSinceLastSpinTag += 1;

            if (mInst->IsPauseOp())
            {
                // Another trip
                if (mInst->IsIdlePauseOp())
                {
                    idleSpinLoopTrips += 1;
                    idleSpinLoopCycles += (cycle - spinLoopStartCycle);
                }
                else if (mInst->IsKernelInstr())
                {
                    kernelSpinLoopTrips += 1;
                    kernelSpinLoopCycles += (cycle - spinLoopStartCycle);
                }
                else
                {
                    userSpinLoopTrips += 1;
                    userSpinLoopCycles += (cycle - spinLoopStartCycle);
                }

                spinLoopStartCycle = cycle;
                instrsSinceLastSpinTag = 0;
            }
            else if (instrsSinceLastSpinTag > 50)
            {
                // After 50 macro instructions assume we're no longer in
                // the spin loop
                inSpinLoop = false;
            }
        }
        else
        {
            //
            // Not currently in spin loop.  Is this the start of one?
            //
            if (mInst->IsPauseOp())
            {
                spinLoopStartCycle = cycle;
                instrsSinceLastSpinTag = 0;
                inSpinLoop = true;
            }
        }
    }
}

void
HW_CONTEXT_CLASS::CheckLiveness(
    UINT64 base_cycle)
{
    // IMPORTANT! We have to ask for the real cycle of this cpu!
    UINT64 cycle = asimSystem->SYS_Cycle(cpu);
    
    //
    // A bit of a hack here.  We want to time out early on bugs but not so
    // early that valid programs fail.  Tweak the timeout value automatically
    // based on the number of available hardware and software contexts.
    //

    //
    // Use the min of hardware and software contexts.  That way if most
    // of the hardware contexts aren't busy we don't count them.
    //
    UINT32 active = SW_CONTEXT_CLASS::NActiveContexts();
    if ((UINT32)uniqueHwCxtId < active)
    {
        active = uniqueHwCxtId;
    }

    UINT64 limit = LIVENESS_TIMEOUT + LIVENESS_TIMEOUT * ((active - 1) / 2);
    //UINT64 limit = LIVENESS_TIMEOUT + LIVENESS_TIMEOUT * ((active - 1));
    if (watchDogTimerOn && ((cycle - lastCommitCycle) >= limit))
    {
        cout << "Active contexts = " << active << endl;
        cout << "Liveness timeout, HWC uid: " << uid << endl;
        cout << "Current cycle: " << cycle << endl;
        cout << "Last commit cycle: " << lastCommitCycle << endl;
        ASSERTX(false);
    }
}


IADDR_CLASS
HW_CONTEXT_CLASS::GetResumptionPC(void) const
{
    if (GetSWC())
    {
        return GetSWC()->GetResumptionPC();
    }
    else
    {
        return IADDR_CLASS(0,0);
    }
}
