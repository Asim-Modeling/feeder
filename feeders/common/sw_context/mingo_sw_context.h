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
 * @author Judy Hall
 * @brief Software context (to be scheduled on a hardware context)
 * Includes methods for fetching and committing operations supplied by Mingo.
 */

#ifndef _SW_CONTEXT_CLASS_
#define _SW_CONTEXT_CLASS_

// generic
#include <iostream>

// ASIM core
#include "asim/syntax.h"
#include "asim/provides/isa.h"

// sw_context.cpp includes files that require these forward references

typedef class CONTEXT_SCHEDULER_CLASS* CONTEXT_SCHEDULER;
typedef class HW_CONTEXT_CLASS* HW_CONTEXT;

// ASIM other modules
#include "asim/provides/instfeeder_implementation.h"

// this include is for the asim_inst
#include "asim/provides/isa.h"

#define ASIM_SWC_NONE NULL

//#define ASIM_TPU_NONE      UINT32_MAX
//#define ASIM_CPU_NONE	    UINT32_MAX

/// States that occur as we are switching a SW_CONTEXT off of a
/// HW_CONTEXT. When a context switch has been completed (the operation
/// has been committed, there are no outstanding ASIM_INSTs, and the
/// scheduler has been notified), the state returns to NONE.


enum CONTEXT_SWITCH_STATE
{
    NONE,                  ///< No context switch requested or in progress
    REQUESTED,             ///< Scheduler has request context switch
    SENDING,               ///< FetchOperation has initiated context switch
    SENT,                  ///< Context switch has been sent to PM
    COMMITTED              ///< PM has committed context switch
};


//***************************************************************************

/**
 * @Brief Information about software context. Exists regardless of whether
 * the software context is currently executing on a hardware context
 * or not.
 *
 * This is "D" in my picture.
 * There will be at least two implementations of D:
 *  - One accepts calls related to instructions, such as Fetch(). This
 *    is used when we have a CPU in the timing model.
 *  - The other accepts requests for Swarm operations.
 *  - We could also have one for network packets.
 * For the first, there will be member functions like Fetch().
 * For the second, there will be member functions like FetchOperation().
 * The purpose of this code is to interpret Mingo events into
 *   instructions or Swarm operations, based on what timing model is
 *   running.
 * It also informs the scheduler when certain events occur in the Mingo
 *   streams.
 *
 */

typedef class SW_CONTEXT_CLASS *SW_CONTEXT;

class SW_CONTEXT_CLASS
{
  public:

    //constructor
    SW_CONTEXT_CLASS (UINT64 streamID, UINT64 pc, CONTEXT_SCHEDULER contextScheduler);
    
    //Destructor
    ~SW_CONTEXT_CLASS (void);
    
    
    //Accessors
    UINT32 GetUniqueID (void) const;
    UINT64 GetFeederStreamID(void) const;
    UINT64 GetResumptionPC (void) const;
    HW_CONTEXT GetHWC (void) const;
    bool GetMappedToHWC (void) const;
    bool GetRunnable (void) const;
    bool GetDeschedulePending (void) const;
    bool GetDeletePending (void) const;
    bool GetInTimedWait (void) const;
    UINT64 GetMinCycleToResume (void) const;
    
      
    //Modifiers

    void SetResumptionPC (
        UINT64 pc
        );                      ///< PC at which to resume
    void SetHWC(HW_CONTEXT);
    void SetNoHWC(void);
    void SetMappedToHWC(void);
    void SetNotMappedToHWC(void);
    void SetRunnable (void);
    void SetNotRunnable (void);
    void SetDeschedulePending (void);
    void SetNotDeschedulePending (void);
    void SetDeletePending (void);
    void SetInTimedWait(void);
    void SetNotInTimedWait(void);
    void SetMinCycleToResume (UINT64 cycle);
    
    // True iff events currently pending from feeder.  The call may block
    // for up to waituS microseconds waiting for an event.
    bool FeederHasEvents(UINT32 waituS = 0);

