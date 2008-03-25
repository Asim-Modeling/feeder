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
 
//
// Authors:  David Goodwin, Steven Wallace, Pritpal Ahuja
//

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "syntax.h"
#include "alphaops.h"

#include "atf.h"
#include "disasm.h"
#include "mopen.h"
#include "mpipe.h"

#define ASIMERROR    printf
#define ASIMWARNING  printf

static ALPHA_INSTRUCTION CreateRegOp (INT32 op, INT32 func, UINT32 rA, UINT32 rB, UINT32 rC);
static bool atfint_GetNextInst (UINT32 sid, struct inst_info *inst);
static bool Parse (UINT32 argc, char **argv);

bool FEED_Init (UINT32 argc, char **argv, char **envp);
void FEED_Pipe ();
void FEED_Done ();

static PIPE_BUFFER pbufs;
static UINT32 threads = 0;
static UINT32 cpus = 10;

static INT32 print_instructions = 0;
static INT32 skip_idle = 1;

static UINT64 *num_insts_fed;
static UINT64 *num_insts_skipped;

static char *main_buffer;
static char *pipename;

INT32 debug_level = 0;

struct inst_info {
    UINT64 vpc;
    UINT64 opcode;
    UINT64 vea;
    UINT64 info;
};

static ALPHA_INSTRUCTION
CreateRegOp (INT32 op, INT32 func, UINT32 rA, UINT32 rB, UINT32 rC)
/*
 *  Generic 3 address operation Ra,Rb,Rc
 */
{
    ALPHA_INSTRUCTION ainst;

    ainst.OpReg.Opcode = op;
    ainst.OpReg.Function = func;
    ainst.OpReg.SBZ = 0;
    ainst.OpReg.RbvType = 0;
    ainst.OpReg.Ra = rA;
    ainst.OpReg.Rb = rB;
    ainst.OpReg.Rc = rC;
    
    return(ainst);
}

static INT64
IsLoadStore(UINT64 inst) 
{
  // True for LDQ_U STQ_U LDF/G/S/T ST F/G/S/T LDL/Q/L_L/Q_L STL/Q/L_C/Q_C
  //          0b    0f    20..27               28..2f
  // False for all others, including LDA LDAH MISC JSR
  // False for PAL LD/ST physical, etc.
  UINT64 opcode = OPCODE(inst);
  return((INT64) ( ((UINT64) UINT64_CONST(0x0000ffff0000fc00) >> (UINT64) opcode) &
		   (UINT64)1 ));
}

static bool
IsPalCodeInst(UINT64 addr)
{
  return ((addr & 0x01) == 0x01);
}

