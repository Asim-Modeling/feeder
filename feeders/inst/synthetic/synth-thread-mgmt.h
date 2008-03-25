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

/* synth-thread-mgmt.h */
/* Mark Charney   <mark.charney@intel.com> */
/*$Id: synth-thread-mgmt.h 799 2006-10-31 12:27:52Z cjbeckma $ */


#ifndef _SYNTH_THREAD_MGMT_H_
# define _SYNTH_THREAD_MGMT_H_

#include "synth-headers.h"
#include "synth-types.h"
#include "synth-thread.h"
#include "synth-inst.h"
#include "ipf-bundle-builder.h"

/*void
init_bundle_builder(std::string even,
                    std::string odd,
                    std::string memop);
*/

///////////////////////////////////////////////////////////////////

class THREAD_MGMT_CLASS
{
    // THREAD_MGMT_CLASS is a class to track the threads and tell ASIM
    // about their existance.

  public:


    THREAD_MGMT_CLASS(); //CONS
    ~THREAD_MGMT_CLASS();

    
    void
    make_threads(IFEEDER_BASE feeder, SYNTH_PARAMS p);

    void
    end_thread(UINT64 streamId);

    
    inline SYNTH_THREAD_CLASS*
    get_thread(UINT32 i) const
    {
        assert(i< max_threads);
        assert(threads[i]);
        return threads[i];
    }

    ////////////////////////////////////////////////////////
    // General feeder interface
    ////////////////////////////////////////////////////////

    void
    issue(ASIM_INST inst,
          UINT32 streamId);

    void
    commit(ASIM_INST inst,
           UINT32 streamId);

    void
    kill(ASIM_INST inst,
         UINT32 streamId,
         bool fetchNext,
         bool killMe);

    /* called by Fetch */
    void
    generate_asim_inst(UINT64      streamId,
                       IADDR_CLASS predicted_pc,
                       ASIM_INST   inst);

    /* helper functions for handling writes to the MVM */

    void
    handle_mvm_writes(ASIM_INST inst,
                      const UINT32 streamId,
                      const MVM_STATE st,
                      const char* str);
    void
    handle_mvm_read(ASIM_INST inst,
                    const UINT32 streamId);

     ////////////////////////////////////////////////////////

  private:

    IPF_INST_BUILDER_CLASS * ipf_builder;
  
    //
    // MVM interface
    //

    inline void
    read_value(MVM_MEMOP* m)
    {
        mvm.read_value_now(m);
    }


    inline void
    store_visible(MVM_MEMOP* m,
                  MVM_STATE mvm_state)
    {
        switch(mvm_state)
        {
          case MVM_LOCALLY_VISIBLE:
            mvm.store_locally_visible_now(m);
            break;
          case MVM_GLOBALLY_VISIBLE:
            mvm.store_globally_visible_now(m);
            break;
          default:
            assert(0);
        }
    }

    inline void
    killed(MVM_MEMOP* m)
    {
        mvm.killed(m);
    }

    /* helper functions for handling writes to the MVM */
    void
    handle_mvm_one_write(ASIM_INST inst,
                         UINT32 streamId,
                         MVM_STATE st,
                         UINT32 bits,
                         UINT64 pa,
                         const char* str,
                         const UWORD128& value);

     ////////////////////////////////////////////////////////
    

    /* accessors for the table that maps
       ASIM_INST's uids to mvm_memop*  */

    MVM_MEMOP*
    find_memop_in_map(UINT64 asim_uid); 

    void
    remove_memop_from_map(MVM_MEMOP* m);

    void
    remember_memop_in_map(UINT64 asim_uid,
                          MVM_MEMOP* m);

    /* the ASIM_INST is ORed with a bit of the phys addr to get a key for
     * the map. */
    map<UINT64, MVM_MEMOP*> asim_inst_to_memop_map;        
           
  private:
    
    ////////////////////////////////////////////////////

    UINT32 max_threads;

    /* "threads" is the main state variables of the synthetic
     * feeder */
    SYNTH_THREAD_CLASS** threads;

    MEMORY_VALUE_MODEL_CLASS mvm;

    // bool using_synthetic_events;
    
    SYNTH_PARAMS params;

    ////////////////////////////////////////////////////
    
    void
    kill_memops(const UINT32 streamId,
                const UINT32 asim_uid,
                const bool killme);
    
    /* inform the memop that we no longer care and if no one else cares,
       delete it */
    void
    reclaim_memop(MVM_MEMOP* m);

    void
    assemble_inst_from_synth_events(SYNTH_THREAD thread,
                                    UINT64 streamId,
                                    ASIM_INST inst,
                                    IADDR_CLASS predicted_pc);
    void
    assemble_inst_from_program(SYNTH_THREAD thread,
                               UINT64 streamId,
                               ASIM_INST inst,
                               IADDR_CLASS predicted_pc);

    void
    misc_inst_setup(UINT64    streamId,
                    ASIM_INST inst);

    void
    emit_first_branch(const SYNTH_THREAD thead,
                      const UINT64      streamId,
                      ASIM_INST         inst,
                      const IADDR_CLASS predicted_pc);

    void
    issue_synth_inst_from_replay(SYNTH_THREAD thead,
                                 ASIM_INST   inst);
    
    void
    correct_path_synthetic(SYNTH_THREAD thead,
                           ASIM_INST   inst,  /* output */
                           UINT64      streamId);

    void
    correct_path_programmed(SYNTH_THREAD thead,
                            ASIM_INST   inst,  /* output */
                            UINT64      streamId,
                            IADDR_CLASS predicted_pc);
    
    void
    wrong_path(IADDR_CLASS predicted_pc,
               SYNTH_THREAD thead,
               ASIM_INST   inst);

    ///////////////////////////////////////////////////////////
    // Junk bundle handling wrong path and correct path filler
    ////////////////////////////////////////////////////////////
    void
    create_junk_bundle_upper(SYNTH_THREAD thread,
                             IADDR_CLASS predicted_pc,
                             ASIM_INST   inst);
    void
    create_junk_bundle(SYNTH_THREAD thread,
                       IADDR_CLASS predicted_pc);
    void 
    create_junk_bundle_keeper(SYNTH_THREAD thread,
                              IADDR_CLASS predicted_pc);
    void
    create_junk_bundle_helper(SYNTH_THREAD thread,
                              IADDR_CLASS predicted_pc,
                              IPF_RAW_BUNDLE* bundle,
                              SYNTH_IPF_ENUM* ipf_syllables);
    
    ////////////////////////////////////////////////////////////

    void
    consume_synthetic_events(SYNTH_THREAD thead,
                             UINT64      streamId);

    void
    convert_synth_inst_to_asim_inst(SYNTH_INST minst,
                                    ASIM_INST  inst);

    SYNTH_INST
    convert_event_to_synth_inst(UINT64       streamId,
                                SYNTH_THREAD thead);

    bool
    need_filler(SYNTH_THREAD thread);
    
    bool
    test_and_make_branch(SYNTH_THREAD thread,
                         ASIM_INST inst);
    void
    make_filler(SYNTH_THREAD thread,
                ASIM_INST inst);


    ///////////////////////////////////////////////
    // synth-inst (si) value map --
    //    interface to thread functions

    UWORD128*
    find_si_value(UINT32 streamId,       
                  ASIM_INST inst);

    void
    remove_si_value(UINT32 streamId,       
                    ASIM_INST inst);

}; /* class THREAD_MGMT_CLASS */


#endif
//Local Variables:
//pref: "synth-thread-mgmt.cpp"
//End:
