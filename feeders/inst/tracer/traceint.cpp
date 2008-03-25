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
 
/*
 * Author:  Steven Wallace
 */


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "syntax.h"
#include "mesg.h"
#include "trace.h"
#include "instfeeder.h"
#include "cmd.h"
#include "alphaops.h"

#include "mopen.h"
#define MULTITHREADED  // indicate this is a pthread environment to use sigwait
#include "feedtrace.h"

static ALPHA_INSTRUCTION CreateRegOp (INT32 op, INT32 func, UINT32 rA, UINT32 rB, UINT32 rC);
static void traceint_GetNextInst (UINT32 sid, struct inst_info *inst);
static ASIM_THREAD traceint_StartNewThread (UINT32 sid, UINT64 vpc);
static UINT64 trace_Skip (ASIM_THREAD thread, UINT64 n);


#define MAX_THREADS 4

static TRACE_BUFFER trace;
static UINT32 wrongpath[MAX_THREADS];
static UINT32 justKilled[MAX_THREADS];
static UINT32 threads = 0;
static pid_t  simos_pid;

INT32 debug_level = 0;

#define IsPalCodeInst(addr)  (addr & 0x01)

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

ASIM_THREAD
traceint_StartNewThread (UINT32 sid, UINT64 vpc)
/*
 * Notify the PM that a new thread is starting at 'vpc'.
 */
{
    static UINT32 number = 0;
    UINT64 pid = 0;

    //
    // Create the object to represent this thread. Assign
    // a unique number to each new thread.

    ASIM_THREAD thread = new ASIM_THREAD_CLASS(number++, pid, vpc);

    //
    // Send it to the PM...

    TRACE(Trace_Feeder, printf("Starting new thread, pid = "FMT64U", at pc "FMT64X".\n", pid, vpc));

    CMD_ThreadBegin(thread);
    return(thread);
}


/********************************************************************
 *
 * FEEDer callbacks
 *
 *******************************************************************/

UINT64
FEED_Skip (ASIM_THREAD thread, UINT64 n, INT32 markerID)
{
        UINT64 Skipped = 0;
    //
    // If 'thread' == NULL, we skip every thread...

    // FIXME comment out
	fprintf(stdout, "Asked to skip %ld instructions for 0x%lx\n", n,
	thread);
    if (thread == NULL) {
        UINT64 minSkipped = UINT64_MAX;
        for (UINT32 i = 0; i < threads; i++) {
            if (!trace[i].Ended()) {
                UINT64 skipped = trace_Skip(trace[i].Thread(), n);
                minSkipped = MIN(skipped, minSkipped);
            }
        }
	fprintf(stdout, "Skipped %ld instructions for all threads\n", minSkipped);

        return(minSkipped);
    }

    Skipped = trace_Skip(thread, n);
	fprintf(stdout, "Skipped %ld instructions for thread %d\n", Skipped, n);
    return(Skipped);
}



static UINT64
trace_Skip (ASIM_THREAD thread, UINT64 n)
{
    UINT64 instructions = 0;
    UINT32 tid = thread->Uid();
    struct inst_info inst;

    while(instructions++ < n) {
      if(trace[tid].Eof())
        break;

      trace[tid].NextInstr();  // Go to next instruction in trace
    }

    thread->SetVirtualPc(trace[tid].VirtualPc());
    wrongpath[tid] = false;
    justKilled[tid] = false;
    return instructions-1;
}

UINT64
FEED_ITranslate(ASIM_THREAD thread, UINT64 vpc)
{
    // Since we don't have the ppc, we return the vpc
    return vpc; 
}

void
FEED_Marker (ASIM_MARKER_CMD cmd, ASIM_THREAD thread,
             UINT32 markerID, UINT64 markerPC,
             UINT32 instBits, UINT32 instMask)
{
  fprintf(stderr,"FEED_Marker is not supported on Tracer feeder\n");
  exit(1);
}

UINT64
FEED_Symbol (ASIM_THREAD thread, char* name)
{
  UINT32 tid = thread->Uid();
  return(FEED_Symbol(tid, name));
}

UINT64
FEED_Symbol (UINT32 tid, char* name)
{
  fprintf(stderr,"FEED_Symbol is not supported on Tracer feeder\n");
  exit(1);
  return 0;
}


void
FEED_Fetch (ASIM_THREAD thread, UINT64 predicted_pc, ASIM_INST inst)
/*
 * Fetch next instruction from 'thread'.
 */
{
    UINT64 pc;
    UINT32 sid = thread->Uid();

    inst->SetThread(thread);

    //
    // If we're at the end of the trace, then signal the end
    // of thread, and feed nops like going down a wrongpath.
    
    if(trace[sid].Eof()) {
        pc = predicted_pc;
        TRACE(Trace_Feeder, printf("Ending thread, uid = "FMT64U".\n", thread->Uid()));
        if (!trace[sid].Ended()) {
            CMD_ThreadEnd(thread);
            trace[sid].Ended() = true;
        }
    }
    else {
        pc = trace[sid].VirtualPc();  // get current PC
    }
    
    //
    // If 'predicted_pc' is not the pc of the next instruction
    // in the trace, then the previous instruction
    // must have mispredicted. Return a nop for now...
    TRACE(Trace_Feeder, printf("\tFEED_Fetch T"FMT32U":"FMT64X" index "FMT32U"\n",
          sid, predicted_pc, trace[sid].GetIdentifier()));
    if (pc != predicted_pc || wrongpath[sid]) {
        //
        // Create a NOP to fake down mispredicted path
        if(!wrongpath[sid]) {
            TRACE(Trace_Feeder, printf("\tWRONGPATH!\n"));
            VERIFYX(!justKilled[sid]);
        }

        TRACE(Trace_Feeder, printf("\tDETECT pc("FMT64X") != predicted_pc("FMT64X")\n", pc, predicted_pc));
        inst->SetInstrNoCache(predicted_pc, 0, 
			      CreateRegOp(BIT_OP, BIS_FUNC, ZERO_REG, ZERO_REG, ZERO_REG));
        inst->SetAtfIdentifier(TRACE_WRONGPATH_ID);
        wrongpath[sid] = true;
        return;
    }

    // the PM is still on trace, so grab the next instruction.
    //
    justKilled[sid] = false;
    inst->SetInstrNoCache(pc, 0, trace[sid].Opcode());
    inst->SetAtfIdentifier(trace[sid].GetIdentifier());
    
    if (inst->IsLoad() || inst->IsStore()) {
        // inst->SetPhysicalEffAddress(TRACE_tlb_translate(sid, vea, false));
	
	// Sythesize a physical address
	// Why are we hardwiring "42" in here??? 
	// We should really set the address to the physical address SimOS really supplies ... 

	//FIXME: Harish commented out due to compiler error. 
        // inst->SetPhysicalEffAddress(inst->Thread()->Pid() << 42 + trace[sid].VirtualEffAddress); 
        inst->SetVirtualEffAddress(trace[sid].VirtualEffAddress());
    }

    UINT32 trap = trace[sid].Trap();         // Get trap from current inst
    trace[sid].NextInstr();                  // Go to next instruction in trace
    UINT64 nextpc = trace[sid].VirtualPc();  // Get pc of subsequent instr

#if 0
    // Force trap on retire every 100th instruction
    if(!trap && (trace[sid].GetIdentifier() % 100) == 99)
        trap = 0x501;
#endif

    if (inst->IsBranch() || inst->IsJump()) {
        // here we need to look at the next instruction in the trace
        // to get the actual target pc of the jump...
        inst->SetActualTarget(nextpc);
        TRACE(Trace_Feeder, printf("\tACTUALTARGET = "FMT64X"\n", nextpc));
    }
    else if (!trap && nextpc != pc + 4) {
        inst->SetNonSequentialPc(nextpc, trap);
        TRACE(Trace_Feeder, printf("\tUNKOWN NONSEQUENTIAL PC from 0x"FMT64X":0x"FMT64X" to 0x"FMT64X"\n",
              pc, inst->Instr(), nextpc));
    }
    if(trap) {
        inst->SetNonSequentialPc(nextpc, trap);
        TRACE(Trace_Feeder, printf("\tTRAP = "FMT32X" TRAPTARGET = 0x"FMT64X"\n", trap, nextpc));
    }
}


void
FEED_Issue (ASIM_INST inst)
/*
 * Issue 'inst'...
 */
{
}


void
FEED_DoRead (ASIM_INST inst)
/*
 * 'inst' is a load accessing the cache
 */
{
}


void
FEED_DoSpecWrite (ASIM_INST inst)
/*
 * Inst is a store making the stored value non-speculative
 */
{
}

void
FEED_DoWrite (ASIM_INST inst)
/*
 * Inst is a store making the stored value non-speculative
 */
{
}


void
FEED_Commit (ASIM_INST inst)
/*
 * 'inst' is committed...
 */
{
        // printf("FEED_Commit: instruction "FMT64X": %s\n",
                  // inst->VirtualPc(), inst->Disassemble());
    if (inst->GetAtfIdentifier() == TRACE_WRONGPATH_ID) {
        FEED_Done();   // Call here so SimOS won't still be running
        ASIMERROR("FEED_Commit: Attempt to commit wrongpath instruction "FMT64X": %s\n",
                  inst->VirtualPc(), inst->Disassemble());
    }
    
    trace[inst->Thread()->Uid()].CommitInstr(inst->GetAtfIdentifier());
}


void
FEED_Kill (ASIM_INST inst, bool mispredict, bool killMe)
/*
 * kill 'inst'...
 */
{
    UINT64 killPc;
    UINT64 nextPc;

    TRACE(Trace_Feeder, printf("\tFEED_Kill id="FMT64U" mp=%d\n", inst->GetAtfIdentifier(), mispredict));
    if (inst->GetAtfIdentifier() != TRACE_WRONGPATH_ID) {
        wrongpath[inst->Thread()->Uid()] = false;
        justKilled[inst->Thread()->Uid()] = true;

        trace[inst->Thread()->Uid()].Backup(inst->GetAtfIdentifier());
        killPc = trace[inst->Thread()->Uid()].VirtualPc();
        trace[inst->Thread()->Uid()].NextInstr();
        nextPc = trace[inst->Thread()->Uid()].VirtualPc();

        if(!mispredict) {
            trace[inst->Thread()->Uid()].PrevInstr();
        }
    }
}

/********************************************************************
 *
 * Controller calls
 *
 *******************************************************************/
static INT32
parse_arg_string(char *c, char *argv[], char **stdin_name, char **stdout_name,
                 char **stderr_name)
{
    INT32 argc = 0;

    while (*c) {
        /*
         * Skip whitespace
         */
        while (*c && *c == ' ') c++;
        
        if (*c) {
            argv[argc++] = c;
            
            /*
             * Find the end of the string.  If the string starts with < or
             * > then it is a file redirection token.  End the argument there.
             */
            if (*c == '<') {
                c++;
                while (*c && *c == ' ') c++;
                *stdin_name = c;
                argv[--argc] = 0;
            }
            else if (*c == '>') {
                c++;
                while (*c && *c == ' ') c++;
                *stdout_name = c;
                argv[--argc] = 0;
            }
            
            while (*c && *c != ' ') c++;
            if (*c) {
                *c++ = 0;
            }
        }
    }
    
    return argc;
}

