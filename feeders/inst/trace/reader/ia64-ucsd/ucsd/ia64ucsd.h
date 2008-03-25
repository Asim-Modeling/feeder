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
 * @brief IA64 Trace Reader for UCSD format
 */

#ifndef _IA64_UCSD_H
#define _IA64_UCSD_H

// generic
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// ASIM core
#include "asim/syntax.h"

// ASIM public modules
#include "asim/provides/traceinstruction.h"

typedef class IA64_UCSD_CLASS *IA64_UCSD;
class IA64_UCSD_CLASS
{
  private:
    // variables
    const UINT32 traceID;            ///< ID of this trace stream

    /// Get instruction bits from the trace.
    INT32 GetInstBits (UINT64 vpc, TRACE_INST traceInst);

  public:
    // constructors / destructors / initializers
    IA64_UCSD_CLASS(UINT32 id = 0);
    ~IA64_UCSD_CLASS();

    // interface
    bool FileOpen (const char * fileName);
    void FileClose (void);
    bool GetNextInst (TRACE_INST traceInst);
};

#endif // _IA64_UCSD_H
