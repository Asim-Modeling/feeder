/*****************************************************************************
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
 * 
 * File:  cpuapi_arch_ipf.h
 * 
 * Description:    
 * 
 *****************************************************************************/
#ifndef CPUAPI_ARCH_IPF_H
#define CPUAPI_ARCH_IPF_H


typedef struct cpuapi_ipf_arch_specific_callbacks_s {

   cpuapi_stat_t (*io_port_access)(cpuapi_cid_t cid,
                                   cpuapi_phys_addr_t paddr, 
                                   cpuapi_size_t size,
                                   cpuapi_access_type_t access,
                                   void* buffer);
   cpuapi_stat_t (*processor_interrupt_block_access) (cpuapi_cid_t cid,
                                                      cpuapi_phys_addr_t paddr,
                                                      cpuapi_size_t size,
                                                      cpuapi_access_type_t access,
                                                      void* buffer);
} cpuapi_ipf_arch_specific_callbacks_t;


typedef enum cpuapi_cpu_mode_s {
	CPUAPI_First_IPF_Cpu_Mode = CPUAPI_Cpu_Mode_Arch_2,
	CPUAPI_Cpu_Mode_8086 = CPUAPI_First_IPF_Cpu_Mode,
	CPUAPI_Cpu_Mode_Big_Real,
	CPUAPI_Cpu_Mode_v8086,
	CPUAPI_Cpu_Mode_Protected_16,
	CPUAPI_Cpu_Mode_Protected_32,
	CPUAPI_Cpu_Mode_SMM,
    CPUAPI_Cpu_Mode_Ipf,
	CPUAPI_Last_IPF_Cpu_Mode = CPUAPI_Cpu_Mode_Ipf,

} cpuapi_cpu_mode_t;

#define CPUAPI_IPF_INST_BYTES_LENGTH             16

typedef struct {
    unsigned char        inst_bytes[CPUAPI_IPF_INST_BYTES_LENGTH];/* instruction bytes */
    cpuapi_virt_addr_t   branch_target_virt_addr;    /* instruction virtual address */
    cpuapi_phys_addr_t   phy_mem_addr;    /* memory physical address */
    cpuapi_virt_addr_t   virt_mem_addr;   /* memory virtual address */
    cpuapi_u64_t         cfm;             /* Current Frame Marker  */
    cpuapi_u64_t         bsp;             /* Backing Store Pointer */
    cpuapi_u64_t         predicates;      /* Predicate Register    */
    cpuapi_boolean_t     branch_taken;
    cpuapi_boolean_t     store_inst;
    cpuapi_boolean_t     load_inst;
    cpuapi_boolean_t     branch_inst;
	cpuapi_boolean_t     exception_inst;
	cpuapi_boolean_t     io_inst;
} cpuapi_ipf_inst_info_t;

/** Element name expansion macrs
 *
 * Here are some macros used to expand names of architecture elements using
 * an index or some other argument for the expansion.
 */
#define CPUAPI_IPF_GR_KEY_NAME "arch.ipf.register.gr."
#define CPUAPI_IPF_GR_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.gr.",num)
#define CPUAPI_IPF_GR_NUM 128
#define CPUAPI_IPF_GR_SIZE 8

#define CPUAPI_IPF_CR_KEY_NAME "arch.ipf.register.cr."
#define CPUAPI_IPF_CR_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.cr.",num)
#define CPUAPI_IPF_CR_NUM 82
#define CPUAPI_IPF_CR_SIZE 8

#define CPUAPI_IPF_NAT_KEY_NAME "arch.ipf.register.nat."
#define CPUAPI_IPF_NAT_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.nat.",num)
#define CPUAPI_IPF_NAT_NUM 128
#define CPUAPI_IPF_NAT_SIZE 1  // actually 1 bit

#define CPUAPI_IPF_FR_KEY_NAME "arch.ipf.register.fr."
#define CPUAPI_IPF_FR_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.fr.",num)
#define CPUAPI_IPF_FR_NUM 128
#define CPUAPI_IPF_FR_SIZE 16

#define CPUAPI_IPF_BR_KEY_NAME "arch.ipf.register.br."
#define CPUAPI_IPF_BR_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.br.",num)
#define CPUAPI_IPF_BR_NUM 8
#define CPUAPI_IPF_BR_SIZE 8

#define CPUAPI_IPF_AR_KEY_NAME "arch.ipf.register.ar."
#define CPUAPI_IPF_AR_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.ar.",num)
#define CPUAPI_IPF_AR_NUM 128
#define CPUAPI_IPF_AR_SIZE 8

#define CPUAPI_IPF_RR_KEY_NAME "arch.ipf.register.rr."
#define CPUAPI_IPF_RR_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.rr.",num)
#define CPUAPI_IPF_RR_NUM 8
#define CPUAPI_IPF_RR_SIZE 8

#define CPUAPI_IPF_DBR_KEY_NAME "arch.ipf.register.dbr."
#define CPUAPI_IPF_DBR_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.dbr.",num)
#define CPUAPI_IPF_DBR_NUM 32
#define CPUAPI_IPF_DBR_SIZE 8

#define CPUAPI_IPF_IBR_KEY_NAME "arch.ipf.register.ibr."
#define CPUAPI_IPF_IBR_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.ibr.",num)
#define CPUAPI_IPF_IBR_NUM 32
#define CPUAPI_IPF_IBR_SIZE 8

#define CPUAPI_IPF_PMC_KEY_NAME "arch.ipf.register.pmc."
#define CPUAPI_IPF_PMC_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.pmc.",num)
#define CPUAPI_IPF_PMC_NUM 32
#define CPUAPI_IPF_PMC_SIZE 8

#define CPUAPI_IPF_PMD_KEY_NAME "arch.ipf.register.pmd."
#define CPUAPI_IPF_PMD_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.pmd.",num)
#define CPUAPI_IPF_PMD_NUM 32
#define CPUAPI_IPF_PMD_SIZE 8

#define CPUAPI_IPF_PKR_KEY_NAME "arch.ipf.register.pkr."
#define CPUAPI_IPF_PKR_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.pkr.",num)
#define CPUAPI_IPF_PKR_NUM 16
#define CPUAPI_IPF_PKR_SIZE 8

#define CPUAPI_IPF_RB_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.bank.",num)
#define CPUAPI_IPF_RB_NUM 16
#define CPUAPI_IPF_RB_SIZE 8

#define CPUAPI_IPF_RB_NAT_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.bank.nat.",num)
#define CPUAPI_IPF_RB_NAT_NUM 16
#define CPUAPI_IPF_RB_NAT_SIZE 1

#define CPUAPI_IPF_CPUID_KEY_NAME "arch.ipf.cpuid."
#define CPUAPI_IPF_CPUID_NAME(str,num) sprintf(str,"%s%d","arch.ipf.cpuid.",num)
#define CPUAPI_IPF_CPUID_NUM 16
#define CPUAPI_IPF_CPUID_SIZE 8       

#define CPUAPI_IPF_MSR_KEY_NAME "arch.ipf.register.msr."
#define CPUAPI_IPF_MSR_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.msr.",num)
#define CPUAPI_IPF_MSR_NUM 2048
#define CPUAPI_IPF_MSR_SIZE 8

/*Tlb entry definition taken from gambit*/
typedef struct
{
    cpuapi_u32_t valid;             /* is the entry valid?       */
    cpuapi_u32_t global;            /* global bit                */    
    cpuapi_u32_t p;                 /* present bit               */
    cpuapi_u32_t rid;               /* region id                 */
    cpuapi_u32_t key;               /* protection key value      */
    cpuapi_u64_t vpn;               /* virtual page number       */
    cpuapi_u64_t ppn;               /* physical page number      */
    cpuapi_u32_t page_size;         /* the entry's page size	 */                              
    cpuapi_u32_t ed;                /* speculative exception bit */
    cpuapi_u32_t ar;                /* access rights             */
    cpuapi_u32_t pl;                /* page privilege level      */
    cpuapi_u32_t d;                 /* dirty bit                 */
    cpuapi_u32_t a;                 /* access bit                */    
    cpuapi_u32_t ma;                /* memory attribute          */    
}  cpuapi_ipf_tlb_entry_t;
//TR Translation Registers - TLB

#define CPUAPI_IPF_TLB_DATA_TR_SIZE sizeof(cpuapi_ipf_tlb_entry_t)
#define CPUAPI_IPF_TLB_INST_TR_SIZE sizeof(cpuapi_ipf_tlb_entry_t)

