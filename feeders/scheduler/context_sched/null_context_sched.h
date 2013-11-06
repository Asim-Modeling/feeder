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
 * @brief Header file for the Null context scheduler
 *
 * @author Sailashri Parthasarathy (based on Judy Hall's original implementation)
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
//#include "asim/provides/instfeeder_implementation.h"


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
    ~CONTEXT_SCHEDULER_CLASS() {};

    /// Dump the contexts of the context scheduler's data structures
    void
    DumpState(void) { };
    
    /**
     * @brief Add a software context to the scheduler's list and run it if 
     * there is a free hardware context.
     */

    void
    AddSoftwareContext (
        SW_CONTEXT swc           ///< pointer to new software context
        ) { };
    
    /// Handle software context's voluntarily releasing the CPU

    void
    SoftwareContextPaused (
        SW_CONTEXT swc          ///< pointer to pausing software context
        ) { };
    
    /// Handle deletion of software thread

    void
    SoftwareContextExited(
        SW_CONTEXT swc           ///< pointer to exiting software context
        ) { };

    /// Handle swc's receiving a timed_wait event from Mingo

    void
    StartTimedWait(
        SW_CONTEXT swc,          ///< Software context that will wait
        UINT64 waitnS,           ///< Requested number of nanoseconds
        bool deschedule_request  ///< True if we should deschedule
        ) { };

    /// Handle arrival of new hardware context

    void
    AddHardwareContext (
        HW_CONTEXT hwc           ///< pointer to new hardware context
        ) { };
    
    /**
     * @brief Declare that a context switch has completed. 
     * The currently-running
     * software context will be unscheduled, and a new software context
     * will be scheduled on the hardware context that was in use.
     */

    void
    ContextSwitchCompleted (
        SW_CONTEXT swc         ///< software context that is being descheduled
        ) { };
    
    /// Called once per cycle

    void
    Clock (
        UINT64 cycle             ///< Clock cycle in which call is made
        ) { };

    /// Get the current cycle
    UINT64 GetCurrentCycle () const { return 0; }

    /*
     * Commands from the simulator instruction
     * performance model to start executing
     * a new thread or stop executing an existing
     * one.
     */
    bool StartThread (ASIM_THREAD thread, UINT64 cycle) { return false; }
    bool RemoveThread (ASIM_THREAD thread, UINT64 cycle) { return false; }
    /*
     *  Commands added for sampling. Unlike StartThread()
     *  and RemoveThread(), HookAllThreads() and UnhookAllThreads()
     *  do not restart or kill threads. What they do are simply
     *  stopping and resuming fetching.
     */
    bool HookAllThreads() { return false; }
    bool UnhookAllThreads() { return false; }
    /*
     * Commands from the simulator instruction
     * performance model to block  or unblock a thread.
     * Block thread kills all younger instructions and stops fetching for
     * the thread.
     * Unblock thread starts instruction fetching for the thread.
     */
    bool BlockThread () { return false; }
    bool UnblockThread () { return false; }

    bool IsHWCActive(UINT32 hwc_num) const { return false; }

    //
    // Public method for retrieving pointers to all the hardware contexts
    // in a system.
    // 
    UINT32 NumHWC(void) const { return 0; };
    HW_CONTEXT GetHWC(UINT32 idx) const
    {
        return HW_CONTEXT();
    };

    void SetSWCProcList( UINT32 index, UINT32 val)
      {
      };
    UINT32 GetSWCProc(UINT32 index)
      {
	return 0;
      };
    void UseSWCProcList ()
      {
      };

};

#endif /* _CONTEXT_SCHEDULER_ */
