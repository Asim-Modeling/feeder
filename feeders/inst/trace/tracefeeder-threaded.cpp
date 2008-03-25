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
 
/**
 * @file
 * @author David Goodwin, Steven Wallace, Pritpal Ahuja, Artur Klauser
 * @brief Trace feeder implementation of ASIM instruction feeder.
 */

// generic
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <pthread.h>

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/trace.h"
#include "asim/cmd.h"
#include "asim/alphaops.h"

// ASIM public modules
#include "asim/provides/instfeeder_interface.h"
#include "asim/provides/instfeeder_implementation.h"
#include "asim/provides/isa.h"
#include "asim/provides/traceinstruction.h"
#include "asim/provides/tracereader.h"
#include "asim/provides/traceconverter.h"

// ASIM local module
#include "tracefeeder-threaded.h"

using namespace std;

/*
 * constructor
 */
TRACE_FEEDER_CLASS::TRACE_FEEDER_CLASS():
    IFEEDER_BASE_CLASS("Trace Feeder")
{
    threads = 0;
    totalSkippedWrongpath = 0;
    runOracle = false;
    fetch_lock = new pthread_mutex_t;
    commit_lock = new pthread_mutex_t;
    kill_lock = new pthread_mutex_t;
    pthread_mutex_init(fetch_lock, NULL);
    pthread_mutex_init(commit_lock, NULL);
    pthread_mutex_init(kill_lock, NULL);
    //oracle = NULL;
    SetCapable(IFEED_V2P_TRANSLATION);
    SetCapable(IFEED_FAST_FORWARD);
}

TRACE_FEEDER_CLASS::~TRACE_FEEDER_CLASS()
{
    delete traceReader;
    delete traceConverter;

    // we have new'ed the threads in Init(), so we will delete them here
    for(UINT32 t = 0; t < threads; t++) {
        const IFEEDER_THREAD thread = trace[t].Thread();
        if (thread) {
            delete thread;
        }
    }
    delete [] trace;
    delete fetch_lock;
    delete commit_lock;
    delete kill_lock;
    //if (oracle) {
    //    delete oracle;
    //}
}

bool
TRACE_FEEDER_CLASS::WarmUp(
    IFEEDER_STREAM_HANDLE stream,
    WARMUP_INFO warmup)
{
    TRACE_HANDLE streamId = stream;

    T1("\tFEED_Warmup from stream id=" << streamId.Handle());

    if ((! trace[streamId].IsWarmUp()) || trace[streamId].Eof())
    {
        return false;
    }

    //
    // Instruction is warm-up
    //
    TRACE_INST trInst = trace[streamId].GetInst();
    UINT64 traceId = trace[streamId].GetIdentifier();
    trace[streamId].NextInstr();

    //
    // Unfortunately we have to translate the instruction to learn
    // about it.  The trace data structures don't describe enough.
    //
    ASIM_INST inst = NULL;
    inst = warmup->InitAsimInst();
    traceConverter->Convert(inst, trInst, trace[streamId].GetInst());

    //
    // Note instruction fetch
    //
    UINT64 va = trInst->VirtualPc().GetBundleAddr();
    UINT64 pa = 0;
    addrTrans.ITranslate(inst->GetSWC()->GetHWCNum(), va, pa);
    warmup->NoteIFetch(va, pa);

    if (inst->IsControlOp())
    {
        warmup->NoteCtrlTransfer();
    }

    //
    // Only describe mem ref if predicated true.
    //
    if (inst->GetQP() && inst->HasEffAddress())
    {
        UINT64 va = inst->GetVirtualEffAddress();
        UINT64 pa = 0;
        addrTrans.DTranslate(inst, va, pa);

        UINT32 accessSize = 8;
        if (inst->IsLoad() || inst->IsStore())
        {
            accessSize = inst->GetAccessSize();
        }

        if (inst->IsStore())
        {
            warmup->NoteStore(va, pa, accessSize);
        }
        else
        {
            warmup->NoteLoad(va, pa, accessSize);
        }
    }

    trace[streamId].CommitInstr(traceId);

    return true;
}

