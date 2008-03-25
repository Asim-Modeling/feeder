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
 * @author Artur Klauser
 * @brief NULL implementation of trace reader for trace feeder.
 */

#ifndef _NULLREADER_H
#define _NULLREADER_H

// ASIM core
#include "asim/syntax.h"

// ASIM public modules
#include "asim/provides/traceinstruction.h"

typedef class TRACE_READER_CLASS *TRACE_READER;
class TRACE_READER_CLASS
{
  public:
    // constructors / destructors / initializers
    TRACE_READER_CLASS () {}
    bool Init (bool skipIdle, UINT32 numCpus) { return true; }
    void Done (void) {}

    // trace file interface
    bool FileOpen(INT32 streamId, const char * fileName) { return true; }
    void FileClose(INT32 streamId) {}

    // trace buffer fill interface
    void GetNextInst (INT32 streamId, TRACE_INST inst) {}
};

#endif // _NULLREADER_H
