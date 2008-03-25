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
 * *****************************************************************
 * *                                                               *
 
/**
 * @file
 * @author Artur Klauser and Steven Wallace
 * @brief IA64-GTrace implementation of trace reader for trace feeder.
 */

#ifndef _IA64_GTRACE_READER_H
#define _IA64_GTRACE_READER_H

// ASIM core
#include "asim/syntax.h"

// ASIM other modules
#include "asim/provides/traceinstruction.h"

// ASIM local module
#include "ia64ciardi.h"

typedef class TRACE_READER_CLASS *TRACE_READER;
class TRACE_READER_CLASS : public TRACEABLE_CLASS
{
    // consts
    static const UINT32 MaxStreams = 128;

    struct {
        IA64_CIARDI ciardi; ///< a ciardi reader per input stream
        IA64_CIARDI_BUNDLE currentBundle; ///< bundle at current trace position
        UINT64 bundlesFed;  ///< number of bundles fed
        INT32 lastSyllable; ///< last syllable of current bundle passed back
        bool used;          ///< stream has been used
        UINT64 cfm;
        UINT64 PRF;
        UINT64 bsp;         ///<the bsp of the instruction updated when it changes
        UINT64 nRemainingWarmup;  ///< Number of upcoming instrs to treat as warm-up
    } stream[MaxStreams];

    INT8 pr_v2p (INT8 virt, UINT64 cfm);
    UINT64 pregs_v2p (UINT64 pregs, UINT64 cfm);
    UINT64 FixCFM(INT32 streamId, TRACE_INST inst);

  public:
    // constructors / destructors / initializers
    TRACE_READER_CLASS ();
    bool Init (void);
    void Done (void);

    // trace file interface
    bool FileOpen(INT32 streamId, const char * fileName);
    void FileClose(INT32 streamId);

    // trace buffer fill interface
    void GetNextInst (INT32 streamId, TRACE_INST inst);
};

#endif // _IA64_GTRACE_READER_H
