/**************************************************************************
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
 **************************************************************************/

/**
 * @file
 * @author Michael Adler
 * @brief SoftSDV stub for talking to Asim
 */

//
// System include files
//
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

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

#define ASIMERROR(mesg) \
    fprintf(stderr, "%s:%d %s\n", __FILE__, __LINE__, mesg); \
    asim_error_exit(1);

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

//
// ASIM Guest I/O shared with the ISA-specific code
//
ASIM_GUEST_OS_INFO_CLASS asimGuestOSInfo;


//
// ASIM I/O class.  By leaving this variable as a static the
// constructor is called automatically when the library is loaded and
// the destructor is called automatically when the program exits.
// The constructor and destructor establish the connection to Asim.
//
// How convenient.
//
SOFTSDV_IO_SOFTSDV_SIDE_CLASS asim_io;


//
// Warm-up method will be passed from Asim during startup
//
ASIM_SOFTSDV_WARMUP_METHOD warmUpMethod = ASIM_SDV_WARMUP_OFF;

//
// Registers to monitor during execution
//
ASIM_REQUESTED_REGS_CLASS regRqst;

//
// Handle to CPU controller callback functions
//
cpuapi_controller_callbacks_t *asimCpucCallBack;

CPU_DMA_INVALIDATE dmaIdx = NULL;

void asim_error_exit(int s)
{
    asim_io.Exiting(false);
    if (s != 0)
    {
        fprintf(stderr, "ASIM/CPUAPI forcing SoftSDV to exit...\n");
    }
    exit(s);
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
    UINT32 cpuNum = (UINT32) cid;

    switch (phase)
    {
      case 0:      
        //
        // Init phase 0 -- Local model initialization
        //
        break;
      case 1:
        //
        // Init phase 1 -- Initialization can depend on global state
        //
        AsimInitCpu_ArchSpecific(cpuNum, regRqst);
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
    asim_io.Exiting(true);

    switch (phase)
    {
      case 0://terminate phase 0
        break;
      case 1://terminate phase 1         
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
    UINT32 cpuNum = (UINT32) cid;

    if (cpuNum == 0)
    {
        if (dmaIdx != NULL)
        {
            delete dmaIdx;
        }
        if (asim_io.MonitorDMA())
        {
            dmaIdx = new CPU_DMA_INVALIDATE_CLASS();
            printf("ASIM: monitoring DMA traffic\n");
        }
    }

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
    UINT32 cpuNum = (UINT32) cid;

    if (dmaIdx != NULL)
    {
        delete dmaIdx;
    }
    dmaIdx = NULL;

    return CPUAPI_Stat_Ok;
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
//   if (!strcmp(name, CPUAPI_SIM_PERF_FREQ))
//	{
//	  *handle = PERF_FREQ;
//      return CPUAPI_Stat_Ok;
//	}

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
//      case PERF_FREQ:
//         memcpy(*name, CPUAPI_SIM_PERF_FREQ,strlen(CPUAPI_SIM_PERF_FREQ));
//         return CPUAPI_Stat_Ok;
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
//   case PERF_FREQ:
         //Put here some function that reads your frequency from
         //config file
//	      freq.numerator = 100;
//         freq.denominator = 1; 
                     
//         src  = (char *)(&freq);
//         *size = CPUAPI_SIM_PERF_FREQ_SIZE;
//         memcpy(value, src,(unsigned int)*size);        
//		 return CPUAPI_Stat_Ok;

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
Function: notify_memory_write
Description:
    Called when memory is modified by some external device (e.g. DMA from
    a disk.)
==============================================================================*/
void notify_memory_write(cpuapi_cid_t cid,
                         cpuapi_phys_addr_t paddr,
                         cpuapi_size_t size
#if CPUAPI_VERSION_MAJOR(CPUAPI_VERSION_CURRENT) >= 9
                         ,
                         cpuapi_boolean_t is_dma
#endif
                        )
{
    UINT32 cpuNum = (UINT32) cid;

//    printf("CPU %d:  Memory write 0x%llx - %lld bytes", cpuNum, paddr, size);
//#if CPUAPI_VERSION_MAJOR(CPUAPI_VERSION_CURRENT) >= 9
//    if (is_dma)
//    {
//        printf(" (DMA)");
//    }
//#endif
//    printf("\n");

    if (dmaIdx != NULL)
    {
        dmaIdx->InvalidateAddr(paddr, size);
    }

    AsimLockStepDMA(paddr, size);
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
cpuapi_stat_t command(cpuapi_cid_t cid, int useless_argc, char *useless_argv[],
		void *inbuf, void *outbuf, cpuapi_size_t outbuf_size)
{
    typedef char ARGSTR[128];
    int argc;
    ARGSTR argv[4];

    char *msg = (char *) inbuf;

    ((char *)outbuf)[0] = 0;
    outbuf_size = 0;
    bool valid = false;

    if (strlen(msg) < sizeof(ARGSTR))
    {
        argc = sscanf(msg, "%s %s %s %s", argv[0], argv[1], argv[2], argv[3]);
        if (argc)
        {
            if (strcasecmp(argv[0], "start_simulation") == 0)
            {
                printf("ASIM: Simulation Starting\n");
                for (unsigned int i = 0; i < asim_io.NCpus(); i++)
                {
                    cpuInfo[i].SetStateInvalid();
                }
                valid = true;
            }
            else if (strcasecmp(argv[0], "end_simulation") == 0)
            {
                printf("ASIM: Simulation Ending\n");
                asim_io.Exiting(true);
                valid = true;
            }
            else if (strcasecmp(argv[0], "start_warmup") == 0)
            {
                printf("ASIM: Enter warm-up mode");
                for (unsigned int i = 0; i < asim_io.NCpus(); i++)
                {
                    cpuInfo[i].SetStateWarmup();
                }

                //
                // Pick either detailed or fast execution mode depending
                // on whether Asim needs warm-up data.
                //
                UINT8 mode;
                if (warmUpMethod == ASIM_SDV_WARMUP_ON)
                {
                    printf(" -- Asim wants warm-up data\n");
                    mode = 2;
                }
                else
                {
                    printf(" -- Asim does not want warm-up data\n");
                    mode = 4;
                }
                ASSERTX(GE_OK == oml_set_value_by_name(GE_VPC_CPU_ID,
                                                       "notifier.vpc.kernel.simul_mode",
                                                       sizeof(mode),
                                                       &mode));

                valid = true;
            }
            else if (strcasecmp(argv[0], "end_warmup") == 0)
            {
                printf("ASIM: Exit warm-up mode\n");
                for (unsigned int i = 0; i < asim_io.NCpus(); i++)
                {
                    cpuInfo[i].ClearStateWarmup();
                }
                valid = true;
            }
            else if (strcasecmp(argv[0], "start_lockstep") == 0)
            {
                printf("ASIM: Lock Step Starting\n");
                AsimStartLockStep();
                valid = true;
            }
            else if (strcasecmp(argv[0], "end_lockstep") == 0)
            {
                printf("ASIM: Lock Step Ending\n");
                AsimStopLockStep();
                valid = true;
            }
            else if (strcasecmp(argv[0], "force_memory_values") == 0)
            {
                printf("ASIM: Turning on memory value collection\n");
                asim_io.SetRecordMemoryValues();
                for (unsigned int i = 0; i < asim_io.NCpus(); i++)
                {
                    cpuInfo[i].CollectMemoryValues(true);
                }
                valid = true;
            }
            else if (strcasecmp(argv[0], "skipping") == 0)
            {
                printf("ASIM: Fast Fordwarding\n");
                for (unsigned int i = 0; i < asim_io.NCpus(); i++)
                {
                    cpuInfo[i].SetStateInvalid();
                }
                valid = true;
            }
            else if (strcasecmp(argv[0], "timer_interrupt_info") == 0)
            {
                if (argc < 2)
                {
                    fprintf(stderr, "Usage: timer_interrupt_info <timer vector>\n");
                }
                else
                {
                    UINT64 timer;
                    sscanf(argv[1], "%llx", &timer);
                    asimGuestOSInfo.SetTimerInterruptVector(timer);
                    valid = true;
                }
            }
            else if (strcasecmp(argv[0], "local_timer_interrupt_info") == 0)
            {
                if (argc < 2)
                {
                    fprintf(stderr, "Usage: local_timer_interrupt_info <timer vector>\n");
                }
                else
                {
                    UINT64 timer;
                    sscanf(argv[1], "%llx", &timer);
                    asimGuestOSInfo.SetLocalTimerInterruptVector(timer);
                    valid = true;
                }
            }
            else if (strcasecmp(argv[0], "context_switch_info") == 0)
            {
                if (argc < 2)
                {
                    fprintf(stderr, "Usage: context_switch_info <os> [pid offset] [proc name offset]\n");
                }
                else
                {
                    int pid = -1;
                    int procName = -1;

                    if (argc > 2)
                    {
                        pid = atoi(argv[2]);
                    }
                    if (argc > 3)
                    {
                        procName = atoi(argv[3]);
                    }

                    asimGuestOSInfo.SetOsName(argv[1]);
                    asimGuestOSInfo.SetPidOffset(pid);
                    asimGuestOSInfo.SetProcNameOffset(procName);

                    //
                    // For 32 bit x86 Linux, set the method for finding the
                    // task struct.
                    //
                    if (strcasecmp(argv[1], "sles9_i386") == 0)
                    {
                        asimGuestOSInfo.SetFindTaskMethod(TASK_STRUCT_RSP_INDIRECT);
                    }

                    valid = true;
                }
            }
            else if (strcasecmp(argv[0], "pda_info") == 0)
            {
                if (argc < 3)
                {
                    fprintf(stderr, "Usage: pda_info <pda VA> <switch to IP>\n");
                }
                else
                {
                    UINT64 pdaVA, switchToIP;
                    sscanf(argv[1], "%llx", &pdaVA);
                    sscanf(argv[2], "%llx", &switchToIP);

                    asimGuestOSInfo.SetPDAArrayAddr(pdaVA);
                    asimGuestOSInfo.SetPDAContextSwitchIP(switchToIP);

                    // Setting PDA info must mean a 64 bit OS
                    asimGuestOSInfo.SetFindTaskMethod(TASK_STRUCT_GS);
                    valid = true;
                }
            }
            else if (strcasecmp(argv[0], "idle_loop_tag") == 0)
            {
                //
                // The address of one instruction representing the kernel's
                // idle loop.
                //
                if (argc < 2)
                {
                    fprintf(stderr, "Usage: idle_loop_tag <instr address (don't pick a NOP!)>\n");
                }
                else
                {
                    UINT64 idleAddr;
                    sscanf(argv[1], "%llx", &idleAddr);
                    asimGuestOSInfo.SetIdleAddr(idleAddr);
                    valid = true;
                }
            }
        }
    }

    if (! valid)
    {
        fprintf(stderr, "\nASIM:  Invalid command.  Valid commands are:\n");
        fprintf(stderr, "    context_switch_info <os> [pid offset] [proc name offset]\n");
        fprintf(stderr, "    start_simulation\n");
        fprintf(stderr, "    end_simulation\n");
        fprintf(stderr, "    start_warmup\n");
        fprintf(stderr, "    end_warmup\n");
        fprintf(stderr, "    start_lockstep\n");
        fprintf(stderr, "    end_lockstep\n");
        fprintf(stderr, "    force_memory_values\n");
        fprintf(stderr, "    skipping\n");
        outbuf_size = strlen(msg);
        memcpy(outbuf, msg, (unsigned int)outbuf_size);
    }

    return CPUAPI_Stat_Ok;
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
    //
    // Invalidate CPU info on any transition
    //
    cpuapi_u32_t cpuNum = (cpuapi_u32_t) cid;
    
    cpuInfo[cpuNum].SetStateInvalid();
    cpuInfo[cpuNum].SetSimulationMode(mode);

    if (mode == CPUAPI_Simul_Mode_Hot)
    {
        cpuInfo[cpuNum].CollectMemoryValues(true);
        if (cpuNum == 0)
        {
            printf("ASIM - simulation mode is in performance mode\n");
        }
    }
    else
    {
        cpuInfo[cpuNum].CollectMemoryValues(false);
        if (cpuNum == 0)
        {
            printf("ASIM - simulation mode is in funtional mode\n");
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
  cbk - Call back structure with Cpu controller interface
  argc - Not used (value is 0)
  argv - A string with configuration items
==============================================================================*/

extern "C" EXPORT_FUNC
cpuapi_stat_t
cpuapi_load_module(
    cpuapi_u32_t version,
    cpuapi_controller_callbacks_t *cbk,
    int argc,
    char *argv[])
{
    cpuapi_properties_t cpu_prop;
    cpuapi_cpu_callbacks_t cpu_clbk;

    //
    // Set the buffering on stdout and stderr to line buffering.  This
    // fixes some problems in batch logs as stderr/stdout have interleaving
    // problems.
    //
    setlinebuf(stdout);
    setlinebuf(stderr);

    printf("ASIM: Load module, CPUAPI version 0x%x\n", version);

    asimCpucCallBack = cbk;

    bzero((void *)&cpu_prop, sizeof(cpu_prop));
    cpu_prop.name = "ASIM";
    cpu_prop.type = (cpuapi_type_t) (CPUAPI_Type_Master_Fetch | CPUAPI_Type_Perf);
    cpu_prop.arch_type = ASIM_CPUAPI_ISA;
#ifndef CPUAPI_STOP_ASAP
#define CPUAPI_STOP_ASAP ""
#endif
    cpu_prop.capabilities = CPUAPI_MFINIT " " CPUAPI_MFTRM " " CPUAPI_EX " " CPUAPI_STOP_ASAP;
    if (CPUAPI_VERSION_MAJOR(CPUAPI_VERSION_CURRENT) <= 9)
    {
        cpu_prop.version = version;
    }
    else
    {
        fprintf(stderr, "ASIM: CPUAPI version not supported\n");
        return CPUAPI_Stat_Err;
    }

    bzero((void *)&cpu_clbk, sizeof(cpu_clbk));
    cpu_clbk.initialize = initialize;
    cpu_clbk.activate = activate;
    cpu_clbk.deactivate = deactivate;
    cpu_clbk.terminate = terminate;
    cpu_clbk.pin_assert = pin_assert;
    cpu_clbk.pin_deassert = pin_deassert;
    cpu_clbk.pin_data = pin_data;
    cpu_clbk.execute = ExecuteAsim;
    cpu_clbk.stop_asap = stop_asap;
    cpu_clbk.abort_execution = stop_asap;
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
    cpu_clbk.notify_memory_write = notify_memory_write;

    //
    // Architecture specific callbacks
    //
    cpu_clbk.architecture_specific_api = CPUAPI_ArchSpecificClbks();

    //
    // How many CPUs are being simulated?
    //
    asim_io.SetNCpus(1);      // Default

    //
    // Get some state from command line arguments.  These values would
    // eventually be available as OML notifier values but this code runs
    // before the names have been registered.
    //
    for(int argId = 0; argId < argc; argId++)
    {
        if (! strcmp(argv[argId], "-os"))
        {
            asim_io.SetOSMode();
        }
        else
        {
            char *numCpusArgPtr = strstr(argv[argId], "-num_cpus=");
            if (NULL != numCpusArgPtr) // Assuming all lower case
            {
                char *numCpusToken = numCpusArgPtr + strlen("-num_cpus=");
                asim_io.SetNCpus(atoi(numCpusToken));
            }
        }
    }

    if (asim_io.AsimPid())
    {
        if (asim_io.MonitorDMAAsim())
        {
            asim_io.SetMonitorDMA();
        }
    }

    if (asim_io.AsimPid())
    {
        ASSERTX(asim_io.NCpus() == asim_io.NCpusAsim());
        ASSERTX(asim_io.OSMode() == asim_io.OSModeAsim());
        ASSERTX(asim_io.MonitorDMA() == asim_io.MonitorDMAAsim());

        //
        // Get the initial requests from Asim
        //
        ASIM_SOFTSDV_REQUEST asimRequest;

        asimRequest = asim_io.AsimRequestQueue().OpenNext();
        ASSERT(asimRequest->Request() == ASIM_REQUEST_INIT,
               "Expected ASIM_REQUEST_INIT message.");
        asim_io.AsimRequestQueue().Close(asimRequest);
        
        bool initDone = false;
        while (! initDone)
        {
            asimRequest = asim_io.AsimRequestQueue().OpenNext();

            switch(asimRequest->Request())
            {
              case ASIM_REQUEST_INIT_DONE:
                initDone = true;
                break;

              case ASIM_REQUEST_REG_MONITOR:
                regRqst.AddRegister(asimRequest->RegNum(),
                                    asimRequest->RegName(),
                                    asimRequest->RegSize());
                break;

              default:
                ASSERT(false, "Unexpected ASIM_REQUEST message.");
                break;
            }

            asim_io.AsimRequestQueue().Close(asimRequest);
        }

        //
        // Wait for Asim to say whether warm-up is needed.  That should
        // be the next message.
        //
        asimRequest = asim_io.AsimRequestQueue().OpenNext();
        ASSERT(asimRequest->Request() == ASIM_REQUEST_WARMUP,
               "Expected ASIM_REQUEST_WARMUP message.");
        warmUpMethod = asimRequest->WarmUpMethod();
        asim_io.AsimRequestQueue().Close(asimRequest);
    }

    AsimInit_ArchSpecific();

    printf("ASIM: %d CPU simulation, OS mode %s\n",
           asim_io.NCpus(),
           asim_io.OSMode() ? "on" : "off");

    //
    // Register simulated CPUs
    //
    for (unsigned int cpuNum = 0; cpuNum < asim_io.NCpus(); cpuNum++) 
    {
        cpuapi_pid_t cpu_pid;

        cpu_prop.proc_num = cpuNum;
        if (CPUAPI_Stat_Err == asimCpucCallBack->cpu_register((void *)cpuNum,
                                                              &cpu_prop,
                                                              &cpu_clbk,
                                                              &cpu_pid))
        {
            fprintf (stderr, "ASIM: Failed to register CPU %d\n", cpuNum);
            return CPUAPI_Stat_Err;
        }

        //
        // Some callbacks should be associated only with one CPU
        //
        cpu_clbk.notify_memory_write = NULL;

        cpuInfo[cpuNum].SetControllerPid(cpuNum, cpu_pid);
    }

    return CPUAPI_Stat_Ok;
}
