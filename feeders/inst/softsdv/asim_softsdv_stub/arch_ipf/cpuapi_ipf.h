/**************************************************************************
 * Copyright (C) 2004-2006 Intel Corporation
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
 **************************************************************************/

/**
 * @file cpuapi_ipf.h
 * @author Michael Adler
 * @brief IPF specific code for SoftSDV stub
 */

#ifndef _CPUAPI_IPF_H
#define _CPUAPI_IPF_H

#include "asim/dynamic_array.h"

//
// SoftSDV ISA-specific include files
//
#include "cpuapi_arch_ipf.h"
#include "disem.h"

#define ASIM_CPUAPI_ISA CPUAPI_IPF

extern cpuapi_controller_callbacks_t *asimCpucCallBack;
extern SOFTSDV_IO_SOFTSDV_SIDE_CLASS asim_io;

// v20040608_0 and later changed the name to CPUAPI_IPF...
#ifndef CPUAPI_SIM_INST_BYTES_LENGTH
#define CPUAPI_SIM_INST_BYTES_LENGTH CPUAPI_IPF_INST_BYTES_LENGTH
#endif


//
// Setup ISA specific callbacks
//
cpuapi_ipf_arch_specific_callbacks_t *
CPUAPI_ArchSpecificClbks(void);


//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// CPU_INFO_CLASS has some generic ISA neutral functions available to all
// of the CPUAPI code and lots of state specific to the ISA.
// 
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

class CPU_INFO_CLASS
{
  public:
    //
    // Constructors sets everything to NULL because not all the information
    // stored here can be known at once.  In particular, controllerPid is
    // assigned when the module is loaded, but handles can not be assigned
    // that early.
    //
    CPU_INFO_CLASS(void) :
        cpuNum(0),
        controllerPid(0),
        simulationMode(CPUAPI_Simul_Mode_Cold),
        stateValid(4),
        stateWarmup(false),
        forceContextSwitch(false),
        view_cpu_handle(0),
        cfm_handle(0),
        cpuMode_handle(0),
        ip_handle(0),
        predicates_handle(0),
        simAliveCpus_handle(0),
        simInstInfo_handle(0),
        simLinearXip_handle(0),
        simLitCount_handle(0)
    {};

    ~CPU_INFO_CLASS() {};
    
    //------------------------------------------------------------------------
    // ISA neutral declarations.  These must be present in any CPU_INFO_CLASS.
    //------------------------------------------------------------------------

    cpuapi_pid_t ControllerPid(void) const { return controllerPid; };

    bool ReadMemory(UINT64 pa, cpuapi_size_t size, void *buf);
    bool VirtualToPhysical(
        UINT64 va,
        UINT64 *pa,
        cpuapi_access_type_t access = CPUAPI_Access_Execute);

    //
    // Compute next IP from completed current instruction.  Doesn't know
    // about interrupts or rfi.
    //
    void SetControllerPid(
        int num,
        cpuapi_pid_t pid
    )
    {
        cpuNum = num;
        controllerPid = pid;
    };

    void InitHandles(void);

    //
    // Called when SoftSDV changes the simulation mode between functional
    // and performance modes.
    //
    void SetSimulationMode(cpuapi_simulmode_t mode);

    //
    // The stateValid counter indicates whether the CPU has been running in
    // performance mode and the cached register values remain valid.  The
    // state is invalidated if SoftSDV switches to functional mode.  It is
    // a counter instead of a boolean because SoftSDV seems to have a bug
    // in which the first few calls to step_reference() return garbage.
    // Using the counter lets us recover slowly from switching modes and
    // we skip the first few results.
    //
    bool StateValid(void) const { return stateValid == 0; };
    void SetStateInvalid(void) { stateValid = 4; };
    void SetStateValid(void)
    {
        if (stateValid != 0) stateValid -= 1;
    };

    //
    // Warm-up mode is the same as normal operation in SoftSDV except that
    // a warm-up flag is set on every instruction sent to Asim.
    //
    bool StateWarmup(void) const { return stateWarmup; };
    void SetStateWarmup(void)
    {
        SetStateInvalid();
        stateWarmup = true;
    };
    void ClearStateWarmup(void)
    {
        SetStateInvalid();
        stateWarmup = false;
    };

    //
    // forceContextSwitch forces the code to pass the pid of the current
    // context along with the next instruction.  It is triggered by the
    // start of new valid state.
    //
    bool ForceContextSwitch(void) const { return forceContextSwitch; };
    void SetForceContextSwitch(void) { forceContextSwitch = true; };
    void ClearForceContextSwitch(void) { forceContextSwitch = false; };

    // Enable/disable returning of memory values for instrs executed by SoftSDV
    void CollectMemoryValues(bool collectValues) {};

    // Set active CPU in SoftSDV's visibility interface
    void SetViewCPU(void);

    //------------------------------------------------------------------------
    // ISA specific decarations.  Not used by cpuapi_interface.cpp.
    //------------------------------------------------------------------------

    UINT64 BSP(void);
    UINT64 CFM(void);
    UINT64 IP(void);
    UINT64 Predicates(void);
    UINT64 PSR(void);

    //
    // Numbered registers
    //
    UINT64 RegGR(UINT32 n);
    void RegFR(UINT32 n, UINT64 value[2]);
    bool RegPR(UINT32 n);
    UINT64 RegAR(UINT32 n);
    UINT64 RegBR(UINT32 n);
    UINT64 RegCR(UINT32 n);

  private:
    int cpuNum;
    cpuapi_pid_t controllerPid;
    cpuapi_simulmode_t simulationMode;

    int stateValid;
    bool stateWarmup;

    bool forceContextSwitch;

    //
    // Handles to Gambit state
    //
    OML_handle_t view_cpu_handle;
    cpuapi_handle_t cfm_handle;
    cpuapi_handle_t cpuMode_handle;
    cpuapi_handle_t ip_handle;
    cpuapi_handle_t predicates_handle;
    cpuapi_handle_t psr_handle;
    cpuapi_handle_t simAliveCpus_handle;
    cpuapi_handle_t simInstInfo_handle;
    cpuapi_handle_t simLinearXip_handle;
    cpuapi_handle_t simLitCount_handle;

    cpuapi_handle_t regsGR_handle[CPUAPI_IPF_GR_NUM];
    cpuapi_handle_t regsFR_handle[CPUAPI_IPF_FR_NUM];
    cpuapi_handle_t regsAR_handle[CPUAPI_IPF_AR_NUM];
    cpuapi_handle_t regsBR_handle[CPUAPI_IPF_BR_NUM];
    cpuapi_handle_t regsCR_handle[CPUAPI_IPF_CR_NUM];
};

typedef CPU_INFO_CLASS *CPU_INFO;

//
// State for each simulated CPU is global.
//
extern DYNAMIC_ARRAY_CLASS<CPU_INFO_CLASS> cpuInfo;


class CPU_DMA_INVALIDATE_CLASS
{
  private:
    enum
    {
        CHUNK_SIZE = 512,
        MAX_LEN = 0x1000000             // 8GB / CHUNK_SIZE
    };

    UINT8 memidx[MAX_LEN];

  public:
    CPU_DMA_INVALIDATE_CLASS(void)
    {
        bzero(memidx, sizeof(memidx));
    }

    ~CPU_DMA_INVALIDATE_CLASS() {};

    void InvalidateAddr(UINT64 addr, UINT64 size)
    {
        UINT64 nChunks = (size + CHUNK_SIZE - 1) / CHUNK_SIZE;
        UINT64 idx = addr / CHUNK_SIZE;
        while (nChunks--)
        {
            if (idx < MAX_LEN)
            {
                memidx[idx] += 1;
            }
            idx += 1;
        }
    };

    UINT32 GetMemIndex(UINT64 addr) const
    {
        UINT64 idx = addr / CHUNK_SIZE;
        return (idx < MAX_LEN) ? memidx[idx] : 0;
    };

    UINT64 GetMaskedMem(UINT64 addr) const
    {
        return addr ^ (UINT64(GetMemIndex(addr)) << 36);
    };
};

typedef CPU_DMA_INVALIDATE_CLASS *CPU_DMA_INVALIDATE;

extern CPU_DMA_INVALIDATE dmaIdx;

#endif // _CPUAPI_IPF_H