/*
 * Parse command line arguments, return false if there is
 * a parse error.
 *
 * r2r: FIXME: this is rather ugly since the trace feeder has to parse
 *      trace reader arguments and pass them to the trace reader init
 *      function, polluting this interface with trace reader specific
 *      parameter requirements;
 *      we need a better way of parsing arguments!
 */
bool 
TRACE_FEEDER_CLASS::Parse (
    UINT32 argc,
    char **argv)
{
    bool success = false;

    for (UINT32 i = 0; i < argc; i++) 
    {
        //if (!strcmp(argv[i], "-oracle"))
        //{
        //    runOracle = true;
        //    continue;
        //}
        
        // Try to open the trace file...
        for( UINT32 j=0; j < FEEDER_REPLICATE; j++)
        {
            //cerr << "Opening replicated trace " << j << " fn= " << argv[i] << endl;
            success = traceReader->FileOpen(threads++, argv[i]);
            if (!success) 
            {
                cerr << "Couldn't open trace file " << argv[i] << endl;
                return(false);
            }
        }
    }

    // Make sure a file was specified.
    if (!success) 
    {
        ASIMWARNING("Trace file not specified\n");
        return(false);
    }

    return(true);
}


/********************************************************************
 *
 * FEEDer callbacks
 *
 *******************************************************************/


UINT64
TRACE_FEEDER_CLASS::Skip (
    IFEEDER_STREAM_HANDLE stream,
    UINT64 n,
    INT32 markerID)
{
    //
    // If 'stream' == NULL, we skip every thread...
    //

    UINT32 firstThread;
    UINT32 lastThread;
    UINT64 minSkipped = UINT64_MAX;

    if(!n) n = UINT64_MAX;

    if (stream == NULL) {
        firstThread = 0;
        lastThread = threads;
    } else {
        TRACE_HANDLE streamId = stream;
        firstThread = streamId;
        lastThread = streamId + 1;
    }


    for (UINT32 i = firstThread; i < lastThread; i++) {
        if (!trace[i].Ended()) {
            // print what we are doing
            cout << "Start Skipping: thread " << i
                 << " at pc 0x" << trace[i].VirtualPc() << endl;
            if (n != UINT64_MAX) {
                cout << " for " << n << " instructions";
                if (markerID >= 0) {
                    cout << " or";
                }
            }
            if (markerID >= 0) {
                cout << " until marker " << markerID;
            }
            cout << endl;

            UINT64 skipped = SkipHelper(i, n, markerID);
            minSkipped = MIN(skipped, minSkipped);

            // print what we are doing
            cout << "End Skipping: thread " << i;
            if (markerID >= 0) {
                cout << " when hitting marker " << markerID;
            }
            cout << " at pc 0x" << trace[i].VirtualPc() << endl;
            cout << "End Skipping: "
                 << skipped << " instructions have been skipped." << endl;
        }
    }

    return(minSkipped);
}


UINT64
TRACE_FEEDER_CLASS::SkipHelper (
    const UINT32 tid,
    const UINT64 n,
    const INT32 markerID)
{
    UINT64 instructions = 0;

    trace[tid].CommitAll();
    while(instructions < n) {
        if(trace[tid].Eof()) {
            break;
        }
        trace[tid].NextInstr();  // Go to next instruction in trace

        // (IA64) traces can have embedded wrongpath instructions
        // (syllables after taken branches but in same bundle); we
        // don't want to count those;
        if (trace[tid].IsWrongpath()) {
            continue;
        } else {
            trace[tid].CommitInstr(trace[tid].GetIdentifier());
            instructions++;
        }

        // stop if we hit a marker;
        // putting this at the end of the loop makes sure we step over
        // this marker if we enter Skip again with the PC still at
        // the marker, i.e. the first insn is always executed!
        if (markerID >= 0 &&
            marker.IsSet(tid, markerID, trace[tid].VirtualPc()))
        {
            break;
        }
    }
    trace[tid].NextInstr(); 
    instructions ++;
    trace[tid].Thread()->SetVirtualPc(trace[tid].VirtualPc());
    wrongpath[tid] = false;
    return instructions-1;
}

/********************************************************************
 * Marker manipulation: set / clear
 * A marker is a binary flag on a static instruction.
 * We support up to 16 markers in this feeder. Each marker ID keeps
 * track of one PC address on a specific thread;
 *******************************************************************/
void
TRACE_FEEDER_CLASS::Marker (
    const ASIM_MARKER_CMD cmd,
    const IFEEDER_STREAM_HANDLE stream,
    const UINT32 markerID,
    const IADDR_CLASS markerPC,
    const UINT32 instBits,
    const UINT32 instMask)
{
    ASSERT (markerID < MAX_MARKER, "marker ID outside valid range");

    TRACE_HANDLE streamId = stream;
    UINT32 tid = streamId;

    switch (cmd) {
      case MARKER_CLEAR_ALL:
        marker.ClearAll (tid, markerID);
        cout << "Clearing marker " << markerID
             << " on thread " << tid
             << " at all addresses" << endl;
        break;
      case MARKER_CLEAR_PC:
        marker.ClearPc (tid, markerID, markerPC);
        cout << "Clearing marker " << markerID
             << " on thread " << tid
             << " at pc 0x" << markerPC << endl;
        break;
      case MARKER_SET_PC:
        marker.SetPc (tid, markerID, markerPC);
        cout << "Setting marker " << markerID
             << " on thread " << tid
             << " at pc 0x" << markerPC << endl;
        break;
      case MARKER_SET_INST:
        ASIMERROR("TRACE_FEEDER::Marker: marker command MARKER_SET_INST "
                  "not supported on Trace Feeder\n");
        break;
      default:
        ASIMERROR("TRACE_FEEDER::Marker: invalid marker command <"
            << cmd << ">" << endl);
    }
}

/********************************************************************
 * Translate from virtual to physical address.
 *******************************************************************/
  
bool
TRACE_FEEDER_CLASS::ITranslate(const UINT32 hwcNum,
                          const UINT64 va,
                          UINT64& pa)
{
    /*
     * Use the addrTranslator module to determine physical address 
     * of instruction.
     */
    return (addrTrans.ITranslate(hwcNum, va, pa));
}

bool
TRACE_FEEDER_CLASS::DTranslate(ASIM_INST inst,
                          const UINT64 va,
                          UINT64& pa)
{
    /*
     * Use the addrTranslator module to determine physical address 
     * of instruction.
     */
    return (addrTrans.DTranslate(inst, va, pa));
}

bool
TRACE_FEEDER_CLASS::Fetch (
    IFEEDER_STREAM_HANDLE stream,
    IADDR_CLASS predicted_pc,
    ASIM_INST inst)
{
    pthread_mutex_lock(fetch_lock);
    IADDR_CLASS pc;
    
    TRACE_HANDLE streamId = stream;

    TTMSG(Trace_Feeder,
          "\tFEED_Fetch from stream id=" << streamId.Handle()
          << "\tip: " << predicted_pc
          << "\tindex " << trace[streamId].GetIdentifier()); 

    // check if there are any wrongpath instructions embedded in the
    // trace that need to be skipped over;
    UINT32 skipped;
    skipped = trace[streamId].SkipOverWrongpath(predicted_pc);
    if (skipped != 0) {
        TRACE(Trace_Feeder,
            cout << "\tSkipped " << skipped
                 << " embedded trace wrongpath instructions.\n");
        totalSkippedWrongpath += skipped;
    }

    // if the context switch flag is set, then all the pm is asking for is for
    // the feeder to create a nop inst.  So, check for this first, and if
    // found, return the nop.
    if (inst->GetAsimSchedulerContextSwitch() == true)
    {
        TTMSG(Trace_Feeder, "Context switch requested.  Creating a NOP to represent a context switch.");
        traceConverter->CreateOffPathInst(predicted_pc, inst);
        //   ... and set the trace ID (for interface betwen PM and FEEDER)
        inst->SetTraceID(traceArtificialInstId);
        pthread_mutex_unlock(fetch_lock);
        return true;
    }

    // If we're at the end of the trace, then signal the end
    // of thread, and pad the end of trace with NOPs;
    if(trace[streamId].Eof())
    {
        pc = predicted_pc;
        TTMSG(Trace_Feeder,
            "\tEnding thread, uid = " << streamId.Handle()); 
        TMSG(Trace_Feeder,
            "\tSkipped " << totalSkippedWrongpath
                 << " total embedded trace wrongpath instructions.");
        if (!trace[streamId].Ended()) {
            trace[streamId].Thread()->ThreadEnd();
            trace[streamId].Ended() = true;
        }
        // now generate a valid ASIM instruction (off trace path)
        traceConverter->CreateOffPathInst(predicted_pc, inst);

        // ***** why are we inserting a valid trace id when this inst didn't come from
        // the trace?
        inst->SetTraceID(trace[streamId].GetIdentifier());
        pthread_mutex_unlock(fetch_lock);
        return false;
    }
    else {
        pc = trace[streamId].VirtualPc();  // get current PC
    }
        
    // if 'predicted_pc' is 0 and it's at slot 0, then we must be just
    // starting up this thread.  Create a branch inst to the start of the
    // trace, that way, when it mispredicts, it'll resteer to the correct
    // address.
    if (predicted_pc.GetAddr() == 0 && predicted_pc.GetSyllableIndex() == 0)
    {
        TTMSG(Trace_Feeder, "\tAttempt to fetch IP 0 due to thread startup.  Creating BR starting trace IP.");

        TRACE_INST traceInst = trace[streamId].GetInst();

        // now generate a valid ASIM branch instruction 
        traceConverter->CreateStartupBranch(predicted_pc, pc, inst, traceInst);
        //   ... and set the trace ID (for interface betwen PM and FEEDER)
        inst->SetTraceID(traceArtificialInstId);
        pthread_mutex_unlock(fetch_lock);
        return true;
    }

    // If 'predicted_pc' is not the pc of the next instruction
    // in the trace, then the previous instruction
    // must have mispredicted. Return a nop for now...
    //
    // also, the 'predicted_pc' could happen to be the same as
    // an embedded wrongpath instruction
    else if (pc != predicted_pc || trace[streamId].IsWrongpath() || wrongpath[streamId]) {
        //
        // Create a NOP to fake down mispredicted path
        if(!wrongpath[streamId]) {
            TRACE(Trace_Feeder, cout << "\tgoing to WRONGPATH!" << endl); 
        }

        TRACE(Trace_Feeder,
            cout << "\tDETECT pc(" << pc
                 << ") != predicted_pc(" << predicted_pc
                 << ")" << endl);

        // now generate a valid ASIM instruction NoOp(off trace path)
        traceConverter->CreateOffPathInst(predicted_pc, inst);
        //   ... and set the trace ID (for interface betwen PM and FEEDER)
        inst->SetTraceID(traceWrongpathId);

        wrongpath[streamId] = true;
        pthread_mutex_unlock(fetch_lock);
        return true;
    }

    // the PM is still on trace, so grab the next instruction.
    //
    TRACE_INST traceInst = trace[streamId].GetInst();
    UINT64 traceId = trace[streamId].GetIdentifier();
    trace[streamId].NextInstr();                 // Go to next instruction in trace
    TRACE_INST nextTraceInst = trace[streamId].GetInst();
    IADDR_CLASS nextpc = trace[streamId].VirtualPc(); // Get pc of subsequent instr

    // now convert the trace instruction into an ASIM instruction
    traceConverter->Convert(inst, traceInst, nextTraceInst);
    //   ... and set the trace ID (for interface between PM and FEEDER)
    inst->SetTraceID(traceId);

    // initialize oracle shadow
//    if (runOracle) {
//        oracle->Fetch(inst, traceInst, nextpc);
//    }
    pthread_mutex_unlock(fetch_lock);
    return true;
}


/*
 * 'inst' is committed...
 */
void
TRACE_FEEDER_CLASS::Commit (
    ASIM_INST inst)
{
    pthread_mutex_lock(commit_lock);
    if (inst->GetTraceID() == traceWrongpathId) {
        ASIMERROR(
            "TRACE_FEEDER::Commit: Attempt to commit wrongpath instruction\n"
            << "\t" << inst->GetVirtualPC() << ": "
            << inst->GetDisassembly() << endl);
    }

    if (inst->GetTraceID() == traceArtificialInstId)
    {
        // we're committing an instruciton that was artificial, i.e., we
        // manufactured it and it doesn't exist on the trace, so just return.
        pthread_mutex_unlock(commit_lock);
        return;
    }
    
    UINT32 sid = TRACE_HANDLE(inst->GetSWC()->GetFeederStreamHandle());
    trace[sid].CommitInstr(inst->GetTraceID());

    // release the oracle shadow if we have one
//    if (runOracle) {
//        oracle->Commit(inst);
//    }

    INT32 markerID = asimSystem->SYS_CommitWatchMarker();
    if (markerID >= 0 &&
        marker.IsSet (sid, markerID, IADDR_CLASS(inst->GetVirtualPC())))
    {
        asimSystem->SYS_CommittedMarkers()++;
    }
    pthread_mutex_unlock(commit_lock);
}


/*
 * kill 'inst'...
 */
void
TRACE_FEEDER_CLASS::Kill (
    ASIM_INST inst,
    const bool fetchNext,
    const bool killMe)
{
    pthread_mutex_lock(kill_lock);

    // in most cases, the current instruction is not killed if we continue
    // to fetch from the next instruction, and the current instruction
    // is killed if we refetch the current instruction; typically
    // this is true for branch mispredicts and ld/st order traps;
    // in some cases, however, we continue to fetch from the next
    // instruction but still kill the current instruction; this is the
    // case if dtb PALflows are embedded in a trace in which case
    // we see the excepting memory operation first, which is killed and
    // requires us to continue fetching from the next instruction;
    // in this case killMe is supplied from the caller;
    TRACE(Trace_Feeder,
        cout << "\tFEED_Kill id=" << inst->GetTraceID()
             << " fetchNext=" << fetchNext
             << " killMe=" << killMe << endl);
    ASSERT (killMe | fetchNext, "Can't refetch unkilled instruction\n");

    UINT32 sid = TRACE_HANDLE(inst->GetSWC()->GetFeederStreamHandle());

    if ((inst->GetTraceID() != traceWrongpathId) && 
        (inst->GetTraceID() != traceArtificialInstId))
    {
        wrongpath[sid] = false;

        trace[sid].Backup(inst->GetTraceID());

        if (fetchNext) 
        {
            // this instruction will not be refetched
            trace[sid].NextInstr();
        }
        // release the oracle shadows
//        if (inst->GetOracle()) 
//        {
//            oracle->Kill(inst, killMe);
//        }
    }

    // hmmm..... if we manufacture a branch to the correct starting IP and it
    // mispredicts, we need to set wrongpath[] to false, so we can resteer to
    // the correct ip
    if (inst->GetTraceID() == traceArtificialInstId)
    {
        wrongpath[sid] = false;
    }

    pthread_mutex_unlock(kill_lock);
}



/********************************************************************
 *
 * Controller calls
 *
 *******************************************************************/

/*
 * Initialize Trace instruction feeder. Return false
 * if we have an error during initialization.
 */
bool
TRACE_FEEDER_CLASS::Init (
    UINT32 argc,
    char **argv,
    char **envp)
{
    // instantiate and initialize the trace reader
    traceReader = new TRACE_READER_CLASS();
    ASSERTX(traceReader);

    // Parse the command-line.
    if (!Parse(argc, argv))
    {
        return(false);
    }

    // finish initializing traceReader
    traceReader->Init();

    // instantiate and initialize the trace converter
    traceConverter = new TRACE_CONVERTER_CLASS;
    ASSERTX(traceConverter);

    // Read the first instruction in the trace file. From this
    // instruction we can create the ASIM_TRACE object for
    // the trace containing the instruction.
    trace = new TRACE_BUFFER_CLASS[threads];
    for(UINT32 t = 0; t < threads; t++) {
        wrongpath[t] = false;
        trace[t].Init(traceReader, t);
        IADDR_CLASS sentinel = UINT64_MAX;
        if (trace[t].VirtualPc() == sentinel) {
            trace[t].Ended() = true;
            traceReader->FileClose(t);
            cerr << "Trace " << t << " contains no instructions." << endl;
            return(false);
        }

        // Notify the PM that a new thread has started.
        const IADDR_CLASS vpc = trace[t].VirtualPc();
        const IFEEDER_STREAM_HANDLE handle = TRACE_HANDLE(t);
        trace[t].Thread() = new IFEEDER_THREAD_CLASS(this, handle, vpc);
    }

//    if (runOracle) {
//        oracle = new TRACE_ORACLE_CLASS;
//    }

    return(true);
}
bool 
TRACE_FEEDER_CLASS::GetInputRegVal(
        ASIM_INST inst,
        UINT32 slot,
        ARCH_REGISTER reg)
{
    bool ret_val;

    if (slot >= NUM_SRC_GP_REGS) return false;


    UINT32 streamId = TRACE_HANDLE(inst->GetSWC()->GetFeederStreamHandle());
    ret_val = trace[streamId].GetInputRegVal(inst->GetUid(),slot,reg);
    if(!ret_val)
    {
        cout<<"Unable to get the register values for: "<<inst->GetRawInst()->DisasmString()<<endl;
        cout<<"Uid: "<<inst->GetUid()<<endl;
        ASSERT(0,"Failure to get dest register values");
    }
    TRACE(Trace_Feeder,cout<<"\tcheck in feeder valid: "<<reg->IsValid()<<" type: "<<reg->GetType()<<" number"<<reg->GetNum()<<endl);

    return true;
}

bool 
TRACE_FEEDER_CLASS::GetPredicateRegVal(
        ASIM_INST inst,
        UINT32 slot,
        ARCH_REGISTER reg)
{
    bool ret_val;

    if (slot >= NUM_SRC_PRED_REGS) return false;

    UINT32 streamId = TRACE_HANDLE(inst->GetSWC()->GetFeederStreamHandle());
    ret_val = trace[streamId].GetPredicateRegVal(inst->GetUid(),slot,reg);
    if(!ret_val)
    {
        cout<<"Unable to get the register values for: "<<inst->GetRawInst()->DisasmString()<<endl;
        cout<<"Uid: "<<inst->GetUid()<<endl;
        ASSERT(0,"Failure to get dest register values");
    }
    TRACE(Trace_Feeder,cout<<"\tcheck in feeder valid: "<<reg->IsValid()<<" type: "<<reg->GetType()<<" number"<<reg->GetNum()<<endl);

    return true;
}


bool 
TRACE_FEEDER_CLASS::GetOutputRegVal(
        ASIM_INST inst,
        UINT32 slot,
        ARCH_REGISTER reg)
{

    bool ret_val;

    if (slot >= NUM_DST_REGS) return false;

    UINT32 streamId = TRACE_HANDLE(inst->GetSWC()->GetFeederStreamHandle());
    ret_val = trace[streamId].GetOutputRegVal(inst->GetUid(),slot,reg);
    if(!ret_val)
    {
        cout<<"Unable to get the register values for: "<<inst->GetRawInst()->DisasmString()<<endl;
        cout<<"Uid: "<<inst->GetUid()<<endl;
        ASSERT(0,"Failure to get dest register values");
    }
    TRACE(Trace_Feeder,cout<<"\tcheck in feeder valid: "<<reg->IsValid()<<" type: "<<reg->GetType()<<" number"<<reg->GetNum()<<endl);

    return true;


}

/*
 * Cleanup..
 */
void
TRACE_FEEDER_CLASS::Done (void)
{
    UINT64 t;
    
    for(t=0; t<threads; t++)
    {
        traceReader->FileClose(t);
    }

    traceReader->Done();
}


/*
 * Print usage...
 */
void
IFEEDER_Usage(
    FILE * const file)
{
    ostringstream os;

    os << "\nFeeder usage: ... <tracefile> [<tracefile> ...] ...\n"
       << "\tFilenames may trail .gz, .z or .Z for piping through gunzip\n"
       << "\tFilenames may trail .bz2 for piping through bunzip2\n"
       << "\tFilenames may end in :# to limit to one CPU's trace\n";

    fputs(os.str().c_str(), file);
}


IFEEDER_BASE IFEEDER_New(void)
{
    return new TRACE_FEEDER_CLASS();
}
