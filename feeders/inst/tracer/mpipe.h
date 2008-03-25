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


#ifndef _MPIPE_H_
#define _MPIPE_H_

#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define DOUT stdout
#define SLEEP
#include "mpipe-syntax.h"

/* Can't figure out how to assign private PWakeupHandler to sa_handler,
   so this will have to do */
static pid_t   thispid;
static void
WakeupHandler (int signo)
{
    DEBUGF(DOUT, "Handler received SIGUSR1 (%d)\n", thispid);
}

typedef class pipe_buffer_class {

struct pipe_buffer {
    volatile UINT64      pbuffer_size;
    volatile UINT64      pbuffer_verntype;
    volatile UINT64      read;
    volatile UINT64      done;
    volatile UINT64      reader_wakeup;
    volatile UINT64      reader_sleeping;
    volatile UINT64      reader_signaled;
    volatile UINT64      any_reader_sleeping;
    volatile UINT64      written;
    volatile UINT64      writer_wakeup;
    volatile UINT64      writer_sleeping;
    volatile UINT64      writer_signaled;
    volatile UINT64      any_writer_sleeping;
    volatile INT64       eof;
    volatile UINT64      pid_master;
    volatile UINT64      pid_slave;
};

private:
    struct pipe_buffer   *pbuf;
    CHAR                 *data_buffer;
    UINT64               data_size;

    CHAR                 *max_ptr;
    CHAR                 *read_ptr, *write_ptr;
    UINT64               watermark;
    sigset_t             osigmask;

    static pid_t         mypid;
    static pid_t         *otherpid;   /* use ptr since slave pid not ready */
    static sigset_t      SIGUSR1set;
    static volatile UINT64 *any_reader_sleeping;
    static volatile UINT64 *any_writer_sleeping;

    VOID   Sleep ();
    VOID   Wakeup (volatile UINT64 *);
    UINT64 SleepReadCheck (UINT64);
    UINT64 SleepWriteCheck (UINT64);

public:
    UINT64 InitPipe (CHAR *, UINT64);
    UINT64 GetPipeOverhead () CONST  { return sizeof(struct pipe_buffer); }
    UINT64 GetReadPos ()      CONST  { return pbuf->read; }
    UINT64 GetWritePos ()     CONST  { return pbuf->written; }
    UINT64 GetDonePos ()      CONST  { return pbuf->done; }
    VOID   *GetReadPtr ()     CONST  { return read_ptr; }
    VOID   *GetWritePtr ()    CONST  { return write_ptr; }

    VOID   IncDonePos (UINT64 size);
    VOID   SetDonePos (UINT64 pos);
    VOID   *IncReadPtr (UINT64 size);
    VOID   *DecReadPtr (UINT64 size);
    VOID   *SetReadPos (UINT64 pos);
    VOID   *IncWritePtr (UINT64 size);
    UINT64 GetMaxReadSize (UINT64);
    UINT64 GetMaxWriteSize (UINT64);

    UINT64 Write (VOID *buffer, UINT64 size);
    UINT64 Read (VOID *buffer, UINT64 size);

    VOID   SetEOF() { pbuf->eof=pbuf->written;  Wakeup(any_reader_sleeping); }
    UINT64 GetEOF() CONST { return pbuf->eof; }
    BOOL   IsEOF()  CONST { return (pbuf->read >= pbuf->eof); }

} PIPE_BUFFER_CLASS, *PIPE_BUFFER;

pid_t            PIPE_BUFFER_CLASS::mypid;
pid_t           *PIPE_BUFFER_CLASS::otherpid;
sigset_t         PIPE_BUFFER_CLASS::SIGUSR1set;
volatile UINT64 *PIPE_BUFFER_CLASS::any_reader_sleeping;
volatile UINT64 *PIPE_BUFFER_CLASS::any_writer_sleeping;

/*
 *
 * Initialize the shared and private data.
 *
 */

