// 5/9/2002: Added dummy Done() to keep mingoint happy.

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
 * @file mingo_feeder.h
 * @author Michael Adler
 * @date March, 2002
 *
 * ASIM side of Mingo feeder.  
 */

#ifndef _MINGO_FEEDER_
#define _MINGO_FEEDER_

#include "asim/syntax.h"
#include "asim/restricted/mingo.h"
#include "asim/restricted/mingo_reg.h"
#include "asim/restricted/mingo_pvt.h"


//
// MINGO_DATA_CLASS --
//  The container for a packet of data coming from the feeder, returned by
//  MINGO_FEEDER::GetNextSoftwareThreadEvent()
//

class MINGO_DATA_CLASS : private MINGO_PACKET_CLASS
{
  public:
    //
    // Packets come from the feeder process as raw MINGO_PACKET_CLASS.
    // This constructor is a simple translation to the C++ version.
    //
    MINGO_DATA_CLASS(
        MINGO_PACKET_CLASS packet)
    {
        msg = packet.msg;
        thread_id = packet.thread_id;
        ip = packet.ip;
        va = packet.va;
        compare_value = packet.compare_value;
        d.info = packet.d.info;


        if ((msg == MINGO_MSG_MEMORY_READ) && (get_load_type() == LDTYPE_NONE))
        {
            if (FORCE_LOAD_ACQUIRE)
            {
                d.info |= ((UINT64)LDTYPE_ACQ << 34);
            }
            else if (FORCE_LOAD_ACQUIRE_NON_SP && ! get_is_sp_relative())
            {
                d.info |= ((UINT64)LDTYPE_ACQ << 34);
            }
        }

        if (msg == MINGO_MSG_MEMORY_WRITE)
        {
            if (FORCE_STORE_RELEASE)
            {
                d.info |= (1LL << 34);
            }
            else if (FORCE_STORE_RELEASE_NON_SP && ! get_is_sp_relative())
            {
                d.info |= (1LL << 34);
            }
        }
    };

    //
    // Simple constructor with undefined initial values
    //
    MINGO_DATA_CLASS()
    {};

    ~MINGO_DATA_CLASS()
    {};

    //
    // Access functions for data fields.  Most access functions check to
    // make sure the field is valid for the request.
    //

    MINGO_MSG get_msg(void) { return msg; }

    UINT32 get_thread_id(void) { return thread_id; };
    
    UINT64 get_ip(void)
    {
        validate_request(MF_IP);
        return ip;
    };
    
    UINT64 get_va(void)
    {
        validate_request(MF_VA);
        return va;
    };
    
    UINT64 get_branch_target_ip(void)
    {
        validate_request(MF_TGTIP);
        return va;
    };
    
    INT32 get_priority(void)
    {
        validate_request(MF_PRIORITY);
        return d.priority;
    };
    
    UINT32 get_cpu_id(void)
    {
        validate_request(MF_CPUID);
        return d.cpu_id;
    };

    UINT32 get_instruction_count(void)
    {
        // This returns a count of "uninteresting" instructions that should
        // be issued before the current "interesting" instruction.
        
        //
        // Don't require that packet have an icount.  Just return 0 if it
        // doesn't.  This makes it easy to skip certain packets but keep
        // a count of instructions.
        //
        if ((msg >= MINGO_MSG_LAST) || ((field_mask[msg] & MF_ICOUNT) == 0))
            return 0;
        else
            return d.info & 0xffff;
    };

    UINT32 get_access_size(void)
    {
        validate_request(MF_ASIZE);
        return (d.info >> 16) & 0xf;
    };

    bool get_is_lock_request(void)
    {
        validate_request(MF_LDST);
        return (d.info >> 20) & 1 == 1;
    };

    bool get_is_sp_relative(void)
    {
        validate_request(MF_LDST);
        return (d.info >> 31) & 1 == 1;
    };

    MINGO_REG get_branch_target_register(void)
    {
        validate_request(MF_REGBRA);
        return (MINGO_REG) ((d.info >> 21) & 0x1ff);
    };

    bool get_is_branch_conditional(void)
    {
        validate_request(MF_CONDBRA);
        return (d.info >> 30) & 1 == 1;
    };

    bool get_is_branch_taken(void)
    {
        validate_request(MF_TAKENBRA);
        return (d.info >> 31) & 1 == 1;
    };

