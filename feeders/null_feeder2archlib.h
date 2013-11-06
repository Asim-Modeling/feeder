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

// Null feeder2archlib module

#ifndef _FEEDER_2_ARCHLIB_H
#define _FEEDER_2_ARCHLIB_H

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/mm.h"
#include "asim/provides/micro_inst.h"
#include "asim/provides/feeder_2_archlib.h"

class NULL_FEEDER_2_ARCHLIB_CLASS:  public TRACEABLE_CLASS
{
  public:

    NULL_FEEDER_2_ARCHLIB_CLASS();
    ~NULL_FEEDER_2_ARCHLIB_CLASS() {};
};

#endif
