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
 * @brief Page Table implementation for translating virtual to physical addresses
 *
 * Uses a page table to translate virtual address provided by the trace to 
 * physical addresses. 
 *
 */

#ifndef _pagetable_h
#define _pagetable_h

// 

// generic (C++/STL)
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <map>
#include <vector>

// ASIM core
#include "asim/syntax.h"
#include "asim/trace.h"
#include "asim/mesg.h"
//#include "asim/thread.h"


//#include "asim/alphaops.h"
//#include "asim/disasm.h"

#include "asim/provides/iaddr.h"
#include "asim/provides/instfeeder_interface.h"

// ASIM public modules


typedef class PAGE_TABLE_CLASS *PAGE_TABLE;

//
// Basic page table class.  There's one of these for every software context
// in machine. 
//
class PAGE_TABLE_CLASS : public TRACEABLE_CLASS
{
  private:
    map<UINT64, UINT64> pageTable;
    UINT64 nextPage;
    UINT32 offset;
    UINT64 maxPages;
    
  public:
    // Constructor
    PAGE_TABLE_CLASS(UINT64 max, UINT32 offset);
    
    // Destructor
    ~PAGE_TABLE_CLASS();
    
    // Accessors
    
    // This methods returns the pte associated with the given va.  
    bool GetPTE(UINT64 key, UINT64& data);
};


inline
PAGE_TABLE_CLASS::~PAGE_TABLE_CLASS()
{}

typedef class ADDRESS_TRANSLATOR_CLASS *ADDRESS_TRANSLATOR;

class ADDRESS_TRANSLATOR_CLASS : public TRACEABLE_CLASS
{
  private:
    vector<PAGE_TABLE> pageTableArray;
    
  public:
    // Constructor
    //
    // physicalBaseAddress is the start address of the physical address
    // range to be hadded out by the translator.
    //
    ADDRESS_TRANSLATOR_CLASS(
        UINT32 log2PageSize = LOG_2_PAGE_SIZE,
        UINT64 physicalBaseAddress = 0
    );
    
    // Destructor
    ~ADDRESS_TRANSLATOR_CLASS();
    
    // Accessors
    bool ITranslate(const UINT32 hwcNum, const UINT64 va, UINT64& pa);
    bool DTranslate(ASIM_INST inst, const UINT64 va, UINT64& pa);
    
  private:
    bool AddrTranslate(UINT64 swc, UINT64 va, UINT64& pa);

    UINT64 log2PageSize;
    UINT64 pageSizeMask;
    UINT64 baseAddress;
};

#endif // _pagetable_h
