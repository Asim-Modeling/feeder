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

/* genable-inst.h */
/* Mark Charney   <mark.charney@intel.com> */
/*$Id: genable-inst.h 799 2006-10-31 12:27:52Z cjbeckma $ */


#ifndef _GENABLE_INST_H_
# define _GENABLE_INST_H_

#include "asim/syntax.h"
#include "synth-inst.h"
#include "synth-util.h"

static const int invalid_reg = 999;

class GENABLE_INST_CLASS
{
    /* generate-able instructions for pre-defined sequences.
     Used to generate programmed SYNTH_INST_CLASS sequences. */

  public:
    /* qualifying predicate, compare output predicates p1 and p2 */

    SYNTH_INST_CLASS minst;

    int    qp; /* src */
    int    p1; /* dest */
    int    p2; /* dest */
    int    src1; /*src*/
    int    src2; /*src*/
    int    dst1; /*dest */
    INT32  imm;
    UINT64 branch_target;       /* branch target */


    GENABLE_INST_CLASS(UINT64 arg_pc,
                       UINT32 arg_syllable,
                       SYNTH_IPF_ENUM arg_type)   //CONS

        : minst(arg_pc,
                arg_syllable,
                arg_type),
          qp(invalid_reg), p1(invalid_reg), p2(invalid_reg),
          src1(invalid_reg), src2(invalid_reg),
          dst1(invalid_reg),
          imm(0),
          branch_target(0)
    {
        set_mii();
    } 
    

    inline UINT64
    GetBundleAddr(void) const
    {
        return minst.GetBundleAddr();
    }
    inline UINT64
    GetSyllableIndex(void) const
    {
        return minst.GetSyllableIndex();
    }

    bool
    flow_depends(GENABLE_INST_CLASS* younger) const
    {
        //FIXME: need to generalize this ...
        if ( p1 != invalid_reg && younger->qp == p1 )
        {
            return true;
        }
        if ( p2 != invalid_reg  && younger->qp == p2 )
        {
            return true;
        }
        if ( dst1 != invalid_reg  &&
             (younger->src1 == dst1 || younger->src2 == dst1) )
        {
            return true;
        } 
        return false;
    }
    ////////////////////////////////////////////////////
    void
    make_store(int src_reg, int addr_reg, int size)
    {
        IPF_RAW_SYLLABLE s;
        src1= src_reg;
        src2= addr_reg;
        s.SetStore(size, src_reg, addr_reg);
        minst.SetSyllable(s);
    }
    void
    make_store_release(int src_reg, int addr_reg, int size)
    {
        IPF_RAW_SYLLABLE s;
        src1= src_reg;
        src2= addr_reg;
        s.SetStoreRelease(size, src_reg, addr_reg);
        minst.SetSyllable(s);
    }

    void
    make_load(int dest_reg, int addr_reg, int size)
    {
        IPF_RAW_SYLLABLE s;
        dst1 = dest_reg;
        src2= addr_reg;
        s.SetLoad(size, dest_reg, addr_reg);
        minst.SetSyllable(s);
    }
    void 
    make_compare(int dest_preg1, int dest_preg2,
                      int src_reg1, int src_reg2)
    {
        IPF_RAW_SYLLABLE s;
        p1 = dest_preg1;
        p2 = dest_preg2;
        src1= src_reg1;
        src2= src_reg2;
        s.SetIntCompareEqRR(src_reg1, src_reg2, dest_preg1, dest_preg2);
        minst.SetSyllable(s);
    }
    void 
    make_compare_exchange(int dest_reg, 
                          int src_reg, int addr_reg, int sz)
    {
        IPF_RAW_SYLLABLE s;
        const bool release = false; /* acquire */
        src1= src_reg;
        src2= addr_reg;
        dst1 = dest_reg;
        s.SetCompareExchange(release, sz, dest_reg, src_reg, addr_reg);
        minst.SetSyllable(s);
    }

    void 
    make_xor(int dest_reg, 
             int src1_reg, int src2_reg)
    {
        IPF_RAW_SYLLABLE s;
        src1= src1_reg;
        src2= src2_reg;
        dst1 = dest_reg;
        s.SetXOR(dest_reg, src1_reg, src2_reg);
        minst.SetSyllable(s);
    }

    void 
    set_mii()
    {
        minst.SetTemplate(IPF_TEMPLATE_MII_STOP);
    }
    void 
    set_bbb()
    {
        minst.SetTemplate(IPF_TEMPLATE_BBB_STOP);
    }

