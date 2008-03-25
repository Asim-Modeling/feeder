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

/**
 * @file 
 * @author Jim Vash
 *
 * @brief Page Table Info Class for ITranslate/DTranslate
 *
 */

#include "page_table_info.h"

PAGE_TABLE_INFO_CLASS::PAGE_TABLE_INFO_CLASS()
{
    Init();
}

PAGE_TABLE_INFO_CLASS::~PAGE_TABLE_INFO_CLASS()
{
}

void
PAGE_TABLE_INFO_CLASS::Init()
{
    isVirtual = false;
    startLevel = PTI_MAX_LEVELS;
    endLevel = 0;
    for (UINT32 level = 0; level < PTI_MAX_LEVELS; level++)
    {
        valid[level] = false;
        addrs[level] = 0;
    }
    isComplete = false;
    pageSize = 0;
    entrySize = 0;
    pde_data = 0;
    pte_data = 0;
}

void
PAGE_TABLE_INFO_CLASS::SetVirtual(bool is_virtual)
{
    isVirtual = is_virtual;
}

bool
PAGE_TABLE_INFO_CLASS::GetVirtual() const
{
    return isVirtual;
}

void
PAGE_TABLE_INFO_CLASS::SetStartLevel(UINT32 start_level)
{
    ASSERTX(start_level <= PTI_MAX_LEVELS);
    startLevel = start_level;
}

UINT32
PAGE_TABLE_INFO_CLASS::GetStartLevel() const
{
    return startLevel;
}

void
PAGE_TABLE_INFO_CLASS::SetEndLevel(UINT32 end_level)
{
    ASSERTX(end_level < PTI_MAX_LEVELS);
    endLevel = end_level;
}

UINT32
PAGE_TABLE_INFO_CLASS::GetEndLevel() const
{
    return endLevel;
}

void
PAGE_TABLE_INFO_CLASS::SetLevelAddr(UINT32 level, UINT64 addr)
{
    ASSERTX(level < PTI_MAX_LEVELS);
    valid[level] = true;
    addrs[level] = addr;
}

UINT64
PAGE_TABLE_INFO_CLASS::GetLevelAddr(UINT32 level) const
{
    ASSERTX(level >= startLevel);
    ASSERTX(level <= endLevel);
    ASSERTX(valid[level]);
    return addrs[level];
}

void
PAGE_TABLE_INFO_CLASS::SetComplete(bool is_complete)
{
    isComplete = is_complete;
}

bool
PAGE_TABLE_INFO_CLASS::GetComplete() const
{
    return isComplete;
}

void
PAGE_TABLE_INFO_CLASS::SetPageSize(UINT32 page_size)
{
    pageSize = page_size;
}

UINT32
PAGE_TABLE_INFO_CLASS::GetPageSize() const
{
    ASSERTX(pageSize != 0);
    return pageSize;
}

void
PAGE_TABLE_INFO_CLASS::SetEntrySize(UINT32 entry_size)
{
    entrySize = entry_size;
}

UINT32
PAGE_TABLE_INFO_CLASS::GetEntrySize() const
{
    ASSERTX(entrySize != 0);
    return entrySize;
}

void
PAGE_TABLE_INFO_CLASS::SetPdeData(UINT64 pdedata) 
{
    pde_data = pdedata;
}


UINT64
PAGE_TABLE_INFO_CLASS::GetPdeData() const
{
    return pde_data;
}

void
PAGE_TABLE_INFO_CLASS::SetPteData(UINT64 ptedata) 
{
    pte_data = ptedata;
}


UINT64
PAGE_TABLE_INFO_CLASS::GetPteData() const
{
    return pte_data;
}
