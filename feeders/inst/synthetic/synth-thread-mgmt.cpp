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

// synth-thread-mgmt.cpp
// Mark Charney   <mark.charney@intel.com>
// $Id: synth-thread-mgmt.cpp 799 2006-10-31 12:27:52Z cjbeckma $

#include "synth-thread-mgmt.h"
#include "synth-debug.h" //XMSG, BMSG, etc.
#include "genable-inst.h"
#include "synth-memop.h"
#include "synth-util.h" // random numbers and some support funcs

////////////////////////////////////////////////////////////////////////////

/*
void
init_bundle_builder(std::string even,
                    std::string odd,
                    std::string memop)
{
    // set up the bundle parameters
    ipf_builder.init( even, odd, memop);
}
*/


////////////////////////////////////////////////////////////////////////////
#define NPROC 32 //FIXME

THREAD_MGMT_CLASS::THREAD_MGMT_CLASS() //CONS
    : max_threads(NPROC),
      mvm(NPROC)
      // using_synthetic_events(false)
{
    threads = new SYNTH_THREAD_CLASS*[max_threads];
    
    for(UINT32 i=0;i<max_threads;i++)
    {
        threads[i]=0;
    }
}

THREAD_MGMT_CLASS::~THREAD_MGMT_CLASS()
{
    for(UINT32 i=0;i<max_threads;i++)
    {
        if (threads[i])
        {
            delete threads[i];
        }
    }
    delete [] threads;
}


void
THREAD_MGMT_CLASS::make_threads(IFEEDER_BASE feeder, SYNTH_PARAMS p)
{
    
    params = p;
    
    ipf_builder = new IPF_INST_BUILDER_CLASS[max_threads](params);
   
    //BMSG("Max_threads " <<  max_threads);
    //BMSG("SYNTH_THREADS " <<  SYNTH_THREADS);
    // NOTE: because THREAD_MGMT_CLASS is a static global,
    // the update for SYNTH_THREADS hasn't occurred yet.
    // So we must re-initialize it here.
    if (max_threads < SYNTH_THREADS)
    {
        cerr << "ERROR: Recompile" << __FILE__
             << " with a larger max_threads. Sorry!"
             << endl;
        exit(1);
    }
    max_threads = SYNTH_THREADS;

    // using_synthetic_events = (SYNTH_SYNCHRONIZATION==0);

    for(UINT32 new_thread_id=0 ;
        new_thread_id < max_threads ;
        new_thread_id++)
    {

        ipf_builder[new_thread_id].init( params->getSynth_Even_Bundle(new_thread_id),
                                         params->getSynth_Odd_Bundle(new_thread_id),
                                         params->getSynth_Memop_Bundle(new_thread_id));

        SMSG("New thread " <<  new_thread_id);

        threads[new_thread_id] = new SYNTH_THREAD_CLASS(new_thread_id,
                                                        params,
                                                        (params -> getSynth_Synchronization(new_thread_id) == 0));
            
        /* We just assume pc = 0 to start and make a branch to the
           correct addres later. */
        const UINT32 pc = 0;

        IFEEDER_STREAM_HANDLE handle =
            SYNTHETIC_FEEDER_CLASS::STREAM_HANDLE(new_thread_id);
        IFEEDER_THREAD asim_ithread =
            new IFEEDER_THREAD_CLASS(feeder, handle, pc);
            
        threads[new_thread_id]->set_asim_thread(asim_ithread);
    } 
        
} /* make_threads */

void
THREAD_MGMT_CLASS::end_thread(UINT64 streamId)
{
    assert(streamId < max_threads);
    IFEEDER_THREAD asim_ithread = threads[streamId]->get_asim_thread();
    assert(asim_ithread);
    asim_ithread->ThreadEnd();
} /* end_thread */

////////////////////////////////////////////////////////////////////////////

void
THREAD_MGMT_CLASS::issue(ASIM_INST inst,
                         UINT32 streamId)
{
    // find the genable insts that we used to
    // make the branch... then figure out if it
    // was a taken branch or not...
    if (inst->IsBranch() && inst->IsPredicated())
    {

        /* $$ setting the branch direction */
        const SYNTH_THREAD thread = get_thread(streamId);
        const SYNTH_INST minst = thread->find_si(inst);
        if (minst)
        {
            const bool blocked = thread->get_iteration_blocked( minst->iteration );
            /* if we are blocked then we branch back to top. */
            BMSG("\ttid= " << streamId
                 << " uid = " << minst->uid
                 << " iter=" << minst->iteration
                 << " issue predicated branch: "
                 << (blocked?"TAKEN/BLOCKED":"NOTTAKEN/ENTERING"));
            inst->SetActualTaken( blocked ); 


            if (blocked)
            {
                /* reaffirm the taken target; we may have overwritten this
                 * on previous issue attempts. */
                inst->SetActualTarget( minst->target );

                BMSG("\ttid= " << streamId
                     << " uid = " << minst->uid
                     << " iter=" << minst->iteration
                     << " target(TKN)= " << inst->GetActualTarget());
            }
            else
            {
                IADDR_CLASS fall_through_pc =  minst->pc.Next();
                SMSG("\ttid= " << streamId
                     << " uid = " << minst->uid
                     << " iter=" << minst->iteration
                     << " NewTarget(FT)= " << fall_through_pc);
                inst->SetActualTarget( fall_through_pc );
            }
        }
        else
        {
            WMSG("issue() did not find the SYNTH_INST");
        }
    }

} /* issue */

