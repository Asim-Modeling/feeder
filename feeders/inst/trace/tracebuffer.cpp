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

// generic
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/trace.h"

// ASIM public modules
#include "asim/provides/traceinstruction.h"
#include "asim/provides/tracereader.h"

// ASIM local module
#include "tracebuffer.h"

/**
 * @brief Fill entire buffer with instructions from trace and
 *        initialize index pointers.
 */
void
TRACE_BUFFER_CLASS::Init (
    const TRACE_READER trd,  ///< use this reader to get new instructions
    const UINT32 tid)        ///< thread ID to associate with this buffer
{
    traceReader = trd;
    sid = tid; // simple mapping: we use the thread ID as stream ID
    head_index = 0;
    lastcommit_index = BufSize - 1;
    filled_index = 0;
    ended = false;
    thread = NULL;
    FillBuffer();
    lastreg_index=0;
}

/**
 * @brief Fill buffer with more instructions and update index pointers.
 */
void
TRACE_BUFFER_CLASS::FillBuffer ( void )
{
    // should be called only when fill is required
    ASSERTX(head_index == filled_index);

    if (filled_index == lastcommit_index)
    {
        cout << "filled_index = " << filled_index << endl;
        cout << "lastcommit_index = " << lastcommit_index << endl;
        ASSERT(false, "Trace buffer overrun on fill");
    }

    // fill new instructions from trace into buffer
    do {
        traceReader->GetNextInst(sid, &buffer[filled_index]);
        T1("\tFillBuffer T" << sid
              << ":" << buffer[filled_index].VirtualPc()
              << " filled_index " << filled_index
              << " lastcommit_index " << lastcommit_index); 
        filled_index = ((filled_index + 1) == BufSize) ? 0 : (filled_index + 1);
    } while (filled_index != lastcommit_index);
}

UINT32
TRACE_BUFFER_CLASS::CommitInstr(const UINT32 id)
{
    UINT32 skippedCount = 0;

    T1("\tCommitInstr T" << sid
          << ":" << buffer[id].VirtualPc()
          << " index " << id
          << " lastcommit " << lastcommit_index);

    // skip over embedded wrongpath
//    do {
//        lastcommit_index = ((lastcommit_index + 1) == BufSize) ? 0 : (lastcommit_index + 1);
//    } while (buffer[lastcommit_index].Wrongpath());

    lastcommit_index = ((lastcommit_index + 1) == BufSize) ? 0 : (lastcommit_index + 1);
    while (lastcommit_index != id)
    {
        // skip over embedded wrongpath, long immediates, and noops
        if (buffer[lastcommit_index].Wrongpath() ||
            buffer[lastcommit_index].LongImm() ||
            buffer[lastcommit_index].Nop())
        {
            lastcommit_index = ((lastcommit_index + 1) == BufSize) ? 0 : (lastcommit_index + 1);
	    ++skippedCount;
        }
        else 
        {
            ASSERT(false, "out of order commit");
        }
    }


    // make sure we are committing what we should
    ASSERT((lastcommit_index == id), "out of order commit");

    return skippedCount;
}

void
TRACE_BUFFER_CLASS::CommitAll()
{

    head_index = 0;
    lastcommit_index = BufSize - 1;
    filled_index = 0;
    ended = false;
    FillBuffer();
    lastreg_index=0;
    lastcommit_index = 0;

}


/**
 * @brief Skip over wrongpath orphan instructions embedded in the trace.
 *
 * (IA64) traces can have embedded wrongpath instructions (syllables
 * after a taken branch in the same bundle). In order to handle these
 * cases with all combination of bundle and syllable based feeders and
 * performance models, we need to be able to search ahead in the trace
 * to see if the next PC we want just skips the wrongpath instructions
 * altogether (e.g. syllable based PM on bundle based feeder). In that
 * case we want to be able to silently ignore those wrongpath orphans.
 */
UINT32
TRACE_BUFFER_CLASS::SkipOverWrongpath (
    const IADDR_CLASS lookaheadPc) ///< skip ahead to this PC in trace
{
    UINT32 skipped = 0;

    if ( ! IsWrongpath()) {
        return skipped;
    }
    
    // now we know we are in a wrongpath section in the trace;
    // look ahead over that section and see if we find the requested
    // PC as the first non-wrongpath instruction; if so, silently skip
    // the wrongpath section;

    UINT32 origId = GetIdentifier();

    while (IsWrongpath()) {
        NextInstr();
        skipped++;
    }
    if (VirtualPc() == lookaheadPc || Eof()) {
        // skip was successful
        // nada
    } else {
        // lookaheadPc nowhere to be found, so stay were you are initially
        Backup(origId);
        skipped = 0;
    }

    return skipped;
}

/**************************************************
 * Find the trace instruction was was used to make
 * asim instruction Uid. If not found return false
 *
 *****************************************/
bool 
TRACE_BUFFER_CLASS::FindUid(UINT64 Uid)
{
    UINT32 start_index=lastreg_index;

    //start with the lastreg index and see if we have
    //a match otherwise increment until we find it
    if(buffer[lastreg_index].Uid()==Uid)
    {
        return true;
    }
    else
    {
        lastreg_index = ((lastreg_index + 1) == BufSize) ? 0 : (lastreg_index + 1);
    }
    
    while(lastreg_index!=start_index )
    {
        if(buffer[lastreg_index].Uid()==Uid)
        {
            return true;
        }
        else
        {
            lastreg_index = ((lastreg_index + 1) == BufSize) ? 0 : (lastreg_index + 1);
        }
    }
    return false;

}

bool 
TRACE_BUFFER_CLASS::GetInputRegVal(
        UINT64 Uid,
        UINT32 slot,
        ARCH_REGISTER reg)
{
    //if we told the simulator to ignore values or this uid was
    //not in the buffer then return false
    if(IGNORE_REG_VALUES || !(FindUid(Uid)))
    {
        return false;
    }
    else
    {
        buffer[lastreg_index].getsrc(slot,reg);
    }
    return true;
}

bool 
TRACE_BUFFER_CLASS::GetPredicateRegVal(
        UINT64 Uid,
        UINT32 slot,
        ARCH_REGISTER reg)
{
    //if we told the simulator to ignore values or this uid was
    //not in the buffer then return false
    if(IGNORE_REG_VALUES || !(FindUid(Uid)))
    {
        return false;
    }
    else
    {
        buffer[lastreg_index].getpred(slot,reg);
    }
    return true;

}

bool 
TRACE_BUFFER_CLASS::GetOutputRegVal(
        UINT64 Uid,
        UINT32 slot,
        ARCH_REGISTER reg)
{
    //if we told the simulator to ignore values or this uid was
    //not in the buffer then return false
    if(IGNORE_REG_VALUES || !(FindUid(Uid)))
    {
        return false;
    }
    else
    {
        buffer[lastreg_index].getdest(slot,reg);
    }
    return true;

}
