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
 * @brief IA64-GTrace implementation of trace reader for trace feeder.
 */

#ifndef _IA64_GTRACE_READER_H
#define _IA64_GTRACE_READER_H

// ASIM core
#include "asim/syntax.h"

// ASIM public modules
#include "asim/provides/traceinstruction.h"
#include "asim/provides/ia64_ucsd_lowlevel.h"

typedef class TRACE_READER_CLASS *TRACE_READER;
class TRACE_READER_CLASS
{
    // consts
    static const UINT32 MaxStreams = 1; ///< UCSD reader only handles 1 stream

    struct {
        IA64_UCSD ucsd;   ///< a ucsd reader per input stream
        UINT64 instsFed;  ///< number of instructions fed
        bool used;        ///< this stream has been used
    } stream[MaxStreams];

  public:
    // constructors / destructors / initializers
    TRACE_READER_CLASS ();
    ~TRACE_READER_CLASS ();
    bool Init (bool skipIdle, UINT32 numCpus);
    void Done (void);

    // trace file interface
    bool FileOpen(INT32 streamId, const char * fileName);
    void FileClose(INT32 streamId);

    // trace buffer fill interface
    void GetNextInst (INT32 streamId, TRACE_INST inst);
};

#endif // _IA64_GTRACE_READER_H
