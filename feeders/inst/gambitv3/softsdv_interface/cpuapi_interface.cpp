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
 * File:  cpuapi_interface.cpp
 * 
 * Description: Meaningless CPU model to test CPU-API for IA-64 SoftSDV
 * 
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

// ASIM include files 
#include "asim/provides/softsdv_import_include.h"
#include "asim/provides/softsdv_interface.h"

// SoftSDV include files -- These are now loaded by softsdv_import_include.h
// #include "cpuapi.h"
// #include "cpuapi_arch_ipf.h"
// #include "cpuapi_sim_elements.h"
// #include "cpuc_odb.h"

#ifdef WIN32
#define EXPORT_FUNC __declspec( dllexport )
#else
#define EXPORT_FUNC
#endif

#ifndef _SYNTAX_
/* Asim syntax definitions */
typedef char                INT8;
typedef unsigned char       UINT8;
typedef short               INT16;
typedef unsigned short      UINT16;
typedef int                 INT32;
typedef unsigned int        UINT32;
typedef long long           INT64;
typedef unsigned long long  UINT64;
#endif

#define CAPABILITY_STR_LEN 100

//make globals for the commonly accessed handles
cpuapi_handle_t CFM_handles[MAX_CPU_DEVICES];
cpuapi_handle_t PRED_handles[MAX_CPU_DEVICES];
cpuapi_handle_t PC_handles[MAX_CPU_DEVICES];
cpuapi_handle_t BSP_handles[MAX_CPU_DEVICES];
cpuapi_handle_t CPU_MODE_handles[MAX_CPU_DEVICES];
cpuapi_handle_t SIM_INST_INFO_handles[MAX_CPU_DEVICES];
cpuapi_handle_t SIM_LINEAR_XIP_handles[MAX_CPU_DEVICES];
cpuapi_handle_t SIM_ALIVE_CPUS_handles[MAX_CPU_DEVICES];
cpuapi_handle_t CPUAPI_SIM_LITCOUNT_handles[MAX_CPU_DEVICES];

bool CPU_HANDLE_INIT[MAX_CPU_DEVICES];

//what mode the model is in
cpuapi_simulmode_t current_mode;

//
// Simulation complete?
//
static bool asim_simulation_done = false;

//
// Print debugging messages?
//
static bool asim_verbose = false;


static cpuapi_pid_t controllerPids[MAX_CPU_DEVICES];
static cpuapi_controller_callbacks_t *cpucCallBack;
int argc;
char **argv;

static cpuapi_u64_t totalInstrs = 0;

enum asim_vis_arch
{
   PERF_FREQ
};

//Get the common handles so that we are not always executing this code
bool asim_set_verbose(bool ON)
{
    if(ON)
    {
        asim_verbose=1;
    }
    else
    {
        asim_verbose=0;
    }
    return true;

}


//Get the common handles so that we are not always executing this code
bool asim_init_handles(cpuapi_u32_t cpuNum)
{

    if (asim_verbose)
    {
        fprintf(stderr, "Initializing the handles for cpunumber %d\n",(int)cpuNum);
    }
    //CFM
    if(CPUAPI_Stat_Ok == cpucCallBack->translate_object_name(controllerPids[cpuNum],CPUAPI_IPF_CFM,&CFM_handles[cpuNum]))
    {
        ;
    }
    else
    {
        return false;
    }
    //PREDS
    if(CPUAPI_Stat_Ok == cpucCallBack->translate_object_name(controllerPids[cpuNum],CPUAPI_IPF_PR , &PRED_handles[cpuNum]))
    {
        ;
    }
    else
    {
        return false;
    }
    //PC
    if(CPUAPI_Stat_Ok == cpucCallBack->translate_object_name(controllerPids[cpuNum],CPUAPI_IPF_IP , &PC_handles[cpuNum]))
    {
        ;
    }
    else
    {
        return false;
    }
    //BSP
    if(CPUAPI_Stat_Ok == cpucCallBack->translate_object_name(controllerPids[cpuNum],"arch.ipf.register.ar.17" , &BSP_handles[cpuNum]))
    {
        ;
    }
    else
    {
        return false;
    }

    //CPU MODE
    if(CPUAPI_Stat_Ok == cpucCallBack->translate_object_name(controllerPids[cpuNum],CPUAPI_SIM_CPU_MODE , &CPU_MODE_handles[cpuNum]))
    {
        ;
    }
    else
    {
        return false;
    }
    //SIM_INST_INFO
    if(CPUAPI_Stat_Ok == cpucCallBack->translate_object_name(controllerPids[cpuNum],CPUAPI_SIM_INST_INSTRUCTION_INFO , &SIM_INST_INFO_handles[cpuNum]))
    {
        ;
    }
    else
    {
        return false;
    }
    //SIM_LINEAR_XIP
    if(CPUAPI_Stat_Ok == cpucCallBack->translate_object_name(controllerPids[cpuNum],CPUAPI_SIM_LINEAR_XIP , &SIM_LINEAR_XIP_handles[cpuNum]))
    {
        ;
    }
    else
    {
        return false;
    }
    //SIM_ALIVE_CPUS
    if(CPUAPI_Stat_Ok == cpucCallBack->translate_object_name(controllerPids[cpuNum],CPUAPI_SIM_ALIVE_CPUS ,&SIM_ALIVE_CPUS_handles[cpuNum]))
    {
        ;
    }
    else
    {
        return false;
    }
    //SIM_Instruction Count
    if(CPUAPI_Stat_Ok == cpucCallBack->translate_object_name(controllerPids[cpuNum],CPUAPI_SIM_LITCOUNT ,&CPUAPI_SIM_LITCOUNT_handles[cpuNum]))
    {
        ;
    }
    else
    {
        return false;
    }
    //mark that this cpu had its handles initialized
    CPU_HANDLE_INIT[cpuNum]=true;

    //we were able to get all the different handles
    return true;
       
}

/***********************************************************************
 *These functions get the values for common elements
 *where the handles were already decoded to speed up simulation
 **********************************************************************/
bool asim_get_cfm(cpuapi_u32_t cpuNum, UINT64* value)
{

    if(	CPUAPI_Stat_Ok == cpucCallBack->get_object(controllerPids[cpuNum],0, 
                                                   CFM_handles[cpuNum], CPUAPI_IPF_CFM_SIZE , (void*) value))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool asim_get_preds(cpuapi_u32_t cpuNum, UINT64* value)
{
    if(	CPUAPI_Stat_Ok == cpucCallBack->get_object(controllerPids[cpuNum],0, 
                                                   PRED_handles[cpuNum],CPUAPI_IPF_PR_SIZE , (void*) value))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool asim_get_pc(cpuapi_u32_t cpuNum, UINT64* value)
{
    if(	CPUAPI_Stat_Ok == cpucCallBack->get_object(controllerPids[cpuNum],0, 
                                                   PC_handles[cpuNum],CPUAPI_IPF_IP_SIZE , (void*) value))
    {
        return true;
    }
    else
    {
        return false;
    }
}
bool asim_get_bsp(cpuapi_u32_t cpuNum, UINT64* value)
{
    if( CPUAPI_Stat_Ok == cpucCallBack->get_object(controllerPids[cpuNum],0,
                                                   BSP_handles[cpuNum],8 , (void*) value))
    {
        return true;
    }
    else
    {
        return false;
    }
}


bool asim_get_cpu_mode(cpuapi_u32_t cpuNum, UINT64* value)
{

    *value=0;
    if(	CPUAPI_Stat_Ok == cpucCallBack->get_object(controllerPids[cpuNum],0, 
                                                   CPU_MODE_handles[cpuNum],CPUAPI_SIM_CPU_MODE_SIZE , (void*) value))
    {
        return true;
    }
    else
    {
        return false;
    }

}

bool asim_get_sim_inst_info(cpuapi_u32_t cpuNum, cpuapi_inst_info_t* value)
{
    memset((void*)value,0,sizeof(cpuapi_inst_info_t));
    if(	CPUAPI_Stat_Ok == cpucCallBack->get_object(controllerPids[cpuNum],0, 
                                                   SIM_INST_INFO_handles[cpuNum],
                                                   CPUAPI_SIM_INST_INSTRUCTION_INFO_SIZE, 
                                                   (void*) value))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool asim_get_xip(cpuapi_u32_t cpuNum, UINT64* value)
{
    if(	CPUAPI_Stat_Ok == cpucCallBack->get_object(controllerPids[cpuNum],0, 
                                                   SIM_LINEAR_XIP_handles[cpuNum],CPUAPI_SIM_LINEAR_XIP_SIZE, (void*) value))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool asim_get_alive_cpus(cpuapi_u32_t cpuNum, UINT64* value)
{

    *value=0;
    if(	CPUAPI_Stat_Ok == cpucCallBack->get_object(controllerPids[cpuNum],0 ,
                                                   SIM_ALIVE_CPUS_handles[cpuNum],CPUAPI_SIM_ALIVE_CPUS_SIZE, (void*) value))
    {
        return true;
    }
    else
    {
        return false;
    }
}


bool asim_get_sim_litcount(cpuapi_u32_t cpuNum, UINT64* value)
{
    if(	CPUAPI_Stat_Ok == cpucCallBack->get_object(controllerPids[cpuNum],0 ,
                                                   CPUAPI_SIM_LITCOUNT_handles[cpuNum],CPUAPI_SIM_LITCOUNT_SIZE, (void*) value))
    {
        return true;
    }
    else
    {
        return false;
    }
}

/*******************************************************************************
 *Objects to get by name (registers)
 *******************************************************************************/

bool asim_get_gr(cpuapi_u32_t cpuNum, UINT32 num, UINT64* value)
{
   cpuapi_inst_t icount = 0;   
   char name[100];
   cpuapi_size_t size;
   CPUAPI_IPF_GR_NAME(name,num); 
      
   size = CPUAPI_IPF_GR_SIZE;

   if(CPUAPI_Stat_Ok == cpucCallBack->get_object_by_name(controllerPids[cpuNum],icount,name, size, value))
   {
       return true;
   }
   else
   {
       return false;
   }
}

//cr
bool asim_get_cr(cpuapi_u32_t cpuNum, UINT32 num, UINT64* value)
{
   cpuapi_inst_t icount = 0;   
   char name[100];
   cpuapi_size_t size;
   CPUAPI_IPF_CR_NAME(name,num); 
      
   size = CPUAPI_IPF_CR_SIZE;

   if(CPUAPI_Stat_Ok == cpucCallBack->get_object_by_name(controllerPids[cpuNum],icount,name, size, value))
   {
       return true;
   }
   else
   {
       return false;
   }
}

bool asim_get_nat(cpuapi_u32_t cpuNum, UINT32 num, UINT64* value)
{
   cpuapi_inst_t icount = 0;   
   char name[100];
   cpuapi_size_t size;
   CPUAPI_IPF_NAT_NAME(name,num); 
      
   size = CPUAPI_IPF_NAT_SIZE;
   *value=0;
   if(CPUAPI_Stat_Ok == cpucCallBack->get_object_by_name(controllerPids[cpuNum],icount,name, size, value))
   {
       return true;
   }
   else
   {
       return false;
   }
}

bool asim_get_fr(cpuapi_u32_t cpuNum, UINT32 num, UINT64* value)
{
   cpuapi_inst_t icount = 0;   
   char name[100];
   cpuapi_size_t size;
   CPUAPI_IPF_FR_NAME(name,num); 
      
   size = CPUAPI_IPF_FR_SIZE;
   value[0]=0;
   value[1]=0;

   asim_get_sim_litcount(cpuNum,&icount);
//   printf("Getting %s\n",name);
//   printf("The size is %d\n",size);
//   printf("The cpu number is %d\n",cpuNum);
//   printf("The size of UINT64 os %d\n",sizeof(UINT64));
   if(CPUAPI_Stat_Ok == cpucCallBack->get_object_by_name(controllerPids[cpuNum],icount,name, size, value))
   {
       return true;
   }
   else
   {
       return false;
   }
}

bool asim_get_br(cpuapi_u32_t cpuNum, UINT32 num, UINT64* value)
{
   cpuapi_inst_t icount = 0;   
   char name[100];
   cpuapi_size_t size;
   CPUAPI_IPF_BR_NAME(name,num); 
      
   size = CPUAPI_IPF_BR_SIZE;

   if(CPUAPI_Stat_Ok == cpucCallBack->get_object_by_name(controllerPids[cpuNum],icount,name, size, value))
   {
       return true;
   }
   else
   {
       return false;
   }
}

bool asim_get_ar(cpuapi_u32_t cpuNum, UINT32 num, UINT64* value)
{
   cpuapi_inst_t icount = 0;   
   char name[100];
   cpuapi_size_t size;
   CPUAPI_IPF_AR_NAME(name,num); 
      
   size = CPUAPI_IPF_AR_SIZE;

   if(CPUAPI_Stat_Ok == cpucCallBack->get_object_by_name(controllerPids[cpuNum],icount,name, size, value))
   {
       return true;
   }
   else
   {
       return false;
   }
}

bool asim_get_rr(cpuapi_u32_t cpuNum, UINT32 num, UINT64* value)
{
   cpuapi_inst_t icount = 0;   
   char name[100];
   cpuapi_size_t size;
   CPUAPI_IPF_RR_NAME(name,num); 
      
   size = CPUAPI_IPF_RR_SIZE;

   if(CPUAPI_Stat_Ok == cpucCallBack->get_object_by_name(controllerPids[cpuNum],icount,name, size, value))
   {
       return true;
   }
   else
   {
       return false;
   }
}

bool asim_get_dbr(cpuapi_u32_t cpuNum, UINT32 num, UINT64* value)
{
   cpuapi_inst_t icount = 0;   
   char name[100];
   cpuapi_size_t size;
   CPUAPI_IPF_DBR_NAME(name,num); 
      
   size = CPUAPI_IPF_DBR_SIZE;

   if(CPUAPI_Stat_Ok == cpucCallBack->get_object_by_name(controllerPids[cpuNum],icount,name, size, value))
   {
       return true;
   }
   else
   {
       return false;
   }
}

bool asim_get_ibr(cpuapi_u32_t cpuNum, UINT32 num, UINT64* value)
{
   cpuapi_inst_t icount = 0;   
   char name[100];
   cpuapi_size_t size;
   CPUAPI_IPF_IBR_NAME(name,num); 
      
   size = CPUAPI_IPF_IBR_SIZE;

   if(CPUAPI_Stat_Ok == cpucCallBack->get_object_by_name(controllerPids[cpuNum],icount,name, size, value))
   {
       return true;
   }
   else
   {
       return false;
   }
}

/*******************************************************
 *actions
 *******************************************************/
bool asim_mem_read(cpuapi_u32_t cpuNum,UINT64 paddr,cpuapi_size_t size,void *buf)
{
    
   if(CPUAPI_Stat_Ok == cpucCallBack->mem_read(controllerPids[cpuNum],
                                                          paddr,size,buf))
   {
       return true;
   }
   else
   {
       return false;
   }

}

bool asim_v2p(cpuapi_u32_t cpuNum,UINT64 vaddr, UINT64* paddr )
{
   if(CPUAPI_Stat_Ok == cpucCallBack->translate_virt_addr(controllerPids[cpuNum],
                                                          vaddr,
                                                          CPUAPI_Access_Read,
                                                          paddr))
   {
       return true;
   }
   else
   {
       return false;
   }
}

bool asim_step_ref(cpuapi_u32_t cpuNum,
                       cpuapi_inst_info_t *inst_info,
                       cpuapi_inst_t nInstructions)
{
    if (asim_verbose)
    {
        fprintf(stderr,"ASIM: step_reference\n");
    }

    if (asim_simulation_done)
    {
        return false;
    }

    if(CPUAPI_Stat_Ok == cpucCallBack->step_reference(controllerPids[cpuNum],
                                                      inst_info,
                                                      nInstructions))
    {
        return true;
    }
    else
    {
        return false;
    }

}

/*==============================================================================
Function: step_reference
Description:
	Step the reference Cpu model
Input:
   cpuNum - Cpu number to step
	icount - 
   new_inst - 
Output:
	inst_info - 
Assumptions:
	none
==============================================================================*/
cpuapi_stat_t step_ref(cpuapi_u32_t cpuNum,
                       cpuapi_inst_info_t *inst_info,
                       cpuapi_inst_t nInstructions)
{
//	cpuapi_u64_t tmp;
	cpuapi_stat_t ret; 
	
//	get_gr13_example(cpuNum);
        if (asim_verbose)
        {
           fprintf(stderr,"ASIM: step_reference\n");
        }
	ret = cpucCallBack->step_reference(controllerPids[cpuNum],
                                       inst_info,
                                       nInstructions);
	return ret;
}
/*==============================================================================
Function: step_slaves
Description:
	Step the slave Cpu model
Input:
   cpuNum - Cpu number to step
	icount - Number of instructions to fetch
Output:
	none
Assumptions:
	none
==============================================================================*/
cpuapi_stat_t step_slaves(cpuapi_u32_t cpuNum,
                          cpuapi_inst_t icount)
{
    if (asim_verbose)
    {
        fprintf(stderr,"ASIM: step_slaves\n");
    }
   return cpucCallBack->step_slaves(controllerPids[cpuNum], icount);
}

/*==============================================================================
Function: inst_retire
Description:
	[DETAILS]
Input:
   cpuNum - Cpu number to step
	icount - Number of instruction to retire
Output:
	none
Assumptions:
	none
==============================================================================*/
cpuapi_stat_t inst_ret(cpuapi_u32_t cpuNum,
                       cpuapi_inst_t icount)
{
   return cpucCallBack->inst_retire(controllerPids[cpuNum], icount);
}

/*==============================================================================
Function: inst_fetch
Description:
	[DETAILS]
Input:
   cpuNum - Cpu number to step
	icount - Number of instruction to fetch
Output:
	none
Assumptions:
	none
==============================================================================*/
cpuapi_stat_t inst_fetch(cpuapi_u32_t cpuNum,
                         cpuapi_inst_t icount)
{
   return cpucCallBack->inst_fetch(controllerPids[cpuNum], icount);
}

/*****************************************************************
Cpuapi callback functions
******************************************************************/
/*==============================================================================
Function: initialize
Description:
	[DETAILS]
Input:
	cid - Cpu identifier
   phase - Stage of initialization (defined by Kernel)
Output:
	none
Assumptions:
	none
==============================================================================*/
static cpuapi_stat_t initialize(cpuapi_cid_t cid,
                                cpuapi_phase_t phase)
{   
    char **envp = NULL;
    if (asim_verbose)
    {
        fprintf(stderr,"ASIM: Initialize\n");
    }

    switch (phase)
    {
      case 0:      
      { //init phase 0
          //In this place you should init ASIM model
          //the init is done in two phases.
          //phase 0 is for local module init
          if (asim_verbose)
          {
              fprintf(stderr, "Initialization stage 1 for cpunumber %d\n",(int)cid);
          }
          
          //initialize the globabls to be zero
          for(int i=0;i<MAX_CPU_DEVICES;i++)
          {
              CPU_HANDLE_INIT[i]=false;
          }
         }
      break;
      case 1:
      { //init phase 1            
          //phase 1 is for init of stuff that depeneds in other
          //modules.
          if (asim_verbose)
          {
              fprintf(stderr, "Initialization stage 2 for cpunumber %d\n",(int)cid);
          }
      }
      break;
   }
    
    return CPUAPI_Stat_Ok;
}

/*==============================================================================
Function: terminate
Description:
	[DETAILS]
Input:
	cid - Cpu identifier
   phase - Stage of initialization (defined by Kernel)
Output:
	none
Assumptions:
	none
==============================================================================*/
static cpuapi_stat_t terminate(cpuapi_cid_t cid,
                               cpuapi_phase_t phase)
{   
   switch (phase)
   {
     case 0://terminate phase 0
      {
          if (asim_verbose)
          {
              fprintf(stderr, "Terminate stage 1 for cpunumber %d\n",(int)cid);
          }
      }
      break;
     case 1://terminate phase 1         
     {
         if (asim_verbose)
         {
             fprintf(stderr, "Terminate stage 2 for cpunumber %d\n",(int)cid);
         }
     }
     break;
   }   
   return CPUAPI_Stat_Ok;
}

/*==============================================================================
Function: activate
Description:
	Notify the Cpu model to start the simulation
Input:
	cid - Cpu identifier
   mode - Simulation mode (Hot, Cold, Warmup...)
   model_type - Cpu model capability/type (FS/PS/Master/Slave/Reference...)
Output:
	none
Assumptions:
	none
==============================================================================*/
static cpuapi_stat_t activate(cpuapi_cid_t cid,
                              cpuapi_simulmode_t mode,
                              cpuapi_type_t model_type)
{      
    if (asim_verbose)
    {
        fprintf(stderr, "Activate cpunumber %d\n",(int)cid);
    }
    //In this place you should activate ASIM model 

    return CPUAPI_Stat_Ok;
}

/*==============================================================================
Function: deactivate
Description:
	Do nothing
Input:
	cid - Cpu identifier
Output:
	none
Assumptions:
	none
==============================================================================*/
cpuapi_stat_t deactivate(cpuapi_cid_t cid)
{   
    if (asim_verbose)
    {
        fprintf(stderr, "Deactivate cpunumber %d\n",(int)cid);
        fprintf(stderr, "ASIM fetched %lld instrs\n", totalInstrs);            
    }

   return CPUAPI_Stat_Ok;
}

/*==============================================================================
Function: execute
Description:
	Do nothing
Input:
	cid - Cpu identifier
   step_type - Step granularity/weight (e.g. instruction, cycles...)
   requested - Number of granted instructions/cycles (by the Kernel)
Output:
	actual - Number of instructions/cycles the Cpu model exhusted
Assumptions:
	none
==============================================================================*/

//void asim()
//{}


static cpuapi_stat_t execute(cpuapi_cid_t cid,
                             cpuapi_step_t step_type,
                             cpuapi_u64_t requested,
                             cpuapi_u64_t *actual)
{
   int i;
   cpuapi_inst_info_t *instInfo;
   cpuapi_u32_t cpuNum;
   static int cycles = 0;
   cpuapi_stat_t ret_val=CPUAPI_Stat_Ok;
   if (asim_verbose)
   {
       fprintf(stderr, "ASIM: execute cpunumber %d for %lld instruction\n",(int)cid,requested); 
   }

   //print out the cfm and preds


   /*Performance model runs only in cycles*/  
   if (CPUAPI_Step_Cycles != step_type)
   {
      return CPUAPI_Stat_Err;
   } 

   UINT64 CFM;
   UINT64 PRED;

   cpucCallBack->get_object(controllerPids[(cpuapi_u32_t)cid],0, 
                            CFM_handles[(cpuapi_u32_t)cid], CPUAPI_IPF_CFM_SIZE , &CFM);
    
   
   
   cpucCallBack->get_object(controllerPids[(cpuapi_u32_t)cid],0, 
                            PRED_handles[(cpuapi_u32_t)cid],CPUAPI_IPF_PR_SIZE , &PRED);


   cout<<"The new cfm is"<<CFM<<endl;
   cout<<"The new preds are "<<PRED<<endl;

   if(current_mode == CPUAPI_Simul_Mode_Hot)
   {
       cpuNum = (cpuapi_u32_t) cid;
       instInfo=new cpuapi_inst_info_t[requested];
       bzero(instInfo, (sizeof(instInfo))*requested);
       ret_val=step_ref(cpuNum, instInfo, requested);
       delete instInfo;
   }

   *actual = requested;

   return ret_val;
}

/*==============================================================================
Function: stop_asap
Description:
	Do nothing
Input:
	cid - Cpu identifier
Output:
	none
Assumptions:
	none
==============================================================================*/
static cpuapi_stat_t stop_asap(cpuapi_cid_t cid)
{
    fprintf(stderr, "ASIM CPUAPI:  Stop\n");
    return CPUAPI_Stat_Ok;
}

static cpuapi_stat_t abort_execution(cpuapi_cid_t cid)
{
    fprintf(stderr, "ASIM CPUAPI:  Abort\n");
    return CPUAPI_Stat_Ok;
}


/*==============================================================================
Function: list_objects
Description:
	Provide a list of all supported objects
Input:
	cid - Cpu identifier
Output:
   key - Object handle
   names - Object name (subject to CPU API definition and naming convention)
Assumptions:
	none
==============================================================================*/
static int list_objects(cpuapi_cid_t cid,
                        char *key,
                        char *names[])
{
   if ( (key == NULL) && names)
   {
      names[0] = strdup(CPUAPI_SIM_PERF_FREQ);
      return 1;
   }
   if ( (key == NULL) && (names == NULL) )
   {
      return 1;
   }
   return 0;
}

/*==============================================================================
Function: translate_object_name
Description:
	Translate an object (by it's name) to the corresponding handle
Input:
	cid - Cpu identifier
   name - Object name as defined in CPU API
Output:
   handle - A numeric identifer for the object (given by this Cpu model)
Assumptions:
	none
==============================================================================*/
static cpuapi_stat_t translate_object_name(cpuapi_cid_t cid,
                                           const char *name,
                                           cpuapi_handle_t *handle)
{
   if (!strcmp(name, CPUAPI_SIM_PERF_FREQ))
	{
	  *handle = PERF_FREQ;
      return CPUAPI_Stat_Ok;
	}

   return CPUAPI_Stat_Err;
}

/*==============================================================================
Function: translate_object_handle
Description:
	Translate an object (by it's handle) to the corresponding name
Input:
	cid - Cpu identifier
   handle - A numeric identifer for the object (given by this Cpu model)
Output:
   name - Object name as defined in CPU API
Assumptions:
	none
==============================================================================*/
static cpuapi_stat_t translate_object_handle(cpuapi_cid_t cid,
                                             cpuapi_handle_t handle,
                                             char **name)
{
   switch (handle)
   {
      case PERF_FREQ:
         memcpy(*name, CPUAPI_SIM_PERF_FREQ,strlen(CPUAPI_SIM_PERF_FREQ));
         return CPUAPI_Stat_Ok;
      default: 
         return CPUAPI_Stat_Err;
   }
}

/*==============================================================================
Function: get_object
Description:
	Get only supported objects
Input:
	cid - Cpu identifier
   handle - A numeric identifer for the object (given by this Cpu model)
   size - Size of the object (in bytes)
Output:
	value - Value of object to be set
Assumptions:
	none
==============================================================================*/
static cpuapi_stat_t get_object(cpuapi_cid_t cid,
                                cpuapi_handle_t handle,
                                cpuapi_size_t *size,
                                void *value)
{

   char *src;
   cpuapi_perf_freq_t freq;

   switch (handle)
   {
   case PERF_FREQ:
         //Put here some function that reads your frequency from
         //config file
	      freq.numerator = 100;
         freq.denominator = 1; 
                     
         src  = (char *)(&freq);
         *size = CPUAPI_SIM_PERF_FREQ_SIZE;
         memcpy(value, src,(unsigned int)*size);        
		 return CPUAPI_Stat_Ok;

      default:
         return CPUAPI_Stat_Err;
   }
}

/*==============================================================================
Function: get_object_by_name
Description:
	Get object using it's name as key
Input:
	cid - Cpu identifier
   name - Object name as defined in CPU API
   size - Size of the object (in bytes)
Output:
	value - Value of object to be set
Assumptions:
	none
==============================================================================*/
static cpuapi_stat_t get_object_by_name(cpuapi_cid_t cid,
                                        const char *name,
                                        cpuapi_size_t *size,
                                        void *value)
{
   cpuapi_handle_t handle;
    
   if (CPUAPI_Stat_Ok != translate_object_name (cid, name, &handle))
   {
      return CPUAPI_Stat_Err;
   }
   return get_object(cid, handle, size, value);  
}

/*==============================================================================
Function: set_object
Description:
	Do nothing
Input:
	cid - Cpu identifier
   handle - A numeric identifer for the object (given by this Cpu model)
   size - Size of the object (in bytes)
	value - Value of object to be set
Output:
	none
Assumptions:
	none
==============================================================================*/
static cpuapi_stat_t set_object(cpuapi_cid_t cid,
                                cpuapi_handle_t handle,
                                cpuapi_size_t size,
                                const void *value)
{   
   return CPUAPI_Stat_Err;
}

/*==============================================================================
Function: set_object_by_name
Description:
	Do nothing
Input:
	cid - Cpu identifier
   name - Object name as defined in CPU API
   size - Size of the object (in bytes)
	value - Value of object to be set
Output:
	none
Assumptions:
	none
==============================================================================*/
static cpuapi_stat_t set_object_by_name(cpuapi_cid_t cid,
                                        const char *name,
                                        cpuapi_size_t size,
                                        const void *value)
{
   return CPUAPI_Stat_Err;
}

/*==============================================================================
Function: translate_virt_addr
Description:
	Do nothing
Input:
	cid - Cpu identifier
   vaddr - Virtual address
Output:
	paddr - Physical address
Assumptions:
	none
==============================================================================*/
static cpuapi_stat_t translate_virt_addr(cpuapi_cid_t cid,
                                         cpuapi_virt_addr_t vaddr,
                                         cpuapi_access_type_t access,
                                         cpuapi_phys_addr_t *paddr)
{
   return CPUAPI_Stat_Err;
}

/*==============================================================================
Function: pin_assert
Description:
	Do nothing
Input:
	cid - Cpu identifier
   pin - Asserted pin number
Output:
	none
Assumptions:
	none
==============================================================================*/
static cpuapi_stat_t pin_assert(cpuapi_cid_t cid,
                                cpuapi_pin_t pin)
{
   return CPUAPI_Stat_Ok;
}

/*==============================================================================
Function: pin_deassert
Description:
	Do nothing
Input:
	cid - Cpu identifier
   pin - De-asserted pin number
Output:
	none
Assumptions:
	none
==============================================================================*/
static cpuapi_stat_t pin_deassert(cpuapi_cid_t cid,
                                  cpuapi_pin_t pin)
{
   return CPUAPI_Stat_Ok;
}

/*==============================================================================
Function: pin_data
Description:
	Do nothing
Input:
	cid - Cpu identifier
   pin - De-asserted pin number
   icount - Instruction count when interrupt accured
   vector - Interrupt data
Output:
	none
Assumptions:
	none
==============================================================================*/
cpuapi_stat_t pin_data(cpuapi_cid_t cid,
                       cpuapi_pin_t pin,
                       cpuapi_inst_t icount,
                       cpuapi_u32_t vector)
{
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
Function: command
Description:
    Extensibility function. 
    Forward the command to inferno.
Input:
	cid - Unique ID identifying the Cpu model that is the target for this call
   argc - Number of tokens in argv
   argv - Token list
   inbuf - command as printed in debugger.
Output:
	outbuf
   outbuf_size
Assumptions:
	none
==============================================================================*/
cpuapi_stat_t command(cpuapi_cid_t cid, int argc, char *argv[],
		void *inbuf, void *outbuf, cpuapi_size_t outbuf_size)
{
    if (asim_verbose)
    {
        fprintf(stderr, "ASIM CPUAPI Command:  %s\n", (char *)inbuf);
    }
   
    //
    // This copies the input buffer to output.  I think it is just used for
    // printing a message to stdout.
    //
    if (outbuf_size > strlen((char *)inbuf))
    {
        outbuf_size = strlen((char*)inbuf) + 1;
    }
    strncpy((char *)outbuf, (char *)inbuf, outbuf_size);
    ((char *)outbuf)[outbuf_size - 1] = 0;        // Guarantee null termination

    //
    // Parse commands
    //
    if (strcmp((char *)inbuf, "end_simulation") == 0)
    {
        asim_simulation_done = true;
    }

    return CPUAPI_Stat_Ok;
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


/*==============================================================================
Function: set_simulation_mode 

Description:
  Do nothing

Input:
  cid - Cpu identifier
  mode - simulation mode

Output:
  none

Assumptions:
  none
==============================================================================*/

static cpuapi_stat_t
set_simulation_mode(
    cpuapi_cid_t cid,
    cpuapi_simulmode_t mode)
{
    //set the mode variable for referencing in the execute function
    current_mode=mode;
    if (mode == CPUAPI_Simul_Mode_Hot)
    {
           //CFM
        cpucCallBack->translate_object_name(controllerPids[(cpuapi_u32_t) cid],CPUAPI_IPF_CFM,&CFM_handles[(cpuapi_u32_t)cid]);
        cpucCallBack->translate_object_name(controllerPids[(cpuapi_u32_t) cid],CPUAPI_IPF_PR , &PRED_handles[(cpuapi_u32_t)cid]);

        if (asim_verbose)
        {
            fprintf(stderr, "ASIM - simulation mode for cpunumber %d is performance mode\n",(int)cid); 
            
        }
    }
    else
    {
        if (asim_verbose)
        {
            fprintf(stderr, "ASIM - simulation mode for cpunumber %d is funtional mode\n",(int)cid);
        }
    }

    return CPUAPI_Stat_Ok;
}


/*==============================================================================
Function: cpuapi_load_module

Description:
  Load the Cpu model and initialize required data.
  Call the Cpu controller to register the model (or models) for this run.
  See CPU API document for more information.

Input:
  version - Cpu controller version
  cbk - Call back strucrure with Cpu controller interface
  argc - Not used (value is 0)
  argv - A string with configuration items

Output:
  none

Assumptions:
  none
==============================================================================*/

extern "C" EXPORT_FUNC
cpuapi_stat_t
cpuapi_load_module(
    cpuapi_u32_t version,
    cpuapi_controller_callbacks_t *cbk,
    int in_argc,
    char *in_argv[])
{
    cpuapi_properties_t cpu_prop;
    cpuapi_cpu_callbacks_t cpu_clbk;
    argc = in_argc;
    argv = in_argv;

    fprintf(stderr, "ASIM: cpuapi_load_module\n");

//    if (CPUAPI_SIM_INST_BYTES_LENGTH != 16)
//    {
//        fprintf (stderr, "ASIM: Expected 16 byte instructions\n");
//        return CPUAPI_Stat_Err;
//    }

    cpucCallBack = cbk;

    bzero((void *)&cpu_prop, sizeof(cpu_prop));
    cpu_prop.name = "ASIM";
    cpu_prop.type = (cpuapi_type_t) (CPUAPI_Type_Master_Fetch | CPUAPI_Type_Perf);
    cpu_prop.arch_type = CPUAPI_IPF;
    cpu_prop.capabilities = CPUAPI_MFINIT " " CPUAPI_MFTRM " " CPUAPI_EX " " CPUAPI_GS;
    fprintf(stderr, "Capabilities: %s\n", cpu_prop.capabilities);

    bzero((void *)&cpu_clbk, sizeof(cpu_clbk));
    cpu_clbk.initialize = initialize;
    cpu_clbk.activate = activate;
    cpu_clbk.deactivate = deactivate;
    cpu_clbk.terminate = terminate;
    cpu_clbk.pin_assert = pin_assert;
    cpu_clbk.pin_deassert = pin_deassert;
    cpu_clbk.pin_data = pin_data;
    cpu_clbk.execute = execute;
    cpu_clbk.stop_asap = stop_asap;
    cpu_clbk.abort_execution = abort_execution;
    cpu_clbk.list_objects = list_objects;
    cpu_clbk.set_object = set_object;
    cpu_clbk.get_object = get_object;
    cpu_clbk.set_object_by_name = set_object_by_name;
    cpu_clbk.get_object_by_name = get_object_by_name;
    cpu_clbk.translate_object_name = translate_object_name;
    cpu_clbk.translate_object_handle = translate_object_handle;
    cpu_clbk.translate_virt_addr = translate_virt_addr;
    cpu_clbk.command = command;
    cpu_clbk.set_simulation_mode = set_simulation_mode;

    cpu_clbk.architecture_specific_api = (cpuapi_ipf_arch_specific_callbacks_t*)malloc(sizeof(cpuapi_ipf_arch_specific_callbacks_t));

    if (NULL == cpu_clbk.architecture_specific_api)
    {
        return CPUAPI_Stat_Err;
    }

    ((cpuapi_ipf_arch_specific_callbacks_t*)(cpu_clbk.architecture_specific_api))->io_port_access = io_port_access;
    ((cpuapi_ipf_arch_specific_callbacks_t*)(cpu_clbk.architecture_specific_api))->processor_interrupt_block_access = processor_interrupt_block_access;

    //
    // Read number of Cpu devices from ODB and call register function
    // for each device.
    //
    {
        int cpuNum;
        int numberOfCpuDevices = 1; // Default

        short argId;
        
        for(argId=0;argId<argc;argId++)
        {
            char *numCpusArgPtr = strstr(argv[argId], "-num_cpus=");
            if (NULL != numCpusArgPtr) // Assuming all lower case
            {
                char *numCpusToken;
                char *numCpusLocalStr = strdup(numCpusArgPtr);
                if(NULL == numCpusLocalStr)
                {
                    return CPUAPI_Stat_Err;
                }
                numCpusToken = strtok(numCpusLocalStr, "=");
                numCpusToken = strtok(NULL , "= "); // Get value of token
                numberOfCpuDevices = atoi(numCpusToken);
                free(numCpusLocalStr);
                break;
            }
        }		

        for (cpuNum = 0; cpuNum < numberOfCpuDevices; cpuNum++) 
        {
            cpu_prop.proc_num = cpuNum;
            if (CPUAPI_Stat_Err == cpucCallBack->cpu_register((void *)cpuNum,
                                                              &cpu_prop,
                                                              &cpu_clbk,
                                                              &controllerPids[cpuNum]))
            {
                fprintf (stderr, "ASIM: Failed to register in cpuc\n");
                return CPUAPI_Stat_Err;
            }
        }
    } 

    return CPUAPI_Stat_Ok;
}

