/*
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
 */

#ifndef CPUAPI_H
#define CPUAPI_H

#include <cpuapi_mem.h>
#include <cpuapi_sim_elements.h>
#include <cpuapi_types.h>

#define CPUAPI_VERSION_1    0x00000002
#define CPUAPI_VERSION_2_0  0x00020000
#define CPUAPI_VERSION_3_0  0x00030000
#define CPUAPI_VERSION_3_1  0x00030001
#define CPUAPI_VERSION_4_0  0x00040000
#define CPUAPI_VERSION_5_0  0x00050000
#define CPUAPI_VERSION_6_0  0x00060000
#define CPUAPI_VERSION_7_0  0x00070000
#define CPUAPI_VERSION_8_0  0x00080000
#define CPUAPI_VERSION_9_0  0x00090000
#define CPUAPI_VERSION_9_1  0x00090001

#define CPUAPI_VERSION_CURRENT CPUAPI_VERSION_9_1
#define CPUAPI_VERSION_MAJOR(ver) (ver>>16)
#define CPUAPI_VERSION_MINOR(ver) (ver&0xffff)

#define CPUAPI_MAX_CPUS_NUM 256

#ifdef EXTERNAL_USERS
	#define VMX_OFF
	#define SMX_OFF
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    struct cpuapi_spec_controller_callbacks_s;
    struct cpuapi_spec_cpu_callbacks_s;
    struct cpuapi_aom_controller_callbacks_s;

    typedef struct cpuapi_cpu_callbacks_s {
        /* Initialization and termination API */
        cpuapi_stat_t  (*initialize)           (cpuapi_cid_t cid, cpuapi_phase_t phase);
        cpuapi_stat_t  (*activate)             (cpuapi_cid_t cid, cpuapi_simulmode_t mode,cpuapi_type_t model_type);
        cpuapi_stat_t  (*deactivate)           (cpuapi_cid_t cid);
        cpuapi_stat_t  (*set_simulation_mode)  (cpuapi_cid_t cid, cpuapi_simulmode_t mode);
        cpuapi_stat_t  (*terminate)            (cpuapi_cid_t cid, cpuapi_phase_t phase);


        /* Fast memory access API */
        cpuapi_stat_t  (*invalidate_hptr)      (cpuapi_cid_t cid, cpuapi_phys_addr_t paddr, cpuapi_size_t size);
        void           (*notify_memory_write)  (cpuapi_cid_t cid, cpuapi_phys_addr_t paddr, cpuapi_size_t size, cpuapi_boolean_t is_dma);

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
        cpuapi_stat_t  (*brk_set)              (cpuapi_cid_t cid, cpuapi_brk_params_t *params, cpuapi_brk_arg_t cpuc_handle);
        cpuapi_stat_t  (*brk_delete)           (cpuapi_cid_t cid, cpuapi_brk_params_t *params, cpuapi_brk_arg_t cpuc_handle);

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
        cpuapi_stat_t  (*set_object_owner)     (cpuapi_cid_t cid, cpuapi_handle_t handle, cpuapi_size_t size, const void *value);
        cpuapi_stat_t  (*release_object_owner) (cpuapi_cid_t cid, cpuapi_handle_t handle, cpuapi_size_t size, void *value);

        /* Extensibility API */
        cpuapi_stat_t  (*command)              (cpuapi_cid_t cid, int argc, char *argv[], void *inbuf, void *outbuf, cpuapi_size_t outbuf_size);

        /* Performance model API*/
        cpuapi_stat_t  (*get_trans_info)       (cpuapi_cid_t cid, cpuapi_trans_t trans, cpuapi_access_t **access, cpuapi_access_type_t *type,
                                                cpuapi_space_t *space, cpuapi_phys_addr_t *paddr, cpuapi_size_t *size,cpuapi_size_t *total_requested_chunks);
        cpuapi_stat_t  (*snoop_query)          (cpuapi_cid_t cid, cpuapi_trans_t trans, cpuapi_access_type_t type, cpuapi_phys_addr_t paddr, cpuapi_size_t size);
        cpuapi_stat_t  (*bus_signal)           (cpuapi_cid_t cid, cpuapi_trans_t trans, cpuapi_signal_t signal_type);
        cpuapi_stat_t  (*read_data_phase)      (cpuapi_cid_t cid, cpuapi_trans_t trans, cpuapi_u32_t chunk_id, cpuapi_size_t size, void *buf);
        cpuapi_stat_t  (*get_isis_session_id)  (cpuapi_cid_t cid,  void *isis_session_id);
        cpuapi_stat_t  (*notification_request) (cpuapi_cid_t cid, cpuapi_handle_t event_handle, cpuapi_boolean_t on_off);

        /* PLUG/UNPLUG CPU interface */
        cpuapi_stat_t  (*plug_cpu)             (cpuapi_cid_t cid);
        cpuapi_stat_t  (*unplug_cpu)           (cpuapi_cid_t cid);
        
        /* Architecture Specific structure */
        void* architecture_specific_api;
        /*Speculation API's*/
        struct cpuapi_spec_cpu_callbacks_s* speculative_execution_api;

    } cpuapi_cpu_callbacks_t;

/* The master CPU gets notifications through an API of this form: */
    typedef cpuapi_stat_t (*cpuapi_notification_callback_t) (cpuapi_cid_t cid, char *elem, cpuapi_handle_t handle, cpuapi_size_t size, void *value);

    typedef struct cpuapi_controller_callbacks_s {
        /* Initialization and termination API */
        cpuapi_stat_t  (*cpu_register)         (cpuapi_cid_t cid, cpuapi_properties_t *cpu_prop, cpuapi_cpu_callbacks_t *cbk, cpuapi_pid_t *pid);
        cpuapi_stat_t  (*cpu_unregister)       (cpuapi_pid_t pid);

        /* Fast memory access API */
        cpuapi_stat_t  (*get_hptr)             (cpuapi_pid_t pid, cpuapi_access_type_t type, cpuapi_phys_addr_t paddr, cpuapi_size_t *size, cpuapi_hptr_t *buf);
        cpuapi_stat_t  (*invalidate_hptr)      (cpuapi_pid_t pid, cpuapi_phys_addr_t paddr);

        /* Event notification API */
        cpuapi_stat_t  (*brk_hit)              (cpuapi_pid_t pid, cpuapi_brk_arg_t cpuc_handle, const cpuapi_brk_hit_info_t *bp_hit_info);
        cpuapi_stat_t  (*request_stop_asap)    (cpuapi_pid_t pid);

        /* Architectural API */
        cpuapi_stat_t  (*pin_ack)              (cpuapi_pid_t pid, cpuapi_pin_t pin, cpuapi_inst_t inst);
        cpuapi_stat_t  (*external_interrupt_message_broadcast)(cpuapi_pid_t pid, cpuapi_inst_t icount, cpuapi_exintr_msg_t *msg);
        cpuapi_stat_t  (*inter_processor_interrupt_message)(cpuapi_pid_t pid, cpuapi_inst_t icount, cpuapi_exintr_msg_t *msg);
        cpuapi_stat_t  (*inst_retire)          (cpuapi_pid_t pid, cpuapi_inst_t icount);
        cpuapi_stat_t  (*inst_fetch)           (cpuapi_pid_t pid, cpuapi_inst_t icount);
        cpuapi_stat_t  (*functional_access)    (cpuapi_pid_t pid, cpuapi_inst_t icount, cpuapi_access_type_t type, cpuapi_space_t space, cpuapi_phys_addr_t paddr, cpuapi_size_t size, void *buf, cpuapi_boolean_t unordered);
        cpuapi_stat_t  (*get_strap_pin_value)  (cpuapi_pid_t pid, cpuapi_strap_pins_t strap_pin, cpuapi_u32_t *strap_pin_value);

        /* Master_slave sync API */
        cpuapi_stat_t  (*element_request)      (cpuapi_pid_t pid, cpuapi_elem_sync_mode_t mode, char *elem);
        cpuapi_stat_t  (*element_access_notify)(cpuapi_pid_t pid, cpuapi_inst_t icount, cpuapi_elem_sync_mode_t mode, char *elem, cpuapi_size_t size, void *value);
        cpuapi_stat_t  (*step_reference)       (cpuapi_pid_t pid, cpuapi_inst_info_t *inst_info, cpuapi_inst_t requested_inst);
        cpuapi_stat_t  (*step_slaves)          (cpuapi_pid_t pid, cpuapi_inst_t icount);

        /* Visibility API's*/
        cpuapi_stat_t  (*set_object)           (cpuapi_pid_t pid, cpuapi_handle_t handle, cpuapi_size_t size, const void *value);
        cpuapi_stat_t  (*set_object_by_name)   (cpuapi_pid_t pid, const char *name, cpuapi_size_t size, const void *value);
        cpuapi_stat_t  (*get_object_by_name)   (cpuapi_pid_t pid, cpuapi_inst_t icount, char *elem, cpuapi_size_t size, void *value);
        cpuapi_stat_t  (*get_object)           (cpuapi_pid_t pid, cpuapi_inst_t icount, cpuapi_handle_t handle, cpuapi_size_t size, void *value);
        cpuapi_stat_t  (*translate_object_name)(cpuapi_pid_t pid, const char* name, cpuapi_handle_t *handle);
        cpuapi_stat_t  (*register_as_owner)    (cpuapi_pid_t pid, char const *name, cpuapi_size_t size, void *value);
        cpuapi_stat_t  (*unregister_as_owner)    (cpuapi_pid_t pid, char const *name, cpuapi_size_t size, const void *value);

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

        /*Speculation API's*/
        struct cpuapi_spec_controller_callbacks_s* speculative_execution_api;

        cpuapi_stat_t  (*get_topology)         (cpuapi_pid_t pid, cpuapi_topology_t *topology);
		/* Event API */
		cpuapi_stat_t  (*event_notification) (cpuapi_pid_t pid, cpuapi_handle_t event_handle, cpuapi_event_notification_t *event_data);

        unsigned       (*create_safe_thread)   (void* thread_proc, void* thread_data, unsigned affinity_mask);
        void           (*exit_safe_thread)     (int param);        
        int            (*is_safe_thread)       (void);
        
        /* AOM API */
        struct cpuapi_aom_controller_callbacks_s* aom;

    } cpuapi_controller_callbacks_t;


/* The system loads a CPU module through an API of this form: */

    typedef cpuapi_stat_t (*cpuapi_load_module_t) (cpuapi_u32_t version, cpuapi_controller_callbacks_t *cbk, int argc, const char *argv[]);

#ifdef __cplusplus
}
#endif

#endif /* CPUAPI_H */
