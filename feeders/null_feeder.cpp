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
 * @author Sailashri Parthasarathy (based on Chris Weaver's original implementation)
 *
 * @brief A Null feeder for ASIM
 *
 */

#include "asim/provides/instfeeder_implementation.h"

NULL_FEEDER_CLASS::NULL_FEEDER_CLASS() //CONS
     : IFEEDER_BASE_CLASS("Null_Feeder", NULL, IFEED_TYPE_MICRO)
{ 
}

IFEEDER_BASE
IFEEDER_New(void)
{
    return new NULL_FEEDER_CLASS();
}


void
IFEEDER_Usage (FILE *file)
{
}

bool
NULL_FEEDER_CLASS::DTranslate(ASIM_INST inst,
                                 const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
                                 UINT64& pa,
                                 PAGE_TABLE_INFO pt_info,
                                 MEMORY_VIRTUAL_REFERENCE_CLASS& vNextRegion)
{
    return false;
}


bool
NULL_FEEDER_CLASS::DTranslate(ASIM_INST inst,
                                 UINT64 va,
                                 UINT64& pa)
{
    return false;
}

void
NULL_FEEDER_CLASS::Done (void)
{
}

