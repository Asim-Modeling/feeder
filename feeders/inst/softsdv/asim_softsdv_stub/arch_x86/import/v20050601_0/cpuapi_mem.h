/*
 * Copyright (C) 2005-2006 Intel Corporation
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
 */

#ifndef CPUAPI_MEM_H
#define CPUAPI_MEM_H


#ifdef WIN32
typedef char               cpuapi_u8_t;
typedef unsigned short     cpuapi_u16_t;
typedef short              cpuapi_s16_t;
typedef unsigned int       cpuapi_u32_t;
typedef int                cpuapi_s32_t;
typedef unsigned __int64   cpuapi_u64_t;
typedef __int64            cpuapi_s64_t;
#else
typedef char               cpuapi_u8_t;
typedef unsigned short     cpuapi_u16_t;
typedef short              cpuapi_s16_t;
typedef unsigned int       cpuapi_u32_t;
typedef int                cpuapi_s32_t;
typedef unsigned long long cpuapi_u64_t;
typedef long long          cpuapi_s64_t;
#endif


typedef void         *cpuapi_trans_t;

#define CPUAPI_Null_Signal                0x0
#define CPUAPI_Write_Completion           0x1
#define CPUAPI_Global_Observation         0x2
#define CPUAPI_Prepare_For_Critical_Chunk 0x4
#define CPUAPI_Target_Ready               0x8
#define CPUAPI_Acknowledge                0x10

typedef cpuapi_u32_t cpuapi_signal_t;

typedef struct cpuapi_access_s cpuapi_access_t;

typedef enum 
{
    CPUAPI_Access_Read,
    CPUAPI_Access_RFO,
    CPUAPI_Access_Write,
    CPUAPI_Access_Execute
} cpuapi_access_type_t;

typedef enum 
{
    CPUAPI_Space_Mem,
    CPUAPI_Space_IO,
    CPUAPI_Space_Data_Cache,
    CPUAPI_Space_Inst_Cache,
    CPUAPI_Space_LaGrande
} cpuapi_space_t;


typedef cpuapi_u64_t cpuapi_phys_addr_t;
typedef cpuapi_u64_t cpuapi_size_t;

typedef enum cpuapi_snoop_s 
{
    CPUAPI_Snoop_Miss,
    CPUAPI_Snoop_Hit,
    CPUAPI_Snoop_HitM,
} cpuapi_snoop_t;



#endif /* CPUAPI_MEM_H */