bool
FEED_Init (UINT32 argc, char **fargv, char **fenvp)
/*
 * Initialize trace instruction feeder. Return false
 * if we have an error during initialization.
 */
{
    INT32 i, j;
    char *argv[32];
    char buffaddr[32];
    char tempname[] = "/tmp/asim-tracebuffer.XXXXX";
    char buffname[] = "-buffname";
    char buffstr[]  = "-buffaddr";
    char *main_buffer, *buffer, *pipename = 0;
    char *feeder_name = fargv[0];
    char *stdin_name = NULL;
    char *stdout_name = NULL;
    char *stderr_name = NULL;
    

    threads = 1;

    atexit(FEED_Done);

    for (i = 1, j = 0; i < argc; i++) {
        if(strcmp(fargv[i], "-threads") == 0)
            threads = strtol(fargv[++i], NULL, 0);
	else if(strcmp(fargv[i], "-buffname") == 0) {
            i++;
	    if(fargv[i][0] == 'X' && fargv[i][1] == 0)
	        pipename = mktemp(tempname);
	    else
	        pipename = fargv[i];
	}
        else if(!stdin_name && strcmp(fargv[i], "-stdin") == 0)
            stdin_name = fargv[++i];
        else if(!stdout_name && strcmp(fargv[i], "-stdout") == 0)
            stdout_name = fargv[++i];
        else if(!stderr_name && strcmp(fargv[i], "-stderr") == 0)
            stderr_name = fargv[++i];
        else if(strcmp(fargv[i], "-p") == 0) {
            // parse string c for arguments to feeder program
            j += parse_arg_string(fargv[++i], &argv[j], &stdin_name,
                                  &stdout_name, &stderr_name);
        }
        else {
            // end of special tracer interface dash args
            // the remainder are feeder program arguments
            for (; i < argc; i++)
                argv[j++] = fargv[i];
            break;
        }
    }

#if defined(HOST_FREEBSD_X86) || defined(HOST_FREEBSD)
    // For those of us who can't MAP_INHERIT anonymous mappings, force map of file
    if(!pipename) {
        pipename = mktemp(tempname);
    }
#endif
    if(pipename) {
	argv[j++] = buffname;
	argv[j++] = pipename;
    }

    // Map shared segment into memory via a file or anonymous mapping
    if(pipename) {
        main_buffer = (char *)open_mpipe(pipename, 0x40000);
    }
    else {
        main_buffer = (char *)open_mpipe_anon(0x40000);
	argv[j++] = buffstr;
	argv[j++] = buffaddr;
	sprintf(buffaddr, "%ld", main_buffer);
    }
    
    if(!main_buffer)
      return false;  // Failed to allocate shared memory

    *(UINT64 *)main_buffer = 0;

    argv[j++] = 0;  // End of passed argments

    if(!(simos_pid = vfork())) {
        int fd;

        // child
        if(stdin_name && ((fd = open(stdin_name, O_RDONLY)) >= 0)) {
            dup2(fd, 0);
            close(fd);
        }
        if(stdout_name && ((fd = open(stdout_name, O_WRONLY|O_CREAT, 0666)) >= 0)) {
            dup2(fd, 1);
            close(fd);
        }
        if(stderr_name && ((fd = open(stderr_name, O_WRONLY|O_CREAT, 0666)) >= 0)) {
            dup2(fd, 2);
            close(fd);
        }

        if(execve(feeder_name, argv, fenvp) < 0)
            perror("exec");
    }

    // Parent continues

    //
    // Read the first instruction in the trace file. From this
    // instruction we can create the ASIM_TRACE object for
    // the trace containing the instruction.

    buffer = main_buffer;
    trace = new TRACE_BUFFER_CLASS[threads];
    for(UINT32 t = 0; t < threads; t++) {
        printf("Feeder: initting trace buffer %d\n", t);
        trace[t].InitTracer(t, 0, &buffer);
    }
    *(UINT64 *)buffer = 0;
    fprintf(stdout, "Feeder: finished init\n");

    for(UINT32 t = 0; t < threads; t++) {
        printf("Feeder: starting trace buffer %d\n", t);
        trace[t].StartTracer();
        if (trace[t].VirtualPc() == UINT64_MAX || trace[t].Eof()) {
            trace[t].Ended() = true;
            fprintf(stdout, "Trace "FMT32U" contains no instructions.\n", t);
            return(false);
        }
        //
        // Notify the PM that a new thread has started.
        trace[t].Thread() = traceint_StartNewThread(t, trace[t].VirtualPc());
    }
    fprintf(stdout, "Feeder: started traces\n");

    return(true);
}


void
FEED_Done (void)
/*
 * Cleanup..
 */
{
    static bool called_before;
    // First check to see if we were called before, otherwise continue shutdown
    if(called_before)
      return;
    called_before = true;

    kill(simos_pid, SIGTERM);
}


void
FEED_Usage (FILE *file)
/*
 * Print usage...
 */
{
    fprintf(file, "\nFeeder usage: <feederprogram> [-threads <#>] [-buffname X/<filname>] <feederarg0> <feederargs> ...\n");
}
