/**************************************************************************
 * Copyright (C) 2006 Intel Corporation
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
 **************************************************************************/

/**
 * @file lockstep.cpp
 * @author Michael Adler
 * @brief Lock-step debugging tool -- compare 2 SoftSDV processes
 */

void asim_error_exit(int s);

#define ASSERT(condition,mesg) \
    if (! (condition)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, mesg); \
        asim_error_exit(1); \
    }

#define ASSERTX(condition) \
    if (! (condition)) { \
        fprintf(stderr, "Assert failure in %s:%d\n", __FILE__, __LINE__); \
        asim_error_exit(1); \
    }

#define VERIFY(condition,mesg) ASSERT(condition,mesg)

#define ASIMERROR(mesg) \
    fprintf(stderr, "%s:%d %s\n", __FILE__, __LINE__, mesg); \
    asim_error_exit(1);

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>

//
// ASIM include files
//
#include "asim/provides/softsdv_stub.h"
#include "asim/provides/softsdv_stub_isa.h"
#include "asim/provides/softsdv_cpuapi.h"


bool
ASIM_LOCKSTEP_CLASS::CompareProcesses(void)
{
    newInstrsMD5.finalize();

    //
    // Write the MD5 checksum to the shared memory
    //

    //
    // Wait for consumer to read the last value
    //
    MemBarrier();
    while (dataOut->ready)
    {
        MemBarrier();
        __asm__ ("pause");
    }

    memcpy(dataOut->md5_digest, newInstrsMD5.digest, sizeof(newInstrsMD5.digest));
    MemBarrier();
    dataOut->ready = 1;

    //
    // Read the checksum for the same region from the other process
    //
  try_again:
    MemBarrier();
    while (! dataIn->ready)
    {
        MemBarrier();
        __asm__ ("pause");
    }

    bool sameMD5 = (memcmp(dataIn->md5_digest, newInstrsMD5.digest, sizeof(newInstrsMD5.digest)) == 0);
    bool hello = (dataIn->ready == 2);
    MemBarrier();
    dataIn->ready = 0;

    if (hello)
    {
        // Still in initial synchronization.
        goto try_again;
    }

    newInstrsMD5.reset();
    return sameMD5;
}



ASIM_LOCKSTEP_CLASS::ASIM_LOCKSTEP_CLASS(void)
{
    //
    // Map the shared output buffer
    //
    int fd = open("lockstep_out", O_RDWR);
    if (fd == -1)
    {
        ASIMERROR("LockStep:  failed to open output file (lockstep_out)\n");
    }
    lseek(fd, sizeof(LOCKSTEP_COMM_BUFFER_CLASS) - 1, SEEK_SET);
    char zero = 0;
    write(fd, &zero, 1);

    dataOut = (LOCKSTEP_COMM_BUFFER)
              mmap(0, sizeof(LOCKSTEP_COMM_BUFFER_CLASS), PROT_READ+PROT_WRITE, MAP_SHARED, fd, 0);
    if (dataOut == MAP_FAILED)
    {
        ASIMERROR("LockStep:  failed to map output buffer (lockstep_out)\n");
    }
    close(fd);


    //
    // Map the shared input buffer
    //
    fd = open("lockstep_in", O_RDWR);
    if (fd == -1)
    {
        ASIMERROR("LockStep:  failed to open input file (lockstep_in)\n");
    }
    lseek(fd, sizeof(LOCKSTEP_COMM_BUFFER_CLASS) - 1, SEEK_SET);
    write(fd, &zero, 1);

    dataIn = (LOCKSTEP_COMM_BUFFER)
              mmap(0, sizeof(LOCKSTEP_COMM_BUFFER_CLASS), PROT_READ+PROT_WRITE, MAP_SHARED, fd, 0);
    if (dataIn == MAP_FAILED)
    {
        ASIMERROR("LockStep:  failed to map input buffer (lockstep_in)\n");
    }
    close(fd);

    //
    // Initial handshake.  Send and receive a hello.  The file may already have
    // a hello in it, so the DataRead() method needs to keep checking for it.
    //
    bzero(&(dataOut->md5_digest), sizeof(dataOut->md5_digest));
    MemBarrier();
    dataOut->ready = 2;

    MemBarrier();
    while (dataIn->ready != 2)
    {
        MemBarrier();
        __asm__ ("pause");
    }

    dataIn->ready = 0;
}


ASIM_LOCKSTEP_CLASS::~ASIM_LOCKSTEP_CLASS()
{
    munmap(dataOut, sizeof(LOCKSTEP_COMM_BUFFER_CLASS));
    munmap(dataIn, sizeof(LOCKSTEP_COMM_BUFFER_CLASS));
}
