/*
 * Copyright (C) 2005-2006 Intel Corporation
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


//
// Author:  Ramon Matas Navarro
//

#ifndef _NULL_FEEDER_H
#define _NULL_FEEDER_H

#include "asim/provides/isa.h"

class NULL_FEEDER_CLASS : public IFEEDER_BASE_CLASS
{
  static const UINT32 traceArtificialInstId = UINT32_MAX - 1;
  IFEEDER_THREAD thread;
  public:
    NULL_FEEDER_CLASS(): IFEEDER_BASE_CLASS(const_cast<char*>("Null Feeder")) {};
    ~NULL_FEEDER_CLASS() {};

    bool Init(UINT32 argc, char **argv, char **envp) {
        UINT32 cpu = 0;
        thread = new IFEEDER_THREAD_CLASS(this, STREAM_HANDLE(cpu), 0);
        return true;
    };
    void Done(void) {};
    bool Fetch(IFEEDER_STREAM_HANDLE stream, IADDR_CLASS pc, ASIM_INST inst) 
    { 
        UINT32 cpu = STREAM_HANDLE(stream);
        inst->CreateNewMacroOp(pc.GetMacro());
        ASIM_MACRO_INST mInst = inst->GetMacroInst();
        T1("Null Feeder: FEED_Fetch: Creating a NOP");
        InjectNOP(cpu, pc, pc, mInst);
        return true; 
    };
    void Commit(ASIM_INST inst) {};
    void Kill(ASIM_INST inst, bool fetchNext, bool killMe) {};
    bool ITranslate(UINT32 hwcNum, UINT64 va, UINT64& pa) { return false; };
    bool DTranslate(ASIM_INST inst, UINT64 va, UINT64& pa) { return false; };
    
    class STREAM_HANDLE
    {
      public:
        STREAM_HANDLE(PTR_SIZED_UINT h) :
            handle(h)
        {};

        STREAM_HANDLE(IFEEDER_STREAM_HANDLE h)
        {
            ASSERTX(h != NULL);
            handle = (PTR_SIZED_UINT) h - 1;
        };

        PTR_SIZED_UINT Handle(void)
        {
            return handle;
        };

        operator PTR_SIZED_UINT()
        {
            return handle;
        };

        operator IFEEDER_STREAM_HANDLE()
        {
            return (IFEEDER_STREAM_HANDLE) (handle + 1);
        };

      private:
        PTR_SIZED_UINT handle;
    };
    void InjectNOP(UINT32 cpuNum,const IADDR_CLASS ipVA,const IADDR_CLASS ipPA,ASIM_MACRO_INST asimInstr)
    {
        static UINT8 nopOpcode[] = { 0x90 };
        asimInstr->Init(nopOpcode, ipVA.GetMacro(), sizeof(nopOpcode));
        asimInstr->SetTraceID(traceArtificialInstId);
        asimInstr->SetDis("NOP");
    }



};

#endif /*_NULL_FEEDER_H*/

