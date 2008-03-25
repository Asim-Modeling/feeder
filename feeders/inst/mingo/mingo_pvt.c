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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <asm/system.h>

#include "asim/syntax.h"
#include "asim/restricted/mingo.h"
#include "asim/restricted/mingo_pvt.h"

//
// Define a private structure for passing control messages between Asim
// and the Mingo feeder process.
//

#define ASIMBUFTAG 0xad1e4ad1e4a55

typedef struct
{
    UINT64  asimBufTag;     // Store a magic number as a quick confirmation
    UINT64  bufferSize;
    UINT64  skipEvents;     // Number of events to skip before sending one to asim
    int     maxThreads;
    int     nThreads;       // Number of active threads
    int     nHardwareContexts;
    pid_t   asimPid;
    int     disableEvents;  // Counter of number of calls to disable events
    int     sentEvent;      // Set to 1 if at least one event has been sent.
                            // Useful for discovering when feeder starts
                            // sending data.
    int     abort;          // Set to 1 to get feeder process to exit
}
MINGO_IPC_CONTROL_CLASS;

typedef MINGO_IPC_CONTROL_CLASS *MINGO_IPC_CONTROL;


//
// Define a private structure for passing per-thread event messages between
// Asim and the Mingo feeder process.
//
typedef struct
{
    MINGO_PACKET_CLASS          packet;
    MINGO_RESPONSE_PACKET_CLASS response;

    MINGO_IPC_STATE state;      // Semaphore for communicating state between
                                // feeder and Asim.

    UINT32          cpu_id;     // Set by MINGO_Thread_Preferred_CPU()
    INT32           priority;   // Set by MINGO_Thread_Priority()
    struct
    {
        int new_cpu_id      : 1;    // cpu_id has new value
        int new_priority    : 1;    // priority has new value
        int packet_consumed : 1;    // internal error checking
        int pseudo_packet   : 1;    // hack for noting that a pseudo event
                                    // was returned by recv_event.  The
                                    // next attempt to change state is
                                    // handled specially.
    }
    flags;
}
MINGO_IPC_THREAD_ENTRY_CLASS;

typedef MINGO_IPC_THREAD_ENTRY_CLASS *MINGO_IPC_THREAD_ENTRY;


static MINGO_IPC_CONTROL        ipc_control;
static MINGO_IPC_THREAD_ENTRY   ipc_buffer;


int
MINGO_PVT_Create_IPC_Region(
    int maxThreads,
    int nHardwareContexts
)
{
    MINGO_IPC_CONTROL_CLASS control;
    MINGO_IPC_THREAD_ENTRY_CLASS tentry;
    int fd;
    char fname[128];
    void *mem;
    int i;
    size_t bufferSize;

    strcpy(fname, "/tmp/mingo_pvt.XXXXXX");

    fd = mkstemp(fname);
    if (fd == -1)
    {
        fprintf(stderr, "MINGO_PVT_Create_IPC_Region: Error opening temp file\n");
        return -1;
    }

    unlink(fname);

    //
    // Write to the file to extend its length.  Write an extra thread entry
    // because thread ids are 1 based and it seems dangerous to have every
    // array access in the code subtract 1 from thread id.
    //
    bzero((void *)&control, sizeof(control));
    if (write(fd, (void *)&control, sizeof(control)) == -1)
    {
        fprintf(stderr, "MINGO_PVT_Create_IPC_Region: Write error\n");
        return -1;
    }
    bzero((void *)&tentry, sizeof(tentry));
    for (i = 0; i < (maxThreads + 1); i++)
    {
        if (write(fd, (void *)&tentry, sizeof(tentry)) == -1)
        {
            fprintf(stderr, "MINGO_PVT_Create_IPC_Region: Write error\n");
            return -1;
        }
    }

    bufferSize = sizeof(control) + (maxThreads + 1) * sizeof(tentry);
    mem = mmap(0, bufferSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mem == (void *)-1)
    {
        fprintf(stderr, "MINGO_PVT_Create_IPC_Region: mmap failed\n");
        return -1;
    }

    //
    // Probably not needed, but make sure file stays open across exec()
    //
    fcntl(fd, F_SETFD, 0);

    //
    // These are the shared buffers...
    //
    ipc_control = (MINGO_IPC_CONTROL) mem;
    ipc_buffer = (MINGO_IPC_THREAD_ENTRY) (ipc_control + 1);

    ipc_control->asimBufTag = ASIMBUFTAG;
    ipc_control->bufferSize = bufferSize;
    ipc_control->maxThreads = maxThreads;
    ipc_control->asimPid = getpid();
    ipc_control->nHardwareContexts = nHardwareContexts;

    return fd;
}

int
MINGO_PVT_Open_IPC(
    int fd
)
{
    size_t bufferSize;

    //
    // First just map the control record to figure out the size of the
    // whole buffer.
    //
    ipc_control = mmap(0, sizeof(MINGO_IPC_CONTROL),
                       PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ipc_control == (MINGO_IPC_CONTROL)-1)
    {
        fprintf(stderr, "MINGO_PVT_Open_IPC: mmap failed\n");
        return -1;
    }

    if (ipc_control->asimBufTag != ASIMBUFTAG)
    {
        fprintf(stderr, "MINGO_PVT_Open_IPC: IPC buffer has incorrect tag\n");
        return -1;
    }

    //
    // Now map the whole buffer.
    //
    bufferSize = ipc_control->bufferSize;
    munmap((void *)ipc_control, sizeof(MINGO_IPC_CONTROL));

    ipc_control = mmap(0, bufferSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ipc_control == (MINGO_IPC_CONTROL)-1)
    {
        fprintf(stderr, "MINGO_PVT_Open_IPC: mremap failed\n");
        return -1;
    }

    close(fd);

    ipc_buffer = (MINGO_IPC_THREAD_ENTRY) (ipc_control + 1);

    return 0;
}


void
MINGO_PVT_Abort(void)
{
    ipc_control->abort = 1;
}


int
MINGO_PVT_Check_Abort(void)
{
    return ipc_control->abort;
}


int
MINGO_PVT_Get_Hardware_Context_Count(void)
{
    return ipc_control->nHardwareContexts;
}


pid_t
MINGO_PVT_Get_Asim_Pid(void)
{
    return ipc_control->asimPid;
}


MINGO_IPC_STATE
MINGO_PVT_Get_State(
    int thread
)
{
    return ipc_buffer[thread].state;
}


int
MINGO_PVT_Set_State(
    int thread,
    MINGO_IPC_STATE state
)
{
    int ok;

    //
    // Check for legal state change
    //
    if (state != MINGO_STATE_EMPTY)
    {
        if ((state == MINGO_STATE_THREAD_DEAD) &&
            (ipc_buffer[thread].state == MINGO_STATE_EMPTY))
        {
            //
            // Ok to go from empty to dead
            //
        }
        else if (state != ipc_buffer[thread].state + 1)
        {
            fprintf(stderr, "MINGO_PVT_Set_State:  Illegal state change\n");
            return -1;
        }

        //
        // Check for pseudo packet.  If packet was pseudo there must be a real
        // packet still sitting in the buffer.
        //
        if ((state == MINGO_STATE_ASIM_ACK) &&
            ipc_buffer[thread].flags.pseudo_packet)
        {
            state = MINGO_STATE_EVENT_READY;
            ipc_buffer[thread].flags.pseudo_packet = 0;
        }

        //
        // Was packet actually consumed then state reaches consumed?
        //
        if ((state == MINGO_STATE_ASIM_ACK) &&
            ! ipc_buffer[thread].flags.packet_consumed)
        {
            fprintf(stderr, "MINGO_PVT_Set_State:  Dropped packet\n");
            return -1;
        }
    }

    mb();
    ipc_buffer[thread].state = state;
    mb();

    return 0;
}
            

void
MINGO_PVT_Disable_Events(void)
{
    ipc_control->disableEvents += 1;
}


void
MINGO_PVT_Enable_Events(void)
{
    if (ipc_control->disableEvents)
    {
        ipc_control->disableEvents -= 1;
    }
}


void
MINGO_PVT_Skip_Events(
    UINT64 nEvents)
{
    if (ipc_control->skipEvents)
    {
        fprintf(stderr, "MINGO_PVT_Skip_Events:  Already skipping\n");
    }
    ipc_control->skipEvents = nEvents;
}


int
MINGO_PVT_Test_Events_Enabled(void)
{
    return (ipc_control->disableEvents == 0);
}


int
MINGO_PVT_Test_First_Event_Received(void)
{
    return ipc_control->sentEvent;
}


