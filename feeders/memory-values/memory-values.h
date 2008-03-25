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


//
// @file    memory-values.h
// @author  Mark Charney
// @brief   A multiprocessor memory-value tracker
//

#ifndef _MEMORY_VALUE_MODEL_H_
#define _MEMORY_VALUE_MODEL_H_

#include <map>
#include <list>
#include <vector>
using namespace std;

// ASIM core
#include "asim/syntax.h"

// ASIM public modules
/* nada */



#define BITS_PER_BYTE 8

typedef UINT32 CPUID;
typedef UINT64 MVM_ADDR;
typedef UINT64 UID;
typedef UINT8 MVM_MEMOP_SIZE;
typedef UINT32 MVM_TIME_STAMP;

union UWORD
{
    UINT8 b[4];
    UINT16 h[2];
    UINT32 w;
    UWORD(UINT32 x=0) : w(x) {}

    friend ostream& operator<<(ostream& o, const UWORD& u);

};

typedef struct 
{
    UINT64 a[2];
} UINT128;

union UWORD128
{
    UINT8 b[16];
    UINT16 h[8];
    UINT32 w[4];
    UINT64 d[2];
    UINT128 q;

    UWORD128() //CONS
    {
        d[0]=0;
        d[1]=0;
    }

    UWORD128(const UWORD128& x) //CONS
    {
        q = x.q;
    }

    inline void set8(MVM_ADDR a, UINT8 v)
    {
        b[a & 0xF] = v;
    }
    inline void set16(MVM_ADDR a, UINT16 v)
    {
        h[a & 0x7] = v;
    }
    inline void set32(MVM_ADDR a, UINT32 v)
    {
        w[a & 0x3] = v;
    }
    inline void set64(MVM_ADDR a, const UINT64& v)
    {
        d[a & 0x1] = v;
    }
    inline void set128(const UINT128& v)
    {
        q = v;
    }

    friend ostream& operator<<(ostream& o, const UWORD128& u);
};

////////////////////////////////////////////////////////////////////////////

typedef enum
{
    MVM_LOAD,
    MVM_STORE,
    MVM_EXCHANGE,
    MVM_FETCH_ADD,
    MVM_MEMOP_LAST
} MVM_MEMOP_TYPE;

ostream& operator<<(ostream& o, const MVM_MEMOP_TYPE& t);


////////////////////////////////////////////////////////////////////////////
typedef enum
{
    MVM_UNINIT,
    MVM_LOCALLY_VISIBLE,  // for stores, value is locally visible
    MVM_GLOBALLY_VISIBLE, // for stores, value is globally visible
    MVM_VALUE_NOT_BOUND,  // for loads, no value available
    MVM_VALUE_BOUND,      // for loads, value is available, and set
    MVM_STATE_LAST
} MVM_STATE;


ostream& operator<<(ostream& o, const MVM_STATE& t);

////////////////////////////////////////////////////////////////////////////

class MVM_MEMOP
{
  public:

    MVM_MEMOP(MVM_MEMOP_TYPE arg_type, //CONS
              MVM_MEMOP_SIZE arg_size,
              CPUID          arg_processor,
              MVM_ADDR       arg_pa,
              UINT64         arg_asim_uid = 0)
        : uid(s_uid++),
          state(MVM_UNINIT),
          mvm_cares(false),
          feeder_cares(true)
    {
        type      = arg_type;
        size      = arg_size;
        processor = arg_processor;
        pa        = arg_pa;
        asim_uid  = arg_asim_uid;
        time_stamp = 0;
        dst_idx_start = pa & 0xF; // 0..15
        dst_idx_end = dst_idx_start + (size/BITS_PER_BYTE);
    }

    /////////////////////////////////////////////////
    //
    // simple reference counting support
    //
    inline void set_mvm_cares(void)
    {
        mvm_cares=true;
    }
    inline void mvm_does_not_care(void)
    {
        mvm_cares=false;
    }
    inline void feeder_does_not_care(void)
    {
        feeder_cares=false;
    }

    inline bool noone_cares(void) const
    {
        return feeder_cares == false && mvm_cares == false;
    }
    /////////////////////////////////////////////////
    
     
    inline bool subsumes(MVM_ADDR a, MVM_MEMOP_SIZE asz) const
    {
        // Does the region defined by (pa ... pa+size]
        //  contain or cover the region spanned by (a... a+asz]
        return (pa <= a && pa+size >= a+asz);
    }
         

    inline bool writes_memory(void) const
    {
        return type == MVM_STORE ||
               type == MVM_EXCHANGE ||
               type == MVM_FETCH_ADD;
    }

    ///////////////////////////////////////////////////////////
    inline void set_value128(const UWORD128& x)
    {
        value = x;
    }
    inline void set_value64(UINT64 x, MVM_ADDR a)
    {
        value.d[a&1] = x;
    }
    inline void set_value32(UINT32 x, MVM_ADDR a)
    {
        value.w[a&3] = x;
    }
    inline void set_value16(UINT16 x, MVM_ADDR a)
    {
        value.h[a&7] = x;
    }
    inline void set_value8(UINT8 x, MVM_ADDR a)
    {
        value.b[a&0xF] = x;
    }

    /////////////////////////////////////////////////
    // set using implicit address
    inline void set_value64(UINT64 x)
    {
        value.d[pa&1] = x;
    }
    inline void set_value32(UINT32 x)
    {
        value.w[pa&3] = x;
    }
    inline void set_value16(UINT16 x)
    {
        value.h[pa&7] = x;
    }
    inline void set_value8(UINT8 x)
    {
        value.b[pa&0xF] = x;
    }

    ///////////////////////////////////////////////////////////

