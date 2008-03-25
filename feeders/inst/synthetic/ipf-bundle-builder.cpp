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

/* ipf-bundle-builder.cpp */
/* Mark Charney   <mark.charney@intel.com> */
/*$Id: ipf-bundle-builder.cpp 799 2006-10-31 12:27:52Z cjbeckma $ */

#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/trace.h"

#include "ipf-bundle-builder.h"

extern SYNTH_PARAMS params;

SYNTH_IPF_ENUM
IPF_INST_BUILDER_CLASS::fi_map(SYNTH_FTYPE f,
                               IPF_SLOT_ENUM s,
                               SYNTH_ETYPE event)
{
    AMSG("FI_MAP: ftype= " << f << " slot= " << s << " etype= " << event);
    SYNTH_IPF_ENUM x = SYNTH_IPF_INVALID;

    switch(f)
    {
      case SYNTH_FTYPE_MEMOP:
        switch(event)
        {
          case SYNTH_ETYPE_LOAD:
            x = SYNTH_IPF_LOAD;
            break;
          case SYNTH_ETYPE_STORE:
            x = SYNTH_IPF_STORE;
            break;
          case SYNTH_ETYPE_EXCHANGE:
            x = SYNTH_IPF_EXCHANGE;
            break;
          case SYNTH_ETYPE_COMPARE_EXCHANGE:
            x = SYNTH_IPF_COMPARE_EXCHANGE;
            break;
          case SYNTH_ETYPE_FETCH_AND_ADD:
            x = SYNTH_IPF_FETCH_AND_ADD;
            break;
          case SYNTH_ETYPE_LFETCH:
            x = SYNTH_IPF_LFETCH;
            break;
          default:
            assert(0);
        }
        break;
      case SYNTH_FTYPE_LOAD:
        x = SYNTH_IPF_LOAD;
        break;
      case SYNTH_FTYPE_STORE:
        x = SYNTH_IPF_STORE;
        break;
      case SYNTH_FTYPE_EXCHANGE:
        x = SYNTH_IPF_EXCHANGE;
        break;
      case SYNTH_FTYPE_COMPARE_EXCHANGE:
        x = SYNTH_IPF_COMPARE_EXCHANGE;
        break;
      case SYNTH_FTYPE_FETCH_AND_ADD:
        x = SYNTH_IPF_FETCH_AND_ADD;
        break;
      case SYNTH_FTYPE_FMA:
        x = SYNTH_IPF_FMA;
        break;
      case SYNTH_FTYPE_COMPARE:
        x = SYNTH_IPF_COMPARE;
        break;
      case SYNTH_FTYPE_NOP:
        switch(s)
        {
          case IPF_SLOT_M:
            x = SYNTH_IPF_NOPM;
            break;
          case IPF_SLOT_F:
            x = SYNTH_IPF_NOPF;
            break;
          case IPF_SLOT_B:
            x = SYNTH_IPF_NOPB;
            break;
          case IPF_SLOT_X: // [sic] 
          case IPF_SLOT_I:
            x = SYNTH_IPF_NOPI;
            break;
          default:
            assert(0);
        }
        break;
      case SYNTH_FTYPE_XOR:
        x = SYNTH_IPF_XOR;
        break;
      case SYNTH_FTYPE_LFETCH:
        x = SYNTH_IPF_LFETCH;
        break;
      case SYNTH_FTYPE_FENCE:
      case SYNTH_FTYPE_DEPENDENT_READ:
      case SYNTH_FTYPE_BRANCH:
      case SYNTH_FTYPE_LONG_BRANCH:
      case SYNTH_FTYPE_LIMM:
      default:
        assert(0);
    }
    return x;
}

IPF_TEMPLATE_ENUM
IPF_INST_BUILDER_CLASS::refine_to_syllables(SYNTH_BUNDLE_ENUM bundle_type,
                                            SYNTH_MEMOP* marray,
                                            SYNTH_IPF_ENUM* ipf_syllables /* output */)
{
    AMSG("refining event");
    /* refine the memop_syllables to "real" IPF syllables, using event for
     * the memop. */

    SYNTH_FTYPE* ftype_syllables = 0;
    IPF_TEMPLATE_ENUM tmplt = IPF_TEMPLATE_INVALID;

    switch(bundle_type)
    {
      case SYNTH_BUNDLE_EVEN:
        ftype_syllables = even_syllables;
        tmplt = even_template;
        break;
      case SYNTH_BUNDLE_ODD:
        ftype_syllables = odd_syllables;
        tmplt = odd_template;
        break;
      case SYNTH_BUNDLE_MEMOP:
        ftype_syllables = memop_syllables;
        tmplt = memop_template;
        break;
      default:
        ASSERTX(0);
    }

    for(UINT32 i=0; i < SYLLABLES_PER_BUNDLE ; i++ )
    {
        const SYNTH_FTYPE f = ftype_syllables[i];
        const IPF_SLOT_ENUM s = slot_map[tmplt][i];
        const SYNTH_IPF_ENUM x =  fi_map(f,s,marray[i].etype);
        AMSG("  FI_MAP returns= " << x);
        assert(x != SYNTH_IPF_INVALID);
        ipf_syllables[i] = x;
    }

    return tmplt;
}


SYNTH_FTYPE
IPF_INST_BUILDER_CLASS::convert_syllable_code(char c)
{
    //FIXME: convert to a table of variable length strings.

    switch(c)
    {
      case 'M':
        return SYNTH_FTYPE_MEMOP; // NOTE: could be load, store, or exchange
      case 'L':
        return SYNTH_FTYPE_LOAD;
      case 'S':
        return SYNTH_FTYPE_STORE;
      case 'E':
        return SYNTH_FTYPE_EXCHANGE;
      case 'C':
        return SYNTH_FTYPE_COMPARE_EXCHANGE;
      case 'D':
        return SYNTH_FTYPE_COMPARE;
      case 'X':
        return SYNTH_FTYPE_XOR;
      case 'N':
        return SYNTH_FTYPE_NOP; // NOTE: could be NOPF, NOPI, NOPM, or NOPB
      case 'F':
        return SYNTH_FTYPE_FMA;
      case 'I':
        return SYNTH_FTYPE_FETCH_AND_ADD;
      case 'P':
        return SYNTH_FTYPE_LFETCH;
      default:
        cerr << "Bad character in bundle parameter: [" << c << "]" << endl;
        exit(1);
        break;
    }
    
}

void
IPF_INST_BUILDER_CLASS::walk_bundle_string(const char* s,
                                           SYNTH_FTYPE* syllables)
{
    for(UINT i=0; i < SYLLABLES_PER_BUNDLE; i++) 
    {
        SYNTH_FTYPE code = convert_syllable_code(s[i]);
        syllables[i] = code;
    }
}


/////////////////////////////////////////////////////////////////////////

void
IPF_INST_BUILDER_CLASS::init_op_map(void)
{
    for(UINT32 i = 0 ; i < SYNTH_FTYPE_LAST; i++)
    {
        fop_count[i] = 0;
        for(UINT32 j = 0 ; j < MAX_SLOTS_PER_TYPE; j++)
        {
            fop_map[i][j] = IPF_SLOT_INVALID;
        }
    }
}

void
IPF_INST_BUILDER_CLASS::add_to_fop_map(SYNTH_FTYPE f,
                                       IPF_SLOT_ENUM sl)
{
    UINT32 c = fop_count[f];
    fop_map[f][c] = sl;
    fop_count[f] = c+1;
    assert(fop_count[f] <= MAX_SLOTS_PER_TYPE);
}


void
IPF_INST_BUILDER_CLASS::init_syllable_mapping(void)
{
#define DEFSLOT(t,x,y,z) ({ \
     slot_map[ t ][0] = IPF_SLOT_ ## x ; \
     slot_map[ t ][1] = IPF_SLOT_ ## y ; \
     slot_map[ t ][2] = IPF_SLOT_ ## z ; \
})

    for(UINT32 i=0;i<IPF_TEMPLATE_LAST;i++)
    {
        DEFSLOT(i,LAST,LAST,LAST);
    }

    DEFSLOT(IPF_TEMPLATE_MII,M,I,I);
    DEFSLOT(IPF_TEMPLATE_MII_STOP,M,I,I);
    DEFSLOT(IPF_TEMPLATE_MLX,M,L,X);
    DEFSLOT(IPF_TEMPLATE_MLX_STOP,M,L,X);
    DEFSLOT(IPF_TEMPLATE_MMI,M,M,I);
    DEFSLOT(IPF_TEMPLATE_MMI_STOP,M,M,I);
    DEFSLOT(IPF_TEMPLATE_MFI,M,F,I);
    DEFSLOT(IPF_TEMPLATE_MFI_STOP,M,F,I);
    DEFSLOT(IPF_TEMPLATE_MMF,M,M,F);
    DEFSLOT(IPF_TEMPLATE_MMF_STOP,M,M,F);
    DEFSLOT(IPF_TEMPLATE_BBB,B,B,B);
    DEFSLOT(IPF_TEMPLATE_BBB_STOP,B,B,B);
#undef DEFSLOT
    

    /// DO THE op_map

    /* There are multiple slots that an IPF instruction
     * can live in. This sets up the op_map that indicates
     * where they can live.
     */

    /// NOW DO THE fop_map

    add_to_fop_map(SYNTH_FTYPE_NOP, IPF_SLOT_M);
    add_to_fop_map(SYNTH_FTYPE_NOP, IPF_SLOT_I);
    add_to_fop_map(SYNTH_FTYPE_NOP, IPF_SLOT_B);
    add_to_fop_map(SYNTH_FTYPE_NOP, IPF_SLOT_X); // [sic] NOPI's

    add_to_fop_map(SYNTH_FTYPE_XOR, IPF_SLOT_I);
    add_to_fop_map(SYNTH_FTYPE_XOR, IPF_SLOT_M);

    add_to_fop_map(SYNTH_FTYPE_MEMOP, IPF_SLOT_M);

    add_to_fop_map(SYNTH_FTYPE_LOAD, IPF_SLOT_M);
    add_to_fop_map(SYNTH_FTYPE_STORE, IPF_SLOT_M);
    add_to_fop_map(SYNTH_FTYPE_EXCHANGE, IPF_SLOT_M);
    add_to_fop_map(SYNTH_FTYPE_COMPARE_EXCHANGE, IPF_SLOT_M);
    add_to_fop_map(SYNTH_FTYPE_FENCE, IPF_SLOT_M);
    add_to_fop_map(SYNTH_FTYPE_FETCH_AND_ADD, IPF_SLOT_M);
    add_to_fop_map(SYNTH_FTYPE_LFETCH, IPF_SLOT_M);

    add_to_fop_map(SYNTH_FTYPE_BRANCH, IPF_SLOT_B);
    add_to_fop_map(SYNTH_FTYPE_LONG_BRANCH, IPF_SLOT_X);
    add_to_fop_map(SYNTH_FTYPE_LIMM, IPF_SLOT_L);
    add_to_fop_map(SYNTH_FTYPE_FMA, IPF_SLOT_F);

    add_to_fop_map(SYNTH_FTYPE_COMPARE, IPF_SLOT_M);
    add_to_fop_map(SYNTH_FTYPE_COMPARE, IPF_SLOT_I);

}

