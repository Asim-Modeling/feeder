/*****************************************************************************
 * Copyright (C) 2003-2006 Intel Corporation
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
 * File:  asim_interface.h
 * 
 * Description: These are the functions that are implemented above the cpuapi
 * that connect asim to softsdv
 * 
 *****************************************************************************/

extern "C"
{
/**********************************************************************
 *!!!!!ALL ASIM_ FUNCTIONS RETURN TRUE ON SUCCESS AND FALSE ON FAILURE
 ***********************************************************************/

//Get the common handles so that we are not always executing this code
bool asim_init_handles(cpuapi_u32_t cpuNum);
bool asim_set_verbose(bool ON);

/***********************************************************************
 *These functions get the values for common elements
 *where the handles were already decoded to speed up simulation
 **********************************************************************/
bool asim_get_cfm(cpuapi_u32_t cpuNum, UINT64* value);
bool asim_get_preds(cpuapi_u32_t cpuNum, UINT64* value);
bool asim_get_pc(cpuapi_u32_t cpuNum, UINT64* value);
bool asim_get_bsp(cpuapi_u32_t cpuNum, UINT64* value);
bool asim_get_cpu_mode(cpuapi_u32_t cpuNum, UINT64* value);
bool asim_get_sim_inst_info(cpuapi_u32_t cpuNum, cpuapi_inst_info_t* value);
bool asim_get_xip(cpuapi_u32_t cpuNum, UINT64* value);
bool asim_get_alive_cpus(cpuapi_u32_t cpuNum, UINT64* value);
bool asim_get_sim_litcount(cpuapi_u32_t cpuNum, UINT64* value);


/*******************************************************************************
 *Objects to get by name (registers)
 *******************************************************************************/
bool asim_get_gr(cpuapi_u32_t cpuNum, UINT32 num, UINT64* value);
bool asim_get_cr(cpuapi_u32_t cpuNum, UINT32 num, UINT64* value);
bool asim_get_nat(cpuapi_u32_t cpuNum, UINT32 num, UINT64* value);
bool asim_get_fr(cpuapi_u32_t cpuNum, UINT32 num, UINT64* value);
bool asim_get_br(cpuapi_u32_t cpuNum, UINT32 num, UINT64* value);
bool asim_get_ar(cpuapi_u32_t cpuNum, UINT32 num, UINT64* value);
bool asim_get_rr(cpuapi_u32_t cpuNum, UINT32 num, UINT64* value);
bool asim_get_dbr(cpuapi_u32_t cpuNum, UINT32 num, UINT64* value);
bool asim_get_ibr(cpuapi_u32_t cpuNum, UINT32 num, UINT64* value);
bool asim_v2p(cpuapi_u32_t cpuNum,UINT64 vaddr, UINT64* paddr );

/***************************************************************
 *Step the cpu by so many instructions
 ***************************************************************/
bool asim_step_ref(cpuapi_u32_t cpuNum,
                       cpuapi_inst_info_t *inst_info,
                   cpuapi_inst_t nInstructions);


bool asim_mem_read(cpuapi_u32_t cpuNum,UINT64 paddr,cpuapi_size_t size,void *buf);

}

