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
 * @file cpuapi_ipf.cpp
 * @author Michael Adler
 * @brief IPF specific code for SoftSDV stub
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



ARCH_REGISTER_TYPE
MapRegisterType(
    EM_Decoder_Reg_Type regType);

void
registerValueError(
    char *regName,
    UINT32 cpuNum);


//------------------------------------------------------------------------
//
// System information
// 
//------------------------------------------------------------------------

class SYSTEM_INFO_CLASS
{
  public:
    SYSTEM_INFO_CLASS() :
        mmapIoStart(0),
        mmapIoEnd(0),
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

        oh = oml_obj_translate_name(GE_VPC_CPU_ID,
                                    "data.config.rc_info.ssc_info.io_baseaddress");
        ASSERTX(oh);
        size = sizeof(mmapIoStart);
        ASSERTX(GE_OK == oml_get_value(GE_VPC_CPU_ID, oh, &size, &mmapIoStart));
        //
        // IO size is not stored anywhere, unfortunately.
        //
        mmapIoEnd = mmapIoStart + 0x3ffffff;

        cout << "OS mode: " << (osMode ? "yes" : "no") << endl;
        if (osMode)
        {
            cout << "I/O Mapped to: "
                 << hex << "0x" << mmapIoStart << " - "
                 << "0x" << mmapIoEnd << dec << endl;
        }
    };

    bool OsMode() const { return osMode; };

    bool PhysicalAddrIsIO(UINT64 pa) const
    {
        // Ignore high bit
        pa &= 0x7fffffffffffffffLL;

        return OsMode() && (pa >= mmapIoStart) && (pa <= mmapIoEnd);
    };

  private:
    //
    // First and last address of memory mapped I/O region (physical).
    //
    UINT64 mmapIoStart;
    UINT64 mmapIoEnd;

    bool osMode;
};

SYSTEM_INFO_CLASS sysInfo;


//------------------------------------------------------------------------
//
// Abstract instruction
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
    cpuapi_inst_info_t *sdvInst;

  public:
    ASIM_CPUAPI_INST_INFO_CLASS(cpuapi_inst_info_t *sdvInst = NULL) :
        sdvInst(sdvInst)
    {};

    ~ASIM_CPUAPI_INST_INFO_CLASS() {};

    void Init(cpuapi_inst_info_t *_sdvInst) { sdvInst = _sdvInst; };

    cpuapi_inst_t      GetInstId(void) const { return sdvInst->inst_id; };
    cpuapi_inst_t      GetIcount(void) const { return sdvInst->icount; };
    cpuapi_phys_addr_t GetPhyAddr(void) const { return sdvInst->phy_addr; };
    cpuapi_virt_addr_t GetVirtAddr(void) const { return sdvInst->virt_addr; };

#if CPUAPI_VERSION_MAJOR(CPUAPI_VERSION_CURRENT) >= 3
    //
    // 2004 and later releases have an architecture specific part separate
    // from the main sdvInst.
    //
  private:
    cpuapi_ipf_inst_info_t* GetArchInfo(void) const
    {
        return (cpuapi_ipf_inst_info_t *)sdvInst->arch_info;
    };

  public:
    unsigned char*     GetInstBytes(void) const { return GetArchInfo()->inst_bytes; };
    cpuapi_boolean_t   GetBranchTaken(void) const { return GetArchInfo()->branch_taken; };
    cpuapi_virt_addr_t GetBranchTargetVirtAddr(cpuapi_virt_addr_t mask = ~0) const { return GetArchInfo()->branch_target_virt_addr & mask; };
    cpuapi_boolean_t   GetStoreInst(void) const { return GetArchInfo()->store_inst; };
    void               ClearStoreInst(void) { GetArchInfo()->store_inst = false; };
    cpuapi_boolean_t   GetLoadInst(void) const { return GetArchInfo()->load_inst; };
    void               ClearLoadInst(void) { GetArchInfo()->load_inst = false; };
    cpuapi_boolean_t   GetBranchInst(void) const { return GetArchInfo()->branch_inst; };
    void               ClearBranchInst(void) { GetArchInfo()->branch_inst = false; };
    cpuapi_phys_addr_t GetPhyMemAddr(void) const { return GetArchInfo()->phy_mem_addr; };
    cpuapi_virt_addr_t GetVirtMemAddr(void) const { return GetArchInfo()->virt_mem_addr; };
    cpuapi_u64_t       GetCFM(void) const { return GetArchInfo()->cfm; };
    cpuapi_u64_t       GetBSP(void) const { return GetArchInfo()->bsp; };
    cpuapi_u64_t       GetPredicates(void) const { return GetArchInfo()->predicates; };

#else
    //
    // Older releases (circa 2003)
    //
    unsigned char*     GetInstBytes(void) const { return sdvInst->inst_bytes; };
    cpuapi_boolean_t   GetBranchTaken(void) const { return sdvInst->branch_taken; };
    cpuapi_virt_addr_t GetBranchTargetVirtAddr(cpuapi_virt_addr_t mask = ~0) const { return sdvInst->branch_target_virt_addr & mask; };
    cpuapi_boolean_t   GetStoreInst(void) const { return sdvInst->store_inst; };
    void               ClearStoreInst(void) { sdvInst->store_inst = false; };
    cpuapi_boolean_t   GetLoadInst(void) const { return sdvInst->load_inst; };
    void               ClearLoadInst(void) { sdvInst->load_inst = false; };
    cpuapi_boolean_t   GetBranchInst(void) const { return sdvInst->branch_inst; };
    void               ClearBranchInst(void) { sdvInst->branch_inst = false; };
    cpuapi_phys_addr_t GetPhyMemAddr(void) const { return sdvInst->phy_mem_addr; };
    cpuapi_virt_addr_t GetVirtMemAddr(void) const { return sdvInst->virt_mem_addr; };
    cpuapi_u64_t       GetCFM(void) const { return sdvInst->cfm; };
    cpuapi_u64_t       GetBSP(void) const { return sdvInst->bsp; };
    cpuapi_u64_t       GetPredicates(void) const { return sdvInst->predicates; };
#endif
};


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
        bzero(ipfInstInfo, sizeof(ipfInstInfo));

        //
        // Initialize abstract instruction info class with pointer to
        // version-specific SoftSDV type.
        //
        for (unsigned int i = 0; i < maxChunkSize; i++)
        {
            instInfo[i].Init(&sdvInstInfo[i]);
        }

        Reset();
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
    void Reset(unsigned int n = 0)
    {
        ASSERTX(n <= maxChunkSize);

        if (n == 0)
        {
            n = maxChunkSize;
        }

        //
        // Sure this could be more efficient, but clearing all fields seems
        // safest and easier to write.
        //
        bzero(ipfInstInfo, sizeof(ipfInstInfo[0]) * n);
        bzero(sdvInstInfo, sizeof(sdvInstInfo[0]) * n);

#if CPUAPI_VERSION_MAJOR(CPUAPI_VERSION_CURRENT) >= 3
        for (unsigned int i = 0; i < n; i++)
        {
            sdvInstInfo[i].arch_info_size = sizeof(ipfInstInfo[0]);
            sdvInstInfo[i].arch_info = &ipfInstInfo[i];
        }
#endif
    };

  private:
    ASIM_CPUAPI_INST_INFO_CLASS instInfo[maxChunkSize];
    cpuapi_inst_info_t sdvInstInfo[maxChunkSize];
#if CPUAPI_VERSION_MAJOR(CPUAPI_VERSION_CURRENT) >= 3
    cpuapi_ipf_inst_info_t ipfInstInfo[maxChunkSize];
#else
    char ipfInstInfo[maxChunkSize];       // dummy
#endif
};


//------------------------------------------------------------------------
//
// Instruction decoder
// 
//------------------------------------------------------------------------

