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
 * @file null_micro-inst.cpp
 * @author Sailashri Parthasarathy (based on Chris Weaver's original implementation)
 * @date 2008 July
 *
 */

#include <map>
#include <string>

// ASIM core
#include "asim/trace.h"

// ASIM public modules
#include "asim/provides/isa.h"

UINT32 MAX_MICRO_INST = REC_INST_STAT ? 65000 : 5000;

ASIM_MM_DEFINE(MICRO_INST_CLASS, MAX_MICRO_INST);

// ----------------------------------------------------------------------------
 MICRO_INST_CLASS::MICRO_INST_CLASS(
     SW_CONTEXT sw_context,
     UINT64 _uid) //CONS
{    
}

// ----------------------------------------------------------------------------
MICRO_INST_CLASS::~MICRO_INST_CLASS()
{
}
