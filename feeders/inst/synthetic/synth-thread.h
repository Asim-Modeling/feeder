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

/* synth-thread.h */
/* Mark Charney   <mark.charney@intel.com> */
/*$Id: synth-thread.h 799 2006-10-31 12:27:52Z cjbeckma $ */


#ifndef _SYNTH_THREAD_H_
# define _SYNTH_THREAD_H_
#include "asim/syntax.h"
#include "synth-types.h"
#include "synth-debug.h"
#include "synth-events.h"
#include "synth-inst.h"
#include "synth-routine.h"
#include "reg-provider.h"
#include "genable-inst.h"
#include "status-array.h"
////////////////////////////////////////////////////////////////////////////


// Here we have two maps:
//   ASIM_INST (really the uid) -> SYNTH_INST pointers
// and
//   ASIM_INST (really the uid) -> UWORD128* pointers (allocated for the map)

class ASIM_INST_TO_SYNTH_INST_MAP_CLASS
{
    //
    // This class maps ASIM_INSTs to the 128b values from SYNTH_INSTs for
    // use with the MVM.
    //
    map<UINT64, SYNTH_INST> asim_inst_to_synth_inst_map;
    typedef map<UINT64,SYNTH_INST>::iterator a2s_map_iter_t;

  private:

    inline UINT64
    get_uid(ASIM_INST inst) const
    {
        return inst->GetUid();
    }

  public:

    SYNTH_INST
    find(ASIM_INST inst)
    {
        return find(get_uid(inst));
    }

    SYNTH_INST
    find(UINT64 uid)
    {
        a2s_map_iter_t mi;
        a2s_map_iter_t m_end = asim_inst_to_synth_inst_map.end();
        
        mi = asim_inst_to_synth_inst_map.find( uid );
        if (mi !=  m_end)
        {
            SMSG("MAPA Find of " << uid << " succeeded. Pointing at  " << (*mi).second);
            return (*mi).second;
        }

        // not found
        SMSG("MAPA Find of " << uid << " failed.");
        return 0; 
    }

    void
    remove(UINT64 uid)
    {
        a2s_map_iter_t mi;
        a2s_map_iter_t m_end = asim_inst_to_synth_inst_map.end();

        mi = asim_inst_to_synth_inst_map.find(uid);
        if (mi !=  m_end)
        {
            SMSG("MAPA Erasing mapping for " << uid);

            // NOTE: WE DO NOT DELETE THE SYNTH_INST HERE!!!
            //delete (*mi).second; // THIS STATMENT SHOULD NOT BE UNCOMMENTED!!!!

            asim_inst_to_synth_inst_map.erase(mi);
        }
        else
        {
            // Not found. Issue warning?
            SMSG("MAPA Remove of " << uid << " failed.");
        }
    }


    void
    remember(ASIM_INST inst,
             SYNTH_INST si)
    {
        const UINT64 uid = get_uid(inst);
        SMSG("MAPA Mapping " << uid << " to " <<  si);
        asim_inst_to_synth_inst_map[uid] = si;
    }
}; /* class ASIM_INST_TO_SYNTH_INST_MAP_CLASS */

class ASIM_INST_TO_VALUE_MAP_CLASS
{
    //
    // This class maps ASIM_INSTs to the 128b values from SYNTH_INSTs for
    // use with the MVM.
    //
    map<UINT64, UWORD128*> asim_inst_to_value_map;
    typedef map<UINT64,UWORD128*>::iterator a2v_map_iter_t;

  private:

    inline UINT64
    get_uid(ASIM_INST inst) const
    {
        return inst->GetUid();
    }

  public:

    UWORD128*
    find(ASIM_INST inst)
    {
        a2v_map_iter_t mi;
        a2v_map_iter_t m_end = asim_inst_to_value_map.end();
        
        UINT64 uid = get_uid(inst);
        mi = asim_inst_to_value_map.find( uid );
        if (mi !=  m_end)
        {
            SMSG("MAPA Find of " << uid << " succeeded. Pointing at  " << (*mi).second);
            return (*mi).second;
        }

        // not found
        SMSG("MAPA Find of " << uid << " failed.");
        return 0; 
    }

    void
    remove(ASIM_INST inst)
    {
        a2v_map_iter_t mi;
        a2v_map_iter_t m_end = asim_inst_to_value_map.end();

        UINT64 uid = get_uid(inst);
        mi = asim_inst_to_value_map.find(uid);
        if (mi !=  m_end)
        {
            SMSG("MAPA Erasing mapping for " << uid);
            delete (*mi).second;
            asim_inst_to_value_map.erase(mi);
        }
        else
        {
            // Not found. Issue warning?
            SMSG("MAPA Remove of " << uid << " failed.");
        }
    }


