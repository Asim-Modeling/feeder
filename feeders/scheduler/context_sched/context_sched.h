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
#include "asim/thread.h"

typedef class SW_CONTEXT_CLASS * SW_CONTEXT;
typedef class HW_CONTEXT_CLASS* HW_CONTEXT;

// ASIM local module

// ASIM other modules
#include "asim/provides/instfeeder_implementation.h"

// 10/22/2002: Judy's thoughts about future statistics
// When analyzing dynamic runs of multithreaded programs, it could be
// useful to have statistics that provide insight into how well the 
// software contexts and hardware contexts are being utilized. 
//
// For hardware contexts, it would be useful to know how much time
// they spend without a software context mapped to them (assuming that
// the workload offers at least as many software contexts as there are
// hardware contexts).
//
// For software contexts, it would be useful to know, over time, how
// many are runnable, and of those, how many don't have a hardware
// context to run on. 

// It is not clear whether it's better to collect these stats in the
// scheduler or in the swc's and hwc's. At present, software contexts
// do not collect statistics.

/*----------------- CONTEXT_SCHEDULER_CLASS -----------------------*/

/// Pointer to the CONTEXT_SCHEDULER_CLASS class

typedef class CONTEXT_SCHEDULER_CLASS* CONTEXT_SCHEDULER;

/**
 * @brief Scheduler class
 *
 * The class for the context scheduler. The primary function of the
 * context scheduler is to map software contexts (instances of class
 * SW_CONTEXT_CLASS) onto hardware contexts (instances of
 * HW_CONTEXT_CLASS)
 *
 */

class CONTEXT_SCHEDULER_CLASS : public ASIM_MODULE_CLASS
{
  public:

    /// Constructor. Initializes data structures for scheduler

    CONTEXT_SCHEDULER_CLASS(
        ASIM_MODULE parent,  ///< Pointer to parent module
        const char* name     ///< Name of this module
        );
    
    /// Destructor

    ~CONTEXT_SCHEDULER_CLASS();

    /// Dump the contexts of the context scheduler's data structures

    void
    DumpState(void);
    
    /**
     * @brief Add a software context to the scheduler's list and run it if 
     * there is a free hardware context.
     */

    void
    AddSoftwareContext (
        SW_CONTEXT swc           ///< pointer to new software context
        );
    
    /// Handle software context's voluntarily releasing the CPU

    void
    SoftwareContextPaused (
        SW_CONTEXT swc          ///< pointer to pausing software context
        );
    
    /// Handle deletion of software thread

    void
    SoftwareContextExited(
        SW_CONTEXT swc           ///< pointer to exiting software context
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
    
    /**
     * @brief Declare that a context switch has completed. 
     * The currently-running
     * software context will be unscheduled, and a new software context
     * will be scheduled on the hardware context that was in use.
     */

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

    /*
     * Commands from the simulator instruction
     * performance model to start executing
     * a new thread or stop executing an existing
     * one.
     */
    bool StartThread (ASIM_THREAD thread, UINT64 cycle);
    bool RemoveThread (ASIM_THREAD thread, UINT64 cycle);
    /*
     *  Commands added for sampling. Unlike StartThread()
     *  and RemoveThread(), HookAllThreads() and UnhookAllThreads()
     *  do not restart or kill threads. What they do are simply
     *  stopping and resuming fetching.
     */
    bool HookAllThreads();
    bool UnhookAllThreads();
    /*
     * Commands from the simulator instruction
     * performance model to block  or unblock a thread.
     * Block thread kills all younger instructions and stops fetching for
     * the thread.
     * Unblock thread starts instruction fetching for the thread.
     */
//    bool BlockThread (ASIM_THREAD thread);
//    bool UnblockThread (ASIM_THREAD thread);
    bool BlockThread ();
    bool UnblockThread ();

    bool IsHWCActive(UINT32 hwc_num) const;

    //
    // Public method for retrieving pointers to all the hardware contexts
    // in a system.
    // 
    UINT32 NumHWC(void) const { return numHWC; };
    HW_CONTEXT GetHWC(UINT32 idx) const
    {
        ASSERTX(idx < numHWC);
        return HWContextList[idx];
    };

    void SetSWCProcList( UINT32 index, UINT32 val)
      {
	ASSERTX(index < numHWC);
	SWCProcList[index] = val;
      };
    UINT32 GetSWCProc(UINT32 index)
      {
	ASSERTX(index < numHWC);
	return SWCProcList[index];
      };
    void UseSWCProcList ()
      {
	useSWCProcList=1;
      };

  private:

    /// Start descheduling a software context

    void
    StartDeschedule (
        SW_CONTEXT swc                ///< software context to be descheduled
        );
    
    /**
     * @brief Remove this software context from the schedulers's list of 
     * software contexts in preparation for deleting the object.
     */

    void 
    RemoveSoftwareContext(
        SW_CONTEXT swc             ///< software context to be removed
    );

    /// See if any hardware context is idle.

    HW_CONTEXT
    FindFreeHardwareContext();
    
    /// Pick a software context to schedule onto a hardware context

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
    
    /**
     * @brief Act like a hardware interval timer on a CPU, and consider 
     * running a different software context.
     */

    void
    IntervalTimer(
        HW_CONTEXT hwc          ///< for which interval time has gone off
        );

    /**
     * @brief See whether there is a hardware context available to run this
     * software context. If so, run it.
     */

    void
    TryRunningSoftwareContext(
        SW_CONTEXT swc          ///< Software context to try running
        );

    /**
     * @brief See if any non-runnable streams have become runnable. If so, mark
     * them as runnable, and possibly schedule them.
     */

    void
    CheckStreamResumption();

    /**
     * @brief See if any new streams have been created. If so, create a 
     * software context for them.
     */

    void 
    CheckForNewStream();    

    /**
     * @brief Check for streams whose timed wait has expired. Mark them as
     * no longer waiting.
     */

    void
    UpdateTimedWait(
        UINT64 cycle                   ///< Current cycle
        );
    

    // Private data

    SW_CONTEXT SWContextList[MAX_NUM_SWCS]; ///<list of software contexts
    HW_CONTEXT HWContextList[MAX_TOTAL_NUM_HWCS]; ///<list of hardware contexts
    UINT32 SWCProcList[MAX_TOTAL_NUM_HWCS]; // list of procid's to map SWC's to. 
    UINT32 nextSWCToTry;               ///< Round-robin index. Should
                                       ///< be in policy module.
    UINT64 currentCycle;               ///< Cycle in which clock was last 
                                       ///< called. Needed for timed wait.
    UINT32 numSWC;                     ///< Number of current SWCs.
    UINT32 numHWC;                     ///< Number of current HWCs.
    UINT32 useSWCProcList; 

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


inline bool
CONTEXT_SCHEDULER_CLASS::HookAllThreads()
{
    ASSERTX(0); return false;
}

inline bool
CONTEXT_SCHEDULER_CLASS::UnhookAllThreads()
{ 
    ASSERTX(0);  return false;
}

inline bool
CONTEXT_SCHEDULER_CLASS::BlockThread()
{
    ASSERTX(0);  return false;
}

inline bool
CONTEXT_SCHEDULER_CLASS::UnblockThread()
{ 
    ASSERTX(0);  return false;
}

#endif /* _CONTEXT_SCHEDULER_ */
