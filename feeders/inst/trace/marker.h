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

#ifndef _marker_h
#define _marker_h

// ASIM public modules
#include "asim/provides/isa.h"

const UINT32 MAX_MARKER = 16;
const UINT32 MAX_THREAD = 4;
const UINT64 INVALID_MARKER = 0;

typedef class TRACE_MARKER_CLASS *TRACE_MARKER;
class TRACE_MARKER_CLASS
{
  private:
    struct {
        IADDR_CLASS addr[MAX_MARKER];
    } thread[MAX_THREAD];

  public:
    // Constructor
    TRACE_MARKER_CLASS();

    /// Clear marker ID at specific PC.
    void
    ClearPc (
        const UINT32 tid,
        const UINT32 markerID,
        const IADDR_CLASS markerPC);

    /// Clear all markers with ID an any PC.
    void
    ClearAll (
        const UINT32 tid,
        const UINT32 markerID);
    void

    /// Set marker ID at specific PC.
    SetPc (
        const UINT32 tid,
        const UINT32 markerID,
        const IADDR_CLASS markerPC);
    bool

    /// Test if marker ID is set at specific PC.
    IsSet (
        const UINT32 tid,
        const UINT32 markerID,
        const IADDR_CLASS markerPC) const;
};

#endif // _marker_h
