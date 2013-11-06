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
 * @author Sailashri Parthasarathy (based on Judy Hall's original implementation)
 * @brief HW_CONTEXT_CLASS - Null implementation
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

    inline UINT32 GetTPU (void) const { return 0; }
     
    inline UINT32 GetCPU (void) const { return 0; }

    SW_CONTEXT GetSWC (void) const { return SW_CONTEXT(); }
    UINT32 GetUID (void) const { return 0; };
    INT32 GetPriority() const { return 0; }
    UINT32 GetCacheOwnerId() const { return 0; }

    void SetWatchDogTimerOn(bool arg) { };
    /*
     * Modifiers
     */

    /// Set the running software context (the source of instructions
    /// or operations for the hardware context)

    inline void SetSWC (SW_CONTEXT swc, UINT64 cycle) { };

    /// Record that no software context is mapped to this hardware context

    inline void SetNoSWC (void) { };
    
    // Other routines

    void SetPriority(const INT32 priority) { };
    void SetHwcCacheOwnerId(const UINT32 cacheOwnerId) { };

    /// Dump the identification information for this hardware context

    // The cast to string operator - returns the ID
    operator string () { return NULL;}
    
    /// Dump State information for this hardware context.
    string IDToString() { return NULL; }
    string StateToString() { return NULL; }
    string TraceToString() { return NULL; }

    // Ask the feeder whether any warm-up data is available (called before
    // the main simulation).  Returns false when there is no more warm-up
    // information.
    void WarmUpClientInfo(const WARMUP_CLIENTS clientInfo) { };
    bool WarmUp(WARMUP_INFO warmup) { return false; }

    // Get the next operation for this hardware context.
    // Looks up software context that is running, and calls its
    // FetchOperation()

    ASIM_INST Fetch(UINT64 cycle, IADDR_CLASS ip) { return ASIM_INST(); }
    ASIM_MACRO_INST FetchMacro(UINT64 cycle, IADDR_CLASS ip) { return ASIM_MACRO_INST(); }
    ASIM_INST FetchMicro(UINT64 cycle,ASIM_MACRO_INST macro, IADDR_CLASS ip, bool do_rename=true) { return ASIM_INST(); }

    inline void Rename(UINT64 cycle, ASIM_INST ainst) { };
    inline void Issue(UINT64 cycle, ASIM_INST ainst) { };
    inline void Execute(ASIM_INST ainst) { };
    inline void Kill(UINT64 cycle, ASIM_INST ainst, UINT64 nKilled,
                     bool fetchNext, bool killMe) { };
    void Commit(UINT64 cycle, ASIM_INST ainst, UINT64 nCommitted) { };
    inline bool DoRead(ASIM_INST ainst) { return false; }
    inline bool CheckFault(ASIM_INST ainst) { return false; }    // pte_fix_hkim6
    inline bool DoSpecWrite(ASIM_INST ainst) { return false; }
    inline bool DoWrite(ASIM_INST ainst) { return false; }
    inline bool DTranslate(ASIM_INST ainst, UINT64 va, UINT64 &pa) {return false; }
    inline bool DTranslate(ASIM_INST ainst,
                           const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
                           UINT64& pa,
                           PAGE_TABLE_INFO pt_info,
                           MEMORY_VIRTUAL_REFERENCE_CLASS& vNextRegion) { return false; }
    inline bool ITranslate(UINT32 hwcNum, UINT64 va, UINT64 &pa) { return false; }
    inline bool ITranslate(const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
                           UINT64& pa,
                           PAGE_TABLE_INFO pt_info,
                           MEMORY_VIRTUAL_REFERENCE_CLASS& vNextRegion) { return false; }
    
    inline bool IsSystemEventTypePending(ASIM_INST ainst,FEEDER_SYSTEM_EVENT_TYPES type) { return false; }
    inline UINT64 GetFirstPC() { return 0; }
    inline bool GetArchRegisterValue(ARCH_REGISTER_TYPE rType,INT32 regNum, ARCH_REGISTER reg) { return false; }
    inline FEEDER_SYSTEM_EVENT_CLASS HandleSystemEvent(ASIM_INST inst,FEEDER_SYSTEM_EVENT_TYPES type) { return FEEDER_SYSTEM_EVENT_CLASS(); }
    void CheckLiveness(UINT64 cycle) { };

    IADDR_CLASS GetResumptionPC(void) const { return IADDR_CLASS(); }

  public:
    // These added to support the insertion/removal/alteration of instructions
    // directed from outside of the feeder.  The primary example are those 
    // instruction mutations directly by the timing model.  These instructions 
    // can be registered in the feeder so that they can be executed, committed, 
    // etc.  The caveat is that they should be manipulated before this or younger 
    // instructions have done Issue().   --slechta
    //
    // Insert ainst just before next_ainst.
    //
    inline void InstInsert(ASIM_INST ainst, ASIM_INST next_ainst) { };
    //
    // Remove ainst completely.  No longer needed.
    //
    inline void InstRemove(ASIM_INST ainst, ASIM_INST next_ainst) { };
    //
    // Inform feeder that the ainst has been modified, recalcute dependencies, etc.
    //
    inline void InstModify(ASIM_INST ainst) { };

}; //HW_CONTEXT_CLASS

#endif /* _HW_CONTEXT_ */
