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
 * @file mingo_pvt.h
 * @author Michael Adler
 * @date March, 2002
 *
 * Mingo internal data structures for passing information between client
 * and ASIM feeder.
 *
 * The data structures must work with both C and C++ since the client
 * code is C and ASIM is C++.
 */

#ifndef _MINGO_PVT_
#define _MINGO_PVT_

#include "mingo.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// ***************************************************************************
//  IF YOU CHANGE THE FOLLOWING ENUM MAKE SURE YOU ALSO CHANGE THE field_mask
//  IN mingo_feeder.h. 
// ***************************************************************************
//
typedef enum
{
    MINGO_MSG_NONE,
    MINGO_MSG_THREAD_END,
    MINGO_MSG_THREAD_PRIORITY,
    MINGO_MSG_PREFERRED_CPU,
    MINGO_MSG_MEMORY_READ,
    MINGO_MSG_MEMORY_WRITE,
    MINGO_MSG_EXCHANGE,
    MINGO_MSG_MEMORY_FENCE,
    MINGO_MSG_DEPENDENT_READ,
    MINGO_MSG_BRANCH,
    MINGO_MSG_SYSCALL,
    MINGO_MSG_PROBE_RESULT,
    MINGO_MSG_TIMED_WAIT,
    MINGO_MSG_LAST              // Must be last
}
MINGO_MSG;

typedef enum
{
    MINGO_MSG_MASK_THREAD_END      = 1 << MINGO_MSG_THREAD_END,
    MINGO_MSG_MASK_THREAD_PRIORITY = 1 << MINGO_MSG_THREAD_PRIORITY,
    MINGO_MSG_MASK_PREFERRED_CPU   = 1 << MINGO_MSG_PREFERRED_CPU,
    MINGO_MSG_MASK_MEMORY_READ     = 1 << MINGO_MSG_MEMORY_READ,
    MINGO_MSG_MASK_MEMORY_WRITE    = 1 << MINGO_MSG_MEMORY_WRITE,
    MINGO_MSG_MASK_EXCHANGE        = 1 << MINGO_MSG_EXCHANGE,
    MINGO_MSG_MASK_MEMORY_FENCE    = 1 << MINGO_MSG_MEMORY_FENCE,
    MINGO_MSG_MASK_DEPENDENT_READ  = 1 << MINGO_MSG_DEPENDENT_READ,
    MINGO_MSG_MASK_BRANCH          = 1 << MINGO_MSG_BRANCH,
    MINGO_MSG_MASK_SYSCALL         = 1 << MINGO_MSG_SYSCALL,
    MINGO_MSG_MASK_PROBE_RESULT    = 1 << MINGO_MSG_PROBE_RESULT,
    MINGO_MSG_MASK_TIMED_WAIT      = 1 << MINGO_MSG_TIMED_WAIT,
}
MINGO_MSG_MASK;

//
// All messages from the feeder process to ASIM are passed in
// MINGO_PACKET_CLASS.  Some messages could fit in a smaller data
// structure, but the most frequent messages all need nearly the
// maximum size.  Fixed size messages make the receiving side simpler.
//
// The bit fields in "info" remain undecoded.  We may as well keep
// them compressed until they make it into ASIM's address space.
//

typedef struct
{
    MINGO_MSG   msg;
    UINT32      thread_id;

    UINT64      ip;
    UINT64      va;
    UINT64      compare_value;  // Comparator in MINGO_Memory_Exchange

    union
    {
        INT32   priority;       // MINGO_Thread_Priority
        UINT32  cpu_id;         // MINGO_Thread_Preferred_CPU
        UINT64  info;           // almost everything else that passes data
        MINGO_INFO minfo;       // another view of the info bits
    }
    d;
}
MINGO_PACKET_CLASS;

typedef MINGO_PACKET_CLASS *MINGO_PACKET;


//
// Response record sent from ASIM to feeder process.
//

typedef enum
{
    MINGO_RESPONSE_NONE,
    MINGO_RESPONSE_ACK,
    MINGO_RESPONSE_PROBE
}
MINGO_RESPONSE;

typedef struct
{
    MINGO_RESPONSE  msg;
    union
    {
        struct
        {
            UINT64  va;
            UINT32  nBytes;
        }
        probe;
    } u;
}
MINGO_RESPONSE_PACKET_CLASS;

typedef MINGO_RESPONSE_PACKET_CLASS *MINGO_RESPONSE_PACKET;


//
// Define the shared memory buffer and data structures for transferring
// packets and responses.
//

typedef enum
{
    MINGO_STATE_EMPTY,
    MINGO_STATE_EVENT_READY,
    MINGO_STATE_ASIM_CONSUMED,
    MINGO_STATE_ASIM_ACK,
    MINGO_STATE_THREAD_DEAD
}
MINGO_IPC_STATE;

//
// Create a shared memory region and return the associated file descriptor.
// Return -1 on error.
//
int
MINGO_PVT_Create_IPC_Region(
    int maxThreads,
    int nHardwareContexts
);

//
// Open shared storage already created by a call to MINGO_PVT_Create_IPC_Region.
// Return -1 on error.
//
int
MINGO_PVT_Open_IPC(
    int fd
);

//
// How many hardware contexts are being simulated?
//
int
MINGO_PVT_Get_Hardware_Context_Count(void);

//
// Send an abort message from one process to another.  Return non-zero for abort.
//
void
MINGO_PVT_Abort(void);

//
// Check for an abort.
//
int
MINGO_PVT_Check_Abort(void);

//
// Get Asim's pid from the control block.
//
pid_t
MINGO_PVT_Get_Asim_Pid(void);

//
// MINGO_PVT_Get_State --
//  Get the current state of a thread's IPC slot.
//
MINGO_IPC_STATE
MINGO_PVT_Get_State(
    int thread
);

//
// MINGO_PVT_Set_State --
//  Set the state of a thread's IPC slot.  Return 0 for no error, -1 on error.
//
int
MINGO_PVT_Set_State(
    int thread,
    MINGO_IPC_STATE state
);

//
// MINGO_PVT_Disable_Events --
//  Note that no events are being passed to Asim.
//
void
MINGO_PVT_Disable_Events(void);

//
// MINGO_PVT_Enable_Events --
//  Note that events are being passed to Asim.
//
void
MINGO_PVT_Enable_Events(void);

//
// MINGO_PVT_Skip_Events --
//  Skip a number of events.
//
void
MINGO_PVT_Skip_Events(
    UINT64 nEvents);

//
// MINGO_PVT_Test_Events_Enabled --
//  Test the events disabled/enabled flag.
int
MINGO_PVT_Test_Events_Enabled(void);

//
// MINGO_PVT_Test_First_Event_Received --
//  Has at least one event been received from the feeder process?
//
int
MINGO_PVT_Test_First_Event_Received(void);

//
// MINGO_PVT_Recv_Event --
//  If an event is available (thread is in EVENT_READY state), copy
//  the event into the buffer to which packet points.
//
//  Return 0 if no event available, otherwise 1.
//
int
MINGO_PVT_Recv_Event(
    int thread,
    MINGO_PACKET packet
);
    
//
// MINGO_PVT_Send_Event --
//  Write a new event to the IPC buffer.
//
//  Return 0 on success, -1 if events disabled and -2 if currently skipping
//  events.
//
int
MINGO_PVT_Send_Event(
    int thread,
    MINGO_PACKET packet
);
    
//
// MINGO_PVT_Recv_Response --
//  If a reply is available (thread is in ASIM_ACK state), return 1 and
//  copy the reply into the buffer to which reply points.  If no reply is
//  available return 0.
//
int
MINGO_PVT_Recv_Response(
    int thread,
    MINGO_RESPONSE_PACKET packet
);
    
//
// MINGO_PVT_Send_Response --
//  Write a response to the IPC buffer.  Return non-zero on error.
//
int
MINGO_PVT_Send_Response(
    int thread,
    MINGO_RESPONSE_PACKET packet
);
    

//
// MINGO_PVT_New_Thread --
//  Called in feeder process to allocate a new thread id.  Return -1 on error.
//
//   ****
//   **** THIS CALL IS NOT THREAD SAFE!!!!  Caller is responsible for making
//   **** sure only one thread calls this at a time.
//   ****
//
int
MINGO_PVT_New_Thread(void);

//
// MINGO_PVT_Allocated_Threads --
//  Return the number of thread ids allocated threads.  This number does not
//  decrease as threads exit.
//
int
MINGO_PVT_Allocated_Threads(void);

//
// MINGO_PVT_Set_Priority --
//  Pass a requested thread priority.  Return 0 on success, -1 on failure.
//
int
MINGO_PVT_Set_Priority(
    int thread,
    INT32 priority
);

//
// MINGO_PVT_Set_Preferred_CPU --
//  Pass a preferred CPU.  Return 0 on success, -1 on failure.
//
int
MINGO_PVT_Set_Preferred_CPU(
    int thread,
    UINT32 cpu_id
);


#ifdef __cplusplus
}
#endif

#endif  // _MINGO_PVT_
