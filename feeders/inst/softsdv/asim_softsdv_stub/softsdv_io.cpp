/**************************************************************************
 * Copyright (C) 2003-2006 Intel Corporation
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
 * @file
 * @author Michael Adler
 * @brief SoftSDV shared memory I/O code for data transfer with SoftSDV
 */

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// This file is bound separately into both Asim and SoftSDV.  In order
// to avoid sucking too many Asim libraries into SoftSDV the list of
// ASIM core include files must be kept small.  Some of the common Asim
// routines (e.g. ASSERT) are replaced by macros inside this module.
//
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/stat.h>

//
// ASIM core
//
#include "asim/syntax.h"

#ifndef SOFTSDV_STUB

#include "asim/mesg.h"

#else

//
// Private ASSERT and friends to avoid using the Asim code in SoftSDV
//

#define ASSERT(condition,mesg) \
    if (! (condition)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, mesg); \
        exit(1); \
    }

#define ASSERTX(condition) \
    if (! (condition)) { \
        fprintf(stderr, "%s:%d: Assertion failure.\n", __FILE__, __LINE__); \
        exit(1); \
    }

#define VERIFY(condition,mesg) ASSERT(condition,mesg)

#define ASIMERROR(mesg) \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, mesg); \
    exit(1);

#endif // ! SOFTSDV_STUB


//
// ASIM include files
//
#include "asim/provides/softsdv_stub.h"


//-------------------------------------------------------------------------
//
// Code common to both ASIM and SoftSDV sides
//
//-------------------------------------------------------------------------

//
// Simple class keeps a list of all active Asim and SoftSDV communications
// classes.  This makes it possible to notify all active I/O classes that
// current process is exiting.  These classes may then try to notify or
// kill the remote process.
//


//
// This static instance guarantees the destructor is called when the
// process exits.  The destructor will call the Exiting() method on
// all active I/O classes.
//
static ASIM_SOFTSDV_IO_LIST_CLASS base_list_class;


ASIM_SOFTSDV_IO_LIST_CLASS::ASIM_SOFTSDV_IO_LIST_CLASS(void)
    : asimIo(NULL),
      softsdvIo(NULL)
{
    SetSide(NEITHER);

    //
    // Verify that this is the only NEITHER instance.  This could be a quicker
    // search but it can happen at most once during the whole program and the
    // list is short.
    //
    ASIM_SOFTSDV_IO_LIST t = head;
    while (t != NULL)
    {
        ASSERT(t->side != NEITHER, "Only one NEITHER class allowed.");
        t = t->next;
    }

    next = head;
    head = this;
}


ASIM_SOFTSDV_IO_LIST_CLASS::ASIM_SOFTSDV_IO_LIST_CLASS(
    SOFTSDV_IO_ASIM_SIDE asimIo) : asimIo(asimIo),
                                   softsdvIo(NULL)
{
    SetSide(ASIM_SIDE);
    next = head;
    head = this;
}


ASIM_SOFTSDV_IO_LIST_CLASS::ASIM_SOFTSDV_IO_LIST_CLASS(
    SOFTSDV_IO_SOFTSDV_SIDE softsdvIo) : asimIo(NULL),
                                         softsdvIo(softsdvIo)
{
    SetSide(SOFTSDV_SIDE);
    next = head;
    head = this;
}


ASIM_SOFTSDV_IO_LIST_CLASS::~ASIM_SOFTSDV_IO_LIST_CLASS()
{
    ASSERTX(head != NULL);

    if ((asimIo == NULL) && (softsdvIo == NULL))
    {
        //
        // This is the base_list_class static instance.  If its destructor
        // was called the program must be exiting.  Make sure any active
        // I/O streams know we are going down.
        //
        Exiting();
    }

    //
    // Remove this instance from the static linked list.  There is no prev
    // pointer since this will happen infrequently and the lists are short.
    //
    if (head == this)
    {
        head = next;
    }
    else
    {
        ASIM_SOFTSDV_IO_LIST t = head;
        while (t->next != this)
        {
            t = t->next;
            ASSERTX(t != NULL);
        }
        t->next = next;
    }
}


void
ASIM_SOFTSDV_IO_LIST_CLASS::SetSide(
    WHICH_SIDE newSide)
{
    //
    // Make sure all I/O classes in one process are the same kind.
    //
    if (newSide != NEITHER)
    {
        ASSERT((side == NEITHER) || (side == newSide),
               "Can't switch sides.");
        side = newSide;
    }
};


void
ASIM_SOFTSDV_IO_LIST_CLASS::Exiting(void)
{
    //
    // Go through all possible Exiting() functions associated with
    // all known instances of I/O classes.
    //
    ASIM_SOFTSDV_IO_LIST t = head;
    while (t != NULL)
    {
        if (t->asimIo)
        {
            t->asimIo->Exiting();
        }
        else if (t->softsdvIo)
        {
            t->softsdvIo->Exiting();
        }

        t = t->next;
    }
};


ASIM_SOFTSDV_IO_LIST_CLASS::WHICH_SIDE ASIM_SOFTSDV_IO_LIST_CLASS::side =
    ASIM_SOFTSDV_IO_LIST_CLASS::NEITHER;

ASIM_SOFTSDV_IO_LIST ASIM_SOFTSDV_IO_LIST_CLASS::head = NULL;


//-------------------------------------------------------------------------
//
// Shared memory layout
//
//-------------------------------------------------------------------------

class ASIM_SOFTSDV_SHARED_MEM_LAYOUT_CLASS
{
  public:
    ASIM_SOFTSDV_SHARED_MEM_LAYOUT_CLASS(UINT32 nCpus);
    ~ASIM_SOFTSDV_SHARED_MEM_LAYOUT_CLASS() {};

    UINT64 GetAsimSize(void) const { return asimSize; };
    UINT64 GetAsimMemOffset(void) const { return asimMemOffset; };
    UINT64 GetAsimInstrRingOffset(void) const { return asimInstrRingOffset; };

    UINT64 GetSoftSDVSize(void) const { return softsdvSize; };
    UINT64 GetSoftSDVMemOffset(void) const { return softsdvMemOffset; };
    UINT64 GetSoftSDVInstrRingOffset(void) const { return softsdvInstrRingOffset; };

    UINT64 GetTotalSize(void) const { return GetAsimSize() + GetSoftSDVSize(); };

  private:
    // Asim side
    UINT64 asimSize;
    UINT64 asimMemOffset;
    UINT64 asimInstrRingOffset;

    // SoftSDV side
    UINT64 softsdvSize;
    UINT64 softsdvMemOffset;
    UINT64 softsdvInstrRingOffset;
};

typedef ASIM_SOFTSDV_SHARED_MEM_LAYOUT_CLASS *ASIM_SOFTSDV_SHARED_MEM_LAYOUT;

ASIM_SOFTSDV_SHARED_MEM_LAYOUT_CLASS::ASIM_SOFTSDV_SHARED_MEM_LAYOUT_CLASS(
    UINT32 nCpus)
{
    asimMemOffset = sizeof(*this);
    asimInstrRingOffset = asimMemOffset + sizeof(SOFTSDV_IO_ASIM_SIDE_MEMORY_CLASS);
    asimSize =
        asimInstrRingOffset + nCpus * sizeof(ASIM_SIDE_INSTR_RING_READER_CLASS);

    softsdvMemOffset = 0;
    softsdvInstrRingOffset = softsdvMemOffset + sizeof(SOFTSDV_IO_SOFTSDV_SIDE_MEMORY_CLASS);
    softsdvSize =
        softsdvInstrRingOffset + nCpus * sizeof(SOFTSDV_SIDE_INSTR_RING_WRITER_CLASS);
};


//-------------------------------------------------------------------------
//
// ASIM side
//
//-------------------------------------------------------------------------

extern "C"
{
void SoftSDVProcessDied(
    int sig)
{
    int s;
    while (waitpid(-1, &s, WNOHANG) > 0) ;
}
}


SOFTSDV_IO_ASIM_SIDE_CLASS::SOFTSDV_IO_ASIM_SIDE_CLASS(
    UINT32 argc,
    char **argv,
    char **envp,
    bool recordRegisterValues,
    bool recordMemoryValues,
    bool monitorDMATraffic) : ASIM_SOFTSDV_IO_LIST_CLASS(this)
{
    int fdOut;
    int fdIn;
    pid_t pid;

    //
    // First some sanity checks on user configurable knobs.
    //
    ASSERT((N_SOFTSDV_REG_RING_ENTRIES & (N_SOFTSDV_REG_RING_ENTRIES - 1)) == 0,
           "N_SOFTSDV_REG_RING_ENTRIES in softsdv_stub configuration must be a power of 2!");
    ASSERT((N_SOFTSDV_INSTR_RING_ENTRIES & (N_SOFTSDV_INSTR_RING_ENTRIES - 1)) == 0,
           "N_SOFTSDV_INSTR_RING_ENTRIES in softsdv_stub configuration must be a power of 2!");
    ASSERT(SOFTSDV_WARMUP_CHUNK_SIZE <= N_SOFTSDV_INSTR_RING_ENTRIES/2,
           "SOFTSDV_WARMUP_CHUNK_SIZE must be less than N_SOFTSDV_INSTR_RING_ENTRIES in softsdv_stub configuration");
    ASSERT(SOFTSDV_EXECUTE_CHUNK_SIZE <= N_SOFTSDV_INSTR_RING_ENTRIES/2,
           "SOFTSDV_EXECUTE_CHUNK_SIZE must be less than N_SOFTSDV_INSTR_RING_ENTRIES in softsdv_stub configuration");

    //
    // Parse options
    //
    UINT32 nCpus = 1;
    bool osMode = false;

    for (unsigned int argId = 0; argId < argc; argId++)
    {
        if (! strcmp(argv[argId], "-os"))
        {
            osMode = true;
        }
        else
        {
            char *numCpusArgPtr = strstr(argv[argId], "-num_cpus=");
            if (NULL != numCpusArgPtr) // Assuming all lower case
            {
                char *numCpusToken = numCpusArgPtr + strlen("-num_cpus=");
                nCpus = atoi(numCpusToken);
            }
        }
    }

    //
    // Allocate the buffers for I/O in each direction.
    //
    ASIM_SOFTSDV_SHARED_MEM_LAYOUT_CLASS memLayout(nCpus);

    if (memLayout.GetTotalSize() > 1024 * 1024)
    {
        fprintf(stderr, "Allocating %llu MB shared buffer for SoftSDV\n",
                memLayout.GetTotalSize() / (1024 * 1024));
    }

    // Asim side
    ASIM_SOFTSDV_SHARED_MEM_LAYOUT asimBuf;
    asimBuf = (ASIM_SOFTSDV_SHARED_MEM_LAYOUT)
        AllocateSharedMemory(memLayout.GetAsimSize(),
                             PROT_READ | PROT_WRITE,
                             &fdOut);
    *asimBuf = memLayout;
    asimMem = (SOFTSDV_IO_ASIM_SIDE_MEMORY)
              ((PTR_SIZED_UINT)asimBuf +
               (PTR_SIZED_UINT)memLayout.GetAsimMemOffset());

    ASIM_SIDE_INSTR_RING_READER_CLASS* asimInstrRing;
    asimInstrRing = (ASIM_SIDE_INSTR_RING_READER_CLASS*)
                    ((PTR_SIZED_UINT)asimBuf +
                     (PTR_SIZED_UINT)memLayout.GetAsimInstrRingOffset());

    // SoftSDV side
    softsdvMem = (SOFTSDV_IO_SOFTSDV_SIDE_MEMORY)
        AllocateSharedMemory(memLayout.GetSoftSDVSize(),
                             PROT_READ,
                             &fdIn);

    SOFTSDV_SIDE_INSTR_RING_WRITER_CLASS* softsdvInstrRing;
    softsdvInstrRing = (SOFTSDV_SIDE_INSTR_RING_WRITER_CLASS*)
                       ((PTR_SIZED_UINT)softsdvMem +
                        (PTR_SIZED_UINT)memLayout.GetSoftSDVInstrRingOffset());

    //
    // Declare SIGCHLD handler to detect end of SoftSDV process and avoid
    // orphans.
    //
    struct sigaction sa;
    sa.sa_handler = SoftSDVProcessDied;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);

    asimMem->SetAsimPid(getpid());
    asimMem->SetIsActive();

    if (recordRegisterValues)
    {
        asimMem->SetRecordRegisterValues();
    }
    if (recordMemoryValues)
    {
        asimMem->SetRecordMemoryValues();
    }

    asimMem->SetNCpus(nCpus);
    if (osMode)
    {
        asimMem->SetOSMode();
    }

    if (monitorDMATraffic)
    {
        asimMem->SetMonitorDMA();
    }

    if (! asimMem->OSMode())
    {
        ASSERT(asimMem->NCpus() == 1, "Non-OS mode supports only 1 CPU");
        asimMem->SetNCpus(1);
    }
    else
    {
        ASSERT(asimMem->NCpus(), "Couldn't compute number of SoftSDV CPUs");
    }

    //
    // Create two pipes for synchronization -- one for each direction.  No
    // data will flow across the pipes.  They will be used for waking up
    // the remote side in case it completed spinning and started waiting.
    // Pipes are portable, efficiently implemented in most versions of Unix
    // and more efficient than using sleep() combined with signals.  The
    // sleep() / signal method has higher kernel overhead since the signal
    // must actually be delivered to a handler in the remote process in order
    // to end a sleep() call early.
    //
    int pipeAsimToSoftsdv[2];
    int pipeSoftsdvToAsim[2];
    ASSERTX(pipe(pipeAsimToSoftsdv) != -1);
    ASSERTX(pipe(pipeSoftsdvToAsim) != -1);

    switch (pid = fork())
    {
      case -1:
        ASIMERROR("fork() failed.");
        break;

      case 0:
        {
            //
            // Child
            //

            //
            // Put Asim->SoftSDV shared memory buffer at file descriptor 25.
            //
            ASSERT(dup2(fdOut, 25) != -1, "dup2() failed");
            close(fdOut);

            //
            // Put SoftSDV->Asim shared memory buffer at file descriptor 26.
            //
            ASSERT(dup2(fdIn, 26) != -1, "dup2() failed");
            close(fdIn);

            //
            // Put Asim->SoftSDV control pipe on 27.
            //
            ASSERT(dup2(pipeAsimToSoftsdv[0], 27) != -1, "dup2() failed");
            close(pipeAsimToSoftsdv[0]);
            close(pipeAsimToSoftsdv[1]);

            //
            // Put SoftSDV->Asim control pipe on 28.
            //
            ASSERT(dup2(pipeSoftsdvToAsim[1], 28) != -1, "dup2() failed");
            close(pipeSoftsdvToAsim[0]);
            close(pipeSoftsdvToAsim[1]);

            //
            // Set an environment variable so SoftSDV side knows it is really
            // connected to Asim.
            //
            ASSERTX(! setenv("ASIM_SOFTSDV_CONNECTED", "t", 0));

            //
            // Ready to exec the feeder process...
            //
            char **localArgv = (char **)malloc(sizeof(char *) * (argc + 2));
            ASSERTX(localArgv != NULL);

            //
            // SoftSDV runs in the softsdv subdirectory
            //
            ASSERT(chdir("softsdv") == 0,
                   "Failed to chdir to softsdv subdirectory");

            int aOffset;
            if ((argc > 0) && (*argv[0] != '-'))
            {
                //
                // The run script specified the image we should run
                //
                localArgv[0] = argv[0];
                aOffset = 0;
            }
            else
            {
                localArgv[0] = "ssdv";
                aOffset = 1;
            }
            
            printf("Starting SoftSDV:  %s", localArgv[0]);
            for (UINT32 i = 1 - aOffset; i < argc; i++)
            {
                localArgv[i+aOffset] = argv[i];
                printf(" %s", argv[i]);
            }
            localArgv[argc] = NULL;
            printf("\n");

            execvp(localArgv[0], localArgv);

            fprintf(stderr, "errno = %d\n", errno);
            ASIMERROR("execvp() failed");
        }

      default:
        //
        // Parent
        //
        asimMem->SetSoftsdvPid(pid);
    }

    close(fdOut);
    close(fdIn);

    close(pipeAsimToSoftsdv[0]);
    close(pipeSoftsdvToAsim[1]);

    pipeIn = pipeSoftsdvToAsim[0];
    pipeOut = pipeAsimToSoftsdv[1];

    //
    // Make the write side non-blocking.  There is not a 1:1 correspondence
    // between writes and reads in the control pipe.  Messages are usually
    // written for groups of objects written to the ring.  Messages are
    // read only if the reader finished spinning and started waiting on
    // kernel logic to wake it up.  If the buffer is full we probably don't
    // need another synchronization message, so setting to non-blocking
    // will just drop new writes.  In the rare cases that the write was
    // actually needed, the reader side will recover on its own within
    // one second.  (Obviously you shouldn't let that happen often!)
    //
    // Minizing the number of writes/reads on the syncronization pipe saves
    // significant CPU usage.
    //
    ASSERTX(fcntl(pipeOut, F_SETFL, fcntl(pipeOut, F_GETFL, 0) | O_NONBLOCK) != -1);

    int trips = 0;
    while (! softsdvMem->IsActive())
    {
        trips += 1;
        sleep(1);

        if (trips > 60)
        {
            //
            // Over a minute elapsed and SoftSDV doesn't seem to have started.
            //
            fprintf(stderr, "SoftSDV process (%d) doesn't seem to be starting.\n",
                    asimMem->SoftsdvPid());
            Exiting();
            exit(1);
        }
    }

    //
    // Initialize Asim side of the instruction ring.
    //
    SOFTSDV_IO_SPIN_CONTROL spin =
        new SOFTSDV_IO_SPIN_CONTROL_CLASS(ASIM_SOFTSDV_IO_LIST_CLASS::ASIM_SIDE,
                                          asimMem,
                                          softsdvMem,
                                          pipeIn);

    UINT32 activeCpus = asimMem->NCpus();
    instrRing.Init(activeCpus);
    for (UINT32 cpu = 0; cpu < activeCpus; cpu++)
    {
        instrRing[cpu] = new SOFTSDV_IO_RING_READER_CLASS<
                                 N_SOFTSDV_INSTR_RING_ENTRIES,
                                 ASIM_SOFTSDV_INST_INFO_CLASS>(
                                     &asimInstrRing[cpu],
                                     &softsdvInstrRing[cpu],
                                     spin);
    }

    regValueRing =
        new SOFTSDV_IO_RING_READER_CLASS<
                N_SOFTSDV_REG_RING_ENTRIES,
                ASIM_SOFTSDV_REG_INFO_CLASS>(
                    &asimMem->regValueRing,
                    &softsdvMem->regValueRing,
                    spin);

    asimRequestQueue =
        new SOFTSDV_IO_RING_WRITER_CLASS<
                ASIM_SOFTSDV_REQUEST_CLASS::MAX_SDV_BUFFERED_REQUESTS,
                ASIM_SOFTSDV_REQUEST_CLASS>(
                    &asimMem->asimRequestQueue,
                    &softsdvMem->asimRequestQueue,
                    spin);
}


