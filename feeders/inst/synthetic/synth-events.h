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

/* synth-events.h */
/* Mark Charney   <mark.charney@intel.com> */
/*$Id: synth-events.h 799 2006-10-31 12:27:52Z cjbeckma $ */


#ifndef _SYNTH_EVENTS_H_
# define _SYNTH_EVENTS_H_

#include "synth-types.h"
#include "synth-debug.h"
#include "synth-memop.h"
#include "value-history.h"
#include "synth-params.h"

class SYNTH_EVENT_CLASS; // forward decl
extern SYNTH_EVENT_CLASS* random_get_event(void); // prototype -- synthetic-feeder-int.cpp

class SYNTH_EVENT_CLASS
{
    // this generates one parameterized random address stream

    UINT64 cur_data_va;
    INT64  stride;
    UINT64 data_shift;

    /* limited data value history */
    VALUE_HISTORY_CLASS dvals;

    UINT64 address_space_mask;
    UINT32 max_share_loop;

    SYNTH_ETYPE etype;
    bool acquire;
    bool release;
    IPF_HINT_ENUM hint; // locality hint in ipf_raw_inst.h

    UINT32 fixed_ref_size_log;

    bool has_dependent_ops;
    UINT32 dependence_distance;
    
    SYNTH_PARAMS params;
    UINT64 threadUid;

    UINT64 previous_data_va;
    
  public:
    SYNTH_EVENT_CLASS(SYNTH_PARAMS p, UINT64 uid) // CONS
        : stride(p->getSynth_Data_Stride(uid)),
          dvals(p->getSynth_History_Size(uid)),
          address_space_mask(0),
          acquire(false),
          release(false),
          hint(IPF_HINT_NONE),
          params(p),
          threadUid(uid)
    {
        has_dependent_ops = false;
        dependence_distance = 0;

        // some parameter validation
        ASSERTX(params->getSynth_Depends_On_Load(threadUid) <= 100);
        ASSERTX(params->getSynth_Dependent_Range_Start(threadUid) <= params->getSynth_Dependent_Range_Stop(threadUid));

        ASSERTX(params->getSynth_Fixed_Size_Refs(threadUid) <= 8);
        ASSERTX(params->getSynth_Fixed_Spacing(threadUid)==0 || params->getSynth_Memop_Frequency(threadUid)>=1);
        if (params->getSynth_Fixed_Size_Refs(threadUid))
        {
            ASSERTX(count_ones(params->getSynth_Fixed_Size_Refs(threadUid)) == 1); //ensure power of 2-ness.
            fixed_ref_size_log = ilog(params->getSynth_Fixed_Size_Refs(threadUid));
        }
        ASSERTX(params->getSynth_Load_Pct(threadUid) <= 100);
        ASSERTX(params->getSynth_Acquire_Pct(threadUid) <= 100);
        ASSERTX(params->getSynth_Release_Pct(threadUid) <= 100);
        ASSERTX(params->getSynth_Locality(threadUid) <= 100);
        ASSERTX(params->getSynth_Sharing_Pct(threadUid) <= 100);
        if (SYNTH_THREADS == 1 && params->getSynth_Sharing_Pct(threadUid) > 0)
        {
            cerr << "You cannot do sharing when you have only one active thread!" << endl;
            exit(1);
        }

        ASSERTX(params->getSynth_Random_Threshold(threadUid) <= 100);
        ASSERTX(params->getSynth_Data_Alignment(threadUid) > 0);
        ASSERTX(SYNTH_SELF_SEED == 0 || SYNTH_SELF_SEED == 1);
        ASSERTX(params->getSynth_Data_Pattern(threadUid) == 0 || params->getSynth_Data_Pattern(threadUid) == 1);
        ASSERTX(params->getSynth_Fixed_Spacing(threadUid) == 0 || params->getSynth_Fixed_Spacing(threadUid) == 1);
        max_share_loop = 10 * SYNTH_THREADS;

        data_shift = count_ones(params->getSynth_Data_Alignment(threadUid)-1);
        cur_data_va = random_aligned_address(data_shift);
        previous_data_va = cur_data_va;
        if (SYNTH_DATA_SPACE)
        {
            address_space_mask = (UINT64) (1ULL << SYNTH_DATA_SPACE);
            address_space_mask--;
        }
    }

    SYNTH_ETYPE get_event_type(void) const
    {
        return etype;
    }