class INSTR_DECODER_CLASS
{
  public:
    INSTR_DECODER_CLASS(void)
    {
        id = em_decoder_open();
        if (-1 == id)
        {
            return;
        }

        if (EM_DECODER_NO_ERROR != em_decoder_setup(id,
                                                    EM_DECODER_CPU_P7,
                                                    EM_DECODER_MODE_EM,
                                                    EM_DECODER_FLAG_NO_MEMSET))
        {
            return;
        }

        if (EM_DECODER_NO_ERROR != em_decoder_setenv(id,
                                                     EM_DECODER_CPU_P7,
                                                     EM_DECODER_MODE_EM))
        {
            return;
        }
    };

    ~INSTR_DECODER_CLASS()
    {
        em_decoder_close(id);
    };

    cpuapi_stat_t
    DecodeBundle(const unsigned char *bundle_bytes,
                 EM_Decoder_Bundle_Info *decoded);

    void
    DisassembleInstr(FILE *f,
                     UINT64 va,
                     const unsigned char *bundle_bytes);

  private:
    EM_Decoder_Id id;
};

INSTR_DECODER_CLASS instrDecoder;

cpuapi_stat_t
INSTR_DECODER_CLASS::DecodeBundle(const unsigned char *bundle_bytes,
                                  EM_Decoder_Bundle_Info *decoded)
{
    EM_Decoder_Err err;

    err = em_decoder_decode_bundle(id, bundle_bytes, EM_BUNDLE_SIZE,
                                   decoded);
    if (err != EM_DECODER_NO_ERROR)
    {
        em_decoder_err_msg(err);
        return CPUAPI_Stat_Err;
    }

    return CPUAPI_Stat_Ok;
}


void
INSTR_DECODER_CLASS::DisassembleInstr(FILE *f,
                                      UINT64 va,
                                      const unsigned char *bundle_bytes)
{
    EM_Dis_Err err;
    unsigned int actualLen = 0;
    char abuf[1000];
    unsigned int abufLen = sizeof(abuf);
    EM_Dis_Fields abufFields;

    err = em_dis_inst((U64 *)&va, EM_DECODER_MODE_NO_CHANGE,
                      bundle_bytes, EM_BUNDLE_SIZE, &actualLen,
                      abuf, &abufLen,
                      &abufFields);

    if (err == EM_DIS_NO_ERROR)
    {
        if (abuf[0] != '0')
        {
            //
            // Always print leading address
            //
            fprintf(f, "  0x%016llx:  ", va);
        }

        //
        // Drop leading spaces
        //
        int i = 0;
        while (abuf[i] && abuf[i] == ' ') i++;
        if (abuf[i] != '(')
        {
            i -= 9;
            if (i < 0)
            {
                i = 0;
            }
        }

        fprintf(f, "%s\n", &abuf[i]);
    }
};


static void
DumpEntireInstrVector(
    ASIM_SOFTSDV_FETCH_INST_BUFFER_CLASS &instInfo,
    UINT32 nInstrs
)
{
    for (UINT32 i = 0; i < nInstrs; i++)
    {
        if ((instInfo[i].GetInstId() == 0) && (instInfo[i].GetIcount() == 0))
        {
            break;
        }

        fprintf(stderr, "    ");
        instrDecoder.DisassembleInstr(stderr,
                                      instInfo[i].GetVirtAddr(),
                                      instInfo[i].GetInstBytes());
    }
}


//------------------------------------------------------------------------
//
// Per-CPU state
// 
//------------------------------------------------------------------------

void
CPU_INFO_CLASS::InitHandles(void)
{
    fprintf(stderr, "Initializing the handles for CPU number %d\n", cpuNum);

    view_cpu_handle = oml_obj_translate_name(GE_VPC_CPU_ID, "notifier.view_cpu");
    ASSERTX(view_cpu_handle);

    ASSERTX(! asimCpucCallBack->translate_object_name(controllerPid,
                                                      CPUAPI_IPF_CFM,
                                                      &cfm_handle));
    ASSERTX(! asimCpucCallBack->translate_object_name(controllerPid,
                                                      CPUAPI_SIM_CPU_MODE,
                                                      &cpuMode_handle));
    ASSERTX(! asimCpucCallBack->translate_object_name(controllerPid,
                                                      CPUAPI_IPF_IP,
                                                      &ip_handle));
    ASSERTX(! asimCpucCallBack->translate_object_name(controllerPid,
                                                      CPUAPI_IPF_PR,
                                                      &predicates_handle));
    ASSERTX(! asimCpucCallBack->translate_object_name(controllerPid,
                                                      CPUAPI_IPF_PSR,
                                                      &psr_handle));
    ASSERTX(! asimCpucCallBack->translate_object_name(controllerPid,
                                                      CPUAPI_SIM_ALIVE_CPUS,
                                                      &simAliveCpus_handle));
    ASSERTX(! asimCpucCallBack->translate_object_name(controllerPid,
                                                      CPUAPI_SIM_INST_INSTRUCTION_INFO,
                                                      &simInstInfo_handle));
    ASSERTX(! asimCpucCallBack->translate_object_name(controllerPid,
                                                      CPUAPI_SIM_LINEAR_XIP,
                                                      &simLinearXip_handle));
    ASSERTX(! asimCpucCallBack->translate_object_name(controllerPid,
                                                      CPUAPI_SIM_LITCOUNT,
                                                      &simLitCount_handle));

    int i;
    char name[100];

    for (i = 0; i < CPUAPI_IPF_GR_NUM; i++)
    {
        CPUAPI_IPF_GR_NAME(name, i);
        ASSERTX(! asimCpucCallBack->translate_object_name(controllerPid,
                                                          name,
                                                          &regsGR_handle[i]));
    }
    for (i = 0; i < CPUAPI_IPF_FR_NUM; i++)
    {
        CPUAPI_IPF_FR_NAME(name, i);
        ASSERTX(! asimCpucCallBack->translate_object_name(controllerPid,
                                                          name,
                                                          &regsFR_handle[i]));
    }
    for (i = 0; i < CPUAPI_IPF_AR_NUM; i++)
    {
        CPUAPI_IPF_AR_NAME(name, i);
        ASSERTX(! asimCpucCallBack->translate_object_name(controllerPid,
                                                          name,
                                                          &regsAR_handle[i]));
    }
    for (i = 0; i < CPUAPI_IPF_BR_NUM; i++)
    {
        CPUAPI_IPF_BR_NAME(name, i);
        ASSERTX(! asimCpucCallBack->translate_object_name(controllerPid,
                                                          name,
                                                          &regsBR_handle[i]));
    }
    for (i = 0; i < CPUAPI_IPF_CR_NUM; i++)
    {
        CPUAPI_IPF_CR_NAME(name, i);
        ASSERTX(! asimCpucCallBack->translate_object_name(controllerPid,
                                                          name,
                                                          &regsCR_handle[i]));
    }
};


void
CPU_INFO_CLASS::SetSimulationMode(cpuapi_simulmode_t mode)
{
    simulationMode = mode;

    if (mode == CPUAPI_Simul_Mode_Hot)
    {
        //
        // Make sure visibility interface is in logical mode
        //
        cpuapi_vis_config_t visibilityMode;
        ASSERTX(CPUAPI_Stat_Ok ==
                asimCpucCallBack->get_object_by_name(cpuInfo[cpuNum].ControllerPid(),
                                                     1,
                                                     CPUAPI_SIM_VISIBILITY_CONFIG,
                                                     CPUAPI_SIM_VISIBILITY_CONFIG_SIZE,
                                                     &visibilityMode));

        (int)visibilityMode &= ~CPUAPI_Visibility_Physical_View;
        (int)visibilityMode |= CPUAPI_Visibility_Logical_View;

        ASSERTX(CPUAPI_Stat_Ok ==
                asimCpucCallBack->set_object_by_name(cpuInfo[cpuNum].ControllerPid(),
                                                     CPUAPI_SIM_VISIBILITY_CONFIG,
                                                     CPUAPI_SIM_VISIBILITY_CONFIG_SIZE,
                                                     &visibilityMode));
    }
};


UINT64
CPU_INFO_CLASS::BSP(void)
{
    return RegAR(17);
}


UINT64
CPU_INFO_CLASS::CFM(void)
{
    UINT64 value;
    ASSERTX(! asimCpucCallBack->get_object(controllerPid, 0, cfm_handle,
                                           CPUAPI_IPF_CFM_SIZE, &value));
    return value;
}


UINT64
CPU_INFO_CLASS::IP(void)
{
    UINT64 value;
    ASSERTX(! asimCpucCallBack->get_object(controllerPid, 0, ip_handle,
                                           CPUAPI_IPF_IP_SIZE, &value));
    return value;
}


UINT64
CPU_INFO_CLASS::Predicates(void)
{
    UINT64 value;
    ASSERTX(! asimCpucCallBack->get_object(controllerPid, 0, predicates_handle,
                                           CPUAPI_IPF_PR_SIZE, &value));
    return value;
}


UINT64
CPU_INFO_CLASS::PSR(void)
{
    UINT64 value;
    ASSERTX(! asimCpucCallBack->get_object(controllerPid, 0, psr_handle,
                                           CPUAPI_IPF_PSR_SIZE, &value));
    return value;
}


UINT64
CPU_INFO_CLASS::RegGR(UINT32 n)
{
    UINT64 value;

    ASSERTX(n < CPUAPI_IPF_GR_NUM);
    ASSERTX(asimCpucCallBack->get_object(controllerPid, 0,
                                         cpuInfo[cpuNum].regsGR_handle[n],
                                         CPUAPI_IPF_GR_SIZE,
                                         &value) == CPUAPI_Stat_Ok);
    return value;
}

void
CPU_INFO_CLASS::RegFR(UINT32 n, UINT64 value[2])
{
    ASSERTX(n < CPUAPI_IPF_FR_NUM);
    value[0] = 0;
    value[1] = 0;
    ASSERTX(asimCpucCallBack->get_object(controllerPid, 0,
                                         cpuInfo[cpuNum].regsFR_handle[n],
                                         CPUAPI_IPF_FR_SIZE,
                                         value) == CPUAPI_Stat_Ok);
}

bool
CPU_INFO_CLASS::RegPR(UINT32 n)
{
    ASSERTX(n < 64);

    return ((Predicates() >> n) & 1) == 1;
}

UINT64
CPU_INFO_CLASS::RegAR(UINT32 n)
{
    UINT64 value;

    ASSERTX(n < CPUAPI_IPF_AR_NUM);

    //
    // Not all ARs are implemented.
    //
    if (asimCpucCallBack->get_object(controllerPid, 0,
                                     cpuInfo[cpuNum].regsAR_handle[n],
                                     CPUAPI_IPF_AR_SIZE,
                                     &value) != CPUAPI_Stat_Ok)
    {
        value = 0;
    }

    return value;
}

UINT64
CPU_INFO_CLASS::RegBR(UINT32 n)
{
    UINT64 value;

    ASSERTX(n < CPUAPI_IPF_BR_NUM);
    ASSERTX(asimCpucCallBack->get_object(controllerPid, 0,
                                         cpuInfo[cpuNum].regsBR_handle[n],
                                         CPUAPI_IPF_BR_SIZE,
                                         &value) == CPUAPI_Stat_Ok);
    return value;
}

