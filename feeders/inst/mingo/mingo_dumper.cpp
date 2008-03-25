/*****************************************************************************
 *
 * @brief Source file for Mingo dumper system driver
 *
 * @author Michael Adler
 *
 * Copyright (C) 2002-2006 Intel Corporation
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
 *****************************************************************************/

#ifndef ASIM_ENABLE_TRACE
//
// Not always defined (e.g. if optimization is enabled).  Force it on
// so trace messages are printed.
//
#define ASIM_ENABLE_TRACE
#endif

// ASIM core
#include "asim/trace.h"
#include "asim/trackmem.h"
#include "asim/cmd.h"
#include "asim/ioformat.h"

// ASIM public modules
#include "asim/provides/instfeeder_interface.h"
#include "asim/provides/system.h"

static ASIM_MINGO_DUMPER_SYSTEM mingo_dumper_system = NULL;

// ASIM local module
// instfeeder_implementation.h is a generated header file
#include "asim/provides/instfeeder_implementation.h" 

extern MINGO_FEEDER MingoFeederHandle;

//
// If non-zero then tracing branches for specified thread.
//
static unsigned int traceThreadBranches = 0;
static unsigned int verbose = 1;

static UINT64 traceEvents = 1000000;

// added by nkatan for data collection
#define ADDRESSHASH(X) ((X>>4)&(128-1))
ASIM_MINGO_DUMPER_SYSTEM_CLASS::asim_mingo_dumper_system_class(
    const char *n, 
    const UINT32 nSlices)
    : ASIM_SYSTEM_CLASS(n, nSlices)
{
    config = new ASIM_CONFIG_CLASS(this);
    config->RegisterSimulatorConfiguration();
}


ASIM_MINGO_DUMPER_SYSTEM_CLASS::~asim_mingo_dumper_system_class()
{
    delete config;

}

bool
ASIM_MINGO_DUMPER_SYSTEM_CLASS::InitModule()
{
  //this is added for data collection nkatan
  address_table.INIT(500,500000,128,500,"address table");
  return true;
}


void
SYS_Usage (FILE *file)
/*
 * Print usage...
 */
{
    //
    // We don't have any processor flags...
    fprintf(file, "\nCommon system model flags: NONE\n");
}


void
ASIM_MINGO_DUMPER_SYSTEM_CLASS::DumpStats (STATE_OUT stateOut)
{
}


bool
ASIM_MINGO_DUMPER_SYSTEM_CLASS::SYS_ScheduleThread (ASIM_THREAD thread)
/*
 * Start 'thread' running on the performance model.
 */
{
    return true;
}


bool
ASIM_MINGO_DUMPER_SYSTEM_CLASS::SYS_UnscheduleThread (ASIM_THREAD thread)
/*
 * Stop and remove 'thread' from the performance model.
 */
{
    return true;
}

bool
ASIM_MINGO_DUMPER_SYSTEM_CLASS::SYS_BlockThread (ASIM_THREAD thread, ASIM_INST inst)
/*
 * Block 'thread' in the performance model.
 */
{
    return true;
}


bool
ASIM_MINGO_DUMPER_SYSTEM_CLASS::SYS_UnblockThread (ASIM_THREAD thread)
/*
 * Un-block 'thread' from the performance model.
 */
{
    return true;
}


bool
ASIM_MINGO_DUMPER_SYSTEM_CLASS::SYS_HookAllThreads()
/*
 * Hook all 'threads' to the performance model.
 */
{
    return true;
}


bool
ASIM_MINGO_DUMPER_SYSTEM_CLASS::SYS_UnhookAllThreads()
/*
 * Unhook all 'threads' from the performance model.
 */
{
    return true;
}

bool
ASIM_MINGO_DUMPER_SYSTEM_CLASS::SYS_Execute (UINT64 stopCycle, UINT64 stopInst, UINT64 stopPacket)
/*
 * Feeder (presumably, Mingo) has been initialized.  Dump all incoming
 * events and allow threads to proceed.
 *
 * Exit when client terminates.
 */
{
    char *llxstr;
    char *lldstr;
    char *llustr;
    bool more;

//    MingoFeederHandle->SetRunAhead(MAX_RUNAHEAD_EVENTS_PER_THREAD);
//    MingoFeederHandle->SetEventMask(MINGO_MSG_MASK_BRANCH +
//                                    MINGO_MSG_MASK_SYSCALL);
    // Java_java_io_FileOutputStream_writeInternal
//    MingoFeederHandle->FastForwardToIP(0x863107cL, 271);

    UINT64 events[1000];
    unsigned int maxevents = 0;
    UINT64 icount = 0;
    UINT64 bcount = 0;
    int maxThreads = 0;

    //added by nkatan to extract data.
    UINT64 va_count =0;
    UINT64 va;
    UINT64 in_count;
    UINT64 total_icount = 0;
    UINT64 age_icount =0;
    UINT64 age=0;
    UINT32 inc=1;
    int low=0;
    //low =1 sets low level metrics.
    int high=1; // set for metrics level
    UINT32 num_read=0;
    UINT32 num_write=0;
    UINT32 num_invalid=0;
    //make me into a big array
    UINT64 count[10000];
    UINT64 D_count[10000];
    // end of Nkatan's stuff.
    

    for (int i = 0; i < 1000; i++) events[i] = 0;

    if (sizeof(HOST_UINT) == sizeof(UINT64))
    {
        llxstr = "0x%lx";
        lldstr = "%ld";
        llustr = "%lu";
    }
    else
    {
        llxstr = "0x%llx";
        lldstr = "%lld";
        llustr = "%llu";
    }

    if (traceThreadBranches)
    {
        printf("; Mingo branch history trace\n");
        printf(";\n");
        printf(";   Tracing thread %d for ", traceThreadBranches);
        printf(lldstr, traceEvents);
        printf(" events\n");
        printf(";\n");
        printf("; Key:\n");
        printf(";\n");
        printf("; <B><T><D><C> <Source IP> <Target IP> <Instrs since last branch> <Indirection reg>\n");
        printf(";      B - Branch type:  B branch, C call, R return\n");
        printf(";      T - Taken flag:   T taken, F fall through\n");
        printf(";      D - Indirection:  D direct, I indirect\n");
        printf(";      C - Conditional:  C conditional, U unconditional\n");
        printf(";      Instrs since last branch -- count of instructions executed\n");
        printf(";          between previous branch and this one\n");
        printf(";      Indirection register -- meaningful only if D flag is 'I'\n");
        printf(";\n");
        printf("; <S> <Source IP>\n");
        printf(";      Syscall\n");
        printf(";\n");
    }

    more = TRUE;
    while (more)
    {
        int i;

        if (MingoFeederHandle->EndOfData()) break;

        do
        {
            i = MingoFeederHandle->CheckForNewSoftwareThread();
            if (i)
            {
                maxThreads = i;
                if (verbose)
                {
                    fprintf(stderr, "New Software Thread:   %d\n", i);
                }
            }
        }
        while (i);

        for (int td = 1; td <= maxThreads; td++)
        {
            MINGO_DATA_CLASS packet;

            if (MingoFeederHandle->GetNextSoftwareThreadEvent(td, &packet))
            {
                events[packet.get_thread_id()] += 1;
                if (packet.get_thread_id() > maxevents) maxevents = packet.get_thread_id();

                if (traceThreadBranches == 0)
                {
                    //
                    // Normal mode.  Dump all packets.
                    //
                    TRACE(Trace_Sys, packet.DumpPacket(stdout));
                   
  // Stuff from nkatan    
      
		  if (Enable_data_analysis)
		    {
		      if (packet.get_msg() == MINGO_MSG_MEMORY_READ || packet.get_msg() == MINGO_MSG_MEMORY_WRITE) 
			{
			  va  = packet.get_va();
			  va = va & 0xFFFFFFFFFFFFF000;
			  // va = va & 0xFFFFFFFFFFFFF000;
			  // va = va & 0xFFFFFFFFFFFFFFF0;
			  
			  in_count = packet.get_instruction_count();
			  total_icount = total_icount + in_count + 1;
			  
			  struct va_address *mystruct=address_table.FIND_DEP(ADDRESSHASH(va),va);
			  //address_table.trace_on();
			  //so if the address is in the hash table do this
			  if(mystruct)
			    {
			      
			      age_icount = total_icount - mystruct->last_icount ;
			      // need to bucket this here. Would make it better.
			      if(age_icount >9999)  
				{
				  age_icount=9999;
				}
			      
			      //  builds the invalid case where new thread and write
			      // There is a new thread accessing this VA
			      if (packet.get_thread_id() != mystruct->last_thread) 
				{
				  //This is the Write case clear the write counter and make count it as invalid
				  if (packet.get_msg() == MINGO_MSG_MEMORY_WRITE) 
				    {
				      if (mystruct->write_td[td]==1) //I wrote it last time 
					{
					  num_write++;
					}
				      else // new thread tries to write.
					{
					  for (UINT32 i = 0; i < 32; i++) 
					    {
					      mystruct->write_td[i]=0;
					    }
					  
					  mystruct->write_td[td]=1;
					  /* if (low ==1)
					     {
					     mystruct->invalid_cnt_td[td]++; //how many invalids are there.
					     }*/
					  if (high == 1)
					    {
					      num_invalid++;
					    }
					  
					  D_count[age_icount] = D_count[age_icount] +1;
					}
				    }
				  else // there is a read here, if not read before then                                     
				    {
				      if (mystruct->write_td[td]==0) //has not been read yet 
					{
					  D_count[age_icount]++;
					  mystruct->write_td[td]=1; //mark it written  
					  /*if (low == 1)
					    {
					    mystruct->invalid_cnt_td[td]++; //how many invalids are there.
					    }*/
					  if (high ==1)
					    {
					      num_invalid ++;
					    }
					  
					}
				      else 
					{
					  count[age_icount] ++;
				/*	if (low ==1)
					{
					mystruct->read_cnt_td[td]++; //how many read are there.
					}*/
					  if (high ==1)
					    {
					      num_read ++;
					    }
					  
					} //don't drop the next reads on the floor.
				      
				      // in either case do the update. 
				      mystruct->last_icount=total_icount;
				      mystruct->last_thread=packet.get_thread_id(); 
				      
				    }
				}
			      else
				{ // same thread do update.
				  count[age_icount]++;
				  mystruct->last_icount=total_icount;
				  mystruct->last_thread=packet.get_thread_id(); 
				  if (packet.get_msg() == MINGO_MSG_MEMORY_WRITE) 
				    {		
				      /*  if (low ==1)
					  {
					  mystruct->read_cnt_td[td]++;
					  }*/
				      if (high ==1)
					{
					  num_read ++;
					}
				    }
				  else
				    {
				      /* if (low ==1)
					 {
					 mystruct->read_cnt_td[td]++; //how many times the thread read this. 
					 } */
				      if (high ==1)
					{
					  num_read ++;
					}
				    }
				  
				  
				}
			      
			    }
			  //else add the address to the table. I am loosing the first time info here and should fix it later.
			  else
			    {
			      address_table.ADD_VA(va,packet.get_thread_id(),total_icount,ADDRESSHASH(va));
			      va_count ++;                            
			    }
			  
			  if (high == 1)
			    {
			      if (total_icount/10000 > inc)
				{
				  inc++;
				  cout << va_count;
				  cout << " ";
				  cout << total_icount;
				  cout << " Num_read: ";
				  cout << num_read;
				  cout << "\n";
				  
				  cout << va_count;
				  cout << " ";
				  cout << total_icount;
				  cout << " Num_write: ";
				  cout << num_write;
				  cout << "\n";
				  
				  cout << va_count;
				  cout << " ";
				  cout << total_icount;
				  cout << " Num_invalid: ";
				  cout << num_invalid;
				  cout << "\n";
				  
				  
				}
			    }
			}
		    }
		  //End of Nkatan stuff.    
		    



		    if (packet.get_msg() == MINGO_MSG_EXCHANGE)
                    {
                        TRACE(Trace_Sys, printf("    Probe (memory):    "));
                        TRACE(Trace_Sys,
                              printf(llxstr,
                                     MingoFeederHandle->ProbeMemory(packet.get_thread_id(),
                                                                    packet.get_va(),
                                                                    1 << packet.get_access_size())));
                        TRACE(Trace_Sys, printf("\n"));
                    }
                }
                else
                {
                    //
                    // Just dumping branch history.
                    //

                    if (packet.get_thread_id() == traceThreadBranches)
                    {
                        if (packet.get_msg() == MINGO_MSG_BRANCH)
                        {
                            char brtype;

                            switch (packet.get_branch_type())
                            {
                              case MINGO_XFER_BRANCH:
                                brtype = 'B';
                                break;
                              case MINGO_XFER_CALL:
                                brtype = 'C';
                                break;
                              case MINGO_XFER_RETURN:
                                brtype = 'R';
                                break;
                              default:
                                fprintf(stderr, "Illegal branch type\n");
                                exit(1);
                            }

                            printf("%c%c%c%c ",
                                   brtype,
                                   packet.get_is_branch_taken() ? 'T' : 'F',
                                   packet.get_branch_is_register_indirect() ? 'I' : 'D',
                                   packet.get_is_branch_conditional() ? 'C' : 'U');
                            printf(llxstr, packet.get_ip());
                            printf(" ");
                            printf(llxstr, packet.get_branch_target_ip());
                            printf(" ");
                            printf(llustr, icount + packet.get_instruction_count());
                            printf(" %d\n", packet.get_branch_target_register());
                            icount = 0;
                            bcount += 1;
                        }
                        else if (packet.get_msg() == MINGO_MSG_SYSCALL)
                        {
                            printf("S    ");
                            printf(llxstr, packet.get_ip());
                            printf("\n");
                            icount = 0;
                            bcount += 1;
                        }
                        else
                        {
                            icount += (1 + packet.get_instruction_count());
                        }
                    }

                    if (bcount == traceEvents)
                    {
                        more = FALSE;
                        break;
                    }
                }

                MingoFeederHandle->AllowSoftwareThreadProgress(td);

                if (verbose && (packet.get_msg() == MINGO_MSG_THREAD_END))
                {
                    fprintf(stderr, "Software thread ended: %d\n", packet.get_thread_id());
                }
            }
        }
    }

    //nkatan

    if (Enable_data_analysis)
      {
	cout << "The total number of VA is: ";
	cout <<va_count;
	cout << "\n"; 
	
	//  cout << address_table[1].va;
	//  for (UINT32 i=0; i<128;i++)
	//  address_table.PRINT_ROW(i);
	
	/* for ( int i = 0; i < 10000; i++) 
	   {
	   cout << i; cout << " "; 
	   cout << count[i]; 
	   cout << " Same_thread \n";
	   }
	   
	   
	   for ( int i = 0; i < 10000; i++) 
	   {
	   cout << i; 
	   cout << " "; 
	   cout << D_count[i];
	   cout << " Different_thread\n"; 
	   }
	*/
      }
    //NKATAN
    
    if (verbose)
    {
        UINT64 totEvents = 0;

        fprintf(stderr, "EOF...\n");

        fprintf(stderr, "Events: ");
        for (unsigned int j = 1; j <= maxevents; j++)
        {
            totEvents += events[j];
            fprintf(stderr, lldstr, events[j]);
            fprintf(stderr, "  ");
        }

        fprintf(stderr, "  (");
        fprintf(stderr, lldstr, totEvents);
        fprintf(stderr, ")\n");
    }

    delete MingoFeederHandle;
    
    exit(0);
}

// Initialize performance model by instantiating common_systemple processor
ASIM_SYSTEM
SYS_Init (UINT32 argc, char *argv[], char *envp[])

{
    TRACE(Trace_Sys, cout << "Initializing common system.\n"); 

    //
    // Parse arguments
    //
    while (*argv)
    {
        char arg;

        if ((*argv)[0] != '-')
        {
            fprintf(stderr, "MINGO_DUMPER::ParseArgs -- Invalid argument (%s)\n", *argv);
            exit(1);
        }

        arg = (*argv)[1];

        if (*++argv == NULL)
        {
            fprintf(stderr, "MINGO_DUMPER::ParseArgs -- Expected argument to (%s)\n", *(argv-1));
            exit(1);
        }
            
        switch (arg)
        {
          case 'b':
            traceThreadBranches = atoi(*argv);
            verbose = 0;
            break;

          case 'e':
            traceEvents = atoll(*argv);
            break;

          default:
            fprintf(stderr, "MINGO_DUMPER::ParseArgs -- Invalid argument (%s)\n", *(argv-1));
            exit(1);
        }

        ++argv;
    }

    mingo_dumper_system =
        new ASIM_MINGO_DUMPER_SYSTEM_CLASS("Mingo_Dumper_System", 1);
    mingo_dumper_system->InitModule();
    return(mingo_dumper_system);
}
