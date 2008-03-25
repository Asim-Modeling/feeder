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
 * @brief SoftSDV IPF specific data passed between SoftSDV and Asim.
 */

#ifndef _SOFTSDV_DATA_IPF_H
#define _SOFTSDV_DATA_IPF_H

#include <string.h>

#include "asim/arch_register.h"

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
    ASIM_SOFTSDV_IPF_MAX_LOADS = 1,
    ASIM_SOFTSDV_IPF_MAX_STORES = 1,
    ASIM_SOFTSDV_IPF_VALUE_BUF_LEN = 32
};

typedef ASIM_SOFTSDV_MEM_ACCESS_CLASS *ASIM_SOFTSDV_MEM_ACCESS;

typedef ASIM_SOFTSDV_MEM_REFS_CLASS<
    ASIM_SOFTSDV_IPF_MAX_LOADS,
    ASIM_SOFTSDV_IPF_MAX_STORES,
    ASIM_SOFTSDV_IPF_VALUE_BUF_LEN> ASIM_SOFTSDV_IPF_MEM_REFS_CLASS;

//
// IPF-specific instruction data.  It is derived from generic descriptor
// classes (e.g. the memory reference class).
//
typedef class ASIM_SOFTSDV_INST_INFO_CLASS *ASIM_SOFTSDV_INST_INFO;
typedef const class ASIM_SOFTSDV_INST_INFO_CLASS *CONST_ASIM_SOFTSDV_INST_INFO;

class ASIM_SOFTSDV_INST_INFO_CLASS
{
  public:
    enum INST_TYPE
    {
        NONE,
        NORMAL,
        CONTROL,
        LOAD,
        STORE
    };

    enum CONSTANTS
    {
        MAX_INPUT_REGISTERS = 5,
        MAX_OUTPUT_REGISTERS = 2
    };

    ASIM_SOFTSDV_INST_INFO_CLASS()
    {
        Reset();
    };

    ~ASIM_SOFTSDV_INST_INFO_CLASS() {};
    
    //
    // Memory references
    //
    UINT32 NLoads(void) const { return memRefs.NLoads(); };
    ASIM_SOFTSDV_MEM_ACCESS GetLoad(UINT32 idx) { return memRefs.GetLoad(idx); };
    const ASIM_SOFTSDV_MEM_ACCESS_CLASS *GetLoad(UINT32 idx) const { return memRefs.GetLoad(idx); };
    UINT32 NStores(void) const { return memRefs.NStores(); };
    ASIM_SOFTSDV_MEM_ACCESS GetStore(UINT32 idx) { return memRefs.GetStore(idx); };
    const ASIM_SOFTSDV_MEM_ACCESS_CLASS *GetStore(UINT32 idx) const { return memRefs.GetStore(idx); };

    //
    // Accessor functions
    //
    bool IsControl(void) const
    {
        return GetType() == CONTROL;
    };
    bool IsLoad(void) const
    {
        return GetType() == LOAD;
    };
    bool IsStore(void) const
    {
        return GetType() == STORE;
    };
    bool IsMemRef(void) const
    {
        return IsLoad() || IsStore();
    };

    INT32 GetPid(void) const
    {
        ASSERTX(GetContextSwitch());
        return pid;
    };
    const char *GetProcessName(void) const
    {
        ASSERTX(GetContextSwitch());
        return processName;
    };

    // Tagged instructions (e.g. SoftSDV SSC markers)
    bool GetInstructionIsTagged(void) const { return instrTag != -1; }
    INT32 GetInstrTag(void) const
    {
        ASSERTX(GetInstructionIsTagged());
        return instrTag;
    }

    // Control
    UINT64 GetTargetPA(void) const
    {
        ASSERTX(GetType() == CONTROL);
        return targetAddrPhysical;
    };
    UINT64 GetTargetVA(void) const
    {
        ASSERTX(GetType() == CONTROL);
        return targetAddrVirtual;
    };
    bool GetBranchTaken(void) const
    {
        ASSERTX(GetType() == CONTROL);
        return branchTaken;
    };

    // x86 compatibility -- separate register values not used
    UINT64 GetRegsUid(void) const { return 0; };
    bool IsIO(void) const { return false; };
    UINT32 GetIOSize(void) const { return 0; };
    UINT64 GetIOData(void) const { return 0; };
    UINT64 GetIOPort(void) const { return 0; };
    bool GetPauseInstr(void) const { return false; };

    //
    // Functions that set values
    //
    void Reset(void)
    {
        SetType(NONE);
        SetUid(0);
        SetSoftsdvId(0);
        SetWarmUp(false);
        SetNewSequence(false);
        SetInterrupt(false);
        SetReturnFromInterrupt(false);
        SetKernelInstr(false);
        SetIdleInstr(false);
        SetContextSwitch(false);
        SetTimerInterrupt(false);
        nInputRegisters = 0;
        nOutputRegisters = 0;
        instrTag = -1;

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

    //
    // Instruction types and associated values
    //
    void SetNormal(void) { SetType(NORMAL); };

    void SetControl(
        bool isTaken,
        UINT64 pa,
        UINT64 va)
    {
        SetType(CONTROL);
        branchTaken = isTaken;
        targetAddrPhysical = pa;
        targetAddrVirtual = va;
    };

    void SetLoad(
        UINT64 pa,
        UINT64 va)
    {
        SetType(LOAD);
        ASIM_SOFTSDV_MEM_ACCESS m = memRefs.ActivateLoad(0, SDV_MEM_REF_INSTRUCTION);
        m->SetVA(va);
        // Ignore translations to page 0
        if (pa)
        {
            m->SetPA(pa, 0);
        }
        m->SetSize(32);     // Doesn't matter for IPF, though it must be at least
                            // as large as the memory reference.
    };

    void SetStore(
        UINT64 pa,
        UINT64 va)
    {
        SetType(STORE);
        ASIM_SOFTSDV_MEM_ACCESS m = memRefs.ActivateStore(0, SDV_MEM_REF_INSTRUCTION);
        m->SetVA(va);
        // Ignore translations to page 0
        if (pa)
        {
            m->SetPA(pa, 0);
        }
        m->SetSize(32);     // Doesn't matter for IPF, though it must be at least
                            // as large as the memory reference.
    };

    //
    // Register values...
    //
    UINT32 NInputRegisters(void) const { return nInputRegisters; };
    UINT32 NOutputRegisters(void) const { return nOutputRegisters; };

    const ARCH_REGISTER_CLASS &InputRegister(UINT32 n) const
    {
        ASSERTX(n < NInputRegisters());
        return inputRegisters[n];
    };

    const ARCH_REGISTER_CLASS &OutputRegister(UINT32 n) const
    {
        ASSERTX(n < NOutputRegisters());
        return outputRegisters[n];
    };

    void AddInputRegister(const ARCH_REGISTER_CLASS &reg)
    {
        //
        // This test could fire if MAX_INPUT_REGISTERS is set too low.
        // The set of input registers is derived in SoftSDV's decoder
        // from two places:  explicit registers and implicit ones.  The
        // sum of the maximum number of explicit and implicit registers
        // is too large to be practical here, since we aren't interested
        // in and don't describe the majority of implicit registers.  If
        // this check fires try incremented MAX_INPUT_REGISTERS by one
        // and see if it goes away.
        //
        ASSERTX(nInputRegisters < MAX_INPUT_REGISTERS);

        //
        // Don't record a register twice.
        //
        for (UINT32 i = 0; i < nInputRegisters; i++)
        {
            if ((inputRegisters[i].GetType() == reg.GetType()) &&
                (inputRegisters[i].GetNum() == reg.GetNum()))
            {
                return;
            }
        }

        inputRegisters[nInputRegisters] = reg;
        nInputRegisters += 1;
    };

    void AddOutputRegister(const ARCH_REGISTER_CLASS &reg)
    {
        //
        // See the comment about MAX_INPUT_REGISTERS in AddInputRegister().
        // The same applies here.
        //
        ASSERTX(nOutputRegisters < MAX_OUTPUT_REGISTERS);

        //
        // Don't record a register twice.
        //
        for (UINT32 i = 0; i < nOutputRegisters; i++)
        {
            if ((outputRegisters[i].GetType() == reg.GetType()) &&
                (outputRegisters[i].GetNum() == reg.GetNum()))
            {
                return;
            }
        }

        outputRegisters[nOutputRegisters] = reg;
        nOutputRegisters += 1;
    };

    // Get bundle as either 16 bytes or 2 64 bit uints.
    const char * GetBundle8(void) const { return bundle.bytes; };
    const UINT64 * GetBundle64(void) const { return bundle.longs; };

    // Registers.
    UINT64 GetPredsBefore_Physical(void) const
    {
        return UnrotatePredicates(GetPredsBefore_Logical());
    };

    UINT64 GetPredsAfter_Physical(void) const
    {
        return UnrotatePredicates(GetPredsAfter_Logical());
    };

    // Bundle set as either 16 bytes or two 64 bit uints.
    void SetBundle(unsigned char *b)
    {
        for (int i = 0; i < 16; i++)
        {
            bundle.bytes[i] = b[i];
        }
    };

    void SetBundle(UINT64 *l)
    {
        bundle.longs[0] = l[0];
        bundle.longs[1] = l[1];
    };

  private:
    UINT64 
    UnrotatePredicates(UINT64 pr) const
    {
        UINT64 rotate_amount = (GetCfmBefore() >> 32) & 0x3f;

        if (rotate_amount)
        {
            UINT64 rotating = (pr & (0xFFFFFFFFFFFF0000LL)) >> 16;
            rotating = (rotating >> (48 - rotate_amount)) |
                       (rotating << rotate_amount);
            return (rotating << 16) | (pr & 0xffff);
        }
        else
        {
            return pr;
        }
    };

    PRIVATE_OBJ(INST_TYPE, Type);

    //
    // A few flags
    //
    PUBLIC_OBJ(bool, WarmUp);               // Instruction is in warm-up stream
    PUBLIC_OBJ(bool, NewSequence);          // First instruction after a skip
    PUBLIC_OBJ(bool, Interrupt);            // Heuristic guess that instr is an interrupt
    PUBLIC_OBJ(bool, ReturnFromInterrupt);
    PUBLIC_OBJ(bool, KernelInstr);          // Instruction is in the kernel
    PUBLIC_OBJ(bool, TimerInterrupt);       // Timer interrupt detected

    // For architectures with no PAUSE instruction where we want to study
    // power management with a PAUSE in the idle loop.  This mechanism
    // allows one instruction to be tagged as the PAUSE in the idle loop.
    PUBLIC_OBJ(bool, IdleInstr);

    PRIVATE_OBJ(bool, ContextSwitch);       // Context switch detected

    // Input and output register values
    __attribute__ ((aligned))
    UINT32 nInputRegisters;
    UINT32 nOutputRegisters;
    ARCH_REGISTER_CLASS inputRegisters[MAX_INPUT_REGISTERS];
    ARCH_REGISTER_CLASS outputRegisters[MAX_OUTPUT_REGISTERS];

    PUBLIC_OBJ(UINT64, Uid);        // Unique ID assigned by Asim stub in SoftSDV
    PUBLIC_OBJ(UINT64, SoftsdvId);  // SoftSDV's tag for the instruction.

    // Address of the instruction.  Low bits indicate syllable.
    PUBLIC_OBJ(UINT64, AddrPhysical);
    PUBLIC_OBJ(UINT64, AddrVirtual);

    // Various register values (name indicates whether value is before
    // or after execution of this instruction.)
    PUBLIC_OBJ(UINT64, CfmBefore);
    PUBLIC_OBJ(UINT64, BspBefore);
    PUBLIC_OBJ(UINT64, PredsBefore_Logical);

    PUBLIC_OBJ(UINT64, CfmAfter);
    PUBLIC_OBJ(UINT64, BspAfter);
    PUBLIC_OBJ(UINT64, PredsAfter_Logical);

    ASIM_SOFTSDV_IPF_MEM_REFS_CLASS memRefs;

    INT32 instrTag;         // Some instructions can be used as semaphores
                            // for SoftSDV scripting.  If the instruction
                            // is a marker, the tag value is stored here.
                            // -1 means no tag.

    // Set only on a context switch
    INT32 pid;
    char processName[32];

    // Branches
    UINT64 targetAddrPhysical;
    UINT64 targetAddrVirtual;
    bool branchTaken;

    // Instruction bytes
    union
    {
        char bytes[16];
        UINT64 longs[2];
    }
    bundle;
}
__attribute__((aligned));

#pragma pack(pop)

#endif // _SOFTSDV_DATA_IPF_H