SOFTSDV_IO_ASIM_SIDE_CLASS::~SOFTSDV_IO_ASIM_SIDE_CLASS()
{
}

void
SOFTSDV_IO_ASIM_SIDE_CLASS::Exiting(void)
{
    if (asimMem->IsActive())
    {
        asimMem->ClearIsActive();
        //
        // Is SoftSDV still running?
        //
        if (asimMem->SoftsdvPid() != 0)
        {
            //
            // Wait for SoftSDV to claim it is inactive.  After 10 seconds just
            // kill it if it won't go away on its own.
            //
            UINT32 trips = 0;
            while (softsdvMem->IsActive() && (trips < 10))
            {
                sleep(1);
                trips += 1;
            }

            if (softsdvMem->IsActive())
            {
                fprintf(stderr, "Done.  Stopping SoftSDV process (%d).\n",
                        asimMem->SoftsdvPid());
                kill(asimMem->SoftsdvPid(), SIGTERM);
            }

            asimMem->SetSoftsdvPid(0);
        }
    }
}


void *
SOFTSDV_IO_ASIM_SIDE_CLASS::AllocateSharedMemory(
    UINT32 size,
    int prot,
    int *fd)
{
    char fname[128];
    int zero = 0;
    int i;
    void *buf;

    strcpy(fname, "/tmp/asim_softsdv.XXXXXX");
    *fd = mkstemp(fname);
    ASSERT(*fd != -1, "mkstemp() failed");
    unlink(fname);

    // Make the file the right size
    off_t pos = size - 1;
    ASSERTX(lseek(*fd, pos, SEEK_SET) == pos);
    ASSERTX(write(*fd, &zero, 1) != -1);

    buf = mmap(0, size, prot, MAP_SHARED, *fd, 0);
    ASSERT((long)buf != -1, "mmap failed");
    fcntl(*fd, F_SETFD, 0);

    return buf;
}



//-------------------------------------------------------------------------
//
// SoftSDV side
//
//-------------------------------------------------------------------------

SOFTSDV_IO_SOFTSDV_SIDE_CLASS::SOFTSDV_IO_SOFTSDV_SIDE_CLASS(void)
    : ASIM_SOFTSDV_IO_LIST_CLASS(this)
{
    nCpus = 0;
    osMode = false;
    recordRegisterValues = false;
    recordMemoryValues = false;
    monitorDMA = false;

    //
    // Synchronization pipes come in on 27 and 28.
    //
    pipeIn = 27;
    pipeOut = 28;

    //
    // Shared memory buffers come in on file descriptors 25 and 26.  Map
    // the files to memory and close the descriptors.  (Map stays connected
    // following close().)
    //
    // If ASIM_SOFTSDV_CONNECTED environment variable is not defined it
    // is definitely not connected to Asim.
    //
    ASIM_SOFTSDV_SHARED_MEM_LAYOUT asimBuf;
    if (! getenv("ASIM_SOFTSDV_CONNECTED"))
    {
        asimBuf = (ASIM_SOFTSDV_SHARED_MEM_LAYOUT)-1;
    }
    else
    {
        //
        // At this point we don't know how much to map.  Start by mapping
        // just enough to read the layout descriptor.
        //
        asimBuf = (ASIM_SOFTSDV_SHARED_MEM_LAYOUT)
                  mmap(0, sizeof(*asimBuf), PROT_READ, MAP_SHARED, 25, 0);
        if ((PTR_SIZED_INT)asimBuf != -1)
        {
            // Now remap the entire buffer
            asimBuf = (ASIM_SOFTSDV_SHARED_MEM_LAYOUT)
                      mremap(asimBuf,
                             sizeof(*asimBuf), asimBuf->GetAsimSize(),
                             PROT_READ);
        }
    }

    ASIM_SIDE_INSTR_RING_READER_CLASS* asimInstrRing;
    SOFTSDV_SIDE_INSTR_RING_WRITER_CLASS* softsdvInstrRing;
    UINT32 activeCpus;

    if ((PTR_SIZED_INT)asimBuf == -1)
    {
        asimMem = NULL;
        asimInstrRing = NULL;

        //
        // The actual number of CPUs used in SoftSDV isn't available yet from
        // SoftSDV.  We aren't connected to Asim so just pick a large number.
        // The array access function will fail for out of bounds references.
        //
        activeCpus = 128;

        //
        // Allocate a fake softsdvMem area so writes to the output buffers
        // will appear to work.  This allows for debugging the SoftSDV side
        // without connecting to Asim.
        //
        ASIM_SOFTSDV_SHARED_MEM_LAYOUT_CLASS memLayout(activeCpus);
        softsdvMem = (SOFTSDV_IO_SOFTSDV_SIDE_MEMORY)
                     malloc(memLayout.GetSoftSDVSize());
        ASSERTX(softsdvMem != NULL);
        bzero(softsdvMem, sizeof(SOFTSDV_IO_SOFTSDV_SIDE_MEMORY_CLASS));

        softsdvInstrRing = (SOFTSDV_SIDE_INSTR_RING_WRITER_CLASS*)
                           ((PTR_SIZED_UINT)softsdvMem +
                            (PTR_SIZED_UINT)memLayout.GetSoftSDVInstrRingOffset());

        fprintf(stderr, "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        fprintf(stderr, "Warning:  Unable to connect shared memory buffer\n");
        fprintf(stderr, "          to Asim.  Running stand-alone.\n");
        fprintf(stderr, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n");

        pipeIn = -1;
        pipeOut = -1;
    }
    else
    {
        //
        // Asim already allocated the storage.  Compute pointers to the
        // Asim side.
        //
        asimMem = (SOFTSDV_IO_ASIM_SIDE_MEMORY)
                  ((PTR_SIZED_UINT)asimBuf +
                   (PTR_SIZED_UINT)asimBuf->GetAsimMemOffset());

        asimInstrRing = (ASIM_SIDE_INSTR_RING_READER_CLASS*)
                        ((PTR_SIZED_UINT)asimBuf +
                         (PTR_SIZED_UINT)asimBuf->GetAsimInstrRingOffset());

        activeCpus = asimMem->NCpus();

        //
        // Map the SoftSDV side
        //
        softsdvMem = (SOFTSDV_IO_SOFTSDV_SIDE_MEMORY)
                     mmap(0, asimBuf->GetSoftSDVSize(),
                          PROT_READ | PROT_WRITE, MAP_SHARED, 26, 0);
        VERIFY((PTR_SIZED_INT)softsdvMem != -1, "mmap failed");

        softsdvInstrRing = (SOFTSDV_SIDE_INSTR_RING_WRITER_CLASS*)
                           ((PTR_SIZED_UINT)softsdvMem +
                            (PTR_SIZED_UINT)asimBuf->GetSoftSDVInstrRingOffset());

        //
        // Recording register values?  Copy the value locally.  It can't
        // change during the run.
        //
        recordRegisterValues = asimMem->RecordRegisterValues();
        if (recordRegisterValues)
        {
            printf("ASIM: Recording register values\n");
        }
        recordMemoryValues = asimMem->RecordMemoryValues();
        if (recordMemoryValues)
        {
            printf("ASIM: Recording memory values\n");
        }

        close(25);
        close(26);
    }

    softsdvMem->SetIsActive();

    //
    // Initialize SoftSDV side of the instruction ring.
    //
    spin = new SOFTSDV_IO_SPIN_CONTROL_CLASS(ASIM_SOFTSDV_IO_LIST_CLASS::SOFTSDV_SIDE,
                                             asimMem,
                                             softsdvMem,
                                             pipeIn);

    instrRing.Init(activeCpus);

    for (UINT32 cpu = 0; cpu < activeCpus; cpu++)
    {
        instrRing[cpu] = new SOFTSDV_IO_RING_WRITER_CLASS<
                                 N_SOFTSDV_INSTR_RING_ENTRIES,
                                 ASIM_SOFTSDV_INST_INFO_CLASS>(
                                     &softsdvInstrRing[cpu],
                                     asimInstrRing != NULL ? &asimInstrRing[cpu] : NULL,
                                     spin);
    }

    regValueRing =
        new SOFTSDV_IO_RING_WRITER_CLASS<
                N_SOFTSDV_REG_RING_ENTRIES,
                ASIM_SOFTSDV_REG_INFO_CLASS>(
                    &softsdvMem->regValueRing,
                    asimMem != NULL ? &asimMem->regValueRing : NULL,
                    spin);

    asimRequestQueue =
        new SOFTSDV_IO_RING_READER_CLASS<
                ASIM_SOFTSDV_REQUEST_CLASS::MAX_SDV_BUFFERED_REQUESTS,
                ASIM_SOFTSDV_REQUEST_CLASS>(
                    &softsdvMem->asimRequestQueue,
                    asimMem != NULL ? &asimMem->asimRequestQueue : NULL,
                    spin);

    //
    // Make the pipe writer non-blocking.  See the reason in the constructor
    // for SOFTSDV_IO_ASIM_SIDE_CLASS.
    //
    if (pipeOut != -1)
    {
        ASSERTX(fcntl(pipeOut, F_SETFL, fcntl(pipeOut, F_GETFL, 0) | O_NONBLOCK) != -1);
    }
}


SOFTSDV_IO_SOFTSDV_SIDE_CLASS::~SOFTSDV_IO_SOFTSDV_SIDE_CLASS()
{
}

void
SOFTSDV_IO_SOFTSDV_SIDE_CLASS::Exiting(bool normalExit)
{
    if (softsdvMem)
    {
        softsdvMem->SetNormalExit();
        softsdvMem->ClearIsActive();
    }
}

//-------------------------------------------------------------------------
//
// Spin control
//
//-------------------------------------------------------------------------

bool
SOFTSDV_IO_SPIN_CONTROL_CLASS::Blocked(
    UINT32 trip,
    time_t *timeTag)
{
    //
    // Is the remote process still alive?
    //
    UINT32 isActive = false;
    pid_t pid = 0;

    if (side == ASIM_SOFTSDV_IO_LIST_CLASS::SOFTSDV_SIDE)
    {
        pid = asimMem->AsimPid();
        isActive = asimMem->IsActive();
    }
    else
    {
        pid = asimMem->SoftsdvPid();
        isActive = softsdvMem->IsActive();
    }

    if (! isActive)
    {
        return false;
    }

    if (trip < 0x10)
    {
        //
        // Spin for a little while.  The pipe synchronization is reasonably
        // efficient, so don't spin for too long.
        //
        return true;
    }

    //
    // Initialize timeTag if it is still 0.
    //
    if (*timeTag == 0)
    {
        *timeTag = time(NULL);
    }

    //
    // Every 8 seconds make sure the remote process actually exists.
    //
    if ((trip & 0x7) == 0)
    {
        time_t now = time(NULL);
        if (now - *timeTag >= 5)
        {
            *timeTag = now;

            if ((kill(pid, 0) == -1) && (errno == ESRCH))
            {
                fprintf(stderr, "Remote %s process (%d) is missing.  Giving up.\n",
                        side == ASIM_SOFTSDV_IO_LIST_CLASS::SOFTSDV_SIDE ?
                            "Asim" : "SoftSDV",
                        pid);
                exit(1);
            }
        }
    }

    //
    // Is data available on the signalling channel?  If so, continue.  The
    // algorithm isn't perfect.  There is some low probability that the incoming
    // message on the synchronization pipe is consumed before the data is
    // read in shared memory.  The poll will eventually recover and the program
    // will continue.  If this happens too often the interface will be
    // incredibly slow.
    //
    struct pollfd ufds;
    ufds.fd = syncPipeIn;
    ufds.events = POLLIN;
    ufds.revents = 0;
    if ((poll(&ufds, 1, 1000) > 0) && (ufds.revents & POLLIN))
    {
        //
        // Signals on the pipe aren't read if the data is found in shared
        // memory on the first attempt.  Consume a large chunk of the
        // synchronization pipe since it holds old news.
        // 
        static char dummy[1024];
        read(syncPipeIn, dummy, sizeof(dummy));
    }

    return true;
};
