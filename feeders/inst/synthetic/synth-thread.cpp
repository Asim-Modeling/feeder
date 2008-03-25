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

// synth-thread.cpp
// Mark Charney   <mark.charney@intel.com>
// $Id: synth-thread.cpp 799 2006-10-31 12:27:52Z cjbeckma $


#include "synth-thread.h"

SYNTH_THREAD_CLASS::SYNTH_THREAD_CLASS( UINT64 arg_uid,
                                        SYNTH_PARAMS p,
                                        bool arg_using_synthetic ) //CONS
    : uid(arg_uid),
      using_synthetic(arg_using_synthetic),
      preceeding_inst(0),
      event(p, uid),
      wrong_path(0),
      have_junk(false),
      load_dests(25,127), // register range for load dest regs.
      xor_dests(5,24),    // register range for xor dest regs.
      fp_dests(4,127),   // register range for fp dest regs.
      num_committed_inst(0)
{        
    
    params = p;
            
    set_need_jump();
    last_dest_reg = 127; // just start somewhere for ptr chasing
    
    lfetch_hint = convert_lfetch_hint(params->getSynth_Lfetch_Hint(uid));
    
    const UINT64 code_shift = count_ones(BYTES_PER_BUNDLE-1); 
    const UINT64 start_pc1 = random_aligned_address(code_shift); 
    
    const UINT64 start_pc2 =
        start_pc1 + params->getSynth_Routine_Offset(uid);
    //random_aligned_address(code_shift); 
    
    const UINT32 static_len_syllables1 = params->getSynth_Static_Syllabes_Per_Routine(uid);
    const UINT32 static_len_syllables2 = params->getSynth_Static_Syllabes_Per_Routine(uid);
    const UINT32 dynamic_len_syllables1 = 0; // implies infinite
    const UINT32 dynamic_len_syllables2 = params->getSynth_Dynamic_Syllabes_Per_Routine(uid); 
    
    const bool leaf = true;
    
    main_path = new SYNTH_ROUTINE_CLASS(params->getSynth_Branch_Off_Path_Pct(uid),
                                        start_pc1,
                                        static_len_syllables1,
                                        dynamic_len_syllables1,
                                        !leaf);
    
    second_path = new SYNTH_ROUTINE_CLASS(params->getSynth_Branch_Off_Path_Pct(uid),
                                          start_pc2,
                                          static_len_syllables2,
                                          dynamic_len_syllables2,
                                          leaf);
    select_main_path();
    
    correct_path_bundle_count=0;

    if (! using_synthetic )
    {
        
        int holding_delay_bundles = params->getSynth_Synch_Hold_Delay(uid);
        int reentry_delay_bundles = params->getSynth_Synch_Retry_Delay(uid);

        program = 
            make_synchronization_routine( start_pc1,
                                          holding_delay_bundles,
                                          reentry_delay_bundles );
    }
} /* SYNTH_THREAD_CLASS */


bool
SYNTH_THREAD_CLASS::time_for_a_dependent_xor(void)
{
    const UINT32 bundle_count =  recent_dest_reg_count();
    SMSG("time_for_a_dependent_xor: bundle_count = " << bundle_count);

    // equality would be correct, the less-than-or-equal is for robustness
    return (bundle_count <= get_correct_path_bundle_count());
}

bool
SYNTH_THREAD_CLASS::insert_dependent_uses(SYNTH_IPF_ENUM* ipf_syllables)
{
    SYNTH_EVENT event = get_event();
    if (event->get_dependent_ops())
    {
        if (time_for_a_dependent_xor())
        {
            for(UINT32 i=0;i<SYLLABLES_PER_BUNDLE;i++)
            {
                if (ipf_syllables[i] == SYNTH_IPF_XOR)
                {
                    SMSG("Making a dependent XOR");
                    ipf_syllables[i] = SYNTH_IPF_XOR_DEPENDENT;
                    return true;
                }
            }
        }
    }
    return false;
}


////////////////////////////////////////////////////////////////////////////
void
SYNTH_THREAD_CLASS::delete_synth_inst(SYNTH_INST s)
{
    SMSG("\t\tRemoving SYNTH_INST uid = " << s->uid);
    if (s->uid != magic_uid)
    {
        remove_si(s->uid);
    }
    delete s;
}

void
SYNTH_THREAD_CLASS::copy_to_replay_queue(UINT64 ref_uid)
{
    SMSG("Copying issued insts younger than " << ref_uid
         << " to replay queue.");
    // copy everything younger than inst to the replay queue.
    if (!issued.empty())
    {
        SYNTH_INST p = issued.back();
        while(p->GetUid() > ref_uid)
        {
            // note: we are copying instructins in the
            // reverse order!!
            cond_replay_push_front_or_delete(p);
            issued.pop_back(); // remove youngest thing (p) on issued list.
            p = issued.back();
            assert(p);
        }
    }
    else
    {
        SMSG("The issued queue was empty on a kill.");
    }
        
}
void
SYNTH_THREAD_CLASS::kill_and_replay_this_inst(UINT64 ref_uid) 
{
    SMSG("Trying to killing and replay uid = " <<ref_uid);
    if (!issued.empty())
    {
        /* there are things that we've sent into the pipe */

        /* look at the very last thing we sent in to the pipe... */
        SYNTH_INST p = issued.back();
        if (p->GetUid() == ref_uid) //FISH
        {
            SMSG("     Killing and replaying uid = " << p->GetUid());
            cond_replay_push_front_or_delete(p);
            issued.pop_back();
            return;
        }
        else
        {
            /* something other than the last (youngest) thing we sent
             * into the pipe got killed ... */

            SMSG("Tried to kill the non-youngest thing.");
            SMSG("That's okay on wrong-path instructions or db-full exception");
            SMSG("     Issued list:");

            /* NOTE: at this point, we have already "killed" all the
               younger things */

            /* start looking at the oldest thing we sent into the pipe */
            SYNTH_INST_ITER it = issued.begin();
            while( it != issued.end())
            {
                SYNTH_INST p = *it;
                SMSG("     uid = " << p->GetUid());
                if (p->GetUid() == ref_uid)
                {
                    SMSG("Oops. Found it in the list. FIXME!\n");
                    issued.erase(it++);
                    cond_replay_push_front_or_delete(p);
                    return;
                }
                it++;
            }
        }
    }
    else
    {
        SMSG("The issued queue was empty on a kill-me.");
        // that's okay for wrong-path stuff.
    }
}


void
SYNTH_THREAD_CLASS::cond_replay_push_front_or_delete(SYNTH_INST p)
{
    if (using_synthetic)
    {
        SMSG("     Copy to replay uid = " << p->GetUid());
        replay.push_front(p);
    }
    else
    {
        /* The issued list DOES NOT get copied to the front of the
         * replay list for canned programs. */

        SMSG("     Deleting uid = " << p->GetUid());

        delete_synth_inst(p);
    }
}


void
SYNTH_THREAD_CLASS::find_new_iteration_count(UINT64 ref_uid, bool killMe)
{
    /* on kills, we must find the minimum iteration count for a cmpxch and
       then jam it (minus one, because we pre-increment) into the status
       array for this thread. */

    unsigned int min_count = 0;
    bool first = true;

    
    SYNTH_INST_ITER i = issued.begin();
    while( i != issued.end() )
    {
        SYNTH_INST p = *i;

        // Throttle this to the inst getting killed and all younger stuff (which
        //  is also killed).

        if (p->uid > ref_uid || (killMe && p->uid == ref_uid))
        {
            
            if ( p->type == SYNTH_IPF_COMPARE_EXCHANGE ||
                 p->type == SYNTH_IPF_LOAD )
            {
                if (first || min_count > p->iteration)
                {
                    min_count = p->iteration;
                    first = 0;
                }
            }
            
        }
        i++;
    }
    
    if (!first)
    {
        SMSG("tid= " << uid
             << " Resetting iteration count to " << min_count - 1
             << " [was " << get_iteration_count() << "]" );
	ASSERT( min_count > 0, "Error in min iteration count");
        modify_synch_iteration_count ( min_count - 1 );
    }
}

void
SYNTH_THREAD_CLASS::commit_skipping_nops(ASIM_INST inst)
{
    const UINT64 ref_uid = inst->GetUid();
    // This is an in-order commit.
    //
    // The ASIM tanglewood model will not commit NOPs so we
    // must remove them manually.

    SYNTH_INST_ITER i = issued.begin();
    while( i != issued.end() )
    {
        SYNTH_INST minst = *i;
        if (minst->GetUid() == ref_uid)  
        {
            //SMSG("Explicitly commiting uid = " << minst->GetUid());
            m_debug("Explicit commit ", inst);

            issued.erase(i++);
            delete_synth_inst(minst);
            
            num_committed_inst++;
            
            return;
        }
        else if (minst->isNOP())
        {
            // the tanglewood mode
            SMSG("Implicitly commiting a NOP: uid = " << minst->GetUid());
            issued.erase(i++);
            delete_synth_inst(minst);
            continue;
        }
        SMSG("Failed to commit uid = " <<
             minst->GetUid() << " type= " << minst->type);
        SMSG("   since committing out of order is not supported yet.\n");
        assert(0); // committing out of order is not supported yet.
    }
}




IPF_HINT_ENUM
SYNTH_THREAD_CLASS::convert_lfetch_hint(std::string s) const
{
    if (s == "NONE" || s == "none") { return IPF_HINT_NONE; }
    if (s == "NT1" || s == "nt1")   { return IPF_HINT_NT1; }
    if (s == "NT2" || s == "nt2")   { return IPF_HINT_NT2; }
    if (s == "NTA" || s == "nta")   { return IPF_HINT_NTA; }
    cerr << "ERROR: Invalid string for lfetch hint: [" << s << "]" << endl;
    cerr << "       Must be NONE, NT1, NT2, or NTA" << endl;
    exit(1);
}



//Local Variables:
//pref: "synth-thread.h"
//pref2: "synth-inst.h"
//End:
