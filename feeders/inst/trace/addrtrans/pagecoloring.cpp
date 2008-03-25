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
 * @author Srilatha Manne
 * @brief Implements page table access methods. 
 *
 */

// ASIM public modules
#include "asim/provides/addrtranslator.h"
#include "asim/provides/software_context.h"

PAGE_TABLE_CLASS::PAGE_TABLE_CLASS(UINT64 max_page,
				   UINT32 page_offset)
{
    SetTraceableName("PAGE_TABLE_CLASS");

    // Start assigning from page 0. 
    nextPage = 0;
    
    offset = page_offset;
    maxPages = max_page;

}

//
// Method searches for existence of va mapping in hash table.  If no
// mapping is available, then it assigns a new page to va.  The method
// assumes that the va has the lowest PAGE_SIZE bits shifted out. 
//
bool
PAGE_TABLE_CLASS::GetPTE(UINT64 va, UINT64& pa)
{

    map<UINT64, UINT64>::iterator pte;
    //set it to the end in case it is not found in the table
    pte = pageTable.end();

    pte = pageTable.find(va);


    if (pte == pageTable.end()) 
    {
        // No page found.  Create a new one. 
        pa = nextPage + offset;
        pageTable[va] = pa;
  
        T1("\tAllocating: VA = " << 
              fmt_x(va) << ", PTE = " << pa);
        
        nextPage++;
        ASSERT ( (nextPage < maxPages), 
                 "Exceeding maximum number of pages for page table.");
        return false;
    }
    else 
    {
        pa = (*pte).second;
        return true;
    }
}


//
// Methods associated with address translator class
//

// 
// Constructor. 
//
ADDRESS_TRANSLATOR_CLASS::ADDRESS_TRANSLATOR_CLASS(
    UINT32 log2PageSize,
    UINT64 physicalBaseAddress)
{
    SetTraceableName("ADDRESS_TRANSLATOR_CLASS");

    PAGE_TABLE pt;
    UINT64 offset;
    UINT64 pte_per_swc;
    
    this->log2PageSize = log2PageSize;
    pageSizeMask = (1 << log2PageSize) - 1;
    baseAddress = physicalBaseAddress;

    ASSERT(baseAddress == (baseAddress & ~pageSizeMask),
           "Address translator base address isn't page aligned");

    ASSERT (PHYSICAL_ADDRESS_BITS <= 64, 
	    "Physical address cannot be greather than 64 bits");
    
    ASSERT (LOG_2_PTE_PER_SWC < (PHYSICAL_ADDRESS_BITS - LOG_2_MAX_SWC), 
	    "LOG_2_PTE_PER_SWC must be less than (PHYSICAL_ADDRESS_BITS - LOG_2_MAX_SWC) bits.");
    pte_per_swc = ((UINT64)(1) << LOG_2_PTE_PER_SWC);
    
    // Create page tables for all SWC.
    for (INT32 i = 0; i < (1 << LOG_2_MAX_SWC); i++) 
    {
	offset = PTE_OFFSET_BETWEEN_SWC * i;
	pt = new PAGE_TABLE_CLASS(pte_per_swc, offset);
	pageTableArray.push_back(pt);
    }
}

//
// Destructor removes all page tables allocated for each SWC. 
//
ADDRESS_TRANSLATOR_CLASS::~ADDRESS_TRANSLATOR_CLASS()
{
    PAGE_TABLE pt;

    for (INT32 i = 0; i < (1 << LOG_2_MAX_SWC); i++) 
    {
        pt = pageTableArray.back();
	pageTableArray.pop_back();
	delete pt;
    }
}

bool
ADDRESS_TRANSLATOR_CLASS::ITranslate(
    const UINT32 hwcNum,
    const UINT64 va, 
    UINT64& pa)
{
    if (SHARE_IADDR_SPACE == 0) 
    {
        return (AddrTranslate(hwcNum, va, pa));
    }
    else 
    {
        return (AddrTranslate(0, va, pa));
    }
}

    
bool
ADDRESS_TRANSLATOR_CLASS::DTranslate(
    ASIM_INST inst,
    const UINT64 va, 
    UINT64& pa)
{
    if (SHARE_DADDR_SPACE == 0) 
    {
        SW_CONTEXT swc = inst->GetSWC();
        ASSERTX(swc != NULL);

        INT32 hwcNum = swc->GetHWCNum();
        ASSERT(hwcNum != -1, "Software context not active.");

        return (AddrTranslate(hwcNum, va, pa));
    }
    else
    {
        return (AddrTranslate(0, va, pa));
    }
}


//
// This method translate a virtual to a physical address.  The page
// table class returns a pte number.  To create a physicall address,
// the pte is modified by adding the low order bits from vea that
// correspond to the address within the page, and adding the swc as 
// the high order bits of the physical address. 
// 
// Data and instruction PTE are allocated next to each other.  
//
bool
ADDRESS_TRANSLATOR_CLASS::AddrTranslate(
    UINT64 swc,
    const UINT64 va, 
    UINT64& pa)
{
    // 
    // BOBBIE: Check with Artur whether it makes sense to call Uid a SWc.
    // He uses Uid as Stream ID in tracefeeder.cpp.
    //
    UINT64 pte;
    UINT64 addr_in_page = va & pageSizeMask;
    bool exists;
    
    T1("\tAddrTranslate T" << swc 
          << ": va " << fmt_x(va));
    
    //
    // Send the virtual address minus the last bits corresponding to 
    // page size. 
    //
    exists = pageTableArray[swc]->GetPTE(va >> log2PageSize, pte);
    
    T1("\tAddrTranslate T" << swc 
          << ": pte " << fmt_x(pte));
    
    // Add the correct low order bits from the vea.  
    pa = ((pte << log2PageSize) | addr_in_page);
    
    // Add the correct high order bits which are a function of the swc. 
    pa = ((swc << (PHYSICAL_ADDRESS_BITS - LOG_2_MAX_SWC)) | pa);
    
    // Add the base address offset
    pa = pa + baseAddress;

    T1("\tAddrTranslate T" << swc 
          << ": pa " << fmt_x(pa));
    
    // return hit or miss information. 
    return (exists);
}