    void
    remember(ASIM_INST inst,
             SYNTH_INST si)
    {
        const UINT64 uid = get_uid(inst);
        SMSG("MAPA Mapping " << uid << " to " <<  si);
        UWORD128* v = new UWORD128(si->value);
        asim_inst_to_value_map[uid] = v;
    }
}; /* class ASIM_INST_TO_VALUE_MAP_CLASS */

////////////////////////////////////////////////////////////////////////////


class SYNTH_THREAD_CLASS 
{
    // sequences events for passing back to ASIM.
    // receives random events from SYNTH_EVENT_CLASS event.
    // handles kills and replays.

  private:
    const UINT64      uid;
    const bool        using_synthetic;
    UINT64            preceeding_inst;

    IPF_RAW_BUNDLE    bundle;
    SYNTH_EVENT_CLASS event;    // provides the random events
    UINT32            wrong_path;

    /*used for terminating the thread in the controller. */
    IFEEDER_THREAD asim_ithread;     

    bool needJumpToNextPC;

    ASIM_INST_TO_SYNTH_INST_MAP_CLASS a2s_map;
    ASIM_INST_TO_VALUE_MAP_CLASS      a2v_map;

    list<SYNTH_INST> issued; // issued but not committed correct-path insts

    // For replaying things to ASIM, on mispredicts, etc.
    // and also for the VERY FIRST ISSUE of every syllable!
    list<SYNTH_INST> replay; 

    list<UINT64> pc_stack; // return stack for subroutines
    list<SYNTH_ROUTINE_CLASS*> path_stack; // return stack for subroutines

    SYNTH_ROUTINE_CLASS* current_path;
    SYNTH_ROUTINE_CLASS* main_path;
    SYNTH_ROUTINE_CLASS* second_path;

    

    IPF_RAW_BUNDLE junk_bundle;
    bool           have_junk;

    /* these are for handing out the destination registers for loads (and
       other memops that write registers) and xors */
    REGISTER_PROVIDER_CLASS load_dests; 
    REGISTER_PROVIDER_CLASS xor_dests;
    REGISTER_PROVIDER_CLASS fp_dests;

    /* used for pointer chasing */
    UINT32 last_dest_reg; 

    SYNTH_IPF_ENUM ipf_syllables[SYLLABLES_PER_BUNDLE];

    /* SYNTH_MEMOPs are the extra information used to generate
     * memory-oriented instructions in the synthetic feeder. */

    SYNTH_MEMOP memop[SYLLABLES_PER_BUNDLE];

    IPF_HINT_ENUM lfetch_hint; // FIXME: this belongs elsewhere

    class REG_COUNT_CLASS
    {
      public:
        UINT32 reg;
        UINT32 count;
    };

    list<REG_COUNT_CLASS> dest_reg_history;

    UINT32 correct_path_bundle_count;

    /* program is used for canned routines */
    GENABLE_PROGRAM_CLASS* program;

    SYNCH_STATUS_CLASS synch_iteration_status;

    SYNTH_PARAMS params;
    
    UINT64 num_committed_inst;
    
    //////////////////////////////////////////////////////////////////////////

  public:

    SYNTH_THREAD_CLASS(UINT64 arg_uid, SYNTH_PARAMS p, bool arg_using_synthetic=true); //CONS

    inline UINT32
    GetNumCommittedInst(void) const
    {
        return num_committed_inst;
    }
    
    inline UINT64
    get_tid(void) const // thread id
    {
        return uid;
    }

    ////////////////////////////////////////////////////////////////////
    GENABLE_INST_CLASS* get_ginst(IADDR_CLASS pc)
    {
        GENABLE_INST_CLASS* g = program->get_inst( pc.GetBundleAddr(),
                                                   pc.GetSyllableIndex() );
        return g;
    }

    bool in_range(IADDR_CLASS pc) const
    {
        return program->in_range( pc.GetBundleAddr(), pc.GetSyllableIndex() );
    }


    ////////////////////////////////////////////////////////////////////
    inline void
    inc_correct_path_bundle_count(void)
    {
        correct_path_bundle_count++;
    }

    inline UINT32
    get_correct_path_bundle_count(void) const
    {
        return correct_path_bundle_count;
    }

    /////////////////////////////////////////////////////////////////////////

    /* junk (filler) bundle handling */

