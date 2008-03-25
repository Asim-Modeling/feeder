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


#ifndef _VALUE_HISTORY_H_
# define _VALUE_HISTORY_H_

#include "synth-util.h"

class VALUE_HISTORY_CLASS
{
    /* for implementing locality and sharing */

    UINT32  p;
    UINT32  lim;
    UINT64* data_va;
  public:
    VALUE_HISTORY_CLASS(UINT32 values=16) // must be a power of 2
        : p(0), lim(values)
    {
        ASSERTX(count_ones(values) == 1); //ensure power of 2-ness.
        ASSERTX(lim > 1);
        data_va = new UINT64[lim];
        for(UINT32 i=0;i<lim;i++)
        {
            data_va[i] = 0;
        }
    }

    ~VALUE_HISTORY_CLASS()
    {
        delete [] data_va;
    }

    void remember(UINT64  v)
    {
        data_va[p] = v;
        p++;
        if (p==lim)
        {
            p=0;
        }
    }

    UINT64 pick_value(void) const {
        UINT32 i =  random_in_range(lim);
        ASSERTX(i<lim);
        return data_va[i];
    }
};

#endif

