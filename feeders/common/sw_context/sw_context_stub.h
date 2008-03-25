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
 * @author Eric Borch
 * @brief Software context stub just to make ape model work
 */


#ifndef _SW_CONTEXT_CLASS_
#define _SW_CONTEXT_CLASS_


// ASIM core
#include "asim/syntax.h"
#include "asim/mm.h"
#include "asim/atomic.h"

//
// SW and HW contexts have circular dependence unless this is here.
//
typedef class SW_CONTEXT_CLASS *SW_CONTEXT;

// ASIM public modules
#include "asim/provides/isa.h"
#include "asim/provides/iaddr.h"
#include "asim/module.h"
#include "asim/provides/instfeeder_interface.h"
#include "asim/provides/hardware_context.h"
//#include "asim/provides/FeederPeekInfo.h"

//we include isa already so we should have this there
//typedef class mmptr<class MICRO_INST_CLASS> ASIM_INST;

// sw_context.cpp includes files that require these forward references

typedef class CONTEXT_SCHEDULER_CLASS* CONTEXT_SCHEDULER;
typedef class HW_CONTEXT_CLASS* HW_CONTEXT;


#define ASIM_SWC_NONE NULL

class SW_CONTEXT_CLASS
{
  public:
    friend class HW_CONTEXT_CLASS;

    //constructor
    SW_CONTEXT_CLASS(
        IFEEDER_BASE iFeeder,
        IFEEDER_STREAM_HANDLE sHandle = NULL) :
        iFeeder(iFeeder),
        sHandle(sHandle),
        pid(-1),
        uniqueID(uniqueStaticID++)
    {
        strcpy(procName, "<unknown>");
    }

    //Destructor
    ~SW_CONTEXT_CLASS() {}

    //Accessors
    IFEEDER_STREAM_HANDLE GetFeederStreamHandle(void) const { return sHandle; }

    void SetHWC(HW_CONTEXT hw_context) { hwc = hw_context; }
    INT32 GetHWCNum(void);

    UINT32 GetUniqueID(void) const { return uniqueID; }

    IFEEDER_BASE GetIFeeder(void) const { return iFeeder; }

    void DumpID() {};
    static UINT32 NActiveContexts(void) { return uniqueStaticID; };

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
    };
    const char *GetProcessName(void) const { return procName; };
    bool InTimerInterrupt() { return false;};

    void WarmUpClientInfo(const WARMUP_CLIENTS clientInfo);
    bool WarmUp(WARMUP_INFO warmup);

    //
    // Virtual to physical translation
    //
    bool DTranslate(ASIM_INST ainst, UINT64 va, UINT64 &pa);
    bool DTranslate(ASIM_INST inst,
                    const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
                    UINT64& pa,
                    PAGE_TABLE_INFO pt_info,
                    MEMORY_VIRTUAL_REFERENCE_CLASS& vNextRegion);
    bool ITranslate(UINT32 hwcNum, UINT64 va, UINT64 &pa);
    bool ITranslate(const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
                    UINT64& pa,
                    PAGE_TABLE_INFO pt_info,
                    MEMORY_VIRTUAL_REFERENCE_CLASS& vNextRegion);

  private:
    //
    // Fetch, Issue, Commit and Kill are private because they are to be
    // called only by the HW_CONTEXT_CLASS, a friend of this class.
    //
    ASIM_INST Fetch(UINT64 cycle, IADDR_CLASS ip);
    void Issue(ASIM_INST ainst);
    void Execute(ASIM_INST ainst);
    void Commit(ASIM_INST ainst);
    void Kill(ASIM_INST ainst, bool fetchNext, bool killMe);

    // these must be here to work with hw_context.h
    //FEEDER_PEEK_INFO Peek(IADDR_CLASS pc);
    bool DoRead(ASIM_INST ainst);
    bool DoSpecWrite(ASIM_INST ainst);
    bool DoWrite(ASIM_INST ainst);

  private:
    IFEEDER_BASE iFeeder;
    IFEEDER_STREAM_HANDLE sHandle;
    HW_CONTEXT hwc;

    INT32 pid;
    char procName[32];

    UINT32 uniqueID;
    static UID_GEN32 uniqueStaticID;
}; //SW_CONTEXT_CLASS


#endif /* _SW_CONTEXT_CLASS_ */

