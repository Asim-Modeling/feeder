/**************************************************************************
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
 **************************************************************************/

/**
 * @file
 * @author Michael Adler
 * @brief Replay buffer for SoftSDV feeder.
 */


//
// Calling this module REPLAY is a bit of a misnomer.  The data structures
// here maintain an ordered list of all active instructions that have come
// from SoftSDV.  The list is used for replaying instruction streams
// to handle good-path instructions that are killed and then reexecuted.
// However, the data structures here do more than that.  They are the sole
// source of information about the SoftSDV instructions and their
// associated register and memory values.
//


#ifndef _SOFTSDV_REPLAY_H
#define _SOFTSDV_REPLAY_H

#include <list>

// ASIM public modules
#include "asim/provides/softsdv_stub.h"


class SOFTSDV_REPLAY_BUFFER_CLASS;
typedef class SOFTSDV_REPLAY_BUFFER_CLASS *SOFTSDV_REPLAY_BUFFER;

typedef class SOFTSDV_REP_MEMORY_REGIONS_CLASS *SOFTSDV_REP_MEMORY_REGIONS;

//
// SOFTSDV_LIVE_INSTRUCTION_CLASS holds a single instruction that came from
// SoftSDV in case it is killed and needs to be replayed.
//
class SOFTSDV_LIVE_INSTRUCTION_CLASS
{
  public:
    SOFTSDV_LIVE_INSTRUCTION_CLASS(SOFTSDV_REPLAY_BUFFER replayQueue) :
        replayQueue(replayQueue),
        inputRegValues(NULL),
        repMemRegions(NULL),
        prev(NULL),
        next(NULL),
        implicitCommit(false),
        artificialInstr(false)
    {};

    ~SOFTSDV_LIVE_INSTRUCTION_CLASS();

    const ASIM_SOFTSDV_INST_INFO_CLASS *GetSdvInstr() const { return &softsdvInstr; };
    const ASIM_SOFTSDV_REG_INFO_CLASS *GetInputRegValues() const { return inputRegValues; };
    const UINT64 GetUid() const { return uid; };

    //
    // Linked list of these instructions
    //
    SOFTSDV_LIVE_INSTRUCTION_CLASS *GetNext() const { return next; };
    SOFTSDV_LIVE_INSTRUCTION_CLASS *GetPrev() const { return prev; };

    //
    // Is the instruction artificial (injected jump, bad path, etc.)?
    //
    bool IsArtificialInstr(void) const { return artificialInstr; };

    //
    // Does the instruction commit implicitly (e.g. IPF NOP)?
    //
    bool IsImplicitCommit(void) const { return implicitCommit; };

    //
    // Set methods for the access methods above
    //
    void SetSdvInstr(CONST_ASIM_SOFTSDV_INST_INFO instr) { softsdvInstr = *instr; };
    void SetInputRegValues(ASIM_SOFTSDV_REG_INFO regs) { inputRegValues = regs; };
    void SetUid(UINT64 id) { uid = id; };

    void SetNext(SOFTSDV_LIVE_INSTRUCTION_CLASS *n) { next = n; };
    void SetPrev(SOFTSDV_LIVE_INSTRUCTION_CLASS *p) { prev = p; };

    void SetArtificialInstr(bool b) { artificialInstr = b; };

    //
    // Tag an instruction with implicit commit (e.g. an injected NOP on
    // a bad path.)  No commit message is guaranteed for these instructions.
    //
    void SetImplicitCommit(bool b) { implicitCommit = b; };

    //
    // REP memory reference regions from a merged set of instructions.
    // These override the memory reference descriptors for the SoftSDV
    // instruction.
    //
    void SetREPMemRegions(SOFTSDV_REP_MEMORY_REGIONS regions);

    //
    // More REP support code -- these functions allow updates to
    // the memory reference sizes in a region.  The REP merge code
    // is merely caching merged virtual regions.  The actual data
    // about the merged regions is stored in the SOFTSDV_REP_MEMORY_REGIONS
    // above.
    //
    void UpdateLoadVirtualRegion(UINT32 idx, MEMORY_VIRTUAL_REFERENCE_CLASS vRef);
    void UpdateStoreVirtualRegion(UINT32 idx, MEMORY_VIRTUAL_REFERENCE_CLASS vRef);

    //
    // Actions to perform on an instruction...
    //