static bool
atfint_GetNextInst (UINT32 sid, struct inst_info *inst)
/*
 * Return the next instruction in the trace (we initialize
 * 'inst' and return a pointer to it, we don't allocate a
 * new object), or return NULL if there are no more instructions.
 */
{
  ulong value1, value2, value3, value4;
  static INT32 idle_ctr = 0;
  
  while (true) 
  {
    AtfAction_enum action = ATF_read_action(sid, &value1, &value2, &value3, &value4);

    switch (action) 
    {
      case act_INSTR_new:
      case act_INSTR_OLD: 
      {
	UINT32 cpu = ATF_get_global(0, avg_CPU);

	// Ignore idle instructions?
	if (skip_idle && idle_ctr)
	{
	  num_insts_skipped[cpu]++;
	  //printf("Skipping idle instr: op=%lx, vpc=%lx\n", inst->opcode, inst->vpc);
	  continue;
	}
	
	// Ignore instructions from cpus we don't want
	// Be carefull:  if trace:<stream> is specified, then we could want a CPU other
	// than 0
	else if (cpu >= cpus)
	{
	  //printf("Skipping inst from cpu=%d\n", cpu);
	  continue;
	}

	else
	{
	  UINT64 vpc = value1;
	  Atf_Instr_Op_Etc *opePtr = (Atf_Instr_Op_Etc*)value2;
	  UINT64 vea = value3;
	  UINT64 pid = ATF_get_local(sid, avl_PID);
	  UINT64 asn = ATF_get_local(sid, avl_ASN);
	  UINT64 ppc = ATF_tlb_translate(sid, vpc, true);
	  UINT64 pea = IsLoadStore(opePtr->opcode) ?
	    ATF_tlb_translate (sid, vea, false/* is_Istream ? */) : 0;

	  //	 Create instruction and initialize fields...
	  inst->opcode = opePtr->opcode;
	  inst->vpc = vpc;
//	  inst->ppc = ppc;
	  inst->vea = vea;
	  inst->info = 0;
//	  inst->cpu = cpu;
//	  inst->pea = pea;

	  if (IsLoadStore(opePtr->opcode))
	  {
//	    printf ("C%d Inum %010lu VEA %016lx PEA %016lx\n", 
//		    cpu, num_insts_fed[cpu], vea, pea);
	  }

	  if (print_instructions) 
	  {
	    UINT64 tid = ATF_get_local  (0, avl_TID);

	    printf ("act_INSTR C%d S%04x P%04x T%011lx A%04x: VPC %016lx "
		    "PPC %016lx  %-40s",
		    cpu, sid, pid, tid, asn, vpc, ppc, 
		    disassemble (opePtr->opcode, vpc));
	    fflush(NULL);
	    
	    if (IsLoadStore(opePtr->opcode)) 
	    {
	      pea = ATF_tlb_translate(sid, vea, false /* is_Istream */);
	      printf ("   ! VEA %016lx  PEA %016lx\n", vea, pea);
	    } 
	    else 
	    {
	      printf ("\n");
	    }
	  }

	  num_insts_fed[cpu]++;
	  
	  return 1;
	}
      }
      
      case act_SET: 
      {
	char *varname = "Unknown";
	AtfVariable_enum varnum = (AtfVariable_enum)value1;
	UINT64 value = value2;
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
//	  TRACE(Trace_Feeder, printf("\tatfint_GetNextInst: "
//				     "SET changed var %d %s  = "FMT64D"\n",
//				     varnum, varname, value)); 
	break;
      }

      case act_PTE: 
      {
	UINT64 vpc = value1;	
	UINT64 pte = value2;	
	//TRACE(Trace_Feeder, printf("\tatfint_GetNextInst: PTE va="FMT64X
	//			   " pte="FMT64X"\n", vpc, pte)); 
	break;
      }

      case act_COMMENT:
	//TRACE(Trace_Feeder, printf("\tatfint_GetNextInst: COM %s\n", 
	//			   (char *)value1)); 
	break;

      case act_DMA:
      {
	UINT64 pa = value1;	
	UINT32 len = value2;	
	Boolean isWrite = value3;	
	//TRACE(Trace_Feeder, 
	//      printf ("act_DMA pa=%#lx len=%#x isWrite=%#x\n", pa, len, isWrite));
	break;
      }
      
      case act_DUMP:
      {
	UINT64 pa = value1;
	UINT64 va = value2;
	Boolean id = value3;
	// Cast added by Ahuja to get code to compile
	AtfDumpState_enum state = (AtfDumpState_enum)value4;
	//TRACE(Trace_Feeder, printf ("act_DUMP PA %016lx  VA %016lx  I/D%d  State%d\n", 
	//			    pa, va, id, state);
	break;
      }
      
      case act_BR_ACTION: 
      {
	// Cast added by Ahuja to get code to compile
	AtfBranchAction_enum action = (AtfBranchAction_enum)value1;
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
	u64 pid = value1;
	UINT64 start = value2;
	UINT64 end = value3;
	//TRACE(Trace_Feeder, printf("\tact_PROC_TEXT_INFO: "
	//			   "pid=%lu text=%#lx-%#lx comm='%s'\n", 
	//			   pid, start, end, (char *)value4);
	break;
      }

      case act_PROC_BSS_INFO: 
      {
	u64 pid = value1;
	UINT64 start = value2;
	UINT64 end = value3;
	//TRACE(Trace_Feeder, printf ("act_PROC_BSS_INFO: "
	//			    "pid=%lu bss=%#lx-%#lx\n", 
	//			    pid, start, end);
	break;
      }

      case act_THREAD_USTACK_ADDR_INFO: 
      {
	u64 pid = value1;
	u64 tid = value2;
	UINT64 start = value3;
	UINT64 end = value4;
	//TRACE(Trace_Feeder, printf("\tact_THREAD_USTACK_ADDR_INFO: "
	//			   "pid=%lu tid=%#lx ustack=%#lx-%#lx\n", 
	//			   pid, tid, start, end);
	break;
      }

      case act_THREAD_USTACK_GROW_INFO: 
      {
	u64 pid = value1;
	u64 tid = value2;
	u64 size = value3;
	Boolean grow_up = value4;
	//TRACE(Trace_Feeder, printf("\tact_THREAD_USTACK_GROW_INFO: pid=%lu "
	//			   "tid=%#lx ustack_size=%lu grow=%s\n", 
	//			   pid, tid, size, grow_up ? "Up" : "Down");
	break;
      }
      
      case act_THREAD_KSTACK_INFO: 
      {
	u64 pid = value1;
	u64 tid = value2;
	UINT64 start = value3;
	UINT64 end = value4;
	//TRACE(Trace_Feeder, printf("\tact_THREAD_KSTACK_INFO: pid=%lu "
	//			   "tid=%#lx kstack=%#lx-%#lx\n", 
	//			   pid, tid, start, end);
	break;
      }

      case act_LIBLOADER_INFO: 
      {
	UINT64 text_start = value1;
	UINT64 text_end = value2;
	UINT64 bss_start = value3;
	UINT64 bss_end = value4;
	//TRACE(Trace_Feeder, printf ("act_LIBLOADER_INFO: text=%#lx-%#lx "
	//			    "bss=%#lx-%#lx\n", 
	//			    text_start, text_end, bss_start, bss_end);
	break;	
      }

      case act_LIB_INFO: 
      {
	UINT64 text_start = value1;
	UINT64 text_end = value2;
	UINT64 bss_start = value3;
	UINT64 bss_end = value4;
	//TRACE(Trace_Feeder, printf ("act_LIB_INFO: text=%#lx-%#lx "
	//			    "bss=%#lx-%#lx\n", 
	//			    text_start, text_end, bss_start, bss_end);
	break;
      }
      
      case act_KERNEL_SWITCH: 
      {
	Boolean in = value1;
	UINT64 addr = value2;
	//TRACE(Trace_Feeder, printf("act_KERNEL_SWITCH: %s addr=%#lx "
	//			   "comment='%s'\n", 
	//			   (in ? "IN" : "OUT"), addr, (char *)value3);
	break;	
      }
      
      case act_IDLE_THREAD: 
      {
	Boolean idle_start = value1;
	//TRACE(Trace_Feeder, printf("act_IDLE_THREAD %s\n", 
	//			   (idle_start ? "START" : "END")));
	//printf("act_IDLE_THREAD %s\n", (idle_start ? "START" : "END"));

	if (idle_start)
	  idle_ctr++;
	else
	  {
	    idle_ctr--;
	    if (idle_ctr < 0)
	      ASIMERROR("Mismatched IDLE STARTs and ENDs!\n");
	  }
	break;
      }

      case act_EOF:
      {
	inst->vpc = 0;
	//TRACE(Trace_Feeder, printf("\tatfint_GetNextInst: EOF\n")); 
	//printf("\tatfint_GetNextInst: EOF\n");
	return 0;
      }
      
      default:
      {
	ASIMERROR("Unknown ATF action "FMT32D".\n", action);
	break;
      }   
    }
  }
}