int
MINGO_PVT_Recv_Event(
    int thread,
    MINGO_PACKET packet
)
{
    if (ipc_buffer[thread].state == MINGO_STATE_EVENT_READY)
    {
        //
        // Was a new preferred CPU requested?
        //
        if (ipc_buffer[thread].flags.new_cpu_id)
        {
            bzero((void *)packet, sizeof(MINGO_PACKET_CLASS));
            packet->msg = MINGO_MSG_PREFERRED_CPU;
            packet->thread_id = thread;
            packet->d.cpu_id = ipc_buffer[thread].cpu_id;
            ipc_buffer[thread].flags.new_cpu_id = 0;
            ipc_buffer[thread].flags.pseudo_packet = 1;
            return 1;
        }
        
        //
        // Was a new thread priority requested?
        //
        if (ipc_buffer[thread].flags.new_priority)
        {
            bzero((void *)packet, sizeof(MINGO_PACKET_CLASS));
            packet->msg = MINGO_MSG_THREAD_PRIORITY;
            packet->thread_id = thread;
            packet->d.priority = ipc_buffer[thread].priority;
            ipc_buffer[thread].flags.new_priority = 0;
            ipc_buffer[thread].flags.pseudo_packet = 1;
            return 1;
        }
        
        //
        // Return the packet
        //
        ipc_buffer[thread].flags.packet_consumed = 1;
        memcpy((void *)packet, (void *)&ipc_buffer[thread].packet,
               sizeof(MINGO_PACKET_CLASS));
        return 1;
    }
    else
    {
        return 0;
    }
}
    
    
int
MINGO_PVT_Send_Event(
    int thread,
    MINGO_PACKET packet
)
{
    if (ipc_control->disableEvents) return -1;
    if (ipc_control->skipEvents)
    {
        ipc_control->skipEvents -= 1;
        return -2;
    }

    ipc_buffer[thread].flags.packet_consumed = 0;
    memcpy((void *)&ipc_buffer[thread].packet, (void *)packet,
           sizeof(MINGO_PACKET_CLASS));
    ipc_control->sentEvent = 1;
    MINGO_PVT_Set_State(thread, MINGO_STATE_EVENT_READY);

    return 0;
}


int
MINGO_PVT_Recv_Response(
    int thread,
    MINGO_RESPONSE_PACKET packet
)
{
    if (ipc_buffer[thread].state == MINGO_STATE_ASIM_ACK)
    {
        memcpy((void *)packet, (void *)&ipc_buffer[thread].response,
               sizeof(MINGO_RESPONSE_PACKET_CLASS));
        return 1;
    }
    else
    {
        return 0;
    }
}


int
MINGO_PVT_Send_Response(
    int thread,
    MINGO_RESPONSE_PACKET packet
)
{
    if ((packet->msg == MINGO_RESPONSE_PROBE) &&
        ipc_buffer[thread].flags.pseudo_packet)
    {
        fprintf(stderr, "MINGO_PVT_Send_Response:  Probe must follow a real packet.  Sorry!\n");
        return -1;
    }

    memcpy((void *)&ipc_buffer[thread].response, (void *)packet,
           sizeof(MINGO_RESPONSE_PACKET_CLASS));
    return MINGO_PVT_Set_State(thread, MINGO_STATE_ASIM_ACK);
}


//
//   ****
//   **** THIS CALL IS NOT THREAD SAFE!!!!  Caller is responsible for making
//   **** sure only one thread calls this at a time.
//   ****
//
int
MINGO_PVT_New_Thread(void)
{
    if (ipc_control->nThreads == ipc_control->maxThreads) return -1;

    ipc_control->nThreads += 1;
    return ipc_control->nThreads;
}

int
MINGO_PVT_Allocated_Threads(void)
{
    return ipc_control->nThreads;
}


int
MINGO_PVT_Set_Priority(
    int thread,
    INT32 priority)
{
    if (ipc_buffer[thread].state != MINGO_STATE_EMPTY)
    {
        fprintf(stderr, "MINGO_PVT_Set_Priority:  invalid thread state\n");
        return -1;
    }

    ipc_buffer[thread].priority = priority;
    ipc_buffer[thread].flags.new_priority = 1;

    return 0;
}


int
MINGO_PVT_Set_Preferred_CPU(
    int thread,
    UINT32 cpu_id)
{
    if (ipc_buffer[thread].state != MINGO_STATE_EMPTY)
    {
        fprintf(stderr, "MINGO_PVT_Set_Preferred_CPU:  invalid thread state\n");
        return -1;
    }

    ipc_buffer[thread].cpu_id = cpu_id;
    ipc_buffer[thread].flags.new_cpu_id = 1;

    return 0;
}
