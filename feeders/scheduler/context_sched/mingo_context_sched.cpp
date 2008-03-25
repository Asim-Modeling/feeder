/* Copyright (C) 2002-2006 Intel Corporation
/* 
/* This program is free software; you can redistribute it and/or
/* modify it under the terms of the GNU General Public License
/* as published by the Free Software Foundation; either version 2
/* of the License, or (at your option) any later version.
/* 
/* This program is distributed in the hope that it will be useful,
/* but WITHOUT ANY WARRANTY; without even the implied warranty of
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/* GNU General Public License for more details.
/* 
/* You should have received a copy of the GNU General Public License
/* along with this program; if not, write to the Free Software
/* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
/* 
 * Copyright (c) 2002 Intel Corp.  Recipient is granted a non-sublicensable 
 * copyright license under Intel copyrights to copy and distribute this code 
 * internally only. This code is provided "AS IS" with no support and with no 
 * warranties of any kind, including warranties of MERCHANTABILITY,
 * FITNESS FOR ANY PARTICULAR PURPOSE or INTELLECTUAL PROPERTY INFRINGEMENT. 
 * By making any use of this code, Recipient agrees that no other licenses 
 * to any Intel patents, trade secrets, copyrights or other intellectual 
 * property rights are granted herein, and no other licenses shall arise by 
 * estoppel, implication or by operation of law. Recipient accepts all risks 
 * of use.
*/

/**
 * @file
 * @brief Implementation of the context scheduler class
 * @author Judy Hall
 *
 * NOTE: This module is not reentrant. Races are possible if multiple
 * calls are made concurrently. This is OK as long as the timing model
 * handles one hardware context at a time.

 * Work that still needs to be done: 
 *   Decide whether swc can be deleted
 *   In PickSoftwareContext change mod to use Artur's macros, clean
 *     out old code.
 */

/* Future enhancement
 * In Interval Timer, don't deschedule if there is a free hardware
 *    context.
 * Separate policy code so that there can be alternatives
 */


#include "asim/provides/context_scheduler.h"
#include "asim/provides/software_context.h"
#include "asim/provides/hardware_context.h"
#include "asim/trace.h"

// Required for call to CMD_Exit when feeder says it has finished.
#include "asim/provides/controller.h"

// Global pointer to Mingo Feeder

extern MINGO_FEEDER MingoFeederHandle;


/* ---------------- CONTEXT_SCHEDULER_CLASS -----------------------------*/

/**
 * Constructor: initializes data structures for scheduler. Must be
 * called only once 
 */

CONTEXT_SCHEDULER_CLASS::CONTEXT_SCHEDULER_CLASS (ASIM_MODULE parent, const
                                                  char* name)
    : ASIM_MODULE_CLASS(parent,name),
      currentCycle(0)
{
    
    TRACE(Trace_Context,
          cout << "CONTEXT_SCHEDULER_CLASS constructor was called"
          << endl);
    
    //Initialize data structures

    for (UINT32 i=0; i<MAX_NUM_SWCS; i++)
    {
        SWContextList[i] = ASIM_SWC_NONE; 
        TRACE(Trace_Context,
             cout << "SWContextList element " << i << " set to 0" << endl);
    }

    for (UINT32 i=0; i<MAX_TOTAL_NUM_HWCS; i++)
    {
        HWContextList[i] = ASIM_HWC_NONE;
        TRACE(Trace_Context,
            cout << "HWContextList element " << i << " set to 0" << endl);
    }
    
    statTimedWait = 0;
    RegisterState(&statTimedWait, "TimedWaitCalls",
                  "Number of calls to StartTimedWait");
    statSWCPaused = 0;
    RegisterState(&statSWCPaused, "SWCPaused", 
                  "Number of calls to SoftwareContextPaused");
    statSWCExited = 0;
    RegisterState(&statSWCExited, "SWCExited",
              "Number of calls to SoftwareContextExited");
    statSWCAdded = 0;
    RegisterState(&statSWCAdded, "SWCAdded",
                  "Number of calls to AddSoftwareContext");
    statContextSwitchCompleted = 0;
    RegisterState(&statContextSwitchCompleted,
                  "ContextSwitchCompleted",
                  "Number of calls to ContextSwitchCompleted");
    statContextSwitchNoSWC = 0;
    RegisterState(&statContextSwitchNoSWC, "ContextSwitchNoSWC",
    "Number of times a context switch completed and no swc was runnable");
    statSWCRunnableNoHWC = 0;
    RegisterState(&statSWCRunnableNoHWC, "SWCRunnableNoHWC",
    "Number of times a swc was runnable but no HWC was free");

}

/* ---------------- ~CONTEXT_SCHEDULER_CLASS ----------------------------*/

/** 
 * Destructor: Will this ever be called?
 */


CONTEXT_SCHEDULER_CLASS::~CONTEXT_SCHEDULER_CLASS ()
{

}

/* ---------------- DumpState --------------------------------------*/

/**
 * Routine to dump all scheduler state
 * Call this instead of writing your own code. That way, as
 * we evolve the way of identifying the state of the scheduler, only this
 * routine will have to change.
 *
 *
 * For tracing, do this: 
 * TRACE(Trace_Scheduler, ContextSchedulerHandle -> DumpState();
 */

void
CONTEXT_SCHEDULER_CLASS::DumpState(void)
{
    SW_CONTEXT tempSWC;
    HW_CONTEXT tempHWC;
    
    cout << "Contents of SWContextList:" << endl;
    for (UINT32 i=0; i<MAX_NUM_SWCS; i++)
    {
        tempSWC = SWContextList[i];
        cout <<"Index " << i << ": ";
        if (tempSWC != ASIM_SWC_NONE)
        {
            cout << "Data at " << tempSWC << " : ";            
            tempSWC->DumpID();
            tempSWC->DumpState();
        }
        else
        {
            cout << "No software context." << endl;
        }
    }
    

    cout << "Contents of HWContextList:" << endl;
    for (UINT32 i=0; i<MAX_TOTAL_NUM_HWCS; i++)
    {
        cout <<"Index " << i << ": ";
        tempHWC = HWContextList[i];        
        if (tempHWC != ASIM_HWC_NONE)
        {
            cout << "Data at " << tempHWC << " : ";            
            tempHWC->DumpID();
            tempHWC->DumpState();
        }
        else
        {
            cout << "No hardware context." << endl;
        }
    }
    
    cout << "nextSWCToTry = " << nextSWCToTry << endl;
}