    inline UINT8 get_value8(MVM_ADDR a) const
    {
        return value.b[a&0xF];
    }
    inline UINT16 get_value16(MVM_ADDR a) const
    {
        return value.h[a&0x7];
    }
    inline UINT32 get_value32(MVM_ADDR a) const
    {
        return value.w[a&0x3];
    }
    inline UINT64 get_value64(MVM_ADDR a) const
    {
        return value.d[a&0x1];
    }
    inline const UWORD128& get_value(void) const
    {
        return value;
    }

    /////////////////////////////////////////////////////////

    inline bool visible(CPUID i) const
    {
        return  (globally_visible() || locally_visible(i));
    }

    inline bool globally_visible(void) const
    {
        return (get_state() == MVM_GLOBALLY_VISIBLE);
    }


    inline bool locally_visible(CPUID i) const
    {
        return  (get_cpuid() == i &&
                 get_state() == MVM_LOCALLY_VISIBLE);
    }


    inline void stamp(void) { time_stamp=s_time_stamp++; }
    inline MVM_TIME_STAMP get_time_stamp(void) const { return time_stamp; }
    inline UID get_uid(void) const             { return uid; }
    inline void set_asim_uid(UINT64 arg_asim_uid) { asim_uid = arg_asim_uid; }
    inline UINT64 get_asim_uid(void) const     { return asim_uid; }
    inline MVM_STATE get_state(void) const     { return state; }
    inline MVM_MEMOP_TYPE get_type(void) const { return type; }
    inline MVM_MEMOP_SIZE get_size(void) const { return size; }
    inline CPUID get_cpuid(void) const         { return processor; }
    inline MVM_ADDR get_addr(void) const       { return pa; }

    void set_state(MVM_STATE s);
    
    friend ostream& operator<<(ostream& o, const MVM_MEMOP& m);

  private:

    static UID     s_uid; /* for handing out uids */
    UID            uid;
    UINT64         asim_uid; // for the corresponding ASIM_INST
    MVM_TIME_STAMP  time_stamp;
    static MVM_TIME_STAMP  s_time_stamp;
    MVM_STATE      state;
    MVM_MEMOP_TYPE type;
    MVM_MEMOP_SIZE size; // in bits
    CPUID          processor;
    MVM_ADDR       pa;

    /* values are 128bit aligned in "value" on 8bit boundaries */
    UWORD128       value;

    UINT32 dst_idx_start;
    UINT32 dst_idx_end;

    /* garbage collection */
    bool mvm_cares;
    bool feeder_cares;
};




////////////////////////////////////////////////////////////////////////////



typedef list<MVM_MEMOP*> MVM_MEMOP_LIST;


////////////////////////////////////////////////////////////////////////////
#define NPROC 32 /*FIXME*/
class MEM_CELL
{
    /* this is what the MVM uses to track memory values at different
       locations in the machine. It knows about the local values on each
       processor and the global values, if any */

  public:
    MEM_CELL()
        : has_global_value(0),
          local_values(NPROC, (MVM_MEMOP_LIST*)0)
    {
    }

    void set_global_value(const UWORD128& u, MVM_ADDR a, MVM_MEMOP_SIZE sz)
    {
        MVM_ADDR idx;

        switch(sz)
        {
          case 8:
            idx = a & 0xF;
            global_value.b[idx] = u.b[idx]; 
            has_global_value |= (1 << idx); // shift by 0, 1, 2 or 3
            break;

          case 16:
            idx = (a & 0x7);
            global_value.h[idx] = u.h[idx]; 
            has_global_value |= (3 << (idx*2)); 
            break;

          case 32:
            idx = (a & 0x3);
            global_value.w[idx] = u.w[idx]; 
            has_global_value |= (0xF << (idx*4)); 
            break;

          case 64:
            idx = (a & 0x1);
            global_value.d[idx] = u.d[idx]; 
            has_global_value |= (0xFF << (idx*8)); 
            break;

          case 128:
            global_value = u;
            has_global_value = 0xFFFF;
            break;

          default:
            assert(0);
            break;
        }
    }

    bool has_byte(MVM_ADDR a) const
    {
        unsigned int shift = a & 0xF;
        return (has_global_value >> shift) & 0x1;
    }


    UWORD128 global_value;
    UINT16 has_global_value; // bit vector

    /* there can be several locally visible values per processor */
    vector<MVM_MEMOP_LIST*> local_values;
};
////////////////////////////////////////////////////////////////////////////


class MEMORY_VALUE_MODEL_CLASS 
{
  public:

    MEMORY_VALUE_MODEL_CLASS(int arg_nprocessors); //CONS
    ~MEMORY_VALUE_MODEL_CLASS();


    /* The performance model tells the feeder to call these when the
     * loads/stores get to the appropriate point in the pipeline. */


    /* the MVM_MEMOP*'s for stores get held on to and deleted by the
       MVM when they become globally visible. */

    void read_value_now(MVM_MEMOP* m);
    /* the value is bound to the load */

    void store_locally_visible_now(MVM_MEMOP* m);

    void killed(MVM_MEMOP* m);
    /* removes local visibility and supporting info */

    void store_globally_visible_now(MVM_MEMOP* m);

  private:

    const UINT32 nprocessors;

    map<MVM_ADDR,MEM_CELL*> memops;
    
    inline MEM_CELL* get_cell(MVM_ADDR a)
    {
        if (memops.find(a) != memops.end())
        {
            return memops[a];
        }
        return 0;
    } 

    inline void store_cell(MVM_ADDR a, MEM_CELL* c)
    {
        memops[a] = c;
    } 

};

#endif /*_MEMORY_VALUE_MODEL_H_*/