    // Other routines
    void DumpID();
    void DumpState();
    void InitiateContextSwitch();
    
    /// Return next operation
    ASIM_INST Fetch(IADDR_CLASS ip);


    /// Commit specified Swarm operation (timing model has finished
    /// with it)

    void Commit(
        ASIM_INST swarmOp         ///< Swarm operation to commit
        ); 
        
  private:

// Private data
// Keep data in same order as initializers in the constructor

    static UINT64 uniqueStaticID; ///< Static used to assign uniqueID
    const UINT64 uniqueID;        ///< A unique number assigned to each SWC
    UINT64 feederStreamID;        ///< Feeder's ID for this software context
    UINT64 resumption_PC; ///<where to start fetching when thread
                               ///<is scheduled or rescheduled
    HW_CONTEXT hwc;         ///< hardware context this sw context is mapped to
    bool mappedToHWC;       ///< Is this swc currently mapped to a HWC?
    bool runnable;       ///< Is it runnable from software point of view?
    bool deschedulePending; ///< Deschedule operation has begun
    bool deletePending;     ///< Delete was requested, and deschedule
                             ///<has begun
    bool inTimedWait;        ///< Waiting for completion of timed_wait
    UINT64 minCycleToResume; ///< Minimum cycle in which another
                             ///< request for an event can be sent to Mingo
                             ///< Valid only when inTimedWait is true
    ASIM_INST pendingOperation; ///< Next Java op to send to PM when
                               ///< preceding_inst goes to zero.
    UINT32 precedingInstCount; ///<Instructions preceding pending
                               ///< (counting down to zero)
    bool streamEnded;          ///< Mingo sent an End Thread
                               // May be redundant with deletePending
    bool receivedTimedWait;
    
    bool proceedNow;           ///< Signal to FetchOperation about
                               ///< whether to allow
                               ///< Mingo to proceed or wait for commit 
                               ///< of operation
    INT32 outstandingOpCount;  ///< Count of operation not committed yet.
    CONTEXT_SWITCH_STATE contextSwitchState; ///< State within a
                                ///<context switch

    const CONTEXT_SCHEDULER contextSchedulerHandle; ///< Pointer to the context scheduler
    
    
// Private methods

    /// Get an event from Mingo

    MINGO_DATA GetFeederEvent(void);    

    // Routines to handle each type of Mingo event

    void ProcessStreamEvent(
        MINGO_DATA event        ///< Event received from Mingo
        );
    void HandleMemoryRead(
        MINGO_DATA event        ///< Event received from Mingo
        );
    void HandleMemoryWrite(
        MINGO_DATA event        ///< Event received from Mingo
        );
    void HandleMemoryFence(
        MINGO_DATA event        ///< Event received from Mingo
        );
    void HandleExchange(
        MINGO_DATA event        ///< Event received from Mingo
        );
    void HandleDependentOp(
        MINGO_DATA event        ///< Event received from Mingo
        );
    void HandleBranch(
        MINGO_DATA event        ///< Event received from Mingo
        );
    void HandleWait(
        MINGO_DATA event        ///< Event received from Mingo
        );
    void HandleTimedWait(
        MINGO_DATA event        ///< Event received from Mingo
        );
    void HandleEndThread(
        MINGO_DATA event        ///< Event received from Mingo
        );
    void HandleSetPriority(
        MINGO_DATA event        ///< Event received from Mingo
        );
    void HandleSetPreferredCPU(
        MINGO_DATA event        ///< Event received from Mingo
        );
    void HandleUnexpectedEvent(
        MINGO_DATA event        ///< Event received from Mingo
        );
    

    /// Make operation for a context switch

    ASIM_INST PrepareContextSwitch(void);

    /// Make ASIM_INST for a real operation or an anonymous instruction

    ASIM_INST PrepareToSend(void);

    void DumpContextSwitchState();

    
}; //SW_CONTEXT_CLASS


#endif /* _SW_CONTEXT_CLASS_ */








