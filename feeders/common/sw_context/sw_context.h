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
 * @brief Software context (to be scheduled on a hardware context)
 */


#ifndef _SW_CONTEXT_CLASS_
#define _SW_CONTEXT_CLASS_

// generic
#include <iostream>
#include <map>

//
// SW and HW contexts have circular dependence unless this is here.
//
typedef class SW_CONTEXT_CLASS *SW_CONTEXT;

// ASIM core
#include "asim/syntax.h"
#include "asim/mm.h"
#include "asim/module.h"
#include "asim/atomic.h"

// ASIM public modules
#include "asim/provides/iaddr.h"
#include "asim/provides/isa.h"

//
// This undefine is very bad, but the synthesized header file has a ifdef that I
// can't remove to make sure we always define things in the same order.
//
#undef _ASIM_INSTFEEDER_INTERFACE_
#include "asim/provides/instfeeder_interface.h"
//#include "asim/provides/hardware_context.h"


// File that provides instruction stats. 
#include "asim/provides/inst_stats.h"

// sw_context.cpp includes files that require these forward references

typedef class CONTEXT_SCHEDULER_CLASS* CONTEXT_SCHEDULER;
typedef class HW_CONTEXT_CLASS* HW_CONTEXT;

//moved to isa
//typedef class mmptr<class MICRO_INST_CLASS> ASIM_INST;

#define ASIM_SWC_NONE NULL

/// States that occur as we are switching a SW_CONTEXT off of a
/// HW_CONTEXT. When a context switch has been completed (the operation
/// has been committed, there are no outstanding ASIM_INSTs, and the
/// scheduler has been notified), the state returns to NONE.


// FIX: we don't use SENDING or COMMITTED right now.
enum CONTEXT_SWITCH_STATE
{
    CONTEXT_SWITCH_NONE,                  ///< No context switch requested or in progress
    CONTEXT_SWITCH_REQUESTED,             ///< Scheduler has request context switch
    CONTEXT_SWITCH_SENDING,               ///< FetchOperation has initiated context switch
    CONTEXT_SWITCH_SENT,                  ///< Context switch has been sent to PM
    CONTEXT_SWITCH_COMMITTED              ///< PM has committed context switch
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
 *
 */

class SW_CONTEXT_CLASS : ASIM_REGISTRY_CLASS, public TRACEABLE_CLASS
{
  private:

// Private data
// Keep data in same order as initializers in the constructor

    const IFEEDER_BASE iFeeder;         ///< Instruction feeder
    static UID_GEN32 uniqueStaticID;    ///< Static used to assign uniqueID
    const UINT32 uniqueID;              ///< A unique number assigned to each SWC
    IFEEDER_STREAM_HANDLE feederStreamHandle;  ///< Feeder's handle for this
                                               ///  software context
    IADDR_CLASS resumptionPC;   ///<where to start fetching when thread
                                ///<is scheduled or rescheduled
    HW_CONTEXT hwc;             ///< hardware context this sw context is mapped to

    CONTEXT_SWITCH_STATE contextSwitchState; ///< State within a
                                             ///<context switch

    CONTEXT_SCHEDULER    contextSchedulerHandle;    ///< Pointer to the context
                                                    ///scheduler

    // Perinst cache data structure
    PERINST_CACHE_CLASS statCache;  /// Data structure that holds perinst stats.

    INT32 pid;
    char procName[32];

    bool inTimerInterrupt;  // Feeder claims we are processing a timer interrupt

    bool mappedToHWC;       ///< Is this swc currently mapped to a HWC?
    bool runnable;          ///< Is it runnable from software point of view?
    bool deschedulePending; ///< Deschedule operation has begun
    bool deletePending;     ///< Delete was requested, and deschedule
                            ///<  has begun

    bool streamEnded;       ///< no more instr. from the feeder for this stream
    bool running;           ///< this indicates if the HWC has performed
                            ///<  the first fetch for this SWC or not and
                            ///<  is used to steer the feeder to the right ip

  private:
    //
    // Fetch, Issue, Commit and Kill are private because they are to be
    // called only by the HW_CONTEXT_CLASS, a friend of this class.
    //
    // Old non-split macro micro call. Used by macro only feeders
    ASIM_INST Fetch(UINT64 cycle, IADDR_CLASS ip);

    
    ASIM_MACRO_INST FetchMacro(UINT64 cycle, IADDR_CLASS ip);
    ASIM_INST FetchMicro(UINT64 cycle,ASIM_MACRO_INST macro, IADDR_CLASS ip);


    inline void Issue(ASIM_INST ainst);
    inline void Execute(ASIM_INST ainst);
    void Commit(ASIM_INST ainst);
    inline void Kill(ASIM_INST ainst, bool fetchNext, bool killMe);
    inline bool DoRead(ASIM_INST ainst);
    inline bool DoSpecWrite(ASIM_INST ainst);
    inline bool DoWrite(ASIM_INST ainst);
    
    void WarmUpClientInfo(const WARMUP_CLIENTS clientInfo);
    bool WarmUp(WARMUP_INFO warmup);

  private:
    /// Make operation for a context switch
    string ContextSwitchStateToString();

  public:
    friend class HW_CONTEXT_CLASS;

    //constructor
    SW_CONTEXT_CLASS(
        IFEEDER_BASE ifeeder,
        IFEEDER_STREAM_HANDLE iStream,
        IADDR_CLASS pc,
        CONTEXT_SCHEDULER contextScheduler);

    //constructor
    SW_CONTEXT_CLASS(
        IFEEDER_BASE iFeeder,
        IFEEDER_STREAM_HANDLE sHandle = NULL) :
        iFeeder(iFeeder),
        uniqueID(uniqueStaticID++),
        feederStreamHandle(sHandle),
        pid(-1)
    {
        strcpy(procName, "<unknown>");
    }
    
    //Destructor
    ~SW_CONTEXT_CLASS (void);
    
    //Accessors
    IFEEDER_BASE GetIFeeder(void) const;
    IFEEDER_STREAM_HANDLE GetFeederStreamHandle(void) const;
    UINT32 GetUniqueID (void) const;
    IADDR_CLASS GetResumptionPC (void) const;
    HW_CONTEXT GetHWC (void) const;

    // This is sort of a hack, but excpetions.h uses this so it doesn't have to
    // include hardware_context.h to get the hwc num for stats collection
    // purposes.
    // return -1 if it's not mapped to a HWC
    INT32 GetHWCNum (void) const;

    bool GetMappedToHWC (void) const;
    bool GetRunnable (void) const;
    bool GetDeschedulePending (void) const;
    bool GetDeletePending (void) const;
      
    static UINT32 NActiveContexts(void) { return uniqueStaticID; };

    //Modifiers

    void SetResumptionPC (IADDR_CLASS pc);                  ///< PC at which to resume
    void SetHWC(HW_CONTEXT);
    void SetNoHWC(void);
    void SetMappedToHWC(void);
    void SetNotMappedToHWC(void);
    void SetRunnable (void);
    void SetNotRunnable (void);
    void SetDeschedulePending (void);
    void SetNotDeschedulePending (void);
    void SetDeletePending (void);

    void HandleEndThread();         // not used right now

    // Other routines
    string IDToString();
    // The cast to string operator
    string StateToString();

    void InitiateContextSwitch();
    
    //
    // Process descriptors.  These calls pass descriptions of the active
    // process from the feeder to the process history maintained in
    // the hardware context.
    //
    void SetPid(INT32 newPid) { pid = newPid; };
    INT32 GetPid(void) const { return pid; };
    
    void SetProcessName(const char *newProcName)
    {
        strncpy(procName, newProcName, sizeof(procName));
        procName[sizeof(procName)-1] = 0;
        SetTraceableName(procName);
    };
    const char *GetProcessName(void) const { return procName; };

    //
    // Timer interrupt state.  Feeders can tell the hardware context
    // when the instruction stream is processing a timer interrupt.
    //
    void TimerInterruptEnter(void) { inTimerInterrupt = true; };
    void TimerInterruptExit(void) { inTimerInterrupt = false; };
    bool InTimerInterrupt(void) const { return inTimerInterrupt; };
    
    //
    // Virtual to physical translation
    //
    inline bool DTranslate(ASIM_INST ainst, UINT64 va, UINT64 &pa);
    inline bool DTranslate(ASIM_INST inst,
                           const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
                           UINT64& pa,
                           PAGE_TABLE_INFO pt_info,
                           MEMORY_VIRTUAL_REFERENCE_CLASS& vNextRegion);
    inline bool ITranslate(UINT32 hwcNum, UINT64 va, UINT64 &pa);
    inline bool ITranslate(const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
                           UINT64& pa,
                           PAGE_TABLE_INFO pt_info,
                           MEMORY_VIRTUAL_REFERENCE_CLASS& vNextRegion);

    inline UINT64 GetFirstPC();
    // Routine to access perinst stats infrastructure. 
    PERINST_CACHE_CLASS& GetPerinstCache();   /// Perinst data cache. 

    // Overloaded routine to dump stats.  This first registers the stats sitting
    // in the perinst_cache. 
    void DumpStats(STATE_OUT stateOut);


}; //SW_CONTEXT_CLASS
/***********************************************************************************
 *
 * Include of the hardware context moved until after the class definition so
 * that the software context can have inlined functions 
 *
 *************************************************************************************/
#include "asim/provides/hardware_context.h"

inline UINT64 
SW_CONTEXT_CLASS::GetFirstPC()
{
    return iFeeder->GetFirstPC(feederStreamHandle);
}


inline PERINST_CACHE_CLASS&
SW_CONTEXT_CLASS::GetPerinstCache()
{
    return statCache;
}

/**
 * read the memory location for this instruction
 */
inline bool
SW_CONTEXT_CLASS::DoRead(
    ASIM_INST ainst)
{
    return iFeeder->DoRead(ainst);
}

/**
 * speculatively write the memory location for this instruction
 */
inline bool
SW_CONTEXT_CLASS::DoSpecWrite(
    ASIM_INST ainst)
{
    return iFeeder->DoSpecWrite(ainst);
}

/**
 * write the memory location for this instruction
 */
inline bool
SW_CONTEXT_CLASS::DoWrite(
    ASIM_INST ainst)
{
    return iFeeder->DoWrite(ainst);
}

/**
 * translate the va to a pa for this instruction
 */
inline bool
SW_CONTEXT_CLASS::DTranslate(
    ASIM_INST ainst,
    UINT64 va,
    UINT64 &pa)
{
    return iFeeder->DTranslate(ainst, va, pa);
}

inline bool
SW_CONTEXT_CLASS::DTranslate(
    ASIM_INST ainst,
    const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
    UINT64& pa,
    PAGE_TABLE_INFO pt_info,
    MEMORY_VIRTUAL_REFERENCE_CLASS& vNextRegion)
{
    return iFeeder->DTranslate(ainst, vRegion, pa, pt_info, vNextRegion);
}

/**
 * translate the va to a pa
 */
inline bool
SW_CONTEXT_CLASS::ITranslate(
    UINT32 hwcNum,
    UINT64 va,
    UINT64 &pa)
{
    return iFeeder->ITranslate(GetFeederStreamHandle(), hwcNum, va, pa);
}

inline bool
SW_CONTEXT_CLASS::ITranslate(
    const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
    UINT64& pa,
    PAGE_TABLE_INFO pt_info,
    MEMORY_VIRTUAL_REFERENCE_CLASS& vNextRegion)
{
    return iFeeder->ITranslate(GetFeederStreamHandle(), vRegion, pa, pt_info, vNextRegion);
}

/**
 * Issue an instruction for this software context.

 */

void
SW_CONTEXT_CLASS::Issue(
    ASIM_INST ainst)
{
    iFeeder->Issue(ainst);
}
/**
 * Execute an instruction for this software context.
 */

void
SW_CONTEXT_CLASS::Execute(
    ASIM_INST ainst)
{
    iFeeder->Execute(ainst);
}
/**
 * Kill an instruction for this software context.

 */

inline void
SW_CONTEXT_CLASS::Kill(
    ASIM_INST ainst,
    bool fetchNext,
    bool killMe)
{
    iFeeder->Kill(ainst, fetchNext, killMe);
}


#endif /* _SW_CONTEXT_CLASS_ */
