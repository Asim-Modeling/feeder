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
 * @brief SoftSDV instruction virtual to physical translation
 */

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/trace.h"

// ASIM public modules
#include "asim/provides/instfeeder_interface.h"
#include "asim/provides/instfeeder_implementation.h"

SOFTSDV_ITRANSLATE_CLASS::SOFTSDV_ITRANSLATE_CLASS(void) :
    lastSoftsdvUid(0)
{
    for (TABLE_INDEX i = 0; i < nTableEntries; i++)
    {
        translationTable[i].softsdvUid = 0;
        translationTable[i].virtualPage = 0;
        translationTable[i].physicalPage = 0;
        translationTable[i].artificial = false;
        translationTable[i].prev = i - 1;
        translationTable[i].next = i + 1;
    }
    translationTable[nTableEntries-1].next = -1;
    head = 0;
    tail = nTableEntries - 1;
}

void
SOFTSDV_ITRANSLATE_CLASS::MoveEntryToHead(TABLE_INDEX i)
{
    //
    // Note access to entry.  Lookup() and AddEntry() ultimately reach this
    // point.
    //
    translationTable[i].softsdvUid = lastSoftsdvUid;

    if (head != i)
    {
        TABLE_INDEX prev = translationTable[i].prev;
        TABLE_INDEX next = translationTable[i].next;

        //
        // Remove from list
        //
        translationTable[prev].next = next;
        if (next == -1)
        {
            tail = prev;
        }
        else
        {
            translationTable[next].prev = prev;
        }

        //
        // Push on head
        //
        translationTable[head].prev = i;
        translationTable[i].prev = -1;
        translationTable[i].next = head;
        head = i;
    }
}


void
SOFTSDV_ITRANSLATE_CLASS::DeleteEntry(TABLE_INDEX i)
{
    if (tail != i)
    {
        TABLE_INDEX prev = translationTable[i].prev;
        TABLE_INDEX next = translationTable[i].next;

        //
        // Remove from list
        //
        translationTable[next].prev = prev;
        if (prev == -1)
        {
            head = next;
        }
        else
        {
            translationTable[prev].next = next;
        }

        translationTable[tail].next = i;
        translationTable[i].prev = tail;
        translationTable[i].next = -1;
        tail = i;
    }

    translationTable[i].softsdvUid = 0;
    translationTable[i].virtualPage = 0;
    translationTable[i].physicalPage = 0;
    translationTable[i].artificial = false;
}


INT32
SOFTSDV_ITRANSLATE_CLASS::Lookup(UINT64 va, UINT64 &pa)
{
    UINT64 page = Page(va);

    UINT32 steps = 1;
    TABLE_INDEX i = head;
    while (i != -1)
    {
        if (translationTable[i].virtualPage == page)
        {
            //
            // Hit
            //
            MoveEntryToHead(i);
            pa = translationTable[i].physicalPage + PageOffset(va);
            return steps;
        }

        i = translationTable[i].next;
        steps += 1;
    }

    pa = 0;
    return 0;
}

bool
SOFTSDV_ITRANSLATE_CLASS::IsEntryArtificial(UINT64 va)
{
    UINT64 page = Page(va);

    TABLE_INDEX i = head;
    while (i != -1)
    {
        if (translationTable[i].virtualPage == page)
        {
            return translationTable[i].artificial;
        }

        i = translationTable[i].next;
    }

    return false;
}


bool
SOFTSDV_ITRANSLATE_CLASS::AddEntry(UINT64 va, UINT64 pa, UINT64 softsdvUid)
{
    UINT64 oldpa;

    lastSoftsdvUid = softsdvUid;

    if (Lookup(va, oldpa))
    {
        //
        // Hit, assuming the mapping remains the same.  If it isn't the same
        // the entry is now at the head of the list.  It will be replaced with
        // the new translation.
        //
        if (oldpa == pa)
        {
            return true;
        }
    }
    else
    {
        //
        // Drop LRU entry
        //
        MoveEntryToHead(tail);
    }

    //
    // Replace head with the new entry unless the head already has a translation
    // from va to something else.  This probably happened because we didn't
    // get the mapping early enough from SoftSDV and made something up ourselves.
    // Too late to change now!
    //
    // For now, we rely on the table getting old and having translation entries
    // replaced to get updated virtual to physical mappings.  We should probably
    // watch for TLB invalidation, instead, if possible.
    //
    if (translationTable[head].virtualPage != Page(va))
    {
        //
        // Allow the new entry
        //
        translationTable[head].virtualPage = Page(va);
        translationTable[head].physicalPage = Page(pa);
        translationTable[head].artificial = false;
        return false;
    }
    else
    {
        return true;
    }
}


//
// AddEntry() for artificial translations -- no SoftSDV Uid
//
bool
SOFTSDV_ITRANSLATE_CLASS::AddEntry(UINT64 va, UINT64 pa)
{
    if (! AddEntry(va, pa, lastSoftsdvUid))
    {
        translationTable[head].artificial = true;
        return false;
    }
    else
    {
        return true;
    }
}


void
SOFTSDV_ITRANSLATE_CLASS::DropTranslation(UINT64 va)
{
    UINT64 page = Page(va);

    TABLE_INDEX i = head;
    while (i != -1)
    {
        if (translationTable[i].virtualPage == page)
        {
            DeleteEntry(i);
            return;
        }

        i = translationTable[i].next;
    }
}


void
SOFTSDV_ITRANSLATE_CLASS::DropOldUserSpaceTranslations(UINT64 softsdvUid)
{
    for (TABLE_INDEX i = 0; i < nTableEntries; i++)
    {
        if (translationTable[i].softsdvUid &&
            (translationTable[i].softsdvUid < softsdvUid) &&
            (translationTable[i].virtualPage < 0xe000000000000000LL))
        {
            DeleteEntry(i);
        }
    }
}
