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
 * @author Sailashri Parthasarathy (based on Judy Hall's original implementation)
 * @brief Null Software context (to be scheduled on a hardware context)
 */

#ifndef _SW_CONTEXT_CLASS_
#define _SW_CONTEXT_CLASS_

// generic
#include <iostream>
#include <map>

//
// SW and HW contexts have circular dependence unless this is here.
//
typedef class SW_CONTEXT_CLASS *SW_CONTEXT;

// ASIM core
#include "asim/syntax.h"
#include "asim/mm.h"
#include "asim/module.h"
#include "asim/atomic.h"

// ASIM public modules
#include "asim/provides/iaddr.h"
#include "asim/provides/isa.h"

#endif /* _SW_CONTEXT_CLASS_ */
