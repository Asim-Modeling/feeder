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
 * @brief Instruction feeder base support
 */

//
// ASIM core
//
#include "asim/syntax.h"
#include "asim/thread.h"
#include "asim/cmd.h"
#include "asim/trace.h"

// ASIM local module
#include "instfeederbase.h"

//
// Linked list of active feeders.
//
IFEEDER_BASE IFEEDER_BASE_CLASS::activeFeederHead = NULL;


IFEEDER_BASE_CLASS::IFEEDER_BASE_CLASS(
    const char *name,
    IFEEDER_BASE parentFeeder,
    FEEDER_TYPE fType
)
  : feederName(name),
    feederType(fType)
{
    SetTraceableName(name);
    T1("constructing new feeder");

    parent = parentFeeder;
    child = NULL;
    nActiveThreads = 0;

    nextActiveFeeder = activeFeederHead;
    activeFeederHead = this;

    // This isn't passed in the constructor because we should get rid of it
    macroType = IFEED_MACRO_UNKNOWN;

    features = std::vector<bool>(IFEED_LAST);
    features.clear();
}


IFEEDER_BASE_CLASS::~IFEEDER_BASE_CLASS()
{
    //
    // Remove feeder from list of active feeders
    //
    if (activeFeederHead == this)
    {
        activeFeederHead = nextActiveFeeder;
    }
    else
    {
        IFEEDER_BASE f = activeFeederHead;
        while (f->nextActiveFeeder != NULL)
        {
            if (f->nextActiveFeeder == this)
            {
                f->nextActiveFeeder = nextActiveFeeder;
                break;
            }
            f = f->nextActiveFeeder;
        }
    }
}   


void
IFEEDER_BASE_CLASS::DumpAllFeederStats(STATE_OUT state_out)
{
    IFEEDER_BASE f = activeFeederHead;
    UINT32 i=0;
    char str[40];

    //if there is a feeder then dump its stats
    while (f != NULL)
    {
        if (f->GetFeederName()==NULL)
        {
            sprintf(str,"%d",i);
            state_out->AddCompound("Feeder", str);
        }
        else
        {
            state_out->AddCompound("Feeder", f->GetFeederName());
        }
        f->DumpStats(state_out);
        state_out->CloseCompound();

        f = f->nextActiveFeeder;
        i++;
    }

}
void 
IFEEDER_BASE_CLASS::DumpAllFeederState(const char * file_name, FUNCTIONAL_MODEL_DUMP_TYPE type)
{
    IFEEDER_BASE f = activeFeederHead;
    while (f != NULL)
    {
        f->DumpState(file_name,type);

        f = f->nextActiveFeeder;
    }

}

void 
IFEEDER_BASE_CLASS::LoadAllFeederState(const char * file_name, FUNCTIONAL_MODEL_DUMP_TYPE type)
{
    IFEEDER_BASE f = activeFeederHead;
    while (f != NULL)
    {
        f->LoadState(file_name,type);

        f = f->nextActiveFeeder;
    }
}



void
IFEEDER_BASE_CLASS::ClearAllFeederStats()
{
    IFEEDER_BASE f = activeFeederHead;
    UINT32 i=0;

    //if there is a feeder then clear its stats
    while (f != NULL)
    {
        f->ClearStats();
        f = f->nextActiveFeeder;
        i++;
    }

}


void
IFEEDER_BASE_CLASS::AllDone()
{
    IFEEDER_BASE f = activeFeederHead;
    while (f != NULL)
    {
        f->Done();
        f = f->nextActiveFeeder;
    }
}


void
IFEEDER_BASE_CLASS::DeleteAllFeeders()
{
    IFEEDER_BASE f = activeFeederHead;

    if (f == NULL) return;

    while (f != NULL)
    {
        IFEEDER_BASE n = f->nextActiveFeeder;
        delete f;
        f = n;
    }
}


//
// ITranslate --
//    Default implementation of ITranslate of a region.  Returns a
//    translation for the first byte and a descriptor for the next
//    untranslated region.  The next region size is 0 when done.
//    Feeders with different page sizes or more complex page mappings
//    are free to reimplement this virtual function.
//
bool
IFEEDER_BASE_CLASS::ITranslate(
    IFEEDER_STREAM_HANDLE stream,
    const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
    UINT64& pa,
    PAGE_TABLE_INFO pt_info,
    MEMORY_VIRTUAL_REFERENCE_CLASS& vNextRegion)
{
    ASSERT(pt_info == NULL, "Feeder cannot supply page table info");

    UINT64 va = vRegion.GetVA();
    UINT64 nBytes = vRegion.GetNBytes();

    bool r = ITranslate(stream, 0, va, pa);

    //
    // Use a heuristic to decide whether the original region crosses a page.
    // We could actually call ITranslate() but we'll let the caller do
    // that instead.
    //
    UINT64 bytesLeftOnPage = 4096 - (va & 4095);
    if (nBytes <= bytesLeftOnPage)
    {
        vNextRegion = MEMORY_VIRTUAL_REFERENCE_CLASS(0, 0);
    }
    else
    {
        vNextRegion = MEMORY_VIRTUAL_REFERENCE_CLASS(va + bytesLeftOnPage,
                                                     nBytes - bytesLeftOnPage);
    }

    return r;
}


