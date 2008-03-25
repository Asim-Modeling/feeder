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
 * @file lockstep_x86.cpp
 * @author Michael Adler
 * @brief x86 lock-step debugging tool -- compare 2 SoftSDV processes
 */

void asim_error_exit(int s);

#define ASSERT(condition,mesg) \
    if (! (condition)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, mesg); \
        asim_error_exit(1); \
    }

#define ASSERTX(condition) \
    if (! (condition)) { \
        fprintf(stderr, "Assert failure in %s:%d\n", __FILE__, __LINE__); \
        asim_error_exit(1); \
    }

#define VERIFY(condition,mesg) ASSERT(condition,mesg)

#define ASIMERROR(mesg) \
    fprintf(stderr, "%s:%d %s\n", __FILE__, __LINE__, mesg); \
    asim_error_exit(1);


//
// ASIM include files
//
#include "asim/provides/softsdv_stub.h"
#include "asim/provides/softsdv_stub_isa.h"
#include "asim/provides/softsdv_cpuapi.h"



ASIM_LOCKSTEP_X86_CLASS::ASIM_LOCKSTEP_X86_CLASS(
    UINT32 interval,
    UINT32 nHistoryInstrs)
    : historyLast(0),
      interval(interval),
      nHistoryInstrs(nHistoryInstrs),
      instCount(0),
      nInstrsDumped(0),
      streamsSame(true)
{
    history = new ASIM_LOCKSTEP_X86_HISTORY_CLASS[nHistoryInstrs];
}


ASIM_LOCKSTEP_X86_CLASS::~ASIM_LOCKSTEP_X86_CLASS()
{
    delete[] history;
}


bool
ASIM_LOCKSTEP_X86_CLASS::NewInstr(
    UINT32 cpuNum,
    ASIM_CPUAPI_INST_INFO_CLASS &instInfo)
{
    if (! streamsSame)
    {
        if (nInstrsDumped < 1024)
        {
            fprintf(stdout, "%3d  ", cpuNum);
            instrDecoder.DisassembleInstr(stdout, instInfo);
            instInfo.DumpMemoryRefs(stdout);
            fprintf(stdout, "      PA 0x%016llx  icount %lld\n\n",
                    instInfo.GetPA(), instInfo.GetIcount());
            fflush(stdout);
            nInstrsDumped += 1;
        }
        return false;
    }

    //
    // Record the instruction for history
    //
    historyLast += 1;
    if (historyLast == nHistoryInstrs)
    {
        historyLast = 0;
    }
    history[historyLast].SetInstr(cpuNum, instInfo);

    //
    // Hash some properties of the instruction
    //
    io.NoteInstrData(&cpuNum, sizeof(cpuNum));
    cpuapi_inst_t icount = instInfo.GetIcount();
    io.NoteInstrData(&icount, sizeof(icount));
    cpuapi_phys_addr_t pa = instInfo.GetPA();
    io.NoteInstrData(&pa, sizeof(pa));
    cpuapi_virt_addr_t va = instInfo.GetVA();
    io.NoteInstrData(&va, sizeof(va));
    io.NoteInstrData(instInfo.GetInstBytes(), instInfo.GetNInstBytes());

    //
    // Hash memory reference info
    //
    cpuapi_inst_memory_access_t *access = instInfo.GetArchInfo()->access_list;
    while (access != NULL)
    {
        io.NoteInstrData(&(access->access), sizeof(access->access));
        io.NoteInstrData(&(access->origin), sizeof(access->origin));
        io.NoteInstrData(&(access->virt_mem_addr), sizeof(access->virt_mem_addr));
        io.NoteInstrData(&(access->phy_mem_addr), sizeof(access->phy_mem_addr));
        io.NoteInstrData(access->data, access->size);
        access = access->next;
    }

    instCount += 1;
    if (instCount >= interval)
    {
        streamsSame = io.CompareProcesses();
        instCount = 0;
    }

    if (! streamsSame)
    {
        //
        // Streams just diverged.  Dump history.
        //
        UINT32 i = historyLast + 1;
        if (i == nHistoryInstrs)
        {
            i = 0;
        }
        while (i != historyLast)
        {

            history[i].Print(stdout);

            i += 1;
            if (i == nHistoryInstrs)
            {
                i = 0;
            }
        }
    }

    return streamsSame;
}

        
bool
ASIM_LOCKSTEP_X86_CLASS::DMA(
    UINT64 addr,
    UINT64 size)
{
    if (! streamsSame)
    {
        if (nInstrsDumped < 1024)
        {
            fprintf(stdout, "   DMA:  0x%016llx - 0x%016llx\n\n",
                    addr, addr + size - 1);
            fflush(stdout);
        }
        return false;
    }

    historyLast += 1;
    if (historyLast == nHistoryInstrs)
    {
        historyLast = 0;
    }
    history[historyLast].SetDMA(addr, size);

    io.NoteInstrData(&addr, sizeof(addr));
    io.NoteInstrData(&size, sizeof(size));

    return true;
}

        
//------------------------------------------------------------------------
//
// Lockstep history class tracks the instructions that have been executed.
// When a difference is hit the history buffer is dumped.
// 
//------------------------------------------------------------------------

void
ASIM_LOCKSTEP_X86_HISTORY_CLASS::SetInstr(
    UINT32 cpu,
    ASIM_CPUAPI_INST_INFO_CLASS &instInfo)
{
    kind = INSTR_ENTRY;
    va = instInfo.GetVA();
    pa = instInfo.GetPA();
    iCount = instInfo.GetIcount();
    memcpy(instr, instInfo.GetInstBytes(), instInfo.GetNInstBytes());
    instrLen = instInfo.GetNInstBytes();
    mode = instInfo.GetCpuMode();
    cpuNum = cpu;

    cpuapi_inst_memory_access_t *a = instInfo.GetArchInfo()->access_list;
    UINT32 i = 0;
    while ((a != NULL) && (i < MAX_DATA_REFS))
    {
        memcpy(&access[i], a, sizeof(cpuapi_inst_memory_access_t));
        UINT32 size = a->size;
        if (size > sizeof(memValue[0]))
        {
            size = sizeof(memValue[0]);
        }
        if (a->data != NULL)
        {
            memcpy(&memValue[i], a->data, size);
        }
        else
        {
            memValue[i] = 0;
        }

        a = a->next;
        i += 1;
    }
    nAccesses = i;
}


void
ASIM_LOCKSTEP_X86_HISTORY_CLASS::SetDMA(
    UINT64 addr,
    UINT64 size)
{
    kind = DMA_ENTRY;
    dmaAddr = addr;
    dmaSize = size;
}


void
ASIM_LOCKSTEP_X86_HISTORY_CLASS::Print(FILE *f)
{
    if (kind == DMA_ENTRY)
    {
        fprintf(stdout, "   DMA:  0x%016llx - 0x%016llx\n\n",
                dmaAddr, dmaAddr + dmaSize - 1);
    }

    if (kind == INSTR_ENTRY)
    {
        fprintf(stdout, "%3d  ", cpuNum);
        instrDecoder.DisassembleInstr(stdout, va, instr, instrLen, mode);
        fprintf(stdout, "\n");
        for (UINT32 i = 0; i < nAccesses; i++)
        {
            DumpMemoryRef(stdout, &access[i], &memValue[i]);
        }
        fprintf(stdout, "      PA 0x%016llx  icount %lld\n\n", pa, iCount);
    }
}
