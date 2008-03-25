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
 * @brief Header file for the context scheduler
 *
 * @author Judy Hall
 */


#ifndef _CONTEXT_SCHEDULER_
#define _CONTEXT_SCHEDULER_

// Include files.

// Generic

// ASIM core
#include "asim/syntax.h"
#include "asim/module.h"

typedef class SW_CONTEXT_CLASS * SW_CONTEXT;
typedef class HW_CONTEXT_CLASS* HW_CONTEXT;

// ASIM local module

// ASIM other modules

#include "asim/provides/instfeeder_implementation.h"


/*----------------- CONTEXT_SCHEDULER_CLASS -----------------------*/

/// Pointer to the CONTEXT_SCHEDULER_CLASS class

typedef class CONTEXT_SCHEDULER_CLASS* CONTEXT_SCHEDULER;

/**
 * @brief Scheduler class

 * The class for the scheduler. The primary function of the
 * scheduler is to map software contexts (instances of class
 * SW_CONTEXT_CLASS) onto hardware contexts (instances of
 * HW_CONTEXT_CLASS)
 *
 */

class CONTEXT_SCHEDULER_CLASS : public ASIM_MODULE_CLASS
{
  public:

    /// Constructor. Initializes data structures for scheduler
    /// CAUTION: This constructor must be called only one. It stores
    /// a pointer to itself in ContextSchedulerHandle.

    CONTEXT_SCHEDULER_CLASS(ASIM_MODULE parent, const char* name);
    
    /// Destructor. Will this ever be called?

    ~CONTEXT_SCHEDULER_CLASS();

    /// Dump the contexts of the data structures

    void
    DumpState(void);
    

    /// Record that a new software context has been created, and possibly
    /// schedule it immediately.

    void
    AddSoftwareContext (
        SW_CONTEXT swc           ///< pointer to new software context
        );
    
    /// Handle software context's voluntarily releasing the CPU (a
    /// temporary measure)

    void
    SoftwareContextPaused (
        SW_CONTEXT swc          ///< pointer to pausing SW context
        );
    
    /// Handle deletion of software thread

    void
    SoftwareContextExited(
        SW_CONTEXT swc           ///< software context that will be deleted
        );

    /// Handle swc's receiving a timed_wait event from Mingo

    void
    StartTimedWait(
        SW_CONTEXT swc,          ///< Software context that will wait
        UINT64 waitnS,           ///< Requested number of nanoseconds
        bool deschedule_request  ///< True if we should deschedule
        );
    

    /// Handle arrival of new hardware context

    void
    AddHardwareContext (
        HW_CONTEXT hwc           ///< pointer to new hardware context
        );
    

    /// Declare that a context switch has completed. The currently-running
    /// software context will be unscheduled, and a new software context
    /// will be scheduled on the hardware context that was in use.

    void
    ContextSwitchCompleted (
        SW_CONTEXT swc         ///< software context that is being descheduled
        );
    
    /// Called once per cycle

    void
    Clock (
        UINT64 cycle             ///< Clock cycle in which call is made
        );

    /// Get the current cycle
    UINT64 GetCurrentCycle () const { return currentCycle; }

    // these will never be called in Mingo, but need to be here for
    // compatibility purposes.  Eventually, they'll need to be removed.
    bool StartThread (UINT64 streamId, UINT64 cycle) { ASSERTX(false); return false; }
    bool RemoveThread (UINT64 streamId, UINT64 cycle) { ASSERTX(false); return false; }
    bool HookAllThreads() { ASSERTX(false); return false; }
    bool UnhookAllThreads() { ASSERTX(false); return false; }
    bool BlockThread () { ASSERTX(false); return false; }
    bool UnblockThread () { ASSERTX(false); return false; }

  private:

    /// Start descheduling a software context

    void
    StartDeschedule (
        SW_CONTEXT swc                ///< software context to be descheduled
        );
    
    /// Remove this software context from the schedulers's list of 
    /// software contexts in preparation for deleting the object.

    void 
    RemoveSoftwareContext(
        SW_CONTEXT swc             ///< software context to be removed
    );

    /// See if any hardware context is idle.

    HW_CONTEXT
    FindFreeHardwareContext();
    
    // Called by PickSoftwareContext.  Go through current software contexts
    // in SWContextList, starting with nextSWCtoTry, and find a runnable
    // context.  If checkForEvents is true then call the feeder and require
    // than an event be pending before considering the context runnable.

    UINT32
    FindActiveSoftwareContext (
        UINT32  nextSWCToTry,  ///< Next index into SWContextList to try
        bool    checkForEvents ///< True if swc must have an event in order 
                               ///< to be selected
        );
    
    /// Find a software context that is in a timed wait. 

    UINT32
    FindWaitingSoftwareContext (
        UINT32 nextSWCToTry     ///< Next index into SWContextList to try
        );
    

    /// Select next software context to run on this hardware context

    SW_CONTEXT 
    PickSoftwareContext(
        HW_CONTEXT hwc          ///< hardware context on which to run SWC
        );

    /// Run selected software context on selected hardware context

    void
    RunSoftwareContext (
        SW_CONTEXT swc,         ///<software context to be run
        HW_CONTEXT hwc          ///<hardware context to run it on
        );
    
    /// Act like a hardware interval timer on a CPU, and consider running
    /// a different software context.

    void
    IntervalTimer(
        HW_CONTEXT hwc          ///< for which interval time has gone off
        );

    /// See whether there is a hardware context available to run this
    /// software context. If so, run it.

    void
    TryRunningSoftwareContext(
        SW_CONTEXT swc          ///< Software context to try running
        );

    /// See if any non-runnable streams have become runnable. If so, mark
    /// them as runnable, and possibly schedule them.

    void
    CheckStreamResumption();

    /// See if any new streams have been created. If so, create a software
    /// context for them.

    void 
    CheckForNewStream();    

    /// Check for streams whose timed wait has expired. Mark them as
    /// no longer waiting.

    void
    UpdateTimedWait(
        UINT64 cycle                   ///< Current cycle
        );
    
    // Private data

    SW_CONTEXT SWContextList[MAX_NUM_SWCS]; ///<list of software contexts
    HW_CONTEXT HWContextList[MAX_TOTAL_NUM_HWCS]; ///<list of software contexts
    UINT32 nextSWCToTry;               ///<round-robin index. Should
                                       ///<be in policy module.
    UINT64 currentCycle;               ///< Cycle in which clock was last 
                                       ///< called. Needed for timed wait.
    // Statistics

    UINT64 statTimedWait;              ///< Calls to StartTimedWait
    UINT64 statSWCPaused;              ///< Calls to SoftwareContextPaused
    UINT64 statSWCExited;              ///< Calls to SoftwareContextExited
    UINT64 statSWCAdded;               ///< Calls to AddSoftwareContext
    UINT64 statContextSwitchCompleted; ///< Calls to ContextSwitchCompleted
    UINT64 statContextSwitchNoSWC;     ///< Calls to ContextSwitchCompleted 
                                       ///< when there was no swc
                                       ///< waiting to run
    UINT64 statSWCRunnableNoHWC;       ///< SWC became runnable but no hwc
                                       ///< was available
};


#endif /* _CONTEXT_SCHEDULER_ */