    void 
    make_nop(SYNTH_IPF_ENUM stype = SYNTH_IPF_NOPI)
    {
        IPF_RAW_SYLLABLE s;
        switch( stype )
        {
          case SYNTH_IPF_NOPF:
            s.SetNopFunit();
            break;
          case SYNTH_IPF_NOPI:
            s.SetNopIunit();
            break;
          case SYNTH_IPF_NOPM:
            s.SetNopMunit(); //FIXME
            break;
          case SYNTH_IPF_NOPB:
            s.SetNopBunit();
            break;
          default:
            ASSERTX(0);
        }
        minst.SetSyllable(s);
    }
    void 
    make_relative_branch(UINT64 target,
                              int qualifying_preg)
    {
        IPF_RAW_SYLLABLE s;
        qp = qualifying_preg;
        branch_target = target;
        INT32 imm21 =  target - GetBundleAddr();
        SMSG("\tRelative branch immediate = " << hex << imm21 << dec << " base10 = " << imm21);
        SMSG("\tRelative branch       tgt = " << hex << target << dec );
        SMSG("\tRelative branch        pc = " << hex << GetBundleAddr() << dec );
        s.SetCBranch(imm21>>4);
        s.SetPredicateReg(qualifying_preg);
        minst.SetSyllable(s);
        set_bbb();
    }

    inline bool
    isPredicated(void) const
    {
        return qp != 0;
    }
        
    ////////////////////////////////////////////////////
    
    SYNTH_INST get_synth_inst(void)
    {
        return &minst;
    }
};



typedef GENABLE_INST_CLASS* GENABLE_BUNDLE[SYLLABLES_PER_BUNDLE];

////////////////////////////////////////////////////////////////////////////

class GENABLE_PROGRAM_CLASS
{
    const UINT64 start_pc;
    const unsigned int nbundles;
    GENABLE_BUNDLE* program;

  public:
    GENABLE_PROGRAM_CLASS(UINT64 arg_pc,         //CONS
                          UINT64 arg_nbundles)
        : start_pc(arg_pc),
          nbundles(arg_nbundles)
    {
        program = new GENABLE_BUNDLE[nbundles];

        zero_program();
    }

    ~GENABLE_PROGRAM_CLASS()
    {
        delete [] program;
    }

    void
    zero_program(void)
    {
        for(unsigned int i=0; i< nbundles ;i++)
        {
            for(unsigned int j=0 ; j< SYLLABLES_PER_BUNDLE ; j++)
            {
                program[i][j] = 0;
            }
        }
    }

    bool
    in_range(UINT64 pc, unsigned int syllable_idx) const
    {
        const UINT64 idx = (pc - start_pc) / BYTES_PER_BUNDLE;
        if (pc >= start_pc &&
            pc < (start_pc + nbundles * BYTES_PER_BUNDLE))
        {
            if (program[idx][syllable_idx])
            {
                return true;
            }
        }
        return false;
    }
    
    void
    add_inst(GENABLE_INST_CLASS* c)
    {
        const UINT64 pc = c->GetBundleAddr();
        const UINT64 syl_idx = c->GetSyllableIndex();
        const UINT64 idx = (pc - start_pc) / BYTES_PER_BUNDLE;

        SMSG("Adding inst at " << hex << pc << dec << " offset: " << idx << " syl: " << syl_idx);
        assert(syl_idx < SYLLABLES_PER_BUNDLE);
        assert(pc >= start_pc);
        assert(pc < start_pc+ nbundles * BYTES_PER_BUNDLE);
        assert(program[idx][syl_idx] == 0);

        program[idx][syl_idx] = c;
    }

    GENABLE_INST_CLASS*
    get_inst(const UINT64 pc,
             const unsigned int syllable_idx) const
    {
        const UINT64 idx = (pc - start_pc) / BYTES_PER_BUNDLE;

        assert(syllable_idx < SYLLABLES_PER_BUNDLE);
        assert(pc >= start_pc);
        assert(pc < start_pc + nbundles * BYTES_PER_BUNDLE);
        assert(program[idx][syllable_idx] != 0);
        
        return program[idx][syllable_idx];
    }
    
};

GENABLE_PROGRAM_CLASS*
make_synchronization_routine(UINT64 arg_pc,
                             int holding_delay_bundles,
                             int reentry_delay_bundles);

#endif
//Local Variables:
//pref: "genable-inst.cpp"
//End:
