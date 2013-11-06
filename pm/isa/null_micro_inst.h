// -*- C++ -*-
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
 * @file null_micro_inst.h
 * @author Sailashri Parthasarathy (based on Chris Weaver's original implementation)
 * @date July 2008
 *
 */


#ifndef _MICRO_INST_
#define _MICRO_INST_


#include "asim/mm.h"

typedef class mmptr<class MICRO_INST_CLASS> MICRO_INST;

#include <string>
#include <list>

// ASIM core
#include "asim/mesg.h"
#include "asim/syntax.h"
#include "asim/trace.h"
#include "asim/event.h"
#include "asim/arch_register.h"
#include "asim/memory_reference.h"
#include "asim/item.h"

typedef class SW_CONTEXT_CLASS* SW_CONTEXT;

class MICRO_INST_CLASS : public ASIM_MM_CLASS<MICRO_INST_CLASS>,
                         public ASIM_ITEM_CLASS
                         //                         public FEEDER_INST_STATE_CLASS
{
  public:
    MICRO_INST_CLASS(SW_CONTEXT sw_context = NULL, UINT64 desired_uid=0); //CONS
    ~MICRO_INST_CLASS();

}; // class MICRO_INST_CLASS 

#include "asim/provides/macro_inst.h"

#endif // _NULL_MICRO_INST_

