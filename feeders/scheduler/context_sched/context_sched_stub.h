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
 * @brief Header file for the scheduler
 * @author Pritpal Ahuja
 */

#ifndef _CONTEXT_SCHEDULER_
#define _CONTEXT_SCHEDULER_


// ASIM core
#include "asim/syntax.h"
#include "asim/module.h"
#include "asim/thread.h"

typedef class SW_CONTEXT_CLASS * SW_CONTEXT;
typedef class HW_CONTEXT_CLASS * HW_CONTEXT;


// ASIM other modules

#include "asim/provides/instfeeder_implementation.h"


typedef class CONTEXT_SCHEDULER_CLASS* CONTEXT_SCHEDULER;

class CONTEXT_SCHEDULER_CLASS : public ASIM_MODULE_CLASS
{
  public:

    CONTEXT_SCHEDULER_CLASS(ASIM_MODULE parent, const char* const name);

    ~CONTEXT_SCHEDULER_CLASS();

    void AddHardwareContext(HW_CONTEXT hwc);

    void Clock(const UINT64 cycle);

    bool StartThread (ASIM_THREAD thread, UINT64 cycle) { return true; }
    bool RemoveThread (ASIM_THREAD thread, UINT64 cycle) { return true; }
    bool UnblockThread () { return true; }
    bool BlockThread () { return true; }
    bool HookAllThreads() { return true; }
    bool UnhookAllThreads() { return true; }
    bool IsHWCActive(UINT32 hwc_num) const { return true; }

  private:

};


#endif /* _CONTEXT_SCHEDULER_ */