void
IPF_INST_BUILDER_CLASS::pick_template(list<IPF_TEMPLATE_ENUM>* tlist,
                                      SYNTH_FTYPE* array)
{
    /* walk through the array and the slot mapping to figure out what
       template match each slot. */

    /* build a list of all templates -- later we winnow it */
    /* FIXME: make this more flexible  */
    tlist->push_front(IPF_TEMPLATE_MII);
    tlist->push_front(IPF_TEMPLATE_MLX);
    tlist->push_front(IPF_TEMPLATE_MMI);
    tlist->push_front(IPF_TEMPLATE_MFI);
    tlist->push_front(IPF_TEMPLATE_MMF);
    tlist->push_front(IPF_TEMPLATE_BBB);
    INT32 tt = 6;

    /* walk the syllables, winnowing out the templates that don't give the
     * required slots.  */

    for(UINT32 i=0; i < SYLLABLES_PER_BUNDLE ; i++) 
    {
        SYNTH_FTYPE ftype = array[i];

        /* build a list of bundle slot types that this ftype uses */
        list<IPF_SLOT_ENUM> eligible_slots; 
        for(UINT32 j=0; j < fop_count[ftype]; j++)
        {
            eligible_slots.push_front(fop_map[ftype][j]);
        }

        /* remove things from the templates tlist, if they don't have the
           right slots for thsi syllable. */

        list<IPF_TEMPLATE_ENUM>::iterator k = tlist->begin();
        while( k != tlist->end() )
        {
            IPF_TEMPLATE_ENUM x = *k; /* our first template is x */

            /* s is the slot the x template uses in the i-th position */
            IPF_SLOT_ENUM s = slot_map[x][i];

            // remove x from template list if s is not in the
            // eligible_slots list.

            // search slots list for s
            list<IPF_SLOT_ENUM>::iterator m = find(eligible_slots.begin(),
                                                   eligible_slots.end(),
                                                   s);
            if (m == eligible_slots.end())
            {
                /* The slot s was not found in slots list.  So remove x
                   (pointed to by k) from templates tlist list. */

                /* NOTE: function args are evaluated first -- k is
                   post-incremented, but the old value of k is passed in to
                   erase(). This is from the STL book. I'm speechless... */
                tlist->erase(k++); 

                tt--;
                if (tt == 0)
                { // we ran out of things on the templates list
                    return;
                }
                continue; // skip the increment of k;
            }
            
            k++;
        } /* while */
        
    }
}

void
IPF_INST_BUILDER_CLASS::satisfied(list<IPF_TEMPLATE_ENUM>* tlist, const char* str)
{
    if (tlist->empty())
    {
        cerr << "ERROR: "
             << " The synthetic feeder was unable to find a satisfactory template for "
             << str << endl;
        exit(1);
    }
}


void
IPF_INST_BUILDER_CLASS::init(std::string even_str,
                             std::string odd_str,
                             std::string memop_str)
{
    AMSG("IPF_INST_BUILDER_CLASS::init");

    AMSG("  init_op_map");
    init_op_map();
    AMSG("  init_syllable_mapping");
    init_syllable_mapping();

    /*
     * Set up the arrays that hold the syllables of interest
     * based on the dynamic parameters.
     */
    
    AMSG("  walk_bundle_string");
    walk_bundle_string(even_str.c_str(),  even_syllables);
    walk_bundle_string(odd_str.c_str(),   odd_syllables);
    walk_bundle_string(memop_str.c_str(), memop_syllables);



    /* set up the syllable sequence based on the string arrays */

    /* set up the templates based on the string arrays */
    list<IPF_TEMPLATE_ENUM> even_templates;
    list<IPF_TEMPLATE_ENUM> odd_templates;
    list<IPF_TEMPLATE_ENUM> memop_templates;

    AMSG("  pick_template");
    pick_template(&even_templates, even_syllables);
    pick_template(&odd_templates, odd_syllables);
    pick_template(&memop_templates, memop_syllables);

    /* make sure we have at least one eligible template */
    AMSG("  satisfied");
    satisfied(&even_templates, "SYNTH_EVEN_BUNDLE");
    satisfied(&odd_templates, "SYNTH_ODD_BUNDLE");
    satisfied(&memop_templates, "SYNTH_MEMOP_BUNDLE");

    /* select the first template that matched for each*/
    AMSG("  select first");
    memop_template  = memop_templates.front();
    even_template   = even_templates.front();
    odd_template    = odd_templates.front();

    /* Now that we have our syllables arrays and our templates, we can go
       generate some instructions. */
}


/////////////////////////////////////////////////////////////////////

void
set_bundle_bits(IPF_RAW_BUNDLE* bundle,
                IADDR_CLASS     pc,
                ASIM_INST       inst)
{

    /* in ASIM, an ASIM_INST is a syllable, not a bundle, so we must point
      to the proper syllable when we do the Init() call. */
    inst->Init(bundle, pc);
    
}


void
HandleJunkOps(SYNTH_THREAD  thread,
              IPF_RAW_BUNDLE* bundle,
              const IPF_TEMPLATE_ENUM tmplt,
              const SYNTH_IPF_ENUM* ipf_syllables)
{
    /* Set the registers and fill in bundle based on tmplt and
     * ipf_syllables. */

    SMSG("HandleJunkOps: tmplt= " << tmplt);

    IPF_RAW_SYLLABLE syllable[SYLLABLES_PER_BUNDLE];

    for(UINT32 i=0;i<SYLLABLES_PER_BUNDLE;i++)
    {
        SYNTH_IPF_ENUM btype = ipf_syllables[i];

        switch(btype)
        {
          case SYNTH_IPF_XOR_DEPENDENT:
            {
                const UINT32 r1= thread->get_next_xor_dest_reg();
                const UINT32 src_reg = thread->get_last_dest_reg();
                SMSG("XOR_DEP: Src_reg = " << src_reg);
                syllable[i].SetXOR(r1,src_reg,src_reg);
            }
            break;
          case SYNTH_IPF_XOR:
            {
                const UINT32 r1= thread->get_next_xor_dest_reg();
                const UINT32 r2=1;
                const UINT32 r3=2;
                syllable[i].SetXOR(r1,r2,r3);
            }
            break;
          case SYNTH_IPF_FMA:
            {
                const UINT32 f1=1; 
                const UINT32 f2=2;
                const UINT32 f3=3;
                const UINT32 fdest = thread->get_next_fp_dest_reg();
                syllable[i].SetFMADouble(fdest,f3,f2,f1);
            }
            break;
          case SYNTH_IPF_COMPARE:
            {
                const UINT32 p1=2; 
                const UINT32 p2=3;
                const UINT32 r2=3;
                const UINT32 r3=4;
                syllable[i].SetIntCompareEqRR(r2,r3,p1,p2);
            }
            break;
          case SYNTH_IPF_NOPF:
            syllable[i].SetNopFunit();
            break;
          case SYNTH_IPF_NOPI:
            syllable[i].SetNopIunit();
            break;
          case SYNTH_IPF_NOPM:
            syllable[i].SetNopMunit();
            break;
          case SYNTH_IPF_NOPB:
            syllable[i].SetNopBunit();
            break;
          default:
            cerr << "Unhandled SYNTH_IPF_* type " << btype << endl;
            assert(0);
            break;
        }
    }

    /* Finally, take the syllable and template and place it in the
     * bundle. */

    bundle->SetTemplate( tmplt );
    bundle->SetSlots(syllable);
}

////////////////////////////////////////////////////////////////////////////

void
HandleMemoryRW(SYNTH_THREAD  thread,
               IPF_TEMPLATE_ENUM tmplt,
               SYNTH_PARAMS params)
{
    /* sets the bundle in the thread, bind registers */

    SMSG("Memory RW: template= " << tmplt);


    IPF_RAW_BUNDLE bundle;
    IPF_RAW_SYLLABLE syllable[SYLLABLES_PER_BUNDLE];

    for(UINT32 i=0;i<SYLLABLES_PER_BUNDLE;i++)
    {
        const SYNTH_IPF_ENUM stype = thread->get_syllable(i);
        SYNTH_MEMOP* m = thread->get_memop(i);
        const UINT32 sz = m->sz;

        switch(stype)
        {
          case SYNTH_IPF_XOR_DEPENDENT:
            {
                const UINT32 r1= thread->get_next_xor_dest_reg();
                const UINT32 src_reg = thread->get_last_dest_reg();
                SMSG("XOR_DEP  Src_reg = " << src_reg);
                syllable[i].SetXOR(r1,src_reg,src_reg);
            }
            break;
          case SYNTH_IPF_XOR:
            {
                const UINT32 r1= thread->get_next_xor_dest_reg();
                const UINT32 r2=1;
                const UINT32 r3=2;
                syllable[i].SetXOR(r1,r2,r3);
            }
            break;
          case SYNTH_IPF_NOPF:
            syllable[i].SetNopFunit();
            break;
          case SYNTH_IPF_NOPI:
            syllable[i].SetNopIunit();
            break;
          case SYNTH_IPF_NOPM:
            syllable[i].SetNopMunit();
            break;
          case SYNTH_IPF_NOPB:
            syllable[i].SetNopBunit();
            break;
          case SYNTH_IPF_FENCE:
            syllable[i].SetFence();
            break;
          case SYNTH_IPF_FMA:
            {
                const UINT32 f1=1; 
                const UINT32 f2=2;
                const UINT32 f3=3;
                const UINT32 fdest = thread->get_next_fp_dest_reg();
                syllable[i].SetFMADouble(fdest,f3,f2,f1);
            }
            break;
          case SYNTH_IPF_COMPARE:
            {
                const UINT32 p1=2; 
                const UINT32 p2=3;
                const UINT32 r2=3;
                const UINT32 r3=4;
                syllable[i].SetIntCompareEqRR(r2,r3,p1,p2);
            }
            break;
          case SYNTH_IPF_LFETCH:
            {
                UINT32 r3=1; //source reg
                syllable[i].SetLfetch(r3, m->hint); 
            }
            break;
          case SYNTH_IPF_LOAD:
            {
                const UINT32 r1 = thread->get_next_load_dest_reg(); // dest
                UINT32 r3=1; //source reg
                if (params->getSynth_Pointer_Chasing(thread->get_uid()))
                {
                    // use last dest reg as the source reg
                    r3 = thread->get_last_dest_reg(); 
                    thread->set_last_dest_reg(r1); // pointer chasing
                }


                /* FIXME: add this to the atomic memops */
                SMSG("Adding register dest " << r1 << " to memop");
                m->set_reg_dest(r1);

                SMSG("load tgt_reg = " << r1 << endl);
                if (m->acquire)
                {
                    syllable[i].SetLoadAcquire(sz, r1,r3);
                }
                else
                {
                    syllable[i].SetLoad(sz, r1,r3);
                }
            }
            break;
          case SYNTH_IPF_STORE:
            {
                const UINT32 r1=0;
                const UINT32 r3=1;
                if (m->release)
                {
                    syllable[i].SetStoreRelease(sz,r1,r3);
                }
                else
                {
                    syllable[i].SetStore(sz,r1,r3);
                }
            }
            break;
          case SYNTH_IPF_COMPARE_EXCHANGE:
            {
                const UINT32 r1= thread->get_next_load_dest_reg(); // dest
                const UINT32 r2=0;
                UINT32 r3=1;
                if (params->getSynth_Pointer_Chasing(thread->get_uid()))
                {
                    // use last dest reg as the source reg
                    r3 = thread->get_last_dest_reg();
                    thread->set_last_dest_reg(r1); // pointer chasing
                }
                SMSG("Tgt_reg = " << r1 << endl);
                
                syllable[i].SetCompareExchange(m->release, sz, r1,r2,r3);
            }
            break;
          case SYNTH_IPF_FETCH_AND_ADD:
            {
                const UINT32 r1= thread->get_next_load_dest_reg(); // dest
                UINT32 r3=1;
                if (params->getSynth_Pointer_Chasing(thread->get_uid()))
                {
                    // use last dest reg as the source reg
                    r3 = thread->get_last_dest_reg();
                    thread->set_last_dest_reg(r1); // pointer chasing
                }
                SMSG("Tgt_reg = " << r1 << endl);
                
                const UINT32 immed =  1;

                // recode sz as sz48
                UINT32 sz48 = 0; // init to bad value
                switch(sz)
                {
                  case 2:
                    sz48 = 4;
                    break;
                  case 3:
                    sz48 = 8;
                    break;
                  default:
                    cerr << "Invalid size for fetch-and-add: " << sz << endl;
                    assert(0);
                }

                syllable[i].SetFetchAndAdd(m->release, sz48, r1, r3, immed);
            }
            break;
          case SYNTH_IPF_EXCHANGE:
            {
                const UINT32 r1= thread->get_next_load_dest_reg(); // dest
                const UINT32 r2=0;
                UINT32 r3=1;
                if (params->getSynth_Pointer_Chasing(thread->get_uid()))
                {
                    // use last dest reg as the source reg
                    r3 = thread->get_last_dest_reg();
                    thread->set_last_dest_reg(r1); // pointer chasing
                }
                SMSG("Tgt_reg = " << r1 << endl);
                syllable[i].SetExchange(sz,r1,r2,r3);
            }
            break;
          default:
            assert(0);
            break;
        }
    }


    bundle.SetTemplate( tmplt );
    bundle.SetSlots(syllable);

    thread->set_bundle(bundle);
}

SYNTH_INST
make_indirect_branch(SYNTH_THREAD thread,
                     UINT64      pc,
                     UINT64      target)
{
    

    SMSG("Emitting an indirect branch at "
         << hex  << pc << " to " << target << dec);

    IPF_RAW_BUNDLE bundle;
    bool conditional = false;
    set_branch_indirect(bundle, /* output */
                        conditional);


    // We are adding to the front of the replay list.  Older things are at
    // the top of the list, syllable 2 first.  Then syllable 1, then 0.

    // or if we want to be cheesy, we can just give an unconditional branch
    // in syllable 0.

    // syllable 0
    const UINT32 br_syllable_idx = 0;
    SYNTH_INST minst = new SYNTH_INST_CLASS(pc,
                                            br_syllable_idx,
                                            SYNTH_IPF_BRANCH);
    minst->SetBundle(bundle);
    minst->SetBranchInfo(true, target);
    return minst;

}
////////////////////////////////////////////////////////////////////////////



static bool
long_relative_offset_required(INT64 immed)
{
    INT64 sh_immed = immed>>4;

    //FIXME!!!! This is bogus
    if (immed > 0 && sh_immed > 0x1FFFFF)
    {
        return true;
    }
    else if (immed < 0 && (-sh_immed) > 0x1FFFFF)
    {
        return true;
    }
    return false;
}

void
set_branch_indirect( IPF_RAW_BUNDLE& bundle, /* output */
                     bool            conditional )
{
    IPF_RAW_SYLLABLE syllable[SYLLABLES_PER_BUNDLE];
    const UINT32 pred_reg_zero = 0;
    const UINT32 pred_reg      = 1;
    const UINT32 br_reg        = 1;

    if (conditional)
    {
        syllable[0].SetIndirectBranch( pred_reg, br_reg ); // conditional
    }
    else
    {
        syllable[0].SetIndirectBranch( pred_reg_zero, br_reg ); // unconditional
    }
    syllable[1].SetNopBunit();
    syllable[2].SetNopBunit();
    
    bundle.SetTemplate(IPF_TEMPLATE_BBB_STOP);
    bundle.SetSlots(syllable);
}

bool //* returns true iff generated a long branch */
set_branch( IPF_RAW_BUNDLE& bundle, /* output */
            INT64           immed,
            bool            conditional )
{
    bool long_rel = long_relative_offset_required(immed);
    INT64 sh_immed = immed >> 4;

    //long_rel = false;
    if (long_rel)
    {
#if 0
        SMSG("Emitting a long branch with unshifted offset "
             << hex << immed << dec);

        FIXME(); // long branches are not supported by asim yet

        IPF_RAW_SYLLABLE syllable[SYLLABLES_PER_BUNDLE];
        syllable[0].SetNopMunit();
        syllable[1].SetLongBranchImmed( sh_immed );
        syllable[2].SetLongBranch( sh_immed, (conditional?1:0) ); 
        
        bundle.SetTemplate(IPF_TEMPLATE_MLX_STOP);
#endif

        SMSG("Emitting an indirect branch because we require a long_rel");
        set_branch_indirect(/* output */ bundle, conditional);
    }
    else
    {
        IPF_RAW_SYLLABLE syllable[SYLLABLES_PER_BUNDLE];
        if (conditional)
        {
            syllable[0].SetCBranch( sh_immed ); // conditional
        }
        else
        {
            syllable[0].SetUBranch( sh_immed ); // unconditional
        }
        syllable[1].SetNopBunit();
        syllable[2].SetNopBunit();
        
        bundle.SetTemplate(IPF_TEMPLATE_BBB_STOP);
        bundle.SetSlots(syllable);
    }
    return long_rel;
}


void
make_relative_branch(SYNTH_THREAD thread,
                     UINT64      pc,
                     UINT64      target)
{
    

    SMSG("Emitting a relative branch at "
         << hex  << pc << " to " << target << dec);

    IPF_RAW_BUNDLE bundle;
    INT64          immed       = (target - pc);
    bool           conditional = false;
    bool           long_rel    = set_branch(bundle, /* output */
                                            immed,
                                            conditional);

    //  need to make two minsts for long branches and push them both on to
    //  the issued queue!!



    // We are adding to the front of the replay list.  Older things are at
    // the top of the list, so we must add the X-type branch, which lives
    // in syllable 2 first. Then we add the L-type immediate that lives in
    // syllable 1 (or a no-op for non long_rel branches). ... 

    // syllable 2
    {
        const UINT32 br_syllable_idx = 2;
        SYNTH_IPF_ENUM type;
        if (long_rel)
        {
            type = SYNTH_IPF_LONG_BRANCH;
        }
        else
        {
            type = SYNTH_IPF_BRANCH;
        }
        SYNTH_INST minst = new SYNTH_INST_CLASS(pc, br_syllable_idx, type);
        minst->SetBundle(bundle);
        minst->SetBranchInfo(true,target);
        XMSG("add rel br syll 2 to replay");
        thread->add_to_replay(minst);
    }

    // syllable 1

    {
        const UINT32 limm_syllable_idx = 1;
        SYNTH_IPF_ENUM type;
        if (long_rel)
        {
            type = SYNTH_IPF_LIMM;
        }
        else
        {
            type = SYNTH_IPF_NOPM;
        }
        SYNTH_INST minst = new SYNTH_INST_CLASS(pc,limm_syllable_idx,type);
        minst->SetBundle(bundle);
        XMSG("add rel br syll 1 to replay");
        thread->add_to_replay(minst);
    }

    // syllable 0

    {
        UINT32 nop_syllable_idx = 0;
        SYNTH_IPF_ENUM type;
        if (long_rel)
        {
            type = SYNTH_IPF_NOPM;
        }
        else
        {
            type = SYNTH_IPF_NOPB;
        }
        SYNTH_INST minst = new SYNTH_INST_CLASS(pc, nop_syllable_idx, type);
        minst->SetBundle(bundle);
        XMSG("add rel br syll 0 to replay");
        thread->add_to_replay(minst);
    }

}







void
HandleBranch(SYNTH_THREAD  thread)
{
    /* FIXME: HandleBranch is not used */

    SMSG("Branch");
    /* br, call, return */
    SYNTH_EVENT event = thread->get_event();

    UINT32 treg = event->get_branch_target_register();
    bool conditional = event->get_is_branch_conditional();
    bool taken = event->get_is_branch_taken();

    UINT64 target = mask_bundle_addr(event->get_branch_target_ip());

    UINT64 pc=thread->get_pc();
    SMSG( ((taken)?"TAKEN":"NOT-TAKEN") << " Branch from "
          << hex << pc << " to " << target << dec);
    INT64 immed = target - pc;
    
    

    if (event->get_branch_is_register_indirect())
    {
    }
    else
    {
    }

    IPF_RAW_BUNDLE bundle;
    set_branch( bundle, /* output */
                immed, conditional );


    thread->set_bundle(bundle);
}



/////////////////////////////////////////////////////////////////////
