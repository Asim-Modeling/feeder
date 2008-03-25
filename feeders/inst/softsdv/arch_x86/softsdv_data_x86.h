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
 * @file
 * @author Michael Adler
 * @brief SoftSDV x86 specific data passed between SoftSDV and Asim.
 */

#ifndef _SOFTSDV_DATA_X86_H
#define _SOFTSDV_DATA_X86_H

#include <string.h>

#include "asim/arch_register.h"
#include "asim/provides/cpu_mode.h"

// Base softsdv_data.h must be included first
#include "asim/restricted/softsdv_data.h"

//-------------------------------------------------------------------------
//
// Structures here must have the same layout on both the SoftSDV and Asim
// sides.  They may be compiled in different modes (32 vs 64 bit).  Force
// the structure layout to be the same in each.
//
//-------------------------------------------------------------------------
#pragma pack(push,4)


//-------------------------------------------------------------------------
//
// Instruction descriptor.
//
//-------------------------------------------------------------------------

//
// Describe loads and stores using the generic reference class
//

enum
{
    ASIM_SOFTSDV_X86_MAX_LOADS = 4,
    ASIM_SOFTSDV_X86_MAX_STORES = 1,
    ASIM_SOFTSDV_X86_VALUE_BUF_LEN = 640
};

typedef ASIM_SOFTSDV_MEM_ACCESS_CLASS *ASIM_SOFTSDV_MEM_ACCESS;

typedef ASIM_SOFTSDV_MEM_REFS_CLASS<
    ASIM_SOFTSDV_X86_MAX_LOADS,
    ASIM_SOFTSDV_X86_MAX_STORES,
    ASIM_SOFTSDV_X86_VALUE_BUF_LEN> ASIM_SOFTSDV_X86_MEM_REFS_CLASS;


//
// x86-specific instruction data.  It is derived from generic descriptor
// classes (e.g. the memory reference class).
//
typedef class ASIM_SOFTSDV_INST_INFO_CLASS *ASIM_SOFTSDV_INST_INFO;
typedef const class ASIM_SOFTSDV_INST_INFO_CLASS *CONST_ASIM_SOFTSDV_INST_INFO;

class ASIM_SOFTSDV_INST_INFO_CLASS
{
  public:
    ASIM_SOFTSDV_INST_INFO_CLASS()
    {
        Reset();
    };

    ~ASIM_SOFTSDV_INST_INFO_CLASS() {};
    
    //
    // Memory references
    //
    UINT32 NRefs(bool getLoad) const { return memRefs.NRefs(getLoad); };
    ASIM_SOFTSDV_MEM_ACCESS GetRef(bool getLoad, UINT32 idx) { return memRefs.GetRef(getLoad, idx); };
    const ASIM_SOFTSDV_MEM_ACCESS_CLASS *GetRef(bool getLoad, UINT32 idx) const { return memRefs.GetRef(getLoad, idx); };

    UINT32 NLoads(void) const { return memRefs.NLoads(); };
    ASIM_SOFTSDV_MEM_ACCESS GetLoad(UINT32 idx) { return memRefs.GetLoad(idx); };
    const ASIM_SOFTSDV_MEM_ACCESS_CLASS *GetLoad(UINT32 idx) const { return memRefs.GetLoad(idx); };

    UINT32 NStores(void) const { return memRefs.NStores(); };
    ASIM_SOFTSDV_MEM_ACCESS GetStore(UINT32 idx) { return memRefs.GetStore(idx); };
    const ASIM_SOFTSDV_MEM_ACCESS_CLASS *GetStore(UINT32 idx) const { return memRefs.GetStore(idx); };

    void SetMemRefs(const ASIM_SOFTSDV_X86_MEM_REFS_CLASS *refs);

    //
    // Accessor functions
    //
    bool IsControl(void) const { return isControl; };
    bool IsRepeat(void) const { return isRepeat; };
    bool IsMemRef(void) const { return (NLoads() + NStores()) != 0; };

    CPU_MODE_TYPE GetCpuMode(void) const { return cpuMode; };

    INT32 GetPid(void) const { ASSERTX(GetContextSwitch()); return pid; };
    const char *GetProcessName(void) const
    {
        ASSERTX(GetContextSwitch());
        return processName;
    };

    // Tagged instructions (e.g. SoftSDV SSC markers)
    bool GetInstructionIsTagged(void) const { return instrTag != -1; };
    INT32 GetInstrTag(void) const { ASSERTX(GetInstructionIsTagged()); return instrTag; };

    // Binary encoding
    enum CONSTANTS
    {
        N_INSTR_BYTES = 16
    };
    typedef UINT8 INSTR_BYTE_ARR[N_INSTR_BYTES];
    const INSTR_BYTE_ARR & GetInstrBytes(void) const { return instrBytes; };
    UINT32 GetInstrNBytes(void) const { return instrNBytes; }

    // Control
    UINT64 GetTargetPA(void) const { ASSERTX(IsControl()); return targetPA; };
    UINT64 GetTargetVA(void) const { ASSERTX(IsControl()); return targetVA; };
    UINT64 GetBranchTaken(void) const { ASSERTX(IsControl()); return branchTaken; };

    // Input/Output registers
    UINT32 NInputRegisters(void) const { return 0; };
    UINT32 NOutputRegisters(void) const { return 0; };

    const ARCH_REGISTER_CLASS &InputRegister(UINT32 n) const
    {
        ASSERTX(n < NInputRegisters());
        static ARCH_REGISTER_CLASS dummy;
        return dummy;
    };

    const ARCH_REGISTER_CLASS &OutputRegister(UINT32 n) const
    {
        ASSERTX(n < NOutputRegisters());
        static ARCH_REGISTER_CLASS dummy;
        return dummy;
    };

    bool IsIO(void) const { return isIO; };
    UINT32 GetIOSize(void) const { ASSERTX(IsIO()); return ioSize; };
    UINT64 GetIOData(void) const { ASSERTX(IsIO()); return ioData; };
    UINT64 GetIOPort(void) const { ASSERTX(IsIO()); return ioPort; };

    //
    // Functions that set values
    //
    void Reset(void)
    {
        //
        // This function does not overwrite all the storage in the class.
        // Some values can be read only when a boolean is set (e.g. the
        // VA and PA for a load).  As long as the boolean protecting the
        // storage is clear we don't take the extra time to clear the
        // protected storage.
        //
        isControl = false;
        isRepeat = false;
        isIO = false;

        SetWarmUp(false);
        SetNewSequence(false);
        SetInterrupt(false);
        SetReturnFromInterrupt(false);
        SetKernelInstr(false);
        SetTimerInterrupt(false);
        SetIdleInstr(false);
        SetPauseInstr(false);
        SetContextSwitch(false);

        SetUid(0);
        SetRegsUid(0);
        SetSoftsdvId(0);
        instrTag = -1;
        instrNBytes = 0;

        SetCpuMode(CPU_MODE_UNKNOWN);

        memRefs.Reset();
    };

    void NoteContextSwitch(INT32 newPid, const char *newName)
    {
        SetContextSwitch(true);
        pid = newPid;
        strncpy(processName, newName, sizeof(processName));
        processName[sizeof(processName)-1] = 0;
    };

    void SetInstrTag(INT32 tag) { instrTag = tag; }

    inline void SetInstrBytes(const unsigned char *instr, UINT32 nBytes);

    //
    // Instruction types and associated values
    //
    inline void SetControl(bool isTaken, UINT64 va, UINT64 pa);
    inline void SetRepeat();

    inline void SetIOInfo(UINT64 port, UINT64 data, UINT32 size);

    inline void SetCpuMode(CPU_MODE_TYPE mode) { cpuMode = mode; };

    //
    // IPF compatibility.
    //
    bool IsTaccess(void) { return false; }          // IPF tag access instr

  private:
    ASIM_SOFTSDV_X86_MEM_REFS_CLASS memRefs;

    bool isControl;
    bool isRepeat;
    bool isIO;

    //
    // A few flags
    //
    PUBLIC_OBJ(bool, WarmUp);               // Instruction is in warm-up stream
    PUBLIC_OBJ(bool, NewSequence);          // First instruction after a skip
    PUBLIC_OBJ(bool, Interrupt);            // Heuristic guess that instr is an interrupt
    PUBLIC_OBJ(bool, ReturnFromInterrupt);
    PUBLIC_OBJ(bool, KernelInstr);          // Instruction is in the kernel
    PUBLIC_OBJ(bool, TimerInterrupt);       // Timer interrupt detected
    PUBLIC_OBJ(bool, IdleInstr);            // Instr is an idle loop tag
    PUBLIC_OBJ(bool, PauseInstr);           // Instr is PAUSE
    PRIVATE_OBJ(bool, ContextSwitch);       // Context switch detected

    bool branchTaken;

    // Instruction bytes
    INSTR_BYTE_ARR instrBytes;
    UINT32 instrNBytes;

    __attribute__ ((aligned))
    UINT64 targetPA;
    UINT64 targetVA;
    
    PUBLIC_OBJ(UINT64, Uid);        // Unique ID assigned by Asim stub in SoftSDV
    PUBLIC_OBJ(UINT64, RegsUid);    // Unique ID of associated input register
                                    // values.  Remains 0 if no register values
                                    // are associated with the instruction.
    PUBLIC_OBJ(UINT64, SoftsdvId);  // SoftSDV's tag for the instruction.

    // Address of the instruction.  Low bits indicate syllable.
    PUBLIC_OBJ(UINT64, AddrPhysical);
    PUBLIC_OBJ(UINT64, AddrVirtual);

    UINT64 ioPort;          // Port for I/O instruction
    UINT64 ioData;          // Data for I/O instruction
    UINT32 ioSize;          // Size of I/O data

    INT32 instrTag;         // Some instructions can be used as semaphores
                            // for SoftSDV scripting.  If the instruction
                            // is a marker, the tag value is stored here.
                            // -1 means no tag.

    CPU_MODE_TYPE cpuMode;

    // Set only on a context switch
    INT32 pid;
    char processName[32];
}
__attribute__((aligned));


inline void
ASIM_SOFTSDV_INST_INFO_CLASS::SetControl(bool isTaken, UINT64 va, UINT64 pa)
{
    isControl = true;
    branchTaken = isTaken;
    targetVA = va;
    targetPA = pa;
};


inline void
ASIM_SOFTSDV_INST_INFO_CLASS::SetRepeat()
{
    isRepeat = true;
};


inline void
ASIM_SOFTSDV_INST_INFO_CLASS::SetMemRefs(
    const ASIM_SOFTSDV_X86_MEM_REFS_CLASS *refs)
{
    memRefs = *refs;
};


inline void
ASIM_SOFTSDV_INST_INFO_CLASS::SetInstrBytes(
    const unsigned char *instr,
    UINT32 nBytes)
{
    ASSERTX(nBytes <= N_INSTR_BYTES);
    instrNBytes = nBytes;
    for (UINT32 i = 0; i < nBytes; i++)
    {
        instrBytes[i] = instr[i];
    }
};


inline void
ASIM_SOFTSDV_INST_INFO_CLASS::SetIOInfo(
    UINT64 port,
    UINT64 data,
    UINT32 size)
{
    isIO = true;
    ioPort = port;
    ioData = data;
    ioSize = size;
};

#pragma pack(pop)

#endif // _SOFTSDV_DATA_X86_H
