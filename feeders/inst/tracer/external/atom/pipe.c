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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <filehdr.h>
#include <syms.h>
#include <ldfcn.h>
#include <assert.h>

#include "mpipe-c.h"
#include "mopen.h"
#include "cflowtypes.h"


struct inst_info {
    unsigned long vpc;
    unsigned long opcode;
    unsigned long vea;
    unsigned long info;
};

int maxcpu = -1;
int printit = 0;
int pipefd[8];

static char *main_buffer;
static char *pipename;
static int   threads = 0;
static long   instcount = 0;
static long FirstPC;
static long FirstInstrumentedPC;
static long LastPC;
static unsigned int *text = NULL; 
static unsigned int *instructions = NULL; 
static unsigned int *init = NULL; 
static unsigned int *fini = NULL; 
static long *global_vea = NULL; 

#define MAXCPU 4
static int                   write_size[MAXCPU];
static struct inst_info     *write_ptr[MAXCPU];
static struct mpipe_private *privs[MAXCPU];
static struct pipe_buffer   *pbufs[MAXCPU];

#define INFOF if(printit) fprintf
#define INFOATOM if(printit) fprintf 
/* #define INFOATOM fprintf*/
#define IOUT  stdout

void
traceinst(int cpu, unsigned long PC, unsigned int opcode, unsigned long vea)
{
  struct inst_info *cur_inst;

  INFOATOM(IOUT, "C%d:0x%8lx: %8x\n", cpu, PC, opcode);

  if(cpu > maxcpu || write_size[cpu] < 0)
    return;
  instcount++;

  cur_inst = write_ptr[cpu] = (struct inst_info *)
    MPIPE_IncWritePtr(write_size[cpu], privs[cpu], pbufs[cpu]);

  write_size[cpu] = sizeof(struct inst_info);
  while(!MPIPE_GetMaxWriteSize(write_size[cpu], privs[cpu], pbufs[cpu])) ;

  cur_inst->vpc = PC;
  cur_inst->opcode = opcode;
  cur_inst->vea = vea;
  cur_inst->info = 0;
}

void
traceblock1(int cpu, int bno, long pc, int Num_Insns)
{
  int i;

  INFOATOM(IOUT, "Block # %d  PC 0x%lx no_insts = %d\n", bno, pc, Num_Insns);
  for ( i = 1; i <= Num_Insns; i++){
    long index;
    index = (pc - FirstInstrumentedPC)/4 + (i - 1);
    INFOATOM(IOUT, "\tInstruction %d : %x index %d \n", i, instructions[index], index ); 
     traceinst(cpu, pc + (i - 1)*4, instructions[index], global_vea[index]);
  }
}

void
trace_load_store1(int cpu, long pc,  long vea)
{
    long index;
  INFOATOM(IOUT, "trace_load_store: pc 0x%lx vea 0x%lx\n", pc, vea);
    index = (pc - FirstInstrumentedPC)/4 - 1;
  global_vea[index] = vea;
}

void
traceblock(int cpu, int bno, long pc, int Num_Insns, long blockindex)
{
  int i;

  INFOATOM(IOUT, "Block # %d  PC 0x%lx no_insts = %d blockindex %d\n", bno, pc, Num_Insns, blockindex);
  for ( i = 1; i <= Num_Insns; i++){
    INFOATOM(IOUT, "\tInstruction %d : %x index %d \n", i, instructions[blockindex + i - 1], blockindex + i -1 ); 
     traceinst(cpu, pc + (i - 1)*4, instructions[blockindex + i - 1], global_vea[blockindex + i - 1]);
  }
}

void
trace_load_store(int cpu, long pc,  long vea, long index)
{
  INFOATOM(IOUT, "trace_load_store: pc 0x%lx vea 0x%lx index %d\n", pc, vea, index);
  global_vea[index] = vea;
}

void
asim_trace_set_vea(int cpu, unsigned long vea)
{
  if(cpu > maxcpu)
    return;

  INFOF(IOUT, "C%d: vea=%8lx\n\r", cpu, vea);
  write_ptr[cpu]->vea = vea;
}

void
asim_trace_trap(int cpu, int trap)
{
  if(cpu > maxcpu)
    return;

  INFOF(IOUT, "C%d: trap=%x\n\r", cpu, trap);
  write_ptr[cpu]->info = trap;
}

void
PIPE_Done(int cpu)
{
  INFOATOM(IOUT, "Pipetracer: C%d done\n\r", cpu);
  INFOATOM(IOUT, "Processed %d insts\n\r", instcount);

  if(cpu > maxcpu || write_size[cpu] < 0)
    return;

  MPIPE_IncWritePtr(sizeof(struct inst_info), privs[cpu], pbufs[cpu]);
  MPIPE_SetEOF(privs[cpu], pbufs[cpu]);
  write_size[cpu] = -1;

} /* PIPE_Done() */

int
PIPE_Parse(int argc, char *argv[])
{
  int i;

  for (i = 0; i < argc; i++) 
  {
    INFOF(IOUT, "Pipetracer: Parsing argument %s\n", argv[i]);
    if (!strcmp(argv[i], "-cpus") || !strcmp(argv[i], "-threads"))
    {
      if (i < argc)
    threads = strtol(argv[++i], NULL, 0);
    }
    else if (!strcmp(argv[i], "-printit"))
    {
      printit = 1;
    }
    else if (!strcmp(argv[i], "-silent"))
    {
      printit = 0;
    }
    else if (!strcmp(argv[i], "-nobuffer"))
    {
      main_buffer = (char *)-1;
    }
    else if (!strcmp(argv[i], "-buffaddr"))
    {
      if (i < argc)
    main_buffer = (char *)strtol(argv[++i], NULL, 0);
    }
    else if (!strcmp(argv[i], "-buffname"))
    {
      if (i < argc)
    pipename = argv[++i];
    }
    else
      return i;
  }
  return i;
} /* Parse() */

int
PIPE_Init()
{
  char *buffer;
  int  t;

  pipefd[0] = -1;
  if(!threads)
    threads = 1;
  if(maxcpu < 0)
    maxcpu = threads - 1;

  buffer = main_buffer;
  if(!buffer && pipename)
    buffer = (char *)open_mpipe(pipename, 0 /* use default length */);
  if(buffer == (char *)-1)
    return 0;
  if(!buffer) {
    /* Failed to find/map buffer space, so use default address */
    buffer = (char *)MOPEN_ADDR_DEFAULT;
    INFOF(IOUT, "WARNING:  Using default pipe buffer address of %lx\n",
      (long)buffer);
  }
  
  while(!*(volatile UINT64 *)buffer) ;
  
  for(t = 0; t < threads; t++) {
    INFOF(IOUT, "Pipetracer: innitting pipe %d\n", t);
    pbufs[t] = (struct pipe_buffer *)buffer;
    buffer += MPIPE_InitPipe(buffer, 0, (void **)&privs[t]);
  }

  INFOF(IOUT, "Pipetracer: done init\n");

  return 1;
} /* PIPE_Init() */

int
PIPE_Parse_Init(int argc, char *argv[])
{
  int argsused;

  argsused = PIPE_Parse(argc, argv);
  INFOF(IOUT, "Pipetracer: Parsed %d args (%d)\n", argsused, getpid());
  PIPE_Init();
  return argsused;
}

void command_line(char *stack)
{
  int   *argc_ptr, argc, local_argc;;
  char **argv;
  char **local_argv;
  int i;

  argc_ptr = (int *)stack;
  local_argc = argc = *argc_ptr;
  local_argv = argv = (char **)(stack+8);

  fprintf(IOUT, "command_line: %d args \n", argc);
  for(i = 0; i < local_argc; i++) {
        fprintf(IOUT, "\targ %d: %s\n", i, local_argv[i]);
  }
  PIPE_Parse_Init(--argc, ++argv);
  /* Get rid of the extra arguments "-buffaddr xxxx" */
  *argc_ptr -= 2; 
  fprintf(IOUT, "command_line: after PIPE_Parse_Init(): %d args \n", local_argc);
  for(i = 0; i < local_argc; i++) {
        fprintf(IOUT, "\targ %d: %s\n", i, local_argv[i]);
  }
}

