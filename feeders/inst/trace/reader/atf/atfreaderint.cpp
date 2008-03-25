/*
 * Copyright (C) 2001-2006 Intel Corporation
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
 */
 
/**
 * @file
 * @author Artur Klauser
 */

// generic
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/trace.h"
#include "asim/thread.h"

// ASIM public modules
#include "asim/provides/traceinstruction.h"
#include "asim/provides/tracereader.h"

// ASIM local module
#include "atf.h"

int debug_level = 0;	/* ATF debugging level */

//--------------------------------------------------------------------------
// TraceReader Interface
//--------------------------------------------------------------------------

TRACE_READER_CLASS::TRACE_READER_CLASS ()
{
    ATF_init();
    printInstructions = false;
}

bool
TRACE_READER_CLASS::Init (
    bool skipIdle,   ///< skip over 'idle loop' instructions in trace
    UINT32 numCpus)  ///< only read instructions for the first numCpus
{
    this->numCpus = numCpus;
    this->skipIdle = skipIdle;
    idle_ctr = 0;

    // Initialize counters
    num_insts_fed     = (UINT64*)calloc(numCpus, sizeof (*num_insts_fed));
    num_insts_skipped = (UINT64*)calloc(numCpus, sizeof (*num_insts_skipped));
    if (!num_insts_fed || !num_insts_skipped)
    {
        ASIMERROR("TraceReader: No memory for instruction counters.\n");
    }

    return true;
}

void
TRACE_READER_CLASS::Done (void)
{
    UINT32 i;
    UINT64 t;

    for (i=0, t=0; i<numCpus; i++)
    {
        printf("num of insts fed to sim for cpu["FMT32U"] = "FMT64U"\n", 
               i, num_insts_fed[i]);
        t += num_insts_fed[i];
    }
    printf("total num of insts fed             = "FMT64U"\n", t);
    
    for (i=0, t=0; i<numCpus; i++)
    {
        printf("num of idle insts skipped for cpu["FMT32U"] = "FMT64U"\n", 
               i, num_insts_skipped[i]);
        t += num_insts_skipped[i];
    }
    printf("total num of idle insts skipped      = "FMT64U"\n", t);
}
  
bool
TRACE_READER_CLASS::FileOpen (
    INT32 streamId,
    const char * fileName)
{
    const bool isWrite = false;
    return ATF_file_open (streamId, fileName, isWrite);
}

void
TRACE_READER_CLASS::FileClose (
  INT32 streamId)
{
    ATF_file_close (streamId);
}

/*
 * Return the next instruction in the trace (we initialize
 * 'inst' and return a pointer to it, we don't allocate a
 * new object), or return NULL if there are no more instructions.
 */
