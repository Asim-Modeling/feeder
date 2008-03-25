/***************************************************************************
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
****************************************************************************/
 
/**
 * @file  memory-values.cpp
 * @author Mark Charney
 *
 * @brief   A multiprocessor memory-value tracker
 *
 */

// generic
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <list>
#include <ostream>
#include <iomanip>

// ASIM core
#include "asim/syntax.h"

// ASIM public modules

using namespace std;

// ASIM local module
#include "memory-values.h"


UID  MVM_MEMOP::s_uid = 0; /* for handing out uids */
MVM_TIME_STAMP MVM_MEMOP::s_time_stamp= 2; /* for handing out stamps*/
////////////////////////////////////////////////////////////////////////

static UINT32 debug_memory_value_model = 1; // turn on debugging messages

#define MVM_TRACE (TRACEP(Trace_Sys))
//#define MVM_TRACE 1

#define CMSG(c,x) ({ \
   if (c) \
   { \
           cout <<  x << endl; \
   }\
})

#define TMSG(x) ({ \
   CMSG( (debug_memory_value_model && MVM_TRACE) , x ); \
})

#define XMSG(x) ({ \
   CMSG(debug_memory_value_model, x ); \
})

#define FIXME() ({ \
   cout << "FIXME: " << __FILE__ << ":" << __LINE__ << endl;  \
   assert(0); \
 })

#define MSG(x) ({ \
   cout <<  "        " \
        << __FILE__ << ":" << __LINE__ << ": " <<  x << endl; \
 })


////////////////////////////////////////////////////////////////////////////
//const static UINT32 BITS_PER_BYTE = 8;
static const UINT32 BYTES_PER_WORD = 4;
static const UINT32 BITS_PER_WORD = BITS_PER_BYTE * BYTES_PER_WORD;

UINT32 mask_128b(MVM_ADDR a) // get 128b aligned address
{
    return a & ~((UINT32)0xF);
}

UINT32 index_128b(MVM_ADDR a) // get 128b offset/index
{
    return a & (MVM_ADDR)0xF;
}

void
MVM_MEMOP::set_state(MVM_STATE s)
{
    assert(s != state);
    state = s;
}
///////////////////////////////////////////////////////////////////////////////


MEMORY_VALUE_MODEL_CLASS::MEMORY_VALUE_MODEL_CLASS(int arg_nprocessors) //CONS
    : nprocessors(arg_nprocessors)
{
}

MEMORY_VALUE_MODEL_CLASS::~MEMORY_VALUE_MODEL_CLASS() 
{
}

////////////////////////////////////////////////////////////////////////////

/* The performance model tells the feeder to call these when the
 * loads/stores get to the appropriate point in the pipeline. */


const MVM_TIME_STAMP  MAX_STAMP = ~(0UL);

#define BYTES_PER_128_BITS 16

class MVM_TRACKER_CLASS
{
  public:
    MVM_TRACKER_CLASS(MVM_ADDR a, MVM_MEMOP_SIZE sz) //CONS
    {
        init_stamps(a,sz);
    }

    inline bool done(void) const
    {
        return rqd == 0;
    }
    inline void global_basis(UINT8 global_bytes)
    {
        /* set the bits for which we have a globally visible value */

        for(UINT32 i=0;i<BYTES_PER_128_BITS;i++)
        {
            if (global_bytes & (1<<i))
            {
                if (should_stamp_idx(i,1))
                {
                    stamp_idx(i,1);
                }
            }
        }
    }

    void stamp_idx(UINT32 idx, MVM_TIME_STAMP s) {
        assert(s>0);
        assert(idx >= first_req);
        assert(idx <= last_req);
        assert(stamps[idx] < s);
        if (stamps[idx] == 0)
        {
            assert(rqd > 0);
            rqd--;
        }
        stamps[idx] = s;
    }

    void stamp(MVM_ADDR a , MVM_TIME_STAMP s) {
        const UINT32 idx = index_128b(a);
        stamp_idx(idx,s);
    }

    bool should_stamp_idx(UINT idx, MVM_TIME_STAMP s) const
    {
        // true iff we should update the stamp with the new value s
        assert(s>0);
        if (idx >= first_req)
        {
            if (idx <= last_req)
            {
                if (stamps[idx] < s)
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool should_stamp(MVM_ADDR a, MVM_TIME_STAMP s) const
    {
        // true iff we should update the stamp with the new value s
        const UINT idx = index_128b(a);
        return should_stamp_idx(idx,s);
    }

    void init_stamps(MVM_ADDR a, MVM_MEMOP_SIZE sz)
    {
        // mark the stamps as zero where we need a store

        first_req = index_128b(a);
        rqd = sz / BITS_PER_BYTE;
        last_req = first_req + rqd;

        for(unsigned int i = 0 ; i < BYTES_PER_128_BITS; i++ )
        {
            stamps[ i ] = MAX_STAMP;
        }
        for(unsigned int i = 0 ; i < rqd ; i++ )
        {
            stamps[ first_req + i ] = 0;
        }
    }

  private:

    UINT32 rqd;
    UINT32 first_req;
    UINT32 last_req;

    MVM_TIME_STAMP stamps[BYTES_PER_128_BITS];
};








void
MEMORY_VALUE_MODEL_CLASS::read_value_now(MVM_MEMOP* m)
{
    /* the value is bound to the load */
    /*
      find the last globally visible store to this address or last
      locally visible store on the same processor.
    */
    const MVM_ADDR       a     = m->get_addr();
    const MVM_ADDR       ma    = mask_128b(a); // get 128b aligned address
    const MVM_MEMOP_SIZE size  = m->get_size();
    const CPUID          cpuid = m->get_cpuid();
    MVM_TRACKER_CLASS    tracker(a, size);

    MEM_CELL* cell = get_cell(ma);
    if (cell)
    {
        // start with the global value and modify it
        m->set_value128(cell->global_value);

        tracker.global_basis(cell->has_global_value);

        MVM_MEMOP_LIST* lst = cell->local_values[cpuid];
        if (lst)
        {
            list< MVM_MEMOP* >::iterator i = lst->begin();
            for( ; i != lst->end() ; i++) // walk through the list of stores
            {
                // Must find most recent store at each BYTE in this word
                const MVM_MEMOP* st = *i;
                const MVM_TIME_STAMP new_stamp = st->get_time_stamp();
                
                const unsigned int st_sz = st->get_size()/BITS_PER_BYTE;
                const MVM_ADDR st_a = st->get_addr();

                // loop over the "src" store indices
                for(UINT32 idx = 0;  idx < st_sz; idx++)
                {
                    const MVM_ADDR st_i = st_a + idx;

                    if (tracker.should_stamp(st_i , new_stamp))
                    {
                        tracker.stamp( st_i,
                                       new_stamp );
                        m->set_value8( st->get_value8(st_i),
                                       st_i );
                    }
                    
                }
            } /* for loop over all stores to this 32b region */
        }
        if (tracker.done())
        {
            m->set_state(MVM_VALUE_BOUND);
            return;
        }
    }

    // there was no fully-bound value
    m->set_state(MVM_VALUE_NOT_BOUND);
    return;

} /* read_value_now */

void
MEMORY_VALUE_MODEL_CLASS::store_locally_visible_now(MVM_MEMOP* m)
{
    /* remove any other locally visible stores subsubed by this store on
     * this processor */

    const MVM_ADDR       a     = m->get_addr();
    const MVM_ADDR       ma    = mask_128b(a); // get 128b aligned address

    MEM_CELL* cell = get_cell(ma); // stores list at each 128b address
    const CPUID cpuid = m->get_cpuid();
    MVM_MEMOP_LIST* lst;
    if (cell)
    {
        lst  = cell->local_values[cpuid];
        if (!lst)
        {
            lst  = new MVM_MEMOP_LIST;
            cell->local_values[cpuid] = lst;
        }
    }
    else
    {
        // make cell
        cell = new MEM_CELL;
        store_cell(ma, cell);
        lst  = new MVM_MEMOP_LIST;
        cell->local_values[cpuid] = lst;
    }
    lst->push_front(m);
    m->stamp();
    m->set_mvm_cares();
}

void
MEMORY_VALUE_MODEL_CLASS::killed(MVM_MEMOP* m)
{
    /* removes local visibility and supporting info */
    const MVM_ADDR  a   = m->get_addr();
    const MVM_ADDR  ma  = mask_128b(a); // get 128b aligned address
    MEM_CELL* cell = get_cell(ma); // stores list at each 128b address

    if (cell)
    {
        const CPUID cpuid = m->get_cpuid();
        MVM_MEMOP_LIST* lst = cell->local_values[cpuid];
        if (lst)
        {
            lst->remove(m);
        }
    }
}

void
MEMORY_VALUE_MODEL_CLASS::store_globally_visible_now(MVM_MEMOP* m)
{
    const MVM_ADDR       a     = m->get_addr();
    const MVM_ADDR       ma    = mask_128b(a); // get 128b aligned address

    MEM_CELL* cell = get_cell(ma); // stores list at each 128b address
    if (!cell)
    {
        // make cell
        cell = new MEM_CELL;
        store_cell(ma, cell);
    }
    else
    {
        const CPUID cpuid = m->get_cpuid();
        MVM_MEMOP_LIST* lst = cell->local_values[cpuid];
        /* remove locally visible value from this cpu */
        if (lst)
        {
            lst->remove(m);
        }
    }

    cell->set_global_value(m->get_value(), m->get_addr(), m->get_size());

    m->mvm_does_not_care();
    if (m->noone_cares())
    {
        delete m;
    }
    
}


////////////////////////////////////////////////////////////////////////////

ostream& operator<<(ostream& o, const MVM_MEMOP_TYPE& t)
{
    switch(t)
    {
      case MVM_LOAD:
        o << "load";
        break;

      case MVM_STORE:
        o << "store";
        break;

      case MVM_EXCHANGE:
        o << "exchange";
        break;

      case MVM_FETCH_ADD:
        o << "fetch-and-add";
        break;

      default:
        o << "???";
        break;
    }
    return o;
}

ostream& operator<<(ostream& o, const MVM_STATE& t)
{
    switch(t)
    {
      case MVM_UNINIT:
        o << "uninitialized";
        break;

      case MVM_LOCALLY_VISIBLE:
        o << "locally-visible";
        break;

      case MVM_GLOBALLY_VISIBLE:
        o << "globally-visible";
        break;

      case MVM_VALUE_NOT_BOUND:
        o << "not-bound";
        break;

      case MVM_VALUE_BOUND:
        o << "bound";
        break;

      default:
        o << "???";
        break;
    }
    
    return o;
}

ostream& operator<<(ostream& o, const UWORD& u)
{
    o << hex
      << setw(8)
      << u.w
      << dec;
    return o;
}

ostream& operator<<(ostream& o, const UWORD128& u)
{
    o << hex;
    for(int i=0; i<4; i++)
    {
        o << setw(8)
          << u.w[i]
          << " ";
    }
    o << dec;
    return o;
}

ostream& operator<<(ostream& o, const MVM_MEMOP& m)
{
    o << "uid: " << m.uid
      << " state: " << m.state
      << " type: " << m.type
      << " size: " << (UINT32) m.size
      << " pa: " << hex << m.pa << dec
      << " value: ";

    if (m.state == MVM_VALUE_NOT_BOUND || m.state == MVM_UNINIT)
    {
        o << "???";
    }
    else
    {
        o << m.value;
    }

    return o;
}
///////////////////////////////////////////////////////////////////////////////
//End of file
///////////////////////////////////////////////////////////////////////////////

