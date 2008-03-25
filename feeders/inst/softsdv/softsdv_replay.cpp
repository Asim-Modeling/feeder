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

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/trace.h"

// ASIM public modules
#include "asim/provides/instfeeder_interface.h"
#include "asim/provides/instfeeder_implementation.h"


//
// ~SOFTSDV_LIVE_INSTRUCTION_CLASS --
//    Release all the extra storage associated with an instruction.
//    This only gets called when the feeder is being destroyed.  During
//    the run the live instruction storage is kept on a private free
//    list and never returned to the system.
//
SOFTSDV_LIVE_INSTRUCTION_CLASS::~SOFTSDV_LIVE_INSTRUCTION_CLASS()
{
    if (repMemRegions != NULL)
    {
        delete repMemRegions;
    }

    if (inputRegValues != NULL)
    {
        delete inputRegValues;
    }
}


void
SOFTSDV_LIVE_INSTRUCTION_CLASS::UpdateLoadVirtualRegion(
    UINT32 idx,
    MEMORY_VIRTUAL_REFERENCE_CLASS vRef)
{
    ASIM_SOFTSDV_MEM_ACCESS ref = softsdvInstr.GetLoad(idx);
    ref->SetVA(vRef.GetVA());
    ref->SetSize_NOCHECK(vRef.GetNBytes());
}


void
SOFTSDV_LIVE_INSTRUCTION_CLASS::UpdateStoreVirtualRegion(
    UINT32 idx,
    MEMORY_VIRTUAL_REFERENCE_CLASS vRef)
{
    ASIM_SOFTSDV_MEM_ACCESS ref = softsdvInstr.GetStore(idx);
    ref->SetVA(vRef.GetVA());
    ref->SetSize_NOCHECK(vRef.GetNBytes());
}



//
// ~SOFTSDV_REPLAY_BUFFER_CLASS --
//    Release all the extra storage associated with an instruction queue.
//
SOFTSDV_REPLAY_BUFFER_CLASS::~SOFTSDV_REPLAY_BUFFER_CLASS()
{
    SOFTSDV_LIVE_INSTRUCTION liveInstr = head;
    SOFTSDV_LIVE_INSTRUCTION nextInstr;

    //
    // Free all the live instruction buffers
    //
    while (liveInstr)
    {
        nextInstr = liveInstr->GetNext();
        delete liveInstr;
        liveInstr = nextInstr;
    }

    //
    // Release all the storage from the free list
    //
    while (freeList)
    {
        nextInstr = freeList->GetNext();
        delete freeList;
        freeList = nextInstr;
    }

    //
    // Release all the register values storage from the free list
    //
    while (! regValuesFreeList.empty())
    {
        ASIM_SOFTSDV_REG_INFO rv = regValuesFreeList.front();
        regValuesFreeList.pop_front();
        delete rv;
    }
}


SOFTSDV_LIVE_INSTRUCTION
SOFTSDV_REPLAY_BUFFER_CLASS::AddInstruction(
    ASIM_SOFTSDV_INST_INFO newInstr)
{
    SOFTSDV_LIVE_INSTRUCTION liveInstr = AllocateLiveInstr();

    liveInstr->SetSdvInstr(newInstr);
    liveInstr->SetInputRegValues(NULL);
    liveInstr->SetUid(++uid);
    liveInstr->SetNext(NULL);
    liveInstr->SetPrev(tail);
    liveInstr->SetImplicitCommit(false);
    liveInstr->SetArtificialInstr(false);

    //
    // Does the instruction have input register values?
    //
    UINT64 regsUid = newInstr->GetRegsUid();
    if (regsUid != 0)
    {
        ASIM_SOFTSDV_REG_INFO rInfo = PopRegisterValues();
        while ((rInfo != NULL) && (rInfo->GetUid() < regsUid))
        {
            // This register state was passed but never used in an instruction
            DeallocateRegisterValues(rInfo);
            rInfo = PopRegisterValues();
        }
        
        VERIFY((rInfo != NULL) && (regsUid == rInfo->GetUid()),
               "Register values arrived out of order.");

        liveInstr->SetInputRegValues(rInfo);
    }

    if (tail == NULL)
    {
        head = liveInstr;
        tail = liveInstr;
    }
    else
    {
        tail->SetNext(liveInstr);
        tail = liveInstr;
    }

    //
    // Make sure the instruction isn't lost
    //
    if (nextFetch == NULL)
    {
        nextFetch = liveInstr;
    }

    return liveInstr;
}


SOFTSDV_LIVE_INSTRUCTION
SOFTSDV_REPLAY_BUFFER_CLASS::AddArtificialPlaceHolder(
    SOFTSDV_LIVE_INSTRUCTION realInstr)
{
    SOFTSDV_LIVE_INSTRUCTION liveInstr = AllocateLiveInstr();

    liveInstr->SetInputRegValues(NULL);
    liveInstr->SetUid(++uid);
    liveInstr->SetImplicitCommit(true);
    liveInstr->SetArtificialInstr(true);

    liveInstr->SetPrev(realInstr->GetPrev());
    liveInstr->SetNext(realInstr);
    realInstr->SetPrev(liveInstr);

    if (liveInstr->GetPrev() == NULL)
    {
        head = liveInstr;
    }
    else
    {
        liveInstr->GetPrev()->SetNext(liveInstr);
    }

    //
    // Copy properties of the real instruction
    //
    CONST_ASIM_SOFTSDV_INST_INFO realSdvInstr = realInstr->GetSdvInstr();
    liveInstr->SetSdvInstr(realSdvInstr);

    //
    // Make sure the instruction isn't lost
    //
    if (nextFetch == NULL)
    {
        nextFetch = liveInstr;
    }

    return liveInstr;
}


UINT32
SOFTSDV_REPLAY_BUFFER_CLASS::CommitInstruction(
    SOFTSDV_LIVE_INSTRUCTION liveInstr,
    INT32 *taggedInstrValue)
{
    ASSERTX(nextFetch != liveInstr);
    UINT32 num_implicitCommit = 0;
    //
    // Remove implicit commiting instructions from the head of the list
    //
    while (head && head->IsImplicitCommit() && (head != liveInstr))
    {
        if (! head->IsArtificialInstr())
        {
           num_implicitCommit += 1;
        }

        if (taggedInstrValue && head->GetSdvInstr()->GetInstructionIsTagged())
        {
            *taggedInstrValue = head->GetSdvInstr()->GetInstrTag();
        }

        SOFTSDV_LIVE_INSTRUCTION prev = head;
        head = head->GetNext();
        head->SetPrev(NULL);
        DeallocateLiveInstr(prev);
    }

    ASSERT(head == liveInstr, "Out of order commit.");

    head = liveInstr->GetNext();

    if (liveInstr->GetNext() == NULL)
    {
        tail = NULL;
    }
    else
    {
        liveInstr->GetNext()->SetPrev(NULL);
    }

    if (taggedInstrValue && liveInstr->GetSdvInstr()->GetInstructionIsTagged())
    {
        *taggedInstrValue = liveInstr->GetSdvInstr()->GetInstrTag();
    }

    DeallocateLiveInstr(liveInstr);
    return num_implicitCommit;
}


void
SOFTSDV_REPLAY_BUFFER_CLASS::DeleteInstruction(
    SOFTSDV_LIVE_INSTRUCTION liveInstr
)
{
    //
    // Drop instruction from linked list...
    //

    if (head == liveInstr)
    {
        head = liveInstr->GetNext();
    }
    else
    {
        liveInstr->GetPrev()->SetNext(liveInstr->GetNext());

        // Move the UID from the instruction being deleted to the previous
        // instruction.  This keeps NUnfetchedQueuedInstrs() a bit more
        // accurate, maintained the guarantee in the feeder that there
        // are always a few incoming instructions buffered following the
        // head of the fetch list.
        liveInstr->GetPrev()->SetUid(liveInstr->GetUid());
    }

    if (tail == liveInstr)
    {
        tail = liveInstr->GetPrev();
    }
    else
    {
        liveInstr->GetNext()->SetPrev(liveInstr->GetPrev());
    }

    if (nextFetch == liveInstr)
    {
        nextFetch = liveInstr->GetNext();
    }

    DeallocateLiveInstr(liveInstr);
}


