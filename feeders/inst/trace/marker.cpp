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
 * @brief Limited "marker" functionality for Instruction Trace Feeder.
 *
 * Implements limited "marker" functionality for Instruction Trace Feeder.
 * A marker is a binary flag on a static instruction. We currently support
 * up to 16 markers, each marker can watch 1 PC address.
 * Functionality is provided to set, clear, and query markers on instructions.
 */

// generic
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/trace.h"

// ASIM local module
#include "marker.h"

//----------------------------------------------------------------------------
// constructor
//----------------------------------------------------------------------------
TRACE_MARKER_CLASS::TRACE_MARKER_CLASS()
{
    for (UINT32 tid = 0; tid < MAX_THREAD; tid++) {
        for (UINT32 mid = 0; mid < MAX_MARKER; mid++) {
            thread[tid].addr[mid] = INVALID_MARKER;
        }
    }
}

//----------------------------------------------------------------------------
// Clear marker "markerID" in thread "tid" at address "markerPC"
//----------------------------------------------------------------------------
void
TRACE_MARKER_CLASS::ClearPc (
    const UINT32 tid,
    const UINT32 markerID, 
    const IADDR_CLASS markerPC)
{
    ASSERT(tid < MAX_THREAD, "threadID overflow");
    ASSERT(markerID < MAX_MARKER, "markerID overflow");

    if (thread[tid].addr[markerID] == markerPC) {
        thread[tid].addr[markerID] = INVALID_MARKER;
    }
}

//----------------------------------------------------------------------------
// Clear marker "markerID" in thread "tid" at all addresses
//----------------------------------------------------------------------------
void
TRACE_MARKER_CLASS::ClearAll (
    const UINT32 tid,
    const UINT32 markerID)
{
    ASSERT(tid < MAX_THREAD, "threadID overflow");
    ASSERT(markerID < MAX_MARKER, "markerID overflow");

    thread[tid].addr[markerID] = INVALID_MARKER;
}

//----------------------------------------------------------------------------
// Set marker "markerID" in thread "tid" at address "markerPC"
//----------------------------------------------------------------------------
void
TRACE_MARKER_CLASS::SetPc (
    const UINT32 tid,
    const UINT32 markerID,
    const IADDR_CLASS markerPC) 
{
    ASSERT(tid < MAX_THREAD, "threadID overflow");
    ASSERT(markerID < MAX_MARKER, "markerID overflow");

    thread[tid].addr[markerID] = markerPC;
}

//----------------------------------------------------------------------------
// Is marker "markerID" set in thread "tid" at address "markerPC"
//----------------------------------------------------------------------------
bool
TRACE_MARKER_CLASS::IsSet (
    const UINT32 tid,
    const UINT32 markerID,
    const IADDR_CLASS markerPC) const
{
    ASSERT(tid < MAX_THREAD, "threadID overflow");
    ASSERT(markerID < MAX_MARKER, "markerID overflow");

    return (thread[tid].addr[markerID] == markerPC);
}