static bool 
Parse (UINT32 argc, char **argv)
/*
 * Parse command line arguments, return false if there is
 * a parse error.
 */
{
  bool success = false;

  for (INT32 i = 0; i < argc; i++) 
  {
    if (!strcmp(argv[i], "-no_skip_idle"))
    {
      skip_idle = 0;
      continue;
    }
    
    if (!strcmp(argv[i], "-cpus"))
    {
      if (i < argc)
	cpus = strtol(argv[++i], NULL, 0);

      if (cpus < 1)
	ASIMERROR("Invalid number of cpus ('%d') specified in command line\n",
		  cpus);
      continue;
    }
    
    if (!strcmp(argv[i], "-buffaddr"))
    {
      if (i < argc)
	main_buffer = (char *)strtol(argv[++i], NULL, 0);

      continue;
    }
    
    if (!strcmp(argv[i], "-buffname"))
    {
      if (i < argc)
	pipename = argv[++i];

      continue;
    }
    
    // Try to open the trace file...
    success = ATF_file_open(threads++, argv[i], false);
    if (!success) 
    {
      fprintf(stderr,"Couldn't open ATF file %s\n", argv[i]);
      return(false);
    }
  }

  // Make sure a file was specified.
  if (!success) 
  {
    ASIMWARNING("Trace file not specified\n");
    return(false);
  }

  return(true);
}


