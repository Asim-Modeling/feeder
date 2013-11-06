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
 * @file cpuapi_x86.cpp
 * @author Michael Adler
 * @brief x86 specific code for SoftSDV stub
 */

void asim_error_exit(int s);

#define ASSERT(condition,mesg) \
    if (! (condition)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, mesg); \
        asim_error_exit(1); \
    }

#define ASSERTX(condition) \
    if (! (condition)) { \
        fprintf(stderr, "Assert failure in %s:%d\n", __FILE__, __LINE__); \
        asim_error_exit(1); \
    }

#define VERIFY(condition,mesg) ASSERT(condition,mesg)

#define ASIMERROR(mesg) \
    fprintf(stderr, "%s:%d %s\n", __FILE__, __LINE__, mesg); \
    asim_error_exit(1);

#include <iostream>
#include <stdio.h>
#include <dlfcn.h>

using namespace std;

//
// ASIM include files
//
#include "asim/provides/softsdv_stub.h"
#include "asim/provides/softsdv_stub_isa.h"
#include "asim/provides/softsdv_cpuapi.h"


#ifdef WIN32
#define EXPORT_FUNC __declspec( dllexport )
#else
#define EXPORT_FUNC
#endif


DYNAMIC_ARRAY_CLASS<CPU_INFO_CLASS> cpuInfo;
INSTR_DECODER_CLASS instrDecoder;

static ASIM_LOCKSTEP_X86 lockStep = NULL;


//------------------------------------------------------------------------
//
// System information
// 
//------------------------------------------------------------------------

class SYSTEM_INFO_CLASS
{
  public:
    SYSTEM_INFO_CLASS() :
        targetProc(GE_PROCESSOR_P6),
        osMode(false)
    {};

    ~SYSTEM_INFO_CLASS() {};

    void Init()
    {
        OML_handle_t oh;
        unsigned int size;
        unsigned int tmpInt;

        oh = oml_obj_translate_name(GE_VPC_CPU_ID, "notifier.exec_mode");
        ASSERTX(oh);
        size = sizeof(tmpInt);
        ASSERTX(GE_OK == oml_get_value(GE_VPC_CPU_ID, oh, &size, &tmpInt));
        osMode = (tmpInt != 0);

        oh = oml_obj_translate_name(GE_VPC_CPU_ID, "notifier.target_processor");
        ASSERTX(oh);
        size = sizeof(tmpInt);
        ASSERTX(GE_OK == oml_get_value(GE_VPC_CPU_ID, oh, &size, &targetProc));
    };

    GE_target_processor_t TargetProc() const { return targetProc; };
    bool OsMode() const { return osMode; };

  private:
    GE_target_processor_t targetProc;
    bool osMode;
};

SYSTEM_INFO_CLASS sysInfo;


//------------------------------------------------------------------------
//
// Abstract instruction -- hide the details of the data returned by
//     SoftSDV.  SoftSDV changes its interface and data structure with
//     each release.  The abstract instruction hides the changes and
//     presents a common interface that works across all SoftSDV versions.
// 
//------------------------------------------------------------------------

//
// Init --
//   Most likely called only once at the beginning of the run to associate
//   the CPUAPI inst_info wrapper with a memory buffer.
//
inline void
ASIM_CPUAPI_INST_INFO_CLASS::Init(
    cpuapi_inst_info_t *_sdvInst,
    UINT32 cpuNum)
{
    sdvInst = _sdvInst;
    Reset(cpuNum);
};


//
// Reset --
//   Must be called before an inst_info buffer is passed to SoftSDV to be
//   filled with new data.  Invalidates any data cached about the last
//   instruction and computes properties of the new instruction.
//
inline void
ASIM_CPUAPI_INST_INFO_CLASS::Reset(UINT32 cpu_num)
{
    cpuNum = cpu_num;
    mem.Reset();
    memInit = false;
};


inline CPU_MODE_TYPE
ASIM_CPUAPI_INST_INFO_CLASS::GetAsimCpuMode(void) const
{
    CPU_MODE_TYPE mode;

    switch (GetArchInfo()->cpu_mode)
    {
      case CPUAPI_Cpu_Mode_8086:
      case CPUAPI_Cpu_Mode_Big_Real:
        mode = CPU_MODE_REAL;
        break;
      case CPUAPI_Cpu_Mode_v8086:
        mode = CPU_MODE_V8086;
        break;
      case CPUAPI_Cpu_Mode_Protected_16:
        mode = CPU_MODE_PROTECTED_16;
        break;
      case CPUAPI_Cpu_Mode_Protected_32:
        mode = CPU_MODE_PROTECTED_32;
        break;
      case CPUAPI_Cpu_Mode_Long_Compatible_16:
        mode = CPU_MODE_LONG_COMPATIBLE_16;
        break;
      case CPUAPI_Cpu_Mode_Long_Compatible_32:
        mode = CPU_MODE_LONG_COMPATIBLE_32;
        break;
      case CPUAPI_Cpu_Mode_Long_64:
        mode = CPU_MODE_LONG_64;
        break;
      case CPUAPI_Cpu_Mode_SMM:
        mode = CPU_MODE_SMM;
        break;
      default:
        mode = CPU_MODE_UNKNOWN;
        break;
    }

    return mode;
};


//
// ChkMem --
//   Verify that the memory properties have been computed.  If they haven't,
//   scan through SoftSDV memory access information.
//
void
ASIM_CPUAPI_INST_INFO_CLASS::ChkMem(void)
{
    if (memInit) return;
    memInit = true;

    //
    // Start by combining small instruction memory references into clumps.
    // Search for clumps of separate classes of references.
    //
    NoteMemoryRefs(CPUAPI_Origin_Instruction, SDV_MEM_REF_INSTRUCTION);

    NoteMemoryRefs(CPUAPI_Origin_Task, SDV_MEM_REF_TSS);

    NoteMemoryRefs(CPUAPI_Origin_Descriptor, SDV_MEM_REF_DESCRIPTOR);

    //
    // The "accessed" bit in descriptors causes problems.  If
    // not set, Archlib will immediately trigger a fault to set
    // it.  The fault updates the descriptor in memory and then
    // tries to resume.  Since the SoftSDV feeder doesn't keep
    // a copy of memory the update of memory is ignored and
    // Asim sits in an endless fault loop trying to set the accessed
    // bit.  Force the accessed bit to be set here.
    //
    if (asim_io.RecordMemoryValues())
    {
        for (unsigned int i = 0; i < mem.NLoads(); i++)
        {
            ASIM_SOFTSDV_MEM_ACCESS ref = mem.GetLoad(i);
            if (ref->HasValue() && (ref->GetRefSource() == SDV_MEM_REF_DESCRIPTOR))
            {
                UINT8 *descP = ref->GetValue();
                descP[5] |= 1;
            }
        }
    }
}


//
// NStores --
//   Number of active stores in instruction.
//
inline UINT32
ASIM_CPUAPI_INST_INFO_CLASS::NStores(void)
{
    if (! memInit) ChkMem();
    return mem.NStores();
}


//
// GetStore --
//   Get the descriptor for a store reference.
//
inline const ASIM_SOFTSDV_MEM_ACCESS
ASIM_CPUAPI_INST_INFO_CLASS::GetStore(UINT32 idx)
{
    ASSERTX(idx < NStores());
    return mem.GetStore(idx);
}


//
// NLoads --
//   Number of active loads in instruction.
//
inline UINT32
ASIM_CPUAPI_INST_INFO_CLASS::NLoads(void)
{
    if (! memInit) ChkMem();
    return mem.NLoads();
}


//
// GetLoad --
//   Get the descriptor for a load reference.
//
inline const ASIM_SOFTSDV_MEM_ACCESS
ASIM_CPUAPI_INST_INFO_CLASS::GetLoad(UINT32 idx)
{
    ASSERTX(idx < NLoads());
    return mem.GetLoad(idx);
}


//
// GetMemRefs --
//   Get the entire memory reference descriptor, suitable for copying directly
//   to Asim.
//
inline const ASIM_SOFTSDV_X86_MEM_REFS_CLASS *
ASIM_CPUAPI_INST_INFO_CLASS::GetMemRefs(void)
{
    if (! memInit) ChkMem();
    return &mem;
}