void
THREAD_MGMT_CLASS::commit(ASIM_INST inst,
                          UINT32 streamId)
{
    /* * 'inst' is committed... */
    SYNTH_THREAD thread = get_thread(streamId);
    
    // find the SYNTH_INST*  inst that matches this asim inst
    thread->commit_skipping_nops(inst);
    
    // Check stop condition
    if (params -> getSynth_Num_Instr_To_Commit(streamId) && (thread->GetNumCommittedInst() == params -> getSynth_Num_Instr_To_Commit(streamId)))
    {
        end_thread(streamId);
    }
    
} /* commit */

void
THREAD_MGMT_CLASS::kill_memops(const UINT32 streamId,
                               const UINT32 asim_uid,
                               const bool killme)
    
{
    /* Walk through the issued ops for this stream and look for the
       memops that match this (uid,pa,size) or are younger.

       If we find one in the asim-inst-to-mvm-memop map, then
       we should remove that from the mvm.
    */
    SYNTH_THREAD thread = get_thread(streamId);

    list<SYNTH_INST>* issued = thread->get_issued_list(); 
    SYNTH_INST_ITER it = issued->begin();
    const SYNTH_INST_ITER ite = issued->end();
    for ( ; it != ite ; it++ )
    {
        const SYNTH_INST p = *it;
        if (	 p->GetUid() > asim_uid     || // kill younger ops too
                 ( killme && p->GetUid() == asim_uid )) 
        {
            SMSG("Killing memops for uid = " << p->GetUid());
            /* look in the asim-to-memop map */
            MVM_MEMOP* m = find_memop_in_map(p->GetUid()); 
            if (m)
            {
                SMSG(" ... killed.");
                killed(m);
            }	 
        }   
    }
} /* kill_memops */


void
THREAD_MGMT_CLASS::kill(ASIM_INST inst,
                        UINT32 streamId,
                        bool fetchNext,
                        bool killMe)
{
    
    /*  Kill instruction and all younger instructions */

    const UINT64 uid = inst->GetUid();
    //m_debug("Kill", inst);

    SMSG("FEED::Kill uid = "
         << uid
         << "  stream = "
         << streamId
         << (fetchNext?" FETCH-NEXT ":" REFETCH-THIS ") 
         << (killMe?" KILLME ":" ")          );

    // fetchNext is true for mispredicts, for example.
    
    
    // reclaim the MVM memops using the issued list
    kill_memops(streamId,
                uid,
                killMe);

    SYNTH_THREAD thread = get_thread(streamId);

    /* kill everything YOUNGER (larger) than uid and possibly also uid, if
     * killMe is set. */

    if (params -> getSynth_Synchronization(thread->get_uid())) {
      thread->find_new_iteration_count(uid, killMe);
    }

    /* move things from the back of the issued queue to the
       front of the replay queue */
    thread->copy_to_replay_queue(uid);

    if (killMe)
    {
        assert(fetchNext==false);
        thread->kill_and_replay_this_inst(uid);
    }
} /* kill */

void
THREAD_MGMT_CLASS::handle_mvm_read(ASIM_INST inst,
                                   const UINT32 streamId)
{

    if (inst->IsPrefetch())
    {
        return;
    }
    const UINT64 pa = inst->GetPhysicalEffAddress();
    const UINT64 log_bytes_size = inst->GetAccessSize();
    const UINT64 bits_size = (1<<log_bytes_size)*8;

    MVM_MEMOP m( MVM_LOAD, bits_size, streamId, pa);
    read_value(&m);
    BMSG("DoRead: tid= " << streamId << " uid = " << inst->GetUid() << " " << m);

    if (inst->IsCompareExchange() || inst->IsLoad() )
    {
        /* $$ determine if the cmxpch got the lock -- or if the load saw the lock available */
        UINT32 old_val = m.get_value32(pa); // pa gets masked

        /*
          If the "old value" we read here is zero, then the
          lock is free and we should make the dependent compare
          produce zero, and the branch should fall-through.

          Use iteration counts! When the branch issues/executes,
          we should have it check the cmpxch result for this iteration.

          Also determined in DoRead, is if the cmpxch should write the
          old or new value back, if it ever stores.
         */

        const UINT32 ar_ccv_proxy = 0; //FIXME
        const bool entry_result = (old_val == ar_ccv_proxy);

        BMSG("tid= " << streamId <<  " uid = " << inst->GetUid() << " Entry_result = "
             << (entry_result?"CLEAR-TO-ENTER":"BLOCKED"));

        /* if the entry_result is 0, the compare-exchange should NOT do its
         * store -- we are blocked.  The conditional loop-back branch
         * should loop back to the top. */
        
        const SYNTH_THREAD thread = get_thread(streamId);
        SYNTH_INST minst = thread->find_si(inst);
        if (minst)
        {
            SMSG("Found SYNTH_INST uid = " << minst->uid
                 << " for ASIM_INST uid = " << inst->GetUid());

            if (entry_result)
            {
                /* entering */
                BMSG("Setting ENTERING for tid= " << streamId
                     << " uid = " << minst->uid
                     << " iter= " << minst->iteration);
                thread->set_iteration_entering( minst->iteration );
            }
            else
            {
                /* blocked */
                BMSG("Setting BLOCKED for tid= " << streamId
                     << " uid = " << minst->uid
                     << " iter= " << minst->iteration);
                thread->set_iteration_blocked( minst->iteration );
            }
        }
        else
        {
            WMSG("load/cmpxch was not found at DoRead time.");
        }
    }
} /* handle_mvm_read */

////////////////////////////////////////////////////////////////////////////

UWORD128*
THREAD_MGMT_CLASS::find_si_value(UINT32 streamId,
                                 ASIM_INST inst)
{
    const SYNTH_THREAD thread = get_thread(streamId);
    UWORD128* v= thread->find_si_value(inst);
    if (!v)
    {
        // FIXME: didn't find the SYNTH_INST with the value to store
    }
    return v;
}
    
