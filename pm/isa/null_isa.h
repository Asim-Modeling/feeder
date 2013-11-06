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

#ifndef _ISA_
#define _ISA_

#include "asim/mm.h"

typedef class mmptr<class MICRO_INST_CLASS> ASIM_INST;
typedef class MICRO_INST_CLASS ASIM_INST_CLASS;

typedef class mmptr<class MACRO_INST_CLASS> ASIM_MACRO_INST;
typedef class MACRO_INST_CLASS ASIM_MACRO_INST_CLASS;

// ASIM public modules
#include "asim/provides/micro_inst.h"
#endif