//
// NoteMemoryRefs --
//   Note all references to memory for a given origin.  SoftSDV separates
//   origin between the instruction itself, CPU descriptor tables, task
//   tables, etc.
//
void
ASIM_CPUAPI_INST_INFO_CLASS::NoteMemoryRefs(
    cpuapi_origin_type_t origin,
    SOFTSDV_MEM_REF_SOURCE refSource)
{
    cpuapi_inst_memory_access_t *access = NULL;

    cpuapi_ia32_inst_info_t* archInfo = GetArchInfo();
    if (archInfo == NULL)
    {
        return;
    }

    //
    // First pass through the references:  combine virtual regions into
    // reference groups.
    //
    access = archInfo->access_list;
    while (access != NULL)
    {
        //
        // Is access of the type we want?
        //
        if (access->origin == origin)
        {
            switch (access->access)     // Good name, eh?
            {
              case CPUAPI_Access_Execute:
                ASIMERROR("Execute as a data reference?");
                break;

              case CPUAPI_Access_Write:
                if (mem.NStores() == 0)
                {
                    mem.ActivateStore(0, refSource);
                }
                if (! MergeRefs(access, mem.GetStore(0), refSource))
                {
                    fprintf(stderr, "    ");
                    instrDecoder.DisassembleInstr(stderr, *this);
                    DumpMemoryRefs();
                    ASIMERROR("Instruction has more than one store region.");
                }

                break;

              default:
                // Assume read

                // There can be multiple load regions (e.g. CMPS takes 2
                // addresses).
                for (unsigned int i = 0; i <= ASIM_SOFTSDV_X86_MAX_LOADS; i++)
                {
                    if (i == ASIM_SOFTSDV_X86_MAX_LOADS)
                    {
                        // Out of slots!  Reference was supposed to be
                        // merged into the first two slots.
                        fprintf(stderr, "    ");
                        instrDecoder.DisassembleInstr(stderr, *this);
                        DumpMemoryRefs();
                        ASIMERROR("Instruction has more than two load regions.");
                    }

                    if (mem.NLoads() <= i)
                    {
                        mem.ActivateLoad(i, refSource);
                    }
                    if (MergeRefs(access, mem.GetLoad(i), refSource))
                    {
                        break;  // Success
                    }
                }
                break;
            }
        }

        access = access->next;
    }

    //
    // If the more than one load region is active, can the last be merged
    // with the one before it.
    //
    UINT32 nLoads = mem.NLoads();
    if ((nLoads > 1) &&
        ! mem.GetLoad(nLoads - 1)->HasValue() &&
        ! mem.GetLoad(nLoads - 2)->HasValue())
    {
        ASIM_SOFTSDV_MEM_ACCESS_CLASS ref0;
        cpuapi_inst_memory_access_t ref1;

        //
        // Construct some objects that can be passed to MergeRefs.  Using
        // temporary objects allows us to undo the merge if it turns out
        // to be undesirable.
        //
        ref0 = *mem.GetLoad(nLoads - 2);
        ref1.origin = origin;
        ref1.access = CPUAPI_Access_Read;
        ref1.size = mem.GetLoad(nLoads - 1)->GetSize();
        ref1.virt_mem_addr = mem.GetLoad(nLoads - 1)->GetVA();
        ref1.phy_mem_addr = mem.GetLoad(nLoads - 1)->PTranslate(ref1.virt_mem_addr);

        //
        // Can the two regions be merged without being in danger of crossing
        // more than 2 pages?
        //
        if (MergeRefs(&ref1, &ref0, refSource) && (ref0.GetSize() <= 4096))
        {
            // Yes
            *mem.GetLoad(nLoads - 2) = ref0;
            mem.DeactivateLoad(nLoads - 1);
        }
    }

    //
    // Allocate storage for the memory references now that the sizes
    // are known.
    //
    if (asim_io.RecordMemoryValues())
    {
        for (unsigned int i = 0; i < mem.NLoads(); i++)
        {
            if (! mem.GetLoad(i)->HasValue() && ! mem.AllocLoadStorage(i))
            {
                fprintf(stderr, "    ");
                instrDecoder.DisassembleInstr(stderr, *this);
                DumpMemoryRefs();
                ASIMERROR("Instruction reads more memory than value available value space.");
            }
        }

        for (unsigned int i = 0; i < mem.NStores(); i++)
        {
            if (! mem.GetStore(i)->HasValue() && ! mem.AllocStoreStorage(i))
            {
                fprintf(stderr, "    ");
                instrDecoder.DisassembleInstr(stderr, *this);
                DumpMemoryRefs();
                ASIMERROR("Instruction writes more memory than value available value space.");
            }
        }
    }

    //
    // Second pass through the references:  note physical translations
    // and loaded/stored values.
    //
    access = archInfo->access_list;
    while (access != NULL)
    {
        //
        // Is access of the type we want?
        //
        if (access->origin == origin)
        {
            switch (access->access)     // Good name, eh?
            {
              case CPUAPI_Access_Write:
                for (unsigned int i = 0; i < mem.NStores(); i++)
                {
                    MergePAandData(access, mem.GetStore(i));
                }
                break;

              default:
                // Assume read.  Merge into all reads.
                for (unsigned int i = 0; i < mem.NLoads(); i++)
                {
                    MergePAandData(access, mem.GetLoad (i));
                }
                break;
            }
        }

        access = access->next;
    }
}


//
// MergeRefs --
//     Merge a new access from CPUAPI with an access already noted.
//     Operations like stack push for a group of registers generate
//     multiple memory reference records for contiguous memory regions.
//     This code joins the references into a single region.
//
//     Returns true iff the new region can be merged.
//
bool
ASIM_CPUAPI_INST_INFO_CLASS::MergeRefs(
    cpuapi_inst_memory_access_t *access,
    ASIM_SOFTSDV_MEM_ACCESS ref,
    SOFTSDV_MEM_REF_SOURCE refSource)
{
    if (ref->HasValue() || (refSource != ref->GetRefSource()))
    {
        // Can't merge with a reference that already has a value or is a
        // different type.
        return false;
    }

    if (ref->GetSize() == 0)
    {
        // Reference has no value, accept new one
        ref->SetVA(access->virt_mem_addr);
        ref->SetPA(access->phy_mem_addr, 0);
        ref->SetSize(access->size);
        return true;
    }

    if ((access->virt_mem_addr >= ref->GetVA()) &&
        (access->virt_mem_addr <= ref->GetVA() + ref->GetSize()))
    {
        // New region begins in or just after the old region
        if (access->virt_mem_addr + access->size > ref->GetVA() + ref->GetSize())
        {
            ref->SetSize(access->virt_mem_addr + access->size - ref->GetVA());
        }
        return true;
    }

    if ((access->virt_mem_addr + access->size + 1 >= ref->GetVA()) &&
        (access->virt_mem_addr + access->size <= ref->GetVA() + ref->GetSize()))
    {
        // New region ends in or just before old region
        UINT64 endVa = ref->GetVA() + ref->GetSize();
        ref->SetVA(access->virt_mem_addr);
        ref->SetPA(access->phy_mem_addr, 0);
        ref->SetSize(endVa - access->virt_mem_addr);
        return true;
    }

    if ((access->virt_mem_addr <= ref->GetVA()) &&
        (access->virt_mem_addr + access->size >= ref->GetVA() + ref->GetSize()))
    {
        // New region subsumes the old region
        ref->SetVA(access->virt_mem_addr);
        ref->SetPA(access->phy_mem_addr, 0);
        ref->SetSize(access->size);
        return true;
    }

    return false;
};


//
// MergePAandData --
//     The reference's virtual information has already been computed.
//     Add details for the loaded/stored value and the virtual to physical
//     translation.
//
void
ASIM_CPUAPI_INST_INFO_CLASS::MergePAandData(
    cpuapi_inst_memory_access_t *access,
    ASIM_SOFTSDV_MEM_ACCESS ref)
{
    MEMORY_VIRTUAL_REFERENCE_CLASS vr(access->virt_mem_addr, access->size);

    if (ref->ContainsVA(vr))
    {
        if (asim_io.RecordMemoryValues() && (access->data != NULL))
        {
            UINT64 offset = access->virt_mem_addr - ref->GetVA();
            ref->SetValue(access->data, access->size, offset);
        }

        //
        // Ref's PA already has the PA of the first page.  It was computed
        // on the first pass through the access list.  Figure out whether
        // the reference crosses pages.
        //
        UINT64 pa = (access->phy_mem_addr + access->size - 1) & ref->PageMask();
        if (pa != ref->GetPA(0))
        {
            //
            // This page can't possibly be contiguous with the first page
            // in the "small page" space of the memory reference data
            // structure.  Force a second page.
            //

            //
            // SoftSDV has a bug:  it doesn't split some unaligned memory
            // references that cross pages into two references.  As a result
            // it misses the second physical page.  Attempt to recover the
            // second page.
            //
            UINT64 truePA;
            if (cpuInfo[cpuNum].VirtualToPhysical(
                    access->virt_mem_addr + access->size - 1,
                    &truePA, access->access))
            {
                pa = truePA & ref->PageMask();
            }

            ASSERT((ref->GetNPages() == 1) || (ref->GetPA(1) == pa),
                   "Memory reference touches more than 2 pages");
            ref->SetPA(pa, 1);
        }
    }
};


//
// DumpMemoryRegions --
//     Dump memory regions computed from CPUAPI memory reference data.
//
void
ASIM_CPUAPI_INST_INFO_CLASS::DumpMemoryRegions(FILE *f)
{
    for (UINT32 i = 0; i < NLoads(); i++)
    {
        fprintf(f, "      L%d ", i);
        DumpOneMemoryRegion(f, GetLoad(i));
    }
    for (UINT32 i = 0; i < NStores(); i++)
    {
        fprintf(f, "      S%d ", i);
        DumpOneMemoryRegion(f, GetStore(i));
    }
}

//
// DumpOneMemoryRegion --
//     Dump a memory region computed from CPUAPI memory reference data.
//
void
ASIM_CPUAPI_INST_INFO_CLASS::DumpOneMemoryRegion(
    FILE *f,
    const ASIM_SOFTSDV_MEM_ACCESS ref)
{
    fprintf(f, "VA 0x%016llx  ", ref->GetVA());
    if (ref->GetNPages())
    {
        fprintf(f, "PA ");
        for (UINT32 i = 0; i < ref->GetNPages(); i++)
        {
            if (i != 0)
            {
                fprintf(f, "/");
            }
            fprintf(f, "0x%016llx", ref->GetPA(i));
        }
    }

    fprintf(f, "  B %d", ref->GetSize());

    if (ref->HasValue())
    {
        fprintf(f, "  V 0x");
        for (UINT32 i = 0; i < ref->GetSize(); i++)
        {
            fprintf(f, "%02x", ref->GetValue()[i]);
        }
    }

    fprintf(f, "\n");
}


//
// DumpMemoryRefs --
//     Emit debugging information:  dump all of an instruction's references
//     to memory.
//

void
DumpMemoryRef(
    FILE *f,
    const cpuapi_inst_memory_access_t *access,
    const void *data)
{
    char a;
    switch (access->access)
    {
      case CPUAPI_Access_Read:
        a = 'R';
        break;
      case CPUAPI_Access_RFO:
        a = 'O';
        break;
      case CPUAPI_Access_Write:
        a = 'W';
        break;
      case CPUAPI_Access_Execute:
        a = 'E';
        break;
      default:
        a = '?';
        break;
    }

    char o;
    switch (access->origin)
    {
      case CPUAPI_Origin_Other:
        o = 'X';
        break;
      case CPUAPI_Origin_Instruction:
        o = 'I';
        break;
      case CPUAPI_Origin_Descriptor:
        o = 'D';
        break;
      case CPUAPI_Origin_Task:
        o = 'T';
        break;
      default:
        o = '?';
        break;
    }

    fprintf(f, "      O=%c A=%c VA 0x%016llx  PA 0x%016llx  B %lld",
            o, a,
            access->virt_mem_addr, access->phy_mem_addr,
            access->size);

    if (data)
    {
        switch (access->size)
        {
          case 1:
            fprintf(f, "  D 0x%02x", *(UINT8 *)data);
            break;
          case 2:
            fprintf(f, "  D 0x%04x", *(UINT16 *)data);
            break;
          case 4:
            fprintf(f, "  D 0x%08x", *(UINT32 *)data);
            break;
          case 8:
            fprintf(f, "  D 0x%016llx", *(UINT64 *)data);
            break;
          default:
            break;
        }
    }

    fprintf(f, "\n");
}


void
ASIM_CPUAPI_INST_INFO_CLASS::DumpMemoryRefs(FILE *f)
{
    cpuapi_inst_memory_access_t *access;

    access = GetArchInfo()->access_list;
    while (access != NULL)
    {
        DumpMemoryRef(f, access, access->data);
        access = access->next;
    }
}


//
// ASIM_SOFTSDV_FETCH_INST_BUFFER_CLASS represents the array that will
// be passed to SoftSDV's step reference.  It takes care of managing
// SoftSDV version-specific fields and exports itself as an array of
// SoftSDV version-independent instructions.
//
class ASIM_SOFTSDV_FETCH_INST_BUFFER_CLASS
{
  public:
    //
    // The max execution chunk size is the larger of the warmup and the
    // execute sizes.
    //
    enum CONSTS
    {
        maxChunkSize =
            SOFTSDV_WARMUP_CHUNK_SIZE > SOFTSDV_EXECUTE_CHUNK_SIZE ?
                SOFTSDV_WARMUP_CHUNK_SIZE : SOFTSDV_EXECUTE_CHUNK_SIZE
    };

    ASIM_SOFTSDV_FETCH_INST_BUFFER_CLASS(void)
    {
        bzero(sdvInstInfo, sizeof(sdvInstInfo));
        bzero(ia32InstInfo, sizeof(ia32InstInfo));

        //
        // Initialize abstract instruction info class with pointer to
        // version-specific SoftSDV type.
        //
        for (unsigned int i = 0; i < maxChunkSize; i++)
        {
            instInfo[i].Init(&sdvInstInfo[i]);
            sdvInstInfo[i].arch_info_size = sizeof(ia32InstInfo[0]);
            sdvInstInfo[i].arch_info = &ia32InstInfo[i];
        }
    };

    ~ASIM_SOFTSDV_FETCH_INST_BUFFER_CLASS() {};

    //
    // Make the class look like an array of version-independent instructions
    //
    ASIM_CPUAPI_INST_INFO_CLASS & operator[](unsigned int i)
    {
        ASSERTX(i <= maxChunkSize);
        return instInfo[i];
    }

    //
    // Handle to the array passed to SoftSDV's step_reference
    //
    cpuapi_inst_info_t *GetSdvInstInfo(void) { return sdvInstInfo; };

    //
    // Clear n entries.  If no n then clear all entries.
    //
    void Reset(UINT32 cpuNum, UINT32 n = maxChunkSize)
    {
        ASSERTX(n <= maxChunkSize);

        for (UINT32 i = 0; i < n; i++)
        {
            sdvInstInfo[i].inst_id = 0;
            sdvInstInfo[i].icount = 0;
            instInfo[i].Reset(cpuNum);
        }
    };

  private:
    ASIM_CPUAPI_INST_INFO_CLASS instInfo[maxChunkSize];
    cpuapi_inst_info_t sdvInstInfo[maxChunkSize];
    cpuapi_ia32_inst_info_t ia32InstInfo[maxChunkSize];
};


//
// DumpEntireInstrVector -- a simple function to dump all instructions
//     from a group of executed instructions.
//
static void
DumpEntireInstrVector(
    ASIM_SOFTSDV_FETCH_INST_BUFFER_CLASS &instInfo,
    UINT32 nInstrs
)
{
    // Limit the number of instruction vector's that are dumped to avoid
    // filling disks.
    static int counter = 0;
    if (++counter >= 20) return;

    for (UINT32 i = 0; i < nInstrs; i++)
    {
        if ((instInfo[i].GetInstId() == 0) && (instInfo[i].GetIcount() == 0))
        {
            break;
        }

        fprintf(stderr, "%2d   ", i);
        instrDecoder.DisassembleInstr(stderr, instInfo[i]);
        instInfo[i].DumpMemoryRefs();
        instInfo[i].DumpMemoryRegions();
    }
}


//------------------------------------------------------------------------
//
// Per-CPU state
// 
//------------------------------------------------------------------------

void
CPU_INFO_CLASS::InitHandles(const ASIM_REQUESTED_REGS_CLASS &regRqst)
{
    view_cpu_handle = oml_obj_translate_name(GE_VPC_CPU_ID, "notifier.view_cpu");
    ASSERTX(view_cpu_handle);

    cpl_handle = oml_obj_translate_name(GE_VPC_CPU_ID, "data.arch.curr_cpl");
    ASSERTX(cpl_handle);

    ASSERTX(! asimCpucCallBack->translate_object_name(controllerPid,
                                                      CPUAPI_SIM_CPU_MODE,
                                                      &cpuMode_handle));
    ASSERTX(! asimCpucCallBack->translate_object_name(controllerPid,
                                                      CPUAPI_SIM_ALIVE_CPUS,
                                                      &simAliveCpus_handle));
    ASSERTX(! asimCpucCallBack->translate_object_name(controllerPid,
                                                      CPUAPI_SIM_INST_INSTRUCTION_INFO,
                                                      &simInstInfo_handle));

    //
    // This CPUAPI version provides memory value
    //
    ASSERTX(! asimCpucCallBack->translate_object_name(controllerPid,
                                                      CPUAPI_SIM_COLLECT_MEM_VALUES_INFO,
                                                      &simCollectMemValues_handle));

    ASSERTX(! asimCpucCallBack->translate_object_name(controllerPid,
                                                      CPUAPI_IA32_RSP,
                                                      &regRSP_handle));

    ASSERTX(! asimCpucCallBack->translate_object_name(controllerPid,
                                                      CPUAPI_IA32_MSR_GS_BASE,
                                                      &gsBase_handle));

    ASSERTX(! asimCpucCallBack->translate_object_name(controllerPid,
                                                      CPUAPI_IA32_MSR_KERNEL_GS_BASE,
                                                      &kernelGSBase_handle));

    ASSERTX(! asimCpucCallBack->translate_object_name(controllerPid,
                                                      CPUAPI_IA32_FSTATUS,
                                                      &fpStatus_handle));

    //
    // Registers to monitor (requested by Asim)
    //
    monitorRegs = regRqst;
    for (UINT32 i = 0; i < monitorRegs.NRegs(); i++)
    {
        cpuapi_handle_t regHandle;
        const char *regName = monitorRegs.GetRegName(i);

        // CPUAPI doesn't define ST FP registers, just FR.  Check that
        // FR registers are ordered and map them to ST.

        char trueName[64];
        if (! strncmp(regName, "arch.ia32.register.st.", 22))
        {
            // ST is visible as FR in SoftSDV
            strcpy(trueName, regName);
            trueName[19] = 'f';
            trueName[20] = 'r';
            trueName[22] = regName[22];
            regName = trueName;

            unsigned int rNum = regName[22] - '0';
            if (rNum == 0)
            {
                firstSTRegIdx = i;
            }
            lastSTRegIdx = i;
            mapSTRegs = true;
            ASSERT(rNum == i - firstSTRegIdx, "ST floating registers declared out of order");
        }

        if (asimCpucCallBack->translate_object_name(controllerPid,
                                                    regName,
                                                    &regHandle))
        {
            fprintf(stderr, "Can't translate register:  %s\n",
                    monitorRegs.GetRegName(i));
            // FIXME: ASIMERROR("Aborting");
        }
        else
        {
            monitorRegs.SetRegHandle(i, regHandle);
        }
    }
};