UINT64
CPU_INFO_CLASS::RegCR(UINT32 n)
{
    UINT64 value;

    ASSERTX(n < CPUAPI_IPF_CR_NUM);

    //
    // Not all CRs are implemented.
    //
    if (asimCpucCallBack->get_object(controllerPid, 0,
                                     cpuInfo[cpuNum].regsCR_handle[n],
                                     CPUAPI_IPF_CR_SIZE,
                                     &value) != CPUAPI_Stat_Ok)
    {
        value = 0;
    }

    return value;
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

bool
CPU_INFO_CLASS::ReadMemory(
    UINT64 pa,
    cpuapi_size_t size,
    void *buf)
{
    return CPUAPI_Stat_Ok == asimCpucCallBack->mem_read(controllerPid,
                                                        pa,
                                                        size,
                                                        buf);
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


//
// A simple class for tracking register values.  Provides a valid bit protecting
// each register and a way to clear all the valid bits at once.
//

template <unsigned int nRegisters, class REG_VALUE_CLASS>
class BUFFERED_REGISTER_VALUE_CLASS
{
  private:
    enum CONSTANTS
    {
        VALID_BIT_ARRAY_ENTRIES = (nRegisters >> 5) + 1
    };

  public:
    BUFFERED_REGISTER_VALUE_CLASS()
    {
        bzero(values, sizeof(values));
        ResetValidBits();
    };

    void ResetValidBits(void)
    {
        for (unsigned int i = 0; i < VALID_BIT_ARRAY_ENTRIES; i++)
        {
            valid_bits[i] = 0;
        }
    };

    bool IsValid(unsigned int regNum) const
    {
        ASSERTX(regNum < nRegisters);
        return ((valid_bits[regNum >> 5] >> (regNum & 31)) & 1) != 0;
    };

    REG_VALUE_CLASS RegisterValue(unsigned int regNum) const
    {
        ASSERTX(IsValid(regNum));
        return values[regNum];
    };

    void SetRegisterValue(unsigned int regNum, REG_VALUE_CLASS regValue)
    {
        ASSERTX(regNum < nRegisters);
        values[regNum] = regValue;
        valid_bits[regNum >> 5] |= (1 << (regNum & 31));
    };

  private:
    UINT32 valid_bits[VALID_BIT_ARRAY_ENTRIES];
    REG_VALUE_CLASS values[nRegisters];
};


class CPU_HISTORY_CLASS
{
  public:

    CPU_HISTORY_CLASS(UINT32 cpuNum) :
        bsp(0),
        cfm(0),
        logicalPredicates(0),
        next_ip_va(0),
        ip_va(0),
        cpuNum(cpuNum),
        decode_va(4),           // An impossible value for an instruction addr
        newSequence(false),
        prev_was_rfi(false)
    {};

    ~CPU_HISTORY_CLASS() {};
    
    //
    // Update history given this new executed instruction.  If you have already
    // decoded the instruction pass the decoder result to the function.  If
    // not, the function will decode the instruction on its own.
    //
    void ExecutedInstruction(const ASIM_CPUAPI_INST_INFO_CLASS &instInfo);

    //
    // Was the previous instruction a decoded opcode?
    //
    bool PrevOpRfi(void) const { return prev_was_rfi; };

    UINT64 BSP(void) const { return bsp; };
    UINT64 CFM(void) const { return cfm; };
    UINT64 LogicalPredicates(void) const { return logicalPredicates; };

    UINT64 NextIP_Virtual(void) const { return next_ip_va; };
    UINT64 LastExecutedIP_Virtual(void) const { return ip_va; };

    bool IsNewSequence(void) const { return newSequence; };
    void SetNewSequence(void) { newSequence = true; };

    void InitAllRegs(void);
    void ResetAllRegs(void);
    void CacheRegisterValue(const EM_Decoder_Operand_Info &operand);
    void CacheRegisterValue(EM_Decoder_Reg_Type regType, UINT32 regNum);

    bool ReadCachedRegisterValue(
        EM_Decoder_Reg_Type regType,
        UINT32 regNum,
        UINT64ArrayPair &value);
    void FetchCurrentRegisterValue(
        EM_Decoder_Reg_Type regType,
        UINT32 regNum,
        UINT64ArrayPair &value);

    //
    // Last bundle passed to the decoder.  This might be useful for error
    // messages about the last instruction executed.  Be careful.  This
    // field is updated by DecodeSyllable(), which is called by
    // ExecutedInstruction().
    //
    const unsigned char *DecodeInstBytes(void) const { return decode_inst_bytes; };

    const EM_Decoder_Info *
    DecodeSyllable(UINT64 ip_va, const unsigned char instBytes[16]);

    const EM_Decoder_Info *
    DecodeSyllable(const ASIM_CPUAPI_INST_INFO_CLASS &instInfo);

    bool DecodeRegister(
        const EM_Decoder_Operand_Info &reg,
        EM_Decoder_Reg_Type *regType,
        UINT32 *regNum
    );

    void HandleImplicitRegister(
        EM_Decoder_Imp_Operand implicitReg,
        bool isDstReg,
        ASIM_SOFTSDV_INST_INFO_CLASS * inst
    );

  private:
    UINT64 bsp;
    UINT64 cfm;
    UINT64 logicalPredicates;

    UINT64 next_ip_va;
    UINT64 ip_va;

    //
    // Complete register state
    //
    BUFFERED_REGISTER_VALUE_CLASS<CPUAPI_IPF_GR_NUM, UINT64> regsGR;
    BUFFERED_REGISTER_VALUE_CLASS<CPUAPI_IPF_FR_NUM, UINT64ArrayPair> regsFR;
    BUFFERED_REGISTER_VALUE_CLASS<CPUAPI_IPF_AR_NUM, UINT64> regsAR;
    BUFFERED_REGISTER_VALUE_CLASS<CPUAPI_IPF_BR_NUM, UINT64> regsBR;
    BUFFERED_REGISTER_VALUE_CLASS<CPUAPI_IPF_CR_NUM, UINT64> regsCR;

    const UINT32 cpuNum;

    //
    // Decoded last bundle.
    //
    EM_Decoder_Bundle_Info decodedBundle;
    UINT64 decode_va;
    unsigned char decode_inst_bytes[CPUAPI_SIM_INST_BYTES_LENGTH];

    //
    // Flags
    //
    bool newSequence;       // First instruction after fast-forward
    bool prev_was_rfi;
};

typedef CPU_HISTORY_CLASS *CPU_HISTORY;


//
// Buffer all registers locally.  This is expensive!  Only use in desperation.
//
void
CPU_HISTORY_CLASS::InitAllRegs(void)
{
    int i;

    for (i = 0; i < CPUAPI_IPF_GR_NUM; i++)
    {
        regsGR.SetRegisterValue(i, cpuInfo[cpuNum].RegGR(i));
    }
    for (i = 0; i < CPUAPI_IPF_FR_NUM; i++)
    {
        UINT64ArrayPair fr;
        cpuInfo[cpuNum].RegFR(i, fr.v);
        regsFR.SetRegisterValue(i, fr);
    }
    for (i = 0; i < CPUAPI_IPF_AR_NUM; i++)
    {
        regsAR.SetRegisterValue(i, cpuInfo[cpuNum].RegAR(i));
    }
    for (i = 0; i < CPUAPI_IPF_BR_NUM; i++)
    {
        regsBR.SetRegisterValue(i, cpuInfo[cpuNum].RegBR(i));
    }
    for (i = 0; i < CPUAPI_IPF_CR_NUM; i++)
    {
        regsCR.SetRegisterValue(i, cpuInfo[cpuNum].RegCR(i));
    }
}


//
// Clear the valid bits on all locally buffered registers.
//
void
CPU_HISTORY_CLASS::ResetAllRegs(void)
{
    regsGR.ResetValidBits();
    regsFR.ResetValidBits();
    regsAR.ResetValidBits();
    regsBR.ResetValidBits();
    regsCR.ResetValidBits();
}


void
CPU_HISTORY_CLASS::CacheRegisterValue(
    const EM_Decoder_Operand_Info &operand)
{
    EM_Decoder_Reg_Type regType;
    UINT32 regNum;

    //
    // Does the operand use a register?
    //
    if (! DecodeRegister(operand, &regType, &regNum))
    {
        return;
    }

    CacheRegisterValue(regType, regNum);
}


void
CPU_HISTORY_CLASS::CacheRegisterValue(
    EM_Decoder_Reg_Type regType,
    UINT32 regNum)
{
    switch (regType)
    {
      case EM_DECODER_INT_REG:
        regsGR.SetRegisterValue(regNum, cpuInfo[cpuNum].RegGR(regNum));
        break;

      case EM_DECODER_FP_REG:
        UINT64ArrayPair fr;
        cpuInfo[cpuNum].RegFR(regNum, fr.v);
        regsFR.SetRegisterValue(regNum, fr);
        break;

      case EM_DECODER_APP_REG:
        regsAR.SetRegisterValue(regNum, cpuInfo[cpuNum].RegAR(regNum));
        break;

      case EM_DECODER_BR_REG:
        regsBR.SetRegisterValue(regNum, cpuInfo[cpuNum].RegBR(regNum));
        break;

      case EM_DECODER_CR_REG:
        regsCR.SetRegisterValue(regNum, cpuInfo[cpuNum].RegCR(regNum));
        break;

      case EM_DECODER_PRED_REG:
      case EM_DECODER_PR_REG:
        // Predicates are computed with a different mechanism
        break;

      case EM_DECODER_IP_REG:
        // IP is known already
        break;

      default:
        ASIMERROR("Invalid register type");
        break;
    }
}
        

bool
CPU_HISTORY_CLASS::ReadCachedRegisterValue(
    EM_Decoder_Reg_Type regType,
    UINT32 regNum,
    UINT64ArrayPair &value)
{
    bool valid = false;

    value.v[0] = 0;
    value.v[1] = 0;

    switch (regType)
    {
      case EM_DECODER_INT_REG:
        if (regsGR.IsValid(regNum))
        {
            value.v[0] = regsGR.RegisterValue(regNum);
            valid = true;
        }
        break;

      case EM_DECODER_FP_REG:
        if (regsFR.IsValid(regNum))
        {
            value = regsFR.RegisterValue(regNum);
            valid = true;
        }
        break;

      case EM_DECODER_APP_REG:
        if (regsAR.IsValid(regNum))
        {
            value.v[0] = regsAR.RegisterValue(regNum);
            valid = true;
        }
        break;

      case EM_DECODER_BR_REG:
        if (regsBR.IsValid(regNum))
        {
            value.v[0] = regsBR.RegisterValue(regNum);
            valid = true;
        }
        break;

      case EM_DECODER_CR_REG:
        if (regsCR.IsValid(regNum))
        {
            value.v[0] = regsCR.RegisterValue(regNum);
            valid = true;
        }
        break;

      default:
        ASIMERROR("Invalid register type");
        break;
    }

    return valid;
}
        

void
CPU_HISTORY_CLASS::FetchCurrentRegisterValue(
    EM_Decoder_Reg_Type regType,
    UINT32 regNum,
    UINT64ArrayPair &value)
{
    bool valid = false;

    value.v[0] = 0;
    value.v[1] = 0;

    switch (regType)
    {
      case EM_DECODER_INT_REG:
        value.v[0] = cpuInfo[cpuNum].RegGR(regNum);
        break;

      case EM_DECODER_FP_REG:
        cpuInfo[cpuNum].RegFR(regNum, value.v);
        break;

      case EM_DECODER_APP_REG:
        value.v[0] = cpuInfo[cpuNum].RegAR(regNum);
        break;

      case EM_DECODER_BR_REG:
        value.v[0] = cpuInfo[cpuNum].RegBR(regNum);
        break;

      case EM_DECODER_CR_REG:
        value.v[0] = cpuInfo[cpuNum].RegCR(regNum);
        break;

      default:
        ASIMERROR("Invalid register type");
        break;
    }
}
        

bool
CPU_HISTORY_CLASS::DecodeRegister(
    const EM_Decoder_Operand_Info &reg,
    EM_Decoder_Reg_Type *regType,
    UINT32 *regNum
)
{
    switch (reg.type)
    {
      case EM_DECODER_REGISTER:
        *regType = reg.reg_info.type;
        *regNum = reg.reg_info.value;
        break;

      case EM_DECODER_MEMORY:
        *regType = reg.mem_info.mem_base.type;
        *regNum = reg.mem_info.mem_base.value;
        break;

      default:
        *regType = EM_DECODER_NO_REG_TYPE;
        *regNum = 0;
        return false;
    }

    //
    // Not all register types are supported
    //
    switch (*regType)
    {
      case EM_DECODER_INT_REG:
      case EM_DECODER_FP_REG:
      case EM_DECODER_PRED_REG:
      case EM_DECODER_PR_REG:       // All predicates as a bit vector
      case EM_DECODER_APP_REG:
      case EM_DECODER_BR_REG:
      case EM_DECODER_CR_REG:
      case EM_DECODER_IP_REG:
        // Ok
        break;

        // Decoder has a bunch of extra names to make decoding even harder...
      case EM_DECODER_APP_CCV_REG:
        ASSERTX(*regNum == 32);
        *regType = EM_DECODER_APP_REG;
        break;

      case EM_DECODER_APP_PFS_REG:
        ASSERTX(*regNum == 64);
        *regType = EM_DECODER_APP_REG;
        break;

      case EM_DECODER_PR_ROT_REG:
        *regType = EM_DECODER_PRED_REG;
        break;

        // Ignore these, for now
      case EM_DECODER_PSR_REG:
      case EM_DECODER_PSR_L_REG:
        *regType = EM_DECODER_NO_REG_TYPE;
        *regNum = 0;
        return false;

      case EM_DECODER_NO_REG_TYPE:
        ASIMERROR("Invalid register type");
        break;

      default:
        ASIMERROR("Unexpected register type");
        break;
    }

    return true;
}


//
// Deal with implicit registers in the EM decoder data structure.  This
// code can handle caching inputs, inputs and outputs.  If inst is NULL then
// it assumes we are caching inputs.  Otherwise isDstReg controls whether
// the register is assumed to be a source or a destination.
//
void
CPU_HISTORY_CLASS::HandleImplicitRegister(
    EM_Decoder_Imp_Operand implicitReg,
    bool isDstReg,
    ASIM_SOFTSDV_INST_INFO_CLASS * inst)
{
    if (implicitReg == EM_DECODER_IMP_OPERAND_NONE) return;

    EM_Decoder_Reg_Type regType;
    UINT32 regNum;

    //
    // There are many implicit registers.  Handle the ones we care about
    // here.
    //
    switch (implicitReg)
    {
      case EM_DECODER_IMP_OPERAND_AR_LC:
        regType = EM_DECODER_APP_REG;
        regNum = 65;
        break;
      case EM_DECODER_IMP_OPERAND_AR_EC:
        regType = EM_DECODER_APP_REG;
        regNum = 66;
        break;
      default:
        regType = EM_DECODER_NO_REG_TYPE;
        regNum = 0;
    }

    if (regType != EM_DECODER_NO_REG_TYPE)
    {
        UINT64ArrayPair value;

        if (inst == NULL)
        {
            //
            // No target Asim instruction.  Must be preparing to execute
            // in SoftSDV.
            //
            CacheRegisterValue(regType, regNum);
        }
        else if (isDstReg)
        {
            FetchCurrentRegisterValue(regType, regNum, value);
            inst->AddOutputRegister(
                ARCH_REGISTER_CLASS(MapRegisterType(regType),
                                    regNum,
                                    value.v[0],
                                    value.v[1]));
        }
        else
        {
            if (ReadCachedRegisterValue(regType, regNum, value))
            {
                inst->AddInputRegister(
                    ARCH_REGISTER_CLASS(MapRegisterType(regType),
                                        regNum,
                                        value.v[0],
                                        value.v[1]));
            }
        }
    }
}


//
// Cache the decoder information for the bundle since we're likely to see the
// same bundle multiple times while processing each syllable.  The returned
// decode information remains valid the next call to DecodeSyllable().
//
const EM_Decoder_Info *
CPU_HISTORY_CLASS::DecodeSyllable(
    UINT64 ip_va,
    const unsigned char instBytes[16])
{
    if ((decode_va ^ ip_va) > 3 ||
        memcmp(decode_inst_bytes, instBytes, sizeof(decode_inst_bytes)))
    {
        memcpy(decode_inst_bytes, instBytes, sizeof(decode_inst_bytes));
        instrDecoder.DecodeBundle(decode_inst_bytes, &decodedBundle);
        decode_va = ip_va;
    }

    UINT32 syl = ip_va & 3;
    ASSERT(syl < decodedBundle.inst_num, "Decode error");

    return &decodedBundle.inst_info[syl];
}


const EM_Decoder_Info *
CPU_HISTORY_CLASS::DecodeSyllable(
    const ASIM_CPUAPI_INST_INFO_CLASS &instInfo)
{
    return DecodeSyllable(instInfo.GetVirtAddr(), instInfo.GetInstBytes());
}


void
CPU_HISTORY_CLASS::ExecutedInstruction(
    const ASIM_CPUAPI_INST_INFO_CLASS &instInfo)
{
    const EM_Decoder_Info *decode = DecodeSyllable(instInfo);

    newSequence = false;

    ip_va = instInfo.GetVirtAddr();

    prev_was_rfi = (decode->inst == EM_RFI);

    bsp = instInfo.GetBSP();
    cfm = instInfo.GetCFM();
    logicalPredicates = instInfo.GetPredicates();

    //
    // Save the registers modified by this instruction
    //
//    UpdateRegister(decode->dst1);
//    UpdateRegister(decode->dst2);

    //
    // Compute next IP
    //
    if (instInfo.GetBranchInst() && instInfo.GetBranchTaken())
    {
        //
        // Branch.  Must compute next physical address.
        //
        UINT64 mask = ~0xf;
        if (prev_was_rfi)
        {
            // RFI is syllable addressable
            mask = ~0;
        }
        next_ip_va = instInfo.GetBranchTargetVirtAddr(mask);
    }
    else if ((instInfo.GetPhyAddr() & 3) == 2)
    {
        next_ip_va = instInfo.GetVirtAddr() + 14;
    }
    else if ((instInfo.GetPhyAddr() & 3) == 1)
    {
        //
        // Need to check for an extended instruction.  Doh!
        //
        int ipf_template = instInfo.GetInstBytes()[0] & 31;
        if ((ipf_template == 4) || (ipf_template == 5))
        {
            next_ip_va = instInfo.GetVirtAddr() + 15;
        }
        else
        {
            next_ip_va = instInfo.GetVirtAddr() + 1;
        }
    }
    else
    {
        next_ip_va = instInfo.GetVirtAddr() + 1;
    }

    //
    // If we are tracking register values then cache input registers to the
    // next instruction.
    //
    if (asim_io.RecordRegisterValues())
    {
        ResetAllRegs();

        //
        // First we must figure out what the next instruction will be.
        //

        UINT64 next_ip_pa = 0;

        if ((ip_va ^ next_ip_va) < 4096)
        {
            //
            // The new instruction is on the same 4k page.  (We really should
            // find out the real page size.)  Compute the new physical address.
            //
            next_ip_pa = instInfo.GetPhyAddr() + (next_ip_va - ip_va);
        }
        else
        {
            cpuInfo[cpuNum].VirtualToPhysical(next_ip_va, &next_ip_pa);
        }

        unsigned char nextInstr[16];

        if ((next_ip_pa == 0) ||
            ! cpuInfo[cpuNum].ReadMemory(next_ip_pa & ~3L, sizeof(nextInstr), nextInstr))
        {
            //
            // Virtual to physical translation failed.  Get all the registers.
            // As long as the is infrequent performance will be ok.
            //
            InitAllRegs();
        }
        else
        {
            //
            // Decode the instruction and buffer just the input registers.
            //
            const EM_Decoder_Info *decode = DecodeSyllable(next_ip_va, nextInstr);
            CacheRegisterValue(decode->src1);
            CacheRegisterValue(decode->src2);
            CacheRegisterValue(decode->src3);
            CacheRegisterValue(decode->src4);
            CacheRegisterValue(decode->src5);

            //
            // If the dst fields refer to memory the address might be in an
            // input register.
            //
            if (decode->dst1.type == EM_DECODER_MEMORY)
            {
                CacheRegisterValue(decode->dst1);
            }
            if (decode->dst2.type == EM_DECODER_MEMORY)
            {
                CacheRegisterValue(decode->dst2);
            }

            //
            // Implicit input registers
            //
            for (int i = 0; i < EM_DECODER_MAX_IMP_SRC; i++)
            {
                EM_Decoder_Imp_Operand implicitReg;
                implicitReg = (EM_Decoder_Imp_Operand) decode->static_info->implicit_src[i];

                HandleImplicitRegister(implicitReg, false, NULL);
            }
        }
    }
};
    
DYNAMIC_ARRAY_CLASS<CPU_HISTORY> cpuHistory;


ARCH_REGISTER_TYPE
MapRegisterType(
    EM_Decoder_Reg_Type regType)
{
    switch (regType)
    {
      case EM_DECODER_INT_REG:
        return REG_TYPE_INT;

      case EM_DECODER_FP_REG:
        return REG_TYPE_FP82;

      case EM_DECODER_APP_REG:
        return REG_TYPE_AR;

      case EM_DECODER_BR_REG:
        return REG_TYPE_BR;

      case EM_DECODER_CR_REG:
        return REG_TYPE_CR;

      default:
        ASIMERROR("Unsupported register type");
        return REG_TYPE_INVALID;
    }
}


void
RecordOutputRegister(
    UINT32 cpuNum,
    ASIM_SOFTSDV_INST_INFO_CLASS * inst,
    const ASIM_CPUAPI_INST_INFO_CLASS &instInfo,
    const EM_Decoder_Operand_Info &operand)
{
    EM_Decoder_Reg_Type regType;
    UINT32 regNum;

    if (! cpuHistory[cpuNum]->DecodeRegister(operand, &regType, &regNum))
    {
        return;
    }

    if ((regType == EM_DECODER_PR_REG) || (regType == EM_DECODER_PR_ROT_REG))
    {
        inst->AddOutputRegister(
            ARCH_REGISTER_CLASS(REG_TYPE_PREDICATE_VECTOR64,
                                0,
                                inst->GetPredsAfter_Physical()));
    }
    else if (regType == EM_DECODER_PRED_REG)
    {
        inst->AddOutputRegister(
            ARCH_REGISTER_CLASS(REG_TYPE_PREDICATE,
                                regNum,
                                (inst->GetPredsAfter_Logical() >> regNum) & 1));
    }
    else if (regType == EM_DECODER_IP_REG)
    {
        // Do nothing.  Let Asim figure it out.
    }
    else
    {
        UINT64ArrayPair value;
        cpuHistory[cpuNum]->FetchCurrentRegisterValue(regType, regNum, value);

        inst->AddOutputRegister(
            ARCH_REGISTER_CLASS(MapRegisterType(regType),
                                regNum,
                                value.v[0],
                                value.v[1]));
    }
}


void
RecordInputRegister(
    UINT32 cpuNum,
    ASIM_SOFTSDV_INST_INFO_CLASS * inst,
    const ASIM_CPUAPI_INST_INFO_CLASS &instInfo,
    const EM_Decoder_Operand_Info &operand)
{
    EM_Decoder_Reg_Type regType;
    UINT32 regNum;

    if (! cpuHistory[cpuNum]->DecodeRegister(operand, &regType, &regNum))
    {
        return;
    }

    UINT64ArrayPair value;

    if ((regType == EM_DECODER_PR_REG) || (regType == EM_DECODER_PR_ROT_REG))
    {
        inst->AddInputRegister(
            ARCH_REGISTER_CLASS(REG_TYPE_PREDICATE_VECTOR64,
                                0,
                                inst->GetPredsBefore_Physical()));
    }
    else if (regType == EM_DECODER_PRED_REG)
    {
        inst->AddInputRegister(
            ARCH_REGISTER_CLASS(REG_TYPE_PREDICATE,
                                regNum,
                                (inst->GetPredsBefore_Logical() >> regNum) & 1));
    }
    else if (regType == EM_DECODER_IP_REG)
    {
        // Do nothing.  Let Asim figure it out.
    }
    else
    {
        if (cpuHistory[cpuNum]->ReadCachedRegisterValue(regType, regNum, value))
        {
            inst->AddInputRegister(
                ARCH_REGISTER_CLASS(MapRegisterType(regType),
                                    regNum,
                                    value.v[0],
                                    value.v[1]));
        }
        else
        {
            //
            // Add register name/number with unknown value
            //
            inst->AddInputRegister(
                ARCH_REGISTER_CLASS(MapRegisterType(regType),
                                    regNum));
        }
    }
}


void
RecordRegisterValues(
    UINT32 cpuNum,
    ASIM_SOFTSDV_INST_INFO_CLASS * inst,
    const ASIM_CPUAPI_INST_INFO_CLASS &instInfo)
{
    const EM_Decoder_Info *decode = cpuHistory[cpuNum]->DecodeSyllable(instInfo);

    if (decode->dst1.type != EM_DECODER_MEMORY)
    {
        RecordOutputRegister(cpuNum, inst, instInfo, decode->dst1);
    }
    else
    {
        // Memory address computation is an input
        RecordInputRegister(cpuNum, inst, instInfo, decode->dst1);
    }

    if (decode->dst2.type != EM_DECODER_MEMORY)
    {
        RecordOutputRegister(cpuNum, inst, instInfo, decode->dst2);
    }
    else
    {
        RecordInputRegister(cpuNum, inst, instInfo, decode->dst2);
    }

    RecordInputRegister(cpuNum, inst, instInfo, decode->src1);
    RecordInputRegister(cpuNum, inst, instInfo, decode->src2);
    RecordInputRegister(cpuNum, inst, instInfo, decode->src3);
    RecordInputRegister(cpuNum, inst, instInfo, decode->src4);
    RecordInputRegister(cpuNum, inst, instInfo, decode->src5);

    for (int i = 0; i < EM_DECODER_MAX_IMP_SRC; i++)
    {
        EM_Decoder_Imp_Operand implicitReg;
        implicitReg = (EM_Decoder_Imp_Operand) decode->static_info->implicit_src[i];

        cpuHistory[cpuNum]->HandleImplicitRegister(implicitReg, false, inst);
    }

    for (int i = 0; i < EM_DECODER_MAX_IMP_DST; i++)
    {
        EM_Decoder_Imp_Operand implicitReg;
        implicitReg = (EM_Decoder_Imp_Operand) decode->static_info->implicit_dst[i];

        cpuHistory[cpuNum]->HandleImplicitRegister(implicitReg, true, inst);
    }
}


void
CheckPhysicalTranslation(
    UINT32 cpuNum,
    const ASIM_CPUAPI_INST_INFO_CLASS &instInfo,
    const EM_Decoder_Info *decode)
{
    if (! instInfo.GetLoadInst() && ! instInfo.GetStoreInst())
    {
        return;
    }

    //
    // Some instructions look like memory but aren't really
    //
    switch (decode->inst)
    {
      case EM_THASH_R1_R3:
      case EM_TTAG_R1_R3:
        return;
    }

    if (instInfo.GetPhyMemAddr() == 0)
    {
        //
        // There can be no translation for many reasons:  instruction
        // predicated false, no translation and about to take a trap...
        //
        return;
    }

    //
    // At least the low 12 bits should match (4k pages)
    //
    if ((instInfo.GetPhyMemAddr() ^ instInfo.GetVirtMemAddr()) & 0x3ff)
    {
        fprintf(stderr, "Invalid V2P translation: 0x%0llx -> 0x%0llx\n",
                instInfo.GetVirtMemAddr(),
                instInfo.GetPhyMemAddr());

        instrDecoder.DisassembleInstr(stderr,
                                      instInfo.GetVirtAddr(),
                                      instInfo.GetInstBytes());
        exit(1);
    }
}


void
RecordMemoryValues(
    UINT32 cpuNum,
    ASIM_SOFTSDV_INST_INFO_CLASS * inst,
    const ASIM_CPUAPI_INST_INFO_CLASS &instInfo)
{
    if (! instInfo.GetLoadInst() && ! instInfo.GetStoreInst())
    {
        //
        // Not a memory instruction.
        //
        return;
    }

    //
    // Gambit fails if you try to read from the PIB (processor interrupt block).
    //
    if ((0xfee00000 <= instInfo.GetPhyMemAddr()) &&
        (instInfo.GetPhyMemAddr() < 0xff000000))
    {
        return;
    }

    //
    // Avoid I/O mapped memory.  Some components crash.
    //
    if (sysInfo.PhysicalAddrIsIO(instInfo.GetPhyMemAddr()))
    {
        return;
    }

    //
    // Get the cache line
    //
//
// Cache line code removed from feeder interface.  It was really there for
// one cache compression study which, as we expected, didn't work.  The
// interface didn't make sense for x86 since an instruction can touch more
// than one memory region.
//
//    ASIM_SOFTSDV_INST_INFO_CLASS::CACHE_LINE_VALUE cacheLine;
//
//    if (cpuInfo[cpuNum].ReadMemory(instInfo.GetPhyMemAddr() & ~ (SOFTSDV_CACHE_LINE_BYTES-1),
//                                   SOFTSDV_CACHE_LINE_BYTES,
//                                   cacheLine))
//    {
//        inst->SetCacheLineValue(cacheLine);
//    }
}


void
RecordContextSwitch(
    UINT32 cpuNum,
    ASIM_SOFTSDV_INST_INFO_CLASS * inst)
{
    UINT64 task_struct_base = cpuInfo[cpuNum].RegAR(6);

    int pid = int(task_struct_base);
    if (asimGuestOSInfo.GetPidOffset() != 0)
    {
        ASSERTX(cpuInfo[cpuNum].ReadMemory(task_struct_base + asimGuestOSInfo.GetPidOffset(),
                                           sizeof(pid), &pid));
    }
                
    char comm[32];
    comm[0] = 0;
    comm[17] = 0;
    if (asimGuestOSInfo.GetProcNameOffset() != 0)
    {
        ASSERTX(cpuInfo[cpuNum].ReadMemory(task_struct_base + asimGuestOSInfo.GetProcNameOffset(),
                                           16, comm));
    }

    inst->NoteContextSwitch(pid, comm);

//    printf("New context cpu %d, pid %d", cpuNum, pid);
//    if (comm[0])
//    {
//        printf(", name %s\n", comm);
//    }
//    else
//    {
//        printf("\n");
//    }
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
        // Just started program or returned from skipping.  Skip the first
        // instruction completely to avoid some SoftSDV bugs on the first
        // instruction.  Then skip one more instruction and use its result
        // to initialize register history.
        //
        cpuInfo[cpuNum].SetForceContextSwitch();

        //
        // Fetching WARMUP_CHUNK_SIZE during warm-up is an attempt to keep
        // fetching aligned to the same size as during functional mode,
        // though it probably doesn't matter.
        //
        unsigned int nFetch = cpuInfo[cpuNum].StateWarmup() ?
                              SOFTSDV_WARMUP_CHUNK_SIZE : 1;

        instInfo.Reset(nFetch);
        if (CPUAPI_Stat_Err == asimCpucCallBack->step_reference(
                                   cpuInfo[cpuNum].ControllerPid(),
                                   instInfo.GetSdvInstInfo(),
                                   nFetch))
        {
            //
            // Something isn't ready yet.  Just pretend and try to make
            // it happy.
            //
            *actual = requested;
            return CPUAPI_Stat_Ok;
        }

        cpuHistory[cpuNum]->ExecutedInstruction(instInfo[nFetch-1]);
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

    if (cpuInfo[cpuNum].CFM() != cpuHistory[cpuNum]->CFM())
    {
        registerValueError("CFM", cpuNum);
    }
    if (cpuInfo[cpuNum].Predicates() != cpuHistory[cpuNum]->LogicalPredicates())
    {
        registerValueError("Predicates", cpuNum);

        //
        // Predicate errors can be caused by incompatible visibility mode.
        // We expect to be in logical mode.
        //
        cpuapi_vis_config_t visibilityMode;
        if (CPUAPI_Stat_Ok == 
            asimCpucCallBack->get_object_by_name(cpuInfo[cpuNum].ControllerPid(),
                                                 1,
                                                 CPUAPI_SIM_VISIBILITY_CONFIG,
                                                 CPUAPI_SIM_VISIBILITY_CONFIG_SIZE,
                                                 &visibilityMode))
        {
            if (visibilityMode & CPUAPI_Visibility_Physical_View)
            {
                fprintf(stderr, "Somebody changed visibility mode to physical!\n");
            }
        }
        else
        {
            fprintf(stderr, "Failed to retrieve visibility state\n");
        }
    }
    if (asim_io.OSMode())
    {
        if (cpuInfo[cpuNum].BSP() != cpuHistory[cpuNum]->BSP())
        {
            registerValueError("BSP", cpuNum);
        }
    }

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
            nInstrsToFetch = ASIM_SOFTSDV_FETCH_INST_BUFFER_CLASS::maxChunkSize;
        }

        if (asim_io.RecordRegisterValues() ||
            asim_io.RecordMemoryValues())
        {
            nInstrsToFetch = 1;
        }

        ASSERT(asim_io.InstrRing(cpuNum).NWriteSlotsAvailable() >= nInstrsToFetch,
               "No instruction ring write slots available");

        instInfo.Reset(nInstrsToFetch);

        asimCpucCallBack->step_reference(cpuInfo[cpuNum].ControllerPid(),
                                         instInfo.GetSdvInstInfo(),
                                         nInstrsToFetch);

        for (unsigned int i = 0; i < nInstrsToFetch; i++)
        {
            if ((instInfo[i].GetInstId() == 0) && (instInfo[i].GetIcount() == 0))
            {
                //
                // Assume early termination (break point, end of workload, etc.)
                //
                earlyTermination = true;
                nInstrsToFetch = i;
                break;
            }

            ASIM_SOFTSDV_INST_INFO_CLASS * inst = asim_io.InstrRing(cpuNum).OpenNext();

            ASSERT(inst != NULL, "Remote Asim process exited");

            bool interrupt = false;

            //
            // Check expected IP and physical address against actual result
            //
            if (cpuHistory[cpuNum]->PrevOpRfi())
            {
                inst->SetReturnFromInterrupt(true);
            }

            //
            // Is the virtual address what we expected?  If not, assume
            // it is an interrupt.
            //
            if (expectedIP != instInfo[i].GetVirtAddr())
            {
                interrupt = true;

                //
                // Interrupts should be in the kernel
                //
                if (instInfo[i].GetVirtAddr() < 0xe000000000000000LL)
                {
                    fprintf(stderr,
                            "Expected IP error:  CPU %d, i %d, expect 0x%016llx, actual 0x%016llx\n",
                            cpuNum, i, expectedIP, instInfo[i].GetVirtAddr());

                    DumpEntireInstrVector(instInfo, nInstrsToFetch);
                }
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

            if (instInfo[i].GetVirtAddr() == asimGuestOSInfo.GetTimerInterruptVector())
            {
                inst->SetTimerInterrupt(true);
            }

            if (instInfo[i].GetVirtAddr() == asimGuestOSInfo.GetIdleAddr())
            {
                inst->SetIdleInstr(true);
            }

            static UINT64 uid = 0;
            uid += 1;
            inst->SetUid(uid);

            inst->SetSoftsdvId(instInfo[i].GetInstId());
            inst->SetBundle(instInfo[i].GetInstBytes());
            if (asim_io.MonitorDMA())
            {
                inst->SetAddrPhysical(dmaIdx->GetMaskedMem(instInfo[i].GetPhyAddr()));
            }
            else
            {
                inst->SetAddrPhysical(instInfo[i].GetPhyAddr());
            }
            inst->SetAddrVirtual(instInfo[i].GetVirtAddr());

            inst->SetPredsBefore_Logical(cpuHistory[cpuNum]->LogicalPredicates());
            inst->SetCfmBefore(cpuHistory[cpuNum]->CFM());
            inst->SetBspBefore(cpuHistory[cpuNum]->BSP());

            inst->SetPredsAfter_Logical(instInfo[i].GetPredicates());
            inst->SetCfmAfter(instInfo[i].GetCFM());
            inst->SetBspAfter(instInfo[i].GetBSP());

            const EM_Decoder_Info *decode = cpuHistory[cpuNum]->DecodeSyllable(instInfo[i]);

            //
            // If this is the first instruction in the stream send the
            // current context.
            //
            if (cpuInfo[cpuNum].ForceContextSwitch())
            {
                cpuInfo[cpuNum].ClearForceContextSwitch();

                RecordContextSwitch(cpuNum, inst);
            }

            //
            // Both Windows and Linux kernels live above 0xe000000000000000
            //
            if (instInfo[i].GetVirtAddr() >= 0xe000000000000000LL)
            {
                inst->SetKernelInstr(true);

                //
                // Linux kernel keeps process context in AR6.  Assume a context
                // switch if it is written.
                //
                if (decode->dst1.type == EM_DECODER_REGISTER)
                {
                    //
                    // Linux kernel keeps process context in AR6.  Assume a
                    // context switch if it is written.
                    //
                    if ((decode->dst1.reg_info.type == EM_DECODER_APP_REG) &&
                        (decode->dst1.reg_info.name == EM_DECODER_REG_AR6))
                    {
                        RecordContextSwitch(cpuNum, inst);
                    }

                    //
                    // ITM update.  Assume a timer interrupt.  (Only do this
                    // if we don't know the actual vector.)
                    //
                    if ((asimGuestOSInfo.GetTimerInterruptVector() == 0) &&
                        (decode->dst1.reg_info.type == EM_DECODER_CR_REG) &&
                        (decode->dst1.reg_info.name == EM_DECODER_REG_CR1))
                    {
                        inst->SetTimerInterrupt(true);
                    }
                }
            }

            switch (decode->inst)
            {
              case EM_NOP_I_IMM21:
              case EM_NOP_M_IMM21:
              case EM_NOP_B_IMM21:
              case EM_NOP_F_IMM21:
                if ((decode->src1.type == EM_DECODER_IMMEDIATE) &&
                    ((decode->src1.imm_info.val64.dw0_32 & 0x177000) == 0x177000))
                {
                    inst->SetInstrTag(decode->src1.imm_info.val64.dw0_32 & 0xfff);
                }
                break;

              case EM_ALLOC_R1_AR_PFS_I_L_O_R:
                //
                // Gambit has a bug somewhere allowing flags to be set for alloc.
                //
                instInfo[i].ClearBranchInst();
                instInfo[i].ClearLoadInst();
                instInfo[i].ClearStoreInst();
                break;

              default:
                break;
            }

            if (instInfo[i].GetBranchInst())
            {
                //
                // IPF ignores low 4 bits.  Make sure they are 0.  Don't do this
                // for RFI since it is syllable addressable.
                //
                UINT64 mask = ~0xf;
                if (decode->inst == EM_RFI)
                {
                    mask = ~0;
                }
                
                //
                // Try to find the physical page of the target.  This helps
                // Asim's instruction virtual to physical translation prediction.
                //
                UINT64 pa;
                cpuInfo[cpuNum].VirtualToPhysical(instInfo[i].GetBranchTargetVirtAddr(mask), &pa);
                if (asim_io.MonitorDMA())
                {
                    pa = dmaIdx->GetMaskedMem(pa);
                }

                inst->SetControl(instInfo[i].GetBranchTaken(),
                                 pa,
                                 instInfo[i].GetBranchTargetVirtAddr(mask));
            }
            else if (instInfo[i].GetLoadInst())
            {
                CheckPhysicalTranslation(cpuNum, instInfo[i], decode);
                UINT64 pa = instInfo[i].GetPhyMemAddr();
                if (asim_io.MonitorDMA())
                {
                    pa = dmaIdx->GetMaskedMem(pa);
                }
                inst->SetLoad(pa, instInfo[i].GetVirtMemAddr());
            }
            else if (instInfo[i].GetStoreInst())
            {
                CheckPhysicalTranslation(cpuNum, instInfo[i], decode);
                UINT64 pa = instInfo[i].GetPhyMemAddr();
                if (asim_io.MonitorDMA())
                {
                    pa = dmaIdx->GetMaskedMem(pa);
                }
                inst->SetStore(pa, instInfo[i].GetVirtMemAddr());
            }
            else
            {
                inst->SetNormal();
            }

            if (asim_io.RecordRegisterValues())
            {
                RecordRegisterValues(cpuNum, inst, instInfo[i]);
            }

            if (asim_io.RecordMemoryValues())
            {
                RecordMemoryValues(cpuNum, inst, instInfo[i]);
            }

            asim_io.InstrRing(cpuNum).Close(inst);

            //
            // Update history for the new instruction
            //
            cpuHistory[cpuNum]->ExecutedInstruction(instInfo[i]);
            expectedIP = cpuHistory[cpuNum]->NextIP_Virtual();
        }

        fetchInstrsRemaining -= nInstrsToFetch;
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

cpuapi_ipf_arch_specific_callbacks_t *
CPUAPI_ArchSpecificClbks(void)
{
    cpuapi_ipf_arch_specific_callbacks_t *cpu_ipf_clbk;
    cpu_ipf_clbk = (cpuapi_ipf_arch_specific_callbacks_t*)malloc(sizeof(cpuapi_ipf_arch_specific_callbacks_t));
    if (NULL == cpu_ipf_clbk)
    {
        return NULL;
    }

    cpu_ipf_clbk->io_port_access = io_port_access;
    cpu_ipf_clbk->processor_interrupt_block_access = processor_interrupt_block_access;
    return cpu_ipf_clbk;
}


void
registerValueError(
    char *regName,
    UINT32 cpuNum)
{
    fprintf(stderr, "%s reg error, CPU %d, V 0x%016llx (prev V 0x%016llx)\n",
            regName,
            cpuNum,
            cpuHistory[cpuNum]->NextIP_Virtual(),
            cpuHistory[cpuNum]->LastExecutedIP_Virtual());

    instrDecoder.DisassembleInstr(stderr,
                                  cpuHistory[cpuNum]->LastExecutedIP_Virtual(),
                                  cpuHistory[cpuNum]->DecodeInstBytes());

//    exit(1);
}


void
AsimInit_ArchSpecific(void)
{
    cpuInfo.Init(asim_io.NCpus());
    cpuHistory.Init(asim_io.NCpus());
}


void
AsimInitCpu_ArchSpecific(
    UINT32 cpuNum,
    const ASIM_REQUESTED_REGS_CLASS &regRqst)
{
    ASSERT(CPUAPI_SIM_INST_BYTES_LENGTH == 16,
           "ASIM: Expected 16 byte instructions\n");

    cpuInfo[cpuNum].InitHandles();

    if (cpuHistory[cpuNum])
    {
        delete cpuHistory[cpuNum];
    };

    cpuHistory[cpuNum] = new CPU_HISTORY_CLASS(cpuNum);

    if (cpuNum == 0)
    {
        sysInfo.Init();
    }
}


//------------------------------------------------------------------------
//
// Lock step
// 
//------------------------------------------------------------------------

//
// Not implemented for IPF
//

extern void
AsimStartLockStep(void)
{
    ASIMERROR("Lock step not implemented for IPF");
}


extern void
AsimStopLockStep(void)
{}


extern void
AsimLockStepDMA(UINT64 addr, UINT64 size)
{}
