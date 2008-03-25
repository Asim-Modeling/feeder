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
 * @file cpuapi_interface.h
 * @author Michael Adler
 * @brief SoftSDV stub interfaces
 */

#ifndef _CPUAPI_INTERFACE_H
#define _CPUAPI_INTERFACE_H

//
// SoftSDV include files
//
#include "cpuapi.h"
#include "cpuc_odb.h"
#include "oml_api.h"

//------------------------------------------------------------------------
//
// Guest OS Information
//
//------------------------------------------------------------------------

//
// task_struct is a kernel data structure with the pid and other details.
// The current task_struct is found in different ways depending on the
// kernel and architecture.
//
enum ASIM_FIND_TASK_STRUCT
{
    TASK_STRUCT_UNKNOWN,
    TASK_STRUCT_RSP,            // task_struct is at RSP & ~8191
    TASK_STRUCT_RSP_INDIRECT,   // task_struct is at *(RSP & ~8191)
    TASK_STRUCT_GS              // task_struct at *GS (IA32e)
};

class ASIM_GUEST_OS_INFO_CLASS
{
  public:
    ASIM_GUEST_OS_INFO_CLASS() {};
    ~ASIM_GUEST_OS_INFO_CLASS() {};

    // Asim's name for the guest OS
    typedef const char * CONST_CHARP;
    PUBLIC_OBJ(CONST_CHARP, OsName);

    // Offset of field "pid" in Linux task_struct
    PUBLIC_OBJ(UINT32, PidOffset);

    // Offset of field "comm" (process name) in Linux task_struct
    PUBLIC_OBJ(UINT32, ProcNameOffset);

    // PDA array base addr (x86_64 Linux array of processor specific data)
    PUBLIC_OBJ(UINT64, PDAArrayAddr);

    // IP after instruction that updates PDA pcurrent __switch_to
    // Executing this IP indicates a context switch on Linux x86_64.
    PUBLIC_OBJ(UINT64, PDAContextSwitchIP);

    // Timer interrupt vector
    PUBLIC_OBJ(UINT64, TimerInterruptVector);

    // Local timer interrupt vector (SMP)
    PUBLIC_OBJ(UINT64, LocalTimerInterruptVector);

    // Idle loop tag -- address of an instruction in the kernel idle loop
    PUBLIC_OBJ(UINT64, IdleAddr);

    // Method for finding the task
    PUBLIC_OBJ(ASIM_FIND_TASK_STRUCT, FindTaskMethod);
};

extern ASIM_GUEST_OS_INFO_CLASS asimGuestOSInfo;


//------------------------------------------------------------------------
//
// Register descriptors for register values requested by Asim
//
//------------------------------------------------------------------------

class ASIM_REQUESTED_REGS_CLASS
{
  public:
    ASIM_REQUESTED_REGS_CLASS() :
        nRegs(0)
    {};

    ~ASIM_REQUESTED_REGS_CLASS() {};

    // Add a register to the set of monitored registers
    //  *** Makes a copy of name in case the original string is destroyed
    void AddRegister(UINT32 regNum, const char *name, UINT32 regSize);

    // How many registers are monitored?
    UINT32 NRegs(void) const { return nRegs; };

    // Get properties of registers
    UINT32 GetAsimRegNum(UINT32 regIdx) const;
    const char *GetRegName(UINT32 regIdx) const;
    UINT32 GetRegSize(UINT32 regIdx) const;
    cpuapi_handle_t GetRegHandle(UINT32 regIdx) const;

    // Set handle
    void SetRegHandle(UINT32 regIdx, cpuapi_handle_t handle);

  private:
    UINT32 nRegs;

    struct
    {
        const char *regName;
        cpuapi_handle_t regHandle;
        UINT32 regNum;
        UINT32 regSize;
    }
    regInfo[MAX_SOFTSDV_REGS];
};


inline void
ASIM_REQUESTED_REGS_CLASS::AddRegister(
    UINT32 regNum,
    const char *name,
    UINT32 regSize)
{
    ASSERTX(nRegs < MAX_SOFTSDV_REGS);

    char *newName = (char *)malloc(strlen(name) + 1);
    strcpy(newName, name);
    regInfo[nRegs].regName = newName;

    regInfo[nRegs].regNum = regNum;
    regInfo[nRegs].regSize = regSize;
    regInfo[nRegs].regHandle = 0;

    nRegs += 1;
};


inline UINT32
ASIM_REQUESTED_REGS_CLASS::GetAsimRegNum(UINT32 regIdx) const
{
    ASSERTX(regIdx < nRegs);
    return regInfo[regIdx].regNum;
};


inline const char *
ASIM_REQUESTED_REGS_CLASS::GetRegName(UINT32 regIdx) const
{
    ASSERTX(regIdx < nRegs);
    return regInfo[regIdx].regName;
};


inline UINT32
ASIM_REQUESTED_REGS_CLASS::GetRegSize(UINT32 regIdx) const
{
    ASSERTX(regIdx < nRegs);
    return regInfo[regIdx].regSize;
};


inline OML_handle_t
ASIM_REQUESTED_REGS_CLASS::GetRegHandle(UINT32 regIdx) const
{
    ASSERTX(regIdx < nRegs);
    return regInfo[regIdx].regHandle;
};


inline void
ASIM_REQUESTED_REGS_CLASS::SetRegHandle(UINT32 regIdx, cpuapi_handle_t handle)
{
    ASSERTX(regIdx < nRegs);
    regInfo[regIdx].regHandle = handle;
};


//
// ISA-specific include file needs ASIM_REQUESTED_REGS_CLASS.
//

#include "asim/provides/softsdv_cpuapi_isa.h"

//------------------------------------------------------------------------
//
// Functions that must be available to the generic CPUAPI code.
// 
//------------------------------------------------------------------------

//
// Execute one instruction callback
//
extern cpuapi_stat_t
ExecuteAsim(cpuapi_cid_t cid,
            cpuapi_step_t step_type,
            cpuapi_u64_t requested,
            cpuapi_u64_t *actual);

//
// Early initialization
//
extern void
AsimInit_ArchSpecific(void);


//
// ISA specific initialization
//
extern void
AsimInitCpu_ArchSpecific(
    UINT32 cpuNum,
    const ASIM_REQUESTED_REGS_CLASS &regRqst);


extern void
AsimStartLockStep(void);

extern void
AsimStopLockStep(void);

extern void
AsimLockStepDMA(UINT64 addr, UINT64 size);

#endif // _CPUAPI_INTERFACE_H