/* ------------------AddSoftwareContext-----------------------------*/

/**
 * Originally this was to be called by the feeder. Currently it is
 * called only internally. Other feeders may want to call it. If not,
 * it can be made private.
 */

void 
CONTEXT_SCHEDULER_CLASS::AddSoftwareContext (SW_CONTEXT swc)
{

    TRACE(Trace_Context, 
        cout << "AddSoftwareContext: New software context received:" 
             << endl);
    TRACE(Trace_Context, swc->DumpID());
    
    statSWCAdded++;

    // Record the software context

    INT32 slot_found = -1;
    for (UINT32 i=0; i<MAX_NUM_SWCS; i++)
    {
        if (SWContextList[i] == ASIM_SWC_NONE)
        {
            SWContextList[i] = swc;
            slot_found = i;
            break;
        }
    }

    if (slot_found >= -1)
    {
        TRACE(Trace_Context,
            cout << "Added software context at slot " 
                 << slot_found  << endl);

        // Run it if there is a free hardware context. Otherwise, it's
        // marked as runnable and will be scheduled eventually.

        TryRunningSoftwareContext(swc);
    }

    else
    {
        // All available slots have been used. Print the ID of the
        // software context that was created, and then crash.

        swc->DumpID();
        ASIMERROR("CONTEXT_SCHEDULER_CLASS::AddSoftwareContext: No "
                  << "slot found for software context.");
     }    
}

/* ----------------- SoftwareContextPaused --------------------------------*/

/**
 * Called when a stream has paused.
 * Mark the software context as not runnable before descheduling
 * Assumption: If it paused, it was assigned to a hardware context.
 * We could verify this assumpion.
 * Note that this is specific to what we know about Mingo. We may have
 * to adjust the assumptions when we integrate with other feeders.
 *
 */

void
CONTEXT_SCHEDULER_CLASS::SoftwareContextPaused(SW_CONTEXT swc)
{
    statSWCPaused++;

    TRACE(Trace_Context, cout << "SoftwareContextPaused: called for"
                               << endl);
    TRACE(Trace_Context, swc->DumpID());

    swc->SetNotRunnable();
    StartDeschedule(swc);
}


/* ----------------- SoftwareContextExited --------------------------------*/

/**
 * Called when a software stream is deleted.
 * Similar to pausing, except at the end of the deschedule, we need to
 * clean up.
 */

void
CONTEXT_SCHEDULER_CLASS::SoftwareContextExited(SW_CONTEXT swc)
{

    statSWCExited++;

    TRACE(Trace_Context, cout << "SoftwareContextExited called for ");
    TRACE(Trace_Context, swc->DumpID());

    // Mark the software context as not runnable
    swc->SetNotRunnable();
    

    //Software context may be assigned to a hardware context or not. If not, 
    //we can complete the deletion now.
    // NOTE: This case probably does not occur with the current design
    // of Mingo. The stream would have to have been running in order
    // for SW_CONTEXT to see the exit event. Consider removing this only
    // after integrating other feeders with the scheduler.


    if (swc->GetMappedToHWC() == false)
    {
        // Not running on a hardware context, so there is no need to
        // run it down. Complete the deletion. Note that Mingo does
        // not require any confirmation of this. Consider whether that
        // is true for other feeders.

        // May want to keep the SW_CONTEXT for statistics
        // purposes. Also, we may need it for stores that complete
        // after the context switch commits.



        RemoveSoftwareContext(swc);

        /* 5/6/2002: Removing this delete until we are sure where it
         * can be done safely. This will create a memory leak for now.
        delete swc;
        */
    }
    else
    {
        // Mark the software context as preparing to be deleted so that when 
        // the context switch completes, we know to delete it. Note
        // that the software context is still mapped to the hardware
        // context. We can't break the association until the
        // performance model has "run down" the software context,
        // ensuring that a context switch has occurred and no more
        // fetching will be done for this software context.

        swc -> SetDeletePending();

        // Start the descheduling operation.

        StartDeschedule(swc);
    }
}

/* ------------------- StartTimedWait -------------------------------- */

/**
 * Mingo has received a timed_wait event. Mark the swc as being in a 
 * timed wait, and record the cycle in which the wait expires.
 * We do this in the scheduler to avoid having to pass the current
 * cycle through the Fetch interface.
 * If deschedule_request is true, mark the swc as not runnable and
 * deschedule it. Otherwise, leave its state unchanged.
 */


void
CONTEXT_SCHEDULER_CLASS::StartTimedWait(SW_CONTEXT swc, 
                                        UINT64 waitNS, 
                                        bool deschedule_request)
{
    
    double cycles_per_ns;
    UINT64 cycles_to_wait;
    UINT64 resumption_cycle;
    double wait_ns_double;

    // Compute number of cycles to wait. The conversion factor is in
    // picoseconds because AWB's parameters appear to limited to integers.

    wait_ns_double = static_cast<double>(waitNS);
    cycles_per_ns = static_cast<double>(CYCLES_PER_PICOSECOND)/1000;
    ASSERT(cycles_per_ns >= 0, "Attempting to add negative cycles to "
           << "current cycle" << endl);
    cycles_to_wait = 
        static_cast<UINT64>(floor(wait_ns_double * cycles_per_ns + .5));

    // Compute the cycle in which this software context should resume.

    resumption_cycle = currentCycle + cycles_to_wait;
    
    // KLUDGE ALERT:
    // We're adding 1 to the cycle because the scheduler's clock
    // routine is called after that CPUs' clock routines. Therefore
    // the real current cycle is 1 greater than what the scheduler
    // recorded the last time it was clocked.

    resumption_cycle++;

    cout << "TEMP: StartTimedWait called for stream " 
         << swc->GetFeederStreamID() << " . Waiting "
         << cycles_to_wait << " cycles until cycle "
         << resumption_cycle << endl;
    
    
    TRACE(Trace_Context,
        cout << "CONTEXT_SCHEDULER_CLASS::StartTimedWait: Waiting " 
          << cycles_to_wait
          << " cycles until cycle " << resumption_cycle
          << endl);
    swc->SetMinCycleToResume(resumption_cycle);
    swc->SetInTimedWait();

    // If the caller requested descheduling, mark the swc as not
    // runnable and start descheduling it. The caller is responsible
    // for feeding a context switch to the timing model.

    if (deschedule_request)
    {
        swc->SetNotRunnable();
        StartDeschedule(swc);
    }

    // Increment count of calls to this routine

    statTimedWait++;
    
}