#define CPUAPI_IPF_TLB_DATA_CACHE_SIZE sizeof(cpuapi_ipf_tlb_entry_t)
#define CPUAPI_IPF_TLB_INST_CACHE_SIZE sizeof(cpuapi_ipf_tlb_entry_t)
#define CPUAPI_IPF_TLB_DATA_TR_KEY_NAME  "arch.ipf.tlb.data.tr."
#define CPUAPI_IPF_TLB_DATA_TR_NAME(str,num) sprintf(str,"%s%d","arch.ipf.tlb.data.tr.",num)
#define CPUAPI_IPF_TLB_DATA_TR_NUM 256

#define CPUAPI_IPF_TLB_INST_TR_KEY_NAME  "arch.ipf.tlb.inst.tr."
#define CPUAPI_IPF_TLB_INST_TR_NAME(str,num) sprintf(str,"%s%d","arch.ipf.tlb.inst.tr.",num)
#define CPUAPI_IPF_TLB_INST_TR_NUM 256

//TLB Cache
#define CPUAPI_IPF_TLB_INST_CACHE_KEY_NAME  "arch.ipf.tlb.inst.cache."
#define CPUAPI_IPF_TLB_INST_CACHE_NAME(str,num) sprintf(str,"%s%d","arch.ipf.tlb.inst.cache.",num)
#define CPUAPI_IPF_TLB_DATA_CACHE_NUM 256

#define CPUAPI_IPF_TLB_DATA_CACHE_KEY_NAME  "arch.ipf.tlb.data.cache."
#define CPUAPI_IPF_TLB_DATA_CACHE_NAME(str,num) sprintf(str,"%s%d","arch.ipf.tlb.data.cache.",num)
#define CPUAPI_IPF_TLB_INST_CACHE_NUM 256

typedef enum {
	CPUAPI_Reg_Type_AR,					/* Application regs   */
    CPUAPI_Reg_Type_BR,					/* Branch regs	      */
    CPUAPI_Reg_Type_CR,					/* Control regs	      */
    CPUAPI_Reg_Type_GR,					/* General regs	      */
    CPUAPI_Reg_Type_FR,					/* Floating regs      */
    CPUAPI_Reg_Type_RR,					/* Region regs	      */
    CPUAPI_Reg_Type_PR,					/* Predicate regs     */
    CPUAPI_Reg_Type_SR,					/* Swap (bank) regs   */
    CPUAPI_Reg_Type_PMC,				/* Perf. control regs */
    CPUAPI_Reg_Type_PMD,				/* Perf. data regs    */
    CPUAPI_Reg_Type_PKR,				/* Protection regs    */
    CPUAPI_Reg_Type_DBR,				/* Data break regs    */
    CPUAPI_Reg_Type_IBR,				/* Inst. break regs   */
    CPUAPI_Reg_Type_MSR,				/* Model spec. regs   */
    CPUAPI_Reg_Type_CPUID,				/* CPU ident. regs    */
    CPUAPI_Reg_Type_SISR,				/* Inserv. req. regs  */
    CPUAPI_Reg_Type_RNATC,				/* RNaT collect regs  */
    CPUAPI_Reg_Type_RSE,				/* RSE internal regs  */
    CPUAPI_Reg_Type_PSR,				/* PSR reg			  */
    CPUAPI_Reg_Type_Last,
}cpuapi_register_type_t;


typedef struct {
    cpuapi_u64_t  addr;					/* Advanced load address */
    cpuapi_register_type_t reg_type;    /* Register_type*/
    cpuapi_u32_t  reg_num;				/* Register number		 */
    cpuapi_size_t size;    
	cpuapi_u32_t valid;
}cpuapi_alat_entry_t;

//ALAT entries
#define CPUAPI_IPF_ALAT_KEY_NAME  "arch.ipf.alat."
#define CPUAPI_IPF_ALAT_NAME(str,num) sprintf(str,"%s%d",CPUAPI_IPF_ALAT_KEY_NAME,num)
#define CPUAPI_IPF_ALAT_SIZE sizeof(cpuapi_alat_entry_t)
#define CPUAPI_IPF_ALAT_NUM 32

/**
 * List of elements that don't have an index (in their name)
 */
#define CPUAPI_IPF_IP                     "arch.ipf.register.ip"        // Instruction Pointer
#define CPUAPI_IPF_IP_SIZE 8       

#define CPUAPI_IPF_PR                     "arch.ipf.register.pr"        // Predicate Registers
#define CPUAPI_IPF_PR_SIZE 8
#define CPUAPI_IPF_PR_NUM  128

#define CPUAPI_IPF_UM                     "arch.ipf.register.um"        // User Mask
#define CPUAPI_IPF_UM_SIZE 6

#define CPUAPI_IPF_CFM                    "arch.ipf.register.cfm"       // Current Frame Marker
#define CPUAPI_IPF_CFM_SIZE 8

#define CPUAPI_IPF_PSR                    "arch.ipf.register.psr"       // Processor Status Register
#define CPUAPI_IPF_PSR_SIZE 8

#define CPUAPI_IPF_PIB_LOW                "arch.ipf.register.pib.low"   // Processor Interrupt Block
#define CPUAPI_IPF_PIB_LOW_SIZE 1000

#define CPUAPI_IPF_PIB_HIGH_XTP           "arch.ipf.register.pib.high.xtp"
#define CPUAPI_IPF_PIB_HIGH_XTP_SIZE 1

#define CPUAPI_IPF_PIB_HIGH_INTA          "arch.ipf.register.pib.high.inta"
#define CPUAPI_IPF_PIB_HIGH_INTA_SIZE 1

#define CPUAPI_IPF_RSE_N_STACKED_PHYS     "arch.ipf.rse.n_stacked_phys" // Number of Stacked Physical Registers
#define CPUAPI_IPF_RSE_N_STACKED_PHYS_SIZE 16

#define CPUAPI_IPF_RSE_BOF                "arch.ipf.rse.bof"            // Bottom of frame register number
#define CPUAPI_IPF_RSE_BOF_SIZE           4

#define CPUAPI_IPF_RSE_STORE_REG          "arch.ipf.rse.store_reg"      // Store register number: Physical register number of next register to be stored by RSE
#define CPUAPI_IPF_RSE_STORE_REG_SIZE     4

#define CPUAPI_IPF_RSE_LOAD_REG           "arch.ipf.rse.load_reg"       // Load register number: Physical register number one greater than the next register to load
#define CPUAPI_IPF_RSE_LOAD_REG_SIZE      4

#define CPUAPI_IPF_RSE_BSPLOAD            "arch.ipf.rse.bspload"        // Backing Store Pointer for memory loads
#define CPUAPI_IPF_RSE_BSPLOAD_SIZE       8

#define CPUAPI_IPF_RSE_RNAT_BIT_INDEX     "arch.ipf.rse.rnat_bit_index" // Nat collection bit index
#define CPUAPI_IPF_RSE_RNAT_BIT_INDEX_SIZE 1

#define CPUAPI_IPF_RSE_CFLE               "arch.ipf.rse.cfle"           // current frameLoad Enable
#define CPUAPI_IPF_RSE_CFLE_SIZE          8

#define CPUAPI_IPF_RSE_NDIRTY             "arch.ipf.rse.ndirty"         // Number of dirty registers on the register stack
#define CPUAPI_IPF_RSE_NDIRTY_SIZE        8

#define CPUAPI_IPF_RSE_NDIRTY_WORDS       "arch.ipf.rse.ndirty_words"   // Number of dirty words on the register stack plus corresponding number of Nat collection
#define CPUAPI_IPF_RSE_NDIRTY_WORDS_SIZE  8

#define CPUAPI_IPF_INTR_PIN                "arch.ipf.pic.pin.intr"
#define CPUAPI_IPF_INTR_PIN_SIZE           1 // flag
      
#define CPUAPI_IPF_SAPIC_ISR_KEY_NAME "arch.ipf.register.sapic.isr."
#define CPUAPI_IPF_SAPIC_ISR_NAME(str,num) sprintf(str,"%s%d",CPUAPI_IPF_SAPIC_ISR_KEY_NAME,num)
#define CPUAPI_IPF_SAPIC_ISR_NUM 16
#define CPUAPI_IPF_SAPIC_ISR_SIZE 2

#endif /* CPUAPI_ARCH_IPF_H */
