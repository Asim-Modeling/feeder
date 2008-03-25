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
 * @file
 * @author Artur Klauser
 * @brief IA64-UCSDTrace implementation of trace reader for trace feeder.
 */

// generic
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/trace.h"
#include "asim/thread.h"
#include "asim/ioformat.h"

// ASIM public modules
#include "asim/provides/traceinstruction.h"
#include "asim/provides/tracereader.h"
#include "asim/provides/ia64_ucsd_lowlevel.h"

namespace iof = IoFormat;
using namespace iof;

//--------------------------------------------------------------------------
// TraceReader Interface
//--------------------------------------------------------------------------

TRACE_READER_CLASS::TRACE_READER_CLASS ()
{
    // clear pointers to readers
    for (UINT32 sid = 0; sid < MaxStreams; sid++) {
        stream[sid].ucsd = NULL;
        stream[sid].used = false;
    }

    // instantiate a ucsd reader for each stream
    for (UINT32 sid = 0; sid < MaxStreams; sid++) {
        stream[sid].ucsd = new IA64_UCSD_CLASS;
        if (! stream[sid].ucsd)
        {
            ASIMERROR("TraceReader: No memory for instruction reader.\n");
        }
        stream[sid].instsFed = 0;
    }
}

TRACE_READER_CLASS::~TRACE_READER_CLASS ()
{
    // delete the ucsd reader for each stream
    for (UINT32 sid = 0; sid < MaxStreams; sid++) {
        if (stream[sid].ucsd)
        {
            delete stream[sid].ucsd;
        }
    }
}

bool
TRACE_READER_CLASS::Init (void)
{
    return true;
}

void
TRACE_READER_CLASS::Done (void)
{
    UINT64 total = 0;
    Format fmtLabel = Format("    ", "-25", " = ");

    for (UINT32 sid = 0; sid < MaxStreams; sid++)
    {
        if (stream[sid].used)
        {
            cout << "stream " << sid << ":" << endl;
            cout << fmtLabel("num insts fed")
                 << stream[sid].instsFed << endl;
            total += stream[sid].instsFed;
        }

        if (stream[sid].ucsd)
        {
            delete stream[sid].ucsd;
        }
    }
    cout << "total:" << endl;
    cout << fmtLabel("num insts fed") << total << endl;
}
  
bool
TRACE_READER_CLASS::FileOpen (
    INT32 streamId,
    const char * fileName)
{
    ASSERT(streamId < static_cast<INT32>(MaxStreams),
        "TraceReader trying to open too many concurrent streams\n");

    stream[streamId].used = true;
    return stream[streamId].ucsd->FileOpen(fileName);
}

void
TRACE_READER_CLASS::FileClose (
  INT32 streamId)
{
    ASSERT(streamId < static_cast<INT32>(MaxStreams),
        "TraceReader trying to close too many concurrent streams\n");

    return stream[streamId].ucsd->FileClose();
}

/*
 * Return the next instruction in the trace (we initialize
 * 'inst' and return a pointer to it, we don't allocate a
 * new object), or return NULL if there are no more instructions.
 */
void
TRACE_READER_CLASS::GetNextInst (
    INT32 streamId,
    TRACE_INST traceInst)
{
    ASSERT(streamId < static_cast<INT32>(MaxStreams),
        "TraceReader streamId out of range - too many concurrent streams\n");

    bool success = stream[streamId].ucsd->GetNextInst(traceInst);

    if (success) {
        stream[streamId].instsFed++;
    } else {
        traceInst->SetEof();
    }
}
