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

#ifndef _HW_CONTEXT_
#define _HW_CONTEXT_

// Includes

// ASIM core
#include "asim/syntax.h"
#include "asim/state.h"
#include "asim/trace.h"

#include "asim/atomic.h"
#include "asim/provides/warmup_manager.h"

//
// SW and HW contexts have circular dependence unless this is here.
//
typedef class HW_CONTEXT_CLASS * HW_CONTEXT;

// Other files
#include "asim/provides/isa.h"
#include "asim/provides/software_context.h"
#include "asim/provides/iaddr.h"

// includes files that require these forward references
#define ASIM_HWC_NONE       NULL


//***********************************************************************

/**
 * @brief Hardware context. One of these per simulated
 * hardware context (CPU, ring interface, etc.), regardless of whether 
 * it is running a SWC or not.
 *
 * There should be a class that inherits from this, that exports an
 * interface to the timing model for requesting
 * instructions/operations. Each timing model will potentially provide
 * a unique inherited class.
 * 
 * NOTE: HW_CONTEXT, SW_CONTEXT, and the scheduler are a unit. A
 * feeder or a timing model can use them, or ASIM_THREAD, but not
 * both. ASIM_THREAD contains elements of both hardware and software
 * contexts, and it assumes that the mapping never changes.
 *
 */

class HW_CONTEXT_CLASS : public TRACEABLE_CLASS
{
  private:

    //The members must stay in this order, to match the order of the 
    //initialization in the constructor.

    // Retaining uid, because it has been used historically for
    // debugging. CPU could be used for
    // letting software choose a CPU to run on. TPU is there in case
    // we do SMT. (In theory it could be put into an inherited class
    // for a timing model that does SMT.)

    static UID_GEN32 uniqueHwCxtId;   ///< static ID to assign uniqueId
    UINT64 uid;       ///< Unique identifier for this hardware context
    const UINT32 cpu; ///< CPU that this HWC represent
    const UINT32 tpu; ///< TPU that this HWC represents
    SW_CONTEXT runningSWC; ///< SWC currently running (or ASIM_SWC_NONE)
    UINT64 lastCommitCycle;

    INT32 myPriority;  ///< Priority. Used when picking a hwc to use.

       
    bool watchDogTimerOn; //when we connect to a shared library for the cpu we
                          //need to turn off the watch dog timer 
    //
    // The following are used to find a context given its uid.  The data
    // and functions are static the mapping can be found without having
    // an instance of the class.
    //
    enum
    {
        maxHWCs = MAX_TOTAL_NUM_CPUS * NUM_HWCS_PER_CPU
    };

    //
    // Histogram storing information about simulated processes in the functional
    // model.
    //
    HISTOGRAM_TEMPLATE<ENABLE_PROCESS_HISTORY> processHistory;

    enum
    {
        procHistPid,
        procHistStartCycle,
        procHistFetched,
        procHistIssued,
        procHistKilled,
        procHistCommitted
    };

    //
    // Index of current process in the processHistory.
    //
    INT32 curProcessIdx;
    INT32 maxProcessIdx;

    //
    // Pid of current process in SWC
    //
    INT32 pid;

//    static HW_CONTEXT allHWCs[maxHWCs];

    //
    // Timer interrupt statistics from software context
    //
    UINT64 nTimerInterrupts;
    UINT64 timerInterruptCycles;
    UINT64 timerInterruptStartCycle;
    bool inTimerInterrupt;

    //
    // Spin loop statistics
    //
    UINT64 userSpinLoopTrips;
    UINT64 userSpinLoopCycles;
    UINT64 kernelSpinLoopTrips;
    UINT64 kernelSpinLoopCycles;
    UINT64 idleSpinLoopTrips;
    UINT64 idleSpinLoopCycles;

    UINT64 spinLoopStartCycle;
    UINT32 instrsSinceLastSpinTag;
    bool inSpinLoop;
    
    //
    // At warm-up time, cache owner id that will be set for this HWC
    //
    UINT32 hwcCacheOwnerId;

  public:

    // Constructor.

    HW_CONTEXT_CLASS (
        ASIM_REGISTRY reg,
        const UINT32 cpu,
        const UINT32 tpu,
        const INT32 priority = 0);

    ~HW_CONTEXT_CLASS ();

    /*
     * Accessors
     */

    inline UINT32 GetTPU (void) const;
    
    inline UINT32 GetCPU (void) const;

    SW_CONTEXT GetSWC (void) const { return(runningSWC); }
    UINT32 GetUID (void) const { return uid; };
    INT32 GetPriority() const { return myPriority; }
    UINT32 GetCacheOwnerId() const { return hwcCacheOwnerId; }

    void SetWatchDogTimerOn(bool arg) { watchDogTimerOn = arg;}
    /*
     * Modifiers
     */

    /// Set the running software context (the source of instructions
    /// or operations for the hardware context)

    inline void SetSWC (SW_CONTEXT swc, UINT64 cycle);

    /// Record that no software context is mapped to this hardware context

    inline void SetNoSWC (void);
    
    // Other routines

    void SetPriority(const INT32 priority);
    void SetHwcCacheOwnerId(const UINT32 cacheOwnerId) { hwcCacheOwnerId = cacheOwnerId; }

    /// Dump the identification information for this hardware context

    // The cast to string operator - returns the ID
    operator string ();
    
    /// Dump State information for this hardware context.
    string IDToString();
    string StateToString();
    string TraceToString();

    // Ask the feeder whether any warm-up data is available (called before
    // the main simulation).  Returns false when there is no more warm-up
    // information.
    void WarmUpClientInfo(const WARMUP_CLIENTS clientInfo);
    bool WarmUp(WARMUP_INFO warmup);

    // Get the next operation for this hardware context.
    // Looks up software context that is running, and calls its
    // FetchOperation()

    ASIM_INST Fetch(UINT64 cycle, IADDR_CLASS ip);
    ASIM_MACRO_INST FetchMacro(UINT64 cycle, IADDR_CLASS ip);
    ASIM_INST FetchMicro(UINT64 cycle,ASIM_MACRO_INST macro, IADDR_CLASS ip);

    inline void Issue(UINT64 cycle, ASIM_INST ainst);
    inline void Execute(ASIM_INST ainst);
    inline void Kill(UINT64 cycle, ASIM_INST ainst, UINT64 nKilled,
                     bool fetchNext, bool killMe);
    void Commit(UINT64 cycle, ASIM_INST ainst, UINT64 nCommitted);
    inline bool DoRead(ASIM_INST ainst);
    inline bool DoSpecWrite(ASIM_INST ainst);
    inline bool DoWrite(ASIM_INST ainst);
    inline bool DTranslate(ASIM_INST ainst, UINT64 va, UINT64 &pa);
    inline bool DTranslate(ASIM_INST ainst,
                           const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
                           UINT64& pa,
                           PAGE_TABLE_INFO pt_info,
                           MEMORY_VIRTUAL_REFERENCE_CLASS& vNextRegion);
    inline bool ITranslate(UINT32 hwcNum, UINT64 va, UINT64 &pa);
    inline bool ITranslate(const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
                           UINT64& pa,
                           PAGE_TABLE_INFO pt_info,
                           MEMORY_VIRTUAL_REFERENCE_CLASS& vNextRegion);

    inline bool IsSystemEventTypePending(ASIM_INST ainst,FEEDER_SYSTEM_EVENT_TYPES type);
    inline UINT64 GetFirstPC();
    inline bool GetArchRegisterValue(ARCH_REGISTER_TYPE rType,INT32 regNum, ARCH_REGISTER reg);
    inline FEEDER_SYSTEM_EVENT_CLASS HandleSystemEvent(ASIM_INST inst,FEEDER_SYSTEM_EVENT_TYPES type);
    void CheckLiveness(UINT64 cycle);

    IADDR_CLASS GetResumptionPC(void) const;
}; //HW_CONTEXT_CLASS

inline FEEDER_SYSTEM_EVENT_CLASS
HW_CONTEXT_CLASS::HandleSystemEvent(ASIM_INST inst,FEEDER_SYSTEM_EVENT_TYPES type)
{
    return GetSWC()->GetIFeeder()->HandleSystemEvent(inst,type);
}
inline UINT64
HW_CONTEXT_CLASS::GetFirstPC()
{
    if(GetSWC())
    {
        IFEEDER_STREAM_HANDLE stream = GetSWC()->GetFeederStreamHandle();
        return GetSWC()->GetIFeeder()->GetFirstPC(stream);
    }
    else
    {
        return 0;
    }
}
inline bool 
HW_CONTEXT_CLASS::GetArchRegisterValue(ARCH_REGISTER_TYPE rType,INT32 regNum, ARCH_REGISTER reg)
{

    if(GetSWC())
    {
        IFEEDER_STREAM_HANDLE stream = GetSWC()->GetFeederStreamHandle();
        GetSWC()->GetIFeeder()->GetArchRegisterValue(stream,rType,regNum,reg);
        return true;
    }
    else
    {
        return false;
    }
}

inline void
HW_CONTEXT_CLASS::Issue(
    UINT64 cycle,
    ASIM_INST ainst)
{
    if (curProcessIdx >= 0)
    {
        processHistory.AddEvent(curProcessIdx, procHistIssued, 1);
    }

    ainst->GetSWC()->Issue(ainst);
}

inline void
HW_CONTEXT_CLASS::Execute(
    ASIM_INST ainst)
{
    ainst->GetSWC()->Execute(ainst);
}

inline void
HW_CONTEXT_CLASS::Kill(
    UINT64 cycle,
    ASIM_INST ainst,
    UINT64 nKilled,
    bool fetchNext,
    bool killMe)
{

    if (curProcessIdx >= 0)
    {
        processHistory.AddEvent(curProcessIdx, procHistKilled, nKilled);
    }

    ainst->GetSWC()->Kill(ainst, fetchNext, killMe);
}
inline bool 
HW_CONTEXT_CLASS::IsSystemEventTypePending(ASIM_INST ainst,FEEDER_SYSTEM_EVENT_TYPES type)
{
    if (!GetSWC())
    {
        return false;
    }
    else
    {
        return GetSWC()->GetIFeeder()->IsSystemEventTypePending(ainst,type);
    }
}
inline bool
HW_CONTEXT_CLASS::DoRead(
    ASIM_INST ainst)
{
    ASSERTX(ainst->GetSWC());
    return ainst->GetSWC()->DoRead(ainst);
}

inline bool
HW_CONTEXT_CLASS::DoSpecWrite(
    ASIM_INST ainst)
{
    ASSERTX(ainst->GetSWC());
    return ainst->GetSWC()->DoSpecWrite(ainst);
}

inline bool
HW_CONTEXT_CLASS::DoWrite(
    ASIM_INST ainst)
{
    ASSERTX(ainst->GetSWC());
    return ainst->GetSWC()->DoWrite(ainst);
}

inline bool
HW_CONTEXT_CLASS::DTranslate(
    ASIM_INST ainst,
    UINT64 va,
    UINT64 &pa)
{
    ASSERTX(ainst->GetSWC());
    return ainst->GetSWC()->DTranslate(ainst, va, pa);
}

inline bool
HW_CONTEXT_CLASS::DTranslate(
    ASIM_INST ainst,
    const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
    UINT64& pa,
    PAGE_TABLE_INFO pt_info,
    MEMORY_VIRTUAL_REFERENCE_CLASS& vNextRegion)
{
    ASSERTX(ainst->GetSWC());
    ASSERTX(&vRegion != &vNextRegion);
    return ainst->GetSWC()->DTranslate(ainst, vRegion, pa, pt_info, vNextRegion);
}

inline bool
HW_CONTEXT_CLASS::ITranslate(
    UINT32 hwcNum,
    UINT64 va,
    UINT64 &pa)
{
    ASSERTX(GetSWC());
    return GetSWC()->ITranslate(hwcNum, va, pa);
}

inline bool
HW_CONTEXT_CLASS::ITranslate(
    const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
    UINT64& pa,
    PAGE_TABLE_INFO pt_info,
    MEMORY_VIRTUAL_REFERENCE_CLASS& vNextRegion)
{
    ASSERTX(GetSWC());
    ASSERTX(&vRegion != &vNextRegion);
    return GetSWC()->ITranslate(vRegion, pa, pt_info, vNextRegion);
}

/* ------------------------ Accessors ----------------------------*/

inline UINT32
HW_CONTEXT_CLASS::GetTPU (void) const
{
    // This function doesn't work, should be removed
    //ASSERTX(0); 
    return(tpu); 
}
   

inline UINT32
HW_CONTEXT_CLASS::GetCPU (void) const
{   
    // This function doesn't work, should be removed
    ASSERTX(0);
    return(cpu); 
}   
    

/* ------------------------- Modifiers --------------------------------*/


inline void 
HW_CONTEXT_CLASS::SetSWC (SW_CONTEXT sw, UINT64 cycle) 
{
    runningSWC = sw;
    lastCommitCycle = cycle;
}   

    // Set SWC to null when there is nothing running, so the hardware
    // context will appear free when a software context needs to run.

inline void
HW_CONTEXT_CLASS::SetNoSWC (void)
{
    runningSWC = ASIM_SWC_NONE;
}


#endif /* _HW_CONTEXT_ */
