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

/** Element name expansion macrs
 *
 * Here are some macros used to expand names of architecture elements using
 * an index or some other argument for the expansion.
 */
#define CPUAPI_IPF_GR_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.gr.",num)
#define CPUAPI_IPF_GR_NUM 128
#define CPUAPI_IPF_GR_SIZE 8

#define CPUAPI_IPF_CR_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.cr.",num)
#define CPUAPI_IPF_CR_NUM 82
#define CPUAPI_IPF_CR_SIZE 8

#define CPUAPI_IPF_NAT_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.nat.",num)
#define CPUAPI_IPF_NAT_NUM 128
#define CPUAPI_IPF_NAT_SIZE 1  // actually 1 bit

#define CPUAPI_IPF_FR_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.fr.",num)
#define CPUAPI_IPF_FR_NUM 128
#define CPUAPI_IPF_FR_SIZE 11

#define CPUAPI_IPF_BR_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.br.",num)
#define CPUAPI_IPF_BR_NUM 8
#define CPUAPI_IPF_BR_SIZE 8

#define CPUAPI_IPF_AR_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.ar.",num)
#define CPUAPI_IPF_AR_NUM 128
#define CPUAPI_IPF_AR_SIZE 8

#define CPUAPI_IPF_RR_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.rr.",num)
#define CPUAPI_IPF_RR_NUM 8
#define CPUAPI_IPF_RR_SIZE 8

#define CPUAPI_IPF_DBR_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.dbr.",num)
#define CPUAPI_IPF_DBR_NUM 32
#define CPUAPI_IPF_DBR_SIZE 8

#define CPUAPI_IPF_IBR_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.ibr.",num)
#define CPUAPI_IPF_IBR_NUM 32
#define CPUAPI_IPF_IBR_SIZE 8

#define CPUAPI_IPF_PMC_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.pmc.",num)
#define CPUAPI_IPF_PMC_NUM 32
#define CPUAPI_IPF_PMC_SIZE 8

#define CPUAPI_IPF_PMD_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.pmd.",num)
#define CPUAPI_IPF_PMD_NUM 32
#define CPUAPI_IPF_PMD_SIZE 8

#define CPUAPI_IPF_PKR_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.pkr.",num)
#define CPUAPI_IPF_PKR_NUM 16
#define CPUAPI_IPF_PKR_SIZE 8

#define CPUAPI_IPF_RB_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.bank.",num)
#define CPUAPI_IPF_RB_NUM 16
#define CPUAPI_IPF_RB_SIZE 8

#define CPUAPI_IPF_RB_NAT_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.bank.nat.",num)
#define CPUAPI_IPF_RB_NAT_NUM 16
#define CPUAPI_IPF_RB_NAT_SIZE 1

#define CPUAPI_IPF_CPUID_NAME(str,num) sprintf(str,"%s%d","arch.ipf.cpuid.",num)
#define CPUAPI_IPF_CPUID_NUM 16
#define CPUAPI_IPF_CPUID_SIZE 8       

#define CPUAPI_IPF_MSR_NAME(str,num) sprintf(str,"%s%d","arch.ipf.register.msr.",num)
#define CPUAPI_IPF_MSR_NUM 2048
#define CPUAPI_IPF_MSR_SIZE 8

//TR Translation Registers - TLB

#define CPUAPI_IPF_TLB_DATA_TR_SIZE 11
#define CPUAPI_IPF_TLB_INST_TR_SIZE 11

#define CPUAPI_IPF_TLB_DATA_CACHE_SIZE 11
#define CPUAPI_IPF_TLB_INST_CACHE_SIZE 11

#define CPUAPI_IPF_TLB_DATA_TR_NAME(str,num) sprintf(str,"%s%d","arch.ipf.tlb.data.tr.",num)
#define CPUAPI_IPF_TLB_DATA_TR_NUM 256

#define CPUAPI_IPF_TLB_INST_TR_NAME(str,num) sprintf(str,"%s%d","arch.ipf.tlb.inst.tr.",num)
#define CPUAPI_IPF_TLB_INST_TR_NUM 256

//TLB Cache
#define CPUAPI_IPF_TLB_INST_CACHE_NAME(str,num) sprintf(str,"%s%d","arch.ipf.tlb.inst.cache.",num)
#define CPUAPI_IPF_TLB_DATA_CACHE_NUM 256

#define CPUAPI_IPF_TLB_DATA_CACHE_NAME(str,num) sprintf(str,"%s%d","arch.ipf.tlb.data.cache.",num)
#define CPUAPI_IPF_TLB_INST_CACHE_NUM 256

/**
 * List of elements that don't have an index (in their name)
 */
#define CPUAPI_IPF_IP                     "arch.ipf.register.ip"        // Instruction Pointer
#define CPUAPI_IPF_IP_SIZE 8       

#define CPUAPI_IPF_PR                     "arch.ipf.register.pr"        // Predicate Registers
#define CPUAPI_IPF_PR_SIZE 8

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
      

#endif /* CPUAPI_ARCH_IPF_H */