int
main (UINT32 argc, char **argv, char **envp)
{
    if(!FEED_Init(argc-1, &argv[1], envp))
        exit(-1);
    FEED_Pipe();
    FEED_Done();
}

/********************************************************************
 *
 * Controller calls
 *
 *******************************************************************/

bool
FEED_Init (UINT32 argc, char **argv, char **envp)
/*
 * Initialize atf instruction feeder. Return false
 * if we have an error during initialization.
 */
{
    char *buffer;

    ATF_init();

    //
    // Parse the command-line.

    if (!Parse(argc, argv))
      return(false);

    // Initialize counters
    num_insts_fed     = (UINT64*)calloc(cpus, sizeof (*num_insts_fed));
    num_insts_skipped = (UINT64*)calloc(cpus, sizeof (*num_insts_skipped));
    if (!num_insts_fed || !num_insts_skipped)
      ASIMERROR("Calloc failure\n");

    buffer = main_buffer;
    if(!buffer && pipename)
        buffer = (char *)open_mpipe(pipename, 0x40000);
    if(!buffer)
        return(false);  // Failed to find/map buffer space

    while(!(volatile UINT64)(*(UINT64 *)buffer)) ;

    pbufs = new PIPE_BUFFER_CLASS[threads];
    for(UINT32 t = 0; t < threads; t++) {
        printf("Atftracer: innitting pipe %d\n", t);
        buffer += pbufs[t].InitPipe(buffer, 0);
    }

    printf("Atftracer: done init\n");

    return(true);
}

void
FEED_Pipe ()
{
    bool    wrote_something = false;
    bool    eof[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    UINT64  min = 0;
    UINT32  eofcount = 0;

    // TODO: should write as much as we can for each thread before checking
    // next one for better performance.
    while(1) {
        for(UINT32 t = 0; t < threads; t++) {
	    // Check to see if buffer space avail for a write.
	    // If so, then get instruction from ATF trace and insert into buf.
	    // Initially don't sleep if there is no space to write
	    if(eof[t])
	        continue;
	    if(pbufs[t].GetMaxWriteSize(min) >= sizeof(struct inst_info)) {
	        if(atfint_GetNextInst(t,
		   (struct inst_info *)pbufs[t].GetWritePtr())) {
		    pbufs[t].IncWritePtr(sizeof(struct inst_info));
		    wrote_something = true;
		    min = 0;
		}
		else {
		    eof[t] = true;
		    pbufs[t].SetEOF();
		    if(++eofcount >= threads)
		        return;
		}
		
	    }
	}

	if(wrote_something)
	    wrote_something = false, min = 0;
	else
	    // Next time through loop, suspend if can't write
	    min = sizeof(struct inst_info);
    }
}


void
FEED_Done (void)
/*
 * Cleanup..
 */
{
  UINT32 i;
  UINT64 t;
  
  for(t=0; t<threads; t++)
    ATF_file_close(t);

  for (i=0, t=0; i<cpus; i++)
  {
    printf("num of insts fed to sim for cpu["FMT32U"] = "FMT64U"\n", 
	   i, num_insts_fed[i]);
    t += num_insts_fed[i];
  }
  printf("total num of insts fed             = "FMT64U"\n", t);
  
  for (i=0, t=0; i<cpus; i++)
  {
    printf("num of idle insts skipped for cpu["FMT32U"] = "FMT64U"\n", 
	   i, num_insts_skipped[i]);
    t += num_insts_skipped[i];
  }
  printf("total num of idle insts skipped      = "FMT64U"\n", t);
  
}


void
FEED_Usage (FILE *file)
/*
 * Print usage...
 */
{
    fprintf(file, "\nFeeder usage: ... <tracefile> [<tracefile> ...] ...\n");
    fprintf(file, "\tFilenames may trail .gz for piping through gunzip\n");
    fprintf(file, "\tFilenames may end in :# to limit to one CPU's trace\n");
}
