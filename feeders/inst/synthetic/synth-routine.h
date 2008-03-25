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

/* synth-routine.h */
/* Mark Charney   <mark.charney@intel.com> */
/*$Id: synth-routine.h 799 2006-10-31 12:27:52Z cjbeckma $ */


#ifndef _SYNTH_ROUTINE_H_
# define _SYNTH_ROUTINE_H_

#include "synth-types.h"
#include "synth-debug.h"

class SYNTH_ROUTINE_CLASS
{
    /* start of the routine */
    const UINT64 start_pc;          

    /* static length, in syllables */
    const UINT32 slength;        

    /* dynamic length, in syllables */
    const UINT32 dlength;        

    /* true if on main path, false otherwise */
    const bool on_main_path;        

    /* count of syllables in this subroutine for the current invocation */
    UINT64 local_icount;

    /* current program counter for this "subroutine" */       
    UINT64 cur_pc;

    /* when syllable == SYLLABLES_PER_BUNDLE we are done with this bundle. */
    UINT32 syllable;

    /* We try to limit the i-space footprint by putting a branch in to the
     * i-stream periodically. The non_branch_ctr determines when we loop
     * back to the loop_target_pc. Counts down to zero (and below)  */
    INT32  non_branch_ctr;      

    bool leaf;
    
    UINT64 branch_off_path_pct;

  public:

    SYNTH_ROUTINE_CLASS(UINT64 br_off_path_pct,
                        UINT64 arg_start_pc,
                        UINT32 arg_static_len,  
                        UINT32 arg_dynamic_len,  
                        bool arg_leaf=false)           // CONS
        : start_pc(arg_start_pc),
          slength(arg_static_len),
          dlength(arg_dynamic_len),
          on_main_path( (arg_dynamic_len == 0) ),
          leaf(arg_leaf),
          branch_off_path_pct(br_off_path_pct)
    {
        BMSG("Synth routine at " << hex << start_pc <<
             " static-len= " << dec << slength <<
             " dyn-len= " << dec << dlength <<
             " leaf= " << leaf);

        reinit();
    }
    
    void reinit(void)
    {
        cur_pc =  start_pc;

        // The following is just so that done() returns true and we
        // manufacture a new event
        make_done(); // sets syllable

        local_icount =  0;
        emitted_branch(); // sets non_branch_ctr
    }

    inline void inc_pc(void)
    {
        cur_pc += BYTES_PER_BUNDLE;
    }
    inline  UINT64 get_pc(void) const
    {
        return cur_pc;
    }
    inline void set_pc(IADDR_CLASS arg_pc)
    {
        cur_pc   = arg_pc.GetBundleAddr();
        syllable = arg_pc.GetSyllableIndex();
    }
    inline void set_pc(UINT64 arg_pc)
    {
        cur_pc    = mask_bundle_addr(arg_pc);
        syllable  = 0;
    }

    inline bool need_to_branch_out(void) const
    {
        SMSG("leaf = " << leaf);
        return (!leaf) && (random_pct() < branch_off_path_pct);
    }
    
    inline bool need_to_return(void) const
    {
        SMSG("on_main_path = " << on_main_path << " icnt= " << local_icount);
        return !on_main_path && local_icount >= dlength;
    }

    inline void emitted_non_branch(void)
    {   // decrement counter when we emit non-branch ops
        local_icount++;
        non_branch_ctr--;
    }
    inline void emitted_branch(void)
    {   // reset the counter
        non_branch_ctr  =  slength;
    }
    inline bool need_branch_to_top(void) const
    {   // if we go negative, then we need to emit a branch.
        return (non_branch_ctr <= 0);
    }

    inline IADDR_CLASS get_ipf_pc(void) const
    {
        IADDR_CLASS pc_ia(cur_pc, syllable);
        return pc_ia;
    }
    
    inline UINT64 get_start_pc(void) const
    {
        return start_pc;
    }

    inline void set_syllable(UINT32 a=0)
    {
        XMSG("Setting syllable index to " << a);
        syllable=a;
    }
    inline UINT32 get_syllable(void) const
    {
        return syllable;
    }
    inline void inc_syllable(void)
    {
        syllable++;
    }
    inline void make_done(void)
    {
        set_syllable(SYLLABLES_PER_BUNDLE);
    }
    
};
typedef SYNTH_ROUTINE_CLASS* SYNTH_ROUTINE;


#endif