void
THREAD_MGMT_CLASS::remove_si_value(UINT32 streamId,
                                   ASIM_INST inst)
{
    const SYNTH_THREAD thread = get_thread(streamId);
    thread->remove_si_value(inst);
}
    
////////////////////////////////////////////////////////////////////////////

void
THREAD_MGMT_CLASS::handle_mvm_writes(ASIM_INST inst,
                                     const UINT32 streamId,
                                     const MVM_STATE st,
                                     const char* str)
{

    /*
      Note: compare-exchange must store back the old data if there is a miscompare.
    */

    m_debug(str, inst);

    const UINT64 pa = inst->GetPhysicalEffAddress();
    const UINT64 log_bytes_size = inst->GetAccessSize();
    const UINT64 bits_size = (1<<log_bytes_size)*8;

    UWORD128 stored_value;
    UWORD128* v = find_si_value(streamId, inst);
    if (v)
    {
        stored_value = *v; 
    }
    else
    {
        WMSG("Missing synth inst -- assuming value 0");
        UINT128 q;
        q.a[0] = 0;
        q.a[1] = 0;
        stored_value.set128(q);
    }
    

    handle_mvm_one_write(inst,
                         streamId,
                         st,
                         bits_size,
                         pa,
                         str,
                         stored_value);

} /* handle_mvm_writes */


void
THREAD_MGMT_CLASS::reclaim_memop(MVM_MEMOP* m)
{
    // remove the memop from our map
    remove_memop_from_map(m);

    // garbage collect the memop
    m->feeder_does_not_care();

    if (m->noone_cares())
    {
        delete m;
    }
} /* reclaim_memop */

void
THREAD_MGMT_CLASS::handle_mvm_one_write(ASIM_INST inst,
                                        UINT32 streamId,
                                        MVM_STATE st,
                                        UINT32 bits,
                                        UINT64 pa,
                                        const char* str,
                                        const UWORD128& value)
{   
    MVM_MEMOP* m = 0;
    bool found = false;

    /* find the memop, or make one */
    //if (st == MVM_GLOBALLY_VISIBLE)
    {
        // GlobalVis stores are sometimes LocalVis first, so we may have a
        // mvm memop already.
        
        m = find_memop_in_map(inst->GetUid()); // match on the inst and the pa
        found = true;
    }


    /*
      Compare-exchange's don't always store (or they atomically store-back
      the old value they read earlier). If our compare-exchage is not storing
      a new value, we must (a) clean up the mvm memop -- but that really shouldn't
      exist -- and (b) remove the value from the asim-inst-to-value map.
     */
    if (inst->IsCompareExchange())
    {
        /* $$ handling conditional write */
        const SYNTH_THREAD thread = get_thread(streamId);
        const SYNTH_INST minst = thread->find_si(inst);
        bool doing_conditional_store = false;
        if (minst)
        {
            /* if the condition is true, we are doing the write. */
            doing_conditional_store = thread->get_iteration_entering(minst->iteration);
            BMSG("STORE-CHECK tid= " << streamId
                 << " uid = " << minst->uid
                 << " iter= " << minst->iteration
                 << (doing_conditional_store?" ENTERING":" BLOCKED")
                 << " arry= " << thread->get_iteration_status(minst->iteration));
        }
        else
        {
            WMSG("handle_mvm_one_write() did not find the cmpxch SYNTH_INST");
        }
        if (!doing_conditional_store)
        {
            /* remove the value from the a2s_map */
            remove_si_value(streamId, inst);
            if (m)
            {
                /* we are done with the memop -- delete it */
                reclaim_memop(m);
            }
            return;
        }
    }
    
    if (!m)
    {
        /* no memop found -- make a new one */   
        m = new MVM_MEMOP( MVM_STORE,
                           bits,
                           streamId,
                           pa,
                           inst->GetUid() );
        m->set_value128(value);
    }

    // set the store state in the MVM_MEMOP
    m->set_state(st);

    BMSG("tid= " << streamId << " uid = " << inst->GetUid() << " " << str << ": " << *m);

    // set the new store state in the MVM 
    store_visible(m, st); 

    if (st == MVM_GLOBALLY_VISIBLE)
    {
        /* we are done with the memop -- delete it */
        reclaim_memop(m);

        /* remove the value from the a2s_map */
        remove_si_value(streamId, inst);
    }
    else if (!found )
    {
        /* we only remember the local visible stuff because we might
           kill them later. We never can kill globally visible stuff. */

        remember_memop_in_map(inst->GetUid(), m);
    }
} /* handle_mvm_one_write */



typedef map<UINT64,MVM_MEMOP*>::iterator mvm_map_iter_t;

MVM_MEMOP*
THREAD_MGMT_CLASS::find_memop_in_map(UINT64 asim_uid)
{

    mvm_map_iter_t mi;
    mvm_map_iter_t m_end = asim_inst_to_memop_map.end();

    const UINT64 key  = asim_uid;
    mi = asim_inst_to_memop_map.find(key);
    if (mi !=  m_end)
    {
        return (*mi).second;
    }

    // not found
    return 0; 
}

void
THREAD_MGMT_CLASS::remove_memop_from_map(MVM_MEMOP* m)
{
    mvm_map_iter_t mi;
    mvm_map_iter_t m_end = asim_inst_to_memop_map.end();

    const UINT64 key  = m->get_asim_uid();
    mi = asim_inst_to_memop_map.find(key);
    if (mi !=  m_end)
    {
        asim_inst_to_memop_map.erase(mi);
    }
}


void
THREAD_MGMT_CLASS::remember_memop_in_map(UINT64 asim_uid,
                                         MVM_MEMOP* m)
{
    const UINT64 key  = asim_uid;
    asim_inst_to_memop_map[key] = m;
}


//////////////////////////////////////////////////////////////////