    SYNTH_ETYPE set_event_type(SYNTH_FTYPE f)
    {
        switch(f)
        {
          case  SYNTH_FTYPE_LOAD:
            select_acquire();
            return SYNTH_ETYPE_LOAD;

          case  SYNTH_FTYPE_STORE:
            select_release();
            return SYNTH_ETYPE_STORE;

          case  SYNTH_FTYPE_EXCHANGE:
            return SYNTH_ETYPE_EXCHANGE;

          case  SYNTH_FTYPE_COMPARE_EXCHANGE:
            select_acquire_or_release();
            return SYNTH_ETYPE_COMPARE_EXCHANGE;

          case  SYNTH_FTYPE_FETCH_AND_ADD:
            select_acquire_or_release();
            return SYNTH_ETYPE_FETCH_AND_ADD;

          case  SYNTH_FTYPE_LFETCH:
            return SYNTH_ETYPE_LFETCH;

          default:
            assert(0);
            break;
        }
    }
    void select_release(void)
    {
        if (params->getSynth_Release_Pct(threadUid) && random_pct() <= params->getSynth_Release_Pct(threadUid))
        {
            release = true;
        }
        else
        {
            release = false;
        }
    }
    void clear_acquire_release(void)
    {
        acquire = false;
        release = false;
    }
    void select_acquire(void)
    {
        if (params->getSynth_Acquire_Pct(threadUid) && random_pct() <= params->getSynth_Acquire_Pct(threadUid))
        {
            acquire = true;
        }
        else
        {
            acquire = false;
        }
    }
    void select_acquire_or_release(void)
    {
        acquire = false;
        /*
           we have conflicting requirements. These ops can either be
           acquires or releases, but not both. It must be one or the
           other. I decided to pick the larger percentage as the one that I
           follow for this situation. If you want something reasonable,
           specify either of SYNTH_AQUIRE_PCT or SYNTH_RELEASE_PCT = 0.
         */
        if (params->getSynth_Acquire_Pct(threadUid) > params->getSynth_Release_Pct(threadUid))
        {
            if (random_pct() < params->getSynth_Acquire_Pct(threadUid))
            {
                acquire = true;
            }
        }
        else
        {
            if (random_pct() > params->getSynth_Release_Pct(threadUid))
            {
                acquire = true;
            }
        }

        release = ! acquire;
    }

    void select_event_type(void)
    {
        //FIXME: add LFETCHes and compare-exchanges

        // select reads or writes 
        clear_hint();
        if (random_pct() > params->getSynth_Atomic_Pct(threadUid))
        {
            if (random_pct() > params->getSynth_Load_Pct(threadUid))
            {
                etype = SYNTH_ETYPE_STORE;
                select_release();
            }
            else
            {
                etype = SYNTH_ETYPE_LOAD;
                select_acquire();
            }
        }
        else
        {
            clear_acquire_release();
            etype = SYNTH_ETYPE_EXCHANGE;
        }
    }


    UINT64 pick_value(void)
    {
        UINT64 x = dvals.pick_value();
        return x;
    }

    UINT64 pick_shared(void)
    {
        SYNTH_EVENT_CLASS* other;
#if 1
        UINT i=0;
        do
        {
            other = random_get_event(); /* see synthetic-feeder-int.cpp */

            i++;
            if (i==max_share_loop)
            {
                //cerr << "bailing" << endl;
                return 0; // pick another value
            }
        }
        while (other == this);
#else
        other = random_get_event(); /* see synthetic-feeder-int.cpp */
#endif
        return other->pick_value();
    }

    ///////////////////////////////////////////////
    inline void
    set_dependent_ops(void)
    {
        if (random_pct() <= params->getSynth_Depends_On_Load(threadUid))
        {
            has_dependent_ops = true;

            /* pick a random number between SYNTH_DEPENDENT_RANGE_START and
            SYNTH_DEPENDENT_RANGE_STOP */
            if (params->getSynth_Dependent_Range_Start(threadUid) == params->getSynth_Dependent_Range_Stop(threadUid))
            {
                dependence_distance = params->getSynth_Dependent_Range_Start(threadUid);
            }
            else
            {
                const UINT32 offset =
                    random_in_range(params->getSynth_Dependent_Range_Stop(threadUid) -
                                    params->getSynth_Dependent_Range_Start(threadUid) + 1);
                dependence_distance = params->getSynth_Dependent_Range_Start(threadUid) + offset;
            }
        }
        else
        {
            has_dependent_ops = false;
            dependence_distance = 0;
        }
    }