void GrabInstructions(char * filename, long linstcount, long instrumented_FirstPC, long instrumented_LastPC)
{
  LDFILE *ldptr;
  SCNHDR *secthead;
  SCNHDR *initsecthead;
  SCNHDR *finisecthead;
  long no_insns;
  long no_initinsns;
  long no_finiinsns;
  long nread;
  long i;
  long first_instrumented_index = 0;

  ldptr = NULL;
  if ( (ldptr = ldopen(filename, ldptr)) != NULL ) {
       unsigned long textfirstpc;
       unsigned long textlastpc;
       unsigned long initfirstpc;
       unsigned long initlastpc;
       unsigned long finifirstpc;
       unsigned long finilastpc;

       secthead = calloc(1, sizeof(SCNHDR));
       initsecthead = calloc(1, sizeof(SCNHDR));
       finisecthead = calloc(1, sizeof(SCNHDR));

       if(ldnshread (ldptr, ".text", secthead) == FAILURE){
         perror("ldnshread .text");
         exit(0);
        }
       if(ldnshread (ldptr, ".init", initsecthead) == FAILURE){
         perror("ldnshread .init");
         exit(0);
        }
       if(ldnshread (ldptr, ".fini", finisecthead) == FAILURE){
         perror("ldnshread .fini");
         exit(0);
        }
         textfirstpc = secthead->s_vaddr;
         textlastpc = secthead->s_vaddr + secthead->s_size - 4;
         initfirstpc = initsecthead->s_vaddr;
         initlastpc = initsecthead->s_vaddr + initsecthead->s_size - 4;
         finifirstpc = finisecthead->s_vaddr;
         finilastpc = finisecthead->s_vaddr + finisecthead->s_size - 4;
        INFOF(stdout, ".text begins at 0x%lx  size = %d ends at 0x%lx\n", textfirstpc,
            secthead->s_size, textlastpc);
        INFOF(stdout, ".init begins at 0x%lx  size = %d ends at 0x%lx\n", initfirstpc,
            initsecthead->s_size, initlastpc);
        INFOF(stdout, ".fini begins at 0x%lx  size = %d ends at 0x%lx\n", finifirstpc,
            finisecthead->s_size, finilastpc);

        if (instrumented_FirstPC != textfirstpc){
           fprintf(stderr, "WARNING: first text address = 0x%lx first instrumented 0x%lx\n", textfirstpc,
            instrumented_FirstPC);
            assert(textfirstpc < instrumented_FirstPC);
        }
       FirstPC = textfirstpc;
       FirstInstrumentedPC = instrumented_FirstPC;
       if (instrumented_LastPC != textlastpc){
           fprintf(stderr, "WARNING: last text address = 0x%lx last instrumented 0x%lx\n", textlastpc,
            instrumented_LastPC);
            assert(textlastpc > instrumented_LastPC);
        }
        LastPC = textlastpc;
        no_insns = (secthead->s_size)/4;
        if (no_insns != linstcount){
           fprintf(stderr, "WARNING: no_text_insns = %d instrumented %d\n", no_insns,
            linstcount);
            assert(linstcount < no_insns);
        }

        text = (unsigned int *)calloc(no_insns,sizeof(unsigned int));
        global_vea = (long *)calloc(no_insns,sizeof(long));
         if(text == NULL){
             perror("calloc");
             exit(0);
        }
        if(ldnsseek (ldptr, ".text") == FAILURE){
             perror("ldnsseek");
             exit(0);
        }
        nread = ldfread((void *)text, sizeof(unsigned int), no_insns, ldptr); 
        if(nread < no_insns ){
            perror("ldfread");
            exit(0);
        }
        first_instrumented_index = (instrumented_FirstPC - FirstPC)/4;
        instructions = text + first_instrumented_index;
  }
 ldclose(ldptr);
}