void
CPU_INFO_CLASS::CollectMemoryValues(bool collectValues)
{
    cpuapi_u32_t v = collectValues ? 1 : 0;
    if (simCollectMemValues_handle != 0 && asim_io.RecordMemoryValues())
    {
        int ok = asimCpucCallBack->set_object(controllerPid,
                                              simCollectMemValues_handle,
                                              sizeof(v), &v);
        // Only fail if trying to enable values.  The call sometimes fails
        // early in a run.
        ASSERTX((ok == CPUAPI_Stat_Ok) || ! collectValues);
    }
}


void
CPU_INFO_CLASS::SetViewCPU(void)
{
    //
    // Make sure view cpu is correct
    //
    if (asim_io.NCpus() > 1)
    {
        UINT32 viewCPU = cpuNum;
        ASSERTX(GE_OK == oml_set_value(GE_VPC_CPU_ID, view_cpu_handle, 4, &viewCPU));
    }
}


UINT32
CPU_INFO_CLASS::CurrentPrivLevel(void)
{
    UINT32 cpl;
    UINT32 size;

    size = sizeof(cpl);
    ASSERTX(GE_OK == oml_get_value(GE_VPC_CPU_ID, cpl_handle, &size, &cpl));

    return cpl;
}


UINT32
CPU_INFO_CLASS::GetMonitoredRegValue(
    UINT32 regIdx,
    void *value,
    UINT32 size)
{
    UINT32 actualSize = monitorRegs.GetRegSize(regIdx);
    ASSERTX(actualSize <= size);

    //
    // Is this an ST floating pointer register request?  Remap to
    // FR if yes.
    //
    if (mapSTRegs && (firstSTRegIdx <= regIdx) && (regIdx <= lastSTRegIdx))
    {
        UINT32 rNum = regIdx - firstSTRegIdx;
        rNum = (rNum + ((FPStatus() & 0x3800) >> 11)) & 7;
        regIdx = firstSTRegIdx + rNum;
        ASSERTX(regIdx <= lastSTRegIdx);
    }

    if (asimCpucCallBack->get_object(controllerPid, 0,
                                     monitorRegs.GetRegHandle(regIdx),
                                     actualSize,
                                     value) != CPUAPI_Stat_Ok)
    {
        fprintf(stderr, "Failed to get value of %s\n",
                monitorRegs.GetRegName(regIdx));
        // FIXME: ASIMERROR("Aborting...");
    }

    return actualSize;
}


UINT64
CPU_INFO_CLASS::RegRSP(void)
{
    UINT64 value;

    ASSERTX(asimCpucCallBack->get_object(controllerPid, 0,
                                         cpuInfo[cpuNum].regRSP_handle,
                                         sizeof(value),
                                         &value) == CPUAPI_Stat_Ok);
    return value;
}

UINT64
CPU_INFO_CLASS::GSBase(void)
{
    UINT64 value;

    ASSERTX(asimCpucCallBack->get_object(controllerPid, 0,
                                         cpuInfo[cpuNum].gsBase_handle,
                                         sizeof(value),
                                         &value) == CPUAPI_Stat_Ok);
    return value;
}

UINT64
CPU_INFO_CLASS::KernelGSBase(void)
{
    UINT64 value;

    ASSERTX(asimCpucCallBack->get_object(controllerPid, 0,
                                         cpuInfo[cpuNum].kernelGSBase_handle,
                                         sizeof(value),
                                         &value) == CPUAPI_Stat_Ok);
    return value;
}

UINT32
CPU_INFO_CLASS::FPStatus(void)
{
    UINT16 value;

    ASSERTX(asimCpucCallBack->get_object(controllerPid, 0,
                                         cpuInfo[cpuNum].fpStatus_handle,
                                         sizeof(value),
                                         &value) == CPUAPI_Stat_Ok);
    return value;
}

bool
CPU_INFO_CLASS::ReadMemory(
    UINT64 pa,
    cpuapi_size_t size,
    void *buf)
{
#if CPUAPI_VERSION_MAJOR(CPUAPI_VERSION_CURRENT) >= 9
    cpuapi_size_t hptrSize = size;
    cpuapi_hptr_t hptrBuf = NULL;
    
    if (CPUAPI_Stat_Ok !=
        asimCpucCallBack->get_hptr(controllerPid, CPUAPI_Access_Read,
                                   pa, &hptrSize, &hptrBuf))
    {
        return false;
    }

    memcpy(buf, hptrBuf, size);

    ASSERTX(CPUAPI_Stat_Ok == asimCpucCallBack->invalidate_hptr(controllerPid, pa));

    return true;
#else
    // Older versions used mem_read and not get_hptr for accessing memory
    return CPUAPI_Stat_Ok == asimCpucCallBack->mem_read(controllerPid,
                                                        pa,
                                                        size,
                                                        buf);
#endif
}


bool
CPU_INFO_CLASS::VirtualToPhysical(
    UINT64 va,
    UINT64 *pa,
    cpuapi_access_type_t access)
{
    cpuapi_stat_t result;

    result = asimCpucCallBack->translate_virt_addr(controllerPid,
                                                   va,
                                                   access,
                                                   pa);
    if (result != CPUAPI_Stat_Ok)
    {
        *pa = 0;
        return false;
    }
    return true;
}


//------------------------------------------------------------------------
//
// History class.  Holds cached machine state for a single CPU.  Useful
// for tracking input register values to the current instruction.
// 
//------------------------------------------------------------------------


struct UINT64ArrayPair
{
    UINT64 v[2];
};


class CPU_HISTORY_CLASS
{
  public:

    CPU_HISTORY_CLASS(UINT32 cpuNum) :
        next_ip_va(0),
        ip_va(0),
        rsp(0),
        cpuNum(cpuNum),
        currPid(-1),
        newSequence(false),
        prev_was_rfi(false),
        interrupt_pending(false),
        newContext(false),
        contextIsReal(false)
    {
        currProcName[0] = 0;
    };

    ~CPU_HISTORY_CLASS() {};
    
    //
    // Update history given this new executed instruction.  If you have already
    // decoded the instruction pass the decoder result to the function.  If
    // not, the function will decode the instruction on its own.
    //
    void ExecutedInstruction(
        const ASIM_CPUAPI_INST_INFO_CLASS &instInfo,
        IA_Decoder_Info *decode);

    //
    // Was the previous instruction a decoded opcode?
    //
    bool PrevOpRfi(void) const { return prev_was_rfi; };

    UINT64 NextIP_Virtual(void) const { return next_ip_va; };
    UINT64 LastExecutedIP_Virtual(void) const { return ip_va; };

    bool IsNewSequence(void) const { return newSequence; };
    bool InterruptPending(void) const { return interrupt_pending; };
    
    void SetNewSequence(void) { newSequence = true; };

    //
    // NewContext indicates whether a context switch has happened.  It
    // gets set when a new context is detected and cleared when the new
    // context has been passed to Asim.
    // 
    //
    bool IsNewContext(void) const { return newContext; };
    INT32 GetPid(void) const { return currPid; };
    const char *GetProcName(void) const { return currProcName; };
    void ClearIsNewContext(void) { newContext = false; };
    void SetNewContext(INT32 pid, const char *procName, bool isReal);
    bool ContextIsReal(void) const { return contextIsReal; };

    //
    // RSP is used to find context info.  Record the last RSP seen as
    // a quick test for whether the context might have changed.
    //
    UINT64 GetLastRSP(void) { return rsp; };
    void SetLastRSP(UINT64 newRSP) { rsp = newRSP; };

  private:
    UINT64 next_ip_va;
    UINT64 ip_va;
    UINT64 rsp;

    const UINT32 cpuNum;

    INT32 currPid;
    char currProcName[32];

    //
    // Flags
    //
    bool newSequence;       // First instruction after fast-forward
    bool prev_was_rfi;
    bool interrupt_pending;
    bool newContext;        // Context switch noted.  This flag remains set
                            // until the new context as passed to Asim.
    bool contextIsReal;     // Is context real or a fake pid?
};

typedef CPU_HISTORY_CLASS *CPU_HISTORY;


inline void
CPU_HISTORY_CLASS::SetNewContext(INT32 pid, const char *procName, bool isReal)
{
    INT32 oldPid = currPid;

    newContext = true;
    contextIsReal = isReal;
    currPid = pid;
    strncpy(currProcName, procName, sizeof(currProcName));
    currProcName[sizeof(currProcName)-1] = 0;

#if 0
    if (isReal && (pid != oldPid))
    {
        printf("New context cpu %d, pid %d", cpuNum, currPid);
        if (currProcName[0])
        {
            printf(", name %s\n", currProcName);
        }
        else
        {
            printf("\n");
        }
    }
#endif
};


void
CPU_HISTORY_CLASS::ExecutedInstruction(
    const ASIM_CPUAPI_INST_INFO_CLASS &instInfo,
    IA_Decoder_Info *decode)
{
    newSequence = false;

    ip_va = instInfo.GetVA();

    IA_Decoder_Info localDecode;
    if (decode == NULL)
    {
        if (CPUAPI_Stat_Ok == instrDecoder.DecodeInstr(instInfo.GetInstBytes(),
                                                       instInfo.GetNInstBytes(),
                                                       instInfo.GetCpuMode(),
                                                       &localDecode))
        {
            decode = &localDecode;
        }
    }

    //
    // SoftSDV will fetch a HLT instruction forever.  Don't.
    //
    if (decode != NULL && decode->inst == X86_HLT)
    {
        fprintf(stderr, "SoftSDV executed HLT instruction.  Exiting SoftSDV...\n");
        asim_io.Exiting(false);
        exit(0);
    }

    //
    // RFI instruction?
    //
    prev_was_rfi = (decode &&
                    ((decode->inst == X86_IRET) ||
                     (decode->inst == X86_IRETD) ||
                     (decode->inst == X86_IRETQ)));

    //
    // Compute next IP
    //
    if (instInfo.GetIsBranchInst() && instInfo.GetBranchTaken())
    {
        next_ip_va = instInfo.GetBranchTargetVirtAddr();
    }
    else
    {
        next_ip_va = instInfo.GetVA() + instInfo.GetNInstBytes();
    }

    //
    // x86 Gambit signals a pending interrupt by "executing" an instruction
    // at VA 0.
    //
    interrupt_pending = (ip_va == 0);
};
    

DYNAMIC_ARRAY_CLASS<CPU_HISTORY> cpuHistory;


//
// SetContextFromProcessorDataArray ==
//     Read pid and process name from the pda (processor data array).  Two
//     CPU numbers are passed as parameters.  One CPU is used for virtual
//     to physical translation and the other for the CPU whose pid we are
//     seeking.  Early in a run we may try to find the pid of a process
//     in user space using kernel access from another CPU.  The kernel page
//     translations aren't available in user space.
//
bool
SetContextFromProcessorDataArray(
    UINT32 cpuNum,
    UINT32 privCpuNum,
    UINT64 pdaVA)
{
    UINT64 pa;

    if (cpuInfo[privCpuNum].VirtualToPhysical(pdaVA, &pa, CPUAPI_Access_Read))
    {
        UINT64 taskVA;
        UINT64 taskPA;

        if (cpuInfo[privCpuNum].ReadMemory(pa, 8, &taskVA) &&
            cpuInfo[privCpuNum].VirtualToPhysical(taskVA, &taskPA, CPUAPI_Access_Read))
        {
            INT32 pid;
            char procName[17];

            ASSERTX(cpuInfo[privCpuNum].ReadMemory(taskPA + asimGuestOSInfo.GetPidOffset(), 4, &pid));
            ASSERTX(cpuInfo[privCpuNum].ReadMemory(taskPA + asimGuestOSInfo.GetProcNameOffset(), 16, procName));

            procName[16] = 0;   // Make sure name is NULL terminated
            cpuHistory[cpuNum]->SetNewContext(pid, procName, true);

            return true;
        }
    }

    return false;
}



//
// NoteContextSwitchGS --
//     For guest OS's on which you can find the task_struct at any time,
//     determine the current context info.
//
bool
NoteContextSwitchGS(
    UINT32 cpuNum,
    UINT32 currPrivLevel)
{
    //
    // Only methods that require polling are considered here.
    //
    ASIM_FIND_TASK_STRUCT method = asimGuestOSInfo.GetFindTaskMethod();
    if (method != TASK_STRUCT_GS)
    {
        return false;
    }

    UINT64 pdaVA = 0;
    if (currPrivLevel == 0)
    {
        // We are in the kernel.  Assume GS points to the PDA.
        pdaVA = cpuInfo[cpuNum].GSBase();

        // There is a very small window where a CPU can be in ring 0 but GS isn't
        // set up with the kernel value.  Here is a quick check that will
        // usually work.
        if (pdaVA < asimGuestOSInfo.GetPDAArrayAddr())
        {
            pdaVA = cpuInfo[cpuNum].KernelGSBase();
        }
    }
    else
    {
        pdaVA = cpuInfo[cpuNum].KernelGSBase();
    }

    if (pdaVA < asimGuestOSInfo.GetPDAArrayAddr())
    {
        // Something is wrong.  Give up.
        return false;
    }

    bool prevWasRealContext = cpuHistory[cpuNum]->ContextIsReal();

    if (! SetContextFromProcessorDataArray(cpuNum, cpuNum, pdaVA))
    {
        return false;
    }

    if (! prevWasRealContext)
    {
        // Try to get the context for all CPUs now that one has privs
        for (UINT32 c = 0; c < asim_io.NCpus(); c++)
        {
            if (! cpuHistory[c]->ContextIsReal())
            {
                cpuInfo[c].SetViewCPU();

                pdaVA = (cpuInfo[c].CurrentPrivLevel() == 0) ?
                        cpuInfo[c].GSBase() :
                        cpuInfo[c].KernelGSBase();

                cpuInfo[cpuNum].SetViewCPU();

                SetContextFromProcessorDataArray(c, cpuNum, pdaVA);
            }
        }
    }

    return true;
}


//
// PollForContextSwitch --
//     Probe the machine state and see whether a context switch has happened.
//
void
PollForContextSwitch(
    UINT32 cpuNum,
    UINT32 currPrivLevel)
{
    ASIM_FIND_TASK_STRUCT method = asimGuestOSInfo.GetFindTaskMethod();
    if (asimGuestOSInfo.GetPDAContextSwitchIP() != 0)
    {
        // No need to poll for context switch since we can identify when
        // it happens by IP.  Only poll when the context is unknown.  This
        // can happen at the beginning of a sequence when it starts in user
        // space.
        if (cpuHistory[cpuNum]->ContextIsReal())
        {
            return;
        }
    }
    else if ((method != TASK_STRUCT_RSP) && (method != TASK_STRUCT_RSP_INDIRECT))
    {
        return;
    }
        
    // Are we in the kernel?
    if (currPrivLevel != 0)
    {
        return;
    }

    if (method == TASK_STRUCT_GS)
    {
        NoteContextSwitchGS(cpuNum, currPrivLevel);
    }
    else
    {
        // "current" in Linux is RSP & ~8191.  Did "current" change since
        // the last time we looked?
        UINT64 rsp = cpuInfo[cpuNum].RegRSP() & ~8191;
        if (rsp != cpuHistory[cpuNum]->GetLastRSP())
        {
            cpuHistory[cpuNum]->SetLastRSP(rsp);

            //
            // Get the physical address of "current".
            //
            UINT64 pa;
            if (cpuInfo[cpuNum].VirtualToPhysical(rsp, &pa, CPUAPI_Access_Read))
            {
                UINT64 taskPA = 0;

                switch (asimGuestOSInfo.GetFindTaskMethod())
                {
                  case TASK_STRUCT_RSP:
                    // task_struct is at "current"
                    taskPA = pa;
                    break;

                  case TASK_STRUCT_RSP_INDIRECT:
                    // task_struct is indirect through "current"
                    UINT64 taskVA;
                    if (! cpuInfo[cpuNum].ReadMemory(pa, 4, &taskVA) ||
                        ! cpuInfo[cpuNum].VirtualToPhysical(taskVA, &taskPA, CPUAPI_Access_Read))
                    {
                        return;
                    }
                    break;

                  default:
                    ASIMERROR("Unknown method of finding task");
                }
                
                INT32 pid;
                char procName[17];

                ASSERTX(cpuInfo[cpuNum].ReadMemory(taskPA + asimGuestOSInfo.GetPidOffset(), 4, &pid));
                ASSERTX(cpuInfo[cpuNum].ReadMemory(taskPA + asimGuestOSInfo.GetProcNameOffset(), 16, procName));

                procName[16] = 0;   // Make sure name is NULL terminated
                cpuHistory[cpuNum]->SetNewContext(pid, procName, true);
            }
        }
    }
}


//------------------------------------------------------------------------
//
// Helper functions for ExecuteAsim.  Mainly for filling in fields
// communicated to Asim.
// 
//------------------------------------------------------------------------

//
// NoteRegisterValues --
//     Called when the machine state changes and a new register state
//     must be sent to Asim.
//
UINT64
NoteRegisterValues(
    cpuapi_u32_t cpuNum)
{
    static UINT64 uid = 0;
    const ASIM_REQUESTED_REGS_CLASS *mRegs = cpuInfo[cpuNum].MonitorRegs();

    if (mRegs->NRegs() == 0)
    {
        return 0;
    }

    ASIM_SOFTSDV_REG_INFO regs = asim_io.RegValues().OpenNext();

    regs->Reset();

    uid += 1;
    regs->SetUid(uid);
    regs->SetCpuNum(cpuNum);

    for (UINT32 i = 0; i < mRegs->NRegs(); i++)
    {
        unsigned char value[16];
        UINT32 trueSize = cpuInfo[cpuNum].GetMonitoredRegValue(i, value, sizeof(value));

        ARCH_REGISTER_CLASS r(REG_TYPE_RAW, mRegs->GetAsimRegNum(i),
                              value, trueSize);

        regs->AddRegister(r);
    }

    asim_io.RegValues().Close(regs);

    return uid;
}


//
// MemWriteInfoValid --
//    Look at the decoder information and the memory write information
//    returned by SoftSDV.  Verify that SoftSDV has the data we expect.
//
inline bool
MemWriteInfoValid(
    ASIM_CPUAPI_INST_INFO_CLASS &instInfo,
    const IA_Decoder_Info &decode)
{
    bool valid = true;

    if (! instInfo.NStores() &&
        ((decode.dst1.type == IA_DECODER_MEMORY) ||
         ((decode.src1.type == IA_DECODER_MEMORY) &&
          (decode.src1.oper_2nd_role == IA_DECODER_OPER_2ND_ROLE_DST)) ||
         ((decode.src2.type == IA_DECODER_MEMORY) &&
          (decode.src2.oper_2nd_role == IA_DECODER_OPER_2ND_ROLE_DST))))
    {
        valid = false;
    }

    return valid;
}


//
// MemReadInfoValid --
//    Look at the decoder information and the memory read information
//    returned by SoftSDV.  Verify that SoftSDV has the data we expect.
//
inline bool
MemReadInfoValid(
    ASIM_CPUAPI_INST_INFO_CLASS &instInfo,
    const IA_Decoder_Info &decode)
{
    bool valid = true;

    if (! instInfo.NLoads() &&
        ((decode.src1.type == IA_DECODER_MEMORY) ||
         (decode.src2.type == IA_DECODER_MEMORY) ||
         ((decode.dst1.type == IA_DECODER_MEMORY) &&
          (decode.dst1.oper_2nd_role == IA_DECODER_OPER_2ND_ROLE_SRC))))
    {
        switch (decode.inst)
        {
          case X86_LEAW:
          case X86_LEAL:
          case X86_LEAQ:
          case X86_INVLPG:
          case X86_PREFETCHT0_MME:
          case X86_PREFETCHT1_MME:
          case X86_PREFETCHT2_MME:
          case X86_PREFETCHNTA_MME:
            // Ignore these instructions.  They don't actually
            // read memory.
            break;

          case X86_MOVSS_R_VXF:
          case X86_MOVSD_MR_WMT:
            // Decoder can be wrong here.  It claims the destination
            // memory is also read for movsd <mem> = xmm0
            break;

          default:
            valid = false;
        }
    }

    return valid;
}

//------------------------------------------------------------------------
//
// ISA specific functions exported to CPUAPI
// 
//------------------------------------------------------------------------

cpuapi_stat_t
ExecuteAsim(cpuapi_cid_t cid,
            cpuapi_step_t step_type,
            cpuapi_u64_t requested,
            cpuapi_u64_t *actual)
{
    static ASIM_SOFTSDV_FETCH_INST_BUFFER_CLASS instInfo;

    //
    // These variables hold information about a previous fetch request from
    // Asim that wasn't completed on the last pass.
    //
    static UINT32 fetchCpuNum = 0;
    static UINT32 fetchInstrsRemaining = 0;

    cpuapi_u32_t cpuNum;

    //
    // Performance model runs only in cycles
    //
    if (CPUAPI_Step_Cycles != step_type)
    {
        return CPUAPI_Stat_Err;
    }

    cpuNum = (cpuapi_u32_t) cid;

    if (! cpuInfo[cpuNum].StateValid())
    {
        //
        // For some modes with pid isn't available until a trap to the
        // kernel.  Make something up for initialization.
        //
        cpuHistory[cpuNum]->SetNewContext(32767 - cpuNum, "<unknown>", false);
        cpuHistory[cpuNum]->SetNewSequence();

        cpuInfo[cpuNum].SetStateValid();

        *actual = requested;
        return CPUAPI_Stat_Ok;
    }

    if (asim_io.AsimPid())
    {
        //
        // Connected to Asim.  Wait for a fetch request...
        //
        if (! fetchInstrsRemaining)
        {
            ASIM_SOFTSDV_REQUEST fetchRequest;

            fetchRequest = asim_io.AsimRequestQueue().OpenNext();
            if (fetchRequest == NULL)
            {
                if (asim_io.AsimIsActive())
                {
                    ASIMERROR("Remote Asim process exited unexpectedly");
                }
                else
                {
                    fprintf(stderr, "Asim is done.  SoftSDV exiting.\n");
                    asim_error_exit(0);
                }
            }
            fetchCpuNum = fetchRequest->CpuNum();
            fetchInstrsRemaining = fetchRequest->NInstrs();

            asim_io.AsimRequestQueue().Close(fetchRequest);
        }

        //
        // Is the fetch request for the current CPU?
        //
        if (fetchCpuNum != cpuNum)
        {
            //
            // Not for current CPU.  Return.
            //
            // Always claim we executed the instructions, even if we didn't.
            // Returning 0 causes CPUAPI to try again, when what we really want
            // is to move on to the next CPU.
            //
            *actual = requested;
            return CPUAPI_Stat_Ok;
        }
    }
    else
    {
        fetchInstrsRemaining = cpuInfo[cpuNum].StateWarmup() ?
                               SOFTSDV_WARMUP_CHUNK_SIZE :
                               SOFTSDV_EXECUTE_CHUNK_SIZE;
    }

    //
    // Do cached copies of registers match?
    //
    UINT64 expectedIP = cpuHistory[cpuNum]->NextIP_Virtual();

    bool earlyTermination = false;

    while ((fetchInstrsRemaining != 0) && ! earlyTermination)
    {
        //
        // In some models we want to fetch something other than the declared
        // chunk size.  Here is the chance.
        //
        unsigned int nInstrsToFetch = fetchInstrsRemaining;

        if (ASIM_SOFTSDV_FETCH_INST_BUFFER_CLASS::maxChunkSize < nInstrsToFetch)
        {
            fprintf(stderr, "Fetch chunk of %d bytes\n", nInstrsToFetch);
            ASIMERROR("Fetch chunk too large");
        }

        ASSERT(asim_io.InstrRing(cpuNum).NWriteSlotsAvailable() >= nInstrsToFetch,
               "No instruction ring write slots available");

        //
        // If we're in the kernel try to figure out which task is running.
        //
        cpuInfo[cpuNum].SetViewCPU();
        INT32 currPrivLevel = cpuInfo[cpuNum].CurrentPrivLevel();
        PollForContextSwitch(cpuNum, currPrivLevel);

        //
        // Has the machine state changed?  If so, get all the monitored
        // register values.
        //
        //           ******************************************
        //     This code requires that traps to the kernel be the first
        //     instruction in a step_reference() group.  step_reference()
        //     MUST end a group early if the privilege level changes.
        //     For now this is done by setting an event breakpoint on
        //     event.arch.cpl_change in the Perl script controlling the
        //     run.  Failure to meet this requirement will most likely
        //     cause the Archlib feeder to enter infinite fault loops on
        //     kernel traps due to protection violations.  Unfortunately
        //     there is no good way to detect that the condition has not
        //     been met other than the Archlib feeder triggering faults.
        //           ******************************************
        //
        UINT64 regsUid = 0;
        if (! cpuInfo[cpuNum].StateWarmup() &&
            (cpuHistory[cpuNum]->IsNewSequence() ||
             cpuHistory[cpuNum]->InterruptPending()))
        {
            regsUid = NoteRegisterValues(cpuNum);
        }

        instInfo.Reset(cpuNum, nInstrsToFetch);

        asimCpucCallBack->step_reference(cpuInfo[cpuNum].ControllerPid(),
                                         instInfo.GetSdvInstInfo(),
                                         nInstrsToFetch);

        unsigned int nInstrsFetched = 0;
        for (unsigned int i = 0; i < nInstrsToFetch; i++)
        {
            if ((instInfo[i].GetInstId() == 0) && (instInfo[i].GetIcount() == 0))
            {
                //
                // Assume early termination (break point, end of workload, etc.)
                //
                earlyTermination = true;
                break;
            }

            //
            // IP 0 is the start of an exception.  All other addresses are valid
            // instructions.
            //
            if (instInfo[i].GetVA() != 0)
            {
                nInstrsFetched += 1;

                if (lockStep != NULL)
                {
                    lockStep->NewInstr(cpuNum, instInfo[i]);
                }

                ASIM_SOFTSDV_INST_INFO inst = asim_io.InstrRing(cpuNum).OpenNext();
                ASSERT(inst != NULL, "Remote Asim process exited");

                IA_Decoder_Info decode;
                if (CPUAPI_Stat_Ok != instrDecoder.DecodeInstr(instInfo[i].GetInstBytes(),
                                                               instInfo[i].GetNInstBytes(),
                                                               instInfo[i].GetCpuMode(),
                                                               &decode))
                {
                    fprintf(stderr,
                            "Decode error:  CPU %d, i %d, IP 0x%016llx\n",
                            cpuNum, i, instInfo[i].GetVA());
                    DumpEntireInstrVector(instInfo, nInstrsToFetch);
                }

//                if (instInfo[i].GetVA() == 0x40c3a4)
//                {
//                    DumpEntireInstrVector(instInfo, nInstrsToFetch);
//                }

                //
                // Do some quick validation that the memory information
                // returned by SoftSDV matches the expectationso of the
                // decoder.
                //
                if (! MemReadInfoValid(instInfo[i], decode))
                {
                    fprintf(stderr,
                            "Missing READ memory info:  CPU %d, i %d, IP 0x%016llx\n",
                            cpuNum, i, instInfo[i].GetVA());
                    DumpEntireInstrVector(instInfo, nInstrsToFetch);
                }

                if (! MemWriteInfoValid(instInfo[i], decode))
                {
                    fprintf(stderr,
                            "Missing WRITE memory info:  CPU %d, i %d, IP 0x%016llx\n",
                            cpuNum, i, instInfo[i].GetVA());
                    DumpEntireInstrVector(instInfo, nInstrsToFetch);
                }

                //
                // Check expected IP and physical address against actual result
                //
                if (cpuHistory[cpuNum]->PrevOpRfi())
                {
                    inst->SetReturnFromInterrupt(true);
                }

                if (currPrivLevel == 0)
                {
                    inst->SetKernelInstr(true);
                }

                //
                // Adjust the expected next IP for repeating instructions
                //
                if (instInfo[i].GetRepCount() > 0)
                {
                    //
                    // Repeating instructions may branch to themselves
                    // without signalling a branch.
                    //
                    if (cpuHistory[cpuNum]->LastExecutedIP_Virtual() == instInfo[i].GetVA())
                    {
                        expectedIP = instInfo[i].GetVA();
                    }
                }

                //
                // Is the virtual address what we expected?  If not, confirm
                // that it is an interrupt.
                //
                bool interrupt = false;
                if ((expectedIP != instInfo[i].GetVA()) &&
                    ! cpuHistory[cpuNum]->IsNewSequence())
                {
                    interrupt = true;
                    if (! cpuHistory[cpuNum]->InterruptPending())
                    {
                        fprintf(stderr,
                                "Expected IP error:  CPU %d, i %d, expect 0x%016llx, actual 0x%016llx\n",
                                cpuNum, i, expectedIP, instInfo[i].GetVA());
                        DumpEntireInstrVector(instInfo, nInstrsToFetch);
                    }
                }

                //
                // Check for some error conditions around kernel traps and
                // interrupt handling.  Register values must be collected
                // on transitions into the kernel.
                //
                if (cpuHistory[cpuNum]->InterruptPending() &&
                    cpuInfo[cpuNum].MonitorRegs()->NRegs() &&
                    ! cpuInfo[cpuNum].StateWarmup())
                {
                    const char *err = NULL;
                    if (i > 0)
                    {
                        err = "Interrupt in the middle of a group";
                    }
                    else if (regsUid == 0)
                    {
                        err = "No register values collected following interrupt";
                    }

                    if (err != NULL)
                    {
                        fprintf(stderr, "%s:  CPU %d, i %d, IP 0x%016llx\n",
                                err, cpuNum, i, instInfo[i].GetVA());
                        DumpEntireInstrVector(instInfo, nInstrsToFetch);
                        exit(1);
                    }
                }

                if (regsUid != 0)
                {
                    inst->SetRegsUid(regsUid);
                    regsUid = 0;
                }

                if (cpuInfo[cpuNum].StateWarmup())
                {
                    //
                    // The script controlling SoftSDV has tagged the current
                    // region as warm-up before actual simulation.
                    //
                    inst->SetWarmUp(true);
                }

                if (cpuHistory[cpuNum]->IsNewSequence())
                {
                    inst->SetNewSequence(true);
                }

                if (interrupt)
                {
                    inst->SetInterrupt(true);
                }

                if ((instInfo[i].GetVA() == asimGuestOSInfo.GetTimerInterruptVector()) ||
                    (instInfo[i].GetVA() == asimGuestOSInfo.GetLocalTimerInterruptVector()))
                {
                    inst->SetTimerInterrupt(true);
                }

                if (instInfo[i].GetVA() == asimGuestOSInfo.GetIdleAddr())
                {
                    inst->SetIdleInstr(true);
                }

                if ((decode.inst == X86_PAUSE_WMT) ||
                    (decode.inst == X86_MWAIT_PSC))
                {
                    inst->SetPauseInstr(true);
                    INT32 pid = cpuHistory[cpuNum]->GetPid();
                    if ((pid == 0) || (pid == -1))
                    {
                        // In the kernel and the pid is either 0 or unknown.
                        // Assume we're in the idle loop.
                        inst->SetIdleInstr(true);
                    }
                }

                static UINT64 uid = 0;
                uid += 1;
                inst->SetUid(uid);

                inst->SetCpuMode(instInfo[i].GetAsimCpuMode());
                inst->SetSoftsdvId(instInfo[i].GetInstId());
                inst->SetInstrBytes(instInfo[i].GetInstBytes(), instInfo[i].GetNInstBytes());
                inst->SetAddrPhysical(instInfo[i].GetPA());
                inst->SetAddrVirtual(instInfo[i].GetVA());

                //
                // Monitor context switches
                //
                if (cpuHistory[cpuNum]->IsNewSequence() ||
                    instInfo[i].GetVA() == asimGuestOSInfo.GetPDAContextSwitchIP())
                {
                    NoteContextSwitchGS(cpuNum, currPrivLevel);
                }

                if (cpuHistory[cpuNum]->IsNewContext() && !cpuInfo[cpuNum].StateWarmup())
                {
                    inst->NoteContextSwitch(cpuHistory[cpuNum]->GetPid(),
                                            cpuHistory[cpuNum]->GetProcName());
                    cpuHistory[cpuNum]->ClearIsNewContext();
                }


                if (instInfo[i].GetIsBranchInst())
                {
                    //
                    // Try to find the physical page of the target.  This helps
                    // Asim's instruction virtual to physical translation prediction.
                    //
                    UINT64 pa;
                    cpuInfo[cpuNum].VirtualToPhysical(instInfo[i].GetBranchTargetVirtAddr(), &pa);

                    inst->SetControl(instInfo[i].GetBranchTaken(),
                                     instInfo[i].GetBranchTargetVirtAddr(),
                                     pa);
                }

                if (instInfo[i].GetRepCount() > 0)
                {
                    inst->SetRepeat();
//                    fprintf(stderr, "%d  ", i);
//                    instrDecoder.DisassembleInstr(stderr, instInfo[i]);
//                    instInfo[i].DumpMemoryRefs();
//                    instInfo[i].DumpMemoryRegions();
                }

                //
                // I/O instructions
                //
                if (instInfo[i].GetIsIO())
                {
                    inst->SetIOInfo(instInfo[i].GetIOPort(),
                                  instInfo[i].GetIOValue(),
                                  instInfo[i].GetIOSize());
                }

                if (instInfo[i].NLoads() || instInfo[i].NStores())
                {
                    inst->SetMemRefs(instInfo[i].GetMemRefs());
                }

                asim_io.InstrRing(cpuNum).Close(inst);

                cpuHistory[cpuNum]->ExecutedInstruction(instInfo[i], &decode);
            }
            else
            {
                // ExecutedInstructions must be called even for a dummy instr
                cpuHistory[cpuNum]->ExecutedInstruction(instInfo[i], NULL);
            }

            //
            // Update history for the new instruction
            //
            expectedIP = cpuHistory[cpuNum]->NextIP_Virtual();
        }

        fetchInstrsRemaining -= nInstrsFetched;
    }
    
    //
    // Signal Asim data available
    //
    asim_io.SendSyncPacket();

    //
    // Always claim we executed the instructions, even if we didn't.  Returning
    // 0 causes CPUAPI to try again, when what we really want is to move on
    // to the next CPU.
    //
    *actual = requested;

    return CPUAPI_Stat_Ok;
}


/*==============================================================================
Function: io_port_access
Description:
    Access a port.
Input:
	cid - Unique ID identifying the Cpu model that is the target for this call
   paddr - Port physical address
   size - the requested data size
Output:
	buffer
Assumptions:
	none
==============================================================================*/
cpuapi_stat_t io_port_access(cpuapi_cid_t         cid,
                                         cpuapi_phys_addr_t   paddr,
                                         cpuapi_size_t        size,
                                         cpuapi_access_type_t access,
                                         void*                buffer)
{
    return CPUAPI_Stat_Err;
}

/*==============================================================================
Function: processor_interrupt_block_access

Description:
  Access sapic memory

Input:
  cid - Unique ID identifying the Cpu model that is the target for this call
  paddr - Sapic memory physical address
  size - the requested data size
  access - read/write

Output:
  buffer

Assumptions:
  none
==============================================================================*/

EXPORT_FUNC cpuapi_stat_t
processor_interrupt_block_access(
    cpuapi_cid_t         cid,
    cpuapi_phys_addr_t   paddr,
    cpuapi_size_t        size,
    cpuapi_access_type_t access,
    void*                buffer)
{
    return CPUAPI_Stat_Err;
}


void *
CPUAPI_ArchSpecificClbks(void)
{
    return NULL;
}


void
AsimInit_ArchSpecific(void)
{
    cpuInfo.Init(asim_io.NCpus());

    cpuHistory.Init(asim_io.NCpus());
    for (UINT32 i = 0; i < asim_io.NCpus(); i++)
    {
        cpuHistory[i] = NULL;
    }
}


void
AsimInitCpu_ArchSpecific(
    UINT32 cpuNum,
    const ASIM_REQUESTED_REGS_CLASS &regRqst)
{
    cpuInfo[cpuNum].InitHandles(regRqst);

    if (cpuHistory[cpuNum])
    {
        delete cpuHistory[cpuNum];
    };

    cpuHistory[cpuNum] = new CPU_HISTORY_CLASS(cpuNum);

    if (cpuNum == 0)
    {
        sysInfo.Init();
        instrDecoder.Init();
    }
}


//------------------------------------------------------------------------
//
// Instruction decoder/disassembler -- implementation
// 
//------------------------------------------------------------------------

INSTR_DECODER_CLASS::INSTR_DECODER_CLASS(void) :
    initialized(false)
{
    //
    // The decoder and disassembler libraries aren't loaded by default...
    //
    decoderHandle = dlopen("libdisp69.so", RTLD_LAZY | RTLD_GLOBAL);
    ASSERT(decoderHandle != NULL, "Asim: SoftSDV failed to load libdisp69.so");

    disasmHandle = dlopen("libdecp69.so", RTLD_LAZY | RTLD_GLOBAL);
    ASSERT(disasmHandle != NULL, "Asim: SoftSDV failed to load libdecp69.so");
}
    

INSTR_DECODER_CLASS::~INSTR_DECODER_CLASS()
{
    ia_decoder_close(id);
    dlclose(disasmHandle);
    dlclose(decoderHandle);
};


void
INSTR_DECODER_CLASS::Init(void)
{
    id = ia_decoder_open();
    if (-1 == id)
    {
        fprintf(stderr, "Asim: SoftSDV failed to load decoder\n");
        exit(1);
    }

    IA_Decoder_Machine_Type m_type = IA_DECODER_CPU_P6;

    switch (sysInfo.TargetProc())
    {
      case GE_PROCESSOR_P5:
        m_type = IA_DECODER_CPU_PENTIUM;
        break;
      case GE_PROCESSOR_P6:
        m_type = IA_DECODER_CPU_P6;
        break;
      case GE_PROCESSOR_P5MM:
        m_type = IA_DECODER_CPU_P5MM;
        break;
      case GE_PROCESSOR_P6MM:
        m_type = IA_DECODER_CPU_P6MM;
        break;
      case GE_PROCESSOR_P6MM_EX:
        m_type = IA_DECODER_CPU_P6MMX;
        break;
      case GE_PROCESSOR_P6KATNI:
        m_type = IA_DECODER_CPU_P6_KATNI;
        break;
      case GE_PROCESSOR_P6WMTNI:
        m_type = IA_DECODER_CPU_WMT;
        break;
      case GE_PROCESSOR_PRESCOTT_YT:
        m_type = IA_DECODER_CPU_PRESCOTT_YT;
        break;
      case GE_PROCESSOR_PRESCOTT_LT:
        m_type = IA_DECODER_CPU_PSC;
        break;
      case GE_PROCESSOR_TEJAS:
        m_type = IA_DECODER_CPU_TEJAS;
        break;
      default:
        fprintf(stderr, "Asim: SoftSDV decoder init -- unknown target processor\n");
        m_type = IA_DECODER_CPU_PRESCOTT_YT;
//        exit(1);
    }

    curMode = CPUAPI_Cpu_Mode_8086;
    if (IA_DECODER_NO_ERROR != ia_decoder_setenv(id,
                                                 m_type,
                                                 IA_DECODER_MODE_86))
    {
        fprintf(stderr, "Asim: SoftSDV decoder setenv failed\n");
        exit(1);
    }

    //
    // Make sure that the CPU mode enumeration is what we expected.
    //

    initialized = true;
};


//
// TranslateDecoderMode --
//     Convert from the CPUAPI representation of the current execution mode
//     to the decoder's represenatation.
//
IA_Decoder_Machine_Mode
INSTR_DECODER_CLASS::TranslateDecoderMode(cpuapi_cpu_mode_t sdvMode)
{
    IA_Decoder_Machine_Mode decoder_mode;

    switch(sdvMode)
    {
      case CPUAPI_Cpu_Mode_8086:
      case CPUAPI_Cpu_Mode_SMM:
        decoder_mode = IA_DECODER_MODE_86;
        break;
      case CPUAPI_Cpu_Mode_Big_Real:
        decoder_mode = IA_DECODER_MODE_BIG_REAL;
        break;
      case CPUAPI_Cpu_Mode_v8086:
        decoder_mode = IA_DECODER_MODE_V86;
        break;
      case CPUAPI_Cpu_Mode_Long_Compatible_16:
      case CPUAPI_Cpu_Mode_Protected_16:
        decoder_mode = IA_DECODER_MODE_PROTECTED_16;
        break;
      case CPUAPI_Cpu_Mode_Long_Compatible_32:
      case CPUAPI_Cpu_Mode_Protected_32:
        decoder_mode = IA_DECODER_MODE_PROTECTED_32;
        break;
      case CPUAPI_Cpu_Mode_Long_64:
        decoder_mode = IA_DECODER_MODE_LONG_64;
        break;
      default:
        fprintf(stderr, "Asim: SoftSDV TranslateDecoderMode(), unexpected mode %d\n",
                int(sdvMode));
        exit(1);
    }

    return decoder_mode;
}


//
// DecodeInstr --
//     Decode a single instruction.
//
cpuapi_stat_t
INSTR_DECODER_CLASS::DecodeInstr(const unsigned char *instr,
                                 unsigned int instrLen,
                                 cpuapi_cpu_mode_t mode,
                                 IA_Decoder_Info *decoded)
{
    IA_Decoder_Err err;

    ASSERTX(initialized);

    //
    // Is the decoder in the right execution mode for this instruction?
    //
    if (mode != curMode)
    {
        if (IA_DECODER_NO_ERROR != ia_decoder_setenv(id,
                                                     IA_DECODER_CPU_NO_CHANGE,
                                                     TranslateDecoderMode(mode)))
        {
            fprintf(stderr, "Asim: SoftSDV decoder setenv failed\n");
            exit(1);
        }
        curMode = mode;
    }

    //
    // Decode
    //
    err = ia_decoder_decode(id, instr, instrLen, decoded);
    if (err != IA_DECODER_NO_ERROR)
    {
        return CPUAPI_Stat_Err;
    }

    return CPUAPI_Stat_Ok;
}


//
// DisassembleInstr --
//     Disassemble one instruction and write the disassembly to the file
//     passed as the first parameter.
//
void
INSTR_DECODER_CLASS::DisassembleInstr(
    FILE *f,
    const ASIM_CPUAPI_INST_INFO_CLASS &instInfo)
{
    ASSERTX(initialized);

    DisassembleInstr(f, instInfo.GetVA(),
                     instInfo.GetInstBytes(), instInfo.GetNInstBytes(),
                     instInfo.GetCpuMode());

    if (instInfo.GetException())
    {
        fprintf(f, "  <exception>");
    }

    fprintf(f, "\n");
}


void
INSTR_DECODER_CLASS::DisassembleInstr(
    FILE *f,
    UINT64 va,
    const unsigned char *instr,
    unsigned int instrLen,
    cpuapi_cpu_mode_t mode)
{
    IA_Dis_Err err;
    unsigned int actualLen = 0;
    char abuf[256];
    unsigned int abufLen = sizeof(abuf);

    ASSERTX(initialized);

    err = ia_dis_inst((U64 *)&va, TranslateDecoderMode(mode),
                      instr, instrLen,
                      &actualLen,
                      abuf, &abufLen,
                      NULL);

    if (err == IA_DIS_NO_ERROR)
    {
        //
        // Drop leading spaces
        //
        int i = 0;
        while (abuf[i] && abuf[i] == ' ') i++;
        fprintf(f, "%s", &abuf[i]);
    }
    else
    {
        fprintf(f, "                                    <decode error>");
    }
}


//------------------------------------------------------------------------
//
// DMA
// 
//------------------------------------------------------------------------

CPU_DMA_INVALIDATE_CLASS::DMA_EVENT_CLASS::DMA_EVENT_CLASS(
    CPU_DMA_INVALIDATE parent,
    UINT64 addr,
    UINT64 size)
    : addr(addr),
      size(size),
      next(NULL),
      parent(parent)
{
    if (parent->tail == NULL)
    {
        parent->head = this;
    }
    else
    {
        parent->tail->next = this;
    }

    parent->tail = this;
}


CPU_DMA_INVALIDATE_CLASS::DMA_EVENT_CLASS::~DMA_EVENT_CLASS()
{
    ASSERTX(parent->head == this);

    parent->head = this->next;
    if (parent->head == NULL)
    {
        parent->tail = NULL;
    }
}


CPU_DMA_INVALIDATE_CLASS::CPU_DMA_INVALIDATE_CLASS(void)
    : head(NULL),
      tail(NULL)
{}


CPU_DMA_INVALIDATE_CLASS::~CPU_DMA_INVALIDATE_CLASS()
{
    while (head != NULL)
    {
        delete head;
    }
}

    
void
CPU_DMA_INVALIDATE_CLASS::InvalidateAddr(
    UINT64 addr,
    UINT64 size)
{
//    new DMA_EVENT_CLASS(this, addr, size);
}


bool
CPU_DMA_INVALIDATE_CLASS::GetDMAEvent(
    UINT64 *addr,
    UINT64 *size)
{
    if (head == NULL)
    {
        return false;
    }

    *addr = head->addr;
    *size = head->size;
    delete head;
    return true;
}



//------------------------------------------------------------------------
//
// Lock step
// 
//------------------------------------------------------------------------

//
// Lock step with another SoftSDV process (for debugging)
//

extern void
AsimStartLockStep(void)
{
    if (lockStep != NULL) return;

    lockStep = new ASIM_LOCKSTEP_X86_CLASS(2049, 20000);
}


extern void
AsimStopLockStep(void)
{
    if (lockStep != NULL)
    {
        delete lockStep;
    }
    lockStep = NULL;
}


extern void
AsimLockStepDMA(UINT64 addr, UINT64 size)
{
    if (lockStep != NULL)
    {
        lockStep->DMA(addr, size);
    }
}
