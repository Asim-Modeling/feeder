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


#include <string.h>
#include "synth-debug.h"

bool itrace = false; 

/* utility function for shortening file names in messages */
char*
string_tail(char* s)
{
    char* p = strrchr(s,'/')+1;
    if (p)
    {
        return p;
    }
    return s;
}

void
print_ip(UINT64 ip)
{
    SMSG("Emitting at PC = " << hex << ip << dec);
}

void
disassemble(ASIM_INST inst)
{
    cout << inst->GetRawInst()->DisasmString();

    if (inst->HasEffAddress())
    {
        UINT64 g = inst->GetVirtualEffAddress();
        cout << " EA= " << hex << g << dec;
    }
}


void
disassemble2(ASIM_INST inst, char* s)
{
    if (s)
    {
        cout << s;
    }
    UINT32 streamId = ((UINT32)(inst->GetSWC()->GetFeederStreamHandle()))-1; //FIXME
    cout << "tid= " << streamId << " ";
    cout << "PC = " << inst->GetVirtualPC() << "  ";
    cout << "uid = " << inst->GetUid() << "  ";
    cout << inst->GetRawInst()->DisasmString();

    if (inst->HasEffAddress())
    {
        UINT64 g = inst->GetVirtualEffAddress();
        cout << " EA= " << hex << g << dec;
    }
    cout << endl;
}
void
debug_disassemble(IADDR_CLASS pc,
                  ASIM_INST   inst)
{
    if (SYNTH_TRACE || itrace)
    {
        cout << "PC = " << pc << "  ";
        cout << "uid = " << inst->GetUid() << "  ";
        disassemble(inst);
        cout << endl;
    }
}

void
m_debug(const char* s,
        ASIM_INST inst)
{
    if (DEBUG_MVM)
    {
        cout << s << " C_" << global_cycle << ": ";
        disassemble2(inst,0);
        //const UINT64 pc = inst->GetVirtualPC().GetUniqueAddr();
        //debug_disassemble(pc,inst);
        //cout << endl;
    }
}

//Local Variables:
//pref: "synth-debug.h"
//End:
