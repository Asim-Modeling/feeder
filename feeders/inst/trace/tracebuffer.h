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
 * @brief Buffer for trace feeder for skipping backward and forward.
 */

#ifndef _TRACEBUFFER_H
#define _TRACEBUFFER_H

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

// ASIM public modules
#include "asim/provides/traceinstruction.h"
#include "asim/provides/tracereader.h"

// we leave this as #define so ASSERT can give us more useful
// linenumber information when an assertion fails
#define IDXCHK ASSERT(head_index < BufSize, "TraceBuffer index out of range")

typedef class TRACE_BUFFER_CLASS *TRACE_BUFFER;
class TRACE_BUFFER_CLASS : public TRACEABLE_CLASS
{
  private:
    // consts
    static const UINT32 BufSize = 2048;  ///< total buffer size

    // variables
    TRACE_INST_CLASS buffer[BufSize]; ///< the actual buffer
    TRACE_READER traceReader;  ///< used to read new instructions into buffer
    UINT32 filled_index;       ///< next index to get filled
    UINT32 head_index;         ///< index to get next instruction from
    UINT32 lastcommit_index;   ///< index of last committed instruction
    UINT32 lastreg_index;      ///< index of the last reg read from the buffer
    UINT32 sid;                ///< stream ID to use for reading from trace
    bool ended;                ///< has this trace ended already?
    IFEEDER_THREAD thread;     ///< thread associated with this trace stream

    void FillBuffer (void);

  public:
    TRACE_BUFFER_CLASS() {
        SetTraceableName("TRACE_BUFFER_CLASS");
    }

    // constructors / destructors / initializers
    void Init (const TRACE_READER trd, const UINT32 tid);

    // accessors
    TRACE_INST
    GetInst(void) const { IDXCHK; return TRACE_INST(&buffer[head_index]); }
    IADDR_CLASS VirtualPc(void) const { return GetInst()->VirtualPc(); }
    bool IsWrongpath(void) const { return GetInst()->Wrongpath(); }
    UINT64
    VirtualEffAddress(void) const { return GetInst()->VirtualEffAddress(); }
    bool IsWarmUp(void) const { return GetInst()->IsWarmUp(); }
    bool Eof(void) const { return GetInst()->Eof(); }
    UINT32 GetIdentifier(void) const { IDXCHK;  return head_index; }
    bool& Ended(void) { return ended; }
    IFEEDER_THREAD& Thread(void) { return thread; }

    // modifiers
    void NextInstr(void);
    void PrevInstr(void);
    void Backup(const UINT32 id) { IDXCHK;  head_index = id; IDXCHK; }
    UINT32 CommitInstr(const UINT32 id);
    void CommitAll(void);
    bool FindUid(UINT64 Uid);
    bool GetInputRegVal(
        UINT64 Uid,
        UINT32 slot,
        ARCH_REGISTER reg);

    bool GetPredicateRegVal(
        UINT64 Uid,
        UINT32 slot,
        ARCH_REGISTER reg);

    bool GetOutputRegVal(
        UINT64 Uid,
        UINT32 slot,
        ARCH_REGISTER reg);

    UINT32 SkipOverWrongpath(const IADDR_CLASS lookaheadPc);
};

/// Advance to next instruction.
inline void
TRACE_BUFFER_CLASS::NextInstr( void )
{
    head_index = ((head_index + 1) == BufSize) ? 0 : (head_index + 1);

    if (head_index == filled_index)
    {
        // Need to fill more instructions into the buffer
        FillBuffer();
    }
}

/// Move back to previous instruction.
inline void
TRACE_BUFFER_CLASS::PrevInstr( void )
{
    ASSERT( head_index != lastcommit_index,
        "Trace buffer underflow while trying to back up");
    if(head_index == 0)
    {
        head_index = BufSize - 1;
    }
    else
    {
        head_index--;
    }
}

#endif // _TRACEBUFFER_H
