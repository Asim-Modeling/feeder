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

// synth-util.cpp
// Mark Charney   <mark.charney@intel.com>
// $Id: synth-util.cpp 799 2006-10-31 12:27:52Z cjbeckma $


////////////////////////////////////////////////////////////////////////////
#include "asim/syntax.h"
#include "asim/provides/instfeeder_interface.h"
#include "asim/provides/instfeeder_implementation.h" 
#include "synth-util.h"

UINT64
mask_bundle_addr(UINT64 x)
{
    return (x & BUNDLE_ADDR_MASK);
}

bool
even_bundle(IADDR_CLASS pc)
{
    const UINT32 ODD_BUNDLE_ADDR = 16;
    if ((pc.GetBundleAddr() & ODD_BUNDLE_ADDR) == ODD_BUNDLE_ADDR)
    {
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////
// RANDOM NUMBER GENERATION
////////////////////////////////////////////////////////////////////////////



void
init_random_number_generator(void)
{
    long int seed = SYNTH_SEED;
    if (SYNTH_SELF_SEED)
    {
        seed = (long int)getpid();
        cout << "Synthetic trace feeder seeding itself with " << seed << endl;
    }
    srand48(seed);
}


double
random_pct(void)
{
    return (drand48()* 100.0);
}

bool
random_over_threshold(UINT32 thresh)
{
    return ((100.0*drand48()) > thresh);
}

bool
random_under_threshold(UINT32 thresh)
{
    return ((100.0*drand48()) <= thresh);
}


UINT64
random_aligned_address(UINT64 shift)
{
    UINT64 v = ((UINT64)lrand48()) << shift;
    return v;
}

UINT32
random_in_range(UINT32 x)    // x must be greater than 1.
{
    UINT32 v =(UINT32) (drand48()*x);
    if (v == x)
    {
        v=x-1;
    }
    return v;
}



////////////////////////////////////////////////////////////////////////////


UINT32
ilog(UINT32 arg)
{
    UINT32 i=0;
    UINT32 a=arg;
    assert(a > 0);
    while(a)
    {
        i++;
        a = a >> 1;
    }
    return i-1;
}
        
bool
is_power_of_2(UINT32 x)
{
    return ( (1U << ilog(x))  == x );
}



UINT32
count_ones(UINT32 x)
{
    UINT32 c = 0;
    while(x!=0)
    {
        if (x&1)
        {
            c++;
        }
        x = x>>1;
    }
    return c;
}

////////////////////////////////////////////////////////////////////////////
