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


#ifndef _MPIPE_C_
#define _MPIPE_C_

#include "mpipe-c.h"

#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static   pid_t      mypid;
static   pid_t     *otherpid;
static   sigset_t   SIGUSR1set;
volatile UINT64    *any_reader_sleeping;
volatile UINT64    *any_writer_sleeping;


static void
WakeupHandler (int signo)
{
    DEBUGF(DOUT, "Handler received SIGUSR1 (%d)\n", mypid);
}

/*
 *
 * Initialize the shared and private data.
 *
 */

UINT64
MPIPE_InitPipe (CHAR *shared_buffer, UINT64 dsize, VOID **vpriv)
{
    BOOL slave;
    sigset_t sigmask;
    struct sigaction      sleep_action;
    struct pipe_buffer   *pbuf;
    struct mpipe_private *priv;

    slave = dsize == 0;
    mypid = getpid();
    pbuf = (struct pipe_buffer *)shared_buffer;

    if(slave && pbuf->pbuffer_size == 0) {
      DEBUGF(DOUT, "Slave buffer size null\n");
      return 0;
    }

    if(!*vpriv) {
      *vpriv = malloc(sizeof(struct mpipe_private));
      if(!*vpriv)
	return 0;
    }
    priv = (struct mpipe_private *)*vpriv;

    priv->data_size = slave ? pbuf->pbuffer_size - sizeof(struct pipe_buffer) :dsize;
    priv->data_buffer = shared_buffer + sizeof(struct pipe_buffer);
    priv->read_ptr = priv->data_buffer;
    priv->write_ptr = priv->data_buffer;
    priv->max_ptr  = priv->data_buffer + priv->data_size;
    priv->watermark = priv->data_size / 4;

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
    any_writer_sleeping = &pbuf->any_writer_sleeping;
    any_reader_sleeping = &pbuf->any_reader_sleeping;
    

    // set the current signal mask to block out SIGUSR1 while running
    sigemptyset(&SIGUSR1set);
    sigaddset(&SIGUSR1set, SIGUSR1);
    sigprocmask(SIG_BLOCK, &SIGUSR1set, &priv->osigmask);
    sigdelset(&priv->osigmask, SIGUSR1);  // Make sure SIGUSR1 will get through

    // Don't want any signals DURING the signal call
    sigfillset(&sigmask); // should not be empty to avoid multiple signals
    sleep_action.sa_handler = WakeupHandler;
    sleep_action.sa_mask = sigmask;
    sleep_action.sa_flags = 0;
    sigaction(SIGUSR1, &sleep_action, 0);
    sigemptyset(&sigmask);

    if(!slave)
      // Make this last indicating master buffer init complete (nonzero)
      pbuf->pbuffer_size = priv->data_size + sizeof(struct pipe_buffer);

    DEBUGF(DOUT, "data_size=%d\n", (UINT32)priv->data_size);
    return pbuf->pbuffer_size;
}

/*
 *
 *  A read and write routines similar to the read() and write() system calls.
 *  Pass it a pointer to a buffer and its size, and it will copy to/from
 *  the data from local memory to shared memory
 *
 */

UINT64
MPIPE_Read (VOID *buffer, UINT64 size, mp_t *priv, pb_t *pbuf)
{
    UINT64 msize = MPIPE_GetMaxReadSize(size, priv, pbuf);
    if(!msize)
        return 0;
    if(msize < size)
        size = msize;
    memcpy(buffer, priv->read_ptr, size);
    MPIPE_IncReadPtr(size, priv, pbuf);
    MPIPE_IncDonePos(size, priv, pbuf);
    return size;
}

UINT64
MPIPE_Write (VOID *buffer, UINT64 size, mp_t *priv, pb_t *pbuf)
{
    UINT64 msize = MPIPE_GetMaxWriteSize(size, priv, pbuf);
    if(!msize)
        return 0;
    if(msize < size)
        size = msize;
    memcpy(priv->write_ptr, buffer, size);
    MPIPE_IncWritePtr(size, priv, pbuf);
    return size;
}


/*
 *
 * The following is code that checks for sleeping conditions and implements
 * the actual sleep and wakeup routines.
 *
 */

VOID
MPIPE_Sleep (mp_t *priv, pb_t *pbuf)
{
  static int actionchecked;

  if(!actionchecked) {
    sigset_t sigmask;
    struct sigaction      sleep_action;

    actionchecked = 1;

    sigfillset(&sigmask); // should not be empty to avoid multiple signals
    sleep_action.sa_handler = WakeupHandler;
    sleep_action.sa_mask = sigmask;
    sleep_action.sa_flags = 0;
    sigaction(SIGUSR1, &sleep_action, 0);
  }

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
    sigsuspend(&priv->osigmask);
#endif

    DEBUGF(DOUT, "...awoke (%d)!\n", (UINT32)mypid);
    FLUSHF(DOUT);
}

VOID
MPIPE_Wakeup (volatile UINT64 *sleeping_pid, mp_t *priv, pb_t *pbuf)
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
MPIPE_SleepReadCheck (UINT64 min, mp_t *priv, pb_t *pbuf)
{
    UINT64 size = pbuf->written - pbuf->read;
    UINT64 wakeup;

    if(min > priv->data_size)  min = priv->data_size;  // Can't go beyond buffer size!
    if(size >= min)
      return size;

    DEBUGF(DOUT, "Want to READ %d while %d avail\n", (UINT32)min, (UINT32)size);
    if(min < priv->watermark)  min = priv->watermark;  // Sleep is exp, so get more
    if(min > (priv->max_ptr - priv->read_ptr))
      min = priv->max_ptr - priv->read_ptr;
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
	MPIPE_Wakeup(any_writer_sleeping, priv, pbuf);
      }
      MPIPE_Sleep(priv, pbuf);
      pbuf->reader_sleeping = 0;
      pbuf->reader_wakeup = UINT64_MAX;
    }
    
    // Recalculate size one more time
    size = pbuf->written - pbuf->read;
    if(priv->read_ptr + size > priv->max_ptr)
      size = priv->max_ptr - priv->read_ptr;
    // Size may still be zero, but return anyways since it awoke
    return size;
}

UINT64
MPIPE_SleepWriteCheck (UINT64 min, mp_t *priv, pb_t *pbuf)
{
    UINT64 size = priv->data_size - (pbuf->written - pbuf->done);
    UINT64 wakeup;

    if(min > priv->data_size)  min = priv->data_size;  // Can't go beyond buffer size!
    if(size >= min)
      return size;

    // Sleep until reader awakens me
    DEBUGF(DOUT, "Want to WRITE %d while %d avail\n", (UINT32)min, (UINT32)size);
    if(min < priv->watermark)  min = priv->watermark;  // Sleep is exp, so get more
    wakeup = pbuf->written - priv->data_size + min;
    
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
	MPIPE_Wakeup(any_reader_sleeping, priv, pbuf);
      }
      MPIPE_Sleep(priv, pbuf);
      pbuf->writer_sleeping = 0;
      pbuf->writer_wakeup = UINT64_MAX;
    }
    
    // Recalculate size one more time
    size = priv->data_size - (pbuf->written - pbuf->done);
    if(priv->write_ptr + size > priv->max_ptr)
      size = priv->max_ptr - priv->write_ptr;
    // Size may still be zero, but return anyways since it awoke
    return size;
}

#endif /* _MPIPE_C_ */
