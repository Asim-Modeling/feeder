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
 * @brief Implementation of the scheduler class
 * @author Pritpal Ahuja
 */

// ASIM core
#include "asim/mesg.h"
#include "asim/trace.h"

// ASIM provides
#include "asim/provides/context_scheduler.h"



CONTEXT_SCHEDULER_CLASS::CONTEXT_SCHEDULER_CLASS(ASIM_MODULE parent, const char* const name)
    : ASIM_MODULE_CLASS(parent, name)
{
    TRACE(Trace_Context, cout << "\tCONTEXT_SCHEDULER_CLASS constructor was called" << endl);
}

CONTEXT_SCHEDULER_CLASS::~CONTEXT_SCHEDULER_CLASS ()
{}

void
CONTEXT_SCHEDULER_CLASS::AddHardwareContext(HW_CONTEXT hwc)
{
    TRACE(Trace_Context, cout << "\tAddHardwareContext called for ");

    if (hwc == NULL)
    {
        TRACE(Trace_Context, cout << "NULL" << endl);
        return;
    }

    ASIMERROR("AddHardwareContext(non-NULL) should never be called when using a STUBBED-OUT context scheduler!!\n");

}

void
CONTEXT_SCHEDULER_CLASS::Clock(const UINT64 cycle)
{}
