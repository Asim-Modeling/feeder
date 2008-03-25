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
 * @file
 * @author Michael Adler
 * @brief SoftSDV stub for talking to Asim
 */

#ifndef _SOFTSDV_FEEDER_H
#define _SOFTSDV_FEEDER_H

#include <string>

// ASIM public modules
#include "asim/dynamic_array.h"
#include "asim/provides/softsdv_stub.h"
#include "asim/provides/addrtranslator.h"

// ASIM private modules
#include "asim/restricted/softsdv_replay.h"
#include "asim/restricted/softsdv_itranslate.h"

typedef class SOFTSDV_FEEDER_BASE_CLASS *SOFTSDV_FEEDER_BASE;

class SOFTSDV_FEEDER_BASE_CLASS : public IFEEDER_BASE_CLASS
{
  protected:
    // consts
    static const UINT32 traceWrongPathId = UINT32_MAX;
    static const UINT32 traceArtificialInstId = UINT32_MAX - 1;

    // System events (interrupt, DMA, etc.)
    typedef class SDV_SYSTEM_EVENT_CLASS *SDV_SYSTEM_EVENT;

    class SDV_SYSTEM_EVENT_CLASS
    {
      public:
        SDV_SYSTEM_EVENT_CLASS()
            : macroUid(0)
        {};

        SDV_SYSTEM_EVENT_CLASS(
            UINT64 macroUid,
            FEEDER_SYSTEM_EVENT_CLASS event)
            : macroUid(macroUid),
              event(event)
        {};

        FEEDER_SYSTEM_EVENT_TYPES GetEventType(void) const { return event.GetType(); };
        FEEDER_SYSTEM_EVENT_CLASS GetEvent(void) const {return event; };
        UINT64 GetMacroUid(void) const {return macroUid; };

      private:
        UINT64 macroUid;
        FEEDER_SYSTEM_EVENT_CLASS event;
    };

    class CPU_STATE_CLASS
    {
      public:
        CPU_STATE_CLASS() :
            lastFetchCycle(0),
            lastCommitCycle(0),
            lastSpinCycle(0),
            lastWarmup(0),
            hwcNum(-1),
            onBadPath(false),
            inTimerInterrupt(false),
            sysEventIsPending(false),
            inKernel(false)
        {};

        ~CPU_STATE_CLASS() {};

        bool OnBadPath(void) const { return onBadPath; };
        void SetOnBadPath(void) { onBadPath = true; };
        void ClearOnBadPath(void) { onBadPath = false; };

        bool InTimerInterrupt(void) const { return inTimerInterrupt; };
        void SetInTimerInterrupt(void) { inTimerInterrupt = true; };
        void ClearInTimerInterrupt(void) { inTimerInterrupt = false; };

        bool InKernel(void) const { return inKernel; };
        void SetInKernel(void) { inKernel = true; };
        void ClearInKernel(void) { inKernel = false; };

        //
        // SoftSDV doesn't want Asim scheduling threads.  It confirms that
        // the hardware context for a given SoftSDV CPU never changes.
        //
        bool CheckHWCNum(INT32 newHwcNum)
        {
            if (hwcNum == -1)
            {
                hwcNum = newHwcNum;
            }
            return hwcNum == newHwcNum;
        };

        INT32 HWCNum(void) const { return hwcNum; };

        //
        // Classes for managing instructions and instruction virtual to
        // physical translation.
        //
        SOFTSDV_REPLAY_BUFFER_CLASS liveInstrs;
        SOFTSDV_ITRANSLATE_CLASS iTranslate;

        //
        // System events are handled in order, following an instruction's
        // commit.  This class is structured so that there could be a list
        // of events pending.  For now only IRQ is implemented and only
        // one IRQ can be pending, since the flow following an IRQ will be
        // killed and restarted.
        //
        const SDV_SYSTEM_EVENT_CLASS *GetNextSystemEvent(void) const
        {
            return sysEventIsPending ? &pendingSystemEvent : NULL;
        };

        void PushSystemEvent(SDV_SYSTEM_EVENT_CLASS event) 
        {
            // Ignore all but the first event since the only event supported
            // now is IRQ and the flow will be killed and restarted.
            if (! sysEventIsPending)
            {
                sysEventIsPending = true;
                pendingSystemEvent = event;
            }
        };

        void PopSystemEvent(void) { sysEventIsPending = false; };

      public:
        // Simple variables that are visible
        UINT64 lastFetchCycle;
        UINT64 lastCommitCycle;
        UINT64 lastSpinCycle;
        SOFTSDV_LIVE_INSTRUCTION lastWarmup;

      private:
        // Unhandled pending interrupt request
        SDV_SYSTEM_EVENT_CLASS pendingSystemEvent;

        INT32 hwcNum;
        bool onBadPath;
        bool inTimerInterrupt;
        bool sysEventIsPending;
        bool inKernel;
    };

    DYNAMIC_ARRAY_CLASS<CPU_STATE_CLASS> cpuState;
        

    //
    // Data structure for holding counters for tagged instructions.  These
    // may be workload dependent trip counts through key loops.
    //
    class TAGGED_INSTR_COUNTERS
    {
      public:
        enum
        {
            N_BUCKETS = 128
        };

        TAGGED_INSTR_COUNTERS()
        {
            for (int i = 0; i < N_BUCKETS; i++)
            {
                hits[i] = 0;
            }
        };

        void NoteTag(INT32 tag)
        {
            if ((0 <= tag) && (tag < N_BUCKETS))
            {
                hits[tag] += 1;
            }
        };

        UINT64 Hits(INT32 tag)
        {
            ASSERTX((0 <= tag) && (tag < N_BUCKETS));
            return hits[tag];
        };

        void DumpStats(STATE_OUT state_out,
                       const char *name,
                       const char *description)
        {
            state_out->AddVector<UINT64*>("uint", name, description,
                                          &hits[0], &hits[N_BUCKETS]);
        }

      private:
        UINT64 hits[N_BUCKETS];
    };


    //
    // CPU specific statistics
    //

    class CPU_STATISTICS_CLASS
    {
      public:
        CPU_STATISTICS_CLASS() :
            instrsFromSoftsdv(0),
            nFetched(0),
            nIssued(0),
            nBadPath(0),
            nNOPsForContextSwitch(0),
            nNOPsAtBenchmarkEnd(0),
            nJMPsInjected(0),
            nRegValueMessages(0),
            nInterrupts(0),
            nSysEvents(0),
            nKilled(0),
            nKilledInstrs(0),
            nCommitted(0),
            nImplicitCommitted(0),
            nArtificialCommitted(0),
            nDTranslateUnavailable(0),
            nDTranslateMultipage(0),
            nKernelInstrs(0),
            nContextSwitches(0),
            nTimerInterrupts(0),
            nIdlePauseInstrs(0),
            nSpinLoopTrips(0),
            nSuccessfulMemReads(0),
            nFailedMemReads(0)
        {};

        UINT64 instrsFromSoftsdv;
        UINT64 nFetched;
        UINT64 nIssued;
        UINT64 nBadPath;
        UINT64 nNOPsForContextSwitch;
        UINT64 nNOPsAtBenchmarkEnd;
        UINT64 nJMPsInjected;
        UINT64 nRegValueMessages;
        UINT64 nInterrupts;
        UINT64 nSysEvents;
        UINT64 nKilled;
        UINT64 nKilledInstrs;
        UINT64 nCommitted;
        UINT64 nImplicitCommitted;
        UINT64 nArtificialCommitted;
        UINT64 nDTranslateUnavailable;
        UINT64 nDTranslateMultipage;
        UINT64 nKernelInstrs;
        UINT64 nContextSwitches;
        UINT64 nTimerInterrupts;
        UINT64 nIdlePauseInstrs;
        TAGGED_INSTR_COUNTERS taggedInstrs;
        UINT64 nSpinLoopTrips;
        UINT64 nSuccessfulMemReads;
        UINT64 nFailedMemReads;
    };

    DYNAMIC_ARRAY_CLASS<CPU_STATISTICS_CLASS> cpuStats;

    //
    // Global statistics
    //

    class GLOBAL_STATISTICS_CLASS
    {
      public:
        GLOBAL_STATISTICS_CLASS() :
            nDTranslateExpectedVA(0),
            nDTranslateUnexpectedVA(0),
            nITranslateHits(0),
            nITranslateMisses(0),
            nITranslateMissPages(0),
            nITranslateEntriesAdded(0),
            nITranslateArtificialPageInstrs(0),
            sumITranslateSteps(0),
            maxITranslateSteps(0),
            nUnknownRegisterValues(0)
        {};

