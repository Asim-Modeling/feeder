/***************************************************************************
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
****************************************************************************/
 
/**
 * @file 
 * @author Ramon Matas
 *
 * @brief A feeder for ASIM that generates memory instructions using traces
 *
 * Simple IPF feeder to set some specific micro-benchmarks.
 * It generates sequences of loads and stores to the addresses specified in 
 * some trace files.
 *
 * Format of the trace files:
 *      <operation_type> <@> <access_size> <byte> <hint> (<thread_id> <inst_id>)
 *
 *      operation_type: r/w/wr/ww/pr/pw (read/write/wait and read/wait and write/warm-up read/warm-up write)
 *                      IMPORTANT: p commands should go before any other operation, otherwise
 *                                 the trace will be considered erroneus
 *      access_size: 1/2/4/8 
 *      hint: 0/1/2/3 (NONE/NT1/NT2/NTA)
 *      thread_id + inst_id: thread and instruction id (line number) to wait
 *                           The instruction won't be fetched until the specified instruction
 *                           has been committed
 *
 */

// generic
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <list>

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/trace.h"
#include "asim/thread.h"
#include "asim/cmd.h"

  /* We require the controller HOWEVER, we cannot
   * state that requirement in the awb file because
   * something else requires the controller and that
   * is a limitation of the ASIM configuration.
   *
   * So, only use this  in models that
   * have controllers required by something else. */
#include "asim/provides/controller.h"

// ASIM public modules
#include "asim/provides/instfeeder_interface.h"
// instfeeder_implementation.h is a generated header file
#include "asim/provides/instfeeder_implementation.h" 

#include "asim/provides/ipf_raw_instruction.h"
//#include "asim/provides/current_frame_mask.h"

#include "asim/provides/memory_value_model.h"

#include "asim/provides/cbox_msg.h"

// ASIM local module
#include "mem-feeder.h"


// Template that will be used: M-unit M-unit I-unit
#define TEMPLATE IPF_TEMPLATE_MMI

#define MEM_DEBUG MEM_PRINT

//////////////////////////////////////////////////////////////////////////////
#define WRITE(x) ({ \
   if (MEM_DEBUG) \
   { \
          x;  \
   }\
})
///////////////////////////////////////////////////////////////////////////////

typedef enum
{
    MEM_ETYPE_INVALID,
    MEM_ETYPE_LOAD,
    MEM_ETYPE_STORE,
    MEM_ETYPE_EXCHANGE,
    MEM_ETYPE_COMPARE_EXCHANGE,
    MEM_ETYPE_MEMORY_FENCE,
    MEM_ETYPE_DEPENDENT_READ,
    MEM_ETYPE_FETCH_AND_ADD,
    MEM_ETYPE_NOPM,
    MEM_ETYPE_NOPI,
    MEM_ETYPE_LAST              // Must be last
}
MEM_ETYPE;


static char* etype_names[] = {
    "INVALID",
    "LOAD",
    "STORE",
    "EXCHANGE",
    "COMPARE_EXCHANGE",
    "MEMORY_FENCE",
    "DEPENDENT_READ",
    "FETCHA_AND_ADD",
    "NOPM",
    "NOPI",
    0
};

///////////////////////////////////////////////////////////////////////
UINT32
MEMORY_FEEDER_CLASS::get_stream_id(ASIM_INST inst)
{
    UINT32 s = STREAM_HANDLE(inst->GetSWC()->GetFeederStreamHandle());
    return s;
}

///////////////////////////////////////////////////////////////////////

class MEM_MEMOP
{
  public:
  
    MEM_MEMOP(UINT64 arg_pc,
              UINT32 arg_syllable = 0) // CONS
        : ea(0),
          sz(0),
          etype(MEM_ETYPE_INVALID),
          pc(arg_pc,arg_syllable)
    {
    }
    
    UINT64      uid; // uid of ASIM_INST
    
    UINT64      ea;
    UINT8       sz; // 0 = 8bits, 1 = 16bits, 2 = 32bits, 3 = 64bits
    UINT8       hint;
    MEM_ETYPE   etype;
    IADDR_CLASS pc;
    
    IPF_RAW_BUNDLE bundle; // bundle where the instruction was first generated
    
    inline void SetUid(UINT64 arg_uid) { uid = arg_uid; }
    inline UINT64 GetUid(void) const { return uid; }
    
    inline void SetBundle(const IPF_RAW_BUNDLE& b){ bundle = b; }
    
    inline bool isNOP(void) const
    {
        return (etype == MEM_ETYPE_NOPI || 
              etype == MEM_ETYPE_NOPM );
    }
    
};

typedef MEM_MEMOP *MEM_INST;

////////////////////////////////////////////////////////////////////////////
class REGISTER_PROVIDER_CLASS
{
    // track registers to hand out next... to avoid register hazards

    UINT32 low_reg;
    UINT32 high_reg;
    UINT32 next_reg;
  public:
    REGISTER_PROVIDER_CLASS(UINT32 arg_low_reg, UINT32 arg_high_reg)
        : low_reg(arg_low_reg),
          high_reg(arg_high_reg),
          next_reg(arg_low_reg)
    { /* nada */ }
    
    inline UINT32 get_next_reg(void)
    {
        UINT32 t = next_reg;
        next_reg++;
        if (next_reg > high_reg)
        {
            next_reg=low_reg;
        }
        return t;
    }
}; /* class REGISTER_PROVIDER_CLASS */

////////////////////////////////////////////////////////////////////////////
class FEEDER_THREAD_MGMT_CLASS;

class MEM_THREAD_CLASS
{

  private:
    const UINT64       uid;

    IFEEDER_THREAD thread;      //used for terminating the thread in the
                                //controller.  
    list<MEM_INST> issued; // issued but not committed correct-path insts
    list<MEM_INST> replay;
    
    IPF_RAW_BUNDLE* current_bundle;
    UINT32 current_inst;
    bool haveBundle;
    
    MEM_INST op[SYLLABLES_PER_BUNDLE];
    
    bool stopped;

    bool makeBundle(IADDR_CLASS predicted_pc);
    
    std::ifstream trace;
    
    REGISTER_PROVIDER_CLASS dests;
    
    FEEDER_THREAD_MGMT_CLASS* threadMgmt;
    
    bool waiting;
    
    // Waiting instruction
    string waitingType;
    UINT32 waitingSz;
    UINT32 waitingHint;
    UINT64 waitingEa;     
    
    UINT64 currentInst;
    
    UINT64 waitingInst;
    UINT64 waitingThread;

  public:
  
    MEM_THREAD_CLASS(UINT64 arg_uid, FEEDER_THREAD_MGMT_CLASS* mgmt) //CONS
        : uid(arg_uid),
          dests(25,127),
          threadMgmt(mgmt)
    {
        stopped=true;
        haveBundle=false;
        waiting=false;
        currentInst = 0;
    }
    
    UINT64 getCurrentInst()
    {
        return currentInst;
    }

    bool InitThread(char* fileName)
    {
        bool ok=true;

        // Open file to read instructions    
        trace.open(fileName,ios::in);

        if(!trace) ok=false;
        
        if(ok)
            cout << "Thread " << uid << " initialized." << endl;
        else
            cout << "Error while initializing thread " << uid << ": File " << fileName <<" not found." << endl;

        stopped=!ok;
        
        // Return false if file can't be opened
        return (ok);
    }

    void StopThread()
    {
        // Close file if necessary
        if(!stopped)
        {
            WRITE(cout << "Stopping thread " << uid << "." << endl);
            trace.close();
            stopped=true;
        }
    }

    inline IPF_RAW_BUNDLE* GetBundle(void)
    {
        return current_bundle;
    }

    MEM_INST GetNextInstr(IADDR_CLASS predicted_pc);
    
    bool getWarmUp(WARMUP_INFO warmup);

    inline void set_thread(IFEEDER_THREAD arg_thread)
    {
        thread = arg_thread;
    }

    inline IFEEDER_THREAD get_thread(void)  const
    {
        return thread;
    }

    inline UINT64 get_uid(void) const
    {
        return uid;
    }
           
    inline void issue(MEM_INST inst)
    {
        issued.push_back(inst);
    }
       
    inline void commit_skipping_nops(UINT64 ref_uid)
    {
        // This is an in-order commit.
        // The ASIM tanglewood model will not commit NOPs so we
        // must remove them manually.

        list< MEM_INST >::iterator i = issued.begin();
        while( i != issued.end() )
        {
            MEM_INST minst = *i;
            if (minst->GetUid() == ref_uid)  
            {
                currentInst++;
                WRITE(cout << "Thread " << uid << ": commit " << currentInst << endl);
                issued.erase(i++);
                delete minst;
                return;
            }
            else if (minst->isNOP())
            {
                issued.erase(i++);
                delete minst;
                continue;
            }
            assert(0); // committing out of order is not supported yet.
        }
    }
    
    inline bool empty_issued(void)
    {
        // Deletes all NOOPs from issued and returns true if the queue is empty
        list< MEM_INST >::iterator i = issued.begin();
        while( i != issued.end() )
        {
            MEM_INST minst = *i;
            if (minst->isNOP())
            {
                issued.erase(i++);
                delete minst;
                continue;
            }
            else i++;
        }
        
        return issued.empty();
    }
    
    inline MEM_INST find_inst(UINT64 ref_uid)
    {
        //FIXME: slow!! Use a secondary hash table
        list< MEM_INST >::iterator i = issued.begin();
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
        if (!issued.empty())
        {
            MEM_INST p = issued.back();
            if (p->GetUid() == ref_uid)
            {
                replay.push_front(p);
                issued.pop_back();
                return;
            }
            else
            {
                list<MEM_INST>::iterator it = issued.begin();
                while( it != issued.end())
                {
                    MEM_INST p = *it;
                    if (p->GetUid() == ref_uid)
                    {
                        issued.erase(it++);
                        replay.push_front(p);
                        return;
                    }
                    it++;
                }
            }
        }
    }
    
    inline void copy_to_replay_queue(UINT64 ref_uid)
    {
        // copy everything younger than inst to the replay queue.
        if (!issued.empty())
        {
            MEM_INST p = issued.back();
            while(p->GetUid() > ref_uid)
            {
                // note: we are copying instructins in the
                // reverse order!!
                replay.push_front(p); // make p the oldest thing on replay list
                issued.pop_back(); // remove youngest thing (p) on issued list.
                p = issued.back();
            }
        }
        
    }

    // "older" insts are at the front of the list
    // "younger" insts are at the back of the list
    // use add_to_replay() to add a sequence of syllables, in order 0, 1 2.
    inline void add_to_replay(MEM_INST m)
    {
        replay.push_back(m);
    }

    inline bool no_replay_available(void) const
    {
        return replay.empty();
    }
    
    inline bool replay_available(void) const
    {
        return !replay.empty();
    }
    
    inline MEM_INST get_replay(void) const
    {
        return replay.front();
    }
    
    inline void pop_replay(void)
    {
        replay.pop_front();
    }        

};

MEM_INST MEM_THREAD_CLASS::GetNextInstr(IADDR_CLASS predicted_pc)
{
    MEM_INST m=0;
    
    
    // First look at the replay queue
    if(replay_available())
    {
        m=get_replay();
        pop_replay();
    }
    else
    {
        WRITE(cout << "Sending an instruction..." << endl);
                
        if(!haveBundle)
        {
            // We have to create more instructions
            if(makeBundle(predicted_pc))
            {
                // EOF reached, send NOOPs until all instructions got commited
                if(empty_issued() && !stopped)
                {
                    // Tell ASIM that the current thread has finished
                    WRITE(cout << "Finishing ASIM thread" << endl);
                
                    thread->ThreadEnd();
                    StopThread();
                }
            }
        }
        
        // We can take the current instruction of the bundle
        m=op[current_inst];
        current_inst++;
        haveBundle=(current_inst<SYLLABLES_PER_BUNDLE);
    }
    
    return m;
}

typedef MEM_THREAD_CLASS *MEM_THREAD;
///////////////////////////////////////////////////////////////////////

class FEEDER_THREAD_MGMT_CLASS
{
    // FEEDER_THREAD_MGMT_CLASS is a class to track the threads and tell ASIM
    // about their existance.

  public:

    FEEDER_THREAD_MGMT_CLASS() //CONS
        : threads(MEM_THREADS),
          max_threads(MEM_THREADS)
    {
        current_state = new MEM_THREAD_CLASS*[MEM_THREADS];

        for(UINT32 i=0;i<MEM_THREADS;i++)
        {
            current_state[i]=0;
        }
    }
    
    // Creates and  initialize all threads (telling ASIM about their existance)
    // Returns true if every thread could be initialized properly
    bool
    make_threads(IFEEDER_BASE feeder, char** argv)
    {
        bool ok=true;
        
        for(UINT32 new_thread_id=0 ;
            (new_thread_id < MEM_THREADS) && ok ;
            new_thread_id++)
        {
            current_state[new_thread_id] = new MEM_THREAD_CLASS(new_thread_id, this);

            ok=ok & current_state[new_thread_id]->InitThread(argv[new_thread_id]);
            
            if(ok)
            {
                threads.push_back(new_thread_id);
            
                UINT32 pc = 0; /* We just assume 0 and make a branch to the
                              correct addres later. */
                IFEEDER_STREAM_HANDLE handle = MEMORY_FEEDER_CLASS::STREAM_HANDLE(new_thread_id);
                IFEEDER_THREAD thread = new IFEEDER_THREAD_CLASS(feeder, handle, pc);
            
                current_state[new_thread_id]->set_thread(thread);
            }
        } 
        
        return ok;
    }
    
    void end_thread(UINT64 streamId)
    {
        assert(streamId < max_threads);
        IFEEDER_THREAD thread = current_state[streamId]->get_thread();
        assert(thread);
        thread->ThreadEnd();

        current_state[streamId]->StopThread();
    }

    void Done()
    {
        for(UINT32 new_thread_id=0 ;
            new_thread_id < MEM_THREADS;
            new_thread_id++)
        {
            current_state[new_thread_id]->StopThread();
        }
    }

    void commit(ASIM_INST inst, UINT32 streamId);
    void kill(ASIM_INST inst, UINT32 streamId, bool fetchNext, bool killMe);
    /* called by Fetch */
    void generate_asim_inst(UINT64      streamId,
                            IADDR_CLASS predicted_pc,
                            ASIM_INST   inst);
                            
    inline MEM_THREAD_CLASS* get_thread(UINT32 i)
    {
        assert(i< max_threads);
        assert(current_state[i]);
        return current_state[i];
    }

    void
    misc_inst_setup(UINT64 streamId,
                    ASIM_INST inst);

    void                    
    convert_mem_inst_to_asim_inst(MEM_INST minst,
                                  ASIM_INST  inst);   
  private:
    vector<UINT32> threads;
    UINT32 max_threads;

    /* "current_state" is the main state variables of the memory
     * feeder */
    MEM_THREAD_CLASS** current_state;
    
};

void
FEEDER_THREAD_MGMT_CLASS::misc_inst_setup(UINT64    streamId,
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
}

bool
MEM_THREAD_CLASS::getWarmUp(WARMUP_INFO warmup)
{
    UINT64 addr;
    UINT32 sz,byte,hint;
    string type;
    
    while(trace.peek() == '\n') trace.get();
    if(trace.peek() != 'p')
    {
        WRITE(cout << "No more warm-up commands for thread " << uid << "." << endl);
        return false;
    }
    
    if(trace >> type >> hex >> addr >> dec >> sz >> byte >> hint)
    {        
        WRITE(cout << "Warm-up command thread " << uid << "." << endl);
        
        if(type == "pr")
            warmup->NoteLoad(addr + 8*byte, addr + 8*byte, sz);
        else if(type == "pw")
            warmup->NoteStore(addr + 8*byte, addr + 8*byte, sz);
        else
            VERIFY(false, "Wrong trace format. Thread " << uid);
       
        return true;
    }        

    return false;
}

bool
MEM_THREAD_CLASS::makeBundle(IADDR_CLASS predicted_pc)
{
    bool eof=false;
    UINT64 addr;
    UINT32 sz,byte,hint;
    UINT64 w_thread, w_inst;
    string type;

    WRITE(cout << "Thread " << uid << ": Creating a new bundle" << endl);

    // Creates a new bundle and initialize it
    current_bundle=new IPF_RAW_BUNDLE();
    current_bundle->SetTemplate(TEMPLATE);
    
    IPF_RAW_SYLLABLE instr[SYLLABLES_PER_BUNDLE];
    
    for(UINT32 i=0;i<SYLLABLES_PER_BUNDLE;i++)
        op[i]=new MEM_MEMOP(predicted_pc.GetBundleAddr(),i);
    
    // 3er instruction is always a NOOP
    instr[2].SetNopIunit();
    op[2]->etype=MEM_ETYPE_NOPI;
        
    // Try reading a memory op from trace   
    for(UINT32 i=0;i<2;)
    {
        if(!eof && !stopped)
        {
          
            if(!waiting)
            {
                if(trace >> type)
                {
                  if(type == "w" || type == "r")
                  {
                      if(trace >> hex >> addr >> dec >> sz >> byte >> hint)
                      {
                          WRITE(cout << "New event readed from file" << endl);
                          // WRITE(cout << "Addr: " << hex << addr << " HAddr: " << CBOX_MSG_CLASS::HashAddress(addr) << " Slice: " << CBOX_MSG_CLASS::GetSliceNum(CBOX_MSG_CLASS::HashAddress(addr)) << " Index: " << CBOX_MSG_CLASS::Index(CBOX_MSG_CLASS::HashAddress(addr)) << endl);
                          
                          if(type=="r")
                          {
                              WRITE(cout << "LOAD generated" << endl);
                              
                              instr[i].SetLoad(sz,dests.get_next_reg(),2,(IPF_HINT_ENUM)hint);
                              op[i]->etype=MEM_ETYPE_LOAD;
                              op[i]->sz=sz;
                              op[i]->hint=hint;
                              op[i]->ea=addr+8*byte;
                          }
                          else if(type=="w")
                          {
                              WRITE(cout << "STORE generated" << endl);
                              
                              instr[i].SetStore(sz,0,1,(IPF_HINT_ENUM)hint);
                              op[i]->etype=MEM_ETYPE_STORE;
                              op[i]->sz=sz;
                              op[i]->hint=hint;
                              op[i]->ea=addr+8*byte;             
                          }
                          else
                          {
                              VERIFY(false, "Wrong trace format. Thread " << uid);
                          }
                          
                          i++;
                      }
                      else
                      {
                          VERIFY(false, "Bad file format: thread " << uid);
                      }
                      
                  }
                  else if(type == "ww" || type == "wr")
                  {
                      if(trace >> hex >> addr >> dec >> sz >> byte >> hint >> w_thread >> w_inst)
                      {
                          
                          waiting = true;
                          
                          waitingType = type;
                          waitingSz = sz;
                          waitingHint = hint;
                          waitingEa = addr+8*byte;                      
                          waitingInst = w_inst;
                          waitingThread = w_thread;                                            
                
                          WRITE(cout << "Thread " << uid << " waiting for thread " << w_thread << " inst " << w_inst << endl);
                      }
                      else
                      {
                          VERIFY(false, "Wrong trace format. Thread " << uid);
                      }
                  }
                  else
                  {
                      VERIFY(false, "Wrong trace format. Thread " << uid << ". Read: " << type);
                  }                 
                }
                else
                {
                  
                  // The trace has finished
                  if(!trace.eof()) cout << "Thread " << uid << ": Bad file format" << endl;
                  else WRITE(cout << "EOF reached at thread" << uid << endl);
                    
                  WRITE(cout << "NOOP Generated" << endl);
                  
                  if(i==0) eof=true;
                  instr[i].SetNopMunit();
                  op[i]->etype=MEM_ETYPE_NOPM;
                  
                  i++;
                }
            }
            else
            {
                // We are waiting for an instruction, check if committed
                if(threadMgmt->get_thread(waitingThread)->getCurrentInst() >= waitingInst)
                {
                    WRITE(cout << "Thread " << uid << " stop waiting..." << endl);
                    
                    waiting = false;
                    if(waitingType=="wr")
                    {
                        WRITE(cout << "LOAD generated" << endl);
                        
                        instr[i].SetLoad(waitingSz,dests.get_next_reg(),2,(IPF_HINT_ENUM)waitingHint);
                        op[i]->etype=MEM_ETYPE_LOAD;
                        op[i]->sz=waitingSz;
                        op[i]->hint=waitingHint;
                        op[i]->ea=waitingEa;
                    }
                    else if(waitingType=="ww")
                    {
                        WRITE(cout << "STORE generated" << endl);
                        
                        instr[i].SetStore(waitingSz,0,1,(IPF_HINT_ENUM)waitingHint);
                        op[i]->etype=MEM_ETYPE_STORE;
                        op[i]->sz=waitingSz;
                        op[i]->hint=waitingHint;
                        op[i]->ea=waitingEa;             
                    }
                    else
                    {
                        VERIFY(false, "Bad file format: thread " << uid);
                    }
                    
                    i++;                    
                }
                else
                {
                    // Still waiting, generate a NOOP
                    instr[i].SetNopMunit();
                    op[i]->etype=MEM_ETYPE_NOPM;
                    
                    WRITE(cout << "Thread " << uid << " still waiting for thread " << waitingThread << " inst " << waitingInst << endl);
                    WRITE(cout << "Generating NOOP" << endl);
                    
                    i++;
                }
            }
          
        }
        else
        {
            // No more instruccions in the trace, generate a NOOP
            WRITE(cout << "NOOP Generated" << endl);
            
            instr[i].SetNopMunit();
            op[i]->etype=MEM_ETYPE_NOPM;
            
            i++;
        }

    }

    //Set instructions in bundle
    current_bundle->SetSlots(instr);

    for(UINT32 i=0;i<SYLLABLES_PER_BUNDLE;i++) op[i]->SetBundle(*current_bundle);
    
    // Inform that a new bundle has been created
    haveBundle=true;
    current_inst=0;
    
    return eof;
}

static FEEDER_THREAD_MGMT_CLASS thread_mgmt;
////////////////////////////////////////////////////////

static bool
init_memory_feeder(IFEEDER_BASE feeder, char** argv)
{ 
    return thread_mgmt.make_threads(feeder, argv);
}


/********************************************************************
 * FEEDer callbacks
 *******************************************************************/

bool
MEMORY_FEEDER_CLASS::DTranslate(ASIM_INST inst, UINT64 vpc, UINT64& pa)
{
    pa = vpc; // virtual equals real for now 
    return true;  // page mapping exists 
}


bool
MEMORY_FEEDER_CLASS::ITranslate(UINT32 hwcNum, UINT64 vpc, UINT64& pa)
{
    pa = vpc; // virtual equals real for now 
    return true;  // page mapping exists 
}

/****************************************************************/
/* Stuff we really use in the feeder interface */
/****************************************************************/
static void
set_bundle_bits(IPF_RAW_BUNDLE* bundle,
                IADDR_CLASS     pc,
                ASIM_INST       inst)
{
    /* in ASIM, an ASIM_INST is a syllable, not a bundle, so we must point
      to the proper syllable when we do the Init() call. */
    inst->Init(bundle, pc);
}

bool
MEMORY_FEEDER_CLASS::Fetch(
    IFEEDER_STREAM_HANDLE stream,
    IADDR_CLASS predicted_pc,
    ASIM_INST inst) 
{
    UINT32 streamId = STREAM_HANDLE(stream);

    // compute the bundle bits by talking to inst-generator
    thread_mgmt.generate_asim_inst(streamId, predicted_pc, inst);

    return true;
}

void
FEEDER_THREAD_MGMT_CLASS::convert_mem_inst_to_asim_inst(MEM_INST minst,
                                           ASIM_INST  inst)
{
    /* fill in the fields in the asim inst */
    set_bundle_bits(&(minst->bundle), minst->pc, inst); // calls inst->Init()
    switch(minst->etype)
    {
      case MEM_ETYPE_LOAD:
      case MEM_ETYPE_STORE:
        inst->SetVirtualEffAddress(minst->ea);
        inst->SetPhysicalEffAddress(minst->ea); // FIXME: V=R
        inst->SetAccessSize(minst->sz);
        break;

      default:
        break;
    }
}

void
FEEDER_THREAD_MGMT_CLASS::generate_asim_inst(UINT64      streamId,
                                IADDR_CLASS predicted_pc,
                                ASIM_INST   inst)
{

    if (inst->GetAsimSchedulerContextSwitch() == true)
    {
        // Context switch requested. Creating a NOP to represent a context switch
        VERIFYX(false);
    }
    else
    {
    
        // There is no need to check if we are in the correct path
        // (there are no branches)
        
        const UINT32 uid = streamId;
        misc_inst_setup(streamId, inst);
        const MEM_THREAD thread = get_thread(uid);

        // Next thread's instruction
        const MEM_INST p = thread->GetNextInstr(predicted_pc);

        // Translate from MEM_INST to ASIM_INST
        convert_mem_inst_to_asim_inst(p,inst);
    
        // MEM_INST's uid is ASIM_INST's
        p->uid=inst->GetUid();
    
        // To issued queue
        thread->issue(p);
    }
    
}


void
MEMORY_FEEDER_CLASS::Commit(ASIM_INST inst)
{
    UINT32 streamId = get_stream_id(inst);

    thread_mgmt.commit(inst, streamId);    
}

void
FEEDER_THREAD_MGMT_CLASS::commit(ASIM_INST inst, UINT32 streamId)
{
    /* * 'inst' is committed... */
    MEM_THREAD thread = get_thread(streamId);

    thread->commit_skipping_nops(inst->GetUid());
}

void
MEMORY_FEEDER_CLASS::Kill(ASIM_INST inst, bool fetchNext, bool killMe)
{
    UINT32 streamId = get_stream_id(inst);

    // fetchNext is true for mispredicts, for example.

    thread_mgmt.kill(inst, streamId, fetchNext, killMe);
}

void
FEEDER_THREAD_MGMT_CLASS::kill(ASIM_INST inst,
                  UINT32 streamId,
                  bool fetchNext,
                  bool killMe)
{
    
    /*  Kill instruction and all younger instructions */
    const UINT64 uid = inst->GetUid();

    // fetchNext is true for mispredicts, for example.
      
    MEM_THREAD thread = get_thread(streamId);
    
    thread->copy_to_replay_queue(uid);

    if (killMe)
    {
        assert(fetchNext==false);
        thread->kill_and_replay_this_inst(uid);
    }
}

bool
MEMORY_FEEDER_CLASS::Init(UINT32 argc, char **argv, char **envp)
{
    /*
     * Initialize Memory instruction feeder. Return false
     * if we have an error during initialization.
     */
    cout << "memory feeder initializing ..." << endl;

    if(argc < MEM_THREADS)
    {
        
        cout << "Error: less input files than MEM_THREADS." << endl;
        return false;
        
    }
    
    return init_memory_feeder(this, argv);
    
}


void
MEMORY_FEEDER_CLASS::Done (void)
{
    // Execution finalized, killing threads
    thread_mgmt.Done();
}


MEMORY_FEEDER_CLASS::MEMORY_FEEDER_CLASS()
    : IFEEDER_BASE_CLASS("Memory Feeder")
{ // Nothing

}

bool
MEMORY_FEEDER_CLASS::WarmUp(
    IFEEDER_STREAM_HANDLE stream,
    WARMUP_INFO warmup)
{

    UINT32 streamId = STREAM_HANDLE(stream);
    
    return thread_mgmt.get_thread(streamId)->getWarmUp(warmup);
    
}

void
IFEEDER_Usage (FILE *file)
{
    
    /* * Print usage... */
    ostringstream os;
    os  << endl << "Feeder usage: ... NO PARAMETERS BY NOW ..." << endl;
    fputs(os.str().c_str(), file);
    
}

IFEEDER_BASE
IFEEDER_New(void)
{
    return new MEMORY_FEEDER_CLASS();
}

/////////////////////////////////////////////////////////////////////////////