/* ----------------- AddHardwareContext --------------------------------*/

/**
 * This routine should be called when a new hardware context is
 * created. 

 * This routine records the existence of the hardware context in the
 * scheduler's list of hardware contexts. After this has happened,
 * when a software context is runnable, the scheduler will consider
 * this hardware context to be available to run it. Therefore, call
 * this when the hardware context is ready to accept input.
 *
 * There is no "remove" function, but one could be written.
 */

void
CONTEXT_SCHEDULER_CLASS::AddHardwareContext(HW_CONTEXT hwc)
{

    TRACE(Trace_Context, cout << "AddHardwareContext called for ");

    if (hwc == NULL)
    {
        TRACE(Trace_Context, cout << "NULL" << endl);
        return;
    }

    TRACE(Trace_Context, hwc->DumpID());
            
    INT32 slot_found = -1;
    for (UINT32 i=0; i<MAX_TOTAL_NUM_HWCS; i++)
    {
        if (HWContextList[i] == ASIM_HWC_NONE)
        {
            HWContextList[i] = hwc;
            slot_found = i;
            break;
        }
    }

    if (slot_found >= -1)
    {
        TRACE(Trace_Context,
           cout << "Added hardware context at slot " << slot_found 
              << ": ");

        // To do: Convert this to a trace statement. For now it is
        // useful on non-tracing runs to ensure that we created the
        // expected hardware contexts.

        hwc -> DumpID();
    }
    
    else
    {
        // All available slots have been used. Print the ID of the
        // hardware context that was created, and then crash.

        hwc->DumpID();
        ASIMERROR("CONTEXT_SCHEDULER_CLASS::AddHardwareContext: No "
                  << "slot found for hardware context.");
    }
    
}


/* ----------------- ContextSwitchCompleted --------------------------------*/

/**
 * Called when the performance model commits a context switch operation.
 * This context switch was initiated earlier, and could have
 * happened for any of these reasons:
 * - the software thread running in the software context terminated
 * - the software thread running in the software context voluntarily
 *   gave up the CPU temporarily
 * - the scheduler made a decision to choose another software context
 *   to run on the hardware context. 
 * In the last case, although the
 * scheduler decided that there was a software context that should be
 * run on this hardware context, that software context may
 * have been grabbed by another hardware context by now. (This can
 * happen if the context switch takes longer to complete on one
 * hardware context than the other.) So just pick the "best" software
 * context to run now. Later, we may want to be more sophisticated
 * about this.
 */


void
CONTEXT_SCHEDULER_CLASS::ContextSwitchCompleted(SW_CONTEXT swc)
{
    HW_CONTEXT hwc;
    
    statContextSwitchCompleted++;
    
    TRACE(Trace_Context, 
         cout << "ContextSwitchCompleted called for ");
    TRACE(Trace_Context, swc->DumpID());
    
    // When this routine is called, there should always be a hardware
    // context mapped to it. 

    if (swc->GetMappedToHWC() == false)
    {
        // Dump the ID of the software context before crashing.

        swc->DumpID();
        ASIMERROR( "CONTEXT_SCHEDULER_CLASS::ContextSwitchCompleted "
                   << "called when software "
                << "context is not mapped to a hardware context." <<
                   endl);
    }

    // no else needed from above if statement because it's crashing.
    // Get the hardware context on which this software context has
    // been running.

    hwc = swc->GetHWC();

    // Software should be descheduling. If not, we need to understand
    // what happened.

    if (swc -> GetDeschedulePending() != true)
    {
        // Dump the ID of the software context before crashing.

        swc -> DumpID();
        ASIMERROR("SW_CONTEXT_CLASS::ContextSwitchCompleted called when "
              << "software context is not being descheduled" << endl);
    }
    
    // This is the end of the descheduling, so clear the flag.

    swc->SetNotDeschedulePending();

    TRACE(Trace_Context, 
           cout << "Finished descheduling software context: " <<
           endl);
    TRACE(Trace_Context, swc -> DumpID());
    TRACE(Trace_Context, swc -> DumpState());
    
    // Record that this software context is no longer running so that
    // it can be picked to run again.
    
    swc -> SetNotMappedToHWC();
    swc -> SetNoHWC();
    
    // Software context might be in the process of being deleted. If
    // so, finish it. Remove it from the scheduler's data structure,
    // and delete it. 
    // We may want to keep it around for stats.
 
    if (swc -> GetDeletePending())
    {

        TRACE(Trace_Context, cout << "Deleted software context: ");
        TRACE(Trace_Context, swc -> DumpID());
        
        RemoveSoftwareContext(swc);

        // 5/15/2002: Temporarily removing this delete until we are
        // sure it is safe to do it (i.e., the timing model won't do
        // anything more with it.
//        delete swc;
        
    }

    // Pick a software context to run. If this descheduling was
    // prompted by finding a higher-priority software context to run,
    // we might want to ensure that we run that one. That would
    // require recording its ID, and marking it as available only to
    // this hardware context. Otherwise another hardware context might
    // have run it by now.

    SW_CONTEXT newSWC = PickSoftwareContext(hwc);

    if (newSWC != ASIM_SWC_NONE)
    {
        
        // PickSoftwareContext implements policy. 
        // Now that we have picked one, run it.

        RunSoftwareContext(newSWC,hwc);
    }
    else
    {

        // Mark that this hardware context is not running any software
        // context. Later, when a software context is created, this
        // hardware context will appear free, and will be chosen to
        // run the software context.

        hwc -> SetNoSWC();
        statContextSwitchNoSWC++;
        
    }
    
}

        
/* ----------------- IntervalTimer-------- --------------------------------*/

/**
 * Called periodically for a hardware context, to simulate the action of
 * an interval timer. This gives the scheduler a chance to decide whether to
 * assign a new software context to this hardware context.
 */

void
CONTEXT_SCHEDULER_CLASS::IntervalTimer(HW_CONTEXT hwc)
{

    TRACE(Trace_Context,
          cout << "IntervalTimer called for hardware context: " << endl);
    TRACE(Trace_Context, hwc -> DumpID());
    

    // If the software context currently running is being descheduled, let
    // that operation continue. When it finishes, a new scheduling decision
    // can be made.

    SW_CONTEXT oldSWC = hwc->GetSWC();
    if (oldSWC-> GetDeschedulePending())
    {
        return;
    }
    
    // For now, always deschedule the current software context, even though
    // we may run it again. Even if we looked around and found a better 
    // candidate for running, it might get run by another HWC before we
    // finish descheduling the current software context.

    // If we do decide that this code should pick a new software
    // context for this hardware context,
    // we should mark it "schedule pending" and record the hardware context
    // on which it is expected to run, to prevent another hardware context
    // from taking it. But is that what we want? If a software context is 
    // ready to run, and another hardware context becomes free, why not run
    // it there? 

    // Future enhancement: If there is at least one free hardware
    // context, don't deschedule.

    // Tell the software context to send a context switch operationg
    // the next time that the timing model requests an operation. We do this
    // only on involuntary deschedules. When descheduling is
    // voluntary, FetchOperation sends the context switch. 

    // NOTE: This is valid for one-stage ONLY!!!! Need to think about how
    // this would work for a more complex processor.

    oldSWC->InitiateContextSwitch();
    
    // Record this in the scheduler's data.

    StartDeschedule (oldSWC);
    
}

/* -------------------------- Clock ------------------------------------ */

/**
 * Called every cycle. Performs certain polling operations at
 * intervals controlled by CHECK* values.
 */


void 
CONTEXT_SCHEDULER_CLASS::Clock(UINT64 cycle)
{

    HW_CONTEXT hwc;

    TRACE(Trace_Context, 
          cout << cycle << ": Context Scheduler" << endl);

    // Save the current cycle. We need this for expiring timed waits

    currentCycle = cycle;
    
    
    // See if Mingo has exited

    if (cycle % CHECK_EXIT == 0)
    {
        if (MingoFeederHandle->EndOfData())
        {

            cout << "TEMP: Scheduler detected Mingo's termination" 
                  << endl;
            TRACE(Trace_Context,
                  cout << "Scheduler detected Mingo's termination" <<
                  endl);

            // This was an experiment. It seems to work. It also seems
            // to be necessary to allow the scheduler to keep running
            // after the call. When there was an ASSERT macro
            // immediately after the call, the assertion was fired and
            // the run didn't terminate properly.
            
            CMD_Exit(ACTION_NOW,0);
            
        }
        
    }
    
    // If any streams that were in timed wait have expired their
    // timers, mark them as no longer waiting.

    UpdateTimedWait(cycle);
    

    // See if any streams that were non-runnable have become runnable

    if (cycle % CHECK_RUNNABLE == 0)
    {
        CheckStreamResumption();
    }

    // See if any new streams have been created.

    if (cycle % CHECK_NEW_STREAM == 0)
    {
        CheckForNewStream();
    }

    // Run interval timer for each hardware context that has a
    // software context running on it. This may trigger a decision to
    // reschedule. Note that this code does not call IntervalTimer for
    // hardware contexts that don't currently have a software context
    // mapped to them. Assumption: when a software context is created or
    // becomes runnable, an attempt is made to schedule it on a free
    // hardware context. Therefore, there should never be a hardware
    // context free when a runnable software context is waiting to run.
    // Also, when a hardware context becomes free at the end of a
    // context switch, an attempt is made to find it a waiting software 
    // context.
    // Also, if a HWC is running, check it for liveness

    bool reschedule = ((INTERVAL_TIMER != 0) && (cycle % INTERVAL_TIMER == 0) && (cycle != 0));

    for (UINT32 i=0; i<MAX_TOTAL_NUM_HWCS; i++)
    {
        hwc = HWContextList[i];
        // Ignore empty slots in the list   
        if (hwc != ASIM_HWC_NONE)
        {
            // If hwc isn't running anything, no need to reschedule
            if (hwc -> GetSWC() != ASIM_SWC_NONE)
            {
                hwc->CheckLiveness(cycle);
                if (reschedule)
                {
                    IntervalTimer(hwc);
                }
            }
        }
    }
}


/************ Start of Private Member Functions ***************************/

/* ----------------- StartDeschedule --------------------------------*/

/**
 * May be called voluntarily when thread pauses or is deleted,
 * or involuntarily if the interval timer routine decides to deschedule
 * the software context.
 * Descheduling occurs in two stages:
 * 1) We mark the software context as being descheduled, and send a 
 *    context switch instruction into the timing model.
 * 2) When the context switch is committed, we finish the descheduling
 *    and find another software context to run.
 * This routine is part of step 1.
 *
 * When the deschedule is voluntary, SW_CONTEXT_CLASS::FetchOperation
 * sends a context switch to the PM without help from the
 * scheduler. It calls one of several public methods in the scheduler,
 * which call this routine to update the scheduler's data.
 *
 * When the deschedule is involuntary, IntervalTimer tells
 * SW_CONTEXT_CLASS to initiate a context switch, and calls this
 * routine to update scheduler state.
 *
 */

void
CONTEXT_SCHEDULER_CLASS::StartDeschedule(SW_CONTEXT swc)
{

    TRACE(Trace_Context, cout << "StartDeschedule called for ");
    TRACE(Trace_Context, swc->DumpID());
    
    // If the software context isn't running on any hardware context, 
    // the caller has made a mistake. The Java thread might have
    // called the feeder saying "syscall" while it wasn't scheduled on
    // a hardware context, but the feeder should not see it and report
    // it to the scheduler until the software context is running on a
    // hardware context.

    if (swc->GetMappedToHWC() == true)
    {

        // It would be strange for this to be called when the software 
        // context is already marked as having a deschedule pending. 
        // Warn about it so that we can understand why it happens.

        if (swc->GetDeschedulePending() == true)
        {
            ASIMWARNING("CONTEXT_SCHEDULER_CLASS::StartDeschedule "
                << "called with a "
                << "software context whose deschedule pending"
                << " flag is set." << endl);
            TRACE(Trace_Context, swc->DumpID());
            TRACE(Trace_Context, swc->DumpState());            
        }

    // Mark the software context as deschedule pending, so that we won't
    // mistakenly schedule it on another hardware context.

        swc -> SetDeschedulePending();

        TRACE(Trace_Context, 
              cout << "Starting to deschedule ");
        TRACE(Trace_Context, swc -> DumpID());
        TRACE(Trace_Context, swc -> DumpState());

    }

    else
    {
        // Dump info about the software context before crashing.

        swc->DumpID();
        swc->DumpState();
        
        ASIMERROR("CONTEXT_SCHEDULER_CLASS::StartDeschedule called "
                  <<" with a software "
                 << "context that is not mapped to a hardware context."
                  << endl);
    }
    
}

/* ----------------- RemoveSoftwareContext --------------------------------*/

/**
 * Called when a software thread has been deleted. Removes the software
 * context from the scheduler's list.
 * 
 * The call to this routine would have been preceded by a call to
 * SoftwareContextExited, which would have led to a context switch if the 
 * software context was running at the time.
 *
 */

void
CONTEXT_SCHEDULER_CLASS::RemoveSoftwareContext (SW_CONTEXT swc)
{
    INT32 slot_found = -1;
    
    for (UINT32 i=0; i< MAX_NUM_SWCS; i++)
    {
        if (SWContextList[i] == swc)
        {
            SWContextList[i] = ASIM_SWC_NONE;
            slot_found = i;
            break;
        }
    }

    if (slot_found > -1)
    {
        TRACE(Trace_Context,
               cout << "Removed software context from index " <<
               slot_found << endl);
        TRACE(Trace_Context, swc -> DumpID());
        TRACE(Trace_Context, swc -> DumpState());
    }
    else
    {
        //Failed to find the software context in the list.
        //Print info about the software context before crashing.
        swc->DumpID();
        swc->DumpState();
        ASIMERROR("CONTEXT_SCHEDULER_CLASS::RemoveSoftwareContext: "
                  << "Trying to remove software "
                  <<"context that is not in the list.");
    }
    
}




/* ----------------- FindFreeHardwareContext --------------------------------*/

/**
 * Called when a software context is created, to see whether there is an idle
 * hardware context on which to run it. Returns pointer to the free hardware
 * context, or NULL if there isn't one.
 */

HW_CONTEXT
CONTEXT_SCHEDULER_CLASS::FindFreeHardwareContext()
{
    HW_CONTEXT free_hwc = ASIM_HWC_NONE;
    for (UINT32 i=0; i<MAX_TOTAL_NUM_HWCS; i++)
    {
        if (HWContextList[i] != ASIM_HWC_NONE)
        {
            if (HWContextList[i]->GetSWC() == ASIM_SWC_NONE)
            {
                free_hwc = HWContextList[i];
                TRACE(Trace_Context, cout << "FindFreeHardwareContext: "
                      << "Found free hardware context at index " 
                      << i << endl);
                TRACE(Trace_Context, free_hwc -> DumpID());
                break;
            }
            
        }
        
    }
    return free_hwc;
    
}

/* ------------------FindActiveSoftwareContext-------------------------------*/

/**
 * Find a software context that is runnable and not mapped to a
 * hardware context
 * Start with the swc at index nextSWCToTry, and continue until all
 * swc's have been checked
 * If checkForEvents is true, the feeder must have an event for the
 * swc in order for it to be selected. The feeder is asked to wait a
 * small amount of time for an event from each candidate swc.
 * If checkFoeEvents is false, any runnable swc can be selected.
 */

UINT32
CONTEXT_SCHEDULER_CLASS::FindActiveSoftwareContext (
    UINT32  nextSWCToTry,
    bool    checkForEvents
)
{
    UINT32 temp_index = nextSWCToTry;
    SW_CONTEXT newSWC;

    TRACE(Trace_Context, cout << "FindActiveSoftwareContext, nextSWCToTry="
          << nextSWCToTry << ", checkForEvents=" << checkForEvents << endl);

    do
    {
        // skip it if there is no software context here
        if (SWContextList[temp_index] != ASIM_SWC_NONE)
        {
            TRACE(Trace_Context, cout << " Considering ");
            TRACE(Trace_Context, SWContextList[temp_index] -> DumpID());
            TRACE(Trace_Context, SWContextList[temp_index] -> DumpState());
            
            if ((SWContextList[temp_index] -> GetRunnable()) && 
                (SWContextList[temp_index] -> GetMappedToHWC() == false) &&
                ((! checkForEvents) ||
                 (SWContextList[temp_index] -> FeederHasEvents(1000) == true)))
            {
                newSWC = SWContextList[temp_index];            

                TRACE(Trace_Context,
                      cout << " Picking software context at index "
                      << temp_index << endl);
                TRACE(Trace_Context, newSWC -> DumpID());

                if (newSWC->GetDeschedulePending() == true)
                {
                    ASIMWARNING("CONTEXT_SCHEDULER_CLASS::FindActiveSoftwareContext: "
                                << "Deschedule pending flag " 
                                << "is set when no hardware context "
                                << "is assigned." << endl);
                    TRACE(Trace_Context, newSWC->DumpState());
                }
                
                return temp_index;
            }
        }

        // Step to next entry.       
        temp_index = (temp_index + 1) % (MAX_NUM_SWCS);

    } while (temp_index != nextSWCToTry); // Stop when reach starting point

    return MAX_NUM_SWCS;    // No context available
}
/* ---------------- FindWaitingSoftwareContext ----------------------- */

/*
 * Find a software context that is in a timed wait and is not
 * currently mapped to a hardware context. The scheduler would prefer
 * to schedule such a swc over one that has no events. By the time
 * this routine is called, PickNextSWC has checked all runnable and 
 * all non-runnable streams to see if they have events. The only
 * remaining choice is a stream that is marked runnable but has no event.
 *
 * This swc may be marked as not runnable. This could have happened
 * when the timed wait first occurred (as requested in the call to 
 * StartTimedWait) or the swc may have been descheduled involuntarily
 * while it was waiting. In order for the scheduler to run it, we set
 * it runnable when we select it.
 */

