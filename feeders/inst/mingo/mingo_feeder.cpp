// 5/17/2002: Deleted local ASIMERROR so we can use the real one.
// 5/17/2002: Added TRACE to AllowSWTProgress
// 5/10/2002: Added TRACE statement
// 5/9/2002: Added dummy Done to keep mingoint.cpp happy. Deleted main()

/*
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
 */
 
/**
 * @file mingo_feeder.c
 * @author Michael Adler
 * @date March, 2002
 *
 * ASIM side of Mingo feeder.  
 */

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <iostream>
#include <sched.h>
#include <sys/wait.h>

#include "asim/syntax.h"
#include "asim/trace.h"
#include "asim/mesg.h"
#include "asim/restricted/mingo.h"
#include "asim/restricted/mingo_pvt.h"
#include "asim/provides/instfeeder_implementation.h"


#ifndef BUILDABSROOT
#error Build root not defined (BUILDROOT)
#endif

//
// Hack for turning #define value into string
//
#define xstr(x) #x
#define makestring(x) xstr(x)


const UINT32 MINGO_DATA_CLASS::field_mask[MINGO_MSG_LAST];

//
// fork2() - like fork, but the new process is immediately orphaned
//           (won't leave a zombie when it exits)
//   Returns 1 to the parent, not any meaningful pid.
//   The parent cannot wait() for the new process (it's unrelated).
//
//     
// This version assumes that you *haven't* caught or ignored SIGCHLD.
// If you have, then you should just be using fork() instead anyway.
//     
static int fork2(void)
{
    pid_t pid;
    int rc;
    int status;
     
    if (!(pid = fork()))
    {
        switch (fork())
        {
          case 0:  return 0;
          case -1: _exit(errno);    /* assumes all errnos are <256 */
          default: _exit(0);
        }
    }
     
    if (pid < 0 || waitpid(pid,&status,0) < 0)
        return -1;
     
    if (WIFEXITED(status))
        if (WEXITSTATUS(status) == 0)
            return 1;
        else
            errno = WEXITSTATUS(status);
    else
        errno = EINTR;  /* well, sort of :-) */
     
    return -1;
}


extern "C"
{
void MingoChildKilled(
    int sig)
{
    int s;
    while (waitpid(-1, &s, WNOHANG) > 0)
        MINGO_PVT_Abort();
}
}


MINGO_FEEDER_CLASS::MINGO_FEEDER_CLASS(
    const char *path,
    char * const argv[],
    char * envp[],
    int nHardwareContexts,
    char *stdinPath,
    char *stdoutPath,
    char *stderrPath) : nHWContexts(nHardwareContexts)
{
    int shared_fd;

    shared_fd = MINGO_PVT_Create_IPC_Region(MINGO_MAX_THREADS, nHardwareContexts);
    if (shared_fd == -1)
    {
        ASIMERROR("MINGO_FEEDER: failed to create IPC buffer.");
    }

    struct sigaction sa;
    sa.sa_handler = MingoChildKilled;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);

    switch (fork())
    {
      case -1:
        ASIMERROR("MINGO_FEEDER: fork() failed.");
        break;

      case 0:
        {
            //
            // Child
            //
            unsigned int i;
            char **ep_dst;
            char **ep_src;
            char **mingo_envp;
            char **ld_library_path;

            //
            // Put shared memory buffer at file descriptor 3.
            //
            close(3);
            if (dup2(shared_fd, 3) == -1)
            {
                ASIMERROR("MINGO_FEEDER: dup2() failed.");
            }
            close(shared_fd);

            //
            // Append Mingo environment variable to the list and make
            // sure the library directory is on LD_LIBRARY_PATH.
            //

            //
            // Start by making room for another environment string
            //
            i = 0;
            ep_src = envp;
            while (*ep_src++)
            {
                i += 1;
            }
            mingo_envp = (char **)malloc(sizeof(*ep_src) * (i + 2));

            ld_library_path = NULL;

            ep_dst = mingo_envp;
            ep_src = envp;

            //
            // Copy source environment array to private copy
            //
            while (*ep_src)
            {
                *ep_dst = *ep_src;

                //
                // Has the LD_LIBRARY_PATH environment
                // variable been defined already?
                //
                if (strncmp(*ep_dst, "LD_LIBRARY_PATH=", 16) == 0)
                {
                    ld_library_path = ep_dst;
                }

                ep_dst += 1;
                ep_src += 1;
            }

            //
            // If LD_LIBRARY_PATH not found make space for it.
            //

            if (ld_library_path == NULL)
            {
                ld_library_path = ep_dst;
                *ld_library_path = "LD_LIBRARY_PATH=";
                ep_dst += 1;
            }

            *ep_dst = NULL;

            //
            // Add Mingo library directory to LD_LIBRARY_PATH
            //
            char *ld_lib = (char *)malloc(strlen(*ld_library_path) +
                                          strlen(makestring(BUILDABSROOT)) +
                                          7);

            strcpy(ld_lib, "LD_LIBRARY_PATH=");
            strcat(ld_lib, makestring(BUILDABSROOT));
            strcat(ld_lib, "/mingo:");
            strcat(ld_lib, &((*ld_library_path)[16]));

            *ld_library_path = ld_lib;

            //
            // Ready to exec the feeder process...
            //
            execve(path, argv, mingo_envp);

            cerr << "MINGO_FEEDER: execv() failed." << endl;
            exit(1);
        }

      default:
        //
        // Parent
        //
        close(shared_fd);
    }
};


MINGO_FEEDER_CLASS::~MINGO_FEEDER_CLASS()
{
    MINGO_PVT_Abort();
};
    

void
MINGO_FEEDER_CLASS::WaitForFeederEvents()
{
    while ((! EndOfData()) &&
           ((MINGO_PVT_Test_Events_Enabled() == 0) ||
            (MINGO_PVT_Test_First_Event_Received() == 0)))
    {
        sleep(1);
    }
}


UINT32
MINGO_FEEDER_CLASS::CheckForNewSoftwareThread()
{
    static int prevThreads = 0;

    //
    // Wait until we've received notification of at least one thread.
    //
    while (MINGO_PVT_Allocated_Threads() == 0)
    {
        sleep(1);
    }

    //
    // Are events disabled?  Wait for feeder to send us something.
    //
    WaitForFeederEvents();

    if (MINGO_PVT_Allocated_Threads() > prevThreads)
    {
        prevThreads += 1;
        return prevThreads;
    }
    
    return 0;
}


//
// Return the time since the epoch in microseconds
//
inline
UINT64 curTime()
{
    struct timeval t;
    UINT64 s;
    UINT64 ms;

    gettimeofday(&t, NULL);

    s = (UINT64) t.tv_sec * 1000000L;
    return s + t.tv_usec;
}


bool
MINGO_FEEDER_CLASS::GetNextSoftwareThreadEvent(
    UINT32 thread_id,
    MINGO_DATA event,
    UINT32 waituS)
{
    MINGO_PACKET_CLASS packet;

    TRACE(Trace_Feeder, 
        cout << "        " 
             << "MINGO_FEEDER_CLASS::GetNextSoftwareThreadEvent called for "
             << "thread " << thread_id <<endl;);
    
    if (thread_id > (UINT32)MINGO_PVT_Allocated_Threads())
    {
        ASIMERROR("MINGO_FEEDER_CLASS::GetNextSoftwareThreadEvent():  Nonexistent thread.");
    }

    //
    // Keep control here until feeder turns on events
    //
    WaitForFeederEvents();

    UINT64 startTime = 0;
    if (waituS)
    {
        startTime = curTime();
    }

    UINT32 trips = 0;
    while (1)
    {
        trips += 1;

        if (MINGO_PVT_Get_State(thread_id) == MINGO_STATE_EVENT_READY)
        {
            if (event != NULL)
            {
                if (! MINGO_PVT_Recv_Event(thread_id, &packet))
                {
                    ASIMERROR("MINGO_FEEDER_CLASS::GetNextSoftwareThreadEvent:  Error receiving event.");
                }
                if (MINGO_PVT_Set_State(thread_id, MINGO_STATE_ASIM_CONSUMED))
                {
                    ASIMERROR("MINGO_FEEDER_CLASS::GetNextSoftwareThreadEvent:  Error changing packet state.");
                }
                *event = packet;
            }
            return true;
        }

        if (waituS == 0)
        {
            return false;
        }

        //
        // Has wait time expired?  Don't check every time.
        //
        if ((trips & 0xfff) == 0)
        {
            if ((curTime() - startTime) > waituS)
            {
                return false;
            }

            //
            // Let some other process have a chance
            //
//            sched_yield();
        }
    }


    return false;
}


