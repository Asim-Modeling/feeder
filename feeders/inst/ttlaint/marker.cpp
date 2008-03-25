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
#include "asim/marker.h"

// ASIM local module
extern "C"
{
#include "icode.h"
#include "globals.h"
}

//----------------------------------------------------------------------------
// constructor
//----------------------------------------------------------------------------
AINT_MARKER_CLASS::marker() {
  // nada
}


//----------------------------------------------------------------------------
// Clear marker "markerID" in thread "tid" at address "markerPC"
//----------------------------------------------------------------------------
void
AINT_MARKER_CLASS::ClearPc (UINT32 tid, UINT32 markerID, UINT64 markerPC) {
  thread_ptr pthread = &Threads[tid];
  process_ptr process = pthread->process;
  icode_ptr picode;
  ASSERT(markerID < MAX_MARKER, "markerID overflow");
  UINT64 mask = ~(1 << markerID);

  picode = addr2iphys(process, markerPC, NULL);
  if(picode == 0) {
    ASIMERROR("AINT_MarkerSetPc: invalid PC 0x" << markerPC << "\n");
  }

  picode->markers &= mask;
}


//----------------------------------------------------------------------------
// Clear marker "markerID" in thread "tid" at all addresses
//----------------------------------------------------------------------------
void
AINT_MARKER_CLASS::ClearAll (UINT32 tid, UINT32 markerID) {
  thread_ptr pthread = &Threads[tid];
  process_ptr process = pthread->process;
  icode_ptr picode;
  ASSERT(markerID < MAX_MARKER, "markerID overflow");
  UINT64 mask = ~(1 << markerID);
  UINT32 tbn;
  size_t i;
  UINT64 numinsn = 0;

  // reset marker on all instructions
  //
  // find all mapped pages
  //
  for (tbn = 0; tbn < TB_SIZE; tbn++) {
    struct TB_Entry *tbe;
    for (tbe = process->TB[tbn]; tbe != NULL; tbe = tbe->next) {
      //
      // only textpages are of interest here
      //
      if (tbe->textpage) {
        UINT64 tag = tbe->tag;
        UINT64 vpage = (tag << (TB_LISTIDX_LENGTH + TB_OFFSET_LENGTH)) |
                       (tbn << TB_OFFSET_LENGTH);
        //
        // walk through all instruction in this textpage
        //
        for (i = 0; i < TB_PAGESIZE; i += sizeof(int)) {
          UINT64 textaddr = vpage + i;
          icode_ptr picode = addr2iphys(process, textaddr, NULL);
          if (picode == NULL) {
            ASIMERROR("AINT_MarkerClearAll: invalid picode at 0x" << textaddr << "\n");
          }
          //
          // clear marker
          //
          picode->markers &= mask;
          numinsn++;
        }
      }
    }
  }
}


//----------------------------------------------------------------------------
// Set marker "markerID" in thread "tid" at address "markerPC"
//----------------------------------------------------------------------------
void
AINT_MARKER_CLASS::SetPc (UINT32 tid, UINT32 markerID, UINT64 markerPC) {
  thread_ptr pthread = &Threads[tid];
  process_ptr process = pthread->process;
  icode_ptr picode;
  ASSERT(markerID < MAX_MARKER, "markerID overflow");
  UINT64 mask = (1 << markerID);

  picode = addr2iphys(process, markerPC, NULL);
  if(picode == 0) {
    ASIMERROR("AINT_MarkerSetPc: invalid PC 0x" << markerPC << "\n");
  }

  picode->markers |= mask;
}


//----------------------------------------------------------------------------
// Set marker "markerID" in thread "tid" at all instructions that
// match "instBits" on all "1" bits in "instMask", i.e. match on all
// instructions of a particular type.
//----------------------------------------------------------------------------
void
AINT_MARKER_CLASS::SetInst (UINT32 tid, UINT32 markerID,
                    UINT32 instBits, UINT32 instMask) {
  thread_ptr pthread = &Threads[tid];
  process_ptr process = pthread->process;
  icode_ptr picode;
  ASSERT(markerID < MAX_MARKER, "markerID overflow");
  UINT64 mask = (1 << markerID);
  UINT32 tbn;
  size_t i;
  UINT64 numinsn = 0;

  // make sure instBits are clean
  instBits &= instMask;

  // set marker on all instructions that match instBits
  //
  // find all mapped pages
  //
  for (tbn = 0; tbn < TB_SIZE; tbn++) {
    struct TB_Entry *tbe;
    for (tbe = process->TB[tbn]; tbe != NULL; tbe = tbe->next) {
      //
      // only textpages are of interest here
      //
      if (tbe->textpage) {
        UINT64 tag = tbe->tag;
        UINT64 vpage = (tag << (TB_LISTIDX_LENGTH + TB_OFFSET_LENGTH)) |
                       (tbn << TB_OFFSET_LENGTH);
        //
        // walk through all instruction in this textpage
        //
        for (i = 0; i < TB_PAGESIZE; i += sizeof(int)) {
          UINT64 textaddr = vpage + i;
          icode_ptr picode = addr2iphys(process, textaddr, NULL);
          if (picode == NULL) {
              ASIMERROR("AINT_MarkerClearAll: invalid picode at 0x" << textaddr << "\n");
          }
          if ((picode->instr & instMask) == instBits) {
            // set marker if significant bits match
            picode->markers |= mask;
            numinsn++;
          }
        }
      }
    }
  }
}


//----------------------------------------------------------------------------
// Is marker "markerID" set in thread "tid" at address "markerPC"
//----------------------------------------------------------------------------
bool
AINT_MARKER_CLASS::IsSet (UINT32 tid, UINT32 markerID, UINT64 markerPC) {
  thread_ptr pthread = &Threads[tid];
  process_ptr process = pthread->process;
  icode_ptr picode;
  ASSERT(markerID < MAX_MARKER, "markerID overflow");
  UINT64 mask = (1 << markerID);

  picode = addr2iphys(process, markerPC, NULL);
  if(picode == 0) {
    ASIMERROR("AINT_MarkerSetPc: invalid PC 0x" << markerPC << "\n");
  }

  return ((picode->markers & mask) != 0);
}
