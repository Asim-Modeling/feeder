/*
 * Copyright (C) 2004-2006 Intel Corporation
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

// reg-file.h
// Mark Charney   <mark.charney@intel.com>
// $Id: reg-file.h 799 2006-10-31 12:27:52Z cjbeckma $


#ifndef _REG_FILE_H_
# define _REG_FILE_H_

#include <assert.h>
#include "asim/syntax.h" /* for UINT32, etc. */



class REG_FILE_CLASS
{
  public:
    REG_FILE_CLASS() // CONS
        : NGRS(128), NPRS(64), NFRS(128)
    {
        
    }

    inline void
    write_gr(int reg, UINT64 val)
    {
        assert(reg < NGRS);
        grs[reg] = val;
    }

    inline UINT64
    read_gr(int reg) const
    {
        assert(reg < NGRS);
        return grs[reg]
    }

    inline void
    write_pr(int reg, bool val)
    {
        assert(reg < NPRS);
        prs[reg] = val;
    }

    inline bool
    read_pr(int reg) const
    {
        assert(reg < NPRS);
        return prs[reg]
    }

  private:
    const int NGRS;
    const int NFRS;
    const int NPRS;
    UINT64 grs[NGRS];
    UINT64 grs[NFRS];
    bool   prs[NPRS];
};

////////////////////////////////////////////////////////////////////////////
#endif
////////////////////////////////////////////////////////////////////////////
