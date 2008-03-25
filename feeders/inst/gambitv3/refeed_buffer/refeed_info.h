/**************************************************************************
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
 **************************************************************************/

/**
 * @file refeed_info.h
 *
 * @brief This is the information structure that is used to hold the feeder 
 * data for refeed or register access
 *
 * @author Chris Weaver
 */


#ifndef _REFEED_INST_INFO_
#define _REFEED_INST_INFO_

#include "asim/provides/isa.h"
#include "asim/provides/iaddr.h"
#include "asim/provides/softsdv_import_include.h"

/* I am recording these in here because to use asim_inst I 
   would have had to create a copy constructor for rawinst
   and make a Set_Uid function. This can be changed as an
   enhancement later. */

struct instruction_feed_info{
    cpuapi_inst_info_t inst_info;
    UINT64 bundle_bits[2];
    IADDR_CLASS pc;
    IADDR_CLASS npc;
    UINT64 oldcfm; 
    UINT64 cfm;
    UINT64 oldpr;
    UINT64 pr;
    UINT64 Uid;
    bool valid;
    bool issued;
    ARCH_REGISTER_CLASS src_Gp[NUM_SRC_GP_REGS];
    ARCH_REGISTER_CLASS src_Pred[NUM_SRC_PRED_REGS];
    ARCH_REGISTER_CLASS dst[NUM_DST_REGS]; 
    UINT64 oldbsp;
    UINT64 bsp;
    bool nop;
    bool longImm;
};
typedef struct instruction_feed_info* instruction_fi;

#endif