UINT64
PIPE_BUFFER_CLASS::InitPipe (CHAR *shared_buffer, UINT64 dsize)
{
    BOOL slave;
    sigset_t sigmask;
    struct sigaction     sleep_action;

    slave = dsize == 0;

    mypid = getpid();
    pbuf = (struct pipe_buffer *)shared_buffer;

    if(slave && pbuf->pbuffer_size == 0) {
      DEBUGF(DOUT, "Slave buffer size null\n");
      return 0;
    }

    data_size = slave ? pbuf->pbuffer_size - sizeof(struct pipe_buffer) :dsize;
    data_buffer = shared_buffer + sizeof(struct pipe_buffer);
    read_ptr = data_buffer;
    write_ptr = data_buffer;
    max_ptr  = data_buffer + data_size;
    watermark = data_size / 4;

    if(!slave) {
	pbuf->pbuffer_verntype = 1;
	pbuf->read = pbuf->done = pbuf->written = 0;
	pbuf->eof = -1;
	pbuf->writer_sleeping = 0;
	pbuf->writer_wakeup = UINT64_MAX;
	pbuf->writer_signaled = 0;
	pbuf->any_writer_sleeping = 0;
	pbuf->reader_sleeping = 0;
	pbuf->reader_wakeup = UINT64_MAX;
	pbuf->reader_signaled = 0;
	pbuf->any_reader_sleeping = 0;
	pbuf->pid_master = mypid;
	otherpid = (pid_t *)&pbuf->pid_slave;
    }
    else {
        pbuf->pid_slave = mypid;
	otherpid = (pid_t *)&pbuf->pid_master;
    }
    thispid = mypid;
    any_writer_sleeping = &pbuf->any_writer_sleeping;
    any_reader_sleeping = &pbuf->any_reader_sleeping;
    

    // set the current signal mask to block out SIGUSR1 while running
    sigemptyset(&SIGUSR1set);
    sigaddset(&SIGUSR1set, SIGUSR1);
    sigprocmask(SIG_BLOCK, &SIGUSR1set, &osigmask);
    sigdelset(&osigmask, SIGUSR1);  // Make sure SIGUSR1 will get through

    // Don't want any signals DURING the signal call
    sigfillset(&sigmask); // should not be empty to avoid multiple signals
    sleep_action.sa_handler = WakeupHandler;
    sleep_action.sa_mask = sigmask;
    sleep_action.sa_flags = 0;
    sigaction(SIGUSR1, &sleep_action, 0);
    sigemptyset(&sigmask);

    if(!slave)
      // Make this last indicating master buffer init complete (nonzero)
      pbuf->pbuffer_size = data_size + sizeof(struct pipe_buffer);

    DEBUGF(DOUT, "data_size=%d\n", (UINT32)data_size);
    return pbuf->pbuffer_size;
}

/*
 *
 *  Functions which return pointers to read and write to the shared data,
 *  as well as incrementing the pointers when complete.
 *
 */  

VOID
PIPE_BUFFER_CLASS::IncDonePos (UINT64 size)
{
    pbuf->done += size;
    if(pbuf->done >= pbuf->writer_wakeup) {
        DEBUGF(DOUT, "Detect done="FMT64U" >= writer_wakeup="FMT64U"\n",
	       pbuf->written, pbuf->writer_wakeup);
        pbuf->writer_wakeup = UINT64_MAX;
        Wakeup(any_writer_sleeping);
    }
}

VOID
PIPE_BUFFER_CLASS::SetDonePos (UINT64 pos)
{
    pbuf->done = pos;
    if(*any_writer_sleeping &&
       pbuf->done >= pbuf->written + watermark - data_size) {
//    if(pbuf->done >= pbuf->writer_wakeup) {
        DEBUGF(DOUT, "Detect done="FMT64U" >= wakeup="FMT64U" writer_wakeup="FMT64U"\n",
	       pbuf->written, pbuf->written + watermark - data_size, pbuf->writer_wakeup);
	FLUSHF(DOUT);
        pbuf->writer_wakeup = UINT64_MAX;
        Wakeup(any_writer_sleeping);
    }
}

VOID *
PIPE_BUFFER_CLASS::IncReadPtr (UINT64 size)
{
    pbuf->read += size;
    read_ptr += size;
    if(read_ptr >= max_ptr)
        read_ptr -= data_size;
    return read_ptr;
}

VOID *
PIPE_BUFFER_CLASS::DecReadPtr (UINT64 size)
{
    pbuf->read -= size;
    read_ptr -= size;
    if(read_ptr < data_buffer)
        read_ptr += data_size;
    return read_ptr;
}

VOID *
PIPE_BUFFER_CLASS::SetReadPos (UINT64 pos)
{
    INT64 size = pos - pbuf->read;
    if(size > data_size)
        size = size % data_size; // really should be an error here

    // Adjust read count and read pointer,
    //   being carefull to observe buffer boundaries both low and high
    pbuf->read = pos;
    read_ptr += size;
    if(read_ptr < data_buffer)
        read_ptr += data_size;
    if(read_ptr >= max_ptr)
        read_ptr -= data_size;
    return read_ptr;
}

UINT64
PIPE_BUFFER_CLASS::GetMaxReadSize (UINT64 min)
{
    UINT64 size = pbuf->written - pbuf->read;
    if(read_ptr + size > max_ptr) {
        size = max_ptr - read_ptr;
        return size;
    }
    // Only sleep reader if there is abs nothing to read
    if(size == 0 && !IsEOF())
        return SleepReadCheck(min);
    return size;
}

VOID *
PIPE_BUFFER_CLASS::IncWritePtr (UINT64 size)
{
    pbuf->written += size;
    write_ptr += size;
    if(write_ptr >= max_ptr)
        write_ptr -= data_size;
//    if(*any_reader_sleeping && pbuf->written >= pbuf->read + watermark) {
    if(pbuf->written >= pbuf->reader_wakeup) {
        DEBUGF(DOUT, "Detect written="FMT64U" >= wakeup="FMT64U" reader_wakeup="FMT64U"\n",
	       pbuf->written, pbuf->read + watermark, pbuf->reader_wakeup);
        pbuf->reader_wakeup = UINT64_MAX;
        Wakeup(any_reader_sleeping);
    }
    return write_ptr;
}

UINT64
PIPE_BUFFER_CLASS::GetMaxWriteSize (UINT64 min)
{
    UINT64 size = data_size - (pbuf->written - pbuf->done);
    if(write_ptr + size >= max_ptr) {
        size = max_ptr - write_ptr;
        return size;
    }
    if(size < min)
        return SleepWriteCheck(min);
    return size;
}

/*
 *
 *  A read and write routines similar to the read() and write() system calls.
 *  Pass it a pointer to a buffer and its size, and it will copy to/from
 *  the data from local memory to shared memory
 *
 */

UINT64
PIPE_BUFFER_CLASS::Read (VOID *buffer, UINT64 size)
{
    UINT64 msize = GetMaxReadSize(size);
    if(!msize)
        return 0;
    if(msize < size)
        size = msize;
    memcpy(buffer, read_ptr, size);
    IncReadPtr(size);
    IncDonePos(size);
    return size;
}

UINT64
PIPE_BUFFER_CLASS::Write (VOID *buffer, UINT64 size)
{
    UINT64 msize = GetMaxWriteSize(size);
    if(!msize)
        return 0;
    if(msize < size)
        size = msize;
    memcpy(write_ptr, buffer, size);
    IncWritePtr(size);
    return size;
}

/*
 *
 * The following is code that checks for sleeping conditions and implements
 * the actual sleep and wakeup routines.
 *
 */

VOID
PIPE_BUFFER_CLASS::Sleep ()
{
    int signal;

    // Suspend process until wakened by counterpart
    DEBUGF(DOUT, "Sleeping (%d)...\n", (UINT32)mypid);
    FLUSHF(DOUT);

    // Sigsuspend will now unblock SIGUSR1 and wait for a signal
    // Suspend could continue if a different signal was received and handled
    // by this process, so continue looping until our SIGUSR1 handler
    // was caught.
#ifdef MULTITHREADED
    while(sigwait(&SIGUSR1set, &signal) == EINTR) ;
#else
    sigsuspend(&osigmask);
#endif

    DEBUGF(DOUT, "...awoke (%d)!\n", (UINT32)mypid);
    FLUSHF(DOUT);
}

VOID
PIPE_BUFFER_CLASS::Wakeup (volatile UINT64 *sleeping_pid)
{
    pid_t wakeuppid = *sleeping_pid;

    if(!wakeuppid)
      return;

    // Wakeup other end that is suspended.
    DEBUGF(DOUT, "It's time to wakeup %d!\n", (UINT32)wakeuppid);
    FLUSHF(DOUT);
    if(wakeuppid != *otherpid) {
      DEBUGF(DOUT, "WARNING: sleeping=%d != other=%d\a\n",wakeuppid,*otherpid);
      FLUSHF(DOUT);
      SLEEP(2);
    }

    *sleeping_pid = 0;
    kill(wakeuppid, SIGUSR1);
}

UINT64
PIPE_BUFFER_CLASS::SleepReadCheck (UINT64 min)
{
    UINT64 size = pbuf->written - pbuf->read;
    UINT64 wakeup;

    if(min > data_size)  min = data_size;  // Can't go beyond buffer size!
    if(size >= min)
      return size;

    DEBUGF(DOUT, "Want to READ %d while %d avail\n", (UINT32)min, (UINT32)size);
    if(min < watermark)  min = watermark;  // Sleep is exp, so get more
    if(min > (max_ptr - read_ptr))
      min = max_ptr - read_ptr;
    wakeup = pbuf->read + min;
    
    DEBUGF(DOUT, "rd="FMT64U" done="FMT64U" wr="FMT64U" eof="FMT64D" wakeup="FMT64U"\n", pbuf->read, pbuf->done, pbuf->written, pbuf->eof, wakeup);
    FLUSHF(DOUT);
    
    if(pbuf->written < wakeup) {
      pbuf->reader_sleeping = mypid; 
      pbuf->reader_wakeup = wakeup;
      *any_reader_sleeping = mypid;
      // Check if we need to wake up writer right before we go to sleep
      if(*any_writer_sleeping) {
	DEBUGF(DOUT, "WARNING:  writer %d already sleeping. I am %d rdr\a\n",
	       (UINT32)*any_writer_sleeping, (UINT32)mypid);
	FLUSHF(DOUT);
	
	SLEEP(4);
	pbuf->writer_wakeup = UINT64_MAX;
	Wakeup(any_writer_sleeping);
      }
      Sleep();
      pbuf->reader_sleeping = 0;
      pbuf->reader_wakeup = UINT64_MAX;
    }
    
    // Recalculate size one more time
    size = pbuf->written - pbuf->read;
    if(read_ptr + size > max_ptr)
      size = max_ptr - read_ptr;
    // Size may still be zero, but return anyways since it awoke
    return size;
}

UINT64
PIPE_BUFFER_CLASS::SleepWriteCheck (UINT64 min)
{
    UINT64 size = data_size - (pbuf->written - pbuf->done);
    UINT64 wakeup;

    if(min > data_size)  min = data_size;  // Can't go beyond buffer size!
    if(size >= min)
      return size;

    // Sleep until reader awakens me
    DEBUGF(DOUT, "Want to WRITE %d while %d avail\n", (UINT32)min, (UINT32)size);
    if(min < watermark)  min = watermark;  // Sleep is exp, so get more
    wakeup = pbuf->written - data_size + min;
    
    DEBUGF(DOUT, "rd="FMT64U" done="FMT64U" wr="FMT64U" eof="FMT64D" wakeup="FMT64U"\n", pbuf->read, pbuf->done, pbuf->written, pbuf->eof, wakeup);
    FLUSHF(DOUT);
    
    // possible race condition here with reader
    // i.e., no space left for writer at top of this procedure,
    // reader is context switched in, reads everything, then comes
    // back to writer who sleeps.  If reader is now waiting for
    // writer there is a problem.
    // ... need to prevent both from sleeping at same time.
    if(pbuf->done < wakeup) {
      pbuf->writer_sleeping = mypid;
      pbuf->writer_wakeup = wakeup; // pbuf->done + (min - size);
      *any_writer_sleeping = mypid;
      // Check if we need to wake up reader right before we go to sleep
      // Could both set their wakeup's at same time and other not
      // see before next statement?  need load lock, store cond.
      // without ldl_l, stl_c, this will work on uniprocessors
      if(*any_reader_sleeping) {
	// Who is correct?  reader or writer?
	DEBUGF(DOUT, "WARNING:  reader %d already sleeping. I am %d wrt\a\n",
	       (UINT32)*any_reader_sleeping, mypid);
	FLUSHF(DOUT);
	
	SLEEP(4);
	pbuf->reader_wakeup = UINT64_MAX;
	Wakeup(any_reader_sleeping);
      }
      Sleep();
      pbuf->writer_sleeping = 0;
      pbuf->writer_wakeup = UINT64_MAX;
    }
    
    // Recalculate size one more time
    size = data_size - (pbuf->written - pbuf->done);
    if(write_ptr + size > max_ptr)
      size = max_ptr - write_ptr;
    // Size may still be zero, but return anyways since it awoke
    return size;
}


#endif /* _MPIPE_H_ */
