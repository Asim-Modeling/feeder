/*
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
 */

#ifndef CPUAPI_TYPES_H
#define CPUAPI_TYPES_H

#include "cpuapi_mem.h"
#include "cpuapi_sim_elements.h"

#ifdef __cplusplus
extern "C"
{
#endif


/* Capabilities strings*/
#define CPUAPI_MFINIT "MFINIT"
#define CPUAPI_MFTRM  "MFTRM"
#define CPUAPI_FMAH   "FMAH"
#define CPUAPI_FMAC   "FMAC"
#define CPUAPI_BRK    "BRK" /* Breakpoints*/
#define CPUAPI_SR     "SR"  /* Save Restore*/
#define CPUAPI_EX     "EX"
#define CPUAPI_APIC   "APIC"
#define CPUAPI_MS     "MS"
#define CPUAPI_PRF    "PRF"
#define CPUAPI_GS     "GS"   /* Gear Shift*/
#define CPUAPI_ISIS   "ISIS" /* Isis Model*/

/* max. number of elements */
#define MAX_ELEMENTS 5000

typedef void         *cpuapi_cid_t;
typedef void         *cpuapi_pid_t;

typedef cpuapi_u64_t cpuapi_inst_t;
typedef void         *cpuapi_hptr_t;
typedef cpuapi_u32_t          cpuapi_handle_t;
typedef void         *cpuapi_brk_arg_t;
typedef unsigned int cpuapi_boolean_t;


typedef cpuapi_u64_t cpuapi_virt_addr_t;
typedef cpuapi_u64_t cpuapi_io_addr_t;
typedef cpuapi_u64_t cpuapi_cpunum_t;
typedef cpuapi_u64_t cpuapi_phase_t;

#define CPUAPI_MAX_ITEM_SIZE 11
#define CPUAPI_MAX_NUM_SOURCES 16
#define CPUAPI_MAX_NUM_DST 16

typedef struct
{
    char src_value[CPUAPI_MAX_NUM_SOURCES][CPUAPI_MAX_ITEM_SIZE];
    char dst_value[CPUAPI_MAX_NUM_DST][CPUAPI_MAX_ITEM_SIZE];
    cpuapi_handle_t src_handle[CPUAPI_MAX_NUM_SOURCES];
    cpuapi_handle_t dst_handle[CPUAPI_MAX_NUM_DST];
	cpuapi_u32_t dst_count;     /*  number of destinations */
    cpuapi_u32_t src_count;    /*  number of sources  */
} cpuapi_operands_info_t;

typedef struct {
    cpuapi_inst_t        inst_id;         /* unique instruction identifier */
    cpuapi_inst_t        icount;
    cpuapi_boolean_t     new_inst;
    unsigned char        inst_bytes[CPUAPI_SIM_INST_BYTES_LENGTH];/* instruction bytes */
    cpuapi_phys_addr_t   phy_addr;        /* instruction physical address */
    cpuapi_virt_addr_t   virt_addr;       /* instruction virtual address */
    cpuapi_u32_t         eflags;
    unsigned long        cur_repeat_count;
    cpuapi_boolean_t     branch_taken;
    cpuapi_virt_addr_t   branch_target_virt_addr;    /* instruction virtual address */
    cpuapi_boolean_t     store_inst;
    cpuapi_boolean_t     load_inst;
    cpuapi_boolean_t     branch_inst;
    cpuapi_phys_addr_t   phy_mem_addr;    /* memory physical address */
    cpuapi_virt_addr_t   virt_mem_addr;   /* memory virtual address */
    cpuapi_u64_t         cfm;             /* Current Frame Marker  (Itanium only) */
    cpuapi_u64_t         bsp;             /* Backing Store Pointer (Itanium only) */
    cpuapi_u64_t         predicates;      /* Predicate Register    (Itanium only) */
} cpuapi_inst_info_t;

typedef struct cpuapi_perf_freq_s{
   cpuapi_u32_t numerator;
   cpuapi_u32_t denominator;  
}cpuapi_perf_freq_t;

typedef enum {
	CPUAPI_Stat_Ok,
	CPUAPI_Stat_Err,
} cpuapi_stat_t;

typedef enum {
   CPUAPI_Type_Func           = 0x1,	
   CPUAPI_Type_Perf           = 0x2,
   CPUAPI_Type_Master_Retire  = 0x4,
   CPUAPI_Type_Master_Fetch   = 0x8,
   CPUAPI_Type_Slave          = 0x10,
   CPUAPI_Type_Indep          = 0x20,
   CPUAPI_Type_Reference      = 0x40,	
   CPUAPI_Type_Inactive       = 0x100,      
   CPUAPI_Type_IntHdl         = 0x200,
   CPUAPI_Type_Spec_Func      = 0x400,
   CPUAPI_Type_Spec_Perf      = 0x800
} cpuapi_type_t;

#define CPUAPI_FUNC              "func"
#define CPUAPI_PERF              "perf"
#define CPUAPI_MASTER_RETIRE     "master_retire"
#define CPUAPI_MASTER_FETCH      "master_fetch"
#define CPUAPI_REFERENCE         "reference"
#define CPUAPI_INACTIVE          "inactive"
#define CPUAPI_SLAVE             "slave"
#define CPUAPI_HANDLE_INT        "handle_int"
#define CPUAPI_INDEP             "indep"
#define CPUAPI_SPEC_FUNC         "spec_func"
#define CPUAPI_SPEC_PERF         "spec_perf"
		
typedef enum {
		CPUAPI_IA32, 
		CPUAPI_IPF 
}cpuapi_arch_type_t;

/**
 * Info: The typedef below actually serves only as an enum (no one needs this type)
 *       In case of multiple architectures, each architecutre has it's own H file
 *       where cpu modes are defined. To organize these enum definition a global
 *       multi-arch definition is here. All cpu modes will be aligned using this
 *       enum. The MAX value below is inherited from IA32 (which has the most modes).
 *       A bonus here is that it also enables a 1:1 translation from CPUAPI cpu mode
 *       to SoftSDV cpu mode.
 */
#define MAX_NUMBER_OF_CPU_MODE_IN_ARCH  6
typedef enum cpuapi_master_cpu_mode_s {
	CPUAPI_Cpu_Mode_Arch_1,                                                           // used by IA32
	CPUAPI_Cpu_Mode_Arch_2 = CPUAPI_Cpu_Mode_Arch_1 + MAX_NUMBER_OF_CPU_MODE_IN_ARCH, // used by IPF
	CPUAPI_Cpu_Mode_Arch_3 = CPUAPI_Cpu_Mode_Arch_2 + MAX_NUMBER_OF_CPU_MODE_IN_ARCH,
	CPUAPI_Cpu_Mode_Arch_4 = CPUAPI_Cpu_Mode_Arch_3 + MAX_NUMBER_OF_CPU_MODE_IN_ARCH,
	CPUAPI_Cpu_Mode_Arch_5 = CPUAPI_Cpu_Mode_Arch_4 + MAX_NUMBER_OF_CPU_MODE_IN_ARCH,
} cpuapi_master_cpu_mode_t;

typedef struct cpuapi_properties_s {
	char          *name;
	cpuapi_type_t type;
	cpuapi_u32_t  proc_num;
	cpuapi_u32_t  thread_num;
	cpuapi_u32_t  version;
   cpuapi_arch_type_t arch_type;
	char          *capabilities;
} cpuapi_properties_t;


typedef enum {
   CPUAPI_Simul_Mode_First,
	CPUAPI_Simul_Mode_Cold = CPUAPI_Simul_Mode_First,
	CPUAPI_Simul_Mode_Warmup,
	CPUAPI_Simul_Mode_Hot,
	CPUAPI_Simul_Mode_Last = CPUAPI_Simul_Mode_Hot
} cpuapi_simulmode_t;

typedef enum {
	CPUAPI_Scope_Arch_State,
	CPUAPI_Scope_Internal_State,
	CPUAPI_Scope_All,
} cpuapi_scope_t;

typedef enum {
   CPUAPI_Pin_Intr,
   CPUAPI_Pin_Reset,
   CPUAPI_Pin_NMI,
   CPUAPI_Pin_SMI,
   CPUAPI_Pin_A20,
   CPUAPI_Pin_Init,
   CPUAPI_Pin_Last = CPUAPI_Pin_Init,
} cpuapi_pin_t;

/* - Delivery modes */
typedef enum {
	CPUAPI_Ext_Intr_Fixed_Delivery_Mode           = 0,
	CPUAPI_Ext_Intr_Lowest_Priority_Delivery_Mode = 1,
	CPUAPI_Ext_Intr_Smi_Delivery_Mode             = 2, /* IA32 only */
	CPUAPI_Ext_Intr_Pmi_Delivery_Mode             = 2, /* IA64 only */
	CPUAPI_Ext_Intr_Reset_Delivery_Mode           = 3, /* IA32 only */
	CPUAPI_Ext_Intr_Nmi_Delivery_Mode             = 4,
	CPUAPI_Ext_Intr_Init_Delivery_Mode            = 5,
	CPUAPI_Ext_Intr_Start_Up_Delivery_Mode        = 6, /* IA32 only */
	CPUAPI_Ext_Intr_Extint_Delivery_Mode          = 7, /* 8259 mode */
} cpu_exintr_delivery_mode_t;

/* - Strap Values */
typedef enum
{
    /* IA32 strap pins */
    CPUAPI_IA32_FIRST_STRAP_PIN = 0,
    CPUAPI_IA32_SMI_strap_pin,
    CPUAPI_IA32_INIT_strap_pin,
    CPUAPI_IA32_A3_strap_pin,
    CPUAPI_IA32_A4_strap_pin,
    CPUAPI_IA32_A5_strap_pin,
    CPUAPI_IA32_A6_strap_pin,
    CPUAPI_IA32_A7_strap_pin,
    CPUAPI_IA32_A8_strap_pin,
    CPUAPI_IA32_A9_strap_pin,
    CPUAPI_IA32_A10_strap_pin,
    CPUAPI_IA32_A11_strap_pin,
    CPUAPI_IA32_A12_strap_pin,
    CPUAPI_IA32_A13_strap_pin,
    CPUAPI_IA32_A14_strap_pin,
    CPUAPI_IA32_A15_strap_pin,
    CPUAPI_IA32_A16_strap_pin,
    CPUAPI_IA32_A17_strap_pin,
    CPUAPI_IA32_A18_strap_pin,
    CPUAPI_IA32_A19_strap_pin,
    CPUAPI_IA32_A20_strap_pin,
    CPUAPI_IA32_A21_strap_pin,
    CPUAPI_IA32_A22_strap_pin,
    CPUAPI_IA32_A23_strap_pin,
    CPUAPI_IA32_A24_strap_pin,
    CPUAPI_IA32_A25_strap_pin,
    CPUAPI_IA32_A26_strap_pin,
    CPUAPI_IA32_A27_strap_pin,
    CPUAPI_IA32_A28_strap_pin,
    CPUAPI_IA32_A29_strap_pin,
    CPUAPI_IA32_A30_strap_pin,
    CPUAPI_IA32_A31_strap_pin,
    CPUAPI_IA32_A32_strap_pin,
    CPUAPI_IA32_A33_strap_pin,
    CPUAPI_IA32_A34_strap_pin,
    CPUAPI_IA32_A35_strap_pin,
    CPUAPI_IA32_A36_strap_pin,
    CPUAPI_IA32_A37_strap_pin,
    CPUAPI_IA32_A38_strap_pin,
    CPUAPI_IA32_A39_strap_pin,
    CPUAPI_IA32_BR0_strap_pin,
    CPUAPI_IA32_BR1_strap_pin,
    CPUAPI_IA32_BR2_strap_pin,
    CPUAPI_IA32_BR3_strap_pin,
    CPUAPI_IA32_A20M_strap_pin,
    CPUAPI_IA32_IGNNE_strap_pin,
    CPUAPI_IA32_INTR_strap_pin,
    CPUAPI_IA32_NMI_strap_pin,
    CPUAPI_IA32_LAST_STRAP_PIN  = 999,
    
    /* IPF strap pins */
    CPUAPI_IPF_FIRST_STRAP_PIN  = 1000,
    CPUAPI_IPF_LAST_STRAP_PIN   = 1999,
} cpuapi_strap_pins_t;

/* - Destination modes */
typedef enum {
	CPUAPI_Ext_Intr_Physical_Destination_Mode = 0,
	CPUAPI_Ext_Intr_Logical_Destination_Mode
} cpu_exintr_destination_mode_t;

/* - Level  */
typedef enum {
	CPUAPI_Ext_Intr_Level_Deassert= 0,
	CPUAPI_Ext_Intr_Level_Assert
} cpu_exintr_level_t;

/* - Trigger mode */
typedef enum {
	CPUAPI_Ext_Intr_Trigger_Edge = 0,
	CPUAPI_Ext_Intr_Trigger_Level
} cpu_exintr_trigger_t;

typedef struct cpuapi_exint_msg_s {
	cpuapi_cpunum_t destination;
	cpu_exintr_destination_mode_t destination_mode;
	cpu_exintr_delivery_mode_t delivery_mode;
	cpuapi_u32_t vector;
	cpu_exintr_level_t level;
	cpu_exintr_trigger_t trigger_mode;
} cpuapi_exintr_msg_t;

typedef enum
{
	CPUAPI_Step_Instructions,
	CPUAPI_Step_Cycles,
	CPUAPI_Step_Time
} cpuapi_step_t;


typedef enum {
	CPUAPI_Brk_Type_Fetch       = 0x1,
	CPUAPI_Brk_Type_Read        = 0x2,
	CPUAPI_Brk_Type_Write       = 0x4,
	CPUAPI_Brk_Type_Single_Step = 0x8,
	CPUAPI_Brk_Type_Event       = 0x10,
   CPUAPI_Brk_Type_Select_Cpu  = 0x12
} cpuapi_brk_type_t;

typedef enum {
	CPUAPI_Brk_Flavor_Phys_Addr,
	CPUAPI_Brk_Flavor_Virt_Addr,
	CPUAPI_Brk_Flavor_Step_Uops,
	CPUAPI_Brk_Flavor_Step_LIT,
	CPUAPI_Brk_Flavor_Step_Inst,
} cpuapi_brk_flavor_t;

/* When using IO breakpoint with addr=CPUAPI_All_Ports
   It means that the breakpoint is for all ports.
   Otherwaise the port is passed in the addr*/

#define CPUAPI_All_Ports -1

typedef enum {
	CPUAPI_Brk_Event_Exception,
	CPUAPI_Brk_Event_Interrupt,
	CPUAPI_Brk_Event_IO_Read,
	CPUAPI_Brk_Event_IO_Write,	
	CPUAPI_Brk_Event_MSR,
} cpuapi_brk_event_t;

typedef struct cpuapi_brk_params_s {
	cpuapi_brk_type_t   type;
	cpuapi_brk_flavor_t flavor;
	cpuapi_u64_t        step_val;
	cpuapi_u32_t 	    size;
	cpuapi_phys_addr_t  addr;
	cpuapi_brk_event_t  event;
	cpuapi_brk_arg_t    arg;
   cpuapi_u32_t        selected_cpu_num;
} cpuapi_brk_params_t;

typedef enum  {
	CPUAPI_Save_On_Fetch,
	CPUAPI_Save_On_Retirement,
	CPUAPI_Save_On_Read,
	CPUAPI_Save_On_Write,
} cpuapi_elem_sync_mode_t;

typedef enum {
	CPUAPI_No_Forward,
	CPUAPI_Forward
} cpuapi_fwd_type_t;

typedef enum  {
	CPUAPI_Msg_Debug,
	CPUAPI_Msg_Warning,
	CPUAPI_Msg_Info,
	CPUAPI_Msg_System_Halt
} cpuapi_msg_class_t;

typedef enum {
	CPUAPI_Err_Config_Error,
	CPUAPI_Err_Bad_Arg,
	CPUAPI_Err_Fatal
} cpuapi_err_class_t;

typedef enum vis_config_s {
   CPUAPI_Visibility_Invalid_Config   = 0x0,
   CPUAPI_Visibility_First_Config     = 0x1,
	CPUAPI_Visibility_Logical_View     = 0x1,   // Support rotation, etc.
	CPUAPI_Visibility_Physical_View    = 0x2,   // Default
	CPUAPI_Visibility_No_Side_Effect   = 0x4,   // Just change the value
	CPUAPI_Visibility_With_Side_Effect = 0x8,   // Default
   CPUAPI_Visibility_Last_Config      = 0x8,
	CPUAPI_Visibility_Default_Config   = CPUAPI_Visibility_Physical_View | 
                                     CPUAPI_Visibility_With_Side_Effect
} cpuapi_vis_config_t;


#ifdef __cplusplus
}
#endif

#endif /* CPUAPI_TYPES_H */