//
// DTranslate --
//    Default implementation of DTranslate of a region.  Returns a
//    translation for the first byte and a descriptor for the next
//    untranslated region.  The next region size is 0 when done.
//    Feeders with different page sizes or more complex page mappings
//    are free to reimplement this virtual function.
//
bool
IFEEDER_BASE_CLASS::DTranslate(
    ASIM_INST inst,
    const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
    UINT64& pa,
    PAGE_TABLE_INFO pt_info,
    MEMORY_VIRTUAL_REFERENCE_CLASS& vNextRegion)
{
    ASSERT(pt_info == NULL, "Feeder cannot supply page table info");
    ASSERTX(&vRegion != &vNextRegion);

    UINT64 va = vRegion.GetVA();
    UINT64 nBytes = vRegion.GetNBytes();

    bool r = DTranslate(inst, va, pa);

    //
    // Use a heuristic to decide whether the original region crosses a page.
    // We could actually call DTranslate() but we'll let the caller do
    // that instead.
    //
    UINT64 bytesLeftOnPage = 4096 - (va & 4095);
    if (nBytes <= bytesLeftOnPage)
    {
        vNextRegion = MEMORY_VIRTUAL_REFERENCE_CLASS(0, 0);
    }
    else
    {
        vNextRegion = MEMORY_VIRTUAL_REFERENCE_CLASS(va + bytesLeftOnPage,
                                                     nBytes - bytesLeftOnPage);
    }

    return r;
}


//----------------------------------------------------------------
// Default versions of symbol lookup functions.  Return errors
// if the feeder claimed to handle symbol lookups.  Otherwise
// just return NULL.
//----------------------------------------------------------------

UINT64
IFEEDER_BASE_CLASS::Symbol(
    IFEEDER_STREAM_HANDLE stream,
    char* name)
{
    ASSERT( ! IsCapable(IFEED_SYMBOL_LOOKUP),
            "Feeder claimed to handle Symbol() method but didn't define it.");
    return 0;
}


UINT64
IFEEDER_BASE_CLASS::Symbol(
    char *name)
{
    ASSERT( ! IsCapable(IFEED_GLOBAL_SYMBOLS),
            "Feeder requires thread handle in Symbol() method.");
    ASSERT( ! IsCapable(IFEED_SYMBOL_LOOKUP),
            "Feeder claimed to handle Symbol() method but didn't define it.");
    return 0;
}


char*
IFEEDER_BASE_CLASS::Symbol_Name(
    IFEEDER_STREAM_HANDLE stream,
    UINT64 address,
    UINT64& offset)
{
    ASSERT( ! IsCapable(IFEED_SYMBOL_LOOKUP),
            "Feeder claimed to handle Symbol_Name() method but didn't define it.");
    return NULL;
}


UINT64
IFEEDER_BASE_CLASS::SingleFeederSymbolHack(
    char *name)
{
    ASSERT((activeFeederHead != NULL) && (activeFeederHead->nextActiveFeeder == NULL),
           "Exactly one feeder must be active");

    ASSERT(activeFeederHead->IsCapable(IFEED_GLOBAL_SYMBOLS),
           "Feeder doesn't support global symbol lookup.");

    return activeFeederHead->Symbol(name);
}


//----------------------------------------------------------------
//
// Thread management -- Hide the underlying representation of a
//     thread.  Feeders use this generic interface.
//
//----------------------------------------------------------------

IFEEDER_THREAD_CLASS::IFEEDER_THREAD_CLASS(
    IFEEDER_BASE feeder,
    IFEEDER_STREAM_HANDLE stream,
    IADDR_CLASS startPC) :
    feeder(feeder)
{
    //
    // Threads are created by the bottom feeder in the hierarchy and associated
    // with the top feeder.  Walk the feeder hierarchy to find the root.
    //
    // In the process, update the count of active threads for each level.
    //
    IFEEDER_BASE parent = feeder;
    while (parent->GetParentFeeder() != NULL)
    {
        parent->NoteThreadActivated();
        parent = parent->GetParentFeeder();
    }

    parent->NoteThreadActivated();
    thread = new ASIM_THREAD_CLASS(parent, stream, startPC);

    SetTraceableName("IFEEDER_THREAD_CLASS");

    T1("Starting new thread, streamId = " << stream
          << ", at pc " << startPC);

    CMD_ThreadBegin(thread);
}

IFEEDER_THREAD_CLASS::~IFEEDER_THREAD_CLASS()
{
    delete thread;
}

void
IFEEDER_THREAD_CLASS::ThreadEnd(void)
{
    //
    // Update the count of active threads
    //
    IFEEDER_BASE f = feeder;
    while (f)
    {
        f->NoteThreadDeactivated();
        f = f->GetParentFeeder();
    }

    CMD_ThreadEnd(thread);
    CheckExitConditions(THREAD_END);    
}

void
IFEEDER_THREAD_CLASS::SetVirtualPc(
    IADDR_CLASS vpc)
{
    thread->SetVirtualPc(vpc);
}
