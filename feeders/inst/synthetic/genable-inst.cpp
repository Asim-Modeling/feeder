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

// genable-inst.cpp
// Mark Charney   <mark.charney@intel.com>
// $Id: genable-inst.cpp 799 2006-10-31 12:27:52Z cjbeckma $


////////////////////////////////////////////////////////////////////////////
#include "synth-debug.h"
#include "synth-types.h"
#include "genable-inst.h"
#include "genable-inst.h"
#include "synth-ipf-pc.h"
////////////////////////////////////////////////////////////////////////////

static GENABLE_INST_CLASS*
mknop(const IPF_PC_CLASS& pc,
      const SYNTH_IPF_ENUM stype)
{
    GENABLE_INST_CLASS* s;

    s = new GENABLE_INST_CLASS( pc.pc,
                                pc.syllable,
                                stype );
    s->make_nop( stype );
    return s;
}
static void
add_cmp(GENABLE_PROGRAM_CLASS* program,
        IPF_PC_CLASS& pc)
{
    int src1 = 1; /* cmpxch dest reg */
    int src2 = 4;
    int p1 =  2;
    int p2 =  3;
    
    GENABLE_INST_CLASS* s;

    s = new GENABLE_INST_CLASS(pc.pc,
                               pc.syllable,
                               SYNTH_IPF_COMPARE);
    
    s->make_compare(p1,p2,src1,src2);
    
    program->add_inst(s);
    pc.inc();
}

static void
add_load_test(GENABLE_PROGRAM_CLASS* program,
              IPF_PC_CLASS& pc)
{
    int dest_reg = 1;
    int mem_reg = 3;
    int size = 1;

    UINT64 mem_addr = 0;
    
    GENABLE_INST_CLASS* s = new GENABLE_INST_CLASS(pc.pc,
                                                   pc.syllable,
                                                   SYNTH_IPF_LOAD);
    s->make_load(dest_reg,
                 mem_reg,
                 size);
    
    s->minst.SetMemopInfo(mem_addr, size);
    
    program->add_inst(s);
    pc.inc();
}

static void
add_cmpxch(GENABLE_PROGRAM_CLASS* program,
           IPF_PC_CLASS& pc)
{
    int cmpxch_dest_reg = 1;
    int cmpxch_src_reg = 2;
    int cmpxch_mem_reg = 3;
    int cmpxch_size = 1;
    int cmpxch_value_to_store = -1;
    UINT64 mem_addr = 0;
    
    GENABLE_INST_CLASS* s;

    s = new GENABLE_INST_CLASS(pc.pc,
                               pc.syllable,
                               SYNTH_IPF_COMPARE_EXCHANGE);

    s->make_compare_exchange(cmpxch_dest_reg,
                             cmpxch_src_reg,
                             cmpxch_mem_reg, cmpxch_size);
    
    s->minst.SetMemopInfo(mem_addr, cmpxch_size);
    
    // Set the value stored. The mem_addr is masked to position the value
    // properly.
    s->minst.value.set32(mem_addr, cmpxch_value_to_store);
    
    program->add_inst(s);
    pc.inc();
} /* add_cmpxch */

static void
add_xor(GENABLE_PROGRAM_CLASS* program,
        IPF_PC_CLASS& pc)
{
    GENABLE_INST_CLASS* s;
    
    s = new GENABLE_INST_CLASS(pc.pc,
                               pc.syllable,
                               SYNTH_IPF_XOR);
    
    int src1 = 1;
    int src2 = 2;
    int dest = 3; /*FIXME: do I need multiple dests? */
    s->make_xor(dest,src1,src2);

    program->add_inst(s);
    pc.inc();
}

static void
add_xor_bundle(GENABLE_PROGRAM_CLASS* program,
               IPF_PC_CLASS& pc)
{
    add_xor(program, pc);
    add_xor(program, pc);
    add_xor(program, pc);
}

static void
add_branch(GENABLE_PROGRAM_CLASS* program,
           IPF_PC_CLASS& pc,
           IPF_PC_CLASS& target_pc, int qp=0)
{
    GENABLE_INST_CLASS* s;
    
    s = new GENABLE_INST_CLASS(pc.pc,
                               pc.syllable,
                               SYNTH_IPF_BRANCH);
    
    s->make_relative_branch(target_pc.pc, qp);
    s->minst.SetBranchInfo(false,target_pc.pc); // the 'false' is just temporary.
    // we finalized the branch state when we know the predicate value.
    
    /* don't set branch info until we know
       the values */
    
    program->add_inst(s);
    
    pc.inc();
}

static void
add_store_release(GENABLE_PROGRAM_CLASS* program,
                  IPF_PC_CLASS& pc)
{
    int store_src_reg = 1;
    int store_addr_reg = 2;
    int store_size = 1;
    int value_to_store = 0;
    UINT64 mem_addr = 0;
    
    GENABLE_INST_CLASS* s;

    s = new GENABLE_INST_CLASS(pc.pc,
                               pc.syllable,
                               SYNTH_IPF_STORE);
    
    s->make_store_release(store_src_reg,
                          store_addr_reg,
                          store_size);
    
    s->minst.SetMemopInfo(mem_addr, store_size);
    
    // Set the value stored. The mem_addr is masked to position the value
    // properly.
    s->minst.value.set32(mem_addr, value_to_store);

    program->add_inst(s);

    pc.inc();
}

static void
fill_rest_with_nops(GENABLE_PROGRAM_CLASS* program,
                    IPF_PC_CLASS& pc,
                    SYNTH_IPF_ENUM type)
{
    GENABLE_INST_CLASS* s;

    // make two nops to end the bundles
    s = mknop(pc, type);
    if (type == SYNTH_IPF_NOPB)
    {
        s->set_bbb();
    }
    program->add_inst(s);
    pc.inc();
    
    s = mknop(pc, type);
    if (type == SYNTH_IPF_NOPB)
    {
        s->set_bbb();
    }
    program->add_inst(s);
    pc.inc();
}



GENABLE_PROGRAM_CLASS*
make_synchronization_routine(UINT64 arg_pc,
                             int holding_delay_bundles,
                             int reentry_delay_bundles)
{
    IPF_PC_CLASS pc(arg_pc);
    IPF_PC_CLASS start_pc = pc;
    const int qp0 = 0;
    const int qp2 = 2;

    const unsigned int max_program_bundles = 3 + 5 + holding_delay_bundles + reentry_delay_bundles; 

    GENABLE_PROGRAM_CLASS* program =
        new GENABLE_PROGRAM_CLASS(arg_pc,
                                  max_program_bundles);

    BMSG("Making synthetic routine at " << start_pc );

    /* first 3 bundles make pre-test to see if the lock is free */
    add_load_test(program,pc);
    fill_rest_with_nops(program, pc, SYNTH_IPF_NOPI);

    add_cmp(program,pc);
    fill_rest_with_nops(program, pc, SYNTH_IPF_NOPI);

    add_branch(program, pc, start_pc, qp2);
    fill_rest_with_nops(program, pc, SYNTH_IPF_NOPB);

    ////////////////////////////////////////////////////////
    add_cmpxch(program,pc);
    // make two nops to end the bundles
    fill_rest_with_nops(program, pc, SYNTH_IPF_NOPI);
    ////////////////////////////////////////////////////////

    add_cmp(program,pc);
    fill_rest_with_nops(program, pc, SYNTH_IPF_NOPI);
    ////////////////////////////////////////////////////////


    add_branch(program, pc, start_pc, qp2);
    fill_rest_with_nops(program, pc, SYNTH_IPF_NOPB);

    ////////////////////////////////////////////////////////

    for ( int i = 0 ; i < holding_delay_bundles ; i++ ) 
    {
        add_xor_bundle(program, pc);
    }
    
    
    ////////////////////////////////////////////////////////

    add_store_release(program, pc);
    // make two nops to end the bundles
    fill_rest_with_nops(program,pc,SYNTH_IPF_NOPI);

    ////////////////////////////////////////////////////////

    for ( int i = 0 ; i < reentry_delay_bundles ; i++ ) 
    {
        add_xor_bundle(program, pc);
    }

    ////////////////////////////////////////////////////////

    add_branch(program, pc, start_pc, qp0); /* always taken! */
    fill_rest_with_nops(program, pc, SYNTH_IPF_NOPB);

    ////////////////////////////////////////////////////////



    return program;
}
