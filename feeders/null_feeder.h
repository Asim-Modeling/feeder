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

// Author: Sailashri Parthasarathy (based on Chris Weaver's original implementation)
//

#ifndef _FEEDER_H
#define _FEEDER_H


// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/trace.h"
#include "asim/dynamic_array.h"

// ASIM public modules
#include "asim/provides/instfeeder_interface.h"

class NULL_FEEDER_CLASS : public IFEEDER_BASE_CLASS
{
  public:
    NULL_FEEDER_CLASS();
    ~NULL_FEEDER_CLASS() {};

    bool Init(UINT32 argc,char** argv, char** envp) { return true; }
    
    void Done(void);

    bool DTranslate(ASIM_INST inst, UINT64 va, UINT64& pa);
    
    bool DTranslate(ASIM_INST inst,
                    const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
                    UINT64& pa,
                    PAGE_TABLE_INFO pt_info,
                    MEMORY_VIRTUAL_REFERENCE_CLASS& vNextRegion);

}; /* class NULL_FEEDER_CLASS */


#endif /*_FEEDER_H*/
