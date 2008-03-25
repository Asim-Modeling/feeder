/**************************************************************************
 * Copyright (C) 2006 Intel Corporation
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
 * @file lockstep_x86.h
 * @author Michael Adler
 * @brief x86 lock-step debugging tool -- compare 2 SoftSDV processes
 */

#include "lockstep.h"

class ASIM_LOCKSTEP_X86_HISTORY_CLASS
{
  private:
    enum
    {
        NONE,
        INSTR_ENTRY,
        DMA_ENTRY
    }
    kind;

  public:
    ASIM_LOCKSTEP_X86_HISTORY_CLASS(void) : kind(NONE) {};
    ~ASIM_LOCKSTEP_X86_HISTORY_CLASS() {};

    void SetInstr(UINT32 cpu, ASIM_CPUAPI_INST_INFO_CLASS &instInfo);
    void SetDMA(UINT64 addr, UINT64 size);
    void Reset(void) { kind = NONE; };
    void Print(FILE *f);

  private:
    UINT64 dmaAddr;
    UINT64 dmaSize;

    UINT64 va;
    UINT64 pa;
    UINT64 iCount;
    unsigned char instr[CPUAPI_IA32_INST_BYTES_LENGTH];
    UINT32 cpuNum;
    unsigned int instrLen;
    cpuapi_cpu_mode_t mode;

    // Memory reference data
    enum
    {
        MAX_DATA_REFS = 8
    };
    UINT64 memValue[MAX_DATA_REFS];
    cpuapi_inst_memory_access_t access[MAX_DATA_REFS];
    UINT32 nAccesses;
};

typedef ASIM_LOCKSTEP_X86_HISTORY_CLASS *ASIM_LOCKSTEP_X86_HISTORY;


class ASIM_LOCKSTEP_X86_CLASS
{
  public:
    ASIM_LOCKSTEP_X86_CLASS(UINT32 interval, UINT32 nHistoryInstrs);
    ~ASIM_LOCKSTEP_X86_CLASS();

    bool NewInstr(UINT32 cpuNum, ASIM_CPUAPI_INST_INFO_CLASS &instInfo);
    bool DMA(UINT64 addr, UINT64 size);

  private:
    ASIM_LOCKSTEP_X86_HISTORY history;
    UINT32 historyLast;

    ASIM_LOCKSTEP_CLASS io;
    UINT32 interval;
    UINT32 nHistoryInstrs;
    UINT32 instCount;
    UINT32 nInstrsDumped;
    bool streamsSame;
};

typedef class ASIM_LOCKSTEP_X86_CLASS *ASIM_LOCKSTEP_X86;
