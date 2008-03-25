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
 * @file
 * @brief Implementation of the context scheduler class
 * @author Judy Hall
 *
 * NOTE: This module is not reentrant. Races are possible if multiple
 * calls are made concurrently. This is OK as long as the timing model
 * handles one hardware context at a time.
 *
 * Creation and deletion of software contexts: The scheduler creates a
 * software context when the feeder says it has a new stream. In theory,
 * it could delete the swc after the stream has ended and the swc has
 * been descheduled. There are deletes in SoftwareContextExited and
 * ContextSwitchCompleted for this purpose. They were commented out
 * because it wasn't clear whether there might be a pointer from an
 * instruction to its software context, and that instruction might linger
 * in the performance model after the context switch completed. For
 * one-stage each ASIM_INST points to its software context. But
 * ContextSwitchCompleted isn't called until all operations have
 * committed, so deleting the software context would be safe. This might
 * not be true for more complex timing models. At some point, a decision can
 * be made:
 * 1) Never delete a software context. Perhaps record per-thread stats in
 * a swc.
 * 2) Delete a swc explicitly where the commented-code is.
 * 3) Use smart pointers and let swc go away when the last pointer is
 * null.
 */

/*
 * Work that still needs to be done: 
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

// 10/21/2002: The following two lines are commented out for now. They
// are specific to Mingo. Eventually the scheduler may call CMD_Exit
// for all feeders, but it does not do so now. Also, eventually the
// scheduler may need a generic pointer to the feeder. Judy.

// Required for call to CMD_Exit when feeder says it has finished.
//#include "asim/provides/controller.h"

// Global pointer to Mingo Feeder

//extern MINGO_FEEDER MingoFeederHandle;




/* ---------------- CONTEXT_SCHEDULER_CLASS -----------------------------*/

/**
 * Constructor: initializes data structures for scheduler. 
 */

CONTEXT_SCHEDULER_CLASS::CONTEXT_SCHEDULER_CLASS (ASIM_MODULE parent, const
                                                  char* name)
    : ASIM_MODULE_CLASS(parent,name),
      nextSWCToTry(0), 
      currentCycle(0),
      numSWC(0),
      numHWC(0),
      useSWCProcList(0)
{
    //Initialize data structures

    for (UINT32 i=0; i<MAX_NUM_SWCS; i++)
    {
        SWContextList[i] = ASIM_SWC_NONE; 
    }

    for (UINT32 i=0; i<MAX_TOTAL_NUM_HWCS; i++)
    {
        HWContextList[i] = ASIM_HWC_NONE;
    }

    for (UINT32 i=0; i<MAX_TOTAL_NUM_HWCS; i++)
    {
      SWCProcList[i] = MAX_TOTAL_NUM_HWCS+1; 
    }

    // Initialize stats
    
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
    // Note: software contexts are 'new'ed in this code, so we also delete
    // them here; hardware contexts are NOT 'new'ed here, so we also do NOT
    // delete them here!
    /*
    // If you want the per-inst stast to work correctly, then comment out the
    // delete of the SWC. We cannot delete the SWC because this
    // causes memory corruption problems.  Instructions are still
    // holding onto the SWC pointer when they get destroyed, and the
    // SWC is destroyed before the instructions. 
    */
    /*
    for (UINT32 i=0; i<MAX_NUM_SWCS; i++)
    {
        if (SWContextList[i] != ASIM_SWC_NONE) {
            delete SWContextList[i];
        }
    }
    */
}

/* ---------------- DumpState --------------------------------------*/

/**
 * Routine to dump all scheduler state
 * Call this instead of writing your own code. That way, as
 * we evolve the way of identifying the state of the scheduler, only this
 * routine will have to change.
 *
 * For tracing, do this: 
 * TRACE(Trace_Scheduler, ContextSchedulerHandle->DumpState();
 */

void
CONTEXT_SCHEDULER_CLASS::DumpState(void)
{
    SW_CONTEXT tempSWC;
    HW_CONTEXT tempHWC;
    
    cout << "\tContents of SWContextList:" << endl;
    for (UINT32 i=0; i<MAX_NUM_SWCS; i++)
    {
        tempSWC = SWContextList[i];
        cout <<"\tIndex " << i << ": ";
        if (tempSWC != ASIM_SWC_NONE)
        {
            cout << "Data at " << tempSWC << " : " <<  tempSWC->StateToString() << endl;
        }
        else
        {
            cout << "No software context." << endl;
        }
    }
    

    cout << "\tContents of HWContextList:" << endl;
    for (UINT32 i=0; i < numHWC; i++)
    {
        cout <<"\tIndex " << i << ": ";
        tempHWC = HWContextList[i];        
        if (tempHWC != ASIM_HWC_NONE)
        {
            cout << "Data at " << tempHWC << " : ";
            cout << tempHWC->TraceToString() << endl;
        }
        else
        {
            cout << "No hardware context." << endl;
        }
    }
    
    cout << "\tnextSWCToTry = " << nextSWCToTry << endl;
}


/* ------------------AddSoftwareContext-----------------------------*/

/**
 * @brief Add a software context to the scheduler's list and run it if 
 * there is a free hardware context.
 * 
 * Caller should have marked the software context as runnable.
 *
 * Originally this was to be called by the feeder. Currently it is
 * called only internally. Other feeders may want to call it. If not,
 * it can be made private.
 */

void 
CONTEXT_SCHEDULER_CLASS::AddSoftwareContext (SW_CONTEXT swc)
{

    T1("\tAddSoftwareContext: New software context received:\n" << swc->IDToString());

    statSWCAdded++;
    numSWC++;
    
    // Record the software context

    for (UINT32 i=0; i<MAX_NUM_SWCS; i++)
    {
        if (SWContextList[i] == ASIM_SWC_NONE)
        {
            SWContextList[i] = swc;
            T1("\tAdded software context at slot " << i);
            
            // Run it if there is a free hardware context. Otherwise, it's
            // marked as runnable and will be scheduled eventually.
            
            TryRunningSoftwareContext(swc);
            return;
        }
    }

    // All available slots have been used. Print the ID of the
    // software context that was created, and then crash.
    
    cout << swc->IDToString() << endl;
    ASIMERROR("CONTEXT_SCHEDULER_CLASS::AddSoftwareContext: No "
              << "slot found for software context.");
}

/* ----------------- SoftwareContextPaused --------------------------------*/

/**
 * @brief Called when a stream has paused.
 * Mark the software context as not runnable and request that it be 
 * descheduled.
 *
 * The caller is responsible for initiating a context switch in the PM
 * 
 * This routine is called by software context when the Mingo feeder
 * says that a thread has paused. For the trace feeder, we might have
 * the comparable situation if we support instructions from more than
 * one stream in a single file. As the feeder detects that instructions
 * are no longer coming from thread A, but from thread B, it can call
 * this routine for thread A. It should also feed to the PM something
 * that causes the PM to do a context switch. In one-stage, this is a
 * context switch instruction. When that instruction is committed,
 * something should call ContextSwitchCompleted.
 *
 * Assumption: If it paused, it was assigned to a hardware context.
 * We could add an assertion to verify this assumpion.
 *
 */

void
CONTEXT_SCHEDULER_CLASS::SoftwareContextPaused(SW_CONTEXT swc)
{
    statSWCPaused++;

    T1("\tSoftwareContextPaused: called for " << swc->IDToString());

    swc->SetNotRunnable();
    StartDeschedule(swc);
}


/* ----------------- SoftwareContextExited --------------------------------*/

/**
 * @brief Called when a software stream is deleted.
 *
 * Similar to SWCPaused, except at the end of the deschedule, we need to
 * clean up.
 *
 * Note that this is specific to what we know about Mingo. We may have
 * to adjust the assum
 *
 *
 * This routine is called by software context when the Mingo feeder
 * says that a thread has exited. For the trace feeder, we might call
 * this when a trace file hits end-of-file. If we support a trace file
 * with instructions from multiple streams, end-of-file could prompt
 * calling this routine for each stream in that file.
 */

void
CONTEXT_SCHEDULER_CLASS::SoftwareContextExited(SW_CONTEXT swc)
{

    statSWCExited++;

    T1("\tSoftwareContextExited called for " << swc->IDToString());

    // Mark the software context as not runnable
    swc->SetNotRunnable();
    
    // Software context may be assigned to a hardware context or not. If not, 
    // we can complete the deletion now.
    // NOTE: This case probably does not occur with the current design
    // of Mingo. The stream would have to have been running in order
    // for SW_CONTEXT to see the exit event.
    // After integrating other feeders with the scheduler, consider
    // causing an assertion failure if GetMappedToHWC returns false.
 
    if (swc->GetMappedToHWC() == false)
    {
        // Not running on a hardware context, so there is no need to
        // run it down. Complete the deletion. Note that Mingo does
        // not require any confirmation of the deletion. Consider
        // whether that is true for other feeders.

        // Remove this software context from the scheduler's list.

        RemoveSoftwareContext(swc);

        /* 5/6/2002: Removing this delete until we are sure where it
         * can be done safely. This will create a memory leak for now.
         * See discussion in the front of this file.

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
        // fetching will be done for this software context. When that
        // happens, ContextSwitchCompleted will be called.

        swc->SetDeletePending();

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

// 10/21/2002: This routine is not called for the trace feeder.


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
    // routine is called after the CPUs' clock routines. Therefore
    // the real current cycle is 1 greater than what the scheduler
    // recorded the last time it was clocked.

    resumption_cycle++;
    
    T1("\tCONTEXT_SCHEDULER_CLASS::StartTimedWait: Waiting " 
          << cycles_to_wait
          << " cycles until cycle " << resumption_cycle);
// 10/21/2002: Commented out because swc for trace feeder does not
// support these calls.
//    swc->SetMinCycleToResume(resumption_cycle);
//    swc->SetInTimedWait();

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
 *
 * This routine records the existence of the hardware context in the
 * scheduler's list of hardware contexts. After this has happened,
 * when a software context is runnable, the scheduler will consider
 * this hardware context to be available to run it. Therefore, call
 * this when the hardware context is ready to accept input.
 *
 * This routine does not try to find a runnable software
 * context. Presently, all hardware contexts are created during PM init,
 * which precedes feeder init. So no software context exists when this
 * routine is called. If hardware contexts are ever instantiated later
 * in a run, this routine can look for a software context that is
 * runnable and not mapped to a hardware context, and map it to the new
 * hardware context.
 *
 * There is no "remove" function, but one could be written.
 */

/*
 * NOTE: This routine is called before statistics collection is turned
 * on. Do not try to count the number of calls to this routine. You will
 * always get 0. There was code here to do that, and it counted up,
 * but then the number was reset to 0 when statistics collection was
 * turned on.
 */

void
CONTEXT_SCHEDULER_CLASS::AddHardwareContext(HW_CONTEXT hwc)
{
    T1("\tAddHardwareContext called for " << (hwc == NULL ? "NULL" : hwc->IDToString()));

    if (hwc == NULL)
    {
        return;
    }

            
    numHWC += 1;
    ASSERT(numHWC <= MAX_TOTAL_NUM_HWCS, "Too many hardware contexts");
    ASSERTX(HWContextList[numHWC-1] == ASIM_HWC_NONE);

    HWContextList[numHWC - 1] = hwc;
    T1("\tAdded hardware context at slot " << numHWC - 1 << ": " << hwc->TraceToString());
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
    
    T1("\tContextSwitchCompleted called for " << swc->IDToString());
    
    // When this routine is called, there should always be a hardware
    // context mapped to the software context.

    if (swc->GetMappedToHWC() == false)
    {
        // Dump the ID of the software context before crashing.

        cout << swc->IDToString() << endl;
        ASIMERROR( "CONTEXT_SCHEDULER_CLASS::ContextSwitchCompleted "
                   << "called when software "
                << "context is not mapped to a hardware context." <<
                   endl);
    }

    // Get the hardware context on which this software context has
    // been running.

    hwc = swc->GetHWC();

    // Software should be descheduling. If not, we need to understand
    // what happened.

    if (swc->GetDeschedulePending() != true)
    {
        // Dump the ID of the software context before crashing.

        cout << swc->IDToString() << endl;
        ASIMERROR("SW_CONTEXT_CLASS::ContextSwitchCompleted called when "
              << "software context is not being descheduled" << endl);
    }
    
    // This is the end of the descheduling, so clear the flag.

    swc->SetNotDeschedulePending();

    T1("\tFinished descheduling software context: " << endl);
    T1(swc->StateToString());
    
    // Record that this software context is no longer running so that
    // it can be picked to run again.
    
    swc->SetNotMappedToHWC();
    swc->SetNoHWC();
    
    // Software context might be in the process of being deleted. If
    // so, finish it. Remove it from the scheduler's data structure.

    if (swc->GetDeletePending())
    {

        T1("\tDeleted software context: " << swc->IDToString());
        
        RemoveSoftwareContext(swc);

        // 5/15/2002: Temporarily removing this delete until we are
        // sure it is safe to do it. See comments at the beginning of
        // this file.

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

        hwc->SetNoSWC();
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

    T1("\tIntervalTimer called for: " << hwc);
    

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

    // Tell the software context to send a context switch operation
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
 * intervals controlled by CHECK* parameters in context_sched.awb.
 */


void 
CONTEXT_SCHEDULER_CLASS::Clock(UINT64 cycle)
{

    HW_CONTEXT hwc;

    T1(cycle << ": Context Scheduler");
    
    // Save the current cycle. We need this for expiring timed waits

    currentCycle = cycle;
    
/*
// 10/21/2002: Removing this entire sequence. It was designed for Mingo.
// Later, we may support EndOfData in all feeders to determine
// whether it's time to terminate the run. 

    // See if Mingo has exited

    if (cycle % CHECK_EXIT == 0)
    {
        if (MingoFeederHandle->EndOfData())
        {

            T1(Trace_Context,
                  cout << "\tScheduler detected Mingo's termination" <<
                  endl);

            // This was an experiment. It seems to work. It also seems
            // to be necessary to allow the scheduler to keep running
            // after the call. When there was an ASSERT macro
            // immediately after the call, the assertion was fired and
            // the run didn't terminate properly.
            
            CMD_Exit(ACTION_NOW,0);
            
        }
        
    }
*/

/*
    // 10/21/2002: The following is specific to Mingo.
    
    // If any streams that were in timed wait have expired their
    // timers, mark them as no longer waiting.

    UpdateTimedWait(cycle);
*/
    
    // See if any streams that were non-runnable have become runnable

    if (cycle % CHECK_RUNNABLE == 0)
    {
        CheckStreamResumption();
    }

    // See if any new streams have been created.

    if (cycle % CHECK_NEW_STREAM == 0)
    {
        // this will eventually be used when we start software contexts from the
        // feeer, but right now, it's not how it's done.  Eric
//        CheckForNewStream();
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
    bool reschedule;

    reschedule = (INTERVAL_TIMER != 0)&&(((cycle % INTERVAL_TIMER) == 0) && (cycle != 0));

    for (UINT32 i=0; i < numHWC; i++)
    {
        hwc = HWContextList[i];
        // Ignore empty slots in the list   
        if (hwc != ASIM_HWC_NONE)
        {
            // If hwc isn't running anything, no need to reschedule
            if (hwc->GetSWC() != ASIM_SWC_NONE)
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

bool
CONTEXT_SCHEDULER_CLASS::IsHWCActive(UINT32 hwc_num) const
{
    return !((HWContextList[hwc_num] != ASIM_HWC_NONE) && (HWContextList[hwc_num]->GetSWC() == ASIM_SWC_NONE));
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

    T1("\tStartDeschedule called for " << swc->IDToString());
    
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
            T1(swc->StateToString());   
        }

    // Mark the software context as deschedule pending, so that we won't
    // mistakenly schedule it on another hardware context.

        swc->SetDeschedulePending();

        T1("\tStarting to deschedule " << swc->StateToString());

    }

    else
    {
        // Dump info about the software context before crashing.

        cout << swc->StateToString() << endl;
        
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
 * software context was running at the time. This routine should not
 * be called until the context switch has completed.
 *
 */

void
CONTEXT_SCHEDULER_CLASS::RemoveSoftwareContext (SW_CONTEXT swc)
{
    

            T1("remove swc called by tid: " << get_asim_thread_id());

//    swc->DumpID();

    for (UINT32 i=0; i< MAX_NUM_SWCS; i++)
    {
        if (SWContextList[i] == swc)
        {

            SWContextList[i] = ASIM_SWC_NONE;


            T1("\tRemoved software context from index " << i);
            T1(swc->StateToString());
            return;
        }
    }

    // Failed to find the software context in the list.
    // Print info about the software context before crashing.
    ASIMERROR("CONTEXT_SCHEDULER_CLASS::RemoveSoftwareContext: "
              << "Trying to remove software "
              <<"context that is not in the list.\n");
    
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
    INT32 free_hwc_priority = 1 << 31;  // smallest neg INT32 possible

    for (UINT32 i=0; i < numHWC; i++)
    {
        if ((HWContextList[i] != ASIM_HWC_NONE) && (HWContextList[i]->GetSWC() == ASIM_SWC_NONE) &&
            (HWContextList[i]->GetPriority() > free_hwc_priority))
        {
            free_hwc = HWContextList[i];
            free_hwc_priority = HWContextList[i]->GetPriority();

            T1("\tFound free hardware context at index " << i);
            T1(free_hwc->TraceToString());
        }
    }

    if (free_hwc != ASIM_HWC_NONE)
    {
        T1("\t--> Winning hardware context: uid=" << free_hwc->GetUID());
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

    T1("\tFindActiveSoftwareContext, nextSWCToTry="
          << nextSWCToTry << ", checkForEvents=" << checkForEvents);

    do
    {
        ASSERTX(temp_index < MAX_NUM_SWCS);

        // skip it if there is no software context here
        if (SWContextList[temp_index] != ASIM_SWC_NONE)
        {
            T1("\tConsidering " << SWContextList[temp_index]->StateToString());
/*
// 10/21/2002: This is the original check, replaced for the trace
// feeder by the one below it. The swc for the trace feeder does not
// support FeederHasEvents. It could do that in the future, especially
// if we support events from more than one stream in a single trace file.
            
            if ((SWContextList[temp_index]->GetRunnable()) && 
                (SWContextList[temp_index]->GetMappedToHWC() == false) &&
                ((! checkForEvents) ||
                 (SWContextList[temp_index]->FeederHasEvents(1000) == true)))
*/
            if ((SWContextList[temp_index]->GetRunnable()) && 
                (SWContextList[temp_index]->GetMappedToHWC() == false))
            {
                newSWC = SWContextList[temp_index];            

                T1("\tPicking software context at index "
                      << temp_index);
                T1(newSWC->IDToString());

                if (newSWC->GetDeschedulePending() == true)
                {
                    ASIMWARNING("CONTEXT_SCHEDULER_CLASS::FindActiveSoftwareContext: "
                                << "Deschedule pending flag " 
                                << "is set when no hardware context "
                                << "is assigned." << endl);
                    T1(newSWC->StateToString());
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

/**
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

// 10/21/2002: This code is useful for Mingo only. Only Mingo events
// include timed waits.

UINT32
CONTEXT_SCHEDULER_CLASS::FindWaitingSoftwareContext (
    UINT32  nextSWCToTry
)

{
    UINT32 temp_index = nextSWCToTry;
    SW_CONTEXT newSWC;

    T1("\tFindWaitingSoftwareContext, nextSWCToTry="
          << nextSWCToTry);

    do
    {
        // skip it if there is no software context here
        if (SWContextList[temp_index] != ASIM_SWC_NONE)
        {
            T1("\tConsidering " << SWContextList[temp_index]->StateToString());
            
/*
// 10/21/2002: This check is specific to Mingo. It is replaced for the
// trace feeder with the check below. In the future, perhaps all
// software contexts other than Mingo's can return false from
// GetInTimedWait.

            if ((SWContextList[temp_index]->GetMappedToHWC() == false) &&
                (SWContextList[temp_index]->GetInTimedWait()))
*/
            if (SWContextList[temp_index]->GetMappedToHWC() == false)
            {
                newSWC = SWContextList[temp_index];            

                T1("\tPicking software context at index "
                      << temp_index);
                T1(newSWC->IDToString());

                if (newSWC->GetDeschedulePending() == true)
                {
                    ASIMWARNING("CONTEXT_SCHEDULER_CLASS::FindWaitingSoftwareContext: "
                                << "Deschedule pending flag " 
                                << "is set when no hardware context "
                                << "is assigned." << endl);
                    T1(newSWC->StateToString());
                }
                
                if (!newSWC->GetRunnable())
                {
                    newSWC->SetRunnable();
                    T1("\tSetting the software context runnable");
                    T1(newSWC->StateToString());
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
    
    T1("\tEntering PickSoftwareContext with " << hwc);

    if (hwc->GetSWC() != ASIM_SWC_NONE)
    {
        T1("\tPreviously ran software context ");
        T1(hwc->GetSWC()->IDToString());
    }
    else
    {
        T1("\tNo software context previously assigned");
    }
        
    T1("\tEntering PickSoftwareContext with nextSWCToTry = " 
          << nextSWCToTry);
    
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

// 10/21/2002: This call applies only to Mingo, since that is the only
// feeder that supports timed waits.
//        newSWC_index = FindWaitingSoftwareContext(nextSWCToTry);
    
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
        T1("\tNo software context is available to run.");
    }
    else
    {
        newSWC = SWContextList[newSWC_index];
        nextSWCToTry = (newSWC_index + 1) % (MAX_NUM_SWCS);
    }
    
    T1("\tLeaving with nextSWCToTry = " << nextSWCToTry);
    
    return newSWC;
}

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

        cout << swc->StateToString() << endl;
        cout << hwc->TraceToString() << endl;
        ASIMERROR("CONTEXT_SCHEDULER_CLASS::RunSoftwareContext: Trying "
            << "to schedule a software context "
            << "that is assigned to a hardware context." << endl);
    }

    if (swc->GetDeschedulePending() == true)
    {
        
        // Dump information before crashing.

        cout << swc->StateToString() << endl;
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

        cout << swc->StateToString() << endl;
        ASIMERROR("CONTEXT_SCHEDULER_CLASS::RunSoftwareContext: Trying "
            <<"to schedule a software context "
            <<"whose runnable flag is false." << endl);
    }
    
    // The effect of setting the hardware context's software context
    // should be that future FetchOperation calls will get instructions
    // from the new software context.

    T1("\tScheduling " << swc->IDToString());
    T1("  on hardware context " << hwc);
 
    swc->SetMappedToHWC();
    swc->SetHWC(hwc);
    hwc->SetSWC(swc, currentCycle);
    
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

    HW_CONTEXT hwc = ASIM_HWC_NONE;
    
    T1("\tTryRunningSoftwareContext called for " << swc->IDToString());
    
    // Is there a free hardware context?

    if(!useSWCProcList)
    {
        hwc = FindFreeHardwareContext ();
    }
    // I don't know what this code is here for.  Might be nice if someone put
    // comments in.
    else
    {
        UINT32 proc_num =   GetSWCProc((useSWCProcList-1));
        ASSERT(proc_num < numHWC, "Proc id is larger than the number of HWC");
        useSWCProcList++;
        for (UINT32 i = 0; i < numHWC; i++)
	{
            UINT32 curr_cpu = i/NUM_HWCS_PER_CPU;
            if ((HWContextList[i] != ASIM_HWC_NONE) && (HWContextList[i]->GetSWC() == ASIM_SWC_NONE) && (curr_cpu == proc_num))
	    {
                hwc = HWContextList[i];
                break;
	    }
	}
        // this assert was outside the else loop, which is definitely wrong,
        // because this code allows for not having a hwc to run a swc on (see
        // else clause at end of this function!).
        ASSERT(hwc, "Free HWC not found");
    }
    
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
            T1("\tNo sw context has events.  Nothing picked.");
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
            T1("\tNew software context: " << swc->StateToString());
            T1("\tSoftware context picked: " << pickedSWC->StateToString());
        }                
    }

    else
        //hwc is ASIM_HWC_NONE
    {
        T1("\tTryRunningSoftwareContext: No free hardware "
              << "context found for software context");  
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

    T1("\tScheduler's CheckStreamResumption was called");
    
/*
// 10/21/2002: For trace feeder, just return. Once a stream has been
// marked not runnable, it should not become runnable again (at
// least not until we support multiple streams in a trace file)

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
                    T1(Trace_Context, cout << "\tStream is runnable" << endl);
                    T1(Trace_Context, swc->DumpState());
                    TryRunningSoftwareContext(swc);
                } // stream has event

                else
                {
                    T1(Trace_Context, cout << "\tStream is still not runnable" <<endl);
                }
            }
            else
            { 
                // Record that stream was skipped for debugging purposes

                T1(Trace_Context, cout << "\tSkipped runnable stream " 
                      << swc->GetFeederStreamID() << endl);
            }
            
        } // software context exists
    } // end of for loop
*/    
}


/* --------------------CheckForNewStream ---------------------------- */

/**
 * See if any new streams have been created. If so, create a software
 * context for each one, and add them to the scheduler's list.
 */

void 
CONTEXT_SCHEDULER_CLASS::CheckForNewStream()
{
    // current feeders don't support this.  Leave it here disabled for now.
    UINT64 streamID;
    
    SW_CONTEXT swc;
    UINT64 starting_PC = 0;

    T1("\tScheduler's CheckForNewStream was called ");
    
    // Feeder returns 0 when there is no new stream. Keep trying until
    // we get a 0.

    // This is converting a UINT32 to UINT64

/* 
// 10/21/2002: Commented out for trace feeder. Eventually we may want
// to use this approach for all feeders. In that case, we'll have a
// handle for whatever feeder is configured.

    streamID = MingoFeederHandle->CheckForNewSoftwareThread();
    T1(Trace_Context,
          cout << "\tMingo's CheckForNewSoftwareThread returned " 
          << streamID << endl);
    
    while (streamID != 0)
    {
        // Create a software context object and make it runnable by
        // default.

        swc = new SW_CONTEXT_CLASS(streamID, starting_PC, this);
        AddSoftwareContext(swc);
        streamID = MingoFeederHandle->CheckForNewSoftwareThread();
        T1(Trace_Context,
            cout << "\tMingo's CheckForNewSoftwareThread returned " 
                 << streamID << endl);
    }
*/
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

// 10/21/2002: This code is needed for Mingo only.

void
CONTEXT_SCHEDULER_CLASS::UpdateTimedWait(UINT64 current_cycle)
{
    // FIX: this routine shouldn't be called right now, not until we get a
    // common interface between feeder and the rest of the pm.
    ASSERT(false, "Shouldn't call UpdateTimedWait with this feeder\n");

#if 0
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
                    SWContextList[i]->IDToString();

                    T1("Timed wait expired for ");
                    T1(SWContextList[i]->StateToString());
                    SWContextList[i]->SetNotInTimedWait();
                }
            }
        }
    }
#endif
}

/**
 * @brief Start 'thread' running on the model.
 */
bool  ///< @return true on success
CONTEXT_SCHEDULER_CLASS::StartThread (
    ASIM_THREAD thread,  ///< thread descriptor
    UINT64 cycle)        ///< current cycle
{
    // why do we pass in the resumption pc here?  Why not just init it to 0?
    // Eric
    SW_CONTEXT swc = new SW_CONTEXT_CLASS(thread->IFeeder(),
                                          thread->IStreamHandle(),
                                          thread->StartVirtualPc(),
                                          this);
    AddSoftwareContext(swc);

    // normally returning true indicates that we've successfully scheduled it on
    // a tpu, but since we have a context scheduler to do that, it only means
    // we've accepted the new swc/thread
    return(true);
}


/**
 * @brief Stop 'thread' running on the model, and remove it.
 * This function will remove the first software context that
 * is mapped to a particular feeder stream. The corresponding
 * hardware context that the software context was on is reset
 * so that no SWC is running on it. This is necessary to allow
 * the skipping of a thread which is scheduled and then unscheduled
 * skipped and then rescheduled to the same processor. 
 * NOTE THIS ASSUMES A ONE TO ONE MAPPING!!! THIS WILL BREAK WHEN
 * MULTIPLE SWC HAVE THE SAME STREAM ID
 * 
 */
bool  ///< @return true on success
CONTEXT_SCHEDULER_CLASS::RemoveThread (
    ASIM_THREAD thread,  ///< thread descriptor
    UINT64 cycle)        ///< current cycle
{

    return true;
    // the code below is to make skipping work, but will break other things
    // (like when a thread runs out of instructions).  The skipping mechanism
    // runs for a cycle, then skips to the place it wants to start, then starts
    // up again.  The running for a cycle really messes things up and should be
    // removed!  So, if you want to fix skipping, do that!
    // The problem is that the context scheduler normally wants to be notified
    // that a thread is going away and it then it takes many cycles to purge the
    // pipeline of that thread before actually killing it.  But, the skipping
    // process wants the thread to be removed immediately, hence the code
    // below.
    // now, the real error is because when the feeder runs out of instructions
    // in a thread, it calls RemoveSoftwareContext through some path, and it
    // also calls RemoveThread.  When this function did nothing (before
    // skipping), no problem.  With the code below, it tried to remove the
    // thread/swc twice, hence the problem.  So, another issue to be fixed when
    // implementing skipping is to have the controller only call the context
    // scheduler once when there are no more instructions.  Eric


    for (UINT32 i=0; i< MAX_NUM_SWCS; i++)
    {
        //if there is a software context  and its feeder id is the same as the
        //streamID given then look up the hardware context that it has and
        //remove the software context and clear the hwc of software contexts.
        if (SWContextList[i] &&
            (SWContextList[i]->GetFeederStreamHandle() == thread->IStreamHandle()))
        {
//            SoftwareContextExited(SWContextList[i]);
            HW_CONTEXT hwc = SWContextList[i]->GetHWC();
            //RemoveSoftwareContext(SWContextList[i]);

            SWContextList[i] = ASIM_SWC_NONE; 

            if (hwc)
            {
                hwc->SetNoSWC();
            }
            //SWContextList[i]->SetNoHWC();
            T1("Removed thread");
            return true;
        }
    }
    ASSERT(0, "I didn't find any valid SWC to remove");
    return false;
}

