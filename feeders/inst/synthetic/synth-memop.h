/*
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
 */

/* synth-memop.h */
/* Mark Charney   <mark.charney@intel.com> */
/*$Id: synth-memop.h 799 2006-10-31 12:27:52Z cjbeckma $ */


#ifndef _SYNTH_MEMOP_H_
# define _SYNTH_MEMOP_H_

#include "synth-types.h"

class SYNTH_MEMOP : public REG_DEST_DIST_CLASS
{
  public:
    SYNTH_MEMOP() // CONS
        : REG_DEST_DIST_CLASS(),
          ea(0),
          sz(0),
          hint(IPF_HINT_NONE),
          acquire(false),
          release(false),
          etype(SYNTH_ETYPE_INVALID)
    {
        // nada
    }
    UINT64      ea;
    UINT8       sz; // 0=8bits, 1=16bits, 2=32bits, 3=64bits
    IPF_HINT_ENUM hint; // locality hint in ipf_raw_inst.h
    bool        acquire;
    bool        release;
    SYNTH_ETYPE etype;
};


#endif

