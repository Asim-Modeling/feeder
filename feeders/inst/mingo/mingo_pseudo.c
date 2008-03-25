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
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "mingo.h"

HOST_UINT info;
HOST_UINT ip;
HOST_UINT va;
UINT64 write_info;

// Amounts to shift values for filling in the info field.

#define count_shift 0
#define size_shift 16
#define target_reg_shift 21
#define sp_shift 31

// Dependent read only
#define source_reg_shift 21


// Memory fence only

#define fence_type_shift 16

// Exchange only

#define acquire_shift 31
#define release_shift 30
#define compare_shift 20

static void *
pseudo_thread(
    void *arg)
{
    HOST_UINT x = (HOST_UINT) arg << 16;
    HOST_UINT i;

    if ((HOST_UINT) arg != 1)
    {
        MINGO_Thread_Start();
    }


    MINGO_Thread_Priority((HOST_UINT)arg+10);
    MINGO_Thread_Preferred_CPU((HOST_UINT)arg);
    MINGO_INFO m;

    //fprintf(stderr, "MINGO_INFO bytes = %d\n", sizeof(MINGO_INFO));

    HOST_UINT ipx = 0x1000;
    for (i = 0; i < 5; i++)
    {
//        if (random() & 4) sleep(1);

        // Read info mask:

        ipx += 16; // skip one intervening insn
        // corresponds to inst_count=1 below

        m.w=0;
        m.mem_read.register_target=8;
        m.mem_read.inst_count=1;
        m.mem_read.access_size=2;
        //fprintf(stderr,"info = %016llx\n", m.w);
        MINGO_Memory_Read(ipx, (x | i) + 128, m.w);

        ipx += 16; // the read we just did...

        // Write info mask:
        // SP-relative is set when i is 1 or 3
        // Access size: 1

        m.w = 0;
        ipx += 16; // skip one intervening insn
        m.mem_write.inst_count=1;
        m.mem_write.access_size = 1;
        MINGO_Memory_Write(ipx, (x | i) + 1, m.w);
        ipx += 16; // the write we just did.
    }

    // Exchange with acquire. 
    // Target register: 5
    // Access size: 1
    // Intervening instructions: 0

    m.w = 0;
    m.mem_exchange.access_size=1;
    m.mem_exchange.register_target=5;
    m.mem_exchange.acquire=1;
    MINGO_Memory_Exchange(ipx, (HOST_UINT)&pseudo_thread, m.w, 0);
    ipx += 16;

    // Exchange with release.
    // Target register: 5
    // Compare and exchange
    // Access size: 2
    // Compare value: 12513661
    // Intervening instructions: 7

    m.w = 0;
    ipx += 7*16;
    m.mem_exchange.inst_count = 7;
    m.mem_exchange.compare = 1;
    m.mem_exchange.register_target = 5;
    m.mem_exchange.access_size = 2;
    m.mem_exchange.release = 1;

    MINGO_Memory_Exchange(ipx, (HOST_UINT)&pseudo_thread+8, m.w, 0x12513661);
    ipx += 16;

    // Exchange with acquire and exchange
    // Target register: 5
    // Compare and exchange
    // Access size: 8
    // Intervening instructions: 8

    m.w=0;
    m.mem_exchange.inst_count = 8;
    ipx += 8*16;
    m.mem_exchange.compare = 1;
    m.mem_exchange.register_target = 5;
    m.mem_exchange.access_size = 3;
    m.mem_exchange.acquire = 1;

    MINGO_Memory_Exchange64(ipx , (HOST_UINT)&pseudo_thread+16,
                            m.w,  0xba9876543210);
    ipx += 16;

    m.w=0;
    m.mem_fence.mem_fence_enum = 1;
    m.mem_exchange.inst_count = 5;
    ipx += 5*16;
    MINGO_Memory_Fence(ipx, m.w);
    ipx += 16;

    m.w=0;
    m.dependent_read.source_register = 5;
    m.dependent_read.inst_count = 1;
    ipx += 1*16;
    MINGO_Dependent_Read(ipx, m.w);
    ipx += 16;

    MINGO_Note_Executed_Instrs(ipx, 20);
    ipx += 20*16;

    m.w = 0;
    m.branch.inst_count = 7;
    ipx += 7*16;
    m.branch.xfer_enum = 1;
    m.branch.indirect = 1;
    m.branch.taken = 1;
    m.branch.branch_target_register = 7;
    MINGO_Branch(ipx, 0x2345, m.w);
    ipx = 0x2345; // fix the tgt

    m.w = 0;
    ipx += 5*16; // skip 5 intervening insts
    m.mem_write.inst_count=5;
    m.mem_write.access_size = 1;
    MINGO_Memory_Write(ipx, (x | i) + 1, m.w);
    ipx += 16; // the write we just did.
    
    MINGO_Syscall(ipx, (2 << 16) | 2);
    ipx += 16;
    MINGO_Timed_Wait(100L+(UINT64)((HOST_UINT)arg));

    // Additions by Judy Hall to test specific interactions between operations.

    // Acceptance memory fence to clean things out. Used same IP as
    // previous fence to avoid cache miss.

    info = 1 << fence_type_shift | 0 << count_shift;
    MINGO_Memory_Fence(ipx,info);
    ipx  += 16;
    
    // Regular write

    va = x | 0x23;
    info = 2 << size_shift | 0 << count_shift;
    MINGO_Memory_Write(ipx, va, info);
    ipx  += 16;

    // Read. Expect merge buffer hit.
    
    va = x | 0x23;
    info = 2 << size_shift | 4 << target_reg_shift | 0 << count_shift;
    MINGO_Memory_Read(ipx, va, info);
    ipx  += 16;
    
    // Read. Expect merge buffer miss because addresses won't match.

    va = x | 0x400;
    info = 2 << size_shift | 5 << target_reg_shift | 0 << count_shift;
    MINGO_Memory_Read(ipx, va, info);
    ipx  += 16;

    // Read. Expect mgb miss because masks won't match.

    va = x | 0x23;
    info = 3 << size_shift | 6 << target_reg_shift | 0 << count_shift;
    MINGO_Memory_Read (ipx, va, info);
    ipx  += 16;

    // Dependent op. Will wait for read. Same IP to avoid istream miss.

    info = 6 << source_reg_shift | 0 << count_shift;
    MINGO_Dependent_Read(ipx, info);
    ipx  += 16;

    // Exchange.release. Won't wait for completion

    va = (HOST_UINT)&pseudo_thread | 0x23;
    info = 0 << acquire_shift | 1 << release_shift | 
        4 << target_reg_shift | 0 << compare_shift |
        2 << size_shift | 0 << count_shift;
    MINGO_Memory_Exchange(ipx, va, info, 0);
    ipx  += 16;
    
    // Dependent op. Should go immediately. Same IP to avoid istream miss.

    info = 4 << source_reg_shift | 0 << count_shift;
    MINGO_Dependent_Read(ipx, info);
    ipx  += 16;
   
    // Read to same address as previous exchange. Should wait for 
    // exchange write to complete.

    va = (HOST_UINT)&pseudo_thread | 0x23;
    info = 2 << size_shift | 6 << target_reg_shift |
        0 << count_shift;
    MINGO_Memory_Read(ipx, va, info);
    ipx  += 16;

    // Store.release. Next read should hit on it
    // Running on X86 the only way to force a store.release is to
    // clear the SP-relative bit, and build with parameters that force
    // store.release for writes that are not SP-relative.

    va = x | 0x120;
    info = 0 << sp_shift | 2 << size_shift | 0 << count_shift;
    MINGO_Memory_Write(ipx, va, info);
    ipx  += 16;

    // Read to same address as previous store release. Use same IP so 
    // we won't get istream miss. Should hit on merge buffer.

    va = x | 0x120;
    info = 2 << size_shift | 6 << target_reg_shift | 0 << count_shift;
    MINGO_Memory_Read(ipx, va, info);
    ipx  += 16;
 
    // Exchange.release. Won't wait for completion.

    va = (HOST_UINT)&pseudo_thread | 0x200;
    info = 0 << acquire_shift | 1 << release_shift | 
        4 << target_reg_shift | 0 << compare_shift |
        2 << size_shift | 0 << count_shift;
    MINGO_Memory_Exchange(ipx, va, info, 0);
    ipx  += 16;
    
    // Regular write to push the merge buffer to the temporary
    // threshold for flushing. We want the exchange write to go out
    // and another write to merge with it.

    va = x | 0x300;
    info = 0 << sp_shift | 2 << size_shift | 0 << count_shift;
    MINGO_Memory_Write(ipx, va, info);
    ipx  += 16;
    
    // Regular write to same VA as exchange, with overlapping bytes. 
    // Intervening instructions so
    // that the first message will go out to the cbox before the
    // second write merges. First message should be write_unlock;
    // second should be write. NOTE: This works only because the
    // purging threshold is 2 for now.

    va = (HOST_UINT)&pseudo_thread | 0x202;
    info = 0 << sp_shift | 2 << size_shift | 5 << count_shift;
    MINGO_Memory_Write(ipx, va, info);
    ipx  += 16;
    
    // Read to a new address.

    va = x | 0x500;
    info = 2 << size_shift | 6 << target_reg_shift | 0 << count_shift;
    MINGO_Memory_Read(ipx, va, info);
    ipx  += 16;

    // Read to an address in the same cache line as the previous
    // read. Expect the MRQ entry to piggyback, resulting in one
    // message to the CBOX.
   
    va = x | 0x504;
    info = 2 << size_shift | 7 << target_reg_shift | 1 << count_shift;
    MINGO_Memory_Read(ipx, va, info);
    ipx  += 16;


    // End with a memory fence so threads won't finish until I/O is
    // complete
    
    info = 1 << fence_type_shift | 0 << count_shift;
    MINGO_Memory_Fence(ipx,info);
    ipx  += 16;

    // End of additions by Judy Hall


    if ((HOST_UINT) arg != 1)
    {
        MINGO_Thread_End();
    }

    return 0;
}


int
main(
    int argc,
    char *argv[])
{
    pthread_t t1, t2;
    int i;

    for (i = 0; i < argc; i++)
    {
        fprintf(stderr, "%s ", argv[i]);
    }
    fprintf(stderr, "\n");
    if (argv[argc])
    {
        fprintf(stderr, "argc / argv size mismatch\n");
        exit(1);
    }

    pthread_create(&t1, NULL, &pseudo_thread, (void *)2);
    pthread_create(&t2, NULL, &pseudo_thread, (void *)3);

    pseudo_thread((void *)1);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);


    fprintf(stderr, "That's all, folks.\n");

    return 0;
}