void
MINGO_FEEDER_CLASS::AllowSoftwareThreadProgress(
    UINT32 thread_id)
{
    TRACE(Trace_Feeder,
          cout << "        "
               << "MINGO_FEEDER_CLASS::AllowSoftwareThreadProgress "
          << "called for thread "  << thread_id << endl);
    
    if (thread_id > (UINT32)MINGO_PVT_Allocated_Threads())
    {
        ASIMERROR("MINGO_FEEDER_CLASS::AllowSoftwareThreadProgress():  Nonexistent thread.");
    }

    MINGO_RESPONSE_PACKET_CLASS packet;

    packet.msg = MINGO_RESPONSE_ACK;
    if (MINGO_PVT_Send_Response(thread_id, &packet))
    {
        ASIMERROR("MINGO_FEEDER_CLASS::AllowSoftwareThreadProgress():  Packet response error.");
    }
}


void
MINGO_FEEDER_CLASS::FastForwardNEvents(
    UINT64 nEvents)
{
    MINGO_PVT_Skip_Events(nEvents);
}


UINT64
MINGO_FEEDER_CLASS::ProbeMemory(
    UINT32 thread_id,
    UINT64 va,
    UINT32 nBytes)
{
    switch (nBytes)
    {
      case 1:
      case 2:
      case 4:
      case 8:
        break;

      default:
        ASIMERROR("MINGO_FEEDER_CLASS::ProbeMemory():  Illegal argument.");
    }

    MINGO_RESPONSE_PACKET_CLASS response_packet;

    response_packet.msg = MINGO_RESPONSE_PROBE;
    response_packet.u.probe.va = va;
    response_packet.u.probe.nBytes = nBytes;
    if (MINGO_PVT_Send_Response(thread_id, &response_packet))
    {
        ASIMERROR("MINGO_FEEDER_CLASS::ProbeMemory():  Packet response error.");
    }

    MINGO_PACKET_CLASS packet;
    while (! MINGO_PVT_Recv_Event(thread_id, &packet))
    {
        sched_yield();
    }

    if (packet.msg != MINGO_MSG_PROBE_RESULT)
    {
        ASIMERROR("MINGO_FEEDER_CLASS::ProbeMemory():  Expected probe result.");
    }

    if (MINGO_PVT_Set_State(thread_id, MINGO_STATE_ASIM_CONSUMED))
    {
        ASIMERROR("MINGO_FEEDER_CLASS::ProbeMemory:  Error changing packet state.");
    }

    MINGO_DATA_CLASS result = packet;
    return result.get_probe_value();
}


bool
MINGO_FEEDER_CLASS::EndOfData()
{
    return MINGO_PVT_Check_Abort();
}


//
// Called by FEED_Done when Asim terminates a simulation run
//
void
MINGO_FEEDER_CLASS::Done(void)
{}



//
// DumpPacket --
//  For debugging:  dump a text representation of a data packet.
//
const void
MINGO_DATA_CLASS::DumpPacket(
    FILE *str)
{
    char *msgstr;
    char *llxstr;
    char *lldstr;

    switch (msg)
    {
      case MINGO_MSG_THREAD_END:
        msgstr = "THREAD_END";
        break;
      case MINGO_MSG_THREAD_PRIORITY:
        msgstr = "THREAD_PRIORITY";
        break;
      case MINGO_MSG_PREFERRED_CPU:
        msgstr = "PREFERRED_CPU";
        break;
      case MINGO_MSG_MEMORY_READ:
        msgstr = "MEMORY_READ";
        break;
      case MINGO_MSG_MEMORY_WRITE:
        msgstr = "MEMORY_WRITE";
        break;
      case MINGO_MSG_EXCHANGE:
        msgstr = "EXCHANGE";
        break;
      case MINGO_MSG_MEMORY_FENCE:
        msgstr = "MEMORY_FENCE";
        break;
      case MINGO_MSG_DEPENDENT_READ:
        msgstr = "DEPENDENT_READ";
        break;
      case MINGO_MSG_BRANCH:
        msgstr = "BRANCH";
        break;
      case MINGO_MSG_SYSCALL:
        msgstr = "SYSCALL";
        break;
      case MINGO_MSG_TIMED_WAIT:
        msgstr = "TIMED_WAIT";
        break;
      default:
        fprintf(str, "Illegal packet\n");
        return;
    }

    if (sizeof(HOST_UINT) == sizeof(UINT64))
    {
        llxstr = "0x%0lx";
        lldstr = "%ld";
    }
    else
    {
        llxstr = "0x%0llx";
        lldstr = "%lld";
    }

    fprintf(str, "%s\n", msgstr);

    fprintf(str, "    Thread Id          %d\n", get_thread_id());

    if (MF_IP & field_mask[msg])
    {
        fprintf(str, "    IP:                ");
        fprintf(str, llxstr, get_ip());
        fprintf(str, "\n");
    };

    if (MF_VA & field_mask[msg])
    {
        fprintf(str, "    VA:                ");
        fprintf(str, llxstr, get_va());
        fprintf(str, "\n");
    };

    if (MF_ASIZE & field_mask[msg])
    {
        fprintf(str, "    Access size:       %d bytes\n", 1 << get_access_size());
    };

    if (MF_CPUID & field_mask[msg])
    {
        fprintf(str, "    CPU ID:            %d\n", get_cpu_id());
    };

    if (MF_FENCE & field_mask[msg])
    {
        char *t;

        switch (get_memory_fence_type())
        {
          case MINGO_FENCE_ORDERING:
            t = "ORDERING";
            break;
          case MINGO_FENCE_ACCEPTANCE:
            t = "ACCEPTANCE";
            break;
          default:
            t = "<<Invalid>>";
            break;
        }
        fprintf(str, "    Type:              %s\n", t);
    };

    if (MF_ICOUNT & field_mask[msg])
    {
        fprintf(str, "    Instruction count: %d\n", get_instruction_count());
    };

    if (MF_INDIRECT & field_mask[msg])
    {
        fprintf(str, "    Indirect:          %c\n",
                get_branch_is_register_indirect() ? 'Y' : 'N');
    };

    if (MF_LDST & field_mask[msg])
    {
        char *t;

        fprintf(str, "    Lock:              %c\n",
                get_is_lock_request() ? 'Y' : 'N');

        fprintf(str, "    SP Relative:       %c\n",
                get_is_sp_relative() ? 'Y' : 'N');

        fprintf(str, "    Hint:              ");
        switch (get_memory_hint())
        {
          case MEMHINT_NONE:
            t = "none";
            break;
          case MEMHINT_NT1:
            t = "nt1";
            break;
          case MEMHINT_NTA:
            t = "nta";
            break;
          default:
            t = "<<Invalid>>";
        }
        fprintf(str, "%s\n", t);
    };

    if (MF_PRIORITY & field_mask[msg])
    {
        fprintf(str, "    Priority:          %d\n", get_priority());
    };

    if (MF_CONDBRA & field_mask[msg])
    {
        fprintf(str, "    Conditional:       %c\n",
                get_is_branch_conditional() ? 'Y' : 'N');
    };

    if (MF_TAKENBRA & field_mask[msg])
    {
        fprintf(str, "    Taken:             %c\n",
                get_is_branch_taken() ? 'Y' : 'N');
    };

    if (MF_REGBRA & field_mask[msg])
    {
        fprintf(str, "    Branch Target Reg: %d\n", get_branch_target_register());
    };

    if (MF_REGSRC & field_mask[msg])
    {
        fprintf(str, "    Source Register:   %d\n", get_source_register());
    };

    if (MF_REGTGT & field_mask[msg])
    {
        fprintf(str, "    Target Register:   %d\n", get_target_register());
    };

    if (MF_SYSCALL & field_mask[msg])
    {
        char *t;

        switch (get_syscall_type())
        {
          case MINGO_SYSCALL_SLOW:
            t = "SLOW";
            break;
          case MINGO_SYSCALL_FAST:
            t = "FAST";
            break;
          case MINGO_SYSCALL_MUTEX:
            t = "MUTEX";
            break;
          case MINGO_SYSCALL_IO:
            t = "IO";
            break;
          default:
            t = "<<Invalid>>";
            break;
        }
        fprintf(str, "    Type:              %s\n", t);
    };

    if (MF_TGTIP & field_mask[msg])
    {
        fprintf(str, "    Branch Target IP   ");
        fprintf(str, llxstr, get_branch_target_ip());
        fprintf(str, "\n");
    };

    if (MF_XFER & field_mask[msg])
    {
        char *t;

        switch (get_branch_type())
        {
          case MINGO_XFER_BRANCH:
            t = "BRANCH";
            break;
          case MINGO_XFER_CALL:
            t = "CALL";
            break;
          case MINGO_XFER_RETURN:
            t = "RETURN";
            break;
          default:
            t = "<<Invalid>>";
            break;
        }
        fprintf(str, "    Type:              %s\n", t);
    };

    if (MF_CMP & field_mask[msg])
    {
        fprintf(str, "    Compare:           %c\n",
                get_is_compare() ? 'Y' : 'N');
        if (get_is_compare())
        {
            fprintf(str, "    Compare value:     ");
            fprintf(str, llxstr, get_compare_value());
            fprintf(str, "\n");
        }
    };

    if (MF_RELACQ & field_mask[msg])
    {
        fprintf(str, "    Semantics:         ");
        if (get_is_release() && get_is_acquire())
        {
            fprintf(str, "Release + Acquire\n");
        }
        else if (get_is_release())
        {
            fprintf(str, "Release\n");
        }
        else if (get_is_acquire())
        {
            fprintf(str, "Acquire\n");
        }
        else
        {
            fprintf(str, "None\n");
        }
    }

    if (MF_LDTYPE & field_mask[msg])
    {
        char *t;

        fprintf(str, "    Load type:         ");
        switch (get_load_type())
        {
          case LDTYPE_NONE:
            t = "none";
            break;
          case LDTYPE_S:
            t = "s";
            break;
          case LDTYPE_A:
            t = "a";
            break;
          case LDTYPE_SA:
            t = "sa";
            break;
          case LDTYPE_BIAS:
            t = "bias";
            break;
          case LDTYPE_ACQ:
            t = "acq";
            break;
          case LDTYPE_FILL:
            t = "fill";
            break;
          case LDTYPE_C_CLR:
            t = "c.clr";
            break;
          case LDTYPE_C_NC:
            t = "c.nc";
            break;
          case LDTYPE_C_CLR_ACQ:
            t = "c.clr.acq";
            break;
          default:
            t = "<<Invalid>>";
        }
        fprintf(str, "%s\n", t);
    }

    if (MF_STTYPE & field_mask[msg])
    {
        fprintf(str, "    Store type:        ");
        if (get_is_store_release())
        {
            if (get_is_store_spill())
            {
                ASIMERROR("MINGO_DATA_CLASS::DumpPacket:  Illegal store specification.");
            }
            fprintf(str, "release\n");
        }
        else if (get_is_store_spill())
        {
            fprintf(str, "spilll\n");
        }
        else
        {
            fprintf(str, "none\n");
        }
    }

    if (MF_WAIT & field_mask[msg])
    {
        fprintf(str, "    Sleep time:        ");
        fprintf(str, lldstr, get_timed_wait());
        fprintf(str, " ns\n");
    }

    fflush(str);
};
