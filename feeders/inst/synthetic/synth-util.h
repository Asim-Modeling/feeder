/*
 * Copyright (C) 2003-2006 Intel Corporation
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

// synth-util.h
// Mark Charney   <mark.charney@intel.com>
// $Id: synth-util.h 799 2006-10-31 12:27:52Z cjbeckma $


#ifndef _SYNTH_UTIL_H_
# define _SYNTH_UTIL_H_

#include "asim/syntax.h"

static const UINT64 bytes_per_word = 4;
static const UINT64 BUNDLE_ADDR_MASK = ~(0xFULL); /* lop off low 4 bits */
static const UINT32 BYTES_PER_BUNDLE = 16;

UINT32 count_ones(UINT32 x);
UINT32 ilog(UINT32 arg);
bool is_power_of_2(UINT32 x);

UINT64 mask_bundle_addr(UINT64 x); // lop off the low bits
bool even_bundle(IADDR_CLASS pc);

////////////////////////////////////////////////////////////////////////////
// RANDOM NUMBER GENERATION
////////////////////////////////////////////////////////////////////////////



void init_random_number_generator(void);

double random_pct(void);
bool random_over_threshold(UINT32 thresh);
bool random_under_threshold(UINT32 thresh);
UINT64 random_aligned_address(UINT64 shift);
UINT32 random_in_range(UINT32 x);    // x must be greater than 1.

#endif
