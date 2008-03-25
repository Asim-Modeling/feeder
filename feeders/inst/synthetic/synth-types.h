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

// synth-types.h
// Mark Charney   <mark.charney@intel.com>
// $Id: synth-types.h 799 2006-10-31 12:27:52Z cjbeckma $


#ifndef _SYNTH_TYPES_H_
# define _SYNTH_TYPES_H_

#include <ostream>
#include "asim/syntax.h" /* for UINT32 */

// generic ftype events -- not slot specific or packing-specific
// these can be combined in the various templates
typedef enum
{
    SYNTH_FTYPE_INVALID,
    SYNTH_FTYPE_MEMOP,
    SYNTH_FTYPE_LOAD,
    SYNTH_FTYPE_STORE,
    SYNTH_FTYPE_EXCHANGE,
    SYNTH_FTYPE_COMPARE_EXCHANGE,
    SYNTH_FTYPE_FENCE,
    SYNTH_FTYPE_DEPENDENT_READ, // dependent XOR
    SYNTH_FTYPE_BRANCH,
    SYNTH_FTYPE_LONG_BRANCH,
    SYNTH_FTYPE_LIMM,
    SYNTH_FTYPE_NOP,
    SYNTH_FTYPE_XOR,
    SYNTH_FTYPE_FMA,
    SYNTH_FTYPE_COMPARE,
    SYNTH_FTYPE_FETCH_AND_ADD,
    SYNTH_FTYPE_LFETCH,
    SYNTH_FTYPE_LAST              // Must be last
}
SYNTH_FTYPE;

std::ostream& operator<<(std::ostream& o, const SYNTH_FTYPE& f);

// these are the "major" events that the synthetic generator creates.

typedef enum
{
    SYNTH_ETYPE_INVALID,
    SYNTH_ETYPE_LOAD,
    SYNTH_ETYPE_STORE,
    SYNTH_ETYPE_EXCHANGE,
    SYNTH_ETYPE_COMPARE_EXCHANGE,
    SYNTH_ETYPE_MEMORY_FENCE,
    SYNTH_ETYPE_DEPENDENT_READ,
    SYNTH_ETYPE_BRANCH,
    SYNTH_ETYPE_LONG_BRANCH,
    SYNTH_ETYPE_FETCH_AND_ADD,
    SYNTH_ETYPE_LFETCH,
    SYNTH_ETYPE_LAST              // Must be last
}
SYNTH_ETYPE;


std::ostream& operator<<(std::ostream& o, const SYNTH_ETYPE& e);

// These are codes for the syllables that implement the major and minor
// (filler) events. These are slot-specific and directly converted into
// ASIM_INSTs.

typedef enum {
    SYNTH_IPF_INVALID,
    SYNTH_IPF_NOPM,
    SYNTH_IPF_NOPI,
    SYNTH_IPF_NOPB,
    SYNTH_IPF_NOPF,
    SYNTH_IPF_XOR,
    SYNTH_IPF_XOR_DEPENDENT, /* make the registers dependent on previous op */
    SYNTH_IPF_LOAD,
    SYNTH_IPF_STORE,
    SYNTH_IPF_COMPARE_EXCHANGE,
    SYNTH_IPF_EXCHANGE,
    SYNTH_IPF_FENCE,
    SYNTH_IPF_BRANCH,
    SYNTH_IPF_LONG_BRANCH,
    SYNTH_IPF_LIMM,
    SYNTH_IPF_FMA,
    SYNTH_IPF_COMPARE,
    SYNTH_IPF_FETCH_AND_ADD,
    SYNTH_IPF_LFETCH,
    SYNTH_IPF_LAST                 // Must be last
} SYNTH_IPF_ENUM;


std::ostream& operator<<(std::ostream& o, const SYNTH_IPF_ENUM& f);



class REG_DEST_DIST_CLASS
{
  public:
    REG_DEST_DIST_CLASS() //CONS
        : reg_dest(0), dependence_distance(0)
    {
    }
    
    inline UINT32
    get_reg_dest(void) const
    {
        return reg_dest;
    }

    inline UINT32
    get_dependence_distance(void) const
    {
        return dependence_distance;
    }

    inline void
    set_reg_dest(UINT32 arg_reg_dest)
    {
        reg_dest = arg_reg_dest;
    }

    inline void
    set_dependence_distance(UINT32 arg_dep_dist)
    {
        dependence_distance = arg_dep_dist;
    }

  protected:

    UINT32 reg_dest;
    UINT32 dependence_distance;
};

#endif
////////////////////////////////////////////////////////////////////////////







