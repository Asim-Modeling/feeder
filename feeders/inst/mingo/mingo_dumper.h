/*****************************************************************************
 *
 * @brief Header file for Mingo dumper system driver
 *
 * @author Michael Adler
 *
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
 *****************************************************************************/

// Container for Common System functions

#ifndef _MINGO_DUMPER_
#define _MINGO_DUMPER_

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/provides/basesystem.h"
#include "asim/state.h"
#include "asim/config.h"
#include "asim/stateout.h"

//include local modules
#include "deptype.h"
#include "deptable.h"

void RegisterSimulatorConfiguration(ASIM_REGISTRY reg);

typedef class asim_mingo_dumper_system_class : public ASIM_SYSTEM_CLASS 
{
  protected: 
    // Simulator configuration
    ASIM_CONFIG config;
    VA_TABLE address_table;

  public:
    asim_mingo_dumper_system_class(const char *n, const UINT32 nSlices);

    virtual ~asim_mingo_dumper_system_class ();

    friend ASIM_SYSTEM SYS_Init(UINT32 argc, char *argv[]);

    virtual void DumpStats (STATE_OUT stateOut);

    bool InitModule(void);

    // Interface functions. The following functions represent
    // a performance models external interface.
    friend void SYS_Usage(FILE * file);
    bool SYS_ScheduleThread(ASIM_THREAD thread);
    bool SYS_UnscheduleThread(ASIM_THREAD thread);
    bool SYS_BlockThread(ASIM_THREAD thread, ASIM_INST inst);
    bool SYS_UnblockThread(ASIM_THREAD thread);
    bool SYS_HookAllThreads();
    bool SYS_UnhookAllThreads();
    bool SYS_Execute(UINT64 stopCycle, UINT64 stopInst, UINT64 stopPacket=0);

    void SYS_Break(void)      { }
    void SYS_ClearBreak(void) { }

} ASIM_MINGO_DUMPER_SYSTEM_CLASS, *ASIM_MINGO_DUMPER_SYSTEM;


#endif /* _MINGO_DUMPER_ */ 
