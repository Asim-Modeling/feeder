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

#ifndef CPUAPI_H
#define CPUAPI_H

#include "cpuapi_mem.h"
#include "cpuapi_sim_elements.h"

#define CPUAPI_VERSION_1    0x00000002
#define CPUAPI_VERSION_2_0  0x00020000
#define CPUAPI_VERSION_CURRENT CPUAPI_VERSION_2_0
#define CPUAPI_VERSION_MAJOR(ver) (ver>>16)
#define CPUAPI_VERSION_MINOR(ver) (ver&0xffff)

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
   cpuapi_u32_t        size;
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

typedef struct cpuapi_cpu_callbacks_s {
   /* Initialization and termination API */
   cpuapi_stat_t  (*initialize)           (cpuapi_cid_t cid, cpuapi_phase_t phase);
   cpuapi_stat_t  (*activate)             (cpuapi_cid_t cid, cpuapi_simulmode_t mode,cpuapi_type_t model_type);
   cpuapi_stat_t  (*deactivate)           (cpuapi_cid_t cid);
   cpuapi_stat_t  (*set_simulation_mode)  (cpuapi_cid_t cid, cpuapi_simulmode_t mode);
   cpuapi_stat_t  (*terminate)            (cpuapi_cid_t cid, cpuapi_phase_t phase);


   /* Fast memory access API */
   cpuapi_stat_t  (*invalidate_hptr)      (cpuapi_cid_t cid, cpuapi_phys_addr_t paddr, cpuapi_size_t size);
   void           (*notify_memory_write)  (cpuapi_cid_t cid, cpuapi_phys_addr_t paddr, cpuapi_size_t size);

   /* Save-restore API */
   cpuapi_stat_t  (*save)                 (cpuapi_cid_t cid, cpuapi_scope_t scope, char *basename);
   cpuapi_stat_t  (*restore)              (cpuapi_cid_t cid, cpuapi_scope_t scope, char *basename);

   /* Architectural API */
   /* PIC interface */
   cpuapi_stat_t  (*pin_assert)           (cpuapi_cid_t cid, cpuapi_pin_t pin);
   cpuapi_stat_t  (*pin_deassert)         (cpuapi_cid_t cid, cpuapi_pin_t pin);
   cpuapi_stat_t  (*pin_data)             (cpuapi_cid_t cid, cpuapi_pin_t pin, cpuapi_inst_t icount, cpuapi_u32_t vector);
   /* APIC interface */
   cpuapi_stat_t  (*external_interrupt_message) (cpuapi_cid_t cid, cpuapi_inst_t icount, cpuapi_exintr_msg_t *msg);
   cpuapi_stat_t  (*inter_processor_interrupt_message)(cpuapi_cid_t cid, cpuapi_inst_t icount, cpuapi_exintr_msg_t *msg);

   /* Execution API */
   cpuapi_stat_t  (*execute)              (cpuapi_cid_t cid, cpuapi_step_t step_type, cpuapi_u64_t requested, cpuapi_u64_t *actual);
   cpuapi_stat_t  (*stop_asap)            (cpuapi_cid_t cid);
   cpuapi_stat_t  (*abort_execution)      (cpuapi_cid_t cid);

   /* Breakpoint API */
   cpuapi_stat_t  (*brk_set)              (cpuapi_cid_t cid, cpuapi_brk_params_t *params, cpuapi_brk_arg_t *c_arg);
   cpuapi_stat_t  (*brk_delete)           (cpuapi_cid_t cid, cpuapi_brk_arg_t c_arg);

   /* Visibility API */
   cpuapi_stat_t  (*visibility_config)    (cpuapi_cid_t cid, cpuapi_vis_config_t config);
   int            (*list_objects)         (cpuapi_cid_t cid, char *key, char *names[]);
   cpuapi_stat_t  (*set_object)           (cpuapi_cid_t cid, cpuapi_handle_t handle, cpuapi_size_t size, const void *value);
   cpuapi_stat_t  (*get_object)           (cpuapi_cid_t cid, cpuapi_handle_t handle, cpuapi_size_t *size, void *value);
   cpuapi_stat_t  (*set_object_by_name)   (cpuapi_cid_t cid, const char *name, cpuapi_size_t size, const void *value);
   cpuapi_stat_t  (*get_object_by_name)   (cpuapi_cid_t cid, const char *name, cpuapi_size_t *size, void *value);
   cpuapi_stat_t  (*translate_object_name)(cpuapi_cid_t cid, const char *name, cpuapi_handle_t *handle);
   cpuapi_stat_t  (*translate_object_handle)(cpuapi_cid_t cid, cpuapi_handle_t handle, char **name);
   cpuapi_stat_t  (*translate_virt_addr)  (cpuapi_cid_t cid, cpuapi_virt_addr_t vaddr, cpuapi_access_type_t access, cpuapi_phys_addr_t *paddr);

   /* Master_slave sync API */
   cpuapi_stat_t  (*notification_request) (cpuapi_cid_t cid, cpuapi_elem_sync_mode_t mode, char *elem);

   /* Extensibility API */
   cpuapi_stat_t  (*command)              (cpuapi_cid_t cid, int argc, char *argv[], void *inbuf, void *outbuf, cpuapi_size_t outbuf_size);

   /* Performance model API*/
   cpuapi_stat_t  (*get_trans_info)       (cpuapi_cid_t cid, cpuapi_trans_t trans, cpuapi_access_t **access, cpuapi_access_type_t *type,
                                           cpuapi_space_t *space, cpuapi_phys_addr_t *paddr, cpuapi_size_t *size,cpuapi_size_t *total_requested_chunks);
   cpuapi_stat_t  (*snoop_query)          (cpuapi_cid_t cid, cpuapi_trans_t trans, cpuapi_access_type_t type, cpuapi_phys_addr_t paddr, cpuapi_size_t size);
   cpuapi_stat_t  (*bus_signal)           (cpuapi_cid_t cid, cpuapi_trans_t trans, cpuapi_signal_t signal_type);
   cpuapi_stat_t  (*read_data_phase)      (cpuapi_cid_t cid, cpuapi_trans_t trans, cpuapi_u32_t chunk_id, cpuapi_size_t size, void *buf);
   cpuapi_stat_t  (*get_isis_session_id)  (cpuapi_cid_t cid,  void *isis_session_id);

   /* Architecture Specific structure */
   void* architecture_specific_api;

} cpuapi_cpu_callbacks_t;

/* The master CPU gets notifications through an API of this form: */
typedef cpuapi_stat_t (*cpuapi_notification_callback_t) (cpuapi_cid_t cid, char *elem, cpuapi_handle_t handle, cpuapi_size_t size, void *value);

typedef struct cpuapi_controller_callbacks_s {
   /* Initialization and termination API */
   cpuapi_stat_t  (*cpu_register)         (cpuapi_cid_t cid, cpuapi_properties_t *cpu_prop, cpuapi_cpu_callbacks_t *cbk, cpuapi_pid_t *pid);
   cpuapi_stat_t  (*cpu_unregister)       (cpuapi_pid_t pid);

   /* Fast memory access API */
   cpuapi_stat_t  (*get_hptr)             (cpuapi_pid_t pid, cpuapi_access_type_t type, cpuapi_phys_addr_t paddr, cpuapi_size_t *size, cpuapi_hptr_t *buf);

   /* Event notification API */
   cpuapi_stat_t  (*brk_hit)              (cpuapi_pid_t pid, cpuapi_brk_params_t *params);
    cpuapi_stat_t (*request_stop_asap)    (cpuapi_pid_t pid);

   /* Architectural API */
   cpuapi_stat_t  (*pin_ack)              (cpuapi_pid_t pid, cpuapi_pin_t pin, cpuapi_inst_t inst);
   cpuapi_stat_t  (*external_interrupt_message_broadcast)(cpuapi_pid_t pid, cpuapi_inst_t icount, cpuapi_exintr_msg_t *msg);
   cpuapi_stat_t  (*inter_processor_interrupt_message)(cpuapi_pid_t pid, cpuapi_inst_t icount, cpuapi_exintr_msg_t *msg);
   cpuapi_stat_t  (*inst_retire)          (cpuapi_pid_t pid, cpuapi_inst_t icount);
   cpuapi_stat_t  (*inst_fetch)           (cpuapi_pid_t pid, cpuapi_inst_t icount);
   cpuapi_stat_t  (*functional_access)    (cpuapi_pid_t pid, cpuapi_inst_t icount, cpuapi_access_type_t type, cpuapi_space_t space, cpuapi_phys_addr_t paddr, cpuapi_size_t size, void *buf);
   cpuapi_stat_t  (*get_strap_pin_value)  (cpuapi_pid_t pid, cpuapi_strap_pins_t strap_pin, cpuapi_u32_t *strap_pin_value);

   /* AOM API */
   cpuapi_access_t* (*access_new)         (cpuapi_pid_t pid, cpuapi_inst_t icount, cpuapi_access_type_t type, cpuapi_space_t space, cpuapi_phys_addr_t paddr, cpuapi_size_t size, cpuapi_fwd_type_t fwd);
   cpuapi_stat_t  (*access_done)          (cpuapi_pid_t pid, cpuapi_access_t *access);
   cpuapi_stat_t  (*access_ref)           (cpuapi_pid_t pid, cpuapi_access_t *access);
   cpuapi_stat_t  (*access_deref)         (cpuapi_pid_t pid, cpuapi_access_t *access);
   cpuapi_stat_t  (*access_observe)       (cpuapi_pid_t pid, cpuapi_access_t *access);
   cpuapi_stat_t  (*access_perform)       (cpuapi_pid_t pid, cpuapi_access_t *access);
   cpuapi_stat_t  (*access_combine)       (cpuapi_pid_t pid, cpuapi_access_t *old_access, cpuapi_access_t *new_access);
   int            (*is_observed)          (cpuapi_pid_t pid, cpuapi_access_t *access);
   int            (*is_performed)         (cpuapi_pid_t pid, cpuapi_access_t *access);
   cpuapi_stat_t  (*get_data)             (cpuapi_pid_t pid, cpuapi_access_t *access, cpuapi_size_t size, void *buf);
   cpuapi_stat_t  (*mem_read)             (cpuapi_pid_t pid, cpuapi_phys_addr_t paddr, cpuapi_size_t size, void *buf);

   /* Master_slave sync API */
   cpuapi_stat_t  (*element_request)      (cpuapi_pid_t pid, cpuapi_elem_sync_mode_t mode, char *elem);
   cpuapi_stat_t  (*element_access_notify)(cpuapi_pid_t pid, cpuapi_inst_t icount, cpuapi_elem_sync_mode_t mode, char *elem, cpuapi_size_t size, void *value);
   cpuapi_stat_t  (*get_object_by_name)   (cpuapi_pid_t pid, cpuapi_inst_t icount, char *elem, cpuapi_size_t size, void *value);
   cpuapi_stat_t  (*get_object)           (cpuapi_pid_t pid, cpuapi_inst_t icount, cpuapi_handle_t handle, cpuapi_size_t size, void *value);
   cpuapi_stat_t  (*translate_object_name)(cpuapi_pid_t pid, const char* name, cpuapi_handle_t *handle);
   cpuapi_stat_t  (*step_reference)       (cpuapi_pid_t pid, cpuapi_inst_info_t *inst_info, cpuapi_inst_t requested_inst);
   cpuapi_stat_t  (*step_slaves)          (cpuapi_pid_t pid, cpuapi_inst_t icount);

   cpuapi_stat_t  (*notification_request) (cpuapi_pid_t pid, char *elem, cpuapi_handle_t *handle, cpuapi_notification_callback_t owner_callback);

   /* Messaging & Error API */
   cpuapi_stat_t  (*message)              (cpuapi_pid_t pid, cpuapi_msg_class_t msg_class, cpuapi_u32_t code, char *reason);
   cpuapi_stat_t  (*error)                (cpuapi_pid_t pid, cpuapi_err_class_t severity, cpuapi_u32_t code, char *reason);

   /* Performance model API */
   cpuapi_stat_t  (*initiate_trans)       (cpuapi_pid_t pid, cpuapi_trans_t trans);
   cpuapi_stat_t  (*initiate_trans_info)  (cpuapi_pid_t pid, cpuapi_trans_t trans, cpuapi_access_t *access, cpuapi_access_type_t type,
                                           cpuapi_space_t space, cpuapi_phys_addr_t paddr, cpuapi_size_t size, cpuapi_size_t total_requested_chunks);

   cpuapi_stat_t  (*snoop_response)       (cpuapi_pid_t pid, cpuapi_trans_t trans, cpuapi_snoop_t resp);
   cpuapi_stat_t  (*write_data_phase)     (cpuapi_pid_t pid, cpuapi_trans_t trans, cpuapi_u32_t chunk_id, cpuapi_size_t size, void *buf);

   /* DEBUG: Int emulation API*/
   cpuapi_stat_t  (*emulate_int)          (cpuapi_pid_t pid, cpuapi_u32_t software_int_num);

   /* Visibility*/
   cpuapi_stat_t  (*translate_virt_addr)  (cpuapi_pid_t pid, cpuapi_virt_addr_t vaddr, cpuapi_access_type_t access, cpuapi_phys_addr_t *paddr);

   cpuapi_stat_t  (*set_object)           (cpuapi_pid_t pid, cpuapi_handle_t handle, cpuapi_size_t size, const void *value);
   cpuapi_stat_t  (*set_object_by_name)   (cpuapi_pid_t pid, const char *name, cpuapi_size_t size, const void *value);
} cpuapi_controller_callbacks_t;


/* The system loads a CPU module through an API of this form: */

typedef cpuapi_stat_t (*cpuapi_load_module_t) (cpuapi_u32_t version, cpuapi_controller_callbacks_t *cbk, int argc, const char *argv[]);

#ifdef __cplusplus
}
#endif

#endif /* CPUAPI_H */
