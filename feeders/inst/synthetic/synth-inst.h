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

/* synth-inst.h */
/* Mark Charney   <mark.charney@intel.com> */
/*$Id: synth-inst.h 799 2006-10-31 12:27:52Z cjbeckma $ */


#ifndef _SYNTH_INST_H_
# define _SYNTH_INST_H_

#include "asim/syntax.h"
#include "synth-types.h"
#include "asim/provides/memory_value_model.h"

/* the magic uid is used to realize an instruction is
   going in to the ASIM pipeline for the first time */
static const UINT64 magic_uid = (~0ULL); 


class SYNTH_INST_CLASS : public REG_DEST_DIST_CLASS
{
    /* This is the internal representation of instructions on the correct
     * path.
     */

  public:

    SYNTH_INST_CLASS(UINT64 arg_pc,
                     UINT32 arg_syllable = 0,
                     SYNTH_IPF_ENUM arg_type = SYNTH_IPF_INVALID)  // CONS
        : REG_DEST_DIST_CLASS(),
          type(arg_type),
          uid(magic_uid),
          pc(arg_pc,arg_syllable),
          ea(0),
          access_size(0),
          taken(false),
          target(0),
          iteration(0),
          conditional_branch(false)
    {} 
    
    SYNTH_INST_CLASS(const IADDR_CLASS& arg_pc,
                     SYNTH_IPF_ENUM arg_type = SYNTH_IPF_INVALID)  // CONS
        : type(arg_type),
          uid(magic_uid),
          pc(arg_pc),
          ea(0),
          access_size(0),
          taken(false),
          target(0),
          iteration(0),
          conditional_branch(false)
    {} 
    
    IPF_RAW_BUNDLE bundle;
    SYNTH_IPF_ENUM type;

    /* uid is the id of the ASIM_INST -- handy for finding this data
     * structure later on. */
    UINT64      uid; 

    IADDR_CLASS pc;    

    UINT64      ea;
    UINT8       access_size;

    bool        taken;
    IADDR_CLASS target;

    UWORD128 value; 

    ////////////////////////////////////////////////////////////////
    /* used by cmpxch and a dependent branch */
    unsigned int iteration;

    bool conditional_branch;
    ////////////////////////////////////////////////////////////////

    inline void
    SetTemplate(IPF_TEMPLATE_ENUM tmplt)
    {
        bundle.SetTemplate(tmplt);
    }
        
    inline void
    SetSyllable(const IPF_RAW_SYLLABLE& s)
    {
        bundle.SetSlot(pc.GetSyllableIndex(), s);
    }

    inline void
    SetBundle(const IPF_RAW_BUNDLE& b)
    {
        bundle = b;
    }
    
    inline UINT64
    GetBundleAddr(void) const
    {
        return pc.GetBundleAddr();
    }

    inline UINT32
    GetSyllableIndex(void) const
    {
        return pc.GetSyllableIndex();
    }
    
    inline void
    SetMemopInfo(UINT64 arg_ea, UINT8 arg_sz)
    {
        ea = arg_ea;
        access_size = arg_sz; //FIXME: validate input values for size
    }
    inline void
    SetBranchInfo(bool arg_taken, const IADDR_CLASS& arg_target)
    {
        taken = arg_taken;
        target = arg_target;
    }

    inline void
    SetConditionalBranch(bool arg_conditional)
    {
        conditional_branch = arg_conditional;
    }

    inline void
    SetBranchInfo(bool arg_taken, const UINT64& arg_target)
    {
        taken = arg_taken;
        target.Set(arg_target,0);
    }

    inline void
    SetUid(UINT64 arg_uid)
    {
        uid = arg_uid;
    }
    inline UINT64
    GetUid(void) const
    {
        return uid;
    }

    inline bool
    isNOP(void) const
    {
        return (type == SYNTH_IPF_NOPI ||
                type == SYNTH_IPF_NOPB || 
                type == SYNTH_IPF_NOPF || 
                type == SYNTH_IPF_NOPM );
    }

    inline bool
    isCompareExchange(void) const
    {
        return type == SYNTH_IPF_COMPARE_EXCHANGE;
    }


    inline bool
    isLoad(void) const
    {
        return type == SYNTH_IPF_LOAD;
    }


    inline bool
    isBranch(void) const
    {
        return type == SYNTH_IPF_BRANCH ||
               type == SYNTH_IPF_LONG_BRANCH;
    }

    inline bool
    isConditionalBranch(void) const
    {
        return conditional_branch;
    }

}; /* class SYNTH_INST_CLASS */

typedef SYNTH_INST_CLASS *SYNTH_INST;

typedef list< SYNTH_INST >::iterator SYNTH_INST_ITER;

#endif

