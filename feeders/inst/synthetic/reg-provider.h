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

/* reg-provider.h */
/* Mark Charney   <mark.charney@intel.com> */
/*$Id: reg-provider.h 799 2006-10-31 12:27:52Z cjbeckma $ */


#ifndef _REG_PROVIDER_H_
# define _REG_PROVIDER_H_

#include "asim/syntax.h"

class REGISTER_PROVIDER_CLASS
{
    // track registers to hand out next... to avoid register hazards

    UINT32 low_reg;
    UINT32 high_reg;
    UINT32 next_reg;
  public:
    REGISTER_PROVIDER_CLASS(UINT32 arg_low_reg, UINT32 arg_high_reg)
        : low_reg(arg_low_reg),
          high_reg(arg_high_reg),
          next_reg(arg_low_reg)
    { /* nada */ }
    
    inline UINT32
    get_next_reg(void)
    {
        UINT32 t = next_reg;
        next_reg++;
        if (next_reg > high_reg)
        {
            next_reg=low_reg;
        }
        return t;
    }
}; /* class REGISTER_PROVIDER_CLASS */


#endif