void
THREAD_MGMT_CLASS::misc_inst_setup(UINT64    streamId,
                                   ASIM_INST inst)
{
    inst->SetTraceID(streamId); 
    
    const UINT64 default_cfm = 0x7F; /* ask Eric! no rotating registers.
                                        Bad, bad, bad naked constant! */
    
    UINT64 old_cfm = default_cfm;
    UINT64 new_cfm = default_cfm;
    UINT64 old_prf = default_cfm;
    UINT64 new_prf = default_cfm;
    
    inst->SetCFM(old_cfm, new_cfm);
    inst->SetPRF(old_prf, new_prf);
} /* misc_inst_setup */



void
THREAD_MGMT_CLASS::create_junk_bundle_helper(SYNTH_THREAD thread,
                                             IADDR_CLASS predicted_pc,
                                             IPF_RAW_BUNDLE* bundle,
                                             SYNTH_IPF_ENUM* ipf_syllables)
{
    /* lower-level routine of the jnk bundle call tree */

    /* This makes a junk bundle for "filler" (non-memop) syllables and
     * wrong-path fetches */

    IPF_TEMPLATE_ENUM tmplt;

    if (even_bundle(predicted_pc))
    {
        tmplt = ipf_builder[thread->get_uid()].refine_to_syllables(SYNTH_BUNDLE_EVEN,
                                                thread->get_memop(0),
                                                ipf_syllables); /* output */
    }
    else
    {
        tmplt = ipf_builder[thread->get_uid()].refine_to_syllables(SYNTH_BUNDLE_ODD,
                                                thread->get_memop(0),
                                                ipf_syllables); /* output */
    }

    if (thread->insert_dependent_uses(ipf_syllables))
    {
        SMSG("inserted use; pop dest reg list");
        //FIXME: need to capture the register to use for the dependent xor.
        thread->set_last_dest_reg( thread->pop_dest_reg_from_history() );
    }

    /* Bind registers and set misc. fields in the syllables specified by
     * ipf_syllables */

    HandleJunkOps(thread,
                  bundle, /* output*/
                  tmplt,  /* input */
                  ipf_syllables); /* input */

} /* create_junk_bundle_helper */

void
THREAD_MGMT_CLASS::create_junk_bundle(SYNTH_THREAD thread,
                                      IADDR_CLASS predicted_pc)
{
    /* called by create_junk_bundle_upper */

    SYNTH_IPF_ENUM ipf_syllables[SYLLABLES_PER_BUNDLE]; /* thrown away */
    IPF_RAW_BUNDLE* bundle = thread->GetJunkBundle();
    create_junk_bundle_helper(thread, predicted_pc, bundle, ipf_syllables);
} /* create_junk_bundle */

void
THREAD_MGMT_CLASS::create_junk_bundle_upper(SYNTH_THREAD thread,
                                            IADDR_CLASS predicted_pc,
                                            ASIM_INST   inst)
{
    /* a wrapper for create_junk_bundle */

    if (!thread->have_a_junk_bundle())
    {
        /* create a junk bundle when we don't have one */
        create_junk_bundle(thread, predicted_pc);
        thread->set_junk_bundle();
    }

    /* issue next syllable of the junk inst */
    set_bundle_bits(thread->GetJunkBundle(), predicted_pc, inst);
    debug_disassemble(predicted_pc, inst);


    /* see if we are done with the current junk bundle */
    UINT32 syllable_idx = predicted_pc.GetSyllableIndex();
    if (syllable_idx == (SYLLABLES_PER_BUNDLE-1))
    {
        thread->clear_junk_bundle();
    }

} /* create_junk_bundle_upper */


void 
THREAD_MGMT_CLASS::create_junk_bundle_keeper(SYNTH_THREAD thread,
                                             IADDR_CLASS predicted_pc)
{
    /*
     * This makes an instruction bundle at the requested address.
     */

    SMSG(" create_junk_bundle_keeper");

    SYNTH_IPF_ENUM ipf_syllables[SYLLABLES_PER_BUNDLE];
    IPF_RAW_BUNDLE bundle;

    /* fill in bundle & ipf_syllables */
    create_junk_bundle_helper(thread, predicted_pc, &bundle, ipf_syllables);

    UINT64 pc = predicted_pc.GetBundleAddr();

    /* make a internal inst for each syllable */

    for ( UINT32 i=0 ; i<SYLLABLES_PER_BUNDLE ; i++ ) 
    {
        SYNTH_IPF_ENUM btype = ipf_syllables[i];
        SYNTH_INST minst = new SYNTH_INST_CLASS(pc,i,btype);
        minst->SetBundle(bundle);
        XMSG("add junk inst syllable " << i << " to replay");
        thread->add_to_replay(minst);
        thread->inc_syllable();
    }

    thread->set_syllable_idx(0); // clear the syllable index

    // we count bundles, not syllables in the skip
    thread->dec_preceeding_inst();


} /* create_junk_bundle_keeper */


////////////////////////////////////////////////////////////////////////////

void
THREAD_MGMT_CLASS::consume_synthetic_events(SYNTH_THREAD thread,
                                            UINT64      streamId)
{
    SMSG("Getting a new event");

    // create an event in thread based on a random number generator
    // fill in "event".
    SYNTH_EVENT event = thread->get_event();
    thread->set_gap();  // the number of fake instructions between "real" insts
    thread->set_syllable_idx(0); 

    for(UINT32 i=0; i< SYLLABLES_PER_BUNDLE; i++)
    {
        /* put memop info in the SYNTH_MEMOP* m for this syllable */

        SYNTH_MEMOP* m = thread->get_memop(i);
        const SYNTH_FTYPE  ms = ipf_builder[thread->get_uid()].get_memop_syllable(i);
        switch(ms)
        {
          case SYNTH_FTYPE_MEMOP:
            /* refine an underspecified generic memop */
            event->select_event_type(); // select the LD/ST/EX etc.
            event->set_next_data_addr();    // set data VA and instruction IP
            event->set_dependent_ops();
            event->set_memop(m); // capture the info from event in to m
            break;

          case SYNTH_FTYPE_LOAD:
          case SYNTH_FTYPE_STORE:
          case SYNTH_FTYPE_EXCHANGE:
          case SYNTH_FTYPE_COMPARE_EXCHANGE:
          case SYNTH_FTYPE_FETCH_AND_ADD:
          case SYNTH_FTYPE_LFETCH:

            event->set_event_type(ms); // also sets acq/rel options randomly

            event->set_next_data_addr();    // set data VA and instruction IP

            event->set_dependent_ops();

            event->set_memop(m); // capture the info from event in to m

            if (ms == SYNTH_FTYPE_LFETCH)
            {
                event->set_hint( thread->get_lfetch_hint() );
            }
            break;
          default:
            m->etype = SYNTH_ETYPE_INVALID;
            break;
        }
    }
    /* refine the memop event into SYNTH_IPF_* syllables */
    SYNTH_IPF_ENUM ipf_memop_syllables[SYLLABLES_PER_BUNDLE];

    IPF_TEMPLATE_ENUM tmplt = 
        ipf_builder[thread->get_uid()].refine_to_syllables(SYNTH_BUNDLE_MEMOP,
                                        /* pass in 1st element of  */
                                        thread->get_memop(0),
                                        ipf_memop_syllables); /* output */

    if ( thread->insert_dependent_uses(ipf_memop_syllables) )
    {
        SMSG("inserted use (memop); pop dest reg list");
        //FIXME: need to capture the register to use for the dependent xor.
        thread->set_last_dest_reg( thread->pop_dest_reg_from_history() );
    }
    
#if 0
    for(UINT32 i=0;i<SYLLABLES_PER_BUNDLE;i++)
    {
        SMSG("ipf_memop_syllable[" << i << "] = " << ipf_memop_syllables[i]);
    }
#endif

    thread->set_syllables(ipf_memop_syllables);

    /* set the bundle details in the thread */
    HandleMemoryRW(thread, tmplt, params);  // create syllables and bundle, bind registers

} /* consume_synthetic_events */


SYNTH_INST
THREAD_MGMT_CLASS::convert_event_to_synth_inst(UINT64      streamId,
                                               SYNTH_THREAD thread)
{
    /*
     * create an SYNTH_INST from the event (for branches) and the memop in
     * the thread.
     */

    const IPF_RAW_BUNDLE* bundle       = thread->get_bundle();
    const UINT32          syllable_idx = thread->get_syllable_idx();
    const IADDR_CLASS     pc           = thread->get_ipf_pc();


    const SYNTH_IPF_ENUM  type = thread->get_syllable(syllable_idx);

    /* create a new SYNTH_INST */
    SYNTH_INST minst = new SYNTH_INST_CLASS(pc,type);

    minst->SetBundle(*bundle);
    SMSG("syllable_idx= " << syllable_idx << " type= " << type);

    switch(type)
    {
      case SYNTH_IPF_NOPM:
      case SYNTH_IPF_NOPI:
      case SYNTH_IPF_NOPB:
      case SYNTH_IPF_NOPF:
      case SYNTH_IPF_XOR:
      case SYNTH_IPF_XOR_DEPENDENT:
      case SYNTH_IPF_FENCE:
      case SYNTH_IPF_LIMM:
      case SYNTH_IPF_FMA:
      case SYNTH_IPF_COMPARE:
        // nothing to do
        thread->inc_syllable();
        break;

      case SYNTH_IPF_LOAD:
      case SYNTH_IPF_FETCH_AND_ADD:
      case SYNTH_IPF_COMPARE_EXCHANGE:
      case SYNTH_IPF_EXCHANGE:
      case SYNTH_IPF_STORE:
      case SYNTH_IPF_LFETCH:
        SMSG("Set EA for Load/Exch/Store(1)");
        {
            /* copy info from the memop to the minst */

            SYNTH_MEMOP* m = thread->get_memop(syllable_idx);
            minst->SetMemopInfo(m->ea, m->sz);
            minst->set_dependence_distance( m->get_dependence_distance() );
            minst->set_reg_dest( m->get_reg_dest() );
        }
            
        thread->inc_syllable();
        break;


      case SYNTH_IPF_LONG_BRANCH:
      case SYNTH_IPF_BRANCH:
        {
            const SYNTH_EVENT     event        = thread->get_event();
            ASSERTX(event);
            bool   taken  = event->get_is_branch_taken();
            UINT64 target = event->get_branch_target_ip();
            SMSG("Telling feeder about a branch(1) to " << hex << target <<dec);
            IADDR_CLASS real_target;

            if (taken)
            {
                real_target.Set(target,0);
            }
            else
            {
                real_target = pc.Next();
            }

            minst->SetBranchInfo(taken, real_target);

            thread->inc_syllable();
            if (taken)
            {
                thread->set_pc(target); // resets the syllable to 0
                thread->make_done();
            }
        }
        break;

      case SYNTH_IPF_INVALID:
      case SYNTH_IPF_LAST:
      default:
        MSG("ERROR: Unexpected SYNTH_IPF_* node.");
        exit(1);
    }

    return minst;
} /* convert_event_to_synth_inst */
    

void
THREAD_MGMT_CLASS::convert_synth_inst_to_asim_inst(SYNTH_INST minst,
                                                   ASIM_INST  inst)
{
    /* fill in the fields in the asim inst */

    SMSG(" convert_synth_inst_to_asim_inst: " << minst->pc);
    set_bundle_bits(&(minst->bundle), minst->pc, inst); // calls inst->Init()
    switch(minst->type)
    {
      case SYNTH_IPF_LOAD:
      case SYNTH_IPF_FETCH_AND_ADD:
      case SYNTH_IPF_COMPARE_EXCHANGE:
      case SYNTH_IPF_EXCHANGE:
      case SYNTH_IPF_STORE:
        SMSG("Set EA for Load/Exch/Store(2)");
        inst->SetVirtualEffAddress(minst->ea);
        inst->SetPhysicalEffAddress(minst->ea); // FIXME: V=R
        inst->SetAccessSize( minst->access_size);
        break;

      case SYNTH_IPF_LFETCH:
        SMSG("Set EA for Lfetch");
        inst->SetVirtualEffAddress(minst->ea);
        inst->SetPhysicalEffAddress(minst->ea); // FIXME: V=R
        break;

      case SYNTH_IPF_LONG_BRANCH:
      case SYNTH_IPF_BRANCH:
        SMSG("Telling feeder about a branch(2) to " << hex
             << minst->target <<dec);
        inst->SetActualTarget(minst->target);
        inst->SetActualTaken(minst->taken);
        break;

      default:
        break;
    }
    debug_disassemble(minst->pc, inst);
} /* convert_synth_inst_to_asim_inst */

void
THREAD_MGMT_CLASS::issue_synth_inst_from_replay(SYNTH_THREAD thread,
                                                ASIM_INST   inst)
{
    /*
     *  Pop a SYNTH_INST from the replay list, convert it to an ASIM_INST.
     *  And add it to the issued list.
     */

    SYNTH_INST minst = thread->get_replay();
    SMSG("issue_synth_inst_from_replay  PC= " << minst->pc);


    // this fills in the ASIM_INST for return to ASIM
    convert_synth_inst_to_asim_inst(minst, inst);

    // and now for some bookeeping:

    thread->pop_replay();
    if (minst->GetUid() == magic_uid)
    {
        // New insts are created with the magic uid.

        SMSG("     Converting magic uid "
             << "to new uid = " << inst->GetUid());
        thread->inc_correct_path_bundle_count();
    }  
    else
    {
        // Old stuff that gets replayed has a uid that is not-equal-to the
        // magic-uid.

        SMSG("     Converting old uid = " << minst->GetUid()
             << " to new uid = " << inst->GetUid());
    }
    minst->SetUid(inst->GetUid()); // get a new uid
    thread->issue(minst);

    // hook everything together so that I can find stuff later.

    if (inst->IsSemaphore() || inst->IsStore() )
    {
        thread->remember_si_value(inst,minst);
    }

    /* $$ Value communication wiring */
    if ( minst->isCompareExchange() || minst->isLoad() )
    {
        /* increment on the cmpxch or load as we will blow everything away
           downstream if we replay the cmpxch or load  */

        thread->inc_iteration_count();
        minst->iteration = thread->get_iteration_count();
        BMSG("load/cmpxch tid= " << thread->get_uid()
             << " uid = "  << minst->uid << " is iteration = " << minst->iteration);

        SMSG("Mapping load/cmpxch ASIM_INST uid = "
             << inst->GetUid() << " to SYNTH_INST uid =" << minst->uid);
        thread->remember_si(inst,minst);
    }
    else if (minst->isBranch() && minst->isConditionalBranch())
    {
        /* really only need to remember the conditional branches */
        minst->iteration = thread->get_iteration_count();
        BMSG("c.branch tid= " << thread->get_uid()
             << " uid = "  << minst->uid << " is iteration = " << minst->iteration);

        SMSG("Mapping branch ASIM_INST uid = " << inst->GetUid() 
             << " to SYNTH_INST uid =" << minst->uid);
        thread->remember_si(inst,minst);
    }


} /* issue_synth_inst_from_replay */

bool
THREAD_MGMT_CLASS::need_filler(SYNTH_THREAD thread)
{
    bool fake_one = false;
    if (params->getSynth_Fixed_Spacing(thread->get_uid())  &&
        !thread->done_with_preceeding_inst())
    {
        // we have some preceeding non-load ops to issue
        fake_one = true;
    }
    else if ((params->getSynth_Fixed_Spacing(thread->get_uid())==0) &&
             random_over_threshold(params->getSynth_Random_Threshold(thread->get_uid())))
    {
        /* we are using random spacing and we are above the threshold ->
         * fake a bundle */
        fake_one = true;
    }
    return fake_one;
} /* need_filler */

void
THREAD_MGMT_CLASS::make_filler(SYNTH_THREAD thread, ASIM_INST inst)
{
    SMSG("Intermediate (junk) inst"); // filler
    XMSG("Intermediate (junk) inst");
    IADDR_CLASS predicted_pc = thread->get_ipf_pc();
    create_junk_bundle_keeper(thread, predicted_pc); // add to replay list
    issue_synth_inst_from_replay(thread, inst);
}

bool
THREAD_MGMT_CLASS::test_and_make_branch(SYNTH_THREAD thread,
                                        ASIM_INST inst) 
{
    UINT64 tgt = 0;
    bool pop_path = false;
    bool push_path = false;
    if (thread->need_to_return())
    {
        pop_path = true;
        tgt = thread->get_return_pc();
        if (itrace)
        {
            XMSG("Branch-back to " << hex << tgt << dec);
        }
    }
    else if (thread->need_branch_to_top())
    {
        tgt = thread->get_loop_target();
    }
    else if ( thread->at_branch() &&
              thread->need_to_branch_out() )
    {
        push_path = true;
        tgt = thread->get_other_start_pc();
        if (itrace)
        {
            XMSG("Branch-out to " << hex << tgt << dec);
        }
        thread->push_pc();

    }

    if (tgt)
    {
        IADDR_CLASS predicted_pc = thread->get_ipf_pc();
        SYNTH_INST minst =
            make_indirect_branch(thread,
                                 predicted_pc.GetBundleAddr(),
                                 tgt);
        thread->add_to_replay(minst);
        issue_synth_inst_from_replay(thread, inst);
        if (push_path) // calling routine -- don't set pc.
        {
            thread->next_bundle();
            thread->push_current_path();
            thread->select_second_path(); // FIXME: need to generalize
        }
        else if (pop_path) // returning -- pc is already set
        {
            thread->pop_path();
            thread->clear_preceeding_insts();
        }
        else // loop branches -- set the pc
        {
            thread->emitted_branch();
            thread->set_pc(tgt);
        }
        return true;
    }
    return false;
} /* test_and_make_branch */


void
THREAD_MGMT_CLASS::correct_path_synthetic(SYNTH_THREAD thread,
                                          ASIM_INST   inst,  /* output */
                                          UINT64      streamId)
{
    /* Create the synth insts for execution on the correct-path. The
     * SYNTH_INSTs get converted into ASIM_INSTs and sent to ASIM.
     *
     * We create:
     *
     * (1) junk/filler bundles,
     * (2) bundles with loads, stores other memops,
     * (3) occasional branch bundles.
     */
    IADDR_CLASS predicted_pc = thread->get_ipf_pc();
    SMSG("CORRECT PATH.   PREDICTED PC: " << predicted_pc
         << " streamId = " << thread->get_uid()
         << " uid = " << inst->GetUid());

    thread->not_wrong_path();
    
    // do we need a loop-closing branch or a routine-ending exit

    if ( test_and_make_branch(thread, inst) )
    {
        return;
    }

    thread->emitted_non_branch();
    thread->emitted_non_branch();
    thread->emitted_non_branch();

    // See if we have some filler ops to generate (when using fixed
    // spacing) OR if a random interval has ended (when using nonfixed
    // spacing).

    if ( need_filler(thread) )
    {
        make_filler(thread, inst);
        return;
    }

    // ...no filler required at this point... so make the memop bundle.
    
    // fill-in the ASIM_INST, adjust the thread, and add to the replay list

    const UINT32 correct_path_bundle_count = thread->get_correct_path_bundle_count();

    // create 3 syllables -- the syllable index increments each time.
    SYNTH_INST memop_minst = 0;
    SYNTH_INST minst;
    minst = convert_event_to_synth_inst(streamId, thread);
    XMSG("add correct path 1 to replay");
    thread->add_to_replay(minst);
    if ( minst->get_dependence_distance() )
    {
        thread->add_dest_reg_to_history( minst->get_reg_dest(),
                                         correct_path_bundle_count +
                                         minst->get_dependence_distance());
    }


    minst = convert_event_to_synth_inst(streamId, thread);
    XMSG("add correct path 2 to replay");
    thread->add_to_replay(minst);
    if ( minst->get_dependence_distance() )
    {
        thread->add_dest_reg_to_history( minst->get_reg_dest(),
                                         correct_path_bundle_count +
                                         minst->get_dependence_distance());
    }

    minst = convert_event_to_synth_inst(streamId, thread);
    XMSG("add correct path 3 to replay");
    thread->add_to_replay(minst);
    if ( minst->get_dependence_distance() )
    {
        thread->add_dest_reg_to_history( minst->get_reg_dest(),
                                         correct_path_bundle_count +
                                         minst->get_dependence_distance());
    }
    // issue the first syllable from the replay list
    issue_synth_inst_from_replay(thread, inst);

} /* correct_path_synthetic */

void
THREAD_MGMT_CLASS::correct_path_programmed(SYNTH_THREAD thread,
                                           ASIM_INST   inst,  /* output */
                                           UINT64      streamId,
                                           IADDR_CLASS predicted_pc)
{
    /* walk the program list and add the next bundle (3 syllables) to the
       replay list.  advance the iterator.  issue_synth_inst_from_replay().
    */

    thread->not_wrong_path();

    GENABLE_INST_CLASS* g = thread->get_ginst(predicted_pc);

    /* copy the SYNTH_INST because the program will issue this
       one again in a loop */

    SYNTH_INST minst = new SYNTH_INST_CLASS(g->minst);
    SMSG("correct_path_programmed: Adding inst to replay queue. pc = " << minst->pc );

    if (g->isPredicated())
    {
        minst->SetConditionalBranch(true);
    }

    thread->add_to_replay(minst);

    // issue the first syllable from the replay list
    issue_synth_inst_from_replay(thread, inst);

    thread->inc_syllable(); // will increment PC, but won't zero syllable_idx.
} /* correct_path_programmed */

void
THREAD_MGMT_CLASS::wrong_path(IADDR_CLASS predicted_pc,
                              SYNTH_THREAD thread,
                              ASIM_INST   inst)
{
   /* number of wrong path fetches before we decide that the simulation is
    * hosed. */
    static const UINT32 WRONG_PATH_LIMIT = 2000;
    
    // Create junk syllables for the wrong path. We do not remember the
    // wrong path instructions. If they every get replayed, we just make
    // new junk syllables.

    SMSG("WRONG PATH.   PREDICTED PC: " << predicted_pc
         << " vs SYNTH_THREAD PC: " << hex << thread->get_ipf_pc() << dec
         << " streamId = " << thread->get_uid()
         << " uid = " << inst->GetUid());
    create_junk_bundle_upper(thread,predicted_pc, inst);
    thread->inc_wrong_path();
    
    if ( thread->get_wrong_path()  >= WRONG_PATH_LIMIT )
    {
        MSG("ERROR: We were on the wrong path for " << WRONG_PATH_LIMIT
            << " calls to FEED::Fetch().");
        exit(1);
    }
} /* wrong_path  */

void
THREAD_MGMT_CLASS::emit_first_branch(const SYNTH_THREAD thread,
                                     const UINT64      streamId,
                                     ASIM_INST         inst,
                                     const IADDR_CLASS predicted_pc)
{
    const UINT64 target = thread->get_pc(); 
    SMSG("First branch (stream " << streamId << ")");
    SYNTH_INST minst = make_indirect_branch(thread,
                                            predicted_pc.GetBundleAddr(),
                                            target);
    XMSG("add 1st br to replay");
    thread->add_to_replay(minst);
    issue_synth_inst_from_replay(thread, inst);
    thread->clear_need_jump(); 
    thread->not_wrong_path();
} /* emit_first_branch */


void
THREAD_MGMT_CLASS::assemble_inst_from_synth_events(SYNTH_THREAD thread,
                                                   UINT64 streamId,
                                                   ASIM_INST inst,
                                                   IADDR_CLASS predicted_pc)
{
    if (thread->done_with_entire_event())
    {
        // we need to get a new event from the random event generator
        // ...or die trying.  Fills in "thread" with the appropriate info.
        consume_synthetic_events(thread, streamId);
    }
    else
    {
        SMSG("Not done with prior event...");
    }

    if (thread->test_need_jump())
    {
        // some stuff for the first time. branch to the first inst.
        // The thread thinks we started at 0.
        emit_first_branch(thread, streamId, inst, predicted_pc);
        return;
    }

    
    SMSG("threadpc= " << hex << thread->get_ipf_pc()
         << " predictedpc= " <<  predicted_pc << dec);

    if (thread->get_ipf_pc() == predicted_pc) // on correct path
    {
        // create correct-path BUNDLES (memops, filler ops and branch-backs)
        //  and issue them.
        correct_path_synthetic(thread, inst, streamId);
    }
    else // wrong path
    {
        // ASIM has gone down the WRONG PATH. We must feed it XORs until it
        // retires the mispredicted path and syncs up with us.  We do not
        // remember ops on the wrong path.
        wrong_path(predicted_pc, thread, inst);
    }        
} /* assemble_inst_from_synth_events */

void
THREAD_MGMT_CLASS::assemble_inst_from_program(SYNTH_THREAD thread,
                                              UINT64 streamId,
                                              ASIM_INST inst,
                                              IADDR_CLASS predicted_pc)
{
    if (thread->get_syllable_idx() == SYLLABLES_PER_BUNDLE)
    {
        thread->set_syllable_idx(0);
    }

    if (thread->test_need_jump())
    {
        // some stuff for the first time. branch to the first inst.
        // The thread thinks we started at 0.
        emit_first_branch(thread, streamId, inst, predicted_pc);
        return;
    }

    
    SMSG("assemble tid= " << streamId
         << " thdPC= " << hex << thread->get_ipf_pc()
         << " predicted= " <<  predicted_pc << dec);

    /* We don't really know what is and isn't on
       the correct path because of speculative fetching.
       We return instructoins as if we were on the correct path
       when we have something in the range of the canned program.

       The issued list DOES NOT get copied to the front of the
       replay list for canned programs.
    */
    

    if (thread->in_range(predicted_pc))
    {
        correct_path_programmed(thread, inst, streamId, predicted_pc);
        return;
    }

    // wrong path

    // ASIM has gone down the WRONG PATH. We must feed it innocuous stuff until it
    // retires the mispredicted path and syncs up with us.  We do not
    // remember ops on the wrong path.
    wrong_path(predicted_pc, thread, inst);
} /* assemble_inst_from_program */

void
THREAD_MGMT_CLASS::generate_asim_inst(UINT64      streamId,
                                      IADDR_CLASS predicted_pc,
                                      ASIM_INST   inst)
{
    /*
     * This is the main entry point for the feeder. It is called by
     * Fetch().  We look around and see what we've got laying around to
     * pass back to the feeder. There may be instructions to
     * replay. Otherwise, we just make something up.  We don't try to
     * remember wrong path instructions. We just remember correct path
     * instructions.

     * NOTE: We pass ALL instructions on the correct path back to the
     * feeder via the replay list. Even the very first time they are
     * fetched.  It is just a convenience thing in this software.
     */

    misc_inst_setup(streamId, inst);
    
    const SYNTH_THREAD thread = get_thread(streamId);

    SMSG("Trying to fetch for stream " << streamId
         << "   uid = " << inst->GetUid()
         << "   at PC = " << hex << predicted_pc << dec);

    // Check for replay syllables first!  NOTE: This is also the place
    // where the 2nd and 3rd syllables in each bundle are sent in to the
    // feeder for the first time. Everything on the correct path starts out
    // on the replay list.
    if (thread->replay_available())
    {
        const SYNTH_INST p = thread->get_replay();
        
        if (p->pc == predicted_pc) // on correct path
        {
            // Convert synth inst to asim inst

            SMSG("CORRECT PATH.   PREDICTED PC: " << predicted_pc
                 << " streamId = " << thread->get_uid()
                 << " uid = " << inst->GetUid());
            issue_synth_inst_from_replay(thread, inst);
            thread->not_wrong_path();
        }
        else
        {
            // Pass back up junk in the asim inst until
            //   the path gets corrected.
            wrong_path(predicted_pc, thread, inst);
        }
        return;
    }


    if ((params -> getSynth_Synchronization(thread->get_uid()) == 0))
    {
        assemble_inst_from_synth_events(thread, streamId, inst, predicted_pc);
    }
    else
    {
        assemble_inst_from_program(thread, streamId, inst, predicted_pc);
    }


} /* generate_asim_inst */



//Local Variables:
//pref: "synth-thread-mgmt.h"
//pref2: "synth-thread.h"
//End:
