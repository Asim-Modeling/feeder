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
 * 
 */

/**
 * @file
 * @brief Implementation of the SW_CONTEXT_CLASS class for ASIM 
 *
 */

// ASIM core
#include "asim/trace.h"
#include "asim/atomic.h"

// ASIM local stuff
#include "asim/provides/software_context.h"
#include "asim/provides/context_scheduler.h"
#include "asim/provides/hardware_context.h"
#include "asim/provides/isa.h"
#include "asim/provides/iaddr.h"
// for feeder ops
#include "asim/provides/instfeeder_interface.h"


// Initialize the shared ID, which is incremented each time an instance
// of SW_CONTEXT_CLASS is created, to generate an object-specific unique ID.

UID_GEN32 SW_CONTEXT_CLASS::uniqueStaticID = 0;

/* ------------------------ Constructor and destructor --------------*/

SW_CONTEXT_CLASS::SW_CONTEXT_CLASS 
   (IFEEDER_BASE ifeeder,
    IFEEDER_STREAM_HANDLE iStream,
    IADDR_CLASS pc,
    CONTEXT_SCHEDULER contextScheduler)
       : iFeeder(ifeeder),
         uniqueID(uniqueStaticID++), 
         feederStreamHandle(iStream), 
         resumptionPC(pc),
         hwc(ASIM_HWC_NONE),
         contextSwitchState(CONTEXT_SWITCH_NONE),
         contextSchedulerHandle(contextScheduler),
         pid(-1),
         inTimerInterrupt(false),
         mappedToHWC(false),
         runnable(true), 
         deschedulePending(false),
         deletePending(false), 
         streamEnded(false) ,
         running(false)
{ 
    strcpy(procName, "<unknown>");
    SetTraceableName("SW_CONTEXT_CLASS");
    T1("Created swc #" << uniqueID);
}


SW_CONTEXT_CLASS::~SW_CONTEXT_CLASS()
{
}



/* ----------------- Accessors and modifiers------------------------- */

//Accessors


IFEEDER_BASE
SW_CONTEXT_CLASS::GetIFeeder(void) const
{
    return iFeeder;
}

UINT32 
SW_CONTEXT_CLASS::GetUniqueID(void) const 
{ 
     return(uniqueID); 
}

IADDR_CLASS
SW_CONTEXT_CLASS::GetResumptionPC (void) const 
{ 
    return(resumptionPC); 
}

HW_CONTEXT
SW_CONTEXT_CLASS::GetHWC (void) const 
{ 
    return(hwc);
}

INT32
SW_CONTEXT_CLASS::GetHWCNum (void) const 
{ 
    if (hwc)
    {
        return(hwc->GetUID());
    }
    else
    {
        return (-1);
    }
}

bool
SW_CONTEXT_CLASS::GetMappedToHWC (void) const 
{ 
    return(mappedToHWC);
}

bool 
SW_CONTEXT_CLASS::GetRunnable (void) const 
{ 
    return(runnable);
}

bool 
SW_CONTEXT_CLASS::GetDeschedulePending (void) const 
{
    return(deschedulePending);
}

bool 
SW_CONTEXT_CLASS::GetDeletePending (void) const 
{
    return(deletePending);
}

IFEEDER_STREAM_HANDLE
SW_CONTEXT_CLASS::GetFeederStreamHandle(void) const
{
    return(feederStreamHandle);
}


//Modifiers

void 
SW_CONTEXT_CLASS::SetResumptionPC (IADDR_CLASS pc) 
{ 
    resumptionPC = pc; 
}

void 
SW_CONTEXT_CLASS::SetMappedToHWC (void)
{ 
    mappedToHWC = true; 
}

void 
SW_CONTEXT_CLASS::SetNotMappedToHWC (void) 
{
    mappedToHWC = false;
}

void 
SW_CONTEXT_CLASS::SetHWC (HW_CONTEXT context)
{ 
    hwc = context;
}

void 
SW_CONTEXT_CLASS::SetNoHWC (void) 
{
    hwc = ASIM_HWC_NONE;
}
 
void 
SW_CONTEXT_CLASS::SetRunnable (void) 
{ 
    runnable = true; 
}

void
SW_CONTEXT_CLASS::SetNotRunnable (void)
{
    runnable = false;
}


void 
SW_CONTEXT_CLASS::SetDeschedulePending (void) 
{
    cout<<"Deschedule called"<<endl;
    deschedulePending = true;
}
    
void 
SW_CONTEXT_CLASS::SetNotDeschedulePending (void) 
{
    deschedulePending = false;
}
    
void 
SW_CONTEXT_CLASS::SetDeletePending (void)
{
    deletePending = true;
}



/* ------------------------ IDToString --------------------------------------*/
/** 
 * Routine to print useful identifying information about a software
 * context to a string. Call this instead of writing your own code. 
 * That way, as we evolve the way of identifying a software context, 
 * only this routine will have to change.
 *
 * For tracing, do this: 
 * TRACE(swc -> IDToString();
 * Substitute the name of the variable that contains a pointer to an 
 * object of type SW_CONTEXT_CLASS.
 */

string
SW_CONTEXT_CLASS::IDToString()
{
    std::ostringstream buf;
    buf << "Software context: Unique ID = " << uniqueID 
        << " Feeder's stream ID = " << feederStreamHandle;
    return(buf.str());
}

/* -------------------- StateToString ---------------------------------------*/
/** 
 * Routine to print useful identifying information about a software
 * context to a string. Call this instead of writing your own code. 
 * That way, as we evolve the way of identifying a software context, 
 * only this routine will have to change.
 *
 * For tracing, do this: 
 * TRACE(swc);
 * Substitute the name of the variable that contains a pointer to an 
 * object of type SW_CONTEXT_CLASS.
 */

string SW_CONTEXT_CLASS::StateToString()
{
    std::ostringstream buf;
    buf << "\n\tState of SWC: ";
    buf << IDToString() << "\n";

    if (mappedToHWC == true)
    {
        buf << "\tMapped to hardware context" << "\n";

    }
    else
    {
        buf << "\tNot mapped to hardware context" << "\n";
    }
    

    if (GetRunnable())
    {
        buf << "\tSoftware context is runnable" << "\n";
    }
    else
    {
        buf << "\tSoftware context is not runnable" << "\n";
    }
    
    if (GetDeschedulePending())
    {
        buf << "\tA deschedule is pending" << "\n";
    }
    else
    {
        buf << "\tNo deschedule is pending" << "\n";
    }

    if (GetDeletePending())
    {
        buf << "\tA delete is pending" << "\n";
    }
    else
    {
        buf << "\tNo delete is pending" << "\n";
    }

    if (streamEnded)
    {
        buf << "\tstreamEnded is true" << "\n";
    }
    else
    {
        buf << "\tstreamEnded is false" << "\n";
    }
        
    buf << ContextSwitchStateToString();

    return(buf.str());
}


/* ---------------------- Initiate Context Switch -------------------------*/

/**
 * Called by the scheduler when it wants a software context to stop running
 * on a hardware context. Records that this happened, so that the next time 
 * that the timing model requests a SWARM_OP, it will get a context switch.
 * We don't want to send it out-of-band from here.
 */

void
SW_CONTEXT_CLASS::InitiateContextSwitch(void)
{
    T1("\tInitiateContextSwitch called for " << IDToString());

    if (contextSwitchState != CONTEXT_SWITCH_NONE)
    {
        ASIMWARNING("SW_CONTEXT_CLASS::InitiateContextSwitch called when " 
                    << "current state is not NONE" <<
                    contextSwitchState << endl);
        T1(this);
    }
    
    contextSwitchState = CONTEXT_SWITCH_REQUESTED;
}



/**
 * Check for pre-execution warm-up data.

 */

void
SW_CONTEXT_CLASS::WarmUpClientInfo(
    const WARMUP_CLIENTS clientInfo)
{
    iFeeder->WarmUpClientInfo(clientInfo);
}


bool
SW_CONTEXT_CLASS::WarmUp(
    WARMUP_INFO warmup)
{
    warmup->NoteSWContext(this);
    return iFeeder->WarmUp(feederStreamHandle, warmup);
}


/* -------------------------Fetch------------------------------------*/

/**
 * Get the next instruction from the feeder.  Called from the hardware context
 * mapped to this software context and return an ASIM_INST.
 *
 */
ASIM_INST 
SW_CONTEXT_CLASS::Fetch(
    UINT64 cycle,
    IADDR_CLASS ip)
{
    ASIM_INST ainst;
    bool success;
    IADDR_CLASS fetchIp = ip;
    
    // The scheduler may have requested a context switch since this
    // routine was called last for this swc. If so, send
    // one and record that we did.
    if (contextSwitchState == CONTEXT_SWITCH_REQUESTED || contextSwitchState == CONTEXT_SWITCH_SENT)
    {
        ainst = new ASIM_INST_CLASS(this);

        T1("\tThis a contextSwitch is requested or sent ");
        ainst->SetAsimSchedulerContextSwitch(true);
        success = iFeeder->Fetch(feederStreamHandle, ip, ainst, cycle);

        ASSERTX(success);

        contextSwitchState = CONTEXT_SWITCH_SENT;
        
        return ainst;
    }
    else
    {
        ainst = new ASIM_INST_CLASS(this);

        if (!running)
        {
            // this is the first fetch for this swc ever, or since it was last
            // switched out by the context scheduler.  As a result, we need to
            // override the IP the model is asking for (as it's most likely
            // wrong), and by telling the feeder to fetch from IP 0, it knows to
            // manufacture a BR to the correct target address.
            fetchIp.SetIstreamStartup();
            T1("\tThis SWC is starting up.  Calling SetIstreamStartup.");
            running = true;
        }

        // Get architectural instruction from feeder. 
        success = iFeeder->Fetch(feederStreamHandle, fetchIp, ainst, cycle);
        if (!success)
        {
            HandleEndThread();
        }
        return ainst;
    }
}

            
/**
 * Get the next instruction from the feeder.  Called from the hardware context
 * mapped to this software context and return an ASIM_INST.
 *
 */
ASIM_INST 
SW_CONTEXT_CLASS::FetchMicro(
    UINT64 cycle,
    ASIM_MACRO_INST macro,
    IADDR_CLASS ip)
{
    ASIM_INST ainst;
    bool success;
    IADDR_CLASS fetchIp = ip;
    
    // The scheduler may have requested a context switch since this
    // routine was called last for this swc. If so, send
    // one and record that we did.
    if (contextSwitchState == CONTEXT_SWITCH_REQUESTED || contextSwitchState == CONTEXT_SWITCH_SENT)
    {
        ainst = new ASIM_INST_CLASS(this);

        T1("\tThis a contextSwitch is requested or sent ");
        ainst->SetAsimSchedulerContextSwitch(true);
        success = iFeeder->FetchMicro(feederStreamHandle, ip, macro, ainst, cycle);

        ASSERTX(success);

        contextSwitchState = CONTEXT_SWITCH_SENT;
        
        return ainst;
    }
    else
    {
        ainst = new ASIM_INST_CLASS(this);

        if (!running)
        {
            // this is the first fetch for this swc ever, or since it was last
            // switched out by the context scheduler.  As a result, we need to
            // override the IP the model is asking for (as it's most likely
            // wrong), and by telling the feeder to fetch from IP 0, it knows to
            // manufacture a BR to the correct target address.
            fetchIp.SetIstreamStartup();
            T1("\tThis SWC is starting up.  Calling SetIstreamStartup.");
            running = true;
        }

        // Get architectural instruction from feeder. 
        success = iFeeder->FetchMicro(feederStreamHandle, fetchIp, macro, ainst, cycle);
        if (!success)
        {
            HandleEndThread();
        }
        return ainst;
    }
}

ASIM_MACRO_INST 
SW_CONTEXT_CLASS::FetchMacro(
    UINT64 cycle,
    IADDR_CLASS ip)
{
    ASIM_MACRO_INST macro_inst;
    bool success;
    IADDR_CLASS fetchIp = ip;
    
    macro_inst = new ASIM_MACRO_INST_CLASS(this);

    // Get architectural instruction from feeder. 
    success = iFeeder->FetchMacro(feederStreamHandle, fetchIp, macro_inst, cycle);
    if (!success)
    {
        macro_inst = NULL;
    }

    return macro_inst;
}

/* ----------------- Commit  --------------------------------*/

/**
 * Commit an instruction for this software context.  Called from ASIM_INST (I think).

 */

void
SW_CONTEXT_CLASS::Commit(
    ASIM_INST ainst) 
{
    // commit all instructions.  The feeder will figure out if it's on trace and
    // should really be committed or if it's a manufactured instruction that
    // doesn't need to commit.
    iFeeder->Commit(ainst);
    
    // commit if it's not a context switch instruction
    if (ainst->GetAsimSchedulerContextSwitch())
    {
        // If this is a context switch AND the state of the context switch is SENT,
        // record that it was done and tell the scheduler that the context switch is
        // complete.  Since we continually send context swtich instructions, we need
        // to make sure we only signal the first one to commit, that's why we're
        // checking the context switch state.  Finally, reset the state so that the
        // next time it runs, it won't look like it is doing a context switch..
        if (contextSwitchState == CONTEXT_SWITCH_SENT)
        {
            contextSwitchState = CONTEXT_SWITCH_NONE;
            
            SetResumptionPC(ainst->GetVirtualPC());
            
            T1("\tContextSwitchCompleted");
            contextSchedulerHandle->ContextSwitchCompleted(this);

            // now that the context scheduler has descehduled this SWC, there
            // should be NO fetch from this swc, and hence, it's not running.
            running = false;
        }
    }
}


/************ Start of Private Member Functions ***************************/


/*---------------------- HandleEndThread -------------------------------*/

/**
 * Java reader has said that a thread was terminated.
 * Tell the scheduler. Set contextSwitchState so that FetchOperation will
 * know to send a context switch.
 * 
 */

void 
SW_CONTEXT_CLASS::HandleEndThread()
{    
    T1("\tRequesting context switch due to thread ending");
//    contextSwitchState = CONTEXT_SWITCH_SENDING;
    contextSwitchState = CONTEXT_SWITCH_REQUESTED;
    contextSchedulerHandle->SoftwareContextExited(this);
    streamEnded = true;
}



/* -------------- ContextSwitchStateToString --------------------------------*/

/** Routine to print context switch state information for a software
 * context to a string. Called from StateToString.
 *
 * Generally, the caller will probably call IDToString before calling this.
 */
string
SW_CONTEXT_CLASS:: ContextSwitchStateToString(void)
{
    std::ostringstream buf;
    
    buf << "\tContext Switch State is ";
    switch (contextSwitchState)
    {
      case CONTEXT_SWITCH_NONE:
        buf << "NONE";
        break;
      case CONTEXT_SWITCH_REQUESTED:
        buf << "REQUESTED";
        break;
      case CONTEXT_SWITCH_SENDING:
        buf << "SENDING";
        break;
      case CONTEXT_SWITCH_SENT:
        buf << "SENT";
        break;
      case CONTEXT_SWITCH_COMMITTED:
        buf << "COMMITTED";
        break;
      default:
        buf << "*** unrecognized ***";
    }
    return(buf.str());
}

/* -------------- DumpStats --------------------------------*/
void
SW_CONTEXT_CLASS::DumpStats(
    STATE_OUT stateOut)
{
    statCache.RegisterPerinstStats(ASIM_REGISTRY(this));
    ASIM_REGISTRY_CLASS::DumpStats(stateOut);
}