    inline bool
    get_dependent_ops(void) const
    {
        return has_dependent_ops;
    }

    inline UINT32
    get_dependence_distance(void) const
    {
        return dependence_distance;
    }

    ///////////////////////////////////////////////
    inline void
    set_next_data_addr(void)
    {
        set_next_data_addr2();
        /*
         * offset the data address after updating all the tables.
         */
        cur_data_va += params->getSynth_Data_Offset(threadUid);
    }

    void
    set_next_data_addr2(void)
    {
        // pick the next data address
        
        // pick a new value, using locality?
        if ((params->getSynth_Locality(threadUid)>0) &&
            random_under_threshold(params->getSynth_Locality(threadUid)))
        {   // reuse an old value
            cur_data_va  = pick_value();
            if (cur_data_va != 0)
            {
                //FIXME: should we record the value again???
                return;
            }
            // not enough history yet --  fall through -- make a new value
        }

        // pick a new shared value?
        if (params->getSynth_Sharing_Pct(threadUid) &&
            random_under_threshold(params->getSynth_Sharing_Pct(threadUid)))
        {
            cur_data_va = pick_shared();

            // cerr << "pick shared returned "
            //      << hex << cur_data_va << dec << endl;

            if ( cur_data_va != 0 )
            {
                // record it for the future
                dvals.remember(cur_data_va); 
                return;
            }
        }

        // pick a strided value?
        if (params->getSynth_Data_Pattern(threadUid))
        {
            cur_data_va = previous_data_va + stride;
        }
        else // pick a random value.
        {
            cur_data_va = random_aligned_address(data_shift);
        }

        // mask the value down to our data footprint
        if (address_space_mask)
        {
            cur_data_va = cur_data_va & address_space_mask;
        }

         // record it for the future
        dvals.remember(cur_data_va);

        //cerr << "Updating vea " << hex << cur_data_va << endl;

        previous_data_va = cur_data_va;
    }

    void set_memop(SYNTH_MEMOP* m)
    {
        m->ea = get_va();
        m->sz = get_access_size();
        m->release = get_release();
        m->acquire = get_acquire();
        m->hint = get_hint();
        m->etype = get_event_type();

        if (get_dependent_ops())
        {
            m->set_dependence_distance( get_dependence_distance() );
            m->set_reg_dest( 0 /*FIXME*/ );
        }
        else /* nothing dependent */
        {
            m->set_dependence_distance( 0 );
            m->set_reg_dest( 0 );
        }
    }
    
    inline UINT64 get_va(void) const
    {
        return cur_data_va;

    }
    inline UINT32 get_source_register(void) const
    {
        return 1;
    }
    inline UINT64 get_intervening_instruction_count(void) const
    {
        if (params->getSynth_Fixed_Spacing(threadUid))
        {
            return params->getSynth_Memop_Frequency(threadUid)-1;
        }
        return 0;
    } 
    
    inline UINT32 get_access_size(void) const
    {
        if (params->getSynth_Fixed_Size_Refs(threadUid))
        {
            return fixed_ref_size_log;
        }           
        else
        {
            UINT32 sz = random_in_range(4); // values: {0,1,2,3}
            return sz;
        }
    }
    inline bool get_is_compare(void) const { return false; }


    ///////////////////////////////////////
    inline void
    clear_hint(void)
    {
        hint=IPF_HINT_NONE;
    }

    inline void
    set_hint(IPF_HINT_ENUM arg_hint)
    {
        hint=arg_hint;
    }

    inline IPF_HINT_ENUM
    get_hint(void) const
    {
        return hint;
    }
    ///////////////////////////////////////

    inline bool get_release(void) const { return release; }
    inline bool get_acquire(void) const { return acquire; }

    inline UINT32 get_branch_target_register(void) const {return 0; }
    inline bool get_branch_is_register_indirect(void) const { return false; }
    inline bool get_is_branch_conditional(void)  const { return false; }
    inline bool get_is_branch_taken(void)  const { return true; }
    inline UINT64 get_branch_target_ip(void)  const { return 0; }
};

typedef SYNTH_EVENT_CLASS *SYNTH_EVENT;

#endif