    //
    // CommitInstr the instruction -- remove it from the replay buffer.
    //   If a tagged instruction is noticed during the commit (e.g. an IPF
    //   tagged NOP) then the value is written to taggedInstrValue.
    //   Unfortunately the tag value check has to be here since NOPs commit
    //   implicitly and won't be seen directly in the feeder's commit
    //   function.
    //
    UINT32 CommitInstr(INT32 *taggedInstrValue);

    //
    // DeleteInputRegValues -- drop register values from this instruction.
    //
    void DeleteInputRegValues(void);

    //
    // DeleteREPMemRegions -- drop REP merged memory regions.
    //
    void DeleteREPMemRegions(void);

    //
    // Delete an instruction from the queue.  Usually used when two SoftSDV
    //   instructions are merged into a single logical instruction for Asim.
    //   E.g. x86 REP instructions.
    //
    void DeleteInstr(void);

    //
    // Memory references -- access the standard instruction memory reference
    // methods through this class since the SoftSDV instruction's memory
    // reference data may be overridden.  This happens on x86 when REP
    // instructions are merged.
    //
    UINT32 NLoads(void) const;
    const ASIM_SOFTSDV_MEM_ACCESS_CLASS *GetLoad(UINT32 idx);
    UINT32 NStores(void) const;
    const ASIM_SOFTSDV_MEM_ACCESS_CLASS *GetStore(UINT32 idx);

  private:
    //
    // Parent queue holding this instruction
    //
    SOFTSDV_REPLAY_BUFFER replayQueue;

    //
    // Copy of the instruction that came from SoftSDV
    //
    ASIM_SOFTSDV_INST_INFO_CLASS softsdvInstr;

    //
    // Input register values from SoftSDV
    //
    ASIM_SOFTSDV_REG_INFO inputRegValues;

    //
    // Memory descriptors for merged REP instructions.  If present, these
    // override the regions found in the SoftSDV instruction.
    //
    SOFTSDV_REP_MEMORY_REGIONS repMemRegions;

    //
    // Ordered linked list of active instructions
    //
    SOFTSDV_LIVE_INSTRUCTION_CLASS *prev;
    SOFTSDV_LIVE_INSTRUCTION_CLASS *next;

    UINT64 uid;

    bool implicitCommit;    // Automatically commits (e.g. NOP)
    bool artificialInstr;   // Artificial instruction generated
                            // by next instr when needed.  (e.g.
                            // JMP to interrupt)
};

typedef SOFTSDV_LIVE_INSTRUCTION_CLASS *SOFTSDV_LIVE_INSTRUCTION;


//
// Make a SOFTSDV_LIVE_INSTRUCTION from an Asim macro instruction.
//
inline SOFTSDV_LIVE_INSTRUCTION_CLASS * SdvHandleFromMacroInstr(
    const ASIM_MACRO_INST_CLASS *instr)
{
    return (SOFTSDV_LIVE_INSTRUCTION_CLASS *)(instr->GetTraceID());
};


//
// Manage the replay buffer -- a linked list of SOFTSDV_LIVE_INSTRUCTIONs.
//
class SOFTSDV_REPLAY_BUFFER_CLASS
{
  public:
    SOFTSDV_REPLAY_BUFFER_CLASS() :
        head(NULL),
        tail(NULL),
        nextFetch(NULL),
        freeList(NULL),
        uid(0)
    {};

    ~SOFTSDV_REPLAY_BUFFER_CLASS();

    //
    // Append a new instruction to the replay queue.
    //
    SOFTSDV_LIVE_INSTRUCTION AddInstruction(ASIM_SOFTSDV_INST_INFO newInstr);

    //
    // Append an artificial place holder in the replay queue for an instruction
    // that was injected in the stream (e.g. a JMP to an interrupt).  The
    // placeholder is needed in case the injected instruction is killed and
    // execution has to be resumed after it.  The softsdvInstr record is
    // invalid in an artificial entry.  If the instruction needs to be
    // reinjected that must happen as a side-effect of executing the next
    // real instruction in the replay queue.
    //
    SOFTSDV_LIVE_INSTRUCTION
    AddArtificialPlaceHolder(SOFTSDV_LIVE_INSTRUCTION realInstr);

    //
    // Get the handle of the next instruction to be fetched from the
    // replay queue.  This may include artificial instructions.
    //
    SOFTSDV_LIVE_INSTRUCTION GetNextFetchHandle(void);

    //
    // Get the handle of the next instruction to be fetched from the
    // replay queue, skipping artificial instructions.
    //
    SOFTSDV_LIVE_INSTRUCTION GetNextRealFetchHandle(void);