    inline IPF_RAW_BUNDLE*
    GetJunkBundle(void)
    {
        return &junk_bundle;
    }
    inline bool
    have_a_junk_bundle(void) const
    {
        return have_junk;
    }
    inline void 
    set_junk_bundle(void)          
    {
        have_junk=true; 
    }
    inline void 
    clear_junk_bundle(void)        
    {
        have_junk=false; 
    }

    inline void 
    clear_preceeding_insts(void)
    {
        preceeding_inst = 0;
    }

    inline void 
    set_gap(void) // filler syllables between "interesting" syllables.
    {
        if (params->getSynth_Fixed_Spacing(uid))
        {
            XMSG("resetting gap");
            preceeding_inst = event.get_intervening_instruction_count();
        }
        else
        {
            clear_preceeding_insts();
        }
    }

    /////////////////////////////////////////////////////////////////////////
    inline IPF_HINT_ENUM 
    get_lfetch_hint(void) const
    {
        return lfetch_hint;
    }
    /////////////////////////////////////////////////////////////////////////

    /* pointer chasing and dependent-use operation support */

    inline void
    set_last_dest_reg(UINT32 dest_reg) 
    {
        last_dest_reg = dest_reg;
    }

    inline UINT32
    get_last_dest_reg(void) const 
    {
        return last_dest_reg;
    }

    //////////////////////////////////////////////
    
    inline UINT32
    recent_dest_reg_count(void) const
    {
        static const UINT32 a_large_number = UINT32_MAX;

        if ( ! dest_reg_history.empty() )
        {
            const REG_COUNT_CLASS& rc = dest_reg_history.front(); 
            return rc.count;
        }
        else
        {
            return a_large_number;
        }
    }
    
    
    inline void
    add_dest_reg_to_history(UINT32 reg, UINT32 dep_distance)
    {
        SMSG("adding ( "
             << reg 
             << ","
             << dep_distance
             << ") to recent dest reg list");

        REG_COUNT_CLASS rc;
        rc.reg = reg;
        rc.count = dep_distance;
        dest_reg_history.push_back(rc);
    }
    
    inline UINT32
    pop_dest_reg_from_history()
    {
        if ( ! dest_reg_history.empty() )
        {
            const REG_COUNT_CLASS& rc = dest_reg_history.front(); 
            dest_reg_history.pop_front();
            return rc.reg;
        }
        else
        {
            BMSG("ERROR: underflow register history");
            exit(1);
        }
    }
    
    
    ////////////////////////////////////////////////////////////////////

    /* thread uids and feeder uids */

    inline void
    set_asim_thread(IFEEDER_THREAD arg_thread)
    {
        asim_ithread = arg_thread;
    }

    inline IFEEDER_THREAD
    get_asim_thread(void)  const
    {
        return asim_ithread;
    }

    inline UINT64
    get_uid(void) const
    {
        return uid;
    }

    /////////////////////////////////////////////////////////////////////

    /* dead-man counter for limiting wrong path execution duration */

    inline void
    inc_wrong_path(void)
    {
        wrong_path++;
    }

    inline void
    not_wrong_path(void)
    {
        wrong_path=0;
    }

    inline UINT32
    get_wrong_path(void) const
    {
        return wrong_path;
    }
    /////////////////////////////////////////////////////////////////////
    
    inline SYNTH_EVENT
    get_event(void)
    {
        return &event; //FIXME: bad style 
    }


    inline SYNTH_MEMOP*
    get_memop(UINT32 syllable_idx)
    {
        assert(syllable_idx < SYLLABLES_PER_BUNDLE);
        return memop+syllable_idx;
    }

    //////////////////////////////////////////////////////////////////


    /* loop, path, routine support */

    inline void
    select_main_path(void)
    {
        current_path = main_path;
        clear_preceeding_insts();
    }
    inline void
    select_second_path(void)
    {
        current_path = second_path;
        second_path->reinit();
        clear_preceeding_insts();
    }

    inline UINT64
    get_loop_target(void) const
    {
        return current_path->get_start_pc();
    }

    inline void
    emitted_non_branch(void)
    {   // decrement counter when we emit non-branch ops
        current_path->emitted_non_branch();
    }
    inline void
    emitted_branch(void)
    {   // reset the counter
        current_path->emitted_branch();
    }
    inline bool
    need_branch_to_top(void) const
    {   
        return current_path->need_branch_to_top();
    }
    inline bool
    need_to_return(void) const
    {   
        return current_path->need_to_return();
    }

