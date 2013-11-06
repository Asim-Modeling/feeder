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
 * @brief Implementation of the Null context scheduler class
 * @author Sailashri Parthasarathy (based on Judy Hall's original implementation)
 *
 */

#include "asim/provides/context_scheduler.h"
#include "asim/provides/software_context.h"
#include "asim/provides/hardware_context.h"
#include "asim/trace.h"


/* ---------------- CONTEXT_SCHEDULER_CLASS -----------------------------*/

/**
 * Constructor: initializes data structures for scheduler. 
 */

CONTEXT_SCHEDULER_CLASS::CONTEXT_SCHEDULER_CLASS (ASIM_MODULE parent, const
                                                  char* name)
  : ASIM_MODULE_CLASS(parent,name)
{
}