    //
    // Set the next instruction to fetch.  Return the number of instructions
    // between what used to be the next instruction and the new one.  This
    // is useful in tracking the number of killed instructions.
    //
    UINT64 SetNextFetch(SOFTSDV_LIVE_INSTRUCTION liveInstr);
        
    //
    // The feeder needs a heuristic for when to prefetch instructions from
    // the incoming SoftSDV->Asim ring buffer.  This function offers a clue
    // of how many instructions have been fetched.  If Asim fetches too
    // far ahead it could change multi-CPU synchronization or make replay
    // more difficult.
    //
    UINT64 NUnfetchedQueuedInstrs(void);

    //
    // Register values come from SoftSDV as separate messages because
    // the data structure is too big to fit with each instruction and
    // is used very infrequently.  Save lists of them here until an
    // instruction comes in that refers to a record.
    //
    // A local copy of the incoming register value class is allocated
    // in PushRegisterValues.  The values must be pushed before the
    // instruction that refers to them arrives.  AddInstruction() above
    // associates the register values with the correct instruction.
    // The storage is relased automatically when CommitInstruction()
    // is called.
    //
    void PushRegisterValues(const ASIM_SOFTSDV_REG_INFO_CLASS& rv);
  private:
    ASIM_SOFTSDV_REG_INFO PopRegisterValues(void);
    void DeallocateRegisterValues(ASIM_SOFTSDV_REG_INFO rv);

  private:
    friend class SOFTSDV_LIVE_INSTRUCTION_CLASS;

    //
    // CommitInstruction -- called only by the Commit() method in the
    //    SOFTSDV_LIVE_INSTRUCTION_CLASS to commit one instruction.
    //
    UINT32 CommitInstruction(
        SOFTSDV_LIVE_INSTRUCTION liveInstr,
        INT32 *taggedInstrValue);

    //
    // DeleteInstruction -- called only by the Delete() method in the
    //    SOFTSDV_LIVE_INSTRUCTION_CLASS to remove the instruction from
    //    the queue.
    //
    void DeleteInstruction(
        SOFTSDV_LIVE_INSTRUCTION liveInstr);

    //
    // The replay buffer keeps its own free list to avoid repeated calls
    // to the memory subsystem for a handful of objects of the same size.
    //
    SOFTSDV_LIVE_INSTRUCTION AllocateLiveInstr(void);
    void DeallocateLiveInstr(SOFTSDV_LIVE_INSTRUCTION oldInstr);

    SOFTSDV_LIVE_INSTRUCTION head;
    SOFTSDV_LIVE_INSTRUCTION tail;
    SOFTSDV_LIVE_INSTRUCTION nextFetch;

    SOFTSDV_LIVE_INSTRUCTION freeList;

    // Management of register values from SoftSDV
    list<ASIM_SOFTSDV_REG_INFO> regValuesList;
    list<ASIM_SOFTSDV_REG_INFO> regValuesFreeList;  // Unused reg info buffers

    UINT64 uid;
};


//
// SOFTSDV_REP_MEMORY_REGIONS_CLASS --
//     Special case memory descriptor for merged x86 REP instructions.
//
class SOFTSDV_REP_MEMORY_REGIONS_CLASS
{
  public:
    SOFTSDV_REP_MEMORY_REGIONS_CLASS(UINT32 nLoads, UINT32 nStores);
    ~SOFTSDV_REP_MEMORY_REGIONS_CLASS();

    //
    // Set memory region info
    //
    void SetLoad(UINT32 idx, ASIM_SOFTSDV_MEM_ACCESS ref);
    void SetStore(UINT32 idx, ASIM_SOFTSDV_MEM_ACCESS ref);

    //
    // Access memory region info
    //
    UINT32 NLoads(void) const { return nLoads; };
    const ASIM_SOFTSDV_MEM_ACCESS_CLASS *GetLoad(UINT32 idx) { return loads[idx]; };
    UINT32 NStores(void) const { return nStores; };
    const ASIM_SOFTSDV_MEM_ACCESS_CLASS *GetStore(UINT32 idx) { return stores[idx]; };

  private:
    ASIM_SOFTSDV_MEM_ACCESS *loads;
    ASIM_SOFTSDV_MEM_ACCESS *stores;

    UINT32 nLoads;
    UINT32 nStores;
};


inline void
SOFTSDV_REP_MEMORY_REGIONS_CLASS::SetLoad(
    UINT32 idx,
    ASIM_SOFTSDV_MEM_ACCESS ref)
{
    ASSERTX(idx < nLoads);
    loads[idx] = ref;
};

inline void
SOFTSDV_REP_MEMORY_REGIONS_CLASS::SetStore(UINT32 idx, ASIM_SOFTSDV_MEM_ACCESS ref)
{
    ASSERTX(idx < nStores);
    stores[idx] = ref;
};




inline UINT32
SOFTSDV_LIVE_INSTRUCTION_CLASS::NLoads(void) const
{
    if (repMemRegions)
    {
        return repMemRegions->NLoads();
    }
    else
    {
        return softsdvInstr.NLoads();
    }
};


inline const ASIM_SOFTSDV_MEM_ACCESS_CLASS *
SOFTSDV_LIVE_INSTRUCTION_CLASS::GetLoad(UINT32 idx)
{
    if (repMemRegions)
    {
        return repMemRegions->GetLoad(idx);
    }
    else
    {
        return softsdvInstr.GetLoad(idx);
    }
};


inline UINT32
SOFTSDV_LIVE_INSTRUCTION_CLASS::NStores(void) const
{
    if (repMemRegions)
    {
        return repMemRegions->NStores();
    }
    else
    {
        return softsdvInstr.NStores();
    }
};


inline const ASIM_SOFTSDV_MEM_ACCESS_CLASS *
SOFTSDV_LIVE_INSTRUCTION_CLASS::GetStore(UINT32 idx)
{
    if (repMemRegions)
    {
        return repMemRegions->GetStore(idx);
    }
    else
    {
        return softsdvInstr.GetStore(idx);
    }
};


inline UINT32
SOFTSDV_LIVE_INSTRUCTION_CLASS::CommitInstr(INT32 *taggedInstrValue) 
{
    return replayQueue->CommitInstruction(this, taggedInstrValue);
};


inline void
SOFTSDV_LIVE_INSTRUCTION_CLASS::DeleteInstr(void)
{
    return replayQueue->DeleteInstruction(this);
};


inline void
SOFTSDV_LIVE_INSTRUCTION_CLASS::DeleteInputRegValues(void)
{
    if (inputRegValues != NULL)
    {
        replayQueue->DeallocateRegisterValues(inputRegValues);
        inputRegValues = NULL;
    }
};


inline void
SOFTSDV_LIVE_INSTRUCTION_CLASS::DeleteREPMemRegions(void)
{
    if (repMemRegions != NULL)
    {
        delete repMemRegions;
        repMemRegions = NULL;
    }
};


inline void
SOFTSDV_LIVE_INSTRUCTION_CLASS::SetREPMemRegions(
    SOFTSDV_REP_MEMORY_REGIONS regions)
{
    repMemRegions = regions;
};



inline SOFTSDV_LIVE_INSTRUCTION
SOFTSDV_REPLAY_BUFFER_CLASS::GetNextFetchHandle(void)
{
    SOFTSDV_LIVE_INSTRUCTION fetch = nextFetch;

    if (fetch != NULL)
    {
        nextFetch = fetch->GetNext();
    }

    return fetch;
};


inline SOFTSDV_LIVE_INSTRUCTION
SOFTSDV_REPLAY_BUFFER_CLASS::GetNextRealFetchHandle(void)
{
    SOFTSDV_LIVE_INSTRUCTION fetch = nextFetch;

    while (fetch != NULL && fetch->IsArtificialInstr())
    {
        fetch = fetch->GetNext();
    }
            
    if (fetch != NULL)
    {
        nextFetch = fetch->GetNext();
    }

    return fetch;
};


inline UINT64
SOFTSDV_REPLAY_BUFFER_CLASS::SetNextFetch(SOFTSDV_LIVE_INSTRUCTION liveInstr)
{
    UINT64 nKilled = 0;

    if (liveInstr != NULL)
    {
        if (nextFetch == NULL)
        {
            nKilled = uid - liveInstr->GetUid();
        }
        else
        {
            nKilled = nextFetch->GetUid() - liveInstr->GetUid();
        }
    }

    nextFetch = liveInstr;

    return nKilled;
};
        

inline UINT64
SOFTSDV_REPLAY_BUFFER_CLASS::NUnfetchedQueuedInstrs(void)
{
    if (nextFetch == NULL)
    {
        return 0;
    }
    else
    {
        return uid - nextFetch->GetUid() + 1;
    }
};


#endif // _SOFTSDV_REPLAY_H
