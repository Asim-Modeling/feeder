/*****************************************************************************
 *
 * Copyright (C) 2005-2006 Intel Corporation
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

File:  cpuapi_arch_ia32.h

Description:    ia32 arch

*****************************************************************************/
#ifndef CPUAPI_ARCH_IA32_H
#define CPUAPI_ARCH_IA32_H

#include <stdio.h>
#include <cpuapi_mem.h>
#include <cpuapi.h>

#ifdef __cplusplus
extern "C"
{
#endif


typedef enum cpuapi_cpu_mode_s {
    CPUAPI_First_IA32_Cpu_Mode = CPUAPI_Cpu_Mode_Arch_1,
    CPUAPI_Cpu_Mode_8086 = CPUAPI_First_IA32_Cpu_Mode,
    CPUAPI_Cpu_Mode_Big_Real,
    CPUAPI_Cpu_Mode_v8086,
    CPUAPI_Cpu_Mode_Protected_16,
    CPUAPI_Cpu_Mode_Protected_32,
    CPUAPI_Cpu_Mode_Long_Compatible_16,
    CPUAPI_Cpu_Mode_Long_Compatible_32,
    CPUAPI_Cpu_Mode_Long_64,
    CPUAPI_Cpu_Mode_SMM,
    CPUAPI_Last_IA32_Cpu_Mode = CPUAPI_Cpu_Mode_SMM
} cpuapi_cpu_mode_t;

typedef enum
{
   CPUAPI_Origin_Unknown = 0x0,
   CPUAPI_Origin_Other  ,       /* memory origin that does not fall into any of the other categories */
   CPUAPI_Origin_Instruction  , /* memory required by the instruction itself (instruction operands)  */
   CPUAPI_Origin_Descriptor   , /* memory required during access to the descriptor tables of the CPU */
   CPUAPI_Origin_Task           /* memory required during access to the TSS table of the CPU         */
} cpuapi_origin_type_t;

typedef struct cpuapi_inst_memory_access_s
{
    cpuapi_access_type_t access;        /* access type read or write    */
    cpuapi_origin_type_t origin;        /* the origin of the access     */
    cpuapi_size_t        size;          /* access size in bytes         */
    cpuapi_phys_addr_t   phy_mem_addr;  /* memory physical address      */
    cpuapi_virt_addr_t   virt_mem_addr; /* memory virtual address       */
    cpuapi_u8_t *        data;          /* a pointer to the memory data */
    struct cpuapi_inst_memory_access_s *next; /* a pointer to the next memory access in the list */
} cpuapi_inst_memory_access_t;


#define CPUAPI_IA32_INST_BYTES_LENGTH             16

typedef struct {
    unsigned char                inst_bytes[CPUAPI_IA32_INST_BYTES_LENGTH]; /* instruction bytes */
    cpuapi_u32_t                 inst_bytes_size;
    cpuapi_boolean_t             new_inst;
    cpuapi_u32_t                 eflags;
    cpuapi_u32_t                 cur_repeat_count;
    cpuapi_boolean_t             branch_inst;
    cpuapi_boolean_t             branch_taken;
    cpuapi_virt_addr_t           branch_target_virt_addr;    /* instruction virtual address */
    cpuapi_size_t                num_of_accesses;            /* number of memory accesses in the access list*/
    cpuapi_inst_memory_access_t* access_list;                /* array of accesses memory accesses*/
    cpuapi_boolean_t             exception_interrupt;        /* A flag indicating that an exception \ interrupt has occured during execution of the instruction */
    cpuapi_cpu_mode_t            cpu_mode;                   /* the cpu mode during the execution of the instruction */
    cpuapi_boolean_t             is_io;                      /* is this an I/O instruction ? */
    cpuapi_access_type_t         io_access_type;             /* read/write (IN/OUT) type of I/O access*/
    cpuapi_io_addr_t             io_port;                    /* port no. for IO instruction */
    cpuapi_size_t                io_access_size;             /* size of I/O access */
    cpuapi_u64_t                 io_value;                   /* value wrote/obtained by I/O access */
} cpuapi_ia32_inst_info_t;

#define MAX_ARCH_NAME_LENGTH 80

/*
   See CPU segment descriptor format in
   IA-32 Architecture Software Developer’s Manual, vol.3
   3.4.3. "Segment Descriptors"
*/
    typedef struct cpuapi_descriptor_s {
        cpuapi_u32_t            raw_data[4];
    } cpuapi_descriptor_t;

typedef struct{
  unsigned base_0_7:8,
  type:4,
  s:1,
  dpl:2,
  present:1,
  limit_16_19:4,
  avl:1,
  reserved1:1,
  db:1,
  granularity:1,
  base_24_31:8;
} cpuapi_descriptor_arbyte_t;


#define CPUAPI_IA32_RAX  "arch.ia32.register.longmode.rax"
#define CPUAPI_IA32_RBX  "arch.ia32.register.longmode.rbx"
#define CPUAPI_IA32_RCX  "arch.ia32.register.longmode.rcx"
#define CPUAPI_IA32_RDX  "arch.ia32.register.longmode.rdx"
#define CPUAPI_IA32_RSI  "arch.ia32.register.longmode.rsi"
#define CPUAPI_IA32_RDI  "arch.ia32.register.longmode.rdi"
#define CPUAPI_IA32_RBP  "arch.ia32.register.longmode.rbp"
#define CPUAPI_IA32_RSP  "arch.ia32.register.longmode.rsp"
#define CPUAPI_IA32_R8   "arch.ia32.register.longmode.r8"
#define CPUAPI_IA32_R9   "arch.ia32.register.longmode.r9"
#define CPUAPI_IA32_R10  "arch.ia32.register.longmode.r10"
#define CPUAPI_IA32_R11  "arch.ia32.register.longmode.r11"
#define CPUAPI_IA32_R12  "arch.ia32.register.longmode.r12"
#define CPUAPI_IA32_R13  "arch.ia32.register.longmode.r13"
#define CPUAPI_IA32_R14  "arch.ia32.register.longmode.r14"
#define CPUAPI_IA32_R15  "arch.ia32.register.longmode.r15"

#define CPUAPI_IA32_EAX  "arch.ia32.register.general.eax"
#define CPUAPI_IA32_EBX  "arch.ia32.register.general.ebx"
#define CPUAPI_IA32_ECX  "arch.ia32.register.general.ecx"
#define CPUAPI_IA32_EDX  "arch.ia32.register.general.edx"
#define CPUAPI_IA32_ESI  "arch.ia32.register.general.esi"
#define CPUAPI_IA32_EDI  "arch.ia32.register.general.edi"
#define CPUAPI_IA32_EBP  "arch.ia32.register.general.ebp"
#define CPUAPI_IA32_ESP  "arch.ia32.register.general.esp"
#define CPUAPI_IA32_R8D  "arch.ia32.register.general.r8d"
#define CPUAPI_IA32_R9D  "arch.ia32.register.general.r9d"
#define CPUAPI_IA32_R10D "arch.ia32.register.general.r10d"
#define CPUAPI_IA32_R11D "arch.ia32.register.general.r11d"
#define CPUAPI_IA32_R12D "arch.ia32.register.general.r12d"
#define CPUAPI_IA32_R13D "arch.ia32.register.general.r13d"
#define CPUAPI_IA32_R14D "arch.ia32.register.general.r14d"
#define CPUAPI_IA32_R15D "arch.ia32.register.general.r15d"

#define CPUAPI_IA32_AX  "arch.ia32.register.word.ax"
#define CPUAPI_IA32_BX  "arch.ia32.register.word.bx"
#define CPUAPI_IA32_CX  "arch.ia32.register.word.cx"
#define CPUAPI_IA32_DX  "arch.ia32.register.word.dx"
#define CPUAPI_IA32_SI  "arch.ia32.register.word.si"
#define CPUAPI_IA32_DI  "arch.ia32.register.word.di"
#define CPUAPI_IA32_BP  "arch.ia32.register.word.bp"
#define CPUAPI_IA32_SP  "arch.ia32.register.word.sp"
#define CPUAPI_IA32_R8W  "arch.ia32.register.word.r8w"
#define CPUAPI_IA32_R9W  "arch.ia32.register.word.r9w"
#define CPUAPI_IA32_R10W "arch.ia32.register.word.r10w"
#define CPUAPI_IA32_R11W "arch.ia32.register.word.r11w"
#define CPUAPI_IA32_R12W "arch.ia32.register.word.r12w"
#define CPUAPI_IA32_R13W "arch.ia32.register.word.r13w"
#define CPUAPI_IA32_R14W "arch.ia32.register.word.r14w"
#define CPUAPI_IA32_R15W "arch.ia32.register.word.r15w"

#define CPUAPI_IA32_AL  "arch.ia32.register.byte.al"
#define CPUAPI_IA32_BL  "arch.ia32.register.byte.bl"
#define CPUAPI_IA32_CL  "arch.ia32.register.byte.cl"
#define CPUAPI_IA32_DL  "arch.ia32.register.byte.dl"
#define CPUAPI_IA32_AH  "arch.ia32.register.byte.ah"
#define CPUAPI_IA32_BH  "arch.ia32.register.byte.bh"
#define CPUAPI_IA32_CH  "arch.ia32.register.byte.ch"
#define CPUAPI_IA32_DH  "arch.ia32.register.byte.dh"
#define CPUAPI_IA32_SIL  "arch.ia32.register.byte.sil"
#define CPUAPI_IA32_DIL  "arch.ia32.register.byte.dil"
#define CPUAPI_IA32_BPL  "arch.ia32.register.byte.bpl"
#define CPUAPI_IA32_SPL  "arch.ia32.register.byte.spl"
#define CPUAPI_IA32_R8B  "arch.ia32.register.byte.r8l"
#define CPUAPI_IA32_R9B  "arch.ia32.register.byte.r9l"
#define CPUAPI_IA32_R10B "arch.ia32.register.byte.r10l"
#define CPUAPI_IA32_R11B "arch.ia32.register.byte.r11l"
#define CPUAPI_IA32_R12B "arch.ia32.register.byte.r12l"
#define CPUAPI_IA32_R13B "arch.ia32.register.byte.r13l"
#define CPUAPI_IA32_R14B "arch.ia32.register.byte.r14l"
#define CPUAPI_IA32_R15B "arch.ia32.register.byte.r15l"

#define CPUAPI_IA32_CS_SEL "arch.ia32.register.segment.cs.sel"
#define CPUAPI_IA32_SS_SEL "arch.ia32.register.segment.ss.sel"
#define CPUAPI_IA32_DS_SEL "arch.ia32.register.segment.ds.sel"
#define CPUAPI_IA32_ES_SEL "arch.ia32.register.segment.es.sel"
#define CPUAPI_IA32_FS_SEL "arch.ia32.register.segment.fs.sel"
#define CPUAPI_IA32_GS_SEL "arch.ia32.register.segment.gs.sel"

#define CPUAPI_IA32_CS_DESC "arch.ia32.register.segment.cs.desc"
#define CPUAPI_IA32_SS_DESC "arch.ia32.register.segment.ss.desc"
#define CPUAPI_IA32_DS_DESC "arch.ia32.register.segment.ds.desc"
#define CPUAPI_IA32_ES_DESC "arch.ia32.register.segment.es.desc"
#define CPUAPI_IA32_FS_DESC "arch.ia32.register.segment.fs.desc"
#define CPUAPI_IA32_GS_DESC "arch.ia32.register.segment.gs.desc"

#define CPUAPI_IA32_EIP     "arch.ia32.register.eip"
#define CPUAPI_IA32_EFLAGS  "arch.ia32.register.eflags"
#define CPUAPI_IA32_RIP     "arch.ia32.register.rip"

#define CPUAPI_IA32_CR_BASE_NAME "arch.ia32.register.control."
#define CPUAPI_IA32_CR_NAME(str,num) sprintf(str,"%s%u",CPUAPI_IA32_CR_BASE_NAME,num)
#define CPUAPI_IA32_CR_NUM 5

#define CPUAPI_IA32_CR0 "arch.ia32.register.control.0"
#define CPUAPI_IA32_CR1 "arch.ia32.register.control.1"
#define CPUAPI_IA32_CR2 "arch.ia32.register.control.2"
#define CPUAPI_IA32_CR3 "arch.ia32.register.control.3"
#define CPUAPI_IA32_CR4 "arch.ia32.register.control.4"
#define CPUAPI_IA32_CR8 "arch.ia32.register.control.8"

#define CPUAPI_IA32_DR_BASE_NAME "arch.ia32.register.debug."
#define CPUAPI_IA32_DR_NAME(str,num) sprintf(str,"%s%u",CPUAPI_IA32_DR_BASE_NAME,num)
#define CPUAPI_IA32_DR_NUM 8

#define CPUAPI_IA32_DR0 "arch.ia32.register.debug.0"
#define CPUAPI_IA32_DR1 "arch.ia32.register.debug.1"
#define CPUAPI_IA32_DR2 "arch.ia32.register.debug.2"
#define CPUAPI_IA32_DR3 "arch.ia32.register.debug.3"
#define CPUAPI_IA32_DR4 "arch.ia32.register.debug.4"
#define CPUAPI_IA32_DR5 "arch.ia32.register.debug.5"
#define CPUAPI_IA32_DR6 "arch.ia32.register.debug.6"
#define CPUAPI_IA32_DR7 "arch.ia32.register.debug.7"

#define CPUAPI_IA32_LDTR_SEL   "arch.ia32.register.ldtr.sel"
#define CPUAPI_IA32_LDTR_DESC  "arch.ia32.register.ldtr.desc"
#define CPUAPI_IA32_TR_SEL     "arch.ia32.register.tr.sel"
#define CPUAPI_IA32_TR_DESC    "arch.ia32.register.tr.desc"

#define CPUAPI_IA32_GDTR_BASE  "arch.ia32.register.gdtr.base"
#define CPUAPI_IA32_GDTR_LIMIT "arch.ia32.register.gdtr.limit"

#define CPUAPI_IA32_IDTR_BASE  "arch.ia32.register.idtr.base"
#define CPUAPI_IA32_IDTR_LIMIT "arch.ia32.register.idtr.limit"

#define CPUAPI_IA32_MXCSR "arch.ia32.register.xmm.mxcsr"

#define CPUAPI_IA32_XMM_BASE_NAME "arch.ia32.register.xmm."
#define CPUAPI_IA32_XMM_NAME(str,num) sprintf(str,"%s%u",CPUAPI_IA32_XMM_BASE_NAME,num)
#define CPUAPI_IA32_XMM_NUM 8

#define CPUAPI_IA32_XMM0 "arch.ia32.register.xmm.0"
#define CPUAPI_IA32_XMM1 "arch.ia32.register.xmm.1"
#define CPUAPI_IA32_XMM2 "arch.ia32.register.xmm.2"
#define CPUAPI_IA32_XMM3 "arch.ia32.register.xmm.3"
#define CPUAPI_IA32_XMM4 "arch.ia32.register.xmm.4"
#define CPUAPI_IA32_XMM5 "arch.ia32.register.xmm.5"
#define CPUAPI_IA32_XMM6 "arch.ia32.register.xmm.6"
#define CPUAPI_IA32_XMM7 "arch.ia32.register.xmm.7"
#define CPUAPI_IA32_XMM8  "arch.ia32.register.xmm.8"
#define CPUAPI_IA32_XMM9  "arch.ia32.register.xmm.9"
#define CPUAPI_IA32_XMM10 "arch.ia32.register.xmm.10"
#define CPUAPI_IA32_XMM11 "arch.ia32.register.xmm.11"
#define CPUAPI_IA32_XMM12 "arch.ia32.register.xmm.12"
#define CPUAPI_IA32_XMM13 "arch.ia32.register.xmm.13"
#define CPUAPI_IA32_XMM14 "arch.ia32.register.xmm.14"
#define CPUAPI_IA32_XMM15 "arch.ia32.register.xmm.15"

#define CPUAPI_IA32_MSR_BASE_NAME "arch.ia32.register.msr."
#define CPUAPI_IA32_MSR_NAME(str,num) sprintf(str,"%s%u",CPUAPI_IA32_MSR_BASE_NAME,num)
#define CPUAPI_IA32_MSR_NUM 3000

#define CPUAPI_IA32_MSR_EFER            "arch.ia32.register.msr.3221225600"
#define CPUAPI_IA32_MSR_FS_BASE         "arch.ia32.register.msr.3221225728"
#define CPUAPI_IA32_MSR_GS_BASE         "arch.ia32.register.msr.3221225729"
#define CPUAPI_IA32_MSR_KERNEL_GS_BASE  "arch.ia32.register.msr.3221225730"
#define CPUAPI_IA32_MSR_STAR            "arch.ia32.register.msr.3221225601"
#define CPUAPI_IA32_MSR_LSTAR           "arch.ia32.register.msr.3221225602"
#define CPUAPI_IA32_MSR_CSTAR           "arch.ia32.register.msr.3221225603"
#define CPUAPI_IA32_MSR_FMASK           "arch.ia32.register.msr.3221225604"

//FPU Control Register - 16 bits
#define CPUAPI_IA32_FCONTROL "arch.ia32.register.fp.control"
//FPU Status Register - 16 bits
#define CPUAPI_IA32_FSTATUS "arch.ia32.register.fp.status"
//FPU Tag Register - 16 bits
#define CPUAPI_IA32_FTAG "arch.ia32.register.fp.tag"
//FPU Opcode Register - 16 bits
#define CPUAPI_IA32_FOPCODE "arch.ia32.register.fp.opcode"
//FPU Instruction pointer - 48 bits - 4 bytes
#define CPUAPI_IA32_FINST_SEL "arch.ia32.register.fp.inst.sel"
#define CPUAPI_IA32_FINST_OFFSET "arch.ia32.register.fp.inst.offset"
//FPU Data (Operand) Pointer Register - 48 bits - 4 bytes
#define CPUAPI_IA32_FDATA_SEL "arch.ia32.register.fp.data.sel"
#define CPUAPI_IA32_FDATA_OFFSET "arch.ia32.register.fp.data.offset"

#define CPUAPI_IA32_FR_BASE_NAME "arch.ia32.register.fr."
#define CPUAPI_IA32_FR_NAME_(str,num) sprintf(str,"%s%u",CPUAPI_IA32_FR_BASE_NAME,num)
#define CPUAPI_IA32_FR_NUM 8

#define CPUAPI_IA32_FR0 "arch.ia32.register.fr.0"
#define CPUAPI_IA32_FR1 "arch.ia32.register.fr.1"
#define CPUAPI_IA32_FR2 "arch.ia32.register.fr.2"
#define CPUAPI_IA32_FR3 "arch.ia32.register.fr.3"
#define CPUAPI_IA32_FR4 "arch.ia32.register.fr.4"
#define CPUAPI_IA32_FR5 "arch.ia32.register.fr.5"
#define CPUAPI_IA32_FR6 "arch.ia32.register.fr.6"
#define CPUAPI_IA32_FR7 "arch.ia32.register.fr.7"

#define CPUAPI_IA32_CPUID0 "arch.ia32.register.cpuid.0"
#define CPUAPI_IA32_CPUID1 "arch.ia32.register.cpuid.1"
#define CPUAPI_IA32_CPUID2 "arch.ia32.register.cpuid.2"
#define CPUAPI_IA32_CPUID3 "arch.ia32.register.cpuid.3"
#define CPUAPI_IA32_CPUID4 "arch.ia32.register.cpuid.4"
#define CPUAPI_IA32_CPUID5 "arch.ia32.register.cpuid.5"
#define CPUAPI_IA32_CPUID_EXT0 "arch.ia32.register.cpuid.ext.0"
#define CPUAPI_IA32_CPUID_EXT1 "arch.ia32.register.cpuid.ext.1"
#define CPUAPI_IA32_CPUID_EXT2 "arch.ia32.register.cpuid.ext.2"
#define CPUAPI_IA32_CPUID_EXT3 "arch.ia32.register.cpuid.ext.3"
#define CPUAPI_IA32_CPUID_EXT4 "arch.ia32.register.cpuid.ext.4"
#define CPUAPI_IA32_CPUID_EXT5 "arch.ia32.register.cpuid.ext.5"
#define CPUAPI_IA32_CPUID_EXT6 "arch.ia32.register.cpuid.ext.6"
#define CPUAPI_IA32_CPUID_EXT7 "arch.ia32.register.cpuid.ext.7"
#define CPUAPI_IA32_CPUID_EXT8 "arch.ia32.register.cpuid.ext.8"

//Local APIC registers names
//address FEE0 0000H - FEE0 03F0H
#define CPUAPI_IA32_LOCAL_APIC_SPACE              "arch.ia32.local_apic.entire_space"  //size 3F0H
//address  FEE0 0020H
#define CPUC_LOCAL_APIC_ID_ADDRESS 0x20
#define CPUAPI_IA32_LOCAL_APIC_ID                 "arch.ia32.local_apic.id" //size 4
//address FEE0 0030H
#define CPUC_LOCAL_APIC_VERSION_ADDRESS 0x30
#define CPUAPI_IA32_LOCAL_APIC_VERSION            "arch.ia32.local_apic.version" //size 4
//address FEE0 0080H
#define CPUC_LOCAL_APIC_TASK_PRIO_ADDRESS 0x80
#define CPUAPI_IA32_LOCAL_APIC_TASK_PRIO          "arch.ia32.local_apic.task_priority" //size 4
//address FEE0 0090H
#define CPUC_LOCAL_APIC_ARB_PRIO_ADDRESS 0x90
#define CPUAPI_IA32_LOCAL_APIC_ARB_PRIO           "arch.ia32.local_apic.arbitration_priority" //size 4
//address FEE0 00A0H
#define CPUC_LOCAL_APIC_PROC_PRIO_ADDRESS 0xA0
#define CPUAPI_IA32_LOCAL_APIC_PROC_PRIO          "arch.ia32.local_apic.processor_prio" //size 4
//address FEE0 00B0H
#define CPUC_LOCAL_APIC_EOI_ADDRESS 0xB0
#define CPUAPI_IA32_LOCAL_APIC_EOI                "arch.ia32.local_apic.eoi" //size 4
//address FEE0 00D0H
#define CPUC_LOCAL_APIC_LOG_DST_ADDRESS 0xD0
#define CPUAPI_IA32_LOCAL_APIC_LOG_DST            "arch.ia32.local_apic.logical_destination" //size 4
//address FEE0 00E0H
#define CPUC_LOCAL_APIC_DST_FORMAT_ADDRESS 0xE0
#define CPUAPI_IA32_LOCAL_APIC_DST_FORMAT         "arch.ia32.local_apic.destination_format" //size 4
//address FEE0 00F0H
#define CPUC_LOCAL_APIC_S_INT_VEC_ADDRESS 0xF0
#define CPUAPI_IA32_LOCAL_APIC_S_INT_VEC          "arch.ia32.local_apic.spurious_interrupt_vector" //size 4
//address FEE0 0100H
#define CPUC_LOCAL_APIC_ISR_ADDRESS     0x100
#define CPUAPI_IA32_LOCAL_APIC_ISR                "arch.ia32.local_apic.isr" //size 32
//address FEE0 0180H
#define CPUC_LOCAL_APIC_TMR_ADDRESS 0x180
#define CPUAPI_IA32_LOCAL_APIC_TMR                "arch.ia32.local_apic.tmr" //size 32
//address FEE0 0200H
#define CPUC_LOCAL_APIC_IRR_ADDRESS 0x200
#define CPUAPI_IA32_LOCAL_APIC_IRR                "arch.ia32.local_apic.irr" //size 32
//address FEE0 0280H
#define CPUC_LOCAL_APIC_ERR_STATUS_ADDRESS 0x280
#define CPUAPI_IA32_LOCAL_APIC_ERR_STATUS         "arch.ia32.local_apic.error_status" //size 4
//address FEE0 0300H
#define CPUC_LOCAL_APIC_INT_CMD_LOW_ADDRESS 0x300
#define CPUAPI_IA32_LOCAL_APIC_INT_CMD_LOW        "arch.ia32.local_apic.interrupt_command.low" //size 4
//address FEE0 0310H
#define CPUC_LOCAL_APIC_INT_CMD_HIGH_ADDRESS 0x310
#define CPUAPI_IA32_LOCAL_APIC_INT_CMD_HIGH       "arch.ia32.local_apic.interrupt_command.high" //size 4
//address FEE0 0320H
#define CPUC_LOCAL_APIC_LVT_TIMER_ADDRESS 0x320
#define CPUAPI_IA32_LOCAL_APIC_LVT_TIMER          "arch.ia32.local_apic.lvt.timer" //size 4
//address FEE0 0330H
#define CPUC_LOCAL_APIC_LVT_THERMAL_ADDRESS 0x330
#define CPUAPI_IA32_LOCAL_APIC_LVT_THERMAL        "arch.ia32.local_apic.lvt.thermal_monitor" //size 4
//address FEE0 0340H
#define CPUC_LOCAL_APIC_LVT_PRF_CNT_ADDRESS 0x340
#define CPUAPI_IA32_LOCAL_APIC_LVT_PRF_CNT        "arch.ia32.local_apic.lvt.performance_conter" //size 4
//address FEE0 0350H
#define CPUC_LOCAL_APIC_LVT_LINT_0_ADDRESS 0x350
#define CPUAPI_IA32_LOCAL_APIC_LVT_LINT_0         "arch.ia32.local_apic.lvt.lint.0" //size 4
//address FEE0 0360H
#define CPUC_LOCAL_APIC_LVT_LINT_1_ADDRESS 0x360
#define CPUAPI_IA32_LOCAL_APIC_LVT_LINT_1         "arch.ia32.local_apic.lvt.lint.1" //size 4
//address FEE0 0370H
#define CPUC_LOCAL_APIC_ERR_REG_ADDRESS 0x370
#define CPUAPI_IA32_LOCAL_APIC_ERR_REG            "arch.ia32.local_apic.error_register" //size 4
//address FEE0 0380H
#define CPUC_LOCAL_APIC_TIMER_INIT_ADDRESS 0x380
#define CPUAPI_IA32_LOCAL_APIC_TIMER_INIT         "arch.ia32.local_apic.timer.initial_count" //size 4
//address FEE0 0390H
#define CPUC_LOCAL_APIC_TIMER_CURR_ADDRESS 0x390
#define CPUAPI_IA32_LOCAL_APIC_TIMER_CURR         "arch.ia32.local_apic.timer.current_count" //size 4
//address FEE0 0E0H
#define CPUC_LOCAL_APIC_TIMER_DIV_CONFIG_ADDRESS 0x3E0
#define CPUAPI_IA32_LOCAL_APIC_TIMER_DIV_CONFIG   "arch.ia32.local_apic.timer.divide_configuration" //size 4

#define CPUAPI_IA32_A20_PIN        "arch.ia32.pin.a20"
#define CPUAPI_IA32_INTR_PIN       "arch.ia32.pin.intr"
#define CPUAPI_IA32_RESET_PIN      "arch.ia32.pin.reset"
#define CPUAPI_IA32_NMI_PIN        "arch.ia32.pin.nmi"
#define CPUAPI_IA32_SMI_PIN        "arch.ia32.pin.smi"
#define CPUAPI_IA32_PSMI_PIN       "arch.ia32.pin.psmi"

//Indicates that STI instruction was executed, but still one instruction
//should be executed before the interrupts are enabled.
//For more information about this instruction please refer to
//The IA-32 Intel Architechture Software Developer's Manual, Volume 2
#define CPUAPI_IA32_STI_SET                      "arch.ia32.sti.set"

#define CPUAPI_IA32_MICROCODE_UPDATE_ID     "arch.ia32.microcode.update.id"
#define CPUAPI_IA32_MICROCODE_UPDATE_ID_SIZE  8

#ifndef VMX_OFF
// LaGrande Technology

#define CPUAPI_IA32_LT_VMX_MODE_ACTIVE                                              "arch.ia32.lt.vm_mode_active"
#define CPUAPI_IA32_LT_VMX_ROOT_MODE_ACTIVE                                         "arch.ia32.lt.vmx_root_mode_active"
#define CPUAPI_IA32_LT_SMM_MONITOR_CONFIGURED                                       "arch.ia32.lt.smm_monitor_configured"
#define CPUAPI_IA32_LT_MTF_ACTIVE                                                   "arch.ia32.lt.mtf_active"
#endif

#ifndef SMX_OFF
#define CPUAPI_IA32_LT_SENTER_FLAG                                                  "arch.ia32.lt.senter_flag"
#define CPUAPI_IA32_LT_ENTERAC_FLAG                                                 "arch.ia32.lt.enterac_flag"
#endif

#ifndef VMX_OFF
    //  VMCS fields
#define CPUAPI_IA32_LT_VMCS_C_POINTER                                                 "arch.ia32.lt.vmcs.c.pointer"

#define CPUAPI_IA32_LT_VMCS_C_REVISION                                                "arch.ia32.lt.vmcs.c.revision"
#define CPUAPI_IA32_LT_VMCS_C_VMX_ABORT_INDICATOR                                     "arch.ia32.lt.vmcs.c.vmx_abort_indicator"
#define CPUAPI_IA32_LT_VMCS_C_PARENT_POINTER                                          "arch.ia32.lt.vmcs.c.parent_pointer"
#define CPUAPI_IA32_LT_VMCS_C_LAUNCH_STATE                                            "arch.ia32.lt.vmcs.c.launch_state"

    // VM Execution Control Fields

#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_PIN_BASED_CONTROLS                         "arch.ia32.lt.vmcs.c.vm_execution_control.pin_based_controls"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_PROCESSOR_BASED_CONTROLS                   "arch.ia32.lt.vmcs.c.vm_execution_control.processor_based_controls"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_EXCEPTION_BITMAP                   "arch.ia32.lt.vmcs.c.vm_execution_control.exception_bitmap"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_CR3_TARGET_COUNT                   "arch.ia32.lt.vmcs.c.vm_execution_control.cr3.target_count"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_CR0_GUEST_HOST_MASK                "arch.ia32.lt.vmcs.c.vm_execution_control.cr0.guest_host_mask"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_CR3_GUEST_HOST_MASK                "arch.ia32.lt.vmcs.c.vm_execution_control.cr3.guest_host_mask"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_CR4_GUEST_HOST_MASK                "arch.ia32.lt.vmcs.c.vm_execution_control.cr4.guest_host_mask"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_CR8_GUEST_HOST_MASK                "arch.ia32.lt.vmcs.c.vm_execution_control.cr8.guest_host_mask"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_CR0_READ_SHADOW                    "arch.ia32.lt.vmcs.c.vm_execution_control.cr0.read_shadow"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_CR3_READ_SHADOW                    "arch.ia32.lt.vmcs.c.vm_execution_control.cr3.read_shadow"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_CR4_READ_SHADOW                    "arch.ia32.lt.vmcs.c.vm_execution_control.cr4.read_shadow"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_CR8_READ_SHADOW                    "arch.ia32.lt.vmcs.c.vm_execution_control.cr8.read_shadow"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_CR3_TARGET_VALUE_FIRST             "arch.ia32.lt.vmcs.c.vm_execution_control.cr3.target_value.first"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_CR3_TARGET_VALUE_LAST              "arch.ia32.lt.vmcs.c.vm_execution_control.cr3.target_value.last"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_CR3_TARGET_VALUE_0                 "arch.ia32.lt.vmcs.c.vm_execution_control.cr3.target_value.0"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_CR3_TARGET_VALUE_1                 "arch.ia32.lt.vmcs.c.vm_execution_control.cr3.target_value.1"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_CR3_TARGET_VALUE_2                 "arch.ia32.lt.vmcs.c.vm_execution_control.cr3.target_value.2"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_CR3_TARGET_VALUE_3                 "arch.ia32.lt.vmcs.c.vm_execution_control.cr3.target_value.3"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_PAGE_FAULT_ERROR_CODE_MASK         "arch.ia32.lt.vmcs.c.vm_execution_control.page_fault.error_code_mask"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_PAGE_FAULT_ERROR_CODE_MATCH        "arch.ia32.lt.vmcs.c.vm_execution_control.page_fault.error_code_match"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_IO_BITMAP_A_ALL                    "arch.ia32.lt.vmcs.c.vm_execution_control.io_bitmap.a_all"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_IO_BITMAP_A_HIGH                   "arch.ia32.lt.vmcs.c.vm_execution_control.io_bitmap.a_high"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_IO_BITMAP_B_ALL                    "arch.ia32.lt.vmcs.c.vm_execution_control.io_bitmap.b_all"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_IO_BITMAP_B_HIGH                   "arch.ia32.lt.vmcs.c.vm_execution_control.io_bitmap.b_high"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_TSC_COMPARATOR_ALL                 "arch.ia32.lt.vmcs.c.vm_execution_control.tsc.comparator_all"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_TSC_COMPARATOR_HIGH                "arch.ia32.lt.vmcs.c.vm_execution_control.tsc.comparator_high"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_TSC_OFFSET_ALL                     "arch.ia32.lt.vmcs.c.vm_execution_control.tsc.offset_all"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_TSC_OFFSET_HIGH                    "arch.ia32.lt.vmcs.c.vm_execution_control.tsc.offset_high"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_OSV_CONTROLLING_VMCS_POINTER_ALL   "arch.ia32.lt.vmcs.c.vm_execution_control.osv_controlling_vmcs_pointer_all"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_OSV_CONTROLLING_VMCS_POINTER_HIGH  "arch.ia32.lt.vmcs.c.vm_execution_control.osv_controlling_vmcs_pointer_high"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_MSR_BITMAPS_ADDRESS_ALL            "arch.ia32.lt.vmcs.c.vm_execution_control.msr_bitmaps_address_all"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXECUTION_CONTROL_MSR_BITMAPS_ADDRESS_HIGH           "arch.ia32.lt.vmcs.c.vm_execution_control.msr_bitmaps_address_high"

    // VM Exit Control Fields

#define CPUAPI_IA32_LT_VMCS_C_VM_EXIT_CONTROL_CONTROLS                                "arch.ia32.lt.vmcs.c.vm_exit_control.controls"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXIT_CONTROL_MSR_STORE_COUNT                         "arch.ia32.lt.vmcs.c.vm_exit_control.msr.store_count"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXIT_CONTROL_MSR_LOAD_COUNT                          "arch.ia32.lt.vmcs.c.vm_exit_control.msr.load_count"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXIT_MSR_STORE_ADDRESS_ALL                           "arch.ia32.lt.vmcs.c.vm_exit_control.msr.store_address_all"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXIT_MSR_STORE_ADDRESS_HIGH                          "arch.ia32.lt.vmcs.c.vm_exit_control.msr.store_address_high"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXIT_MSR_LOAD_ADDRESS_ALL                            "arch.ia32.lt.vmcs.c.vm_exit_control.msr.load_address_all"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXIT_MSR_LOAD_ADDRESS_HIGH                           "arch.ia32.lt.vmcs.c.vm_exit_control.msr.load_address_high"

    // VM Entry Control Fields

#define CPUAPI_IA32_LT_VMCS_C_VM_ENTRY_CONTROL_CONTROLS                               "arch.ia32.lt.vmcs.c.vm_entry_control.controls"
#define CPUAPI_IA32_LT_VMCS_C_VM_ENTRY_CONTROL_INTERRUPTION_INFORMATION               "arch.ia32.lt.vmcs.c.vm_entry_control.interruption_information"
#define CPUAPI_IA32_LT_VMCS_C_VM_ENTRY_CONTROL_EXCEPTION_ERROR_CODE                   "arch.ia32.lt.vmcs.c.vm_entry_control.exception_error_code"
#define CPUAPI_IA32_LT_VMCS_C_VM_ENTRY_CONTROL_MSR_LOAD_COUNT                         "arch.ia32.lt.vmcs.c.vm_entry_control.msr.load_count"
#define CPUAPI_IA32_LT_VMCS_C_VM_ENTRY_MSR_LOAD_ADDRESS_ALL                           "arch.ia32.lt.vmcs.c.vm_entry_control.msr.load_address_all"
#define CPUAPI_IA32_LT_VMCS_C_VM_ENTRY_MSR_LOAD_ADDRESS_HIGH                          "arch.ia32.lt.vmcs.c.vm_entry_control.msr.load_address_high"
#define CPUAPI_IA32_LT_VMCS_C_VM_ENTRY_CONTROL_INSTRUCTION_LENGTH                     "arch.ia32.lt.vmcs.c.vm_entry_control.vm_entry_instruction_length"

    // VM Exit Information Fields

#define CPUAPI_IA32_LT_VMCS_C_VM_EXIT_INFO_VM_INSTRUCTION_ERROR                       "arch.ia32.lt.vmcs.c.vm_exit_info.vm_instruction_error"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXIT_INFO_EXIT_REASON                                "arch.ia32.lt.vmcs.c.vm_exit_info.exit_reason"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXIT_INTERRUPTION_INFO                               "arch.ia32.lt.vmcs.c.vm_exit_info.vm_exit_interruption_info"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXIT_INTERRUPTION_ERROR_CODE                         "arch.ia32.lt.vmcs.c.vm_exit_info.vm_exit_interruption_error_code"
#define CPUAPI_IA32_LT_VMCS_C_IDT_VECTORING_INFO                                      "arch.ia32.lt.vmcs.c.vm_exit_info.idt_vectoring_info"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXIT_INFO_IDT_VECTORING_ERROR_CODE                   "arch.ia32.lt.vmcs.c.vm_exit_info.idt_vectoring_error_code"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXIT_INFO_INSTRUCTION_LENGTH                         "arch.ia32.lt.vmcs.c.vm_exit_info.instruction_length"
#define CPUAPI_IA32_LT_VMCS_C_INSTRUCTION_INFORMATION                                 "arch.ia32.lt.vmcs.c.vm_exit_info.instruction_information"
#define CPUAPI_IA32_LT_VMCS_C_VM_EXIT_INFO_EXIT_QUALIFICATION                         "arch.ia32.lt.vmcs.c.vm_exit_info.exit_qualification"
#define CPUAPI_IA32_LT_VMCS_C_I0_ECX                                                  "arch.ia32.lt.vmcs.c.vm_exit_info.io_ecx"
#define CPUAPI_IA32_LT_VMCS_C_IO_ESI                                                  "arch.ia32.lt.vmcs.c.vm_exit_info.io_esi"
#define CPUAPI_IA32_LT_VMCS_C_IO_EDI                                                  "arch.ia32.lt.vmcs.c.vm_exit_info.io_edi"
#define CPUAPI_IA32_LT_VMCS_C_IO_EIP                                                  "arch.ia32.lt.vmcs.c.vm_exit_info.io_eip"
#define CPUAPI_IA32_LT_VMCS_C_IO_INSTRUCTION_INITIAL_ADDRESS                          "arch.ia32.lt.vmcs.c.vm_exit_info.io_instruction_initial_address"

    // VM Guest State Area Fields

#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_CR0                                         "arch.ia32.lt.vmcs.c.guest_state.cr0"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_CR3                                         "arch.ia32.lt.vmcs.c.guest_state.cr3"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_CR4                                         "arch.ia32.lt.vmcs.c.guest_state.cr4"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_DR7                                         "arch.ia32.lt.vmcs.c.guest_state.dr7"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_ES_SELECTOR                                 "arch.ia32.lt.vmcs.c.guest_state.es.selector"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_ES_BASE                                     "arch.ia32.lt.vmcs.c.guest_state.es.base"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_ES_LIMIT                                    "arch.ia32.lt.vmcs.c.guest_state.es.limit"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_ES_AR                                       "arch.ia32.lt.vmcs.c.guest_state.es.ar"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_CS_SELECTOR                                 "arch.ia32.lt.vmcs.c.guest_state.cs.selector"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_CS_BASE                                     "arch.ia32.lt.vmcs.c.guest_state.cs.base"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_CS_LIMIT                                    "arch.ia32.lt.vmcs.c.guest_state.cs.limit"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_CS_AR                                       "arch.ia32.lt.vmcs.c.guest_state.cs.ar"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_SS_SELECTOR                                 "arch.ia32.lt.vmcs.c.guest_state.ss.selector"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_SS_BASE                                     "arch.ia32.lt.vmcs.c.guest_state.ss.base"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_SS_LIMIT                                    "arch.ia32.lt.vmcs.c.guest_state.ss.limit"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_SS_AR                                       "arch.ia32.lt.vmcs.c.guest_state.ss.ar"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_DS_SELECTOR                                 "arch.ia32.lt.vmcs.c.guest_state.ds.selector"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_DS_BASE                                     "arch.ia32.lt.vmcs.c.guest_state.ds.base"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_DS_LIMIT                                    "arch.ia32.lt.vmcs.c.guest_state.ds.limit"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_DS_AR                                       "arch.ia32.lt.vmcs.c.guest_state.ds.ar"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_FS_SELECTOR                                 "arch.ia32.lt.vmcs.c.guest_state.fs.selector"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_FS_BASE                                     "arch.ia32.lt.vmcs.c.guest_state.fs.base"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_FS_LIMIT                                    "arch.ia32.lt.vmcs.c.guest_state.fs.limit"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_FS_AR                                       "arch.ia32.lt.vmcs.c.guest_state.fs.ar"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_GS_SELECTOR                                 "arch.ia32.lt.vmcs.c.guest_state.gs.selector"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_GS_BASE                                     "arch.ia32.lt.vmcs.c.guest_state.gs.base"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_GS_LIMIT                                    "arch.ia32.lt.vmcs.c.guest_state.gs.limit"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_GS_AR                                       "arch.ia32.lt.vmcs.c.guest_state.gs.ar"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_LDTR_SELECTOR                               "arch.ia32.lt.vmcs.c.guest_state.ldtr.selector"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_LDTR_BASE                                   "arch.ia32.lt.vmcs.c.guest_state.ldtr.base"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_LDTR_LIMIT                                  "arch.ia32.lt.vmcs.c.guest_state.ldtr.limit"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_LDTR_AR                                     "arch.ia32.lt.vmcs.c.guest_state.ldtr.ar"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_TR_SELECTOR                                 "arch.ia32.lt.vmcs.c.guest_state.tr.selector"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_TR_BASE                                     "arch.ia32.lt.vmcs.c.guest_state.tr.base"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_TR_LIMIT                                    "arch.ia32.lt.vmcs.c.guest_state.tr.limit"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_TR_AR                                       "arch.ia32.lt.vmcs.c.guest_state.tr.ar"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_GDTR_BASE                                   "arch.ia32.lt.vmcs.c.guest_state.gdtr.base"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_GDTR_LIMIT                                  "arch.ia32.lt.vmcs.c.guest_state.gdtr.limit"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_IDTR_BASE                                   "arch.ia32.lt.vmcs.c.guest_state.idtr.base"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_IDTR_LIMIT                                  "arch.ia32.lt.vmcs.c.guest_state.idtr.limit"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_RSP                                         "arch.ia32.lt.vmcs.c.guest_state.rsp"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_RIP                                         "arch.ia32.lt.vmcs.c.guest_state.rip"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_RFLAGS                                      "arch.ia32.lt.vmcs.c.guest_state.rflags"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_IA32_SYSENTER_ESP                           "arch.ia32.lt.vmcs.c.guest_state.ia32_sysenter_esp"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_IA32_SYSENTER_EIP                           "arch.ia32.lt.vmcs.c.guest_state.ia32_sysenter_eip"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_IA32_DEBUGCTL_MSR                           "arch.ia32.lt.vmcs.c.guest_state.ia32_debugctl_msr"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_SMBASE                                      "arch.ia32.lt.vmcs.c.guest_state.smbase"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_IA32_SYSENTER_CS                            "arch.ia32.lt.vmcs.c.guest_state.ia32_sysenter_cs"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_PENDING_DEBUG_EXCEPTION                     "arch.ia32.lt.vmcs.c.guest_state.pending_debug_exception"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_INTERRUPTIBILITY                            "arch.ia32.lt.vmcs.c.guest_state.interruptibility"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_SLEEP_STATE                                 "arch.ia32.lt.vmcs.c.guest_state.sleep_state"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_GUEST_WORKING_VMCS_POINTER_ALL              "arch.ia32.lt.vmcs.c.guest_state.guest_working_vmcs_pointer_all"
#define CPUAPI_IA32_LT_VMCS_C_GUEST_STATE_GUEST_WORKING_VMCS_POINTER_HIGH             "arch.ia32.lt.vmcs.c.guest_state.guest_working_vmcs_pointer_high"

    // VM Monitor State Area Fields

#define CPUAPI_IA32_LT_VMCS_C_MONITOR_STATE_CR0                                       "arch.ia32.lt.vmcs.c.monitor_state.cr0"
#define CPUAPI_IA32_LT_VMCS_C_MONITOR_STATE_CR3                                       "arch.ia32.lt.vmcs.c.monitor_state.cr3"
#define CPUAPI_IA32_LT_VMCS_C_MONITOR_STATE_CR4                                       "arch.ia32.lt.vmcs.c.monitor_state.cr4"
#define CPUAPI_IA32_LT_VMCS_C_MONITOR_STATE_ES_SELECTOR                               "arch.ia32.lt.vmcs.c.monitor_state.es.selector"
#define CPUAPI_IA32_LT_VMCS_C_MONITOR_STATE_CS_SELECTOR                               "arch.ia32.lt.vmcs.c.monitor_state.cs.selector"
#define CPUAPI_IA32_LT_VMCS_C_MONITOR_STATE_SS_SELECTOR                               "arch.ia32.lt.vmcs.c.monitor_state.ss.selector"
#define CPUAPI_IA32_LT_VMCS_C_MONITOR_STATE_DS_SELECTOR                               "arch.ia32.lt.vmcs.c.monitor_state.ds.selector"
#define CPUAPI_IA32_LT_VMCS_C_MONITOR_STATE_FS_SELECTOR                               "arch.ia32.lt.vmcs.c.monitor_state.fs.selector"
#define CPUAPI_IA32_LT_VMCS_C_MONITOR_STATE_GS_SELECTOR                               "arch.ia32.lt.vmcs.c.monitor_state.gs.selector"
#define CPUAPI_IA32_LT_VMCS_C_MONITOR_STATE_FS_BASE                                   "arch.ia32.lt.vmcs.c.monitor_state.fs.base"
#define CPUAPI_IA32_LT_VMCS_C_MONITOR_STATE_GS_BASE                                   "arch.ia32.lt.vmcs.c.monitor_state.gs.base"
#define CPUAPI_IA32_LT_VMCS_C_MONITOR_STATE_TR_SELECTOR                               "arch.ia32.lt.vmcs.c.monitor_state.tr.selector"
#define CPUAPI_IA32_LT_VMCS_C_MONITOR_STATE_TR_BASE                                   "arch.ia32.lt.vmcs.c.monitor_state.tr.base"
#define CPUAPI_IA32_LT_VMCS_C_MONITOR_STATE_GDTR_BASE                                 "arch.ia32.lt.vmcs.c.monitor_state.gdtr.base"
#define CPUAPI_IA32_LT_VMCS_C_MONITOR_STATE_IDTR_BASE                                 "arch.ia32.lt.vmcs.c.monitor_state.idtr.base"
#define CPUAPI_IA32_LT_VMCS_C_MONITOR_STATE_RSP                                       "arch.ia32.lt.vmcs.c.monitor_state.rsp"
#define CPUAPI_IA32_LT_VMCS_C_MONITOR_STATE_RIP                                       "arch.ia32.lt.vmcs.c.monitor_state.rip"
#define CPUAPI_IA32_LT_VMCS_C_MONITOR_STATE_IA32_SYSENTER_ESP                         "arch.ia32.lt.vmcs.c.monitor_state.ia32_sysenter_esp"
#define CPUAPI_IA32_LT_VMCS_C_MONITOR_STATE_IA32_SYSENTER_EIP                         "arch.ia32.lt.vmcs.c.monitor_state.ia32_sysenter_eip"
#define CPUAPI_IA32_LT_VMCS_C_MONITOR_STATE_IA32_SYSENTER_CS                          "arch.ia32.lt.vmcs.c.monitor_state.ia32_sysenter_cs"





    //  VMCS fields
#define CPUAPI_IA32_LT_VMCS_W_POINTER                                                 "arch.ia32.lt.vmcs.w.pointer"

#define CPUAPI_IA32_LT_VMCS_W_REVISION                                                "arch.ia32.lt.vmcs.w.revision"
#define CPUAPI_IA32_LT_VMCS_W_VMX_ABORT_INDICATOR                                     "arch.ia32.lt.vmcs.w.vmx_abort_indicator"
#define CPUAPI_IA32_LT_VMCS_W_PARENT_POINTER                                          "arch.ia32.lt.vmcs.w.parent_pointer"
#define CPUAPI_IA32_LT_VMCS_W_LAUNCH_STATE                                            "arch.ia32.lt.vmcs.w.launch_state"

    // VM Execution Control Fields

#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_PIN_BASED_CONTROLS                         "arch.ia32.lt.vmcs.w.vm_execution_control.pin_based_controls"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_PROCESSOR_BASED_CONTROLS                   "arch.ia32.lt.vmcs.w.vm_execution_control.processor_based_controls"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_EXCEPTION_BITMAP                   "arch.ia32.lt.vmcs.w.vm_execution_control.exception_bitmap"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_CR3_TARGET_COUNT                   "arch.ia32.lt.vmcs.w.vm_execution_control.cr3.target_count"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_CR0_GUEST_HOST_MASK                "arch.ia32.lt.vmcs.w.vm_execution_control.cr0.guest_host_mask"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_CR3_GUEST_HOST_MASK                "arch.ia32.lt.vmcs.w.vm_execution_control.cr3.guest_host_mask"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_CR4_GUEST_HOST_MASK                "arch.ia32.lt.vmcs.w.vm_execution_control.cr4.guest_host_mask"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_CR8_GUEST_HOST_MASK                "arch.ia32.lt.vmcs.w.vm_execution_control.cr8.guest_host_mask"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_CR0_READ_SHADOW                    "arch.ia32.lt.vmcs.w.vm_execution_control.cr0.read_shadow"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_CR3_READ_SHADOW                    "arch.ia32.lt.vmcs.w.vm_execution_control.cr3.read_shadow"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_CR4_READ_SHADOW                    "arch.ia32.lt.vmcs.w.vm_execution_control.cr4.read_shadow"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_CR8_READ_SHADOW                    "arch.ia32.lt.vmcs.w.vm_execution_control.cr8.read_shadow"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_CR3_TARGET_VALUE_FIRST             "arch.ia32.lt.vmcs.w.vm_execution_control.cr3.target_value.first"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_CR3_TARGET_VALUE_LAST              "arch.ia32.lt.vmcs.w.vm_execution_control.cr3.target_value.last"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_CR3_TARGET_VALUE_0                 "arch.ia32.lt.vmcs.w.vm_execution_control.cr3.target_value.0"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_CR3_TARGET_VALUE_1                 "arch.ia32.lt.vmcs.w.vm_execution_control.cr3.target_value.1"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_CR3_TARGET_VALUE_2                 "arch.ia32.lt.vmcs.w.vm_execution_control.cr3.target_value.2"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_CR3_TARGET_VALUE_3                 "arch.ia32.lt.vmcs.w.vm_execution_control.cr3.target_value.3"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_PAGE_FAULT_ERROR_CODE_MASK         "arch.ia32.lt.vmcs.w.vm_execution_control.page_fault.error_code_mask"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_PAGE_FAULT_ERROR_CODE_MATCH        "arch.ia32.lt.vmcs.w.vm_execution_control.page_fault.error_code_match"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_IO_BITMAP_A_ALL                    "arch.ia32.lt.vmcs.w.vm_execution_control.io_bitmap.a_all"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_IO_BITMAP_A_HIGH                   "arch.ia32.lt.vmcs.w.vm_execution_control.io_bitmap.a_high"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_IO_BITMAP_B_ALL                    "arch.ia32.lt.vmcs.w.vm_execution_control.io_bitmap.b_all"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_IO_BITMAP_B_HIGH                   "arch.ia32.lt.vmcs.w.vm_execution_control.io_bitmap.b_high"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_TSC_COMPARATOR_ALL                 "arch.ia32.lt.vmcs.w.vm_execution_control.tsc.comparator_all"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_TSC_COMPARATOR_HIGH                "arch.ia32.lt.vmcs.w.vm_execution_control.tsc.comparator_high"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_TSC_OFFSET_ALL                     "arch.ia32.lt.vmcs.w.vm_execution_control.tsc.offset_all"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_TSC_OFFSET_HIGH                    "arch.ia32.lt.vmcs.w.vm_execution_control.tsc.offset_high"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_OSV_CONTROLLING_VMCS_POINTER_ALL   "arch.ia32.lt.vmcs.w.vm_execution_control.osv_controlling_vmcs_pointer_all"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_OSV_CONTROLLING_VMCS_POINTER_HIGH  "arch.ia32.lt.vmcs.w.vm_execution_control.osv_controlling_vmcs_pointer_high"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_MSR_BITMAPS_ADDRESS_ALL            "arch.ia32.lt.vmcs.w.vm_execution_control.msr_bitmaps_address_all"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXECUTION_CONTROL_MSR_BITMAPS_ADDRESS_HIGH           "arch.ia32.lt.vmcs.w.vm_execution_control.msr_bitmaps_address_high"

    // VM Exit Control Fields

#define CPUAPI_IA32_LT_VMCS_W_VM_EXIT_CONTROL_CONTROLS                                "arch.ia32.lt.vmcs.w.vm_exit_control.controls"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXIT_CONTROL_MSR_STORE_COUNT                         "arch.ia32.lt.vmcs.w.vm_exit_control.msr.store_count"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXIT_CONTROL_MSR_LOAD_COUNT                          "arch.ia32.lt.vmcs.w.vm_exit_control.msr.load_count"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXIT_MSR_STORE_ADDRESS_ALL                           "arch.ia32.lt.vmcs.w.vm_exit_control.msr.store_address_all"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXIT_MSR_STORE_ADDRESS_HIGH                          "arch.ia32.lt.vmcs.w.vm_exit_control.msr.store_address_high"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXIT_MSR_LOAD_ADDRESS_ALL                            "arch.ia32.lt.vmcs.w.vm_exit_control.msr.load_address_all"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXIT_MSR_LOAD_ADDRESS_HIGH                           "arch.ia32.lt.vmcs.w.vm_exit_control.msr.load_address_high"

    // VM Entry Control Fields

#define CPUAPI_IA32_LT_VMCS_W_VM_ENTRY_CONTROL_CONTROLS                               "arch.ia32.lt.vmcs.w.vm_entry_control.controls"
#define CPUAPI_IA32_LT_VMCS_W_VM_ENTRY_CONTROL_INTERRUPTION_INFORMATION               "arch.ia32.lt.vmcs.w.vm_entry_control.interruption_information"
#define CPUAPI_IA32_LT_VMCS_W_VM_ENTRY_CONTROL_EXCEPTION_ERROR_CODE                   "arch.ia32.lt.vmcs.w.vm_entry_control.exception_error_code"
#define CPUAPI_IA32_LT_VMCS_W_VM_ENTRY_CONTROL_MSR_LOAD_COUNT                         "arch.ia32.lt.vmcs.w.vm_entry_control.msr.load_count"
#define CPUAPI_IA32_LT_VMCS_W_VM_ENTRY_MSR_LOAD_ADDRESS_ALL                           "arch.ia32.lt.vmcs.w.vm_entry_control.msr.load_address_all"
#define CPUAPI_IA32_LT_VMCS_W_VM_ENTRY_MSR_LOAD_ADDRESS_HIGH                          "arch.ia32.lt.vmcs.w.vm_entry_control.msr.load_address_high"
#define CPUAPI_IA32_LT_VMCS_W_VM_ENTRY_CONTROL_INSTRUCTION_LENGTH                     "arch.ia32.lt.vmcs.w.vm_entry_control.vm_entry_instruction_length"

    // VM Exit Information Fields

#define CPUAPI_IA32_LT_VMCS_W_VM_EXIT_INFO_VM_INSTRUCTION_ERROR                       "arch.ia32.lt.vmcs.w.vm_exit_info.vm_instruction_error"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXIT_INFO_EXIT_REASON                                "arch.ia32.lt.vmcs.w.vm_exit_info.exit_reason"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXIT_INTERRUPTION_INFO                               "arch.ia32.lt.vmcs.w.vm_exit_info.vm_exit_interruption_info"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXIT_INTERRUPTION_ERROR_CODE                         "arch.ia32.lt.vmcs.w.vm_exit_info.vm_exit_interruption_error_code"
#define CPUAPI_IA32_LT_VMCS_W_IDT_VECTORING_INFO                                      "arch.ia32.lt.vmcs.w.vm_exit_info.idt_vectoring_info"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXIT_INFO_IDT_VECTORING_ERROR_CODE                   "arch.ia32.lt.vmcs.w.vm_exit_info.idt_vectoring_error_code"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXIT_INFO_INSTRUCTION_LENGTH                         "arch.ia32.lt.vmcs.w.vm_exit_info.instruction_length"
#define CPUAPI_IA32_LT_VMCS_W_INSTRUCTION_INFORMATION                                 "arch.ia32.lt.vmcs.w.vm_exit_info.instruction_information"
#define CPUAPI_IA32_LT_VMCS_W_VM_EXIT_INFO_EXIT_QUALIFICATION                         "arch.ia32.lt.vmcs.w.vm_exit_info.exit_qualification"
#define CPUAPI_IA32_LT_VMCS_W_I0_ECX                                                  "arch.ia32.lt.vmcs.w.vm_exit_info.io_ecx"
#define CPUAPI_IA32_LT_VMCS_W_IO_ESI                                                  "arch.ia32.lt.vmcs.w.vm_exit_info.io_esi"
#define CPUAPI_IA32_LT_VMCS_W_IO_EDI                                                  "arch.ia32.lt.vmcs.w.vm_exit_info.io_edi"
#define CPUAPI_IA32_LT_VMCS_W_IO_EIP                                                  "arch.ia32.lt.vmcs.w.vm_exit_info.io_eip"
#define CPUAPI_IA32_LT_VMCS_W_IO_INSTRUCTION_INITIAL_ADDRESS                          "arch.ia32.lt.vmcs.w.vm_exit_info.io_instruction_initial_address"

    // VM Guest State Area Fields

#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_CR0                                         "arch.ia32.lt.vmcs.w.guest_state.cr0"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_CR3                                         "arch.ia32.lt.vmcs.w.guest_state.cr3"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_CR4                                         "arch.ia32.lt.vmcs.w.guest_state.cr4"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_DR7                                         "arch.ia32.lt.vmcs.w.guest_state.dr7"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_ES_SELECTOR                                 "arch.ia32.lt.vmcs.w.guest_state.es.selector"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_ES_BASE                                     "arch.ia32.lt.vmcs.w.guest_state.es.base"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_ES_LIMIT                                    "arch.ia32.lt.vmcs.w.guest_state.es.limit"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_ES_AR                                       "arch.ia32.lt.vmcs.w.guest_state.es.ar"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_CS_SELECTOR                                 "arch.ia32.lt.vmcs.w.guest_state.cs.selector"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_CS_BASE                                     "arch.ia32.lt.vmcs.w.guest_state.cs.base"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_CS_LIMIT                                    "arch.ia32.lt.vmcs.w.guest_state.cs.limit"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_CS_AR                                       "arch.ia32.lt.vmcs.w.guest_state.cs.ar"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_SS_SELECTOR                                 "arch.ia32.lt.vmcs.w.guest_state.ss.selector"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_SS_BASE                                     "arch.ia32.lt.vmcs.w.guest_state.ss.base"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_SS_LIMIT                                    "arch.ia32.lt.vmcs.w.guest_state.ss.limit"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_SS_AR                                       "arch.ia32.lt.vmcs.w.guest_state.ss.ar"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_DS_SELECTOR                                 "arch.ia32.lt.vmcs.w.guest_state.ds.selector"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_DS_BASE                                     "arch.ia32.lt.vmcs.w.guest_state.ds.base"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_DS_LIMIT                                    "arch.ia32.lt.vmcs.w.guest_state.ds.limit"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_DS_AR                                       "arch.ia32.lt.vmcs.w.guest_state.ds.ar"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_FS_SELECTOR                                 "arch.ia32.lt.vmcs.w.guest_state.fs.selector"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_FS_BASE                                     "arch.ia32.lt.vmcs.w.guest_state.fs.base"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_FS_LIMIT                                    "arch.ia32.lt.vmcs.w.guest_state.fs.limit"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_FS_AR                                       "arch.ia32.lt.vmcs.w.guest_state.fs.ar"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_GS_SELECTOR                                 "arch.ia32.lt.vmcs.w.guest_state.gs.selector"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_GS_BASE                                     "arch.ia32.lt.vmcs.w.guest_state.gs.base"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_GS_LIMIT                                    "arch.ia32.lt.vmcs.w.guest_state.gs.limit"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_GS_AR                                       "arch.ia32.lt.vmcs.w.guest_state.gs.ar"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_LDTR_SELECTOR                               "arch.ia32.lt.vmcs.w.guest_state.ldtr.selector"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_LDTR_BASE                                   "arch.ia32.lt.vmcs.w.guest_state.ldtr.base"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_LDTR_LIMIT                                  "arch.ia32.lt.vmcs.w.guest_state.ldtr.limit"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_LDTR_AR                                     "arch.ia32.lt.vmcs.w.guest_state.ldtr.ar"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_TR_SELECTOR                                 "arch.ia32.lt.vmcs.w.guest_state.tr.selector"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_TR_BASE                                     "arch.ia32.lt.vmcs.w.guest_state.tr.base"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_TR_LIMIT                                    "arch.ia32.lt.vmcs.w.guest_state.tr.limit"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_TR_AR                                       "arch.ia32.lt.vmcs.w.guest_state.tr.ar"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_GDTR_BASE                                   "arch.ia32.lt.vmcs.w.guest_state.gdtr.base"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_GDTR_LIMIT                                  "arch.ia32.lt.vmcs.w.guest_state.gdtr.limit"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_IDTR_BASE                                   "arch.ia32.lt.vmcs.w.guest_state.idtr.base"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_IDTR_LIMIT                                  "arch.ia32.lt.vmcs.w.guest_state.idtr.limit"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_RSP                                         "arch.ia32.lt.vmcs.w.guest_state.rsp"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_RIP                                         "arch.ia32.lt.vmcs.w.guest_state.rip"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_RFLAGS                                      "arch.ia32.lt.vmcs.w.guest_state.rflags"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_IA32_SYSENTER_ESP                           "arch.ia32.lt.vmcs.w.guest_state.ia32_sysenter_esp"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_IA32_SYSENTER_EIP                           "arch.ia32.lt.vmcs.w.guest_state.ia32_sysenter_eip"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_IA32_DEBUGCTL_MSR                           "arch.ia32.lt.vmcs.w.guest_state.ia32_debugctl_msr"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_SMBASE                                      "arch.ia32.lt.vmcs.w.guest_state.smbase"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_IA32_SYSENTER_CS                            "arch.ia32.lt.vmcs.w.guest_state.ia32_sysenter_cs"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_PENDING_DEBUG_EXCEPTION                     "arch.ia32.lt.vmcs.w.guest_state.pending_debug_exception"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_INTERRUPTIBILITY                            "arch.ia32.lt.vmcs.w.guest_state.interruptibility"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_SLEEP_STATE                                 "arch.ia32.lt.vmcs.w.guest_state.sleep_state"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_GUEST_WORKING_VMCS_POINTER_ALL              "arch.ia32.lt.vmcs.w.guest_state.guest_working_vmcs_pointer_all"
#define CPUAPI_IA32_LT_VMCS_W_GUEST_STATE_GUEST_WORKING_VMCS_POINTER_HIGH             "arch.ia32.lt.vmcs.w.guest_state.guest_working_vmcs_pointer_high"

    // VM Monitor State Area Fields

#define CPUAPI_IA32_LT_VMCS_W_MONITOR_STATE_CR0                                       "arch.ia32.lt.vmcs.w.monitor_state.cr0"
#define CPUAPI_IA32_LT_VMCS_W_MONITOR_STATE_CR3                                       "arch.ia32.lt.vmcs.w.monitor_state.cr3"
#define CPUAPI_IA32_LT_VMCS_W_MONITOR_STATE_CR4                                       "arch.ia32.lt.vmcs.w.monitor_state.cr4"
#define CPUAPI_IA32_LT_VMCS_W_MONITOR_STATE_ES_SELECTOR                               "arch.ia32.lt.vmcs.w.monitor_state.es.selector"
#define CPUAPI_IA32_LT_VMCS_W_MONITOR_STATE_CS_SELECTOR                               "arch.ia32.lt.vmcs.w.monitor_state.cs.selector"
#define CPUAPI_IA32_LT_VMCS_W_MONITOR_STATE_SS_SELECTOR                               "arch.ia32.lt.vmcs.w.monitor_state.ss.selector"
#define CPUAPI_IA32_LT_VMCS_W_MONITOR_STATE_DS_SELECTOR                               "arch.ia32.lt.vmcs.w.monitor_state.ds.selector"
#define CPUAPI_IA32_LT_VMCS_W_MONITOR_STATE_FS_SELECTOR                               "arch.ia32.lt.vmcs.w.monitor_state.fs.selector"
#define CPUAPI_IA32_LT_VMCS_W_MONITOR_STATE_GS_SELECTOR                               "arch.ia32.lt.vmcs.w.monitor_state.gs.selector"
#define CPUAPI_IA32_LT_VMCS_W_MONITOR_STATE_FS_BASE                                   "arch.ia32.lt.vmcs.w.monitor_state.fs.base"
#define CPUAPI_IA32_LT_VMCS_W_MONITOR_STATE_GS_BASE                                   "arch.ia32.lt.vmcs.w.monitor_state.gs.base"
#define CPUAPI_IA32_LT_VMCS_W_MONITOR_STATE_TR_SELECTOR                               "arch.ia32.lt.vmcs.w.monitor_state.tr.selector"
#define CPUAPI_IA32_LT_VMCS_W_MONITOR_STATE_TR_BASE                                   "arch.ia32.lt.vmcs.w.monitor_state.tr.base"
#define CPUAPI_IA32_LT_VMCS_W_MONITOR_STATE_GDTR_BASE                                 "arch.ia32.lt.vmcs.w.monitor_state.gdtr.base"
#define CPUAPI_IA32_LT_VMCS_W_MONITOR_STATE_IDTR_BASE                                 "arch.ia32.lt.vmcs.w.monitor_state.idtr.base"
#define CPUAPI_IA32_LT_VMCS_W_MONITOR_STATE_RSP                                       "arch.ia32.lt.vmcs.w.monitor_state.rsp"
#define CPUAPI_IA32_LT_VMCS_W_MONITOR_STATE_RIP                                       "arch.ia32.lt.vmcs.w.monitor_state.rip"
#define CPUAPI_IA32_LT_VMCS_W_MONITOR_STATE_IA32_SYSENTER_ESP                         "arch.ia32.lt.vmcs.w.monitor_state.ia32_sysenter_esp"
#define CPUAPI_IA32_LT_VMCS_W_MONITOR_STATE_IA32_SYSENTER_EIP                         "arch.ia32.lt.vmcs.w.monitor_state.ia32_sysenter_eip"
#define CPUAPI_IA32_LT_VMCS_W_MONITOR_STATE_IA32_SYSENTER_CS                          "arch.ia32.lt.vmcs.w.monitor_state.ia32_sysenter_cs"
#endif

    // Architectural events

#define CPUAPI_IA32_EVENT_EXT_INTERRUPT                 "event.arch.ia32.external.interrupt"
#define CPUAPI_IA32_EVENT_HALT                          "event.arch.ia32.halt"
#define CPUAPI_IA32_EVENT_RESET                         "event.arch.ia32.reset"
#define CPUAPI_IA32_EVENT_SHUTDOWN                      "event.arch.ia32.shutdown"
#define CPUAPI_IA32_EVENT_READ_MSR                      "event.arch.ia32.read.msr"
#define CPUAPI_IA32_EVENT_WRITE_MSR                     "event.arch.ia32.write.msr"
#define CPUAPI_IA32_EVENT_EXCEPT_DIVIDE_BY_ZERO         "event.arch.ia32.exception.divide.by.zero"
#define CPUAPI_IA32_EVENT_EXCEPT_NMI                    "event.arch.ia32.exception.nmi"
#define CPUAPI_IA32_EVENT_EXCEPT_DEBUG                  "event.arch.ia32.exception.debug"
#define CPUAPI_IA32_EVENT_EXCEPT_BREAKPOINT             "event.arch.ia32.exception.breakpoint"
#define CPUAPI_IA32_EVENT_EXCEPT_OVERFLOW               "event.arch.ia32.exception.overflow"
#define CPUAPI_IA32_EVENT_EXCEPT_BOUND                  "event.arch.ia32.exception.bound"
#define CPUAPI_IA32_EVENT_EXCEPT_INVALID_OPCODE         "event.arch.ia32.exception.invalid.opcode"
#define CPUAPI_IA32_EVENT_EXCEPT_DEVICE_NOT_AVAILABLE   "event.arch.ia32.exception.device.not.available"
#define CPUAPI_IA32_EVENT_EXCEPT_DOUBLE_FAULT           "event.arch.ia32.exception.double.fault"
#define CPUAPI_IA32_EVENT_EXCEPT_INVALID_TSS            "event.arch.ia32.exception.invalid.tss"
#define CPUAPI_IA32_EVENT_EXCEPT_SEGMENT_NOT_PRESET     "event.arch.ia32.exception.segment.not.preset"
#define CPUAPI_IA32_EVENT_EXCEPT_STACK_FAULT            "event.arch.ia32.exception.invalid.opcode"
#define CPUAPI_IA32_EVENT_EXCEPT_GENERAL_PROTECTION     "event.arch.ia32.exception.general.protection"
#define CPUAPI_IA32_EVENT_EXCEPT_PAGE_FAULT             "event.arch.ia32.exception.page.fault"
#define CPUAPI_IA32_EVENT_EXCEPT_FLOAT_ERROR            "event.arch.ia32.exception.float.error"
#define CPUAPI_IA32_EVENT_EXCEPT_ALIGNMENT_CHECK        "event.arch.ia32.exception.alignment.check"
#define CPUAPI_IA32_EVENT_EXCEPT_MACHINE_CHECK          "event.arch.ia32.exception.machine.check"
#define CPUAPI_IA32_EVENT_EXCEPT_SIMD_FLOAT_ERROR       "event.arch.ia32.exception.simd.float.error"

#ifndef MEROM_APE_NI_OFF
#define CPUAPI_IA32_TCE_STATE                                                         "arch.ia32.tce.state"
#define CPUAPI_IA32_TCE_PRIMARY_AGENT                                                 "arch.ia32.tce.primary_agent"
#define CPUAPI_IA32_TCE_TASK_CR3                                                      "arch.ia32.tce.task_cr3"
#define CPUAPI_IA32_TCE_SXFR_ID                                                       "arch.ia32.tce.sxfr_id"

#define CPUAPI_IA32_APE_YIELD_BLOCK_BIT                                               "arch.ia32.ape.yield_block_bit"
#define CPUAPI_IA32_APE_SAVED_EIP                                                     "arch.ia32.ape.saved_eip"
#define CPUAPI_IA32_APE_RING3_CHANNEL0_EAX                                            "arch.ia32.ape.ring3.channel0.eax"
#define CPUAPI_IA32_APE_RING3_CHANNEL0_EBX                                            "arch.ia32.ape.ring3.channel0.ebx"
#define CPUAPI_IA32_APE_RING3_CHANNEL0_ECX                                            "arch.ia32.ape.ring3.channel0.ecx"
#define CPUAPI_IA32_APE_RING3_CHANNEL0_EDX                                            "arch.ia32.ape.ring3.channel0.edx"
#define CPUAPI_IA32_APE_RING3_CHANNEL1_EAX                                            "arch.ia32.ape.ring3.channel1.eax"
#define CPUAPI_IA32_APE_RING3_CHANNEL1_EBX                                            "arch.ia32.ape.ring3.channel1.ebx"
#define CPUAPI_IA32_APE_RING3_CHANNEL1_ECX                                            "arch.ia32.ape.ring3.channel1.ecx"
#define CPUAPI_IA32_APE_RING3_CHANNEL1_EDX                                            "arch.ia32.ape.ring3.channel1.edx"
#endif /* MEROM_APE_NI_OFF */

#define CPUAPI_IA32_RAX_SIZE 8
#define CPUAPI_IA32_RBX_SIZE 8
#define CPUAPI_IA32_RCX_SIZE 8
#define CPUAPI_IA32_RDX_SIZE 8
#define CPUAPI_IA32_RSI_SIZE 8
#define CPUAPI_IA32_RDI_SIZE 8
#define CPUAPI_IA32_RBP_SIZE 8
#define CPUAPI_IA32_RSP_SIZE 8
#define CPUAPI_IA32_R8_SIZE  8
#define CPUAPI_IA32_R9_SIZE  8
#define CPUAPI_IA32_R10_SIZE 8
#define CPUAPI_IA32_R11_SIZE 8
#define CPUAPI_IA32_R12_SIZE 8
#define CPUAPI_IA32_R13_SIZE 8
#define CPUAPI_IA32_R14_SIZE 8
#define CPUAPI_IA32_R15_SIZE 8


#define CPUAPI_IA32_EAX_SIZE 4
#define CPUAPI_IA32_EBX_SIZE 4
#define CPUAPI_IA32_ECX_SIZE 4
#define CPUAPI_IA32_EDX_SIZE 4
#define CPUAPI_IA32_ESI_SIZE 4
#define CPUAPI_IA32_EDI_SIZE 4
#define CPUAPI_IA32_EBP_SIZE 4
#define CPUAPI_IA32_ESP_SIZE 4
#define CPUAPI_IA32_R8D_SIZE 4
#define CPUAPI_IA32_R9D_SIZE 4
#define CPUAPI_IA32_R10D_SIZE 4
#define CPUAPI_IA32_R11D_SIZE 4
#define CPUAPI_IA32_R12D_SIZE 4
#define CPUAPI_IA32_R13D_SIZE 4
#define CPUAPI_IA32_R14D_SIZE 4
#define CPUAPI_IA32_R15D_SIZE 4


#define CPUAPI_IA32_AX_SIZE  2
#define CPUAPI_IA32_BX_SIZE  2
#define CPUAPI_IA32_CX_SIZE  2
#define CPUAPI_IA32_DX_SIZE  2
#define CPUAPI_IA32_SI_SIZE  2
#define CPUAPI_IA32_DI_SIZE  2
#define CPUAPI_IA32_BP_SIZE  2
#define CPUAPI_IA32_SP_SIZE  2
#define CPUAPI_IA32_R8W_SIZE   2
#define CPUAPI_IA32_R9W_SIZE   2
#define CPUAPI_IA32_R10W_SIZE  2
#define CPUAPI_IA32_R11W_SIZE  2
#define CPUAPI_IA32_R12W_SIZE  2
#define CPUAPI_IA32_R13W_SIZE  2
#define CPUAPI_IA32_R14W_SIZE  2
#define CPUAPI_IA32_R15W_SIZE  2

#define CPUAPI_IA32_AL_SIZE  1
#define CPUAPI_IA32_BL_SIZE  1
#define CPUAPI_IA32_CL_SIZE  1
#define CPUAPI_IA32_DL_SIZE  1
#define CPUAPI_IA32_AH_SIZE  1
#define CPUAPI_IA32_BH_SIZE  1
#define CPUAPI_IA32_CH_SIZE  1
#define CPUAPI_IA32_DH_SIZE  1
#define CPUAPI_IA32_SIL_SIZE   1
#define CPUAPI_IA32_DIL_SIZE   1
#define CPUAPI_IA32_BPL_SIZE   1
#define CPUAPI_IA32_SPL_SIZE   1
#define CPUAPI_IA32_R8B_SIZE   1
#define CPUAPI_IA32_R9B_SIZE   1
#define CPUAPI_IA32_R10B_SIZE  1
#define CPUAPI_IA32_R11B_SIZE  1
#define CPUAPI_IA32_R12B_SIZE  1
#define CPUAPI_IA32_R13B_SIZE  1
#define CPUAPI_IA32_R14B_SIZE  1
#define CPUAPI_IA32_R15B_SIZE  1


#define CPUAPI_IA32_CS_SEL_SIZE 2
#define CPUAPI_IA32_SS_SEL_SIZE 2
#define CPUAPI_IA32_DS_SEL_SIZE 2
#define CPUAPI_IA32_ES_SEL_SIZE 2
#define CPUAPI_IA32_FS_SEL_SIZE 2
#define CPUAPI_IA32_GS_SEL_SIZE 2

#define CPUAPI_IA32_CS_DESC_SIZE sizeof(cpuapi_descriptor_t)
#define CPUAPI_IA32_SS_DESC_SIZE sizeof(cpuapi_descriptor_t)
#define CPUAPI_IA32_DS_DESC_SIZE sizeof(cpuapi_descriptor_t)
#define CPUAPI_IA32_ES_DESC_SIZE sizeof(cpuapi_descriptor_t)
#define CPUAPI_IA32_FS_DESC_SIZE sizeof(cpuapi_descriptor_t)
#define CPUAPI_IA32_GS_DESC_SIZE sizeof(cpuapi_descriptor_t)

#define CPUAPI_IA32_RIP_SIZE 8
#define CPUAPI_IA32_EIP_SIZE 4
#define CPUAPI_IA32_EFLAGS_SIZE 4

#define CPUAPI_IA32_CR0_SIZE 8
#define CPUAPI_IA32_CR1_SIZE 8
#define CPUAPI_IA32_CR2_SIZE 8
#define CPUAPI_IA32_CR3_SIZE 8
#define CPUAPI_IA32_CR4_SIZE 8
#define CPUAPI_IA32_CR8_SIZE 8
#define CPUAPI_IA32_DR0_SIZE 8
#define CPUAPI_IA32_DR1_SIZE 8
#define CPUAPI_IA32_DR2_SIZE 8
#define CPUAPI_IA32_DR3_SIZE 8
#define CPUAPI_IA32_DR4_SIZE 8
#define CPUAPI_IA32_DR5_SIZE 8
#define CPUAPI_IA32_DR6_SIZE 8
#define CPUAPI_IA32_DR7_SIZE 8

#define CPUAPI_IA32_GDTR_BASE_SIZE 8
#define CPUAPI_IA32_GDTR_LIMIT_SIZE 2

#define CPUAPI_IA32_IDTR_BASE_SIZE 8
#define CPUAPI_IA32_IDTR_LIMIT_SIZE 2

#define CPUAPI_IA32_LDTR_SEL_SIZE 2
#define CPUAPI_IA32_LDTR_DESC_SIZE sizeof(cpuapi_descriptor_t)

#define CPUAPI_IA32_TR_SEL_SIZE 2
#define CPUAPI_IA32_TR_DESC_SIZE sizeof(cpuapi_descriptor_t)


#define CPUAPI_IA32_FR0_SIZE 10
#define CPUAPI_IA32_FR1_SIZE 10
#define CPUAPI_IA32_FR2_SIZE 10
#define CPUAPI_IA32_FR3_SIZE 10
#define CPUAPI_IA32_FR4_SIZE 10
#define CPUAPI_IA32_FR5_SIZE 10
#define CPUAPI_IA32_FR6_SIZE 10
#define CPUAPI_IA32_FR7_SIZE 10

#define CPUAPI_IA32_MXCSR_SIZE 4

#define CPUAPI_IA32_XMM0_SIZE 16
#define CPUAPI_IA32_XMM1_SIZE 16
#define CPUAPI_IA32_XMM2_SIZE 16
#define CPUAPI_IA32_XMM3_SIZE 16
#define CPUAPI_IA32_XMM4_SIZE 16
#define CPUAPI_IA32_XMM5_SIZE 16
#define CPUAPI_IA32_XMM6_SIZE 16
#define CPUAPI_IA32_XMM7_SIZE 16
#define CPUAPI_IA32_XMM8_SIZE 16
#define CPUAPI_IA32_XMM9_SIZE 16
#define CPUAPI_IA32_XMM10_SIZE 16
#define CPUAPI_IA32_XMM11_SIZE 16
#define CPUAPI_IA32_XMM12_SIZE 16
#define CPUAPI_IA32_XMM13_SIZE 16
#define CPUAPI_IA32_XMM14_SIZE 16
#define CPUAPI_IA32_XMM15_SIZE 16



#define CPUAPI_IA32_MSR_SIZE         8
#define CPUAPI_IA32_MSR_EFER_SIZE    8
#define CPUAPI_IA32_MSR_FS_BASE_SIZE 8
#define CPUAPI_IA32_MSR_GS_BASE_SIZE 8
#define CPUAPI_IA32_MSR_KERNEL_GS_BASE_SIZE 8
#define CPUAPI_IA32_MSR_STAR_SIZE    8
#define CPUAPI_IA32_MSR_LSTAR_SIZE   8
#define CPUAPI_IA32_MSR_CSTAR_SIZE   8
#define CPUAPI_IA32_MSR_FMASK_SIZE   8


#define CPUAPI_IA32_CPUID0_SIZE 16
#define CPUAPI_IA32_CPUID1_SIZE 16
#define CPUAPI_IA32_CPUID2_SIZE 16
#define CPUAPI_IA32_CPUID3_SIZE 16
#define CPUAPI_IA32_CPUID4_SIZE 16
#define CPUAPI_IA32_CPUID5_SIZE 16

#define CPUAPI_IA32_CPUID_EXT0_SIZE 16
#define CPUAPI_IA32_CPUID_EXT1_SIZE 16
#define CPUAPI_IA32_CPUID_EXT2_SIZE 16
#define CPUAPI_IA32_CPUID_EXT3_SIZE 16
#define CPUAPI_IA32_CPUID_EXT4_SIZE 16
#define CPUAPI_IA32_CPUID_EXT5_SIZE 16
#define CPUAPI_IA32_CPUID_EXT6_SIZE 16
#define CPUAPI_IA32_CPUID_EXT7_SIZE 16
#define CPUAPI_IA32_CPUID_EXT8_SIZE 16

//FPU Control Register - 16 bits
#define CPUAPI_IA32_FCONTROL_SIZE 2
//FPU Status Register - 16 bits
#define CPUAPI_IA32_FSTATUS_SIZE 2
//FPU Tag Register - 16 bits
#define CPUAPI_IA32_FTAG_SIZE 2
//FPU Opcode Register - 16 bits
#define CPUAPI_IA32_FOPCODE_SIZE 2
//FPU Instruction pointer - 80 bits - 10 bytes
#define CPUAPI_IA32_FINST_SEL_SIZE 2
#define CPUAPI_IA32_FINST_OFFSET_SIZE 8
//FPU Data (Operand) Pointer Register - 80 bits - 10 bytes
#define CPUAPI_IA32_FDATA_SEL_SIZE 2
#define CPUAPI_IA32_FDATA_OFFSET_SIZE 8



//Local APIC registers names
//address FEE0 0000H - FEE0 03F0H
#define CPUAPI_IA32_LOCAL_APIC_SPACE_SIZE  0x3F0
//address  FEE0 0020H
#define CPUAPI_IA32_LOCAL_APIC_ID_SIZE 4
//address FEE0 0030H
#define CPUAPI_IA32_LOCAL_APIC_VERSION_SIZE 4
//address FEE0 0080H
#define CPUAPI_IA32_LOCAL_APIC_TASK_PRIO_SIZE 4
//address FEE0 0090H
#define CPUAPI_IA32_LOCAL_APIC_ARB_PRIO_SIZE 4
//address FEE0 00A0H
#define CPUAPI_IA32_LOCAL_APIC_PROC_PRIO_SIZE 4
//address FEE0 00B0H
#define CPUAPI_IA32_LOCAL_APIC_EOI_SIZE 4
//address FEE0 00D0H
#define CPUAPI_IA32_LOCAL_APIC_LOG_DST_SIZE 4
//address FEE0 00E0H
#define CPUAPI_IA32_LOCAL_APIC_DST_FORMAT_SIZE 4
//address FEE0 00F0H
#define CPUAPI_IA32_LOCAL_APIC_S_INT_VEC_SIZE 4
//address FEE0 0100H
#define CPUAPI_IA32_LOCAL_APIC_ISR_SIZE 32
//address FEE0 0180H
#define CPUAPI_IA32_LOCAL_APIC_TMR_SIZE 32
//address FEE0 0200H
#define CPUAPI_IA32_LOCAL_APIC_IRR_SIZE 32
//address FEE0 0280H
#define CPUAPI_IA32_LOCAL_APIC_ERR_STATUS_SIZE 4
//address FEE0 0300H
#define CPUAPI_IA32_LOCAL_APIC_INT_CMD_LOW_SIZE 4
//address FEE0 0310H
#define CPUAPI_IA32_LOCAL_APIC_INT_CMD_HIGH_SIZE 4
//address FEE0 0320H
#define CPUAPI_IA32_LOCAL_APIC_LVT_TIMER_SIZE 4
//address FEE0 0330H
#define CPUAPI_IA32_LOCAL_APIC_LVT_THERMAL_SIZE 4
//address FEE0 0340H
#define CPUAPI_IA32_LOCAL_APIC_LVT_PRF_CNT_SIZE 4
//address FEE0 0350H
#define CPUAPI_IA32_LOCAL_APIC_LVT_LINT_0_SIZE 4
//address FEE0 0360H
#define CPUAPI_IA32_LOCAL_APIC_LVT_LINT_1_SIZE 4
//address FEE0 0370H
#define CPUAPI_IA32_LOCAL_APIC_ERR_REG_SIZE 4
//address FEE0 0380H
#define CPUAPI_IA32_LOCAL_APIC_TIMER_INIT_SIZE 4
//address FEE0 0390H
#define CPUAPI_IA32_LOCAL_APIC_TIMER_CURR_SIZE 4
//address FEE0 0E0H
#define CPUAPI_IA32_LOCAL_APIC_TIMER_DIV_CONFIG_SIZE 4

#define CPUAPI_IA32_A20_PIN_SIZE   1 // flag
#define CPUAPI_IA32_INTR_PIN_SIZE  1 // flag
#define CPUAPI_IA32_RESET_PIN_SIZE 1 //flag
#define CPUAPI_IA32_NMI_PIN_SIZE   1 //flag
#define CPUAPI_IA32_SMI_PIN_SIZE   1 //flag
#define CPUAPI_IA32_PSMI_PIN_SIZE  1 //flag

#define CPUAPI_IA32_STI_SET_SIZE   1 //flag

#ifndef VMX_OFF
#define CPUAPI_IA32_LT_VMX_MODE_ACTIVE_SIZE                                                1
#define CPUAPI_IA32_LT_VMX_ROOT_MODE_ACTIVE_SIZE                                           1
#define CPUAPI_IA32_LT_SMM_MONITOR_CONFIGURED_SIZE                                         1
#define CPUAPI_IA32_LT_MTF_ACTIVE_SIZE                                                     1
#endif

#ifndef SMX_OFF
#define CPUAPI_IA32_LT_SENTER_FLAG_SIZE                                                    1
#define CPUAPI_IA32_LT_ENTERAC_FLAG_SIZE                                                   1
#endif

#ifndef VMX_OFF
#define CPUAPI_IA32_LT_VMCS_POINTER_SIZE                                                   8

#define CPUAPI_IA32_LT_VMCS_REVISION_SIZE                                                  4
#define CPUAPI_IA32_LT_VMCS_VMX_ABORT_INDICATOR_SIZE                                       4
#define CPUAPI_IA32_LT_VMCS_PARENT_POINTER_SIZE                                            8
#define CPUAPI_IA32_LT_VMCS_LAUNCH_STATE_SIZE                                              2

#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_PIN_BASED_CONTROLS_SIZE                         4
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_PROCESSOR_BASED_CONTROLS_SIZE                   4
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_EXCEPTION_BITMAP_SIZE                   4
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_CR3_TARGET_COUNT_SIZE                   4
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_CR0_GUEST_HOST_MASK_SIZE                8
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_CR3_GUEST_HOST_MASK_SIZE                8
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_CR4_GUEST_HOST_MASK_SIZE                8
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_CR8_GUEST_HOST_MASK_SIZE                8
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_CR0_READ_SHADOW_SIZE                    8
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_CR3_READ_SHADOW_SIZE                    8
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_CR4_READ_SHADOW_SIZE                    8
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_CR8_READ_SHADOW_SIZE                    8
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_CR3_TARGET_VALUE_FIRST_SIZE             8
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_CR3_TARGET_VALUE_LAST_SIZE              8
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_CR3_TARGET_VALUE_0_SIZE                 8
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_CR3_TARGET_VALUE_1_SIZE                 8
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_CR3_TARGET_VALUE_2_SIZE                 8
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_CR3_TARGET_VALUE_3_SIZE                 8
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_PAGE_FAULT_ERROR_CODE_MASK_SIZE         8
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_PAGE_FAULT_ERROR_CODE_MATCH_SIZE        8

#define   CPUAPI_IA32_LT_VMCS_VM_EXIT_CONTROL_CONTROLS_SIZE                                4
#define   CPUAPI_IA32_LT_VMCS_VM_EXIT_CONTROL_MSR_STORE_COUNT_SIZE                         4
#define   CPUAPI_IA32_LT_VMCS_VM_EXIT_CONTROL_MSR_LOAD_COUNT_SIZE                          4

#define   CPUAPI_IA32_LT_VMCS_VM_ENTRY_CONTROL_CONTROLS_SIZE                               4
#define   CPUAPI_IA32_LT_VMCS_VM_ENTRY_CONTROL_INTERRUPTION_INFORMATION_SIZE               4
#define   CPUAPI_IA32_LT_VMCS_VM_ENTRY_CONTROL_EXCEPTION_ERROR_CODE_SIZE                   4
#define   CPUAPI_IA32_LT_VMCS_VM_ENTRY_CONTROL_MSR_LOAD_COUNT_SIZE                         4
#define   CPUAPI_IA32_LT_VMCS_VM_ENTRY_CONTROL_INSTRUCTION_LENGTH_SIZE                     4

#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_IO_BITMAP_A_ALL_SIZE                    8
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_IO_BITMAP_A_HIGH_SIZE                   4
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_IO_BITMAP_B_ALL_SIZE                    8
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_IO_BITMAP_B_HIGH_SIZE                   4
#define   CPUAPI_IA32_LT_VMCS_VM_EXIT_MSR_STORE_ADDRESS_ALL_SIZE                           8
#define   CPUAPI_IA32_LT_VMCS_VM_EXIT_MSR_STORE_ADDRESS_HIGH_SIZE                          4
#define   CPUAPI_IA32_LT_VMCS_VM_EXIT_MSR_LOAD_ADDRESS_ALL_SIZE                            8
#define   CPUAPI_IA32_LT_VMCS_VM_EXIT_MSR_LOAD_ADDRESS_HIGH_SIZE                           4
#define   CPUAPI_IA32_LT_VMCS_VM_ENTRY_MSR_LOAD_ADDRESS_ALL_SIZE                           8
#define   CPUAPI_IA32_LT_VMCS_VM_ENTRY_MSR_LOAD_ADDRESS_HIGH_SIZE                          4
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_TSC_COMPARATOR_ALL_SIZE                 8
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_TSC_COMPARATOR_HIGH_SIZE                4
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_TSC_OFFSET_ALL_SIZE                     8
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_TSC_OFFSET_HIGH_SIZE                    4
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_OSV_CONTROLLING_VMCS_POINTER_ALL_SIZE   8
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_OSV_CONTROLLING_VMCS_POINTER_HIGH_SIZE  4
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_MSR_BITMAPS_ADDRESS_ALL_SIZE            8
#define   CPUAPI_IA32_LT_VMCS_VM_EXECUTION_CONTROL_MSR_BITMAPS_ADDRESS_HIGH_SIZE           4

#define   CPUAPI_IA32_LT_VMCS_VM_EXIT_INFO_VM_INSTRUCTION_ERROR_SIZE                       4
#define   CPUAPI_IA32_LT_VMCS_VM_EXIT_INFO_EXIT_REASON_SIZE                                4
#define   CPUAPI_IA32_LT_VMCS_VM_EXIT_INTERRUPTION_INFO_SIZE                               4
#define   CPUAPI_IA32_LT_VMCS_VM_EXIT_INTERRUPTION_ERROR_CODE_SIZE                         4
#define   CPUAPI_IA32_LT_VMCS_IDT_VECTORING_INFO_SIZE                                      4
#define   CPUAPI_IA32_LT_VMCS_VM_EXIT_INFO_IDT_VECTORING_ERROR_CODE_SIZE                   4
#define   CPUAPI_IA32_LT_VMCS_VM_EXIT_INFO_INSTRUCTION_LENGTH_SIZE                         4
#define   CPUAPI_IA32_LT_VMCS_INSTRUCTION_INFORMATION_SIZE                                 4
#define   CPUAPI_IA32_LT_VMCS_VM_EXIT_INFO_EXIT_QUALIFICATION_SIZE                         8
#define   CPUAPI_IA32_LT_VMCS_I0_ECX_SIZE                                                  8
#define   CPUAPI_IA32_LT_VMCS_IO_ESI_SIZE                                                  8
#define   CPUAPI_IA32_LT_VMCS_IO_EDI_SIZE                                                  8
#define   CPUAPI_IA32_LT_VMCS_IO_EIP_SIZE                                                  8
#define   CPUAPI_IA32_LT_VMCS_IO_INSTRUCTION_INITIAL_ADDRESS_SIZE                          8

#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_CR0_SIZE                                         8
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_CR3_SIZE                                         8
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_CR4_SIZE                                         8
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_DR7_SIZE                                         8
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_ES_SELECTOR_SIZE                                 2
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_ES_BASE_SIZE                                     8
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_ES_LIMIT_SIZE                                    4
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_ES_AR_SIZE                                       4
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_CS_SELECTOR_SIZE                                 2
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_CS_BASE_SIZE                                     8
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_CS_LIMIT_SIZE                                    4
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_CS_AR_SIZE                                       4
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_SS_SELECTOR_SIZE                                 2
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_SS_BASE_SIZE                                     8
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_SS_LIMIT_SIZE                                    4
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_SS_AR_SIZE                                       4
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_DS_SELECTOR_SIZE                                 2
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_DS_BASE_SIZE                                     8
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_DS_LIMIT_SIZE                                    4
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_DS_AR_SIZE                                       4
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_FS_SELECTOR_SIZE                                 2
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_FS_BASE_SIZE                                     8
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_FS_LIMIT_SIZE                                    4
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_FS_AR_SIZE                                       4
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_GS_SELECTOR_SIZE                                 2
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_GS_BASE_SIZE                                     8
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_GS_LIMIT_SIZE                                    4
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_GS_AR_SIZE                                       4
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_LDTR_SELECTOR_SIZE                               2
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_LDTR_BASE_SIZE                                   8
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_LDTR_LIMIT_SIZE                                  4
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_LDTR_AR_SIZE                                     4
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_TR_SELECTOR_SIZE                                 2
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_TR_BASE_SIZE                                     8
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_TR_LIMIT_SIZE                                    4
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_TR_AR_SIZE                                       4
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_GDTR_BASE_SIZE                                   8
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_GDTR_LIMIT_SIZE                                  2
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_IDTR_BASE_SIZE                                   8
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_IDTR_LIMIT_SIZE                                  2
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_RSP_SIZE                                         8
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_RIP_SIZE                                         8
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_RFLAGS_SIZE                                      8
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_PENDING_DEBUG_EXCEPTION_SIZE                     8
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_INTERRUPTIBILITY_SIZE                            4
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_SLEEP_STATE_SIZE                                 4
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_GUEST_WORKING_VMCS_POINTER_ALL_SIZE              8
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_GUEST_WORKING_VMCS_POINTER_HIGH_SIZE             4
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_IA32_SYSENTER_ESP_SIZE                           8
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_IA32_SYSENTER_EIP_SIZE                           8
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_IA32_DEBUGCTL_MSR_SIZE                           8
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_SMBASE_SIZE                                      4
#define   CPUAPI_IA32_LT_VMCS_GUEST_STATE_IA32_SYSENTER_CS_SIZE                            4




#define   CPUAPI_IA32_LT_VMCS_MONITOR_STATE_CR0_SIZE                                       8
#define   CPUAPI_IA32_LT_VMCS_MONITOR_STATE_CR3_SIZE                                       8
#define   CPUAPI_IA32_LT_VMCS_MONITOR_STATE_CR4_SIZE                                       8
#define   CPUAPI_IA32_LT_VMCS_MONITOR_STATE_ES_SELECTOR_SIZE                               2
#define   CPUAPI_IA32_LT_VMCS_MONITOR_STATE_CS_SELECTOR_SIZE                               2
#define   CPUAPI_IA32_LT_VMCS_MONITOR_STATE_SS_SELECTOR_SIZE                               2
#define   CPUAPI_IA32_LT_VMCS_MONITOR_STATE_DS_SELECTOR_SIZE                               2
#define   CPUAPI_IA32_LT_VMCS_MONITOR_STATE_FS_SELECTOR_SIZE                               2
#define   CPUAPI_IA32_LT_VMCS_MONITOR_STATE_GS_SELECTOR_SIZE                               2
#define   CPUAPI_IA32_LT_VMCS_MONITOR_STATE_FS_BASE_SIZE                                   2
#define   CPUAPI_IA32_LT_VMCS_MONITOR_STATE_GS_BASE_SIZE                                   2
#define   CPUAPI_IA32_LT_VMCS_MONITOR_STATE_TR_SELECTOR_SIZE                               2
#define   CPUAPI_IA32_LT_VMCS_MONITOR_STATE_TR_BASE_SIZE                                   8
#define   CPUAPI_IA32_LT_VMCS_MONITOR_STATE_GDTR_BASE_SIZE                                 8
#define   CPUAPI_IA32_LT_VMCS_MONITOR_STATE_IDTR_BASE_SIZE                                 8
#define   CPUAPI_IA32_LT_VMCS_MONITOR_STATE_RSP_SIZE                                       8
#define   CPUAPI_IA32_LT_VMCS_MONITOR_STATE_RIP_SIZE                                       8
#define   CPUAPI_IA32_LT_VMCS_MONITOR_STATE_IA32_SYSENTER_ESP_SIZE                         8
#define   CPUAPI_IA32_LT_VMCS_MONITOR_STATE_IA32_SYSENTER_EIP_SIZE                         8
#define   CPUAPI_IA32_LT_VMCS_MONITOR_STATE_IA32_SYSENTER_CS_SIZE                          4

#endif

#ifndef MEROM_APE_NI_OFF                  
#define CPUAPI_IA32_TCE_STATE_SIZE                                                         4
#define CPUAPI_IA32_TCE_PRIMARY_AGENT_SIZE                                                 4
#define CPUAPI_IA32_TCE_TASK_CR3_SIZE                                                      8
#define CPUAPI_IA32_TCE_SXFR_ID_SIZE                                                       4

#define CPUAPI_IA32_APE_YIELD_BLOCK_BIT_SIZE                                               4
#define CPUAPI_IA32_APE_SAVED_EIP_SIZE                                                     8
#define CPUAPI_IA32_APE_RING3_CHANNEL0_EAX_SIZE                                            8
#define CPUAPI_IA32_APE_RING3_CHANNEL0_EBX_SIZE                                            8
#define CPUAPI_IA32_APE_RING3_CHANNEL0_ECX_SIZE                                            8
#define CPUAPI_IA32_APE_RING3_CHANNEL0_EDX_SIZE                                            8
#define CPUAPI_IA32_APE_RING3_CHANNEL1_EAX_SIZE                                            8
#define CPUAPI_IA32_APE_RING3_CHANNEL1_EBX_SIZE                                            8
#define CPUAPI_IA32_APE_RING3_CHANNEL1_ECX_SIZE                                            8
#define CPUAPI_IA32_APE_RING3_CHANNEL1_EDX_SIZE                                            8
#endif /* MEROM_APE_NI_OFF */             

#ifdef __cplusplus
}
#endif

#endif /* CPUAPI_ARCH_IA32_H */

