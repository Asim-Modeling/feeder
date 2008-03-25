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
 
//
// Authors:  Artur Klauser
//
// Implements "marker" functionality in AINT.
// A marker is a binary flag on a static instruction. We currently support
// up to 64 markers on an instruction by storing a 64-bit vector with each
// instruction. Functionality is provided to set, clear, and query markers
// on instructions.
//

#ifndef _marker_h
#define _marker_h

const UINT32 MAX_MARKER = 64;

typedef class marker {
private:

public:
  // Constructor
  marker();

  void ClearPc (UINT32 tid, UINT32 markerID, UINT64 markerPC);
  void ClearAll (UINT32 tid, UINT32 markerID);
  void SetPc (UINT32 tid, UINT32 markerID, UINT64 markerPC);
  void SetInst (UINT32 tid, UINT32 markerID,
         UINT32 instBits, UINT32 instMask);
  bool IsSet (UINT32 tid, UINT32 markerID, UINT64 markerPC);

} AINT_MARKER_CLASS, *AINT_MARKER;

#endif // _marker_h