// Future optimization: find the swc whose timeout value is
// lowest. This would require a call to GetMinCycleToResume for each
// swc for which GetInTimedWait returns true.

UINT32
CONTEXT_SCHEDULER_CLASS::FindWaitingSoftwareContext (
    UINT32  nextSWCToTry
)

{
    UINT32 temp_index = nextSWCToTry;
    SW_CONTEXT newSWC;

    TRACE(Trace_Context, cout << "FindWaitingSoftwareContext, nextSWCToTry="
          << nextSWCToTry << endl);

    do
    {
        // skip it if there is no software context here
        if (SWContextList[temp_index] != ASIM_SWC_NONE)
        {
            TRACE(Trace_Context, cout << " Considering ");
            TRACE(Trace_Context, SWContextList[temp_index]->DumpID());
            TRACE(Trace_Context, SWContextList[temp_index]->DumpState());
            
            if ((SWContextList[temp_index]->GetMappedToHWC() == false) &&
                (SWContextList[temp_index]->GetInTimedWait()))
            {
                newSWC = SWContextList[temp_index];            

                TRACE(Trace_Context,
                      cout << " Picking software context at index "
                      << temp_index << endl);
                TRACE(Trace_Context, newSWC -> DumpID());

                if (newSWC->GetDeschedulePending() == true)
                {
                    ASIMWARNING("CONTEXT_SCHEDULER_CLASS::FindWaitingSoftwareContext: "
                                << "Deschedule pending flag " 
                                << "is set when no hardware context "
                                << "is assigned." << endl);
                    TRACE(Trace_Context, newSWC->DumpState());
                }
                
                if (!newSWC->GetRunnable())
                {
                    newSWC->SetRunnable();
                    TRACE(Trace_Context, 
                          cout << "Setting the software context runnable"
                          << endl);
                    TRACE(Trace_Context, newSWC->DumpState());
                }
                
                return temp_index;
            }
        }

        // Step to next entry.       
        temp_index = (temp_index + 1) % (MAX_NUM_SWCS);

    } while (temp_index != nextSWCToTry); // Stop when reach starting point

    return MAX_NUM_SWCS;    // No context available
}

/* ------------------PickSoftwareContext-------------------------------*/

/**
 * This is the policy code. It should be in a module that can be replaced
 * by alternative policies
 */

SW_CONTEXT
CONTEXT_SCHEDULER_CLASS::PickSoftwareContext (HW_CONTEXT hwc)
{

    // In words: Find the next slot whose swc's hwc is null. If
    // you go all the way around the array without finding one, then
    // all software contexts are scheduled.

    //Checks required:
    // If at the end of the array, wrap to the top
    // If have gone all the way around, stop and return null
    // If find an entry that meets the requirements, stop and return pointer 

    // This relies on data member nextSWCToTry, which is initialized
    // to 0 when the scheduler class is instantiated. After a software
    // context is picked, this value is set to point to the slot after
    // the one where the picked software context was found.

    // If nothing is found, nextSWCToPick is incremented, so we start
    // at the next entry on the next pass. If I don't want that to
    // happen, I can move the saving of NextSWCToPick to the place
    // where the code decides it has a good software context.

    UINT32 newSWC_index;
    SW_CONTEXT newSWC;
    
    TRACE(Trace_Context,
          cout << "Entering PickSoftwareContext with ");
    TRACE(Trace_Context, hwc->DumpID());

    if (hwc->GetSWC() != ASIM_SWC_NONE)
    {
        TRACE(Trace_Context, 
              cout << "Previously ran software context " << endl);
        TRACE(Trace_Context, hwc->GetSWC()->DumpID());
    }
    else
    {
        TRACE(Trace_Context, 
              cout << "No software context previously assigned" << endl);
    }
        
    TRACE(Trace_Context,
        cout << "Entering PickSoftwareContext with nextSWCToTry = " 
             << nextSWCToTry << endl);
    
    //
    // First try to find a runnable software context with pending events
    //
    newSWC_index = FindActiveSoftwareContext(nextSWCToTry, true);

    if (newSWC_index == MAX_NUM_SWCS)
    {

        // Nothing runnable has events.  Check non-runnable streams to
        // see if they have events. If so, mark them as runnable.

        CheckStreamResumption();

        // Try again to find a runnable software context with pending
        // events. Note that CheckStreamResumption tries immediately
        // to schedule a newly-runnable swc. However, it will not
        // mistakenly schedule one onto the hwc that we are working
        // on, because this hwc still points to the swc that it was
        // previously running. 

        newSWC_index = FindActiveSoftwareContext(nextSWCToTry, true);
    }
    if (newSWC_index == MAX_NUM_SWCS)
        
    {
        // The preceding checks have skipped over any software context
        // that is waiting for the completion of a timed wait.
        // Now see if any swc is in that condition. If so, mark it as
        // runnable (if necessary), and we'll schedule that in
        // preference to one that is runnable but has no events.

        newSWC_index = FindWaitingSoftwareContext(nextSWCToTry);
    
    }
    
    if (newSWC_index == MAX_NUM_SWCS)
    {
        //
        // Still no luck.  Just pick a runnable software context
        // without requiring it to have events.
        //
        newSWC_index = FindActiveSoftwareContext(nextSWCToTry, false);
    }

    if (newSWC_index == MAX_NUM_SWCS)
    {
        //
        // Still no context available.  Give up.
        //
        newSWC = ASIM_SWC_NONE;
        nextSWCToTry = (nextSWCToTry + 1) % (MAX_NUM_SWCS);
        TRACE(Trace_Context,
            cout << "No software context is available to run." << endl);
    }
    else
    {
        newSWC = SWContextList[newSWC_index];
        nextSWCToTry = (newSWC_index + 1) % (MAX_NUM_SWCS);
    }
    
    TRACE(Trace_Context, 
         cout << "Leaving with nextSWCToTry = " << nextSWCToTry << endl);
    
    return newSWC;
}


    // Eric's version #1. Problem: if I init lastSWCPicked to 0, we will
    // skip over that. If I init it to -1, we'll never detect that we
    // have wrapped past it. It turns out that if we start with 0, and
    // we don't find anything in the other entries, we stop when we
    // get to 0 without looking there.

