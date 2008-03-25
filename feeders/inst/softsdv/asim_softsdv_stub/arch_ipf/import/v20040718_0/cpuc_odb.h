/*****************************************************************************
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
 * File:  cpuc_odb.h
 *
 * Description:
 *
 *
 *****************************************************************************/
#ifndef __Cpuc__Odb__
#define __Cpuc__Odb__

#include "cpuapi.h"

// Deivce/Model definitions
#define MAX_CPU_DEVICES   32
#define MAX_CPU_MODELS    4
#define MAX_NAME          256

// The state of the cpu model
#define CPU_REGISTERED 0
#define CPU_ACTIVE     1 

/*
// The type of cpu model
#define CPUAPI_Type_Unsync_Func    0x1
#define CPUAPI_Type_Unsync_Perf    0x2
#define CPUAPI_Type_Master_Func    0x4
#define CPUAPI_Type_Master_Fetch   0x8
#define CPUAPI_Type_Master_Retire  0x10
#define CPUAPI_Type_Slave_Func     0x20
#define CPUAPI_Type_Slave_Perf     0x40
*/

#endif // __Cpuc__Odb__