    inline bool
    at_branch(void) const
    {
        //FIXME: need the location of branches in each nonleaf routine
        return current_path->get_start_pc()
            + params->getSynth_Branch_Bundle_In_Routine(uid) * BYTES_PER_BUNDLE; 
        
    }

    inline bool
    test_need_jump(void) const
    {
        return needJumpToNextPC;
    }

    inline void
    set_need_jump(void)
    {
        needJumpToNextPC = true;
    }

    inline void
    clear_need_jump(void)
    {
        needJumpToNextPC = false;
    }

    inline bool
    need_to_branch_out(void) const
    {   
        return current_path->need_to_branch_out();
    }

    inline UINT64
    get_other_start_pc(void) const
    {
        return second_path->get_start_pc();
    }

    inline void
    push_current_path(void)
    {
        path_stack.push_front(current_path);
    }

    inline void
    pop_path(void)
    {
        ASSERTX(!path_stack.empty());
        current_path = path_stack.front();
        path_stack.pop_front();
    }
    
    inline UINT64
    get_return_pc(void) 
    {
        return pop_pc();
    }
    inline void
    push_pc(void)
    {
        pc_stack.push_front( current_path->get_pc() + BYTES_PER_BUNDLE );
    }
    inline UINT64
    pop_pc(void)
    {
        ASSERTX( !pc_stack.empty() );
        UINT64 p  = pc_stack.front();
        pc_stack.pop_front();
        return p;
    }


    //////////////////////////////////////////////////////////////////////

    /* bundles and syllable arrays */

    void
    set_syllables(SYNTH_IPF_ENUM* array)
    {
        for(UINT32 i=0;i<SYLLABLES_PER_BUNDLE; i++)
        {
            ipf_syllables[i] = array[i];
        }
    }
    inline SYNTH_IPF_ENUM
    get_syllable(UINT32 idx) const
    {
        ASSERTX(idx < SYLLABLES_PER_BUNDLE);
        return ipf_syllables[idx];
    }

    inline IPF_RAW_BUNDLE* 
    get_bundle(void)
    {
        return &bundle;
    }

    inline void 
    set_bundle(IPF_RAW_BUNDLE arg_bundle)
    {
        bundle=arg_bundle;
        set_syllable_idx(0);
    }

    //////////////////////////////////////////////////////////////////////


    /* program counter support */

    inline void 
    next_bundle()
    {
        /* advance the pc and make sure the syllable indx is at the end
           of the bundle, so we know to get a new thing */
        inc_pc();
        make_done();
    }
    
    inline void 
    set_pc(IADDR_CLASS arg_pc)
    {
        current_path->set_pc(arg_pc);
    }
    inline void 
    set_pc(UINT64 arg_pc)
    {
        current_path->set_pc(arg_pc);
    }

    inline UINT64 
    get_pc(void) const
    {
        return current_path->get_pc();
    }
    inline void 
    inc_pc(void)
    {
        current_path->inc_pc();
    }

    inline IADDR_CLASS
    get_ipf_pc(void) const
    {
        return current_path->get_ipf_pc();
    }
    
    inline void
    set_syllable_idx(UINT32 a=0)
    {
        XMSG("Setting syllable index to " << a);
        current_path->set_syllable(a);
    }

    inline UINT32
    get_syllable_idx(void) const
    {
        return current_path->get_syllable();
    }

    inline bool
    inc_syllable(void)
    {
        current_path->inc_syllable();
        SMSG("inc_syllable. New PC value= " << current_path->get_ipf_pc());
        if (done())
        {
            inc_pc();
            return true;
        }
        return false;
    }

    ///////////////////////////////////////////////////////////////////

    /* "done" with whatever support */

    inline bool
    done(void) const
    {
        return (get_syllable_idx()==SYLLABLES_PER_BUNDLE);
    }

    inline void
    make_done(void)
    {
        current_path->make_done();
        clear_preceeding_insts();
    }

    inline bool
    done_with_preceeding_inst(void) const
    {
        return (preceeding_inst == 0);
    } 
    inline void
    dec_preceeding_inst(void)
    {
        if (params->getSynth_Fixed_Spacing(uid) && preceeding_inst > 0)
        {
            preceeding_inst--;
            XMSG("Decrementing preceeding inst to " << preceeding_inst);
        }
    }

    inline bool
    done_with_entire_event(void) const
    {
        return done() && done_with_preceeding_inst();
    }

    //////////////////////////////////////////////////////////////////

    /* register destinations */
    
