/*
 * *************************************************************************
 * Copyright (C) 2005-2006 Intel Corporation
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

#ifndef _PAGE_TABLE_INFO_
#define _PAGE_TABLE_INFO_

// ASIM core
#include "asim/mesg.h"
#include "asim/syntax.h"

#define PTI_MAX_LEVELS 4

typedef class PAGE_TABLE_INFO_CLASS* PAGE_TABLE_INFO;

class PAGE_TABLE_INFO_CLASS
{
  private:
    bool isVirtual;
    UINT32 startLevel;
    UINT32 endLevel;
    bool valid[PTI_MAX_LEVELS];
    UINT64 addrs[PTI_MAX_LEVELS];
    bool isComplete;
    UINT32 pageSize;
    UINT32 entrySize;
    UINT64 pde_data;
    UINT64 pte_data;
    UINT32 pageMode;

  public:
    // Constructor
    PAGE_TABLE_INFO_CLASS();

    // Destructor
    ~PAGE_TABLE_INFO_CLASS();
   
    // initialize members
    void Init();

    // are page table addresses virtual or physical?
    void SetVirtual(bool is_virtual);
    bool GetVirtual() const;

    // what is the starting level?
    void SetStartLevel(UINT32 start_level);
    UINT32 GetStartLevel() const;

    // what is the ending level?
    void SetEndLevel(UINT32 end_level);
    UINT32 GetEndLevel() const;

    // what is the address for a particular level?
    void SetLevelAddr(UINT32 level, UINT64 addr);
    UINT64 GetLevelAddr(UINT32 level) const;

    // did the walk complete?
    void SetComplete(bool is_complete);
    bool GetComplete() const;

    // what is the page size (log2 bytes)?
    void SetPageSize(UINT32 page_size);
    UINT32 GetPageSize() const;

    // what is the entry size (bytes)?
    void SetEntrySize(UINT32 entry_size);
    UINT32 GetEntrySize() const;

    // what is the pde data?
    void SetPdeData(UINT64 pdedata);
    UINT64 GetPdeData() const;
    
    // what is the pte data?
    void SetPteData(UINT64 ptedata);
    UINT64 GetPteData() const;

    // ISA-specific mode, also specific to a reference model (ie. archlib)
    // these modes start at 0 and -1 is invalid.
    void SetPageMode(UINT32 mode);
    UINT32 GetPageMode() const;

};

#endif /* _PAGE_TABLE_INFO_ */
