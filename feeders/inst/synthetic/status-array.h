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

/* status-array.h */
/* Mark Charney   <mark.charney@intel.com> */
/*$Id: status-array.h 799 2006-10-31 12:27:52Z cjbeckma $ */


#ifndef _STATUS_ARRAY_H_
# define _STATUS_ARRAY_H_
#include "synth-headers.h"
#include "synth-debug.h"
////////////////////////////////////////////////////////////////////////////


typedef enum
{
    SYNCH_STATUS_INVALID,
    SYNCH_STATUS_ENTERING,
    SYNCH_STATUS_BLOCKED
} SYNCH_STATUS_ENUM;

std::ostream& operator<<(std::ostream& o, const SYNCH_STATUS_ENUM& s);

#define SYNCH_STATUS_HISTORY  10000

class SYNCH_STATUS_CLASS
{
  private:
    SYNCH_STATUS_ENUM array[ SYNCH_STATUS_HISTORY ];
    unsigned int iteration;
  public:
    SYNCH_STATUS_CLASS() // CONS
    {
        iteration = 0;
        zero_range(0, SYNCH_STATUS_HISTORY);
    }

    void
    zero_range(unsigned int start,
               unsigned int end)
    {
        unsigned int full_end;
        if (end < start)
        {
            full_end = end + SYNCH_STATUS_HISTORY;
        }
        else
        {
            full_end = end;
        }

        for ( unsigned int i=start ; i<full_end ;  i++ ) 
        {
            unsigned int j;
            if (i >= SYNCH_STATUS_HISTORY)
            {
                j = i - SYNCH_STATUS_HISTORY;
            }
            else
            {
                j = i;
            }
            array[j] =  SYNCH_STATUS_INVALID;
        }
    }

    inline void
    set_iter_count( unsigned int new_count )
    {
        if (new_count >= SYNCH_STATUS_HISTORY)
        {
            ASIMERROR("Overflowed the synch status array: " 
		      << new_count);
        }

        BMSG("set_iter_count new_count = " << new_count);
        //zero_range(new_count, iteration);

        iteration = new_count;
    }

    inline void
    inc_iter()
    {
        iteration++;
        if (iteration == SYNCH_STATUS_HISTORY)
        {
            iteration = 0;
        }
    }

    inline unsigned int
    get_iter() const
    {
        return iteration;
    }

    inline SYNCH_STATUS_ENUM
    get_elem(unsigned int idx) const
    {
        if (idx >= SYNCH_STATUS_HISTORY)
        {
            ASIMERROR("Overflowed the synch status array" << std::endl);
        }
        return array[idx];
    }

    inline bool
    entering(unsigned int idx) const
    {
        return get_elem(idx) == SYNCH_STATUS_ENTERING;
    }

    inline bool
    blocked(unsigned int idx) const
    {
        return get_elem(idx) == SYNCH_STATUS_BLOCKED;
    }
    
    inline bool
    invalid(unsigned int idx) const
    {
        return get_elem(idx) == SYNCH_STATUS_INVALID;
    }

    inline void
    set_blocked(unsigned int idx)
    {
        set_elem(idx, SYNCH_STATUS_BLOCKED);
    }

    inline void
    set_entering(unsigned int idx)
    {
        set_elem(idx, SYNCH_STATUS_ENTERING);
    }
    
    
    inline void
    set_elem(unsigned int idx, SYNCH_STATUS_ENUM s)
    {
        if (idx >= SYNCH_STATUS_HISTORY)
        {
            ASIMERROR("Overflowed the synch status array" << std::endl);
        }
        BMSG("Setting iteration element " << idx << " to " << s);
        array[idx] = s;
    }
    
    inline void
    clear_elem(unsigned int idx)
    {
        BMSG("clear_elem idx = " << idx);
        set_elem(idx, SYNCH_STATUS_INVALID);
    }
    
};





////////////////////////////////////////////////////////////////////////////
#endif
//Local Variables:
//pref: "status-array.cpp"
//End:
