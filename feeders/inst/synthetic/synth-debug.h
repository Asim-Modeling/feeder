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

// synth-debug.h
// Mark Charney   <mark.charney@intel.com>
// $Id: synth-debug.h 799 2006-10-31 12:27:52Z cjbeckma $


#ifndef _SYNTH_DEBUG_H_
# define _SYNTH_DEBUG_H_

#include <ostream>
#include <assert.h>
#include "synth-headers.h"

#define SYNTH_TRACE (TRACEP(Trace_Feeder))
//#define SYNTH_TRACE 1

#define DEBUG_MVM 0
//#define DEBUG_MVM 1

char* string_tail(char* s);

extern UINT64 global_cycle;
////////////////////////////////////////////////////////////////////////////
#define IMSG() string_tail(__FILE__) << ":" << __LINE__ << ": C_"  << global_cycle << ":"

#define SMSG(x) ({ \
   if (SYNTH_TRACE) \
   { \
          std::cout << IMSG() <<  x << endl;  \
   }\
})



#define XMSG(x) ({ \
   if (0) \
   { \
           std::cout << IMSG() <<  x << endl; \
   }\
})

#define BMSG(x) ({ \
   if (0) \
   { \
           std::cout << IMSG() <<  x << endl; \
   }\
})

#define WMSG(x) ({ \
     std::cout << "ERROR: " << IMSG() <<  x << endl; \
})

#define AMSG(x) ({ \
   if (0) \
   { \
           std::cout <<  x << endl; \
   }\
})


#define FIXME() ({ \
   std::cout << "FIXME: " << __FILE__ << ":" << __LINE__ << ": C_" << global_cycle << endl;  \
   assert(0); \
 })

#define MSG(x) ({ \
   std::cout <<  "        " \
        << __FILE__ << ":" << __LINE__ << ": C_" << global_cycle << ":" <<  x << endl; \
 })


/* support for the -d feeder argument or the SYNTH_PRINT=1 option */
extern bool itrace; 


////////////////////////////////////////////////////////////////////////////
// DEBUG SUPPORT 
////////////////////////////////////////////////////////////////////////////
void
print_ip(UINT64 ip);

void
disassemble(ASIM_INST inst);

void
disassemble2(ASIM_INST inst, char* s);

void
debug_disassemble(IADDR_CLASS pc,
                  ASIM_INST   inst);
void
m_debug(const char* s,
        ASIM_INST inst);



#endif
////////////////////////////////////////////////////////////////////////////







//Local Variables:
//pref: "synth-debug.cpp"
//End:
