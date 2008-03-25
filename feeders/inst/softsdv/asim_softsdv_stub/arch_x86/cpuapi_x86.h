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
 * @file cpuapi_x86.h
 * @author Michael Adler
 * @brief x86 specific code for SoftSDV stub
 */

#ifndef _CPUAPI_X86_H
#define _CPUAPI_X86_H

#include "asim/dynamic_array.h"

//
// SoftSDV ISA-specific include files
//
#include "cpuapi_arch_ia32.h"
#include "disp69.h"

#define ASIM_CPUAPI_ISA CPUAPI_IA32

extern cpuapi_controller_callbacks_t *asimCpucCallBack;
extern SOFTSDV_IO_SOFTSDV_SIDE_CLASS asim_io;

//
// Setup ISA-specific callbacks
//
void *
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
        stateValid(false),
        stateWarmup(false),
        view_cpu_handle(0),
        cpl_handle(0),
        cpuMode_handle(0),
        simAliveCpus_handle(0),
        simInstInfo_handle(0),
        firstSTRegIdx(0),
        lastSTRegIdx(0),
        mapSTRegs(false)
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
    // Set unique id's for the CPU.  These values are passed to a number of
    // CPUAPI callbacks.
    //
    void SetControllerPid(
        int num,
        cpuapi_pid_t pid
    )
    {
        cpuNum = num;
        controllerPid = pid;
    };

    void InitHandles(const ASIM_REQUESTED_REGS_CLASS &regRqst);

    //
    // Called when SoftSDV changes the simulation mode between functional
    // and performance modes.
    //
    void SetSimulationMode(cpuapi_simulmode_t mode)
    {
        simulationMode = mode;
    };

    //
    // The stateValid counter indicates whether the CPU has been running in
    // performance mode and the cached register values remain valid.  The
    // state is invalidated if SoftSDV switches to functional mode.  It is
    // a counter instead of a boolean because SoftSDV seems to have
    //
    bool StateValid(void) const { return stateValid; };
    void SetStateInvalid(void) { stateValid = false; };
    void SetStateValid(void) { stateValid = true; };

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

    // Enable/disable returning of memory values for instrs executed by SoftSDV
    void CollectMemoryValues(bool collectValues);

    // Set active CPU in SoftSDV's visibility interface
    void SetViewCPU(void);

    // Access class describing the set of regsters to monitor for Asim
    const ASIM_REQUESTED_REGS_CLASS *MonitorRegs(void) const { return &monitorRegs; };
    UINT32 GetMonitoredRegValue(
        UINT32 regIdx,
        void *value,
        UINT32 size);

    //------------------------------------------------------------------------
    // ISA specific decarations.  Not used by cpuapi_interface.cpp.
    //------------------------------------------------------------------------

    UINT64 EFlags(void);
    UINT64 CpuMode(void);
    UINT32 CurrentPrivLevel(void);
    UINT64 RegRSP(void);
    UINT64 GSBase(void);
    UINT64 KernelGSBase(void);
    UINT32 FPStatus(void);

  private:
    int cpuNum;                             // CPU number (0 based, dense numbering)
    cpuapi_pid_t controllerPid;             // SoftSDV CPU controller ID for
                                            // the processor
    cpuapi_simulmode_t simulationMode;

    bool stateValid;
    bool stateWarmup;

    //
    // Handles to Gambit state
    //
    OML_handle_t view_cpu_handle;
    OML_handle_t cpl_handle;
    cpuapi_handle_t cpuMode_handle;
    cpuapi_handle_t simAliveCpus_handle;
    cpuapi_handle_t simInstInfo_handle;
    cpuapi_handle_t simCollectMemValues_handle;
    cpuapi_handle_t regRSP_handle;
    cpuapi_handle_t gsBase_handle;
    cpuapi_handle_t kernelGSBase_handle;
    cpuapi_handle_t fpStatus_handle;

    ASIM_REQUESTED_REGS_CLASS monitorRegs;
    UINT32 firstSTRegIdx;           // Hack for mapping ST to FR regs
    UINT32 lastSTRegIdx;
    bool mapSTRegs;
};

typedef CPU_INFO_CLASS *CPU_INFO;

//
// State for each simulated CPU is global.
//
extern DYNAMIC_ARRAY_CLASS<CPU_INFO_CLASS> cpuInfo;


//------------------------------------------------------------------------
//
// DMA management class
// 
//------------------------------------------------------------------------

class CPU_DMA_INVALIDATE_CLASS
{
  private:
    class DMA_EVENT_CLASS
    {
      public:
        DMA_EVENT_CLASS(CPU_DMA_INVALIDATE_CLASS *parent, UINT64 addr, UINT64 size);
        ~DMA_EVENT_CLASS();
        
        UINT64 addr;
        UINT64 size;
        DMA_EVENT_CLASS *next;
        CPU_DMA_INVALIDATE_CLASS *parent;
    };

    typedef DMA_EVENT_CLASS *DMA_EVENT;

    DMA_EVENT head;
    DMA_EVENT tail;

  public:
    CPU_DMA_INVALIDATE_CLASS(void);
    ~CPU_DMA_INVALIDATE_CLASS();

    void InvalidateAddr(UINT64 addr, UINT64 size);
    bool GetDMAEvent(UINT64 *addr, UINT64 *size);
};

typedef CPU_DMA_INVALIDATE_CLASS *CPU_DMA_INVALIDATE;

extern CPU_DMA_INVALIDATE dmaIdx;


//------------------------------------------------------------------------
//
// Instruction decoder -- There will be one instance of the decoder class
//     used by all the code in this module.
// 
//------------------------------------------------------------------------

typedef class ASIM_CPUAPI_INST_INFO_CLASS;

class INSTR_DECODER_CLASS
{
  public:
    INSTR_DECODER_CLASS(void);
    ~INSTR_DECODER_CLASS();

    void Init(void);

    cpuapi_stat_t
    DecodeInstr(const unsigned char *instr,
                unsigned int instrLen,
                cpuapi_cpu_mode_t mode,
                IA_Decoder_Info *decoded);

    void
    DisassembleInstr(FILE *f,
                     const ASIM_CPUAPI_INST_INFO_CLASS &instInfo);

    void
    DisassembleInstr(
        FILE *f,
        UINT64 va,
        const unsigned char *instr,
        unsigned int instrLen,
        cpuapi_cpu_mode_t mode);

  private:
	IA_Decoder_Machine_Mode
    TranslateDecoderMode(cpuapi_cpu_mode_t sdvMode);

    //
    // Handles to decoder and disassembler shared libraries.  These must
    // be loaded explicitly.
    //
    void *decoderHandle;
    void *disasmHandle;

    IA_Decoder_Id id;
    cpuapi_cpu_mode_t curMode;
    bool initialized;
};

//
// All decoding will be through one instance of the class
//
extern INSTR_DECODER_CLASS instrDecoder;


//------------------------------------------------------------------------
//
// Abstract instruction -- hide the details of the data returned by
//     SoftSDV.  SoftSDV changes its interface and data structure with
//     each release.  The abstract instruction hides the changes and
//     presents a common interface that works across all SoftSDV versions.
// 
//------------------------------------------------------------------------

//
// SoftSDV keeps changing the instruction info record.  This class
// works across versions.
//

typedef class ASIM_CPUAPI_INST_INFO_CLASS* ASIM_CPUAPI_INST_INFO;

class ASIM_CPUAPI_INST_INFO_CLASS
{
  private:
    enum constants
    {
        MAX_LOADS = 4
    };

    cpuapi_inst_info_t *sdvInst;

    ASIM_SOFTSDV_X86_MEM_REFS_CLASS mem;
    UINT32 cpuNum;
    bool memInit;

  public:
    ASIM_CPUAPI_INST_INFO_CLASS(cpuapi_inst_info_t *sdvInst = NULL) :
        sdvInst(sdvInst),
        cpuNum(0),
        memInit(false)
    {};

    ~ASIM_CPUAPI_INST_INFO_CLASS() {};

    inline void Init(cpuapi_inst_info_t *_sdvInst, UINT32 cpuNum = 0);
    inline void Reset(UINT32 cpu_num);
    void ChkMem(void);

    cpuapi_inst_t      GetInstId(void) const { return sdvInst->inst_id; };
    cpuapi_inst_t      GetIcount(void) const { return sdvInst->icount; };
    cpuapi_phys_addr_t GetPA(void) const { return sdvInst->phy_addr; };
    cpuapi_virt_addr_t GetVA(void) const { return sdvInst->virt_addr; };

    UINT32 NStores(void);
    const ASIM_SOFTSDV_MEM_ACCESS GetStore(UINT32 idx);
        
    UINT32 NLoads(void);
    const ASIM_SOFTSDV_MEM_ACCESS GetLoad(UINT32 idx);
        
    const ASIM_SOFTSDV_X86_MEM_REFS_CLASS *GetMemRefs(void);

    void DumpMemoryRefs(FILE *f = stderr);
    void DumpMemoryRegions(FILE *f = stderr);

    cpuapi_ia32_inst_info_t* GetArchInfo(void) const
    {
        return (cpuapi_ia32_inst_info_t *)sdvInst->arch_info;
    };

  private:
    void DumpOneMemoryRegion(FILE *f, const ASIM_SOFTSDV_MEM_ACCESS ref);

    //
    // Memory values are available in releases starting in 2005.
    //
    void NoteMemoryRefs(
        cpuapi_origin_type_t origin,
        SOFTSDV_MEM_REF_SOURCE refSource);
    bool MergeRefs(
        cpuapi_inst_memory_access_t *access,
        ASIM_SOFTSDV_MEM_ACCESS ref,
        SOFTSDV_MEM_REF_SOURCE refSource);
    void MergePAandData(
        cpuapi_inst_memory_access_t *access,
        ASIM_SOFTSDV_MEM_ACCESS ref);

  public:
    unsigned char*     GetInstBytes(void) const { return GetArchInfo()->inst_bytes; };
    cpuapi_u32_t       GetNInstBytes(void) const { return GetArchInfo()->inst_bytes_size; };
    cpuapi_boolean_t   GetNewInst(void) const { return GetArchInfo()->new_inst; };
    cpuapi_u32_t       GetEFlags(void) const { return GetArchInfo()->eflags; };
    cpuapi_u32_t       GetRepCount(void) const { return GetArchInfo()->cur_repeat_count; };

    cpuapi_boolean_t   GetIsBranchInst(void) const { return GetArchInfo()->branch_inst; };
    cpuapi_boolean_t   GetBranchTaken(void) const { return GetArchInfo()->branch_taken; };
    cpuapi_virt_addr_t GetBranchTargetVirtAddr(void) const { return GetArchInfo()->branch_target_virt_addr; };

    cpuapi_boolean_t   GetException(void) const { return GetArchInfo()->exception_interrupt; };

    // CPU mode can be queried as either the SoftSDV enumeration or the
    // equivalent Asim enumeration.
    cpuapi_cpu_mode_t  GetCpuMode(void) const { return GetArchInfo()->cpu_mode; };
    CPU_MODE_TYPE      GetAsimCpuMode(void) const;

    cpuapi_boolean_t   GetIsIO(void) const { return GetArchInfo()->is_io; };
    cpuapi_io_addr_t   GetIOPort(void) const{ return GetArchInfo()->io_port; };
    cpuapi_size_t      GetIOSize(void) const{ return GetArchInfo()->io_access_size; };
    cpuapi_u64_t       GetIOValue(void) const{ return GetArchInfo()->io_value; };
};


extern void
DumpMemoryRef(
    FILE *f,
    const cpuapi_inst_memory_access_t *access,
    const void *data);

#endif // _CPUAPI_X86_H
