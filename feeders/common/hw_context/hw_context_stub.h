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
 */

/**
 * @file
 * @author Pritpal Ahuja
 * @brief HW_CONTEXT_CLASS
 *
 */

#ifndef _HW_CONTEXT_
#define _HW_CONTEXT_

#include "asim/provides/warmup_manager.h"

/**
 * A dummy HW_CONTEXT_CLASS.
 */


typedef class HW_CONTEXT_CLASS * HW_CONTEXT;

class HW_CONTEXT_CLASS
{
  public:

    HW_CONTEXT_CLASS();

    UINT32 GetUID (void) const { return 0; }
    bool WarmUp(WARMUP_INFO warmup) { return false; }
    void WarmUpClientInfo(const WARMUP_CLIENTS clientInfo) { }
    UINT32 GetCacheOwnerId() const { return 0; }

};


#endif /* _HW_CONTEXT_ */
