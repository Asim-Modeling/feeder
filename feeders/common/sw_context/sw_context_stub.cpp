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
 * 
 */

/**
 * @file
 * @brief Software context stub just to make ape model work
 *
 */

// ASIM local stuff
#include "asim/provides/software_context.h"

// Initialize the shared ID, which is incremented each time an instance
// of SW_CONTEXT_CLASS is created, to generate an object-specific unique ID.

UID_GEN32 SW_CONTEXT_CLASS::uniqueStaticID = 0;


void
SW_CONTEXT_CLASS::WarmUpClientInfo(const WARMUP_CLIENTS clientInfo)
{
    iFeeder->WarmUpClientInfo(clientInfo);
}


bool
SW_CONTEXT_CLASS::WarmUp(WARMUP_INFO warmup)
{
    warmup->NoteSWContext(this);
    return iFeeder->WarmUp(sHandle, warmup);
}


ASIM_INST
SW_CONTEXT_CLASS::Fetch(UINT64 cycle, IADDR_CLASS ip)
{
    ASIM_INST ainst = new ASIM_INST_CLASS(this);
    iFeeder->Fetch(sHandle, ip, ainst, cycle);
    return ainst;
}

void
SW_CONTEXT_CLASS::Issue(ASIM_INST ainst)
{
    iFeeder->Issue(ainst);
}

void
SW_CONTEXT_CLASS::Execute(ASIM_INST ainst)
{
    iFeeder->Execute(ainst);
}


void
SW_CONTEXT_CLASS::Commit(ASIM_INST ainst)
{
    iFeeder->Commit(ainst);
}



void
SW_CONTEXT_CLASS::Kill(
    ASIM_INST ainst,
    bool fetchNext,
    bool killMe)
{
    iFeeder->Kill(ainst, fetchNext, killMe);
}

/* -------------------------Peek------------------------------------*/

/**
 * Get info from the feeder (in x86, this happens to be the macro inst)..
 * Called from the hardware context mapped to this software context.
 *
 */
/*
FEEDER_PEEK_INFO
SW_CONTEXT_CLASS::Peek(
    IADDR_CLASS pc)
{
    ASSERT(false, "Peek feeder function not supported in sw_context_stub.h\n");
    return NULL;
}
*/

/**
 * read the memory location for this instruction
 */
bool
SW_CONTEXT_CLASS::DoRead(
    ASIM_INST ainst)
{
    ASSERT(false, "DoRead function not supported in sw_context_stub.h\n");
    return  iFeeder->DoRead(ainst);
}

/**
 * speculatively write the memory location for this instruction
 */
bool
SW_CONTEXT_CLASS::DoSpecWrite(
    ASIM_INST ainst)
{
    ASSERT(false, "DoSpecWrite function not supported in sw_context_stub.h\n");
//    iFeeder->DoSpecWrite(ainst);
    return false;
}

/**
 * write the memory location for this instruction
 */
bool
SW_CONTEXT_CLASS::DoWrite(
    ASIM_INST ainst)
{
    ASSERT(false, "DoWrite feeder function not supported in sw_context_stub.h\n");
//    iFeeder->DoWrite(ainst);
    return false;
}

/**
 * translate the va to a pa for this instruction
 */
bool
SW_CONTEXT_CLASS::DTranslate(
    ASIM_INST ainst,
    UINT64 va,
    UINT64 &pa)
{
    ASSERT(false, "DTranslate feeder function not supported in sw_context_stub.h\n");
    return false;
//    return iFeeder->DTranslate(ainst, va, pa);
}

inline bool
SW_CONTEXT_CLASS::DTranslate(
    ASIM_INST ainst,
    const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
    UINT64& pa,
    PAGE_TABLE_INFO pt_info,
    MEMORY_VIRTUAL_REFERENCE_CLASS& vNextRegion)
{
    ASSERT(false, "DTranslate feeder function not supported in sw_context_stub.h\n");
    return false;
}

/**
 * translate the va to a pa
 */
bool
SW_CONTEXT_CLASS::ITranslate(
    UINT32 hwcNum,
    UINT64 va,
    UINT64 &pa)
{
    ASSERT(false, "Itranslate feeder function not supported in sw_context_stub.h\n");
    return false;
//    return iFeeder->ITranslate(GetFeederStreamHandle(), hwcNum, va, pa);
}

inline bool
SW_CONTEXT_CLASS::ITranslate(
    const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
    UINT64& pa,
    PAGE_TABLE_INFO pt_info,
    MEMORY_VIRTUAL_REFERENCE_CLASS& vNextRegion)
{
    ASSERT(false, "ITranslate feeder function not supported in sw_context_stub.h\n");
    return false;
}

INT32
SW_CONTEXT_CLASS::GetHWCNum(void)
{
    if (hwc)
    {
        return(hwc->GetUID());
    }
    else
    {
        return (-1);
    }
}