        UINT64 nDTranslateExpectedVA;
        UINT64 nDTranslateUnexpectedVA;
        UINT64 nITranslateHits;
        UINT64 nITranslateMisses;
        UINT64 nITranslateMissPages;
        UINT64 nITranslateEntriesAdded;
        UINT64 nITranslateArtificialPageInstrs;
        UINT64 sumITranslateSteps;
        UINT64 maxITranslateSteps;
        UINT64 nUnknownRegisterValues;
        TAGGED_INSTR_COUNTERS taggedInstrs;
        string workloadGroup;
        string workloadName;
    };

    GLOBAL_STATISTICS_CLASS globalStats;

  public:
    SOFTSDV_FEEDER_BASE_CLASS(IFEEDER_BASE parentFeeder = NULL);
    virtual ~SOFTSDV_FEEDER_BASE_CLASS() {};

    void Done(void);

    void ForceThreadExit(IFEEDER_STREAM_HANDLE stream);

    void WarmUpClientInfo(const WARMUP_CLIENTS clientInfo);
    bool WarmUp(IFEEDER_STREAM_HANDLE stream, WARMUP_INFO warmup);

    bool FetchMacro(
        IFEEDER_STREAM_HANDLE stream,
        IADDR_CLASS pc,
        ASIM_MACRO_INST mInst,
        UINT64 cycle);

    bool FetchMicro(
        IFEEDER_STREAM_HANDLE stream,
        IADDR_CLASS pc,
        ASIM_MACRO_INST macroInst,
        ASIM_INST inst,
        UINT64 cycle)
    {
        return true;
    };

    bool Fetch(
        IFEEDER_STREAM_HANDLE stream,
        IADDR_CLASS pc,
        ASIM_INST inst,
        UINT64 cycle);

    void Issue(ASIM_INST inst);
    void Commit(ASIM_INST inst);
    void Kill(ASIM_INST inst, bool fetchNext, bool killMe);

    bool ITranslate(IFEEDER_STREAM_HANDLE stream, UINT32 hwcNum, UINT64 va, UINT64& pa);
    bool ITranslate(IFEEDER_STREAM_HANDLE stream,
                    const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
                    UINT64& pa,
                    PAGE_TABLE_INFO pt_info,
                    MEMORY_VIRTUAL_REFERENCE_CLASS& vNextRegion);
    bool DTranslate(ASIM_INST inst, UINT64 va, UINT64& pa);
    bool DTranslate(ASIM_INST inst,
                    const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
                    UINT64& pa,
                    PAGE_TABLE_INFO pt_info,
                    MEMORY_VIRTUAL_REFERENCE_CLASS& vNextRegion);

    void DumpStats(STATE_OUT state_out);

    //
    // System events.
    //
    FEEDER_SYSTEM_EVENT_CLASS
    HandleSystemEvent(ASIM_INST inst, FEEDER_SYSTEM_EVENT_TYPES type);

    FEEDER_SYSTEM_EVENT_TYPES GetNextSystemEventType(ASIM_INST inst);

    //
    // Input and output register values are ISA independent on the Asim side.
    // Predicates are ISA dependent and defined in the ISA specific parent
    // class of this class.
    //
    // Register numbering is logical (virtual), not physical.
    //
    bool GetInputRegisterValue(
        ASIM_INST inst,
        ARCH_REGISTER_TYPE rType,
        INT32 regNum,
        ARCH_REGISTER reg);

    bool GetOutputRegisterValue(
        ASIM_INST inst,
        ARCH_REGISTER_TYPE rType,
        INT32 regNum,
        ARCH_REGISTER reg);

    bool GetInputRegVal(ASIM_INST inst, UINT32 slot, ARCH_REGISTER reg);
    bool GetOutputRegVal(ASIM_INST inst, UINT32 slot, ARCH_REGISTER reg);

    //
    // Memory values
    //
    bool ReadMemory(ASIM_INST inst, void *buffer, UINT32 size, UINT64 pAddr);

    bool ReadIOPort(ASIM_INST inst, void *buffer, UINT32 size, UINT32 port);

  protected:
    //*****************************************************************
    //  ISA independent methods required by ISA dependent part
    //  of the feeder.
    //*****************************************************************

    bool Init(UINT32 argc, char **argv, char **envp);

    UINT32 FetchIncomingInstructions(
        UINT32 cpu,
        UINT32 fetchChunkSize,
        UINT64 cycle);

    //*****************************************************************
    //  ISA specific methods required by ISA independent part
    //  of the feeder.
    //*****************************************************************

    //
    // ISA-specific initialization
    //
    virtual void
    InitISASpecific(UINT32 argc, char **argv, char **envp) {};

