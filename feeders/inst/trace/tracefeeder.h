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
 
/**
 * @file
 * @author Steven Wallace, Artur Klauser
 * @brief Trace feeder implementation of ASIM instruction feeder.
 */

#ifndef _TRACEFEEDER_H
#define _TRACEFEEDER_H

// generic
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/trace.h"
#include "asim/cmd.h"
#include "asim/alphaops.h"

// ASIM public modules
#include "asim/provides/instfeeder_interface.h"
#include "asim/provides/isa.h"
#include "asim/provides/traceinstruction.h"
#include "asim/provides/tracereader.h"
#include "asim/provides/traceconverter.h"
#include "asim/provides/addrtranslator.h"

// ASIM local module
#include "tracebuffer.h"
//#include "Oracle.h"
#include "marker.h"


class TRACE_FEEDER_CLASS : public IFEEDER_BASE_CLASS
{
  private:
    // consts
    static const UINT32 traceWrongpathId = UINT32_MAX;
    static const UINT32 traceArtificialInstId = UINT32_MAX - 1;

    // variables
    TRACE_READER traceReader;         ///< the trace reader
    TRACE_CONVERTER traceConverter;   ///< the trace converter
    TRACE_BUFFER_CLASS * trace;       ///< pointer to array of trace buffers
    bool wrongpath[MAX_TOTAL_NUM_HWCS]; ///< wrongpath flags
    UINT32 threads;                   ///< number of threads

    UINT32 totalSkippedWrongpath;

    UINT64 mtInstLimit;		// When all of the threads have reached this limit, we're done
    UINT64 committedInsts[MAX_TOTAL_NUM_HWCS];
    UINT32 mtThreadsRunningVector;	//  When all bits are cleared, we're done
    bool   mtExitCondition;		//  Exit when this becomes set

    bool runOracle;

    ADDRESS_TRANSLATOR_CLASS addrTrans;  ///< Translates from virtual to physical address

    // Locks for multi-threaded model
    pthread_mutex_t fetchLock;
    pthread_mutex_t commitLock;
    pthread_mutex_t killLock;

    void MTLock(pthread_mutex_t &lock);
    void MTUnlock(pthread_mutex_t &lock);

//    TRACE_ORACLE oracle;
    TRACE_MARKER_CLASS marker;

    //
    // TRACE_HANDLE is the internal representation of an IFEEDER_STREAM_HANDLE.
    // TRACE_HANDLE is one less than IFEEDER_STREAM_HANDLE so that the former
    // is 0 based and the latter has NULL meaning all streams.
    //
    class TRACE_HANDLE
    {
      public:
        TRACE_HANDLE(PTR_SIZED_UINT h) :
            handle(h)
        {};

        TRACE_HANDLE(IFEEDER_STREAM_HANDLE h)
        {
            ASSERTX(h != NULL);
            handle = (PTR_SIZED_UINT) h - 1;
        };

        PTR_SIZED_UINT Handle(void)
        {
            return handle;
        };

        operator PTR_SIZED_UINT()
        {
            return handle;
        };

        operator IFEEDER_STREAM_HANDLE()
        {
            return (IFEEDER_STREAM_HANDLE) (handle + 1);
        };

      private:
        PTR_SIZED_UINT handle;
    };

    // methods
    bool
    Parse (
        UINT32 argc,
        char **argv);

    UINT64
    SkipHelper (
        const UINT32 tid,
        const UINT64 n,
        const INT32 markerID);

  public:
    // constructors
    TRACE_FEEDER_CLASS();
    ~TRACE_FEEDER_CLASS();

    // public feeder interface via FEED_*
    virtual bool
    Init (
        UINT32 argc,
        char **argv,
        char **envp);

    void Done (void);

    virtual bool GetInputRegVal(
        ASIM_INST inst,
        UINT32 slot,
        ARCH_REGISTER reg);

    virtual bool GetPredicateRegVal(
        ASIM_INST inst,
        UINT32 slot,
        ARCH_REGISTER reg);

    virtual bool GetOutputRegVal(
        ASIM_INST inst,
        UINT32 slot,
        ARCH_REGISTER reg);


    virtual UINT64
    Skip (
        IFEEDER_STREAM_HANDLE stream,
        UINT64 n,
        INT32 markerID = -1);

    virtual void
    Marker (
        const ASIM_MARKER_CMD cmd,
        const IFEEDER_STREAM_HANDLE stream,
        const UINT32 markerID,
        const IADDR_CLASS markerPC,
        const UINT32 instBits = 0,
        const UINT32 instMask = 0);

    virtual bool
    ITranslate(
        const UINT32 hwcNum,
        const UINT64 va,
        UINT64& pa);

    virtual bool
    DTranslate(
        ASIM_INST inst,
        const UINT64 va,
        UINT64& pa);

    virtual bool
    WarmUp(
        IFEEDER_STREAM_HANDLE stream,
        WARMUP_INFO warmup);

    virtual bool
    Fetch(
        IFEEDER_STREAM_HANDLE stream,
        IADDR_CLASS predicted_pc, 
        ASIM_INST inst);

    virtual void
    Commit(
        ASIM_INST inst);

    virtual void
    Kill(
        ASIM_INST inst,
        const bool fetchNext,
        const bool killMe);
};


//
// Lock management functions for multi-threaded models.  Conditionally compiled
// to do the least work possible on single-threaded runs.
// 
inline void
TRACE_FEEDER_CLASS::MTLock(pthread_mutex_t &lock)
{
#if MAX_PTHREADS > 1
    if (ASIM_SMP_CLASS::GetTotalRunningThreads() > 1)
    {
        pthread_mutex_lock(&lock);
    }
#endif
};

inline void
TRACE_FEEDER_CLASS::MTUnlock(pthread_mutex_t &lock)
{
#if MAX_PTHREADS > 1
    if (ASIM_SMP_CLASS::GetTotalRunningThreads() > 1)
    {
        pthread_mutex_unlock(&lock);
    }
#endif
};

#endif // _TRACEFEEDER_H
