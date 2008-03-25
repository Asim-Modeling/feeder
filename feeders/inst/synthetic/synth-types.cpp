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

// synth-types.cpp
// Mark Charney   <mark.charney@intel.com>
// $Id: synth-types.cpp 799 2006-10-31 12:27:52Z cjbeckma $

#include "synth-types.h"
#include <assert.h>


static char* ftype_names[] = {
    "INVALID",
    "MEMOP",
    "LOAD",
    "STORE",
    "EXCHANGE",
    "COMPARE_EXCHANGE",
    "FENCE",
    "DEPENDENT_READ",
    "BRANCH",
    "LONG_BRANCH",
    "LIMM",
    "NOP",
    "XOR",
    "FMA",
    "COMPARE",
    "FETCH_AND_ADD",
    "LFETCH",
    0
};


std::ostream& operator<<(std::ostream& o, const SYNTH_FTYPE& f)
{
    assert(f <  SYNTH_FTYPE_LAST);
    o << ftype_names[f];
    return o;
}


static char* etype_names[] = {
    "INVALID",
    "LOAD",
    "STORE",
    "EXCHANGE",
    "COMPARE_EXCHANGE",
    "MEMORY_FENCE",
    "DEPENDENT_READ",
    "BRANCH",
    "LONG_BRANCH",
    "FETCH_AND_ADD",
    "LFETCH",
    0
};

std::ostream& operator<<(std::ostream& o, const SYNTH_ETYPE& e)
{
    assert(e <  SYNTH_ETYPE_LAST);
    o << etype_names[e];
    return o;
}



static char* ipf_enum_names[] = {
   "INVALID",
   "NOPM",
   "NOPI",
   "NOPB",
   "NOPF",
   "XOR",
   "XOR_DEPENDENT",
   "LOAD",
   "STORE",
   "COMPARE_EXCHANGE",
   "EXCHANGE",
   "FENCE",
   "BRANCH",
   "LONG_BRANCH",
   "LIMM",
   "FMA",
   "COMPARE",
   "FETCH_AND_ADD",
   "LFETCH",
    0
};

std::ostream& operator<<(std::ostream& o, const SYNTH_IPF_ENUM& f)
{
    assert(f <  SYNTH_IPF_LAST);
    o << ipf_enum_names[f];
    return o;
}


////////////////////////////////////////////////////////////////////////////