    MINGO_REG get_source_register(void)
    {
        validate_request(MF_REGSRC);
        return (MINGO_REG) ((d.info >> 21) & 0x1ff);
    };

    MINGO_REG get_target_register(void)
    {
        validate_request(MF_REGTGT);
        return (MINGO_REG) ((d.info >> 21) & 0x1ff);
    };

    MINGO_MEMORY_FENCE get_memory_fence_type(void)
    {
        validate_request(MF_FENCE);
        return (MINGO_MEMORY_FENCE) ((d.info >> 16) & 0x7);
    };

    MINGO_CONTROL_TRANSFER get_branch_type(void)
    {
        validate_request(MF_XFER);
        return (MINGO_CONTROL_TRANSFER) ((d.info >> 16) & 0x7);
    };

    UINT64 get_probe_value(void)
    {
        validate_request(MF_PROBE);
        return d.info;
    };

    UINT64 get_timed_wait(void)
    {
        validate_request(MF_WAIT);
        return d.info;
    };

    bool get_branch_is_register_indirect(void)
    {
        validate_request(MF_INDIRECT);
        return (d.info >> 19) & 1 == 1;
    };

    MINGO_SYSCALL get_syscall_type(void)
    {
        validate_request(MF_SYSCALL);
        return (MINGO_SYSCALL) ((d.info >> 16) & 0x7);
    };

    bool get_is_compare(void)
    {
        validate_request(MF_CMP);
        return (d.info >> 20) & 1 == 1;
    };
    
    bool get_is_release(void)
    {
        validate_request(MF_RELACQ);
        return (d.info >> 30) & 1 == 1;
    };
    
    bool get_is_acquire(void)
    {
        validate_request(MF_RELACQ);
        return (d.info >> 31) & 1 == 1;
    };
    
    UINT64 get_compare_value(void)
    {
        if (! get_is_compare())
        {
            fprintf(stderr, "MINGO_DATA: Request for data from inactive field.\n");
            exit(1);
        }

        return compare_value;
    };
    
    enum MEMHINT
    {
        MEMHINT_NONE,
        MEMHINT_NT1,
        MEMHINT_NTA = 3
    };
    
    MEMHINT
    get_memory_hint(void)
    {
        validate_request(MF_LDST);
        return (MEMHINT)((d.info >> 32) & 3);
    };
    
    enum LDTYPE
    {
        LDTYPE_NONE         = 0,
        LDTYPE_S            = 1,
        LDTYPE_A            = 2,
        LDTYPE_SA           = 3,
        LDTYPE_BIAS         = 4,
        LDTYPE_ACQ          = 5,
        LDTYPE_FILL         = 6,
        LDTYPE_C_CLR        = 8,
        LDTYPE_C_NC         = 9,
        LDTYPE_C_CLR_ACQ    = 10
    };

    LDTYPE
    get_load_type(void)
    {
        validate_request(MF_LDTYPE);
        return (LDTYPE)((d.info >> 34) & 0xf);
    };

    bool get_is_store_release(void)
    {
        validate_request(MF_STTYPE);
        return (d.info >> 34) & 1 == 1;
    }

    bool get_is_store_spill(void)
    {
        validate_request(MF_STTYPE);
        return (d.info >> 35) & 1 == 1;
    }

    const void
    DumpPacket(
        FILE *str);

  private:
    //
    // FIELDS and field_mask will be used to verify that requested fields
    // in access functions are valid for specific packet types.
    //
    enum FIELDS
    {
        MF_ASIZE    = 1,
        MF_CPUID    = 2,
        MF_FENCE    = 4,
        MF_ICOUNT   = 8,
        MF_INDIRECT = 16,
        MF_IP       = 32,
        MF_LDST     = 64,
        MF_PRIORITY = 128,
        MF_REGBRA   = 256,
        MF_REGSRC   = 512,
        MF_REGTGT   = 1024,
        MF_SYSCALL  = 2048,
        MF_TGTIP    = 4096,
        MF_VA       = 8192,
        MF_XFER     = 16384,
        MF_PROBE    = 32768,
        MF_CMP      = 65536,
        MF_RELACQ   = 131072,
        MF_CONDBRA  = 262144,
        MF_TAKENBRA = 524288,
        MF_LDTYPE   = 1048576,
        MF_STTYPE   = 2097152,
        MF_WAIT     = 4194304
    };

    //
    // Mask of fields allowed, indexed by message type.  This static storage
    // is defined in mingo_feeder.cpp.
    //
    static const UINT32 field_mask[MINGO_MSG_LAST] =
    {
        /* NONE */              0,
        /* THREAD_END */        0,
        /* THREAD_PRIORITY */   MF_PRIORITY,
        /* PREFERRED_CPU */     MF_CPUID,
        /* MEMORY_READ */       MF_IP | MF_VA | MF_LDST | MF_ASIZE | MF_ICOUNT | MF_LDTYPE | MF_REGTGT,
        /* MEMORY_WRITE */      MF_IP | MF_VA | MF_LDST | MF_ASIZE | MF_ICOUNT | MF_STTYPE,
        /* EXCHANGE */          MF_IP | MF_VA | MF_CMP | MF_ASIZE | MF_REGTGT | MF_RELACQ | MF_ICOUNT,
        /* MEMORY_FENCE */      MF_IP | MF_FENCE | MF_ICOUNT,
        /* DEPENDENT_READ */    MF_IP | MF_REGSRC | MF_ICOUNT,
        /* BRANCH */            MF_IP | MF_TGTIP | MF_REGBRA | MF_INDIRECT | MF_XFER | MF_ICOUNT |
                                    MF_CONDBRA | MF_TAKENBRA,
        /* SYSCALL */           MF_IP | MF_SYSCALL | MF_ICOUNT,
        /* PROBE_RESULT */      MF_VA | MF_PROBE,
        /* TIMED_WAIT */        MF_WAIT
    };

    void
    validate_request(
        FIELDS field)
    {
        if ((msg >= MINGO_MSG_LAST) || ((field_mask[msg] & field) == 0))
        {
            //
            // XXX replace with ASIM error routine
            //
            fprintf(stderr, "MINGO_DATA: Request for data from inactive field.\n");
            exit(1);
        }
    };
};

typedef MINGO_DATA_CLASS *MINGO_DATA;



//
// MINGO_FEEDER_CLASS --
//  Controlling class for the feeder process.  The constructor forks a feeder
//  and the class manages I/O with the feeder.
//

class MINGO_FEEDER_CLASS
{
  public:
    static const int maxthreads = 256;

    MINGO_FEEDER_CLASS(
        const char *path,
        char * const argv[],
        char * envp[],
        int nHardwareContexts,
        char *stdinPath = NULL,
        char *stdoutPath = NULL,
        char *stderrPath = NULL
        );

    ~MINGO_FEEDER_CLASS();

    //
    // Returns true iff feeder has exited
    //
    bool EndOfData();

    //
    // Returns non-zero value if a new thread has started.  Returned value
    // is the thread_id of the new thread.  Caller should continue calling
    // the function until it returns 0 in order to learn about all new
    // threads.
    //
    UINT32 CheckForNewSoftwareThread();

    //
    // Returns true if an event is available for the given thread.
    // If the event argument is NULL, the function merely indicates
    // whether an event is available and keeps the data queued until
    // the function is called again with a non-NULL event pointer.
    //
    // If waituS is non-zero then the routine will block for up to
    // waituS microseconds before giving up and returning false.
    //
    bool GetNextSoftwareThreadEvent(
        UINT32 thread_id,
        MINGO_DATA event = NULL,
        UINT32 waituS = 0);

    //
    // Acknowledge an event returned by GetNextThreadEvent() and allow
    // the thread to proceed.  Threads are blocked until this is called.
    //
    void AllowSoftwareThreadProgress(UINT32 thread_id);

    //
    // The feeder process can be told not to send events (fast forward)
    // until a certain number of events are seen.  Critical events are
    // not counted.  (See MINGO_MSG_MASK_CRITICAL.)
    //
    void FastForwardNEvents(
        UINT64 nEvents);

    //
    // Probe memory in the feeder process.  This is a slow operation.
    // A message is sent back to the feeder process to read memory and
    // the call blocks until the response is received.
    //
    UINT64 ProbeMemory(
        UINT32 thread_id,
        UINT64 va,
        UINT32 nBytes);

    //
    // Called by FEED_Done when Asim terminates a simulation run.
    //
    void Done(void);


  private:
    int nHWContexts;        // Number of ASIM hardware contexts

    //
    // Sleep while the feeder has events disabled.
    //
    void WaitForFeederEvents();
};

typedef MINGO_FEEDER_CLASS *MINGO_FEEDER;

#endif  // _MINGO_FEEDER_
