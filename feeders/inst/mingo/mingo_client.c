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
 * @file mingo_client.c
 * @author Michael Adler
 * @date March, 2002
 *
 * This is the code that gets linked into the instrumented program.
 * The module has main() and calls the original program.  It also provides
 * all the routines defined in mingo.h.
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include "asim/syntax.h"
#include "mingo.h"
#include "mingo_pvt.h"


//
// Key for Mingo per-thread data
//
#ifdef MINGO_THREADED
static pthread_key_t mingo_thread_id_key;
static pthread_mutex_t thread_start_lock;
#endif

typedef struct
{
    UINT32  thread_id;

    //
    // Count of executed instructions updated by MINGO_Note_Executed_Instrs()
    //
    UINT64 executed_instrs;

}
THREAD_DATA_CLASS;

typedef THREAD_DATA_CLASS *THREAD_DATA;


//
// If pthreads isn't bound in we need a place to hold the thread data record
// for the single thread in the image.
//
static THREAD_DATA root_thread_data;

pid_t rootPid;      // pid of primary thread in the process

int sentAbort = 0;  // True if in the process of exiting

static int noAsimConnection;

int fastFwdVerbose = 0;


static inline THREAD_DATA
GetCurThreadData(
    int requireThreadData);

static int
SendMessage(
    MINGO_PACKET packet,
    THREAD_DATA  thread_data);


static void
MINGO_Quit(void)
{
    THREAD_DATA thread_data;

    //
    // Drop the Asim connection
    //
    if (! noAsimConnection)
    {
        MINGO_PVT_Abort();
        noAsimConnection = 1;
    }

    sentAbort = 1;

    //
    // Start a kill at the parent thread.
    //
    kill(rootPid, SIGINT);

    exit(1);
}


static void
MINGO_ERROR(
    char *str,
    ...)
{
    static int printedFatalError = 0;
    va_list ap;

    va_start(ap, str);
    if (! printedFatalError)
    {
        vfprintf(stderr, str, ap);
        fprintf(stderr, "\n");

        printedFatalError = 1;
    }
    va_end(ap);

    MINGO_Quit();
};


//
// Handle a memory probe.  If we were being utterly paranoid, the probe would
// be tagged with the thread that should do the load.  We don't bother due to
// all the inter-thread mutex locking.  Memory will be synchronized by the time
// the probe comes in.
//
static void
MakeMemoryProbePacket(
    UINT64 va,
    UINT32 nBytes,
    MINGO_PACKET packet)
{
    bzero((void *)packet, sizeof(*packet));
    packet->msg = MINGO_MSG_PROBE_RESULT;
    packet->va = va;

    switch (nBytes)
    {
      case 1:
        packet->d.info = (UINT64)(* ((UINT8 *)(HOST_UINT)va));
        break;
      case 2:
        packet->d.info = (UINT64)(* ((UINT16 *)(HOST_UINT)va));
        break;
      case 4:
        packet->d.info = (UINT64)(* ((UINT32 *)(HOST_UINT)va));
        break;
      case 8:
        packet->d.info = (UINT64)(* ((UINT64 *)(HOST_UINT)va));
        break;
      default:
        MINGO_ERROR("MINGO MakeMemoryProbePacket:  Illegal size");
    }
}


//
// WaitForASIMResponse --
//  Called by a thread that must block.  This routine blocks until the
//  "proceed" message arrives from ASIM.
//
static void
WaitForASIMResponse(
    THREAD_DATA  thread_data)
{
    int old_errno = errno;
    MINGO_RESPONSE_PACKET_CLASS response;
    UINT32 trips;

    trips = 0;
    while (! MINGO_PVT_Recv_Response(thread_data->thread_id, &response))
    {
        trips += 1;

        if (MINGO_PVT_Check_Abort())
        {
            MINGO_ERROR("Asim sent abort.  Exiting...");
        }

        if ((trips & 0xffff) == 0)
        {
            struct timespec t;

            //
            // Wait time is variable, a simple way of waiting increasing amounts
            // of time at the beginning and then enough of the time not to be
            // a burden if no response is seen quickly.
            //
            t.tv_sec = 0;
            t.tv_nsec = 10000 * ((trips & 0xfff0000) >> 16);
            nanosleep(&t, NULL);
        }
        else if ((trips & 0xfff) == 0)
        {
            sched_yield();
        }

        if ((trips & 0x007fffff) == 0)
        {
            //
            // Is ASIM dead?  This isn't a foolproof test, but it should
            // generally work.
            //
            if (kill(MINGO_PVT_Get_Asim_Pid(), 0))
            {
                MINGO_ERROR("Asim appears dead.  Exiting...");
            }
        }
    }

    if (response.msg == MINGO_RESPONSE_PROBE)
    {
        MINGO_PACKET_CLASS probePacket;

        bzero(&probePacket, sizeof(probePacket));
        MakeMemoryProbePacket(response.u.probe.va, response.u.probe.nBytes,
                              &probePacket);
        probePacket.thread_id = thread_data->thread_id;

        if (MINGO_PVT_Set_State(thread_data->thread_id, MINGO_STATE_EMPTY))
        {
            MINGO_Quit();
        }
        SendMessage(&probePacket, thread_data);
    }
    else if (response.msg != MINGO_RESPONSE_ACK)
    {
        MINGO_ERROR("Unsupported");
    }

    if (MINGO_PVT_Set_State(thread_data->thread_id, MINGO_STATE_EMPTY))
    {
        MINGO_Quit();
    }

    errno = old_errno;
}


