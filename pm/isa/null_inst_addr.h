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
 * @file null_inst_addr.h
 * @author Sailashri Parthasarathy (based on Chris Weaver and Mark Charney's original implementation)
 *
 */


#ifndef _IADDR_
#define _IADDR_

// generic
#include <ostream>

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/item.h"

typedef class IADDR_CLASS* IADDR;

class IADDR_CLASS : public ASIM_SILENT_ITEM_CLASS
{
  public: 
    IADDR_CLASS(const UINT64 arg_addr,
                const UINT32 arg_micro_address,
                const UINT32 arg_length = 0,
                const bool arg_start_of_flow = 0,
                const bool arg_end_of_flow = 0) { }; //CONS

    IADDR_CLASS() { }; //CONS

    IADDR_CLASS( const UINT64 arg_addr ) { }; //CONS

    ~IADDR_CLASS() { };
};

// output operator
std::ostream & operator << (std::ostream & os, const IADDR_CLASS & ia);

#endif //_IADDR_
