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

#include "syntax.h"
#include "disasm.h"
#include "mopen.h"
#include "mpipe.h"
#define NO_ASIM_THREAD
#include "feedtrace.h"


bool FEED_Init (UINT32 argc, char **argv, char **envp);
void FEED_List ();
void FEED_Done ();

static TRACE_BUFFER trace;
static UINT32 threads = 0;
static pid_t  simos_pid;
static int    nokill, silent;

int
main (UINT32 argc, char **argv, char **envp)
{
    if(!FEED_Init(argc-1, &argv[1], envp))
        exit(-1);
    FEED_List();
    FEED_Done();
}

void
FEED_List()
{
  UINT64 vpc, opcode, id;
  UINT32 eofcount = 0;
  UINT64 count = 0;
  UINT64 tcount[4] = {0, 0, 0, 0};

  while(1) {
    for(UINT32 t=0; t < threads; t++) {
#if 0
      if(t == 1 && tcount[t] % 8 != 0) {
	tcount[t]++;
	continue;
      }
#endif
      if(!trace[t].Eof()) {
	vpc = trace[t].VirtualPc();
	opcode = trace[t].Opcode();
	id = trace[t].GetIdentifier();

	// Display instruction from trace
	if(!silent)
	  fprintf(stdout, FMT64U":T"FMT64U":"FMT64X": %s\n", id,
		  t, vpc, disassemble(opcode, vpc));
	count++;
	tcount[t]++;

	trace[t].NextInstr();
	trace[t].CommitInstr(id);
      }
      else if(++eofcount >= threads)
	return;
    }
    eofcount = 0;
  }
}

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

/********************************************************************
 *
 * Controller calls
 *
 *******************************************************************/

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
        else if(strcmp(fargv[i], "-nokill") == 0)
	    nokill = 1;
        else if(strcmp(fargv[i], "-silent") == 0)
	    silent = 1;
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
        printf("Tracelister: initting trace buffer %d\n", t);
        trace[t].InitTracer(t, 0, &buffer);
    }
    *(UINT64 *)buffer = 0;
    fprintf(stdout, "Tracelister: finished init\n");

    for(UINT32 t = 0; t < threads; t++) {
        printf("Tracelister: starting trace buffer %d\n", t);
        trace[t].StartTracer();
        if (trace[t].VirtualPc() == UINT64_MAX || trace[t].Eof()) {
            trace[t].Ended() = true;
            fprintf(stdout, "Trace "FMT32U" contains no instructions.\n", t);
            return(false);
        }
    }
    fprintf(stdout, "Tracelister: started traces\n");

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
    
    if(!nokill)
        kill(simos_pid, SIGTERM);
}


void
FEED_Usage (FILE *file)
/*
 * Print usage...
 */
{
    fprintf(file, "\nTracelister usage: tracelist <feederprogram> [-threads <#>] [-buffname X/<filname>] <feederarg0> <feederargs>\n");
}

