/**
 *
 * @file deptype.h
 * @brief Header file for Tanglewood Retire 
 *
 * @author Chris Weaver, Tanglewood Architecture, MMDC, Intel Corporation
 * @date 7/4/02
 *
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
#include "asim/syntax.h"
#ifndef _va_deptype_
#define _va_deptype_

//int low=0;
//set low to 1 to get low level metrics.
//low =1;

struct va_address{
  UINT64 address;
  UINT32 last_thread;
  UINT64 last_icount;
  UINT32 write_td[19]; //which thread last wrote
  UINT32 write_cnt_td[19]; //number of write
  UINT32 read_cnt_td[19]; //number read
  UINT32 invalid_cnt_td[19]; //number of invalid read/write
};


#endif 





