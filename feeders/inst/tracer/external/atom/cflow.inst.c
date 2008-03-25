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
// Author:  Harish Patil
//


#ifndef _ATOMSTD_
#define _ATOMSTD_
#endif /* _ATOMSTD_ */
/*
 * Atom instrumentation code.
 */



#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include <cmplrs/atom.inst.h>
#include "cflowtypes.h"


static long instcount = 0;
static long blockcount = 0;
int printit = 0;
#define INFOF if(printit) fprintf

char * infilename;
long   FirstPC = 0;
long   LastPC = 0;

void InstrumentInit(
		       int argc,
		       char **argv
		       )
{
  int i;
  
  for(i = 0; i < argc; i++){
	INFOF(stdout,"argv[%d] = %s\n", i , argv[i]);
  }
  infilename = argv[0];
  AddCallProto("command_line(REGV)");
  AddCallProto("traceblock(int, int, long, int, long)");
  AddCallProto("traceinst(int , long , int , long )");
  AddCallProto("trace_load_store(int, long,  VALUE, long)");
  AddCallProto("GrabInstructions(char *, long, long, long)");

  AddCallProto("PIPE_Done(int)");
  AddCallProgram(ProgramAfter, "PIPE_Done", 0);

}

void InstrumentFini()
{
int i;
 printf("Instrumented %s %d instructions in %d blocks\n", infilename, instcount, blockcount);
 AddCallProgram (ProgramBefore, "GrabInstructions",infilename, instcount, FirstPC, LastPC); 
}

void Instrument(
		       int argc,
		       char **argv,
               Obj        *obj
		       )
{
  Proc       *proc;
  Block      *bb;
  Inst       *inst;
  ProcRes     proc_rsrc;
  long   PC;
  

  ResolveNamedProc("__start", &proc_rsrc);
    if (proc_rsrc.proc != NULL)
      AddCallProc(proc_rsrc.proc, ProcBefore, "command_line", REG_SP);
  ResolveNamedProc("_exit", &proc_rsrc);
    if (proc_rsrc.proc != NULL)
      AddCallProc(proc_rsrc.proc, ProcBefore, "PIPE_Done", 0);

    for (proc = GetFirstObjProc(obj); proc != NULL; proc = GetNextProc(proc)) {
      for (bb = GetFirstBlock(proc); bb != NULL; bb = GetNextBlock(bb)) {
        int instPerBlock = 0;
        int load_store_seen;
        Inst       *lastinst;

        instPerBlock = 0;
        load_store_seen = 0;
        blockcount++;
	for (inst = GetFirstInst(bb); inst != NULL; inst = GetNextInst(inst)) {
	  int instruction;
	  int opc,rega,regb,regc,blit,ffunct,disp,bdisp,blitvalue,hint;

	  instruction = GetInstInfo(inst,InstBinary);
	  PC = InstPC(inst);
          if(!FirstPC) FirstPC = PC;
          instPerBlock += 1;

	  rega = (instruction >> 21) & 0x1f;
	  regb = (instruction >> 16) & 0x1f;
	  regc = instruction & 0x1f;
	  blit = (instruction >> 12) & 1;
	  blitvalue = (instruction >> 13) & 0xff;
	  opc = (instruction >> 26) & 0x3f;
	  disp = instruction & 0x3fff;
	  bdisp = instruction & 0x1fffff;
	  hint = (instruction >> 14) & 0x3;
	  ffunct = (instruction >> 5) & 0x3f; /* this is the part of the fp funct field that determines function */
	  instcount++;
          if (IsInstType(inst,InstTypeLoad) || IsInstType(inst,InstTypeStore)) {
             AddCallInst(inst,InstBefore,"trace_load_store",0, PC, EffAddrValue, (PC - FirstPC)/4 - 1);
             load_store_seen = 1;
          }
          lastinst = inst;
      }
       assert(GetBlockInfo(bb,BlockNumberInsts) == instPerBlock);
       /* When do we trace instructions for a block? Before the block is executed or after?
        * Tracing instructions before: problem effective addresses of load stores not known.
        * Tracing instructions after: a basic block B with the last instruction which
        *        is a subroutine call causes a problem: the call leads to many dynamic block 
        *        executions;
        *        instructions from those blocks get traced before instructions from B because 
        *        the instrumentation is 'BlockAfter' and block B does not end till the call 
        *        returns.
        */                
       if(load_store_seen == 0) {
            AddCallBlock(bb,BlockBefore,"traceblock",0, blockcount, BlockPC(bb), instPerBlock,
                   (BlockPC(bb) - FirstPC)/4);
       } else {
            /* Give load/store(s) a chance to set vea */
            if( GetInstClass(lastinst) ==  ClassSubroutine){
	       int lastopcode;

	       lastopcode = GetInstInfo(lastinst,InstBinary);

               INFOF(stderr, "Last PC (0x%lx) a call in a block [PC= 0x%lx] with loads/stores \n",
                      PC, BlockPC(bb));
               /* trace everything but the last instruction with traceblock() with the right 
                * instruction count
                */
               AddCallInst(lastinst,InstBefore,"traceblock",0, blockcount, BlockPC(bb), instPerBlock - 1, (BlockPC(bb) - FirstPC)/4);
               /* trace last instruction */
               AddCallInst(lastinst,InstBefore,"traceinst",0, InstPC(lastinst), lastopcode, 0);
            } else {
AddCallBlock(bb,BlockAfter,"traceblock",0, blockcount, BlockPC(bb), instPerBlock, (BlockPC(bb) - FirstPC)/4 );
            }
            /* A ClassSubroutine instructions should be in  1-instruction basic blocks only */
       }
     }	
    }	
    LastPC = PC;
}