/*
    cout << "Entering with lastSWCPicked = " << lastSWCPicked << endl;
    
    temp_index = (lastSWCPicked + 1) % (MAX_NUM_SWCS);
    
    SW_CONTEXT newSWC = ASIM_SWC_NONE;
    
    while (temp_index != lastSWCPicked)
    {
        cout << "temp_index = " << temp_index << endl;
        // skip it if there is no software context here
        if (SWContextList[temp_index])
        {
            if ((SWContextList[temp_index] -> GetRunnable()) && 
                (SWContextList[temp_index] ->GetHWC()))
            {
                newSWC = SWContextList[temp_index];            
                break;
            }
        }
        temp_index = (temp_index + 1) % (MAX_NUM_SWCS);
    }
    lastSWCPicked = temp_index;
    cout << "Leaving with lastSWCPicked = " << lastSWCPicked << endl;
    
    return newSWC;
*/

    // My version. Problem: on first pass, temp_index = nextSWCToPick, so it
    // stops 

/*
    UINT32 nextSWCToPick = 0;

    temp_index = nextSWCToPick;
    bool found_one = false;
    
    SW_CONTEXT newSWC = ASIM_SWC_NONE;
    while ((temp_index != nextSWCToPick) && !found_one)
    {
        cout << "temp_index = " << temp_index;
        
        if ((SWContextList[temp_index] -> GetRunnable()) && 
            (SWContextList[temp_index] -> GetHWC()))
        {
            newSWC = SWContextList[temp_index];
            found_one = true;
            break;
            
        }
        temp_index = (temp_index + 1) % (MAX_NUM_SWCS);
        
     }
    nextSWCToPick = temp_index;
    return newSWC;
*/    



/* ------------------RunSoftwareContext---------------------------------*/

/**
 * This is the lowest-level routine for mapping a software context
 * onto a hardware context. The caller must ensure that the software
 * context is in an appropriate state for running, and that the
 * hardware context is not running another software context.
 */

void
CONTEXT_SCHEDULER_CLASS::RunSoftwareContext (SW_CONTEXT swc, HW_CONTEXT hwc)
{

    // The software context should not already be mapped to a hardware
    // context. 

    if (swc->GetMappedToHWC() == true)
    {
        // Dump information before crashing.

        swc->DumpID();
        swc->DumpState();
        hwc->DumpID();
        ASIMERROR("CONTEXT_SCHEDULER_CLASS::RunSoftwareContext: Trying "
            << "to schedule a software context "
            << "that is assigned to a hardware context." << endl);
    }

    if (swc->GetDeschedulePending() == true)
    {
        
        // Dump information before crashing.

        swc->DumpID();
        swc->DumpState();
        ASIMERROR("CONTEXT_SCHEDULER_CLASS::RunSoftwareContext: Trying "
            <<"to schedule a software context "
            <<"whose deschedule pending flag is set." << endl);
    }
    
    // For now, we assume that it is runnable. Mingo is not
    // going to call the scheduler when a thread that previously
    // released the CPU starts running again. The scheduler's clock
    // routine calls CheckStreamResumption to detect that a thread has
    // become runnable again. If we get here, it should be runnable.

    if (!swc->GetRunnable())
    {

        // Dump information before crashing.

        swc->DumpID();
        swc->DumpState();
        ASIMERROR("CONTEXT_SCHEDULER_CLASS::RunSoftwareContext: Trying "
            <<"to schedule a software context "
            <<"whose runnable flag is false." << endl);
    }
    
    // The effect of setting the hardware context's software context
    // should be that future FetchOperation calls will get instructions
    // from the new software context.

    TRACE(Trace_Context, 
               cout << "Scheduling ");
    TRACE(Trace_Context, swc->DumpID());
    TRACE(Trace_Context, cout << "  on hardware context ");
    TRACE(Trace_Context, hwc->DumpID());
 
    swc -> SetMappedToHWC();
    swc -> SetHWC(hwc);
    hwc -> SetSWC(swc, currentCycle);
    
}

/* ---------------------- TryRunningSoftwareContext -------------------*/

/**
 * A software context has become runnable (after being non-runnable, or
 * having just been created). See whether there is a hardware context 
 * available to run this software context. If so, run it.
 */

/*
 * Future enhancement: Move this into the policy module. Tell the
 * policy module that we have a newly-runnable SWC, and let it decide whether
 * to run it now or not. It can find a free HWC, map this SWC to
 * it, and make NextSWCToRun point to the next slot after the one
 * that this swc occupies.
 */


void
CONTEXT_SCHEDULER_CLASS::TryRunningSoftwareContext(SW_CONTEXT swc)
{

    HW_CONTEXT hwc;
    
    TRACE(Trace_Context, cout << "TryRunningSoftwareContext called for ");
    TRACE(Trace_Context, swc->DumpID());
    
    // Is there a free hardware context?

    hwc = FindFreeHardwareContext ();

    if (hwc != ASIM_HWC_NONE)
    {
        // As a way to get the policy engine to know that it
        // considered this software context (and presumably picked
        // it), tell it to pick a software context to run on a free
        // hardware context. Presumably if it knew there was a free
        // hardware context before, and there was a runnable software
        // context, it would have run it. For now we won't consider
        // that fatal; but we do want to know it happened to figure
        // out why it might.

        // Purpose of this hack: We need to let the policy module record
        // that this swc was scheduled, so that its pointer will be
        // adjusted to avoid having this be the first swc to be considered
        // on the next pass. 

        SW_CONTEXT pickedSWC = PickSoftwareContext(hwc);

        if (pickedSWC == swc)
        {
            // This is the case that we expect. A hardware context was
            // free, and a new software context comes along, and it is
            // run on that hardware context.

            RunSoftwareContext(swc,hwc);
        }
        else if (pickedSWC == NULL)
        {
            TRACE(Trace_Context,
                  cout << "  No sw context has events.  Nothing picked." << endl);
        }
        else
        {
            // Some other software context was picked. For now, just
            // report it so we can try to understand it. We have
            // recorded the new software context, and it will be run 
            // eventually. The effect should be the same as if there
            // had been no free hardware context at the time of creation.

            // 10/3/2002: Michael changed the behavior of
            // PickSoftwareContext such that if this swc has no
            // events, but another swc does have events, that other
            // swc will be picked. Therefore, this warning may go off,
            // and we may want to get rid of it if it happens
            // frequently.

            ASIMWARNING("CONTEXT_SCHEDULER_CLASS::TryRunningNewSoftwareContext: "
                <<"New software context not picked "
                << "when hardware context is free" << endl);
            TRACE(Trace_Context,
                  cout << "New software context: ");
            TRACE(Trace_Context, swc -> DumpID());
            TRACE(Trace_Context, swc -> DumpState());
            TRACE(Trace_Context, cout << "Software context picked: ");
            TRACE(Trace_Context, pickedSWC -> DumpID());
            TRACE(Trace_Context, pickedSWC -> DumpState());
        }                
    }

    else
        //hwc is ASIM_HWC_NONE
    {
        TRACE(Trace_Context,
           cout << "TryRunningSoftwareContext: No free hardware context found"
                << " for software context"
                << endl);  
        statSWCRunnableNoHWC++;
        
    }
}