    inline UINT32
    get_next_fp_dest_reg(void)
    {
        return fp_dests.get_next_reg();
    }
    inline UINT32
    get_next_load_dest_reg(void)
    {
        return load_dests.get_next_reg();
    }
    inline UINT32
    get_next_xor_dest_reg(void)
    {
        return xor_dests.get_next_reg();
    }

    //////////////////////////////////////////////////////////////////

    /* issued list and replay list */
    

    inline list<SYNTH_INST>*
    get_issued_list(void)
    {
        return  &issued;
    }

    inline void
    issue(SYNTH_INST inst)
    {
        issued.push_back(inst);
    }


    ///////////////////////////////////////////////////
    inline void
    remember_si_value(ASIM_INST inst,
                      SYNTH_INST minst)
    {
        a2v_map.remember(inst,minst);
    }
    
    inline UWORD128* 
    find_si_value(ASIM_INST inst)
    {
        return a2v_map.find(inst);
    }
    
    inline void
    remove_si_value(ASIM_INST inst)
    {
        return a2v_map.remove(inst);
    }
    ///////////////////////////////////////////////////
    inline void
    remember_si(ASIM_INST inst,
                SYNTH_INST minst)
    {
        a2s_map.remember(inst,minst);
    }
    
    inline SYNTH_INST
    find_si(ASIM_INST inst)
    {
        return a2s_map.find(inst);
    }
    
    inline void
    remove_si(UINT64 uid)
    {
        //FIXX return a2s_map.remove(uid);
    }

    ////////////////////////////////////////////////////
    
    void
    delete_synth_inst(SYNTH_INST s);

    void
    commit_skipping_nops(ASIM_INST inst);

    void
    cond_replay_push_front_or_delete(SYNTH_INST p);
   
    void
    kill_and_replay_this_inst(UINT64 ref_uid);

    void
    copy_to_replay_queue(UINT64 ref_uid);
    

    /* on kills, we must find the minimum iteration count for a cmpxch and
       then jam it (minus one, because we pre-increment) into the status
       array for this thread. */
    void
    find_new_iteration_count(UINT64 ref_uid, bool killMe);


    // "older" insts are at the front of the list
    // "younger" insts are at the back of the list
    // use add_to_replay() to add a sequence of syllables, in order 0, 1 2.
    inline void
    add_to_replay(SYNTH_INST m)
    {
        replay.push_back(m);
    }

    inline bool
    no_replay_available(void) const
    {
        return replay.empty();
    }
    inline bool
    replay_available(void) const
    {
        return !replay.empty();
    }
    inline SYNTH_INST
    get_replay(void) const
    {
        return replay.front();
    }
    inline void
    pop_replay(void)
    {
        replay.pop_front();
    }

    bool
    insert_dependent_uses(SYNTH_IPF_ENUM* ipf_syllables);

    bool
    time_for_a_dependent_xor(void);


    ////////////////////////////////////////////////////////////////////

    inline void
    modify_synch_iteration_count ( unsigned int new_count )
    {
        synch_iteration_status.set_iter_count(new_count);
    }

    inline void
    inc_iteration_count()
    {
        BMSG("inc_iteration_count");
        synch_iteration_status.inc_iter();
        synch_iteration_status.clear_elem( synch_iteration_status.get_iter() );
    }

    inline unsigned int
    get_iteration_count() const
    {
        return synch_iteration_status.get_iter();
    }

    inline bool
    get_iteration_blocked(unsigned int iteration) const
    {
        return synch_iteration_status.blocked(iteration);
    }
    inline bool
    get_iteration_entering(unsigned int iteration) const
    {
        return synch_iteration_status.entering(iteration);
    }
    inline SYNCH_STATUS_ENUM
    get_iteration_status(unsigned int iteration) const
    {
        return synch_iteration_status.get_elem(iteration);
    }

    inline void
    set_iteration_entering(unsigned int iteration) 
    {
        synch_iteration_status.set_entering(iteration);
    }

    inline void
    set_iteration_blocked(unsigned int iteration) 
    {
        synch_iteration_status.set_blocked(iteration);
    }
    
    ////////////////////////////////////////////////////////////////////////

  private:

    IPF_HINT_ENUM
    convert_lfetch_hint(std::string s) const;


}; /* class SYNTH_THREAD_CLASS */

typedef SYNTH_THREAD_CLASS *SYNTH_THREAD;
///////////////////////////////////////////////////////////////////////

#endif





//Local Variables:
//pref: "synth-thread.cpp"
//pref2: "synth-inst.h"
//End:
