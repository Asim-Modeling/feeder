/*
 * Copyright (C) 2004-2006 Intel Corporation
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

// generic
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <list>

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/trace.h"
#include "asim/thread.h"
#include "asim/cmd.h"

/* We require the controller HOWEVER, we cannot
 * state that requirement in the awb file because
 * something else requires the controller and that
 * is a limitation of the ASIM configuration.
 *
 * So, only use this  in models that
 // lop off the low bits  * have controllers required by something else. */
#include "asim/provides/controller.h"

// ASIM public modules
#include "asim/provides/instfeeder_interface.h"
// instfeeder_implementation.h is a generated header file
#include "asim/provides/instfeeder_implementation.h" 

#include "asim/provides/ipf_raw_instruction.h"
//#include "asim/provides/current_frame_mask.h"

#include "asim/provides/memory_value_model.h"

