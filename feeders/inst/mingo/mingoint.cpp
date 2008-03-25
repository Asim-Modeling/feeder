/*
 * Copyright (C) 2002-2006 Intel Corporation
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
 */
 
/**
 * @file
 * @author Mark Charney
 *
 * @brief Interface between the performance model and the Mingo feeder
 *
 * This file supports the generic FEED_* calls that are required by the
 * controller.
 */

// generic
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <vector>
#include <list>

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"

  /* We require the controller HOWEVER, we cannot
   * state that requirement in the awb file because
   * something else requires the controller and that
   * is a limitation of the ASIM configuration.
   *
   * So, only use this mingo-ctl.awb in models that
   * have controllers required by something else. */
#include "asim/provides/controller.h"

// ASIM local module
#include "asim/provides/instfeeder_interface.h"
// instfeeder_implementation.h is a generated header file
#include "asim/provides/instfeeder_implementation.h" 

#include "asim/provides/ipf_raw_instruction.h"
//#include "asim/provides/current_frame_mask.h"

/////////////////////////////////////////////////////////////////////////

static UINT32 debug_mingoint = 1; // turn on debugging messages
static const UINT64 magic_uid = (~0ULL);

#define MINGO_TRACE (TRACEP(Trace_Sys))

#define TMSG(x) ({ \
   if (debug_mingoint) \
   { \
      TRACE(Trace_Sys, \
           cout <<  "        " \
                << __FILE__ << ":" << __LINE__ << ": " <<  x << endl) \
   }\
})

#define FIXME() ({ \
   cout << "FIXME: " << __FILE__ << ":" << __LINE__ << endl;  \
   assert(0); \
 })

#define MSG(x) ({ \
   cout <<  "        " \
        << __FILE__ << ":" << __LINE__ << ": " <<  x << endl; \
 })


///////////////////////////////////////////////////////////////////////////

/* IPF bundle templates */
typedef enum {
    MINGO_IPF_TEMPLATE_MII     =0x00,
    MINGO_IPF_TEMPLATE_MII_STOP=0x01,
    MINGO_IPF_TEMPLATE_MMI     =0x08,
    MINGO_IPF_TEMPLATE_MMI_STOP=0x09,
    MINGO_IPF_TEMPLATE_BBB     =0x16,
    MINGO_IPF_TEMPLATE_BBB_STOP=0x17,
    MINGO_IPF_TEMPLATE_MLX     =0x04,
    MINGO_IPF_TEMPLATE_MLX_STOP=0x05
} MINGO_IPF_TEMPLATE_ENUM;


typedef enum {
    MINGO_IPF_INVALID,
    MINGO_IPF_NOPM,
    MINGO_IPF_NOPI,
    MINGO_IPF_NOPB,
    MINGO_IPF_XOR,
    MINGO_IPF_LOAD,
    MINGO_IPF_STORE,
    MINGO_IPF_EXCHANGE,
    MINGO_IPF_FENCE,
    MINGO_IPF_BRANCH,
    MINGO_IPF_LIMM,
    MINGO_IPF_LAST
} MINGO_IPF_ENUM;


///////////////////////////////////////////////////////////////////////
const UINT64 BUNDLE_ADDR_MASK = ~(0xFULL);
static const UINT32 BYTES_PER_BUNDLE = 16;

typedef enum {
    do_not_keep = 0,
    keeper = 1
} KEPT_INST_ENUM;

static UINT64
mask_bundle_addr(UINT64 x)
{
    return (x & BUNDLE_ADDR_MASK);
}

class MINGO_INST_CLASS
{
  public:
    MINGO_INST_CLASS() : ea(0), access_size(0), taken(false), target(0) {} // CONS
    
    IPF_RAW_BUNDLE bundle;
    MINGO_IPF_ENUM type;

    UINT64 uid;
    IADDR_CLASS pc;    

    UINT64 ea;
    UINT8 access_size;
    
    bool   taken;
    IADDR_CLASS target;
    
    inline UINT64 GetUid(void) { return uid; }
    inline bool isNOP(void) const { return (type == MINGO_IPF_NOPI ||
                                            type == MINGO_IPF_NOPB || 
                                            type == MINGO_IPF_NOPM ); }
};

typedef MINGO_INST_CLASS *MINGO_INST;

class MINGO_STATE_CLASS
{
  private:
    const UINT64 uid;
    UINT64 preceeding_inst;
    UINT32 syllable; // when syllable==SYLLABLES_PER_BUNDLE we are done
                     //   with this bundle.  
    UINT64 pc;
    IPF_RAW_BUNDLE bundle;
    MINGO_DATA_CLASS event;
    UINT32 wrong_path;

    ASIM_THREAD thread; //used for terminating the thread in the controller.

    bool needJumpToNextPC;

    list<MINGO_INST> issued;
    // for replaying things to ASIM, on mispredicts, etc.
    list<MINGO_INST> replay;

  public:
    MINGO_STATE_CLASS(UINT64 arg_uid) //CONS
        : uid(arg_uid),
          preceeding_inst(0),
          syllable(SYLLABLES_PER_BUNDLE), //start off as done with this "event"
          pc(0),
          wrong_path(0),
          needJumpToNextPC(true)
    {}
    
    void init(void)
    {
        // after event has been set 
        syllable = 0; // now we have some work to do.
        preceeding_inst = event.get_instruction_count();
    }
    
    inline void set_thread(ASIM_THREAD arg_thread) {  thread = arg_thread; }
    inline ASIM_THREAD get_thread(void) {  return thread; }

    inline UINT64 get_uid(void) {  return uid; }
        

    inline void inc_wrong_path(void) { wrong_path++; }
    inline void not_wrong_path(void) { wrong_path=0; }
    inline UINT32 get_wrong_path(void) { return wrong_path; }
    
    inline MINGO_DATA get_event(void) { return &event; } //FIXME: bad style points

    inline void set_pc(IADDR_CLASS arg_pc) {
        pc       = arg_pc.GetBundleAddr();
        syllable = arg_pc.GetSyllableIndex();
    }
    inline void set_pc(UINT64 arg_pc) {
        pc = mask_bundle_addr(arg_pc);
        syllable  = 0;
    }

    inline UINT64 get_pc(void) { return pc; }
    inline void inc_pc(void) { pc += BYTES_PER_BUNDLE; }
    inline IADDR_CLASS get_ipf_pc(void) {
        IADDR_CLASS pc_ia(pc, syllable);
        return pc_ia;
    }
    
    inline IPF_RAW_BUNDLE* get_bundle(void) { return &bundle; }
    inline void set_bundle(IPF_RAW_BUNDLE arg_bundle) {
        bundle=arg_bundle;
        set_syllable();
    }

    inline void set_syllable(UINT32 a=0) { syllable=a; }
    inline UINT32 get_syllable(void) { return syllable; }
    inline void inc_syllable(void) {
        syllable++;
        if (done()) {
            inc_pc();
        }
    }
    inline bool done(void) { return (syllable==SYLLABLES_PER_BUNDLE); }
    inline void make_done(void) { syllable=SYLLABLES_PER_BUNDLE; }

    inline bool done_with_preceeding_inst(void) { return (preceeding_inst == 0); } 
    inline void dec_preceeding_inst(void) { preceeding_inst--; }

    inline bool test_need_jump(void) { return needJumpToNextPC; }
    inline void set_need_jump(void) { needJumpToNextPC = true; }
    inline void clear_need_jump(void) { needJumpToNextPC = false; }
    

    inline void issue(MINGO_INST inst) { issued.push_back(inst); }
    
    // FIXME: slow linear search:
    inline void commit_sloppy(MINGO_INST inst) {
        issued.remove(inst);
    }
    inline void commit_skipping_nops(UINT64 ref_uid) {
        // This is an in-order commit.
        //
        // The ASIM tanglewood model will not commit NOPs so we
        // must remove them manually.

        list< MINGO_INST >::iterator i = issued.begin();
        while( i != issued.end() )
        {
            MINGO_INST minst = *i;
            if (minst->GetUid() == ref_uid)  
            {
                TMSG("Explicitly commiting UID = " << minst->GetUid());
                issued.erase(i++);
                delete minst;
                return;
            }
            else if (minst->isNOP())
            {
                // the tanglewood mode
                TMSG("Implicitly commiting a NOP: UID = " << minst->GetUid());
                issued.erase(i++);
                delete minst;
                continue;
            }
            TMSG("Committing out of order is not supported yet\n");
            assert(0); // committing out of order is not supported yet.
        }
    }

    inline MINGO_INST find_mingo_inst(UINT64 ref_uid)
    {
        //FIXME: slow!! Use a secondary hash table
        list< MINGO_INST >::iterator i = issued.begin();
        for ( ; i != issued.end(); i++)
        {
            if ((*i)->GetUid() == ref_uid)  
            {
                return *i;
            }
        }
        return 0;
    }

    inline void kill_and_replay_this_inst(UINT64 ref_uid) 
    {
        TMSG("Trying to killing and replay UID = " <<ref_uid);
        if (!issued.empty())
        {
            MINGO_INST p = issued.back();
            if (p->GetUid() == ref_uid)
            {
                TMSG("     Killing and replaying UID = " << p->GetUid());
                replay.push_front(p);
                issued.pop_back();
                return;
            }
            else
            {
                TMSG("Tried to kill the non-youngest thing.");
                TMSG("That's okay on wrong-path instructions or db-full exception");
                TMSG("     Issued list:");
                list<MINGO_INST>::iterator it = issued.begin();
                while( it != issued.end())
                {
                    MINGO_INST p = *it;
                    TMSG("     UID = " << p->GetUid());
                    if (p->GetUid() == ref_uid)
                    {
                        TMSG("Oops. Found it in the list. FIXME!\n");
                        issued.erase(it++);
                        replay.push_front(p);
                        return;
                    }
                    it++;
                }
            }
        }
        else
        {
            TMSG("The issued queue was empty on a kill-me.");
            // that's okay for wrong-path stuff.
        }
    }
    inline void copy_to_replay_queue(UINT64 ref_uid)
    {
        TMSG("Copying issued insts younger than " << ref_uid
             << " to replay queue.");
        // copy everything younger than inst to the replay queue.
        if (!issued.empty())
        {
            MINGO_INST p = issued.back();
            while(p->GetUid() > ref_uid)
            {
                // note: we are copying instructins in the
                // reverse order!!
                TMSG("     Copying UID = " << p->GetUid());
                replay.push_front(p); // make p the oldest thing on replay list
                issued.pop_back(); // remove youngest thing (p) on issued list.
                p = issued.back();
            }
        }
        else
        {
            TMSG("The issued queue was empty on a kill.");
        }
        
    }

    // "older" insts are at the front of the list
    // "younger" insts are at the back of the list
    // use add_to_replay() to add a sequence of syllables, in order 0, 1 2.
    inline void add_to_replay(MINGO_INST m) { replay.push_back(m); }

    inline bool no_replay_available(void) { return replay.empty(); }
    inline bool replay_available(void) { return !replay.empty(); }
    inline MINGO_INST get_replay(void) { return replay.front(); }
    inline void pop_replay(void) { replay.pop_front(); }

  protected:
  private:
};

typedef MINGO_STATE_CLASS *MINGO_STATE;
///////////////////////////////////////////////////////////////////////


static const UINT32 initial_max_threads = 100;

static MINGO_FEEDER mingo_client;

const UINT32 WRONG_PATH_LIMIT = 700;

//The event we are working on for this thread.
static vector<MINGO_STATE> current_state(initial_max_threads);
///////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

class FEEDER_THREAD_MGMT_CLASS
{
    // FEEDER_THREAD_MGMT_CLASS is a class to track the threads and tell ASIM
    // about their existance.

  public:

    FEEDER_THREAD_MGMT_CLASS(UINT32 nthreads=16) 
        : threads(nthreads),
          max_threads(nthreads) // start with nthreads -- this can grow
    {}
    
    void
    feeder_check_for_threads()
    {
        // grow threads array if necessary
        UINT32 new_thread_id;
        
        do 
        {
            new_thread_id = mingo_client->CheckForNewSoftwareThread();
            if (new_thread_id) 
            {
                TMSG("New mingo thread " <<  new_thread_id);
                if (new_thread_id >= max_threads)
                {
                    max_threads = (new_thread_id+1) * 2 ;
                    current_state.resize(max_threads,0);
                }

                current_state[new_thread_id] =
                    new MINGO_STATE_CLASS(new_thread_id);

                threads.push_back(new_thread_id);
                UINT32 pc = 0; //We just assume 0 and put a branch to the
                               //  correct addres later.
                UINT32 uid = new_thread_id;
                UINT32 pid = new_thread_id;

                ASIM_THREAD thread = new ASIM_THREAD_CLASS(uid, pid, pc);

                current_state[new_thread_id]->set_thread(thread);

                // tell the simulator about this thread
                // NB: 16 Oct 2002: Eric is changing this interface.
                CMD_ThreadBegin(thread);
            }
            
        } while(new_thread_id);
        
    }
    
    void end_thread(UINT64 streamId)
    {
        assert(streamId < max_threads);
        ASIM_THREAD thread = current_state[streamId]->get_thread();
        assert(thread);
        CMD_ThreadEnd(thread);
    }
    
  private:
    vector<UINT32> threads;
    UINT32 max_threads;

};


static FEEDER_THREAD_MGMT_CLASS thread_mgmt(16);

///////////////////////////////////////////////////////////////////////////
static MINGO_IPF_ENUM
    syllable_sequence[MINGO_MSG_LAST][SYLLABLES_PER_BUNDLE];

static MINGO_IPF_ENUM
    long_branch_syllable_sequence[SYLLABLES_PER_BUNDLE];

#define MI(x,y,z,w) \
  ({    syllable_sequence[ MINGO_MSG_ ## x ][0]  =  MINGO_IPF_ ## y ; \
        syllable_sequence[ MINGO_MSG_ ## x ][1]  =  MINGO_IPF_ ## z ; \
        syllable_sequence[ MINGO_MSG_ ## x ][2]  =  MINGO_IPF_ ## w ; })

static void
init_syllable_sequence(void)
{
    for(UINT32 i=0;i<MINGO_MSG_LAST;i++)
    {
        for(UINT32 j=0;j<SYLLABLES_PER_BUNDLE;j++)
        {
            syllable_sequence[i][j] = MINGO_IPF_INVALID;
        }
    }

    MI(MEMORY_READ,    LOAD,     NOPM,   NOPI);
    MI(MEMORY_WRITE,   STORE,    NOPM,   NOPI);
    MI(EXCHANGE,       EXCHANGE, NOPM,   NOPI);
    MI(MEMORY_FENCE,   FENCE,    NOPM,   NOPI);
    MI(DEPENDENT_READ, NOPM,     XOR,    NOPI);
    MI(BRANCH,         BRANCH,   NOPB,   NOPB);

    long_branch_syllable_sequence[0] = MINGO_IPF_NOPM;
    long_branch_syllable_sequence[1] = MINGO_IPF_LIMM;
    long_branch_syllable_sequence[2] = MINGO_IPF_BRANCH;
}

#undef MI

class MINGO_IPF_INST
{
  public:
    MINGO_IPF_INST(); //CONS

    ~MINGO_IPF_INST();

    UINT32 get_read_index(void); // get the internal pointer
    bool done(void); // are we done consuming the content of this node?

    void set_bundle(IPF_RAW_BUNDLE& arg_bundle, UINT64 pc );
    IPF_RAW_BUNDLE& get_bundle(void);
    UINT64 get_pc(void);
    IADDR_CLASS get_ipf_pc(void);
    
    MINGO_IPF_ENUM get_type(void);
    MINGO_DATA get_event(void);
    void set(MINGO_DATA event, MINGO_IPF_ENUM type);
    

  private:

    UINT32 write_indx; // which syllable is next write when we're building
                       // this node

    UINT32 read_indx; // which syllable is next to send to ASIM
    // when indx == 3 then we are done with this node.

    IPF_RAW_BUNDLE bundle;
    UINT64 pc; // req'd because things without events need to know where
               // they are.
        
    MINGO_IPF_ENUM e[SYLLABLES_PER_BUNDLE]; 
    MINGO_DATA event[SYLLABLES_PER_BUNDLE]; // null for NOPs and other
                                            // simple ops.
};


MINGO_IPF_INST::MINGO_IPF_INST() //CONS
{
    write_indx = 0;
    read_indx = 0;
    for(int i=0;i<SYLLABLES_PER_BUNDLE;i++)
    {
        event[i]=0;
    }
}

void
MINGO_IPF_INST::set(MINGO_DATA arg_event, MINGO_IPF_ENUM type)
{
    ASSERTX(write_indx < SYLLABLES_PER_BUNDLE);
    event[write_indx] = arg_event;
    e[write_indx] = type;
    write_indx++;
}

MINGO_IPF_ENUM
MINGO_IPF_INST::get_type(void)
{
    return e[read_indx];
}

MINGO_DATA 
MINGO_IPF_INST::get_event(void)
{
    return event[read_indx];
}

void
MINGO_IPF_INST::set_bundle(IPF_RAW_BUNDLE& arg_bundle, UINT64 arg_pc)
{
    pc = arg_pc;
    bundle = arg_bundle;
}

IPF_RAW_BUNDLE&
MINGO_IPF_INST::get_bundle(void)
{
    return bundle;
}

UINT64
MINGO_IPF_INST::get_pc(void)
{
    return pc;
}

IADDR_CLASS
MINGO_IPF_INST::get_ipf_pc(void)
{
    IADDR_CLASS ic(get_pc(), read_indx);
    return ic;
}

MINGO_IPF_INST::~MINGO_IPF_INST()
{
    for(int i=0;i<SYLLABLES_PER_BUNDLE;i++)
    {
        if (event[i])
        {
            delete event[i];
        }
    }
}

UINT32
MINGO_IPF_INST::get_read_index(void)
{
    return read_indx;
}

bool
MINGO_IPF_INST::done(void)
{
    return read_indx == SYLLABLES_PER_BUNDLE;
}


///////////////////////////////////////////////////////////////////////

static void
print_ip(UINT64 ip)
{
    TMSG("Emitting at PC = " << hex << ip << dec);
}

static void
disassemble(ASIM_INST inst) {
    cout << inst->GetRawInst()->DisasmString() << endl; 
}

void
set_bundle_bits(IPF_RAW_BUNDLE* bundle, IADDR_CLASS pc, ASIM_INST inst)
{
    UINT64 bundle64[2];
    bundle64[0] = bundle->Get64(0);
    bundle64[1] = bundle->Get64(1);
    //printf("%016llx : %016llx does this match?\n", bundle64[1], bundle64[0]);
    
    /* in asim, an ASIM_INST is a syllable, not a bundle,
       so we must point to the proper syllable when we
       do the Init() call. */
    inst->Init(bundle64, pc);
    if (MINGO_TRACE)
    {
        cout << "PC = " << pc << endl;
        disassemble(inst);
    }
    
}

static void
misc_inst_setup(UINT64 streamId,
                ASIM_INST inst)
{

    inst->SetTraceID(0); // FIXME: what does this do? (Copied from gambit)

    
    const UINT64 default_cfm = 0x7f; // See Eric. No rotating regs.
    
    UINT64 old_cfm = default_cfm;
    UINT64 new_cfm = default_cfm;
    UINT64 old_prf = default_cfm;
    UINT64 new_prf = default_cfm;
    
    
    inst->SetCFM(old_cfm, new_cfm);
    inst->SetPRF(old_prf, new_prf);
    
}

static MINGO_INST
create_junk_instr(IADDR_CLASS predicted_pc,
                  ASIM_INST   inst,
                  KEPT_INST_ENUM keep)
{
    UINT64 pc           = predicted_pc.GetBundleAddr();
    UINT32 syllable_idx = predicted_pc.GetSyllableIndex();

    const UINT32 r1=0;
    const UINT32 r2=0;
    const UINT32 r3=1;
    const UINT32 r4=2;
    const UINT32 r5=2;
    const UINT32 r6=3;

    IPF_RAW_SYLLABLE syllable[3];
    IPF_RAW_BUNDLE bundle;
    syllable[0].SetXOR(r1,r2,r3);
    syllable[1].SetNopIunit();
    syllable[2].SetXOR(r4,r5,r6);
    bundle.SetTemplate(MINGO_IPF_TEMPLATE_MII_STOP);
    bundle.SetSlots(syllable);

    TMSG(" create_junk_instr");
    set_bundle_bits(&bundle, predicted_pc, inst); // calls inst->Init()

    if (keep == keeper)
    {
        MINGO_INST minst = new MINGO_INST_CLASS;
        minst->pc = predicted_pc;
        minst->uid = inst->GetUid();
        minst->bundle = bundle;
        if (syllable_idx == 1)
        {
            minst->type = MINGO_IPF_NOPI;
        }
        else
        {
            minst->type = MINGO_IPF_XOR;
        }
        
        return minst;
    }
    return 0;
}

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

static void
set_branch_indirect( IPF_RAW_BUNDLE& bundle, /* output */
                     bool conditional )
{
    IPF_RAW_SYLLABLE syllable[SYLLABLES_PER_BUNDLE];
    const UINT32 pred_reg_zero = 0;
    const UINT32 pred_reg = 1;
    const UINT32 br_reg = 1;

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
    
    bundle.SetTemplate(MINGO_IPF_TEMPLATE_BBB_STOP);
    bundle.SetSlots(syllable);
}

static bool //* returns true iff generated a long branch */
set_branch( IPF_RAW_BUNDLE& bundle, /* output */
            INT64 immed,
            bool conditional )
{
    bool long_rel = long_relative_offset_required(immed);
    INT64 sh_immed = immed >> 4;

    //long_rel = false;
    if (long_rel)
    {
#if 0
        TMSG("Emitting a long branch with unshifted offset "
             << hex << immed << dec);

        FIXME(); // long branches are not supported by asim yet

        IPF_RAW_SYLLABLE syllable[SYLLABLES_PER_BUNDLE];
        syllable[0].SetNopMunit();
        syllable[1].SetLongBranchImmed( sh_immed );
        syllable[2].SetLongBranch( sh_immed, (conditional?1:0) ); 
        
        bundle.SetTemplate(MINGO_IPF_TEMPLATE_MLX_STOP);
#endif

        TMSG("Emitting an indirect branch because we require a long_rel");
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
        
        bundle.SetTemplate(MINGO_IPF_TEMPLATE_BBB_STOP);
        bundle.SetSlots(syllable);
    }
    return long_rel;
}


static void
make_relative_branch(MINGO_STATE state,
                     UINT64 pc,
                     UINT64 target)
{
    

    TMSG("Emitting a relative branch at "
        << hex  << pc << " to " << target << dec);

    INT64 immed = (target - pc);
    IPF_RAW_BUNDLE bundle;
    bool conditional = false;
    bool long_rel = set_branch(bundle, /* output */
                               immed, conditional);

    //  need to make two minsts for long branches and push them both on to
    //  the issued queue!!



    // We are adding to the front of the replay list.  Older things are at
    // the top of the list, so we must add the X-type branch, which lives
    // in syllable 2 first. Then we add the L-type immediate that lives in
    // syllable 1 (or a no-op for non long_rel branches). ... 

    // syllable 2
    {
        UINT32 br_syllable_idx = 2;
        IADDR_CLASS pc_ipf(pc,br_syllable_idx);
        MINGO_INST minst = new MINGO_INST_CLASS;
        minst->pc = pc_ipf;
        minst->uid = magic_uid;
        minst->bundle = bundle;
        minst->type = MINGO_IPF_BRANCH;
        minst->taken = true;
        minst->target.Set(target,0);
        state->add_to_replay(minst);
    }

    // syllable 1

    {
        UINT32 limm_syllable_idx = 1;
        IADDR_CLASS pc_ipf(pc,limm_syllable_idx);
        MINGO_INST minst = new MINGO_INST_CLASS;
        minst->pc = pc_ipf;
        minst->uid = magic_uid;
        minst->bundle = bundle;
        if (long_rel)
        {
            minst->type = MINGO_IPF_LIMM;
        }
        else
        {
            minst->type = MINGO_IPF_NOPM;
        }
        state->add_to_replay(minst);
    }

    // syllable 0

    {
        UINT32 nop_syllable_idx = 0;
        IADDR_CLASS pc_ipf(pc,nop_syllable_idx);
        MINGO_INST minst = new MINGO_INST_CLASS;
        minst->pc = pc_ipf;
        minst->uid = magic_uid;
        minst->bundle = bundle;
        if (long_rel)
        {
            minst->type = MINGO_IPF_NOPM;
        }
        else
        {
            minst->type = MINGO_IPF_NOPB;
        }
            
    }

}

static void
make_indirect_branch(MINGO_STATE state,
                     UINT64 pc,
                     UINT64 target)
{
    

    TMSG("Emitting an indirect branch at "
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
    {
        UINT32 br_syllable_idx = 0;
        IADDR_CLASS pc_ipf(pc,br_syllable_idx);
        MINGO_INST minst = new MINGO_INST_CLASS;
        minst->pc = pc_ipf;
        minst->uid = magic_uid;
        minst->bundle = bundle;
        minst->type = MINGO_IPF_BRANCH;
        minst->taken = true;
        minst->target.Set(target,0);
        state->add_to_replay(minst);
    }

}

static void
HandleEndThread(MINGO_DATA  event)
{
    FIXME();
}

static void
HandleSetPriority(MINGO_DATA  event)
{
    // ignored 
}

static void
HandleSetPreferredCPU(MINGO_DATA  event)
{
    // ignored 
}


static bool
HandleMemoryRead(MINGO_STATE  state)
{
    TMSG("Mem Read");
    UINT32 r1=0;
    UINT32 r3=1;
    IPF_RAW_BUNDLE bundle;
    IPF_RAW_SYLLABLE syllable[SYLLABLES_PER_BUNDLE];

    MINGO_DATA  event = state->get_event();
    MINGO_REG tgt_reg = event->get_target_register();
    UINT32 sz =  event->get_access_size();
    if (sz > 3)
    {
        MSG("Truncating size to 8 bytes\n");
        sz = 8;
    }

    TMSG("Tgt_reg = " << tgt_reg << endl);
    r1 = tgt_reg;  

    syllable[0].SetLoad(sz, r1,r3);
    syllable[1].SetNopMunit();
    syllable[2].SetNopIunit();
    
    bundle.SetTemplate(MINGO_IPF_TEMPLATE_MMI_STOP);
    bundle.SetSlots(syllable);

    state->set_bundle(bundle);

    return true;
}


static bool
HandleMemoryWrite(MINGO_STATE  state)
{
    TMSG("Memory Write");
    UINT32 r1=0;
    UINT32 r3=1;
    IPF_RAW_BUNDLE bundle;
    IPF_RAW_SYLLABLE syllable[3];

    syllable[0].SetStore(r1,r3);
    syllable[1].SetNopMunit();
    syllable[2].SetNopIunit();

    bundle.SetTemplate(MINGO_IPF_TEMPLATE_MMI_STOP);
    bundle.SetSlots(syllable);

    state->set_bundle(bundle);
    return true;
}

static bool
HandleExchange(MINGO_STATE  state)
{
    UINT32 r1=1;
    UINT32 r2=2;
    UINT32 r3=3;
    IPF_RAW_BUNDLE bundle;
    IPF_RAW_SYLLABLE syllable[3];

    MINGO_DATA event = state->get_event();
    MINGO_REG tgt_reg = event->get_target_register();

    TMSG("Tgt_reg = " << tgt_reg << endl);
    r1 = tgt_reg;  

    UINT32 sz =  event->get_access_size();
    if (event->get_is_compare())
    {
        bool release = event->get_is_release();
        syllable[0].SetCompareExchange(sz,release, r1,r2,r3);
    }
    else
    {
        syllable[0].SetExchange(sz,r1,r2,r3);
    }
    syllable[1].SetNopMunit();
    syllable[2].SetNopIunit();

    bundle.SetTemplate(MINGO_IPF_TEMPLATE_MMI_STOP);
    bundle.SetSlots(syllable);

    state->set_bundle(bundle);
    return true;
}


static bool
HandleMemoryFence(MINGO_STATE  state)
{
    IPF_RAW_BUNDLE bundle;
    IPF_RAW_SYLLABLE syllable[3];

    syllable[0].SetFence();
    syllable[1].SetNopMunit();
    syllable[2].SetNopIunit();

    bundle.SetTemplate(MINGO_IPF_TEMPLATE_MMI_STOP);
    bundle.SetSlots(syllable);

    state->set_bundle(bundle);
    return true;
}

static bool
HandleDependentOp(MINGO_STATE  state)
{
    UINT32 r1=0;
    UINT32 r2=0;
    UINT32 r3=1;
    IPF_RAW_BUNDLE bundle;
    IPF_RAW_SYLLABLE syllable[3];

    MINGO_DATA event = state->get_event();
    MINGO_REG src_reg = event->get_source_register();

    TMSG("Src_reg = " << src_reg);
    r2 = src_reg;  
    r3 = src_reg;  

    syllable[0].SetNopMunit();
    syllable[1].SetXOR(r1,r2,r3);
    syllable[2].SetNopIunit();

    bundle.SetTemplate(MINGO_IPF_TEMPLATE_MII_STOP);
    bundle.SetSlots(syllable);

    state->set_bundle(bundle);
    return true;
}


static bool
HandleBranch(MINGO_STATE  state)
{
    TMSG("Branch");
     /* br, call, return */
    MINGO_DATA event = state->get_event();
    MINGO_CONTROL_TRANSFER tranfer_type = event->get_branch_type();

    MINGO_REG treg = event->get_branch_target_register();
    bool conditional = event->get_is_branch_conditional();
    bool taken = event->get_is_branch_taken();

    UINT64 target = mask_bundle_addr(event->get_branch_target_ip());

    UINT64 pc=state->get_pc();
    TMSG( ((taken)?"TAKEN":"NOT-TAKEN") << " Branch from "
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


    state->set_bundle(bundle);
    return true;
}



static bool
HandleUnexpectedEvent(MINGO_STATE state)
{
    TMSG("HandleUnexpectedEvent NDY");
    exit(1);
    return false;
}

static bool
HandleIgnored(MINGO_STATE  state)
{
    TMSG("Ignored mingo_client request (NDY)");
    return false;
}

  

////////////////////////////////////////////////////////////////////////////

static bool (*dispatch_table[MINGO_MSG_LAST])(MINGO_STATE state);

static void
init_dispatch_table(void) 
{
    for(UINT32 i = 0; i < MINGO_MSG_LAST; i++)
    {
        dispatch_table[i]  = HandleIgnored;
    }

    dispatch_table[MINGO_MSG_MEMORY_READ] =  HandleMemoryRead;
    dispatch_table[MINGO_MSG_MEMORY_WRITE] =  HandleMemoryWrite;
    dispatch_table[MINGO_MSG_EXCHANGE]  = HandleExchange;
    dispatch_table[MINGO_MSG_MEMORY_FENCE] =  HandleMemoryFence;
    dispatch_table[MINGO_MSG_DEPENDENT_READ] =  HandleDependentOp;
    dispatch_table[MINGO_MSG_BRANCH]  = HandleBranch;
    dispatch_table[MINGO_MSG_NONE] = HandleUnexpectedEvent;
}


static bool
dispatch_mingo_event(MINGO_STATE state)
{
    // Create the bundle that we'll pass back to ASIM
    // based on the data in the event.

    MINGO_DATA event = state->get_event();
    MINGO_MSG event_type = event->get_msg();

    if (event_type < MINGO_MSG_LAST)
    {
        return (*dispatch_table[event_type])(state);
    }
    else
    {
        ASIMWARNING("dispatch_mingo_events: Received unexpected Mingo event type: "
                    << event_type << endl;);
        HandleUnexpectedEvent(state);
        return false;
    }
} 
    
static bool
consume_mingo_events(MINGO_STATE state, UINT64 streamId)
{
        
    bool got_an_inst;
    do  // loop until we can get something we can send to ASIM.
    {
        thread_mgmt.feeder_check_for_threads(); // could do this less frequently?

        UINT32 mingo_thread = streamId;
        
        const UINT32 MINGO_TIMEOUT_USEC = 2*3000000;
        
        MINGO_DATA event = state->get_event(); // just get the pointer // bad style!

        
        // see if the trace ended
        if (mingo_client->EndOfData())
        {
            MSG("End of data from Mingo");
            thread_mgmt.end_thread(streamId);
            return false;
        }

        // fill in the event pointed to by "event"
        bool got_one =
            mingo_client->GetNextSoftwareThreadEvent(mingo_thread,
                                                     event, 
                                                     MINGO_TIMEOUT_USEC);
        
        if (!got_one)
        {
            MSG("Time-out or end of data from Mingo info source.");
            thread_mgmt.end_thread(streamId);
            return false;
        }
        
        mingo_client->AllowSoftwareThreadProgress(mingo_thread);
        
        got_an_inst = dispatch_mingo_event(state);
        
    } while(!got_an_inst);
    return true;
}

static void
convert_internal_node_to_asim_inst(UINT64 streamId,
                                   MINGO_STATE state,
                                   ASIM_INST inst)
{
    MINGO_DATA       event = state->get_event();
    IPF_RAW_BUNDLE* bundle = state->get_bundle();
    UINT32    syllable_idx = state->get_syllable();
    IADDR_CLASS         pc = state->get_ipf_pc();
    MINGO_MSG        etype = event->get_msg();
    MINGO_IPF_ENUM   type;
    UINT64            vea;
    UINT64           size;

    ASSERTX(etype < MINGO_MSG_LAST);
    ASSERTX(syllable_idx < SYLLABLES_PER_BUNDLE);
    type = syllable_sequence[ etype ][syllable_idx];

    MINGO_INST minst = new MINGO_INST_CLASS;
    minst->pc = pc;
    minst->bundle = *bundle;
    minst->type = type;
    minst->uid = inst->GetUid();

    TMSG("current node PC = " << pc << ", thread " << streamId);
    set_bundle_bits(bundle, pc, inst); // calls inst->Init()

    switch(type)
    {
      case MINGO_IPF_NOPM:
      case MINGO_IPF_NOPI:
      case MINGO_IPF_NOPB:
      case MINGO_IPF_XOR:
      case MINGO_IPF_FENCE:
      case MINGO_IPF_LIMM:
        // nothing to do
        state->inc_syllable();
        break;

      case MINGO_IPF_LOAD:
      case MINGO_IPF_EXCHANGE:
      case MINGO_IPF_STORE:
        TMSG("Set EA for Load/Exch/Store");
        ASSERTX(event);
        size = event->get_access_size();
        vea = (event->get_va() & (UINT64) (~(size-1ULL)));//FIXME: masking unaligned stuff!
        inst->SetVirtualEffAddress(vea);
        inst->SetPhysicalEffAddress(vea);
        inst->SetAccessSize( size );

        minst->ea = vea;
        minst->access_size = size;

        state->inc_syllable();
        break;


      case MINGO_IPF_BRANCH:
        {
            TMSG("Telling feeder about a branch(1)");
            ASSERTX(event);
            bool   taken  = event->get_is_branch_taken();
            UINT64 target = event->get_branch_target_ip();
            IADDR_CLASS real_target;

            if (taken)
            {
                real_target.Set(target,0);
            }
            else
            {
                real_target = pc.Next();
            }

            inst->SetActualTarget(real_target);
            minst->target = real_target;
            
            inst->SetActualTaken(taken);
            minst->taken = taken;

            state->inc_syllable();
            if (taken)
            {
                state->set_pc(target); // resets the syllable to 0
                state->make_done();
            }
        }
        break;

      case MINGO_IPF_INVALID:
      case MINGO_IPF_LAST:
      default:
        MSG("ERROR: Unexpected node.");
        exit(1);
    }

    state->issue(minst);
}
    

static bool
on_correct_path(IADDR_CLASS mingo_pc,
                IADDR_CLASS predicted_pc)
{
    return (mingo_pc == predicted_pc);
}

static bool
correct_path(IADDR_CLASS predicted_pc, MINGO_STATE state, ASIM_INST inst,
             UINT64 streamId)
{
    
    TMSG("CORRECT PATH.   PREDICTED PC: " << predicted_pc
         << " streamId = " << state->get_uid()
         << " UID = " << inst->GetUid());
    state->not_wrong_path();
    
    // emit the junk insts
    if (!state->done_with_preceeding_inst())
    {
        TMSG("Junk inst");
        MINGO_INST minst = create_junk_instr(predicted_pc, inst, keeper);  
        state->issue(minst);
        state->dec_preceeding_inst();
        state->set_pc(predicted_pc);
        return true;
    }
    
    // fill the inst in and adjust the state
    convert_internal_node_to_asim_inst(streamId, state, inst);
    return true;
}

static void
wrong_path(IADDR_CLASS predicted_pc, MINGO_STATE state, ASIM_INST inst)
{
    TMSG("WRONG PATH.   PREDICTED PC: " << predicted_pc
         << " vs MINGO PC: " << hex << state->get_ipf_pc() << dec
         << " streamId = " << state->get_uid()
         << " UID = " << inst->GetUid());
    (void) create_junk_instr(predicted_pc, inst, do_not_keep);
    state->inc_wrong_path();
    
    if ( state->get_wrong_path()  >= WRONG_PATH_LIMIT )
    {
        MSG("ERROR: We were on the wrong path for " << WRONG_PATH_LIMIT
            << " calls to FEED_Fetch().");
        exit(1);
    }
}
static void
convert_mingo_inst_to_asim_inst(MINGO_INST minst,
                                ASIM_INST inst)
{
    TMSG("convert_mingo_inst_to_asim_inst");
    set_bundle_bits(&(minst->bundle), minst->pc, inst); // calls inst->Init()
    switch(minst->type)
    {
      case MINGO_IPF_LOAD:
      case MINGO_IPF_EXCHANGE:
      case MINGO_IPF_STORE:
        TMSG("Set EA for Load/Exch/Store");
        inst->SetVirtualEffAddress(minst->ea);
        inst->SetPhysicalEffAddress(minst->ea);
        inst->SetAccessSize( minst->access_size);
        break;

      case MINGO_IPF_BRANCH:
        TMSG("Telling feeder about a branch(2)");
        
        inst->SetActualTarget(minst->target);
        inst->SetActualTaken(minst->taken);
        break;

      default:
        break;
    }
}

static void
issue_mingo_inst_from_replay(MINGO_STATE state, ASIM_INST inst)
{
    TMSG("issue_mingo_inst_from_replay");
    MINGO_INST minst = state->get_replay();
    convert_mingo_inst_to_asim_inst(minst, inst);
    state->pop_replay();
    if (minst->uid == magic_uid)
    {
        TMSG("     Converting old UID = magic "
             << " to new UID = " << inst->GetUid());
    }  
    else
    {
        TMSG("     Converting old UID = " << minst->uid
             << " to new UID = " << inst->GetUid());
    }
    minst->uid = inst->GetUid(); // get a new UID
    state->issue(minst);
}

static bool
generate_asim_inst(UINT64 streamId,
                   IADDR_CLASS predicted_pc,
                   ASIM_INST inst)
{
    UINT32 uid = streamId;
    misc_inst_setup(streamId, inst);

    MINGO_STATE state = current_state[uid];
    ASSERTX(state);

    TMSG("Trying to fetch for stream " << streamId
         << "   UID = " << inst->GetUid()
         << "   at PC = " << hex << predicted_pc << dec);


    if (state->replay_available())
    {
        MINGO_INST p = state->get_replay();
        
        if (on_correct_path(p->pc, predicted_pc))
        {
            // Convert mingo inst to asim inst
            TMSG("CORRECT PATH.   PREDICTED PC: " << predicted_pc
                 << " streamId = " << state->get_uid()
                 << " UID = " << inst->GetUid());
            issue_mingo_inst_from_replay(state, inst);
            state->not_wrong_path();
        }
        else
        {
            // Pass back up junk in the asim inst until
            //   the path gets corrected.
            wrong_path(predicted_pc, state, inst);
        }
        return true;
    }
 
    if (state->done()) // are we done with the current bundle?
    {
        // we need to get a new event from the mingo client
        // ...or die trying.
        // fills in "state" with the appropriate info.
        bool alive = consume_mingo_events(state, streamId);
        
        if (!alive)
        {
            // the thread died... just pass back a nop.
            (void)create_junk_instr(predicted_pc, inst, do_not_keep);
            return false;
        }
    }

    // some stuff for the first time. branch to the first inst.
    // The thread thinks we started at 0.
    if (state->test_need_jump())
    {
        MINGO_DATA event = state->get_event();
        UINT64 target = event->get_ip();
        state->set_pc(target); // set the initial PC
        TMSG("First branch (stream " << streamId << ")");
        make_indirect_branch(state,
                             predicted_pc.GetBundleAddr(),
                             target);
        issue_mingo_inst_from_replay(state, inst);
        state->clear_need_jump();
        state->not_wrong_path();
        return true;
    }



    if (on_correct_path(state->get_ipf_pc(), predicted_pc))
    {
        return correct_path(predicted_pc, state, inst, streamId);

    }
    else        // wrong path
    {
        // ASIM has gone down the WRONG PATH. We must feed it XORs until it
        // retires the mispredicted path and syncs up with us.
        wrong_path(predicted_pc, state, inst);
        return true;
    }        
}




////////////////////////////////////////////////////////////////////////

// Required entry points for the controller to call the feeder

/**
 * @brief Initialize Mingo feeder.
 * @return false if we have an error during initialization.
 */

// NOTE: The arguments passed via FEED_Init do not match what
// Mingo is expecting. Mingo wants:
//   path/name of program to run
//   argv
//   number of hardware contexts
//   stdin (defaults to NULL)
//   stdout (defaults to NULL)
//   stderr (defaults to NULL)

// FEED_Init supplies
//    argc
//    argv
//    envp

// Michael can decide what to do with envp. 
// Need to find a way to pass the number of CPUs.

    
// Mingo expects to be told the number of CPUs. This also
// controls the number that we create and loop through.


bool
FEED_Init (
    UINT32 argc,  ///< argument count of feeder specific arguments
    char **argv,  ///< feeder specific arguments
    char **envp)  ///< global program environment
{
    if ((argc == 0) || (argv[0] == NULL))
    {
        MSG("FEED_Init:  Must specify client program with --feeder <prog>");
        exit(1);
    }
    init_syllable_sequence();
    init_dispatch_table(); 

    // FIXME: Instead of passing MAX_TOTAL_NUM_HWCS to mingo, this should call
    // the static function of HWC class to query how many hwc there really are
    // and pass that number.
    mingo_client = new MINGO_FEEDER_CLASS(argv[0], argv, envp,
                                          MAX_TOTAL_NUM_HWCS,
                                          "/dev/null", "/dev/tty");

    ASSERTX (mingo_client);

    thread_mgmt.feeder_check_for_threads();

    return true;
}


/**
 * @brief Simulation has finished. Clean up Mingo feeder.
 */

void
FEED_Done (void)
{
    MSG("FEED_Done() called.");
    mingo_client->Done();
    delete mingo_client;
}

/**
 * @brief Tell user how to invoke the feeder
 */

void
FEED_Usage (FILE *file)
{
    ostringstream os;

    os << "\nFeeder usage: ... <path/program name> [<program args> ...] "
       <<"[stdin] [stdout] [stderr] \n";
    fputs(os.str().c_str(), file);
}

/**
 * @brief Marker manipulation 
 * Not implemented for Mingo feeder
 */
void
FEED_Marker (
    ASIM_MARKER_CMD cmd,  ///< specific marker command to perform
    ASIM_THREAD thread,   ///< thread to perform operation in
    UINT32 markerID,      ///< marker unique ID of this operation
    IADDR_CLASS markerPC, ///< marker PC of this operation
    UINT32 instBits,      ///< marker instruction bits for operation
    UINT32 instMask)      ///< marker instruction bit mask for operation
{
    MSG("FEED_Marker is not supported for Mingo feeder.");
    exit(1);
}

/**
 * @brief Symbol table query using thread ID 
 * Mot implemented for Mingo feeder
 */

UINT64
FEED_Symbol (
    UINT32 tid,  ///< thread ID to lookup symbol in
    char* name)  ///< symbol name to look up
{
    MSG("FEED_Symbol is not supported on Mingo Feeder");
    exit(1);
    return 0;
}


/**
 * @brief Symbol table query using ASIM_THREAD 
 * Not implemented for Mingo feeder
 */

UINT64
FEED_Symbol (
    ASIM_THREAD thread,  ///< thread to lookup symbol in
    char* name)          ///< symbol name to look up
{
    MSG("FEED_Symbol is not supported on Mingo Feeder");
    exit(1);
    return 0;
}


/**
 * @brief Skip numInst instructions or up to markerID, whatever occurs first.
 * Not implemented for Mingo feeder.
 */

UINT64
FEED_Skip (
    ASIM_THREAD thread,  ///< thread to skip instructions for
    UINT64 numInst,      ///< number of instructions to skip
    INT32 markerID)      ///< marker ID to skip to
{
    MSG("FEED_Skip is not supported on Mingo Feeder");
    exit(1);
    return 0;
}

/**
 * @brief Translates virtual PC to physical PC
 * Not implemented for Mingo feeder
 */

bool
FEED_ITranslate(UINT32 hwcNum, UINT64 vpc, UINT64& pa) /* mjc */
{
    pa = vpc; // virtual equals real for now 
    return 1;  // page mapping exists 
}

/**
 * @brief Translates virtual PC to physical PC
 * Not implemented for Mingo feeder
 */

bool
FEED_DTranslate(ASIM_INST inst, UINT64 vpc, UINT64& pa) /* mjc */
{
    pa = vpc; // virtual equals real for now 
    return 1;  // page mapping exists 
}


/**
 * @brief Initialize "inst" with instruction information for
 * instruction from "thread" at "pc".
 * Not implemented for Mingo feeder.
 * Equivalent function is in HW_CONTEXT_CLASS::FetchOperation.
 *
 */
void
FEED_Fetch (ASIM_THREAD thread,
            IADDR_CLASS predicted_pc,
            ASIM_INST inst) 
{
    MSG("FEED_Fetch should not be called. Use FEED_Fetch2().");
    assert(0); // cause a core dump
}

bool
FEED_Fetch2(UINT64 streamId,
            IADDR_CLASS predicted_pc,
            ASIM_INST inst) 
{
    // compute the bundle bits by talking to Mingo
    return generate_asim_inst(streamId, predicted_pc, inst);
}

/**
 * @brief Return the contents of one of inst's register operands.
 * Not implemented for Mingo feeder
 */

UINT64
FEED_GetRegValue (ASIM_INST inst, RegOps regOp)
{
    MSG("FEED_GetRegValue is not supported for Mingo Feeder");
    exit(1);
    return 0;
}

/**
 * @brief Force feeder to use address supplied by performance model.
 * Not implemented for Mingo feeder.
 */

void
FEED_SetEA(ASIM_INST inst, char vf, UINT64 vea)
{
    MSG("FEED_SetEA is not supported for Mingo Feeder");
    exit(1);
}

/**
 * Inform feeder that instruction is being issued.
 * Not implemented for Mingo feeder.
 */

void
FEED_Issue (ASIM_INST inst)
/*
 * Issue 'inst'...
 */
{
    // ignore these
}

/**
 * Do cache access for load
 * Not implemented for Mingo feeder.
 */

void
FEED_DoRead (ASIM_INST inst)
{
    TMSG("FEED_DoRead is not supported for Mingo Feeder.");
    //exit(1);
}

/**
 * Do speculative write for store.
 * Not implemented for Mingo feeder. 
 */

void
FEED_DoSpecWrite (ASIM_INST inst)
{
    TMSG("FEED_DoSpecWrite is not supported for Mingo Feeder");
    //exit(1);
}

/**
 * Commit speculative write data.
 * Not implemented for Mingo feeder.
 */

void
FEED_DoWrite (ASIM_INST inst)
{    
    TMSG("FEED_DoWrite is not supported for Mingo Feeder");
    //exit(1);
}

/**
 * brief Store conditional failed
 * Not implemented for Mingo feeder
 */

void
FEED_StcFailed (ASIM_INST inst)
{
    MSG("FEED_StcFailed is not supported for Mingo Feeder");
    exit(1);
}

/**
 * Commit the instruction.
 */

void
FEED_Commit (ASIM_INST inst, UINT64 streamId)
{
    TMSG("FEED_Commit UID = " << inst->GetUid() << "  stream = " << streamId);
    MINGO_STATE state = current_state[streamId];
    assert(state);
    // find the mingo inst that matches this asim inst
    state->commit_skipping_nops(inst->GetUid());
}

/**
 * Kill instruction and all younger instructions
 */

void
FEED_Kill (ASIM_INST inst, bool fetchNext, bool killMe, UINT64 streamId)
{
    TMSG("FEED_Kill UID = " << inst->GetUid() << "  stream = " << streamId
         << (fetchNext?" FETCH-NEXT ":" REFETCH-THIS ") 
         << (killMe?" KILLME ":" ")          );
    // fetchNext is true for mispredicts, for example.

    MINGO_STATE state = current_state[streamId];
    ASSERTX(state);
    state->copy_to_replay_queue(inst->GetUid());
    if (killMe)
    {
        assert(fetchNext==false);
        state->kill_and_replay_this_inst(inst->GetUid());
    }

}