inline SOFTSDV_LIVE_INSTRUCTION
SOFTSDV_REPLAY_BUFFER_CLASS::AllocateLiveInstr(void)
{
    SOFTSDV_LIVE_INSTRUCTION newInstr;

    //
    // Grab a live instruction from the free list or allocate new storage
    // for it.
    //
    if (freeList)
    {
        newInstr = freeList;
        freeList = freeList->GetNext();
    }
    else
    {
        newInstr = new SOFTSDV_LIVE_INSTRUCTION_CLASS(this);
    }

    return newInstr;
};

inline void
SOFTSDV_REPLAY_BUFFER_CLASS::DeallocateLiveInstr(
    SOFTSDV_LIVE_INSTRUCTION oldInstr)
{
    ASSERTX(oldInstr != NULL);

    oldInstr->DeleteREPMemRegions();
    oldInstr->DeleteInputRegValues();

    oldInstr->SetNext(freeList);
    freeList = oldInstr;
};


//----------------------------------------------------------------
// Register value management
//----------------------------------------------------------------

//
// PushRegisterValues --
//    Allocate a local copy of an incoming register value message and
//    push it on the end of the incoming list.
//
void
SOFTSDV_REPLAY_BUFFER_CLASS::PushRegisterValues(
    const ASIM_SOFTSDV_REG_INFO_CLASS& rv)
{
    //
    // First make a local copy of the register info to release the shared
    // memory ring buffer entry.
    //
    ASIM_SOFTSDV_REG_INFO rInfo;
    if (! regValuesFreeList.empty())
    {
        rInfo = regValuesFreeList.front();
        regValuesFreeList.pop_front();
    }
    else
    {
        rInfo = new ASIM_SOFTSDV_REG_INFO_CLASS();
    }
    *rInfo = rv;

    //
    // Put the register values on the list of incoming states
    //
    regValuesList.push_back(rInfo);
};


//
// PopRegisterValues --
//    Remove the oldest register value class from the head of the list.
//
ASIM_SOFTSDV_REG_INFO
SOFTSDV_REPLAY_BUFFER_CLASS::PopRegisterValues(void)
{
    if (regValuesList.empty()) return NULL;
    ASIM_SOFTSDV_REG_INFO rInfo = regValuesList.front();
    regValuesList.pop_front();
    return rInfo;
};


//
// DeallocateRegisterValues --
//    Free the storage from a register values class.
//
void
SOFTSDV_REPLAY_BUFFER_CLASS::DeallocateRegisterValues(
    ASIM_SOFTSDV_REG_INFO rv)
{
    ASSERTX(rv);
    regValuesFreeList.push_front(rv);
};



//----------------------------------------------------------------
//  Descriptor for REP instruction merged memory regions
//----------------------------------------------------------------


SOFTSDV_REP_MEMORY_REGIONS_CLASS::SOFTSDV_REP_MEMORY_REGIONS_CLASS(
    UINT32 nLoads,
    UINT32 nStores)
    : nLoads(nLoads),
      nStores(nStores)
{
    //
    // The class just holds pointers to regions that are allocated
    // externally.  Allocate space for the pointers.
    //
    loads = new ASIM_SOFTSDV_MEM_ACCESS[nLoads];
    stores = new ASIM_SOFTSDV_MEM_ACCESS[nStores];
};


SOFTSDV_REP_MEMORY_REGIONS_CLASS::~SOFTSDV_REP_MEMORY_REGIONS_CLASS()
{
    //
    // Delete all the storage used by the references.  Value storage
    // for the reference regions must have been allocated with malloc.
    // The region descriptors themselves must have been allocated with
    // C++ new.
    //
    for (unsigned int i = 0; i < nLoads; i++)
    {
        if (loads[i])
        {
            if (loads[i]->HasValue())
            {
                free(loads[i]->GetValue());
            }
            delete loads[i];
        }
    }

    for (unsigned int i = 0; i < nStores; i++)
    {
        if (stores[i])
        {
            if (stores[i]->HasValue())
            {
                free(stores[i]->GetValue());
            }
            delete stores[i];
        }
    }

    delete[] loads;
    delete[] stores;
};