void
TRACE_READER_CLASS::GetNextInst (
    INT32 streamId,
    TRACE_INST traceInst)
{
  AtfGenParam value1, value2, value3, value4;
  
  while (true) 
  {
    AtfAction_enum action =
        ATF_read_action(streamId, &value1, &value2, &value3, &value4);

    switch (action) 
    {
      case act_INSTR_new:
      case act_INSTR_OLD: 
      {
	UINT32 cpu = ATF_get_global(0, avg_CPU);

	// Ignore idle instructions?
	if (skipIdle && idle_ctr)
	{
	  num_insts_skipped[cpu]++;
	  //printf("Skipping idle instr: op=%lx, vpc=%lx\n",
          // traceInst->Opcode(),traceInst->VirtualPc().GetAddr());
	  continue;
	}
	
	// Ignore instructions from cpus we don't want
	else if (cpu >= numCpus)
	{
	  //printf("Skipping inst from cpu=%d\n", cpu);
	  continue;
	}

	else
	{
	  UINT64 vpc = value1.uint64;
	  Atf_Instr_Op_Etc *opePtr = (Atf_Instr_Op_Etc*)value2.ptr;
	  UINT64 vea = value3.uint64;
	  UINT64 pid = ATF_get_local(streamId, avl_PID);
	  UINT64 asn = ATF_get_local(streamId, avl_ASN);
	  UINT64 ppc = ATF_tlb_translate(streamId, vpc, true);
	  UINT64 pea = IsLoadStore(opePtr->opcode) ?
	    ATF_tlb_translate (streamId, vea, false/* is_Istream ? */) : 0;
          UINT32 opcode = opePtr->opcode;

	  //	 Create instruction and initialize fields...
          traceInst->Init(IADDR_CLASS(vpc), IADDR_CLASS(ppc), vea, pea,
              opcode, cpu);

	  if (IsLoadStore(opePtr->opcode))
	  {
//	    printf ("C%d Inum %010lu VEA %016lx PEA %016lx\n", 
//		    cpu, num_insts_fed[cpu], vea, pea);
	  }

	  if (printInstructions) 
	  {
	    UINT64 tid = ATF_get_local  (0, avl_TID);

	    printf ("act_INSTR C%d S"FMT32X" P"FMT64X" T"FMT64X" A"FMT64X": "
		    "VPC "FMT64X" PPC "FMT64X"  %-40s",
		    cpu, streamId, pid, tid, asn, vpc, ppc, 
		    disassemble (opePtr->opcode, vpc));
	    fflush(NULL);
	    
	    if (IsLoadStore(opePtr->opcode)) 
	    {
	      pea = ATF_tlb_translate(streamId, vea, false /* is_Istream */);
	      printf ("   ! VEA "FMT64X"  PEA "FMT64X"\n", vea, pea);
	    } 
	    else 
	    {
	      printf ("\n");
	    }
	  }

	  num_insts_fed[cpu]++;
	  
	  return;
	}
      }
      
      case act_SET: 
      {
	char *varname = "Unknown";
	AtfVariable_enum varnum = (AtfVariable_enum)value1.uint64;
	UINT64 value = value2.uint64;
	/* Based on what variable changed, do something... */
	/* (Yes this could be better done as a array, it's just an example) */
	switch (varnum) 
	{
	  case avg_CPU:		varname = "CPU#"; break;
	  case avg_EPOCH:	varname = "EPOCH"; break;
	  case avg_VERSION: 	varname = "VERSION"; break;
	  case avg_OS:		varname = "OS"; break;
	  case avl_TID:		varname = "TID"; break;
	  case avl_PID:		varname = "PID"; break;
	  case avl_ASN:		varname = "ASN"; break;
	  case avl_ASM:		varname = "ASM"; break;
	  case avl_SUPPAG:    	varname = "SUPPAG"; break;
	}
//	if (varnum != avg_CPU)
//	  TRACE(Trace_Feeder, printf("\tGetNextInst: "
//				     "SET changed var %d %s  = "FMT64D"\n",
//				     varnum, varname, value)); 
	break;
      }

      case act_PTE: 
      {
	UINT64 vpc = value1.uint64;	
	UINT64 pte = value2.uint64;	
	//TRACE(Trace_Feeder, printf("\tGetNextInst: PTE va="FMT64X
	//			   " pte="FMT64X"\n", vpc, pte)); 
	break;
      }

      case act_COMMENT:
	//TRACE(Trace_Feeder, printf("\tGetNextInst: COM %s\n", 
	//			   (char *)value1.ptr)); 
	break;

      case act_DMA:
      {
	UINT64 pa = value1.uint64;	
	UINT32 len = value2.uint64;	
	Boolean isWrite = value3.uint64;	
	//TRACE(Trace_Feeder, 
	//      printf ("act_DMA pa=%#lx len=%#x isWrite=%#x\n", pa, len, isWrite));
	break;
      }
      
      case act_DUMP:
      {
	UINT64 pa = value1.uint64;
	UINT64 va = value2.uint64;
	Boolean id = value3.uint64;
	// Cast added by Ahuja to get code to compile
	AtfDumpState_enum state = (AtfDumpState_enum)value4.uint64;
	//TRACE(Trace_Feeder, printf ("act_DUMP PA %016lx  VA %016lx  I/D%d  State%d\n", 
	//			    pa, va, id, state);
	break;
      }
      
      case act_BR_ACTION: 
      {
	// Cast added by Ahuja to get code to compile
	AtfBranchAction_enum action = (AtfBranchAction_enum)value1.uint64;
	//TRACE(Trace_Feeder, printf("\tact_BR_ACTION %s\n", 
	//	  (action==atf_br_act_	TAKEN ? "TAKEN" : "NOT_TAKEN"));
	break;
      }

      case act_INTERRUPT: 
      {
	//TRACE(Trace_Feeder, printf("\tact_INTERRUPT\n");
	break;
      }

      case act_PROC_TEXT_INFO: 
      {
	u64 pid = value1.uint64;
	UINT64 start = value2.uint64;
	UINT64 end = value3.uint64;
	//TRACE(Trace_Feeder, printf("\tact_PROC_TEXT_INFO: "
	//			   "pid=%lu text=%#lx-%#lx comm='%s'\n", 
	//			   pid, start, end, (char *)value4.ptr);
	break;
      }

      case act_PROC_BSS_INFO: 
      {
	u64 pid = value1.uint64;
	UINT64 start = value2.uint64;
	UINT64 end = value3.uint64;
	//TRACE(Trace_Feeder, printf ("act_PROC_BSS_INFO: "
	//			    "pid=%lu bss=%#lx-%#lx\n", 
	//			    pid, start, end);
	break;
      }

      case act_THREAD_USTACK_ADDR_INFO: 
      {
	u64 pid = value1.uint64;
	u64 tid = value2.uint64;
	UINT64 start = value3.uint64;
	UINT64 end = value4.uint64;
	//TRACE(Trace_Feeder, printf("\tact_THREAD_USTACK_ADDR_INFO: "
	//			   "pid=%lu tid=%#lx ustack=%#lx-%#lx\n", 
	//			   pid, tid, start, end);
	break;
      }

      case act_THREAD_USTACK_GROW_INFO: 
      {
	u64 pid = value1.uint64;
	u64 tid = value2.uint64;
	u64 size = value3.uint64;
	Boolean grow_up = value4.uint64;
	//TRACE(Trace_Feeder, printf("\tact_THREAD_USTACK_GROW_INFO: pid=%lu "
	//			   "tid=%#lx ustack_size=%lu grow=%s\n", 
	//			   pid, tid, size, grow_up ? "Up" : "Down");
	break;
      }
      
      case act_THREAD_KSTACK_INFO: 
      {
	u64 pid = value1.uint64;
	u64 tid = value2.uint64;
	UINT64 start = value3.uint64;
	UINT64 end = value4.uint64;
	//TRACE(Trace_Feeder, printf("\tact_THREAD_KSTACK_INFO: pid=%lu "
	//			   "tid=%#lx kstack=%#lx-%#lx\n", 
	//			   pid, tid, start, end);
	break;
      }

      case act_LIBLOADER_INFO: 
      {
	UINT64 text_start = value1.uint64;
	UINT64 text_end = value2.uint64;
	UINT64 bss_start = value3.uint64;
	UINT64 bss_end = value4.uint64;
	//TRACE(Trace_Feeder, printf ("act_LIBLOADER_INFO: text=%#lx-%#lx "
	//			    "bss=%#lx-%#lx\n", 
	//			    text_start, text_end, bss_start, bss_end);
	break;	
      }

      case act_LIB_INFO: 
      {
	UINT64 text_start = value1.uint64;
	UINT64 text_end = value2.uint64;
	UINT64 bss_start = value3.uint64;
	UINT64 bss_end = value4.uint64;
	//TRACE(Trace_Feeder, printf ("act_LIB_INFO: text=%#lx-%#lx "
	//			    "bss=%#lx-%#lx\n", 
	//			    text_start, text_end, bss_start, bss_end);
	break;
      }
      
      case act_KERNEL_SWITCH: 
      {
	Boolean in = value1.uint64;
	UINT64 addr = value2.uint64;
	//TRACE(Trace_Feeder, printf("act_KERNEL_SWITCH: %s addr=%#lx "
	//			   "comment='%s'\n", 
	//			   (in ? "IN" : "OUT"), addr, (char *)value3.ptr);
	break;	
      }
      
      case act_IDLE_THREAD: 
      {
	Boolean idle_start = value1.uint64;
	//TRACE(Trace_Feeder, printf("act_IDLE_THREAD %s\n", 
	//			   (idle_start ? "START" : "END")));
	//printf("act_IDLE_THREAD %s\n", (idle_start ? "START" : "END"));

	if (idle_start) {
	    idle_ctr++;
        }
	else
        {
	    idle_ctr--;
	    if (idle_ctr < 0) {
	        ASIMERROR("Mismatched IDLE STARTs and ENDs!\n");
            }
        }
	break;
      }

      case act_EOF:
      {
	traceInst->SetEof();
	//TRACE(Trace_Feeder, printf("\tGetNextInst: EOF\n")); 
	return;
      }
      
      default:
      {
	ASIMERROR("Unknown ATF action " << action << endl);
	break;
      }   
    }
  }
}
