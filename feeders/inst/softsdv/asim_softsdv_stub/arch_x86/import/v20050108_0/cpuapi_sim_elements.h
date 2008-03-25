/*****************************************************************************
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
 * 
 * 
 * File:  cpuapi_sim_elements.h
 * 
 * Description:    
 *         Definition of Simulation elements (name, size).
 *         These elements are architecture independent.
 *         They can be used by any/all Cpu model.
 * 
 *****************************************************************************/
#ifndef __CPUAPI_SIM_ELEMENTS_H__
#define __CPUAPI_SIM_ELEMENTS_H__

// Accurate instruction count
#define CPUAPI_SIM_LITCOUNT                      "cpuapi.sim.litcount"
#define CPUAPI_SIM_LITCOUNT_SIZE                 sizeof(cpuapi_u64_t)

// Processor mode (I.e. PM16, PM32, Real...)
#define CPUAPI_SIM_CPU_MODE                      "cpuapi.sim.cpu_mode"
#define CPUAPI_SIM_CPU_MODE_SIZE                 sizeof(cpuapi_u32_t)

// Instruction info
#define CPUAPI_SIM_INST_INSTRUCTION_INFO         "cpuapi.sim.inst.info.instruction"
#define CPUAPI_SIM_INST_INSTRUCTION_INFO_SIZE    sizeof(cpuapi_inst_info_t)

// Knob to start \ stop collection of memory values in the instruction info 
#define CPUAPI_SIM_COLLECT_MEM_VALUES_INFO       "cpuapi.sim.inst.info.collect_mem_values"
#define CPUAPI_SIM_COLLECT_MEM_VALUES_INFO_SIZE  sizeof(cpuapi_u32_t)


// Performance model frequency
#define CPUAPI_SIM_PERF_FREQ                     "cpuapi.sim.perf.frequency"
#define CPUAPI_SIM_PERF_FREQ_SIZE                sizeof(cpuapi_perf_freq_t)

// Nano per inst
#define CPUAPI_SIM_NANOS_PER_INST                "cpuapi.sim.nanos_per_inst"
#define CPUAPI_SIM_NANOS_PER_INST_SIZE           sizeof(double)

// Instruction Pointer - linear value
#define CPUAPI_SIM_LINEAR_XIP                    "cpuapi.sim.xip.linear"
#define CPUAPI_SIM_LINEAR_XIP_SIZE               sizeof(cpuapi_u64_t)

// A bit is set for every alive CPU
#define CPUAPI_SIM_ALIVE_CPUS                    "cpuapi.sim.alive.cpus"
#define CPUAPI_SIM_ALIVE_CPUS_SIZE               sizeof(cpuapi_u32_t)

// Visibility config
#define CPUAPI_SIM_VISIBILITY_CONFIG             "cpuapi.sim.visibility_config"
#define CPUAPI_SIM_VISIBILITY_CONFIG_SIZE        sizeof(cpuapi_vis_config_t)



#endif // __CPUAPI_SIM_ELEMENTS_H__