    //
    // Fill in warm-up data for memory references
    //
    virtual void
    NoteMemoryRefForWarmup(
        UINT32 cpuNum,
        SOFTSDV_LIVE_INSTRUCTION instrHandle,
        WARMUP_INFO warmup) = 0;

    //
    // Predicate values are ISA specific
    //
    virtual bool
    GetInputPredicateValue(
        UINT32 cpuNum,
        SOFTSDV_LIVE_INSTRUCTION instrHandle,
        INT32 regNum,
        ARCH_REGISTER reg)
    {
        *reg = ARCH_REGISTER_CLASS(REG_TYPE_PREDICATE, regNum);
        return false;
    };

    virtual bool
    GetOutputPredicateValue(
        UINT32 cpuNum,
        SOFTSDV_LIVE_INSTRUCTION instrHandle,
        INT32 regNum,
        ARCH_REGISTER reg)
    {
        *reg = ARCH_REGISTER_CLASS(REG_TYPE_PREDICATE, regNum);
        return false;
    };


    //
    // ISA specific feeder class must define a routine to convert from
    // the instructions received from SoftSDV to an ASIM_INST.  WarmUp
    // is true if feeder is in warm-up mode.
    //
    virtual bool
    AsimInstFromSoftsdvInst(
        UINT32 cpuNum,
        SOFTSDV_LIVE_INSTRUCTION instrHandle,
        ASIM_MACRO_INST asimInstr,
        bool warmUp,
        UINT64 cycle) = 0;

    //
    // Instruction just arrived from SoftSDV and has been stored in the
    // live instruction (replay) buffer.  For some ISAs, another instruction
    // must be added to the buffer (e.g. extended immediate on IPF).  The
    // handle of the first instruction added to the replay buffer must be
    // returned.
    //
    virtual SOFTSDV_LIVE_INSTRUCTION
    NoteNewIncomingInstr(
        UINT32 cpuNum,
        ASIM_SOFTSDV_INST_INFO sdvInstr)
    {
        return 0;
    };

    //
    // Inject a NOP in the instruction stream.
    //
    virtual void
    InjectNOP(
        UINT32 cpuNum,
        const IADDR_CLASS ipVA,
        const IADDR_CLASS ipPA,
        ASIM_MACRO_INST asimInstr) = 0;

    //
    // Inject a bad path instruction in the stream.
    //
    virtual void
    InjectBadPathInstr(
        UINT32 cpuNum,
        const IADDR_CLASS ipVA,
        const IADDR_CLASS ipPA,
        ASIM_MACRO_INST asimInstr) = 0;

    //
    // Inject a JMP in the instruction stream.
    //
    virtual void
    InjectJMP(
        UINT32 cpuNum,
        const IADDR_CLASS ipVA,
        const IADDR_CLASS ipPA,
        SOFTSDV_LIVE_INSTRUCTION targetHandle,
        ASIM_MACRO_INST asimInstr) = 0;

    //
    // ISA specific statistics
    //
    virtual void
    DumpISAGlobalStats(STATE_OUT state_out)
    {};

    virtual void
    DumpISAPerCPUStats(STATE_OUT state_out, UINT32 cpuNum)
    {};

    // Shared data with SoftSDV
    SOFTSDV_IO_ASIM_SIDE asimIO;

  private:
    //
    // Add an instruction arriving from SoftSDV to the replay buffer.
    //
    SOFTSDV_LIVE_INSTRUCTION
    BufferIncomingInstruction(
        UINT32 cpu,
        ASIM_SOFTSDV_INST_INFO instr,
        UINT64 cycle);

    void CheckForIncomingRegValues(void);

    UINT64 nSpinLoopSamples;
    UINT64 nSpinLoopCycles;

    DYNAMIC_ARRAY_CLASS<IFEEDER_THREAD> threads;

    ADDRESS_TRANSLATOR_CLASS addrTrans;

    // Warm-up requests -- these will be set once the warm-up manager
    // calls WarmUpClientInfo()
    WARMUP_CLIENTS_CLASS warmUpClients;
    bool warmUpClientInfoCalled;

    // Set to true when the first thread exits, forcing all threads to exit
    bool simComplete;

    //
    // STREAM_HANDLE is the internal representation of an IFEEDER_STREAM_HANDLE.
    // STREAM_HANDLE is one less than IFEEDER_STREAM_HANDLE so that the former
    // is 0 based and the latter has NULL meaning all streams.
    //
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
};

#endif // _SOFTSDV_FEEDER_H