//
// SendMessage --
//  Send one packet to ASIM.
//
static int
SendMessage(
    MINGO_PACKET packet,
    THREAD_DATA  thread_data
)
{
    int old_errno = errno;
    static int eventsDisabled = 0;

    if (noAsimConnection) return 0;

    //
    // Are events enabled?
    //
    if (! MINGO_PVT_Test_Events_Enabled())
    {
        eventsDisabled = 1;
        return 0;
    }
    if (eventsDisabled)
    {
        eventsDisabled = 0;
        if (fastFwdVerbose)
        {
            fprintf(stderr, "Enabling events...\n");
        }
    }

    if (MINGO_PVT_Send_Event(thread_data->thread_id, packet))
    {
        eventsDisabled = 1;
        return 0;
    }

    if (eventsDisabled)
    {
        eventsDisabled = 0;
        if (fastFwdVerbose)
        {
            fprintf(stderr, "Enabling events...\n");
        }
    }
        
    WaitForASIMResponse(thread_data);
    
    errno = old_errno;

    return 1;
}


static inline THREAD_DATA
GetCurThreadData(
    int requireThreadData)
{
    THREAD_DATA data;
    int old_errno = errno;

    if (! sentAbort)
    {
#ifdef MINGO_THREADED
        data = (THREAD_DATA) pthread_getspecific(mingo_thread_id_key);
#else
        data = root_thread_data;
#endif
    }
    else
    {
        //
        // Abort sent.  Try exit() again.
        //
        exit(1);
    }

    if (requireThreadData && (data == NULL))
    {
        MINGO_ERROR("MINGO:  Call MINGO_Thread_Start() immediately after creating thread");
    }

    errno = old_errno;

    return data;
}


static inline void
InitPacketForThread(
    MINGO_PACKET packet,
    MINGO_MSG    msg,
    THREAD_DATA  thread_data)
{
    packet->msg = msg;
    packet->thread_id = thread_data->thread_id;
    packet->ip = 0;
    packet->va = 0;
    packet->compare_value = 0;
    packet->d.info = 0;
};


//
// UpdateExecutedInstrs --
//  Add executed_isntrs static to the count of executed instructions in the
//  info field.  This passes the accumulation of calls to
//  MINGO_Note_Executed_Instrs().
//
static inline HOST_UINT
UpdateExecutedInstrs(
    THREAD_DATA thread_data,
    HOST_UINT   info)
{
    if (thread_data->executed_instrs == 0)
    {
        return info;
    }

    if (thread_data->executed_instrs + (info & 0xffff) > 0xffff)
    {
        info = info | 0xffff;
    }
    else
    {
        info += thread_data->executed_instrs;
    }

    thread_data->executed_instrs = 0;

    return info;
};


int
MINGO_NCPUs(
    void)
{
    return MINGO_PVT_Get_Hardware_Context_Count();
}


void
MINGO_Disable_Events(
    void)
{
    if (! noAsimConnection) MINGO_PVT_Disable_Events();
}


void
MINGO_Resume_Events(
    void)
{
    if (! noAsimConnection) MINGO_PVT_Enable_Events();
}


void
MINGO_Trigger(
    INT32 trigger_id)
{
    if (fastFwdVerbose)
    {
        fprintf(stderr, "MINGO_Trigger:  %d\n", trigger_id);
    }

    if (noAsimConnection == 0)
    {
        if (trigger_id == -1)
        {
            //
            // Terminate
            //
            MINGO_Disable_Events();
            if (fastFwdVerbose)
            {
                fprintf(stderr, "MINGO_Trigger(-1) causing feeder exit...\n");
            }
            MINGO_Quit();
        }
        else if (trigger_id)
        {
            if (! MINGO_PVT_Test_Events_Enabled())
            {
                MINGO_Resume_Events();
                if (fastFwdVerbose && MINGO_PVT_Test_Events_Enabled())
                {
                    fprintf(stderr, "Enabling ASIM events...\n");
                }
            }
        }
        else
        {
            if (MINGO_PVT_Test_Events_Enabled())
            {
                MINGO_Disable_Events();
                if (fastFwdVerbose)
                {
                    fprintf(stderr, "Disabling ASIM events...\n");
                }
            }
        }
    }
}


void
MINGO_Thread_Start(
    void)
{
    THREAD_DATA data;
    int old_errno = errno;
    static int dummyThreadID = 0;   // Used only when there is no asim connection

    data = (THREAD_DATA)malloc(sizeof(THREAD_DATA_CLASS));
    bzero(data, sizeof(THREAD_DATA_CLASS));
    if (root_thread_data == NULL)
    {
        //
        // Put the root thread's data in a well known location
        //
        root_thread_data = data;
    }

#ifdef MINGO_THREADED
    //
    // Store thread descriptor as thread specific data.
    //
    if (pthread_getspecific(mingo_thread_id_key) != NULL)
    {
        MINGO_ERROR("MINGO_Thread_Start:  thread already has MINGO id");
    }

    if (pthread_setspecific(mingo_thread_id_key, (void *)data))
    {
        MINGO_ERROR("MINGO_Thread_Start:  pthread_setspecific failed");
    }

    //
    // Thread IDs are supposed to be monotonically increasing.  Get a lock
    // before allocating a new thread_id and hold it until we write the
    // new thread message to the pipe.
    //
    if (pthread_mutex_lock(&thread_start_lock))
    {
        MINGO_ERROR("MINGO_Thread_Start:  error acquiring mutex");
    }
#endif

    if (noAsimConnection)
    {
        data->thread_id = ++dummyThreadID;
    }
    else
    {
        data->thread_id = MINGO_PVT_New_Thread();
    }

#ifdef MINGO_THREADED
    pthread_mutex_unlock(&thread_start_lock);
#else
    //
    // Not threaded.  Allow only one call to MINGO_Thread_Start().
    //
    if (data->thread_id != 1)
    {
        MINGO_ERROR("MINGO_Thread_Start:  program is multi-threaded");
    }
#endif

    errno = old_errno;
}


void
MINGO_Thread_End(
    void)
{
    MINGO_PACKET_CLASS packet;
    THREAD_DATA thread_data;

    thread_data = GetCurThreadData(1);
    InitPacketForThread(&packet, MINGO_MSG_THREAD_END, thread_data);

    SendMessage(&packet, thread_data);
    if (! noAsimConnection)
    {
        if (MINGO_PVT_Set_State(thread_data->thread_id, MINGO_STATE_THREAD_DEAD))
        {
            MINGO_Quit();
        }
    }
};


void
MINGO_Thread_Priority(
    INT32   priority)
{
    THREAD_DATA thread_data;

    thread_data = GetCurThreadData(1);
    if (! noAsimConnection)
    {
        if (MINGO_PVT_Set_Priority(thread_data->thread_id, priority))
        {
            MINGO_Quit();
        }
    }
};


void
MINGO_Thread_Preferred_CPU(
    UINT32  cpu_id)
{
    THREAD_DATA thread_data;

    thread_data = GetCurThreadData(1);
    if (! noAsimConnection)
    {
        if (MINGO_PVT_Set_Preferred_CPU(thread_data->thread_id, cpu_id))
        {
            MINGO_Quit();
        }
    }
};


void
MINGO_Memory_Read(
    HOST_UINT IP,
    HOST_UINT VA,
    HOST_UINT info)
{
    MINGO_PACKET_CLASS packet;
    int cisc_access;
    int load_lock;
    THREAD_DATA thread_data;
    static int n_writes = 0;

    thread_data = GetCurThreadData(1);
    load_lock = (info >> 20) & 1;
    cisc_access = (info >> 30) & 1;
    info &= ~(1L << 30);

    InitPacketForThread(&packet, MINGO_MSG_MEMORY_READ, thread_data);
    packet.ip = IP;
    packet.va = VA;
    packet.d.info = UpdateExecutedInstrs(thread_data, info);

    SendMessage(&packet, thread_data);

    if (cisc_access)
    {
        //
        // Implicit dependent read for CISC access.  Generate the extra
        // event.
        //
        info = info & (0x1ffL << 21);
        MINGO_Dependent_Read(IP, info);
    }
};


void
MINGO_Memory_Write(
    HOST_UINT IP,
    HOST_UINT VA,
    HOST_UINT info)
{
    MINGO_PACKET_CLASS packet;
    THREAD_DATA thread_data;

    thread_data = GetCurThreadData(1);
    InitPacketForThread(&packet, MINGO_MSG_MEMORY_WRITE, thread_data);
    packet.ip = IP;
    packet.va = VA;
    packet.d.info = UpdateExecutedInstrs(thread_data, info);

    SendMessage(&packet, thread_data);
};


void
MINGO_Memory_Exchange(
    HOST_UINT IP,
    HOST_UINT VA,
    HOST_UINT info,
    UINT32    compared_value)
{
    MINGO_Memory_Exchange64(IP, VA, info, compared_value);
};


void
MINGO_Memory_Exchange64(
    HOST_UINT IP,
    HOST_UINT VA,
    HOST_UINT info,
    UINT64    compared_value)
{
    MINGO_PACKET_CLASS packet;
    THREAD_DATA thread_data;

    if ((info & 0xc0000000) == 0)
    {
        MINGO_ERROR("MINGO_Memory_Exchange:  release/acquire semantics not specified");
    }

    thread_data = GetCurThreadData(1);
    InitPacketForThread(&packet, MINGO_MSG_EXCHANGE, thread_data);
    packet.ip = IP;
    packet.va = VA;
    packet.compare_value = compared_value;
    packet.d.info = UpdateExecutedInstrs(thread_data, info);

    SendMessage(&packet, thread_data);
};



void
MINGO_Memory_Fence(
    HOST_UINT IP,
    HOST_UINT info)
{
    MINGO_PACKET_CLASS packet;
    THREAD_DATA thread_data;

    thread_data = GetCurThreadData(1);
    InitPacketForThread(&packet, MINGO_MSG_MEMORY_FENCE, thread_data);
    packet.ip = IP;
    packet.d.info = UpdateExecutedInstrs(thread_data, info);

    SendMessage(&packet, thread_data);
};


void
MINGO_Dependent_Read(
    HOST_UINT IP,
    HOST_UINT info)
{
    MINGO_PACKET_CLASS packet;
    THREAD_DATA thread_data;

    thread_data = GetCurThreadData(1);
    InitPacketForThread(&packet, MINGO_MSG_DEPENDENT_READ, thread_data);
    packet.ip = IP;
    packet.d.info = UpdateExecutedInstrs(thread_data, info);

    SendMessage(&packet, thread_data);
};


void
MINGO_Branch(
    HOST_UINT IP,
    HOST_UINT target_IP,
    HOST_UINT info)
{
    MINGO_PACKET_CLASS packet;
    THREAD_DATA thread_data;

    thread_data = GetCurThreadData(1);
    InitPacketForThread(&packet, MINGO_MSG_BRANCH, thread_data);
    packet.ip = IP;
    packet.va = target_IP;
    packet.d.info = UpdateExecutedInstrs(thread_data, info);

    SendMessage(&packet, thread_data);
};


void
MINGO_Note_Executed_Instrs(
    HOST_UINT IP,
    UINT32    instr_count)
{
    THREAD_DATA thread_data;

    thread_data = GetCurThreadData(1);
    thread_data->executed_instrs += instr_count;
};


void
MINGO_Syscall(
    HOST_UINT IP,
    HOST_UINT info)
{
    MINGO_PACKET_CLASS packet;
    THREAD_DATA thread_data;
    struct timespec t;

    thread_data = GetCurThreadData(1);
    InitPacketForThread(&packet, MINGO_MSG_SYSCALL, thread_data);
    packet.ip = IP;
    packet.d.info = UpdateExecutedInstrs(thread_data, info);

    if (SendMessage(&packet, thread_data))
    {
        //
        // This will be removed later.  For now, it keeps ORP spin locks from
        // having a nanosleep() call to the kernel from returning too quickly
        // and generating spurious spin loops.
        //
        t.tv_sec = 0;
        t.tv_nsec = 10000000;
        nanosleep(&t, NULL);
    }
};


void
MINGO_Timed_Wait(
    UINT64 nsWait)
{
    MINGO_PACKET_CLASS packet;
    THREAD_DATA thread_data;

    thread_data = GetCurThreadData(1);
    InitPacketForThread(&packet, MINGO_MSG_TIMED_WAIT, thread_data);
    packet.d.info = nsWait;

    SendMessage(&packet, thread_data);
}


static void
ParseArgs(
    char *argstring)
{
#define MAXMARGS 32
    char *mingoArgs[MAXMARGS];
    char **argv;
    int inWhite;
    int argval;

    //
    // Find the mingo arguments section
    //
    if (argstring == NULL) return;

    //
    // Break apart the string into separate arguments
    //
    argv = mingoArgs;
    inWhite = 1;
    while (*argstring)
    {
        if (*argstring == ' ')
        {
            *argstring = 0;
            inWhite = 1;
        }
        else if (inWhite)
        {
            *argv++ = argstring;
            inWhite = 0;
        }

        argstring += 1;
    }
    *argv = NULL;
        
    argv = mingoArgs;
    while (*argv)
    {
        char arg;

        if ((*argv)[0] != '-')
        {
            MINGO_ERROR("MINGO::ParseArgs -- Invalid argument (%s)", *argv);
        }

        arg = (*argv)[1];

        switch (arg)
        {
            //
            // -fs -- fast forward skip (for any events).  Lower precedence
            //        than -ft.
            // -ft -- disable events until MINGO_Trigger() called n times.
            // -fv -- counting verbosity (currently 0 or 1)
            //
          case 'f':
            //
            // All 'f' arguments have a numeric value.
            //
            if (*(++argv) == NULL)
            {
                MINGO_ERROR("MINGO::ParseArgs -- Expected argument to (%s)", *(argv-1));
            }

            switch ((*(argv-1))[2])
            {
              case 's':
                argval = strtol(*argv, NULL, 0);
                MINGO_PVT_Skip_Events(argval);
                if (fastFwdVerbose)
                {
                    fprintf(stderr, "Fast Forward Skip: %d\n", argval);
                }
                break;
              case 't':
                argval = strtol(*argv, NULL, 0);
                if (! noAsimConnection)
                {
                    //
                    // Call disable events once for each required call
                    // to MINGO_Trigger().  This could be accomplished more
                    // easily but it keeps the interface simple.  The disable
                    // events call keeps a counter.
                    //
                    int i;
                    for (i = 0; i < argval; i++)
                    {
                        MINGO_PVT_Disable_Events();
                    }
                }
                
                if (fastFwdVerbose)
                {
                    fprintf(stderr, "Fast Forward Until Trigger: %d\n", argval);
                }
                break;
              case 'v':
                fastFwdVerbose = strtol(*argv, NULL, 0);
                break;
              default:
                MINGO_ERROR("MINGO::ParseArgs -- Invalid argument (%s)", *(argv-1));
            }
            break;

          default:
            MINGO_ERROR("MINGO::ParseArgs -- Invalid argument (%s)", *(argv-1));
        }

        ++argv;
    }
};


//
// Termination code, invoked automatically when mingo library is unloaded.
//
void mingo_fini(void) __attribute__((destructor));

void mingo_fini(
    void)
{
    if (! noAsimConnection)
    {
        MINGO_PVT_Abort();
    }
};


//
// Startup code, invoked automatically when mingo library is loaded.
//
void mingo_init(void) __attribute__((constructor));

void mingo_init(
    void)
{
    int i;
    MINGO_PACKET_CLASS packet;

    rootPid = getpid();

    noAsimConnection = MINGO_PVT_Open_IPC(3);
    if (noAsimConnection)
    {
        fprintf(stderr, "Warning: Failed to open shared buffer with Asim.\n");
    }

    //
    // Asim's command line syntax and support for describing how to bypass
    // startup code is a mess.  This environment variable allows a benchmark
    // description to bypass Asim's syntax and pass arguments to the Mingo
    // client without going through Asim.
    //
    ParseArgs(getenv("MINGO_PVT_ARGS"));
    
    if (noAsimConnection)
    {
        fprintf(stderr, "Warning: Running with no connection to Asim.\n\n");
    }

#ifdef MINGO_THREADED
    //
    // Set up per-thread data and create thread for receiving data from ASIM.
    //
    {
        pthread_t ack_thread;

        if (pthread_key_create(&mingo_thread_id_key, NULL))
        {
            MINGO_ERROR("MINGO::main():  Error creating pthread key");
        }

        if (pthread_mutex_init(&thread_start_lock, NULL))
        {
            MINGO_ERROR("MINGO::main():  Failed to init data in mutex.");
        }
    }
#endif

    //
    // Implicit thread start for main thread
    //
    MINGO_Thread_Start();
}
