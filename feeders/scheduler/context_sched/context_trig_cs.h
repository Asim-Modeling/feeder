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

#ifndef _CONTEXT_TRIG_
#define _CONTEXT_TRIG_

// ASIM core
#include "asim/syntax.h"
#include "asim/module.h"
#include "asim/thread.h"

enum CS_TRIGGER_TYPE 
{
    INTERVAL_TRIG = 0,
    SWAP_INTERVAL_TRIG,
    PV_DEFECT_TRIG,
    ECLASS_UNAVAILABLE_TRIG
};

typedef class CONTEXT_TRIG_CLASS* CONTEXT_TRIG;

class CONTEXT_TRIG_CLASS
{
 public:
    //constructor
    CONTEXT_TRIG_CLASS();
    
    // destructor
    ~CONTEXT_TRIG_CLASS();
    
    bool computeTrigger(UINT64 cycle, HW_CONTEXT* hwc_list, UINT32 numHWC);
    CS_TRIGGER_TYPE GetTriggerType();
    UINT32 GetTriggerHWC();
    
 private:
    CS_TRIGGER_TYPE   trigger_type;
    UINT32            trigger_hwc;
};

#endif /* _CONTEXT_TRIG_ */
