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


#ifndef _MPIPE_C_H_
#define _MPIPE_C_H_

#include <stdio.h>

#define DOUT stdout
#define SLEEP(s)
#include "mpipe-syntax.h"

typedef struct pipe_buffer {
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
} pb_t;

typedef struct mpipe_private {
    struct pipe_buffer   *pbuf;
    CHAR                 *data_buffer;
    UINT64               data_size;

    CHAR                 *max_ptr;
    CHAR                 *read_ptr, *write_ptr;
    UINT64               watermark;
    sigset_t             osigmask;
} mp_t;

extern volatile UINT64 *any_reader_sleeping;
extern volatile UINT64 *any_writer_sleeping;

UINT64 MPIPE_Read (VOID *buffer, UINT64 size, mp_t *priv, pb_t *pbuf);
UINT64 MPIPE_Write (VOID *buffer, UINT64 size, mp_t *priv, pb_t *pbuf);
UINT64 MPIPE_InitPipe (CHAR *, UINT64, VOID **);
VOID   MPIPE_Sleep (mp_t *priv, pb_t *pbuf);
VOID   MPIPE_Wakeup (volatile UINT64 *, mp_t *, pb_t *);
UINT64 MPIPE_SleepReadCheck (UINT64 min, mp_t *priv, pb_t *pbuf);
UINT64 MPIPE_SleepWriteCheck (UINT64 min, mp_t *priv, pb_t *pbuf);

static inline VOID   MPIPE_IncDonePos (UINT64 size, mp_t *priv, pb_t *pbuf);
static inline VOID   MPIPE_SetDonePos (UINT64 pos, mp_t *priv, pb_t *pbuf);
static inline VOID   *MPIPE_IncReadPtr (UINT64 size, mp_t *priv, pb_t *pbuf);
static inline VOID   *MPIPE_DecReadPtr (UINT64 size, mp_t *priv, pb_t *pbuf);
static inline VOID   *MPIPE_SetReadPos (UINT64 pos, mp_t *priv, pb_t *pbuf);
static inline UINT64 MPIPE_GetMaxReadSize (UINT64, mp_t *priv, pb_t *pbuf);
static inline VOID   *MPIPE_IncWritePtr (UINT64 size, mp_t *priv, pb_t *pbuf);
static inline UINT64 MPIPE_GetMaxWriteSize (UINT64, mp_t *priv, pb_t *pbuf);

static inline UINT64 MPIPE_GetPipeOverhead () { return sizeof(struct pipe_buffer); }
static inline UINT64 MPIPE_GetReadPos (mp_t *priv, pb_t *pbuf)      { return pbuf->read; }
static inline UINT64 MPIPE_GetWritePos (mp_t *priv, pb_t *pbuf)     { return pbuf->written; }
static inline UINT64 MPIPE_GetDonePos (mp_t *priv, pb_t *pbuf)      { return pbuf->done; }
static inline VOID   *MPIPE_GetReadPtr (mp_t *priv, pb_t *pbuf)     { return priv->read_ptr; }
static inline VOID   *MPIPE_GetWritePtr (mp_t *priv, pb_t *pbuf)    { return priv->write_ptr; }
static inline BOOL   MPIPE_IsEOF(mp_t *priv, pb_t *pbuf)            { return (pbuf->read >= pbuf->eof); }
static inline UINT64 MPIPE_GetEOF(mp_t *priv, pb_t *pbuf)           { return pbuf->eof; }
static inline VOID   MPIPE_SetEOF(mp_t *priv, pb_t *pbuf) { pbuf->eof=pbuf->written;  MPIPE_Wakeup(any_reader_sleeping, priv, pbuf); }

/*
 *
 *  Functions which return pointers to read and write to the shared data,
 *  as well as incrementing the pointers when complete.
 *
 */  

static inline VOID
MPIPE_IncDonePos (UINT64 size, mp_t *priv, pb_t *pbuf)
{
    pbuf->done += size;
    if(pbuf->done >= pbuf->writer_wakeup) {
        DEBUGF(DOUT, "Detect done="FMT64U" >= writer_wakeup="FMT64U"\n",
	       pbuf->written, pbuf->writer_wakeup);
        pbuf->writer_wakeup = UINT64_MAX;
        MPIPE_Wakeup(any_writer_sleeping, priv, pbuf);
    }
}

static inline VOID
MPIPE_SetDonePos (UINT64 pos, mp_t *priv, pb_t *pbuf)
{
    pbuf->done = pos;
    if(*any_writer_sleeping &&
       pbuf->done >= pbuf->written + priv->watermark - priv->data_size) {
//    if(pbuf->done >= pbuf->writer_wakeup) {
        DEBUGF(DOUT, "Detect done="FMT64U" >= wakeup="FMT64U" writer_wakeup="FMT64U"\n",
	       pbuf->written, pbuf->written + priv->watermark - priv->data_size, pbuf->writer_wakeup);
	FLUSHF(DOUT);
        pbuf->writer_wakeup = UINT64_MAX;
        MPIPE_Wakeup(any_writer_sleeping, priv, pbuf);
    }
}

static inline VOID *
MPIPE_IncReadPtr (UINT64 size, mp_t *priv, pb_t *pbuf)
{
    pbuf->read += size;
    priv->read_ptr += size;
    if(priv->read_ptr >= priv->max_ptr)
        priv->read_ptr -= priv->data_size;
    return priv->read_ptr;
}

static inline VOID *
MPIPE_DecReadPtr (UINT64 size, mp_t *priv, pb_t *pbuf)
{
    pbuf->read -= size;
    priv->read_ptr -= size;
    if(priv->read_ptr < priv->data_buffer)
        priv->read_ptr += priv->data_size;
    return priv->read_ptr;
}

static inline VOID *
MPIPE_SetReadPos (UINT64 pos, mp_t *priv, pb_t *pbuf)
{
    INT64 size = pos - pbuf->read;
    if(size > priv->data_size)
        size = size % priv->data_size; // really should be an error here

    // Adjust read count and read pointer,
    //   being carefull to observe buffer boundaries both low and high
    pbuf->read = pos;
    priv->read_ptr += size;
    if(priv->read_ptr < priv->data_buffer)
        priv->read_ptr += priv->data_size;
    if(priv->read_ptr >= priv->max_ptr)
        priv->read_ptr -= priv->data_size;
    return priv->read_ptr;
}

static inline UINT64
MPIPE_GetMaxReadSize (UINT64 min, mp_t *priv, pb_t *pbuf)
{
    UINT64 size = pbuf->written - pbuf->read;
    if(priv->read_ptr + size > priv->max_ptr) {
        size = priv->max_ptr - priv->read_ptr;
        return size;
    }
    // Only sleep reader if there is abs nothing to read
    if(size == 0 && !MPIPE_IsEOF(priv, pbuf))
        return MPIPE_SleepReadCheck(min, priv, pbuf);
    return size;
}

static inline VOID *
MPIPE_IncWritePtr (UINT64 size, mp_t *priv, pb_t *pbuf)
{
    pbuf->written += size;
    priv->write_ptr += size;
    if(priv->write_ptr >= priv->max_ptr)
        priv->write_ptr -= priv->data_size;
//    if(*any_reader_sleeping && pbuf->written >= pbuf->read + priv->watermark) {
    if(pbuf->written >= pbuf->reader_wakeup) {
        DEBUGF(DOUT, "Detect written="FMT64U" >= wakeup="FMT64U" reader_wakeup="FMT64U"\n",
	       pbuf->written, pbuf->read + priv->watermark, pbuf->reader_wakeup);
        pbuf->reader_wakeup = UINT64_MAX;
        MPIPE_Wakeup(any_reader_sleeping, priv, pbuf);
    }
    return priv->write_ptr;
}

static inline UINT64
MPIPE_GetMaxWriteSize (UINT64 min, mp_t *priv, pb_t *pbuf)
{
    UINT64 size = priv->data_size - (pbuf->written - pbuf->done);
    if(priv->write_ptr + size >= priv->max_ptr) {
        size = priv->max_ptr - priv->write_ptr;
        return size;
    }
    if(size < min)
        return MPIPE_SleepWriteCheck(min, priv, pbuf);
    return size;
}

#endif /* _MPIPE_C_H_ */
