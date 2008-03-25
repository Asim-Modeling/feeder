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

class SOFTSDV_ITRANSLATE_CLASS
{
  public:
    SOFTSDV_ITRANSLATE_CLASS(void);
    ~SOFTSDV_ITRANSLATE_CLASS() {};

    //
    // Return true if page was already mapped.  False if a new entry had to
    // be added.
    //
    // There are two forms of AddEntry.  The 3 argument version is for entries
    // added from real SoftSDV instructions.  The 2 argument verion is for
    // "artificial" entries created due to lack of data from SoftSDV on the
    // correct mapping.  Namely, Asim requested instruction virtual to physical
    // translation for a virtual address never seen from SoftSDV.
    //
    //
    bool AddEntry(UINT64 va, UINT64 pa, UINT64 softsdvUid);
    bool AddEntry(UINT64 va, UINT64 pa);

    //
    // Return non-zero if page corresponding to va found.  The value returned
    // is the number of steps down the linked list of translations.  It might
    // be useful for statistics.
    //
    INT32 Lookup(UINT64 va, UINT64 &pa);

    //
    // Is the artificial flag set for the entry corresponding to va?  Returns
    // false if no entry is found.
    //
    bool IsEntryArtificial(UINT64 va);

    //
    // Drop translation for a specific VA
    //
    void DropTranslation(UINT64 va);

    //
    // Drop old user space translations.  Most often used on a context switch.
    //
    void DropOldUserSpaceTranslations(UINT64 softsdvUid);

  private:
    //
    // Page size depends on the SoftSDV simulation, not on Asim configuration,
    // since the virtual to physical mapping is happening in SoftSDV.  Until
    // we have a way to ask SoftSDV its page size, assume 16k pages.
    //
    UINT64 Page(UINT64 addr)
    {
        return addr & ~(0x3fffL);
    };
    UINT64 PageOffset(UINT64 addr)
    {
        return addr & 0x3fffL;
    };

    //
    // Allocate a 50 entry translation table.  prev and next are indices
    // into the table instead of pointers, allowing for smaller values.
    //
    enum ItranslateConstants {
        nTableEntries = 50
    };
    
    typedef short TABLE_INDEX;

    struct
    {
        UINT64 softsdvUid;
        UINT64 virtualPage;
        UINT64 physicalPage;
        TABLE_INDEX prev;
        TABLE_INDEX next;
        bool artificial;
    }
    translationTable[nTableEntries];

    UINT64 lastSoftsdvUid;

    TABLE_INDEX head;
    TABLE_INDEX tail;

    //
    // Move an entry in the translation table list to the head.
    //
    void SOFTSDV_ITRANSLATE_CLASS::MoveEntryToHead(TABLE_INDEX i);

    //
    // Drop an entry from the table.
    //
    void DeleteEntry(TABLE_INDEX i);
};

typedef SOFTSDV_ITRANSLATE_CLASS *SOFTSDV_ITRANSLATE;
