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

// ASIM core
#include "asim/trace.h"
#include "asim/syntax.h"
#include "asim/atomic.h"

// ASIM local stuff
#include "asim/provides/hardware_context.h"
#include "asim/provides/context_trigger.h"

CONTEXT_TRIG_CLASS::CONTEXT_TRIG_CLASS () : trigger_type(INTERVAL_TRIG), trigger_hwc(0)
{}

CONTEXT_TRIG_CLASS::~CONTEXT_TRIG_CLASS () {}

// computeTrigger:
// Only 1 outstanding trigger is allowed in the system. This can be
// easily extended by maintaining more state to record trigger requesting hwc, 
// and other stuff in context scheduler, sw context etc. Presently, context scheduler
// class raises an assertion failure when it sees a trigger generated while the
// old context switch is not over.
bool 
CONTEXT_TRIG_CLASS::computeTrigger(UINT64 cycle, HW_CONTEXT* hwc_list, UINT32 numHWC) 
{
    bool trigger_core_swap = false;
    UINT32 i;
    
    // is there a context switch interval?
    if (CS_INTERVAL_TIMER != 0) 
    {
        trigger_core_swap = ((cycle % CS_INTERVAL_TIMER == 0) && (cycle != 0));
        trigger_type = INTERVAL_TRIG;
	trigger_hwc = UINT32_MAX;
	
	ASSERTX((CS_SWAP_TIMER == 0) && (CS_ECLASS_DEFECTS == 0) && (CS_PV_DEFECTS == 0));
    }

    if (CS_SWAP_TIMER != 0) 
    {
        trigger_core_swap = ((cycle % CS_SWAP_TIMER == 0) && (cycle != 0));
        trigger_type = SWAP_INTERVAL_TRIG;
	trigger_hwc = UINT32_MAX;

	if (CS_PV_DEFECTS != 0) 
	{
	    trigger_type = PV_DEFECT_TRIG;	    
	}
	ASSERTX(CS_ECLASS_DEFECTS == 0);
    }

    // is there any exec class defect trigger? 
    if (CS_ECLASS_DEFECTS != 0) 
    {
        // go over all the hardware context and see if the bit is set
        for (i = 0; i < numHWC; i++) {
            if (hwc_list[i]->GetTriggerContextSwitch()) {
                trigger_type = ECLASS_UNAVAILABLE_TRIG;
                trigger_core_swap = true;
                trigger_hwc = i;
                // reset the trigger bit
                hwc_list[i]->ResetTriggerContextSwitch();
                // cout << "Trigger seen by the compute trigger function for hwc " 
                //     << trigger_hwc << endl;
                break;
            }
        }
    }

    return trigger_core_swap;
}

// what kind of a trigger was generated?
CS_TRIGGER_TYPE CONTEXT_TRIG_CLASS::GetTriggerType() 
{
    return trigger_type;
}

// where did the trigger originate?
UINT32 CONTEXT_TRIG_CLASS::GetTriggerHWC()
{
    return trigger_hwc;
}