/* --------------------- CheckStreamResumption ----------------------*/

/**
 * See if any non-runnable streams have become runnable. If so, mark
 * them as runnable, and possibly schedule them.
 */

void
CONTEXT_SCHEDULER_CLASS::CheckStreamResumption()
{
    SW_CONTEXT swc;
    HW_CONTEXT hwc;
    bool stream_runnable;

    TRACE(Trace_Context,
          cout << "Scheduler's CheckStreamResumption was called" <<
          endl);
    
    
    for (UINT32 i=0; i<MAX_NUM_SWCS; i++)
    {
        swc = SWContextList[i];
        if (swc != ASIM_SWC_NONE)
        {

            // Consider only streams that are not currently assigned to a
            // hardware context, not marked as runnable, and not 
            // being descheduled. The last check is redundant; there
            // should always be an assigned hardware context when
            // deschedule is pending.

            if ((swc->GetMappedToHWC() == false) && (!swc->GetRunnable()) &&
                (!swc->GetDeschedulePending()))
            {

                // See if the feeder has an event for the swc, but
                // don't allow it to wait. Optional argument of wait
                // time is 0 by default.

                if (swc->FeederHasEvents() == true)
                {

                    // Found a stream that has a waiting event. Set it
                    // runnable, and if there is a free hardware
                    // context, run it now. Otherwise, it will be
                    // picked eventually.

                    swc->SetRunnable();
                    TRACE(Trace_Context, cout << "Stream is runnable"
                          << endl);
                    TRACE(Trace_Context, swc->DumpState());
                    TryRunningSoftwareContext(swc);
                } // stream has event

                else
                {
                    TRACE(Trace_Context, cout << "Stream is still not "
                          <<"runnable" <<endl);
                }
                
            }
            else
            { 
                // Record that stream was skipped for debugging purposes

                TRACE(Trace_Context, cout << "Skipped runnable stream " 
                      << swc->GetFeederStreamID() << endl);
            }
            
        } // software context exists
    } // end of for loop
    
}


/* --------------------CheckForNewStream ---------------------------- */

/*
 * See if any new streams have been created. If so, create a software
 * context for them, and add them to the scheduler's list.
 */

void 
CONTEXT_SCHEDULER_CLASS::CheckForNewStream()
{

    UINT64 streamID;
    
    SW_CONTEXT swc;
    UINT64 starting_PC = 0;

    TRACE (Trace_Context,
           cout << "Scheduler's CheckForNewStream was called " <<
           endl);
    
    // Feeder returns 0 when there is no new stream. Keep trying until
    // we get a 0.

    // This is converting a UINT32 to UINT64

    streamID = MingoFeederHandle->CheckForNewSoftwareThread();
    TRACE(Trace_Context,
          cout << "Mingo's CheckForNewSoftwareThread returned " 
          << streamID << endl);
    
    while (streamID != 0)
    {
        // Create a software context object and make it runnable by
        // default.

        swc = new SW_CONTEXT_CLASS(streamID, starting_PC, this);
        AddSoftwareContext(swc);
        streamID = MingoFeederHandle -> CheckForNewSoftwareThread();
        TRACE(Trace_Context,
            cout << "Mingo's CheckForNewSoftwareThread returned " 
                 << streamID << endl);
    }

}

/* ---------------------- UpdateTimedWait ---------------------------- */

/**
 * For all streams that are in timed wait, see whether their time has come.
 * The scheduler does this work because it knows the current time. If
 * so, clear the inTimedWait bit in the software context.
 * This is an
 * alternative to passing the cycle to the software context in FetchOperation.
 */

// NOTE: This code can be optimized in multiple ways. This is just a first
// implementation. We can remember the earliest time that any stream
// will time out, for example.

// Also, in the case where TIMED_WAIT_CAUSES_DESCHEDULE is true, this
// code could mark the swc as runnable when its timeout expires. For
// now, we just mark is as no longer in timed wait. The next time the
// scheduler calls swc->FeederHasEvents, the timeout will have expired
// and if there is an event, the swc will be marked as runnable.

// We could also set the cycle to resume to 0 when we set the swc as
// no longer in timed wait, but it's not necessary, and the data might
// prove useful in analyzing a bug.

void
CONTEXT_SCHEDULER_CLASS::UpdateTimedWait(UINT64 current_cycle)
{
    UINT64 resumption_cycle;

    for (UINT32 i=0; i<MAX_NUM_SWCS; i++)
    {
        if (SWContextList[i] != ASIM_SWC_NONE)
        {
            if (SWContextList[i]->GetInTimedWait())
            {
                resumption_cycle = SWContextList[i]->GetMinCycleToResume();
                if (current_cycle > resumption_cycle)
                {
                    ASIMWARNING("Timed wait expired in a previous cycle. "
                                << " Resumption cycle = " << resumption_cycle
                                << " Current cycle = " << current_cycle
                                << endl);
                }
                if (current_cycle >= resumption_cycle)
                {
                    
                    cout << "TEMP: Timed wait expired for " << endl;
                    SWContextList[i]->DumpID();

                    TRACE (Trace_Context, 
                        cout << "Timed wait expired for " << endl);
                    TRACE(Trace_Context, SWContextList[i]->DumpID());
                    TRACE(Trace_Context, SWContextList[i]->DumpState());
                    SWContextList[i]->SetNotInTimedWait();
                }
            }
        }
    }
}


