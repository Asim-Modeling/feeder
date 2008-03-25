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
 */

/**
 * @file
 * @brief Implementation of the SW_CONTEXT_CLASS class for Swarm
 * Accepts requests for Swarm operations and gets events from Mingo
 *
 * @author Judy Hall
 *
 */

/*
 * Possible future enhancements:
 *
 * - Don't create a new MINGO_DATA for each request, since there is
 *   never more than one outstanding. Would need to clear the fields,
 *   though. Not clear which is cheaper.
 *
 * - In Fetch, count up AllOutstandingOpCount for everything that
 *   goes to Swarm. In Commit, decrement it for all op types. If a
 *   context switch is committed, crash if the count isn't zero, so we
 *   can understand why that would happen.
 *
 */

// ASIM core
#include "asim/trace.h"

// ASIM local stuff
#include "asim/provides/software_context.h"
#include "asim/provides/context_scheduler.h"
#include "asim/provides/hardware_context.h"

// Pointer to Mingo

extern MINGO_FEEDER MingoFeederHandle;


// Initialize the shared ID, which is incremented each time an instance
// of SW_CONTEXT_CLASS is created, to generate an object-specific unique ID.

UINT64 SW_CONTEXT_CLASS::uniqueStaticID = 0;


/* ------------------------ Constructor and destructor --------------*/

SW_CONTEXT_CLASS::SW_CONTEXT_CLASS 
    (UINT64 streamID, UINT64 pc, CONTEXT_SCHEDULER contextScheduler)
        : uniqueID(uniqueStaticID++), 
          feederStreamID(streamID), 
          resumption_PC(pc),
          hwc(ASIM_HWC_NONE),
          mappedToHWC(false),
          runnable(true), 
          deschedulePending(false),
          deletePending(false), 
          inTimedWait(false),
          minCycleToResume(0),
          pendingOperation(false), 
          precedingInstCount(0), 
          streamEnded(false) ,
          receivedTimedWait(false),
          proceedNow(false),
          outstandingOpCount(0),
          contextSwitchState(NONE),
          contextSchedulerHandle(contextScheduler)
{ 
    
}


SW_CONTEXT_CLASS::~SW_CONTEXT_CLASS()
{
}



/* ----------------- Accessors and modifiers------------------------- */

//Accessors


UINT32 
SW_CONTEXT_CLASS::GetUniqueID(void) const 
{ 
     return(uniqueID); 
}

UINT64 
SW_CONTEXT_CLASS::GetResumptionPC (void) const 
{ 
    return(resumption_PC); 
}

HW_CONTEXT
SW_CONTEXT_CLASS::GetHWC (void) const 
{ 
    return(hwc);
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

bool
SW_CONTEXT_CLASS::GetInTimedWait (void) const
{
    return(inTimedWait);
}

UINT64
SW_CONTEXT_CLASS::GetMinCycleToResume (void) const
{
    return(minCycleToResume);
}


UINT64
SW_CONTEXT_CLASS::GetFeederStreamID(void) const
{
    return(feederStreamID);
}


//Modifiers

void 
SW_CONTEXT_CLASS::SetResumptionPC (UINT64 pc) 
{ 
    resumption_PC = pc; 
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

void 
SW_CONTEXT_CLASS::SetInTimedWait (void)
{
    inTimedWait = true;
}

void
SW_CONTEXT_CLASS::SetNotInTimedWait (void)
{
    inTimedWait = false;
}

void
SW_CONTEXT_CLASS::SetMinCycleToResume (UINT64 cycle)
{
    minCycleToResume = cycle;
}



/* ------------------------ FeederHasEvents -----------------------------*/
/** 
 * Return true iff the feeder has new events pending for this context.
 * Default value for waituS is 0.
 */

bool
SW_CONTEXT_CLASS::FeederHasEvents (UINT32 waituS)
{
    bool stream_runnable;

    TRACE(Trace_Feeder,
          cout << "FeederHasEvents Checking on stream " << feederStreamID <<
          endl);

    // If stream is in a timed wait, we don't want to ask Mingo for
    // the next event. We're supposed to wait for the timeout.

    if (inTimedWait)
    {
        TRACE(Trace_Feeder,
              cout << "Stream is in timed wait. Minimum cycle to "
              << "resume is " << minCycleToResume << endl);
        return false;
    }

    if ((precedingInstCount != 0) || 
        (pendingOperation != ASIM_INST_NONE))
    {
        TRACE(Trace_Feeder,
              cout << "  Software context has buffered events" << endl);

        return true;
    }

    stream_runnable =
        MingoFeederHandle->GetNextSoftwareThreadEvent(feederStreamID, NULL, waituS);

    TRACE(Trace_Feeder,
          cout << "Returned from GetNextSoftwareThreadEvent "
               << (stream_runnable ? "(" : "(Not ")
               << "Runnable)"
               << endl);

    return stream_runnable;
}


/* ------------------------ DumpID --------------------------------------*/
/** 
 * Routine to print useful identifying information about a software
 * context. Call this instead of writing your own code. That way, as
 * we evolve the way of identifying a software context, only this
 * routine will have to change.
 *
 * For tracing, do this: 
 * TRACE(Trace_xxx, swc -> DumpID();
 * where xxx is the trace flag for the calling component. Substitute
 * the name of the variable that contains a pointer to an object of
 * type SW_CONTEXT_CLASS.
 */

void
SW_CONTEXT_CLASS::DumpID()
{
    cout << "Software context: Unique ID = " << uniqueID 
         << " Feeder's ID = " << feederStreamID
         << endl;

}

/* -------------------- DumpState ---------------------------------------*/
/** 
 * Routine to print state information about a software
 * context. Call this instead of writing your own code. That way, as
 * we evolve the way of identifying the state of a software context, only this
 * routine will have to change.
 *
 * Generally, the caller will probably call DumpID before calling this.
 *
 * For tracing, do this: 
 * TRACE(Trace_xxx, swc -> DumpState();
 * where xxx is the trace flag for the calling component. Substitute
 * the name of the variable that contains a pointer to an object of
 * type SW_CONTEXT_CLASS.
 */

void
SW_CONTEXT_CLASS::DumpState()
{
    if (mappedToHWC == true)
    {
        cout << "  Mapped to hardware context" << endl;

    }
    else
    {
        cout << "  Not mapped to hardware context" << endl;
    }
    

    if (GetRunnable())
    {
        cout << "  Software context is runnable" << endl;
    }
    else
    {
        cout << "  Software context is not runnable" << endl;
    }
    
    if (GetDeschedulePending())
    {
        cout << "  A deschedule is pending" << endl;
    }
    else
    {
        cout << "  No deschedule is pending" << endl;
    }

    if (GetDeletePending())
    {
        cout << "  A delete is pending" << endl;
    }
    else
    {
        cout << "  No delete is pending" << endl;
    }
    if (inTimedWait)
    {
        cout << "  In timed wait. Minimum cycle to resume is "
             << minCycleToResume << endl;
    }
    else
    {
        cout << "  Not in timed wait" << endl;
    }

    cout << "  Preceding instruction count = " << precedingInstCount
         << endl;
    
    if (pendingOperation != ASIM_INST_NONE)
    {
        cout << "  Pending Swarm operation is ";
        pendingOperation->DumpInfo();
    }
    else 
    {
        cout << "  There is no pending Swarm operation." << endl;
    }

    if (streamEnded)
    {
        cout << "  streamEnded is true" << endl;
    }
    else
    {
        cout << "  streamEnded is false" << endl;
    }
    
    if (proceedNow)
    {
        cout << "  proceedNow is true" << endl;
    }
    else
    {
        cout << "  proceedNow is false" << endl;
    }
    
    cout << "  Number of Swarm operations outstanding in timing model is "
         << outstandingOpCount << endl;
    
    DumpContextSwitchState();
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
    TRACE(Trace_Feeder,
          cout << "InitiateContextSwitch called for ");
    TRACE(Trace_Feeder, DumpID());

    if (contextSwitchState != NONE)
    {
        ASIMWARNING("SW_CONTEXT_CLASS::InitiateContextSwitch called when " 
                    << "current state is not NONE" <<
                    contextSwitchState << endl);
        TRACE(Trace_Feeder, DumpState());
    }
    
    contextSwitchState = REQUESTED;
}



/* -------------------------Fetch------------------------------------*/

/**
 * Get the next operation. Called by hw_context_class::Fetch
 * when the PM calls it requesting a new operation.
 *
 * 4/29/2002: Updated because scheduler's request for context switch
 * caused commit to call AllowSWTProgress when GetNextSWTEvent had
 * not been called.
 *
 * 4/17/2002: New scheme due to Joel. There can be multiple
 * outstanding SWARM_OPs. However, Mingo can't provide multiple
 * outstanding events. The algorithm for when to tell Mingo to proceed
 * is aimed at not allowing it to proceed an operation until that
 * operation would execute on the simulated machine. But since mingo
 * won't accept a second request for an event until the first has been
 * proceeded, we have to tell it to proceed when a second call to
 * Fetch is made while an operation is outstanding.
 *
 * We keep a count of outstanding SWARM_OPs. This is counted up
 * whenever we give Swarm a "real" operation, but not when we give it
 * an ANON or CONTEXT_SWITCH. Previous scheme (coded 4/17) counted
 * ANONs. It had a bug:
 * if Mingo returned a memory operation with some intervening
 * instructions, Fetch returned the first ANON, and Swarm called
 * Commit, Commit would call AllowSWTProgress before we had 
 * sent the
 * memory operation to Swarm. That is not what Joel wants. He wants
 * the thread to proceed after Swarm has said it can commit the operation.
 *
 * Calls to AllowSoftwareThreadProgress occur at the following times:
 *   In Fetch if, when we enter, we don't need to send a context
 *     switch, the thread hasn't terminated, we have no pending event,
 *     we have no intervening instructions, and outstandingOpCount is
 *     non-zero. We also call it when we receive a timed_wait event
 *     and the global flag tells us not to deschedule the stream. In
 *     that case, we do not call GetNextSWTEvent.
 *   In Fetch if we get an event that doesn't produce a SWARM_OP
 *     (regardless of outstandingOpCount)
 *   In Commit if we're not committing an ANON or CONTEXT_SWITCH, 
 *      outstandingOpCount is 0, and the thread hasn't exited.
 *
 */


ASIM_INST 
SW_CONTEXT_CLASS::Fetch(
    IADDR_CLASS ip)
{

    ASIM_INST temp_op;
    MINGO_DATA stream_event;
    bool success;
    
    // Report the state of things for debugging purposes

    TRACE(Trace_Feeder, 
          cout << "Fetch called for " );
    TRACE(Trace_Feeder, DumpID());
    TRACE(Trace_Feeder, DumpState());
    
    // The scheduler may have requested a context switch since this
    // routine was called last for this swc. If so, send
    // one and record that we did.
    // NOTE: We do this immediately, even though there may be
    // preceding instructions and even a pending operation. We don't
    // overwrite pendingOperation, so it will be sent the next time
    // the software context runs. If we want to send everything first,
    // and then do the context switch, this check will have to move
    // to where we have nothing to return and normally would call
    // GetNextSoftwareThreadEvent.

    if (contextSwitchState == REQUESTED)
    {
        temp_op = PrepareContextSwitch();
        return temp_op;
    }
        
    // See whether the stream is in a timed wait. The scheduler
    // controls the setting and clearing of the flag. If the flag is
    // set, the time has not expired. Don't return an operation in
    // that case. We make this check *after* the check for a requested
    // context switch because an involuntary context switch could be
    // requested for a stream that is in a timed wait

    if (inTimedWait)
    {
        TRACE(Trace_Feeder,
              cout <<"Returning no operation due to timed wait. "
              << "Minimum cycle to resume is " << minCycleToResume
              << endl);
        return ASIM_INST_NONE;
    }

    // If Mingo has already said that a thread terminated, don't try
    // to get more events from it. Expect the timing model to be able
    // to handle this.

    if (streamEnded == true)
    {
        return(ASIM_INST_NONE);
    }

    // THE COMMON CASE        
    // See whether we have anything waiting to send. If we do, send
    // something. If not, we have to ask Mingo for the next event. But
    // if there is an uncommitted SWARM_OP, we have to tell it to 
    // proceed first.


    if ((precedingInstCount != 0) || 
        (pendingOperation != ASIM_INST_NONE))
    {
        // We have something to send. Make the SWARM_OP and return it.

        temp_op = PrepareToSend();
        if (temp_op != ASIM_INST_NONE)
        {
            return temp_op;
        }
        else
        {
            // PrepareToSend should return something, either an
            // anonymous instruction or a real operation. 
            // DumpState will do a COUT. That's
            // OK because we are terminating anyway.

            DumpState();
            ASIMERROR("SW_CONTEXT_CLASS::Fetch: Didn't generate "
                  <<"Swarm operation after "
                  << "passing test saying something was pending" <<
                  endl);
        }
    }
    else
    {
        // We have nothing to send. Before asking for an event, tell
        // Mingo to proceed if there are any outstanding Swarm ops.

        if (outstandingOpCount > 0)
        {
            TRACE(Trace_Feeder,
                  cout << "Calling AllowSoftwareThreadProgress" << endl);
            MingoFeederHandle->AllowSoftwareThreadProgress(feederStreamID);
        }
    }
    
    // We got here because we had nothing to send to Swarm. 
    // Loop indefinitely, getting an event from Mingo and deciding what to
    // do with it. Exit whenever we can return a SWARM_OP, even if
    // it's a NULL one.

    while (1)
    {
        
        // Ask Mingo for an event.
 
        stream_event = GetFeederEvent();
        if (stream_event == NULL)
        {

            // We didn't get an event, despite telling Mingo it can
            // wait for a while. Most likely the thread has terminated
            // without telling Mingo. But we can't be sure. So treat
            // it like a voluntary pause, and start a context
            // switch. Note that this sequence is different from the
            // one where we received a syscall, in that it does not
            // call AllowSoftwareThreadProgress. That is because we
            // did not get an event from GetFeederEvent, so we can't
            // tell it to proceed. Also, it does not check for
            // intervening instructions, since we got nothing back
            // from Mingo.

            // If the thread wakes up again, it will send a message to 
            // Mingo. The scheduler will detect this in its periodic
            // check of non-runnable software contexts.

            // It is also possible that Mingo has exited. If so, the
            // scheduler's next call to EndOfData will indicate that,
            // and the scheduler will terminate the run.

            ASIMWARNING("SW_CONTEXT_CLASS::Fetch: " << 
                        "timed out waiting for event from "
                        << " stream " << feederStreamID << ". "
                        << " Marking it as not runnable." << endl);
            contextSchedulerHandle->SoftwareContextPaused(this);
            temp_op = PrepareContextSwitch();
            return temp_op;
            
        }
        
        // Look at the event and determine its type. Handle it
        // according to its type. See comments at the
        // start of ProcessStreamEvent. Then delete the object since
        // all useful data has been copied.

        ProcessStreamEvent(stream_event);
        delete stream_event;
  
        // If the event should trigger a context switch, send that.
        // This can happen when a thread pauses or when it exits. In
        // the latter case, threadEnded will be true, but we don't
        // need to check for that now. If another call comes in for
        // that SWC, we'll return a null SWARM_OP.
        // NOTE: We check this first. There may have been preceding
        // instructions. If so, they will be sent the next time the 
        // software context is run. If we want to send them, we need to move
        // this check to the point where we would call GetNextSWTEvent.

        if (contextSwitchState == SENDING)
        {
            temp_op = PrepareContextSwitch();
            // *** Trying to fix context switch. Since we are not
            // counting these, need to tell Mingo to proceed now.
            TRACE(Trace_Feeder,
                  cout << "Calling AllowSoftwareThreadProgress after " 
                  << "thread paused or ended" << endl);
            MingoFeederHandle->AllowSoftwareThreadProgress(feederStreamID);
            // ***            

            return temp_op;
        }
            
        // proceedNow will be set if the stream event was not meant for
        // the CPU model. For example, it sets the preferred CPU for the
        // thread. Such events go to the scheduler. Tell Mingo to
        // proceed so that we can get an event to return to Swarm.

        if (proceedNow)
        {
            TRACE(Trace_Feeder,
                  cout << "Calling AllowSoftwareThreadProgress after " 
                  << "receiving event not meant for Swarm" << endl);
            MingoFeederHandle->AllowSoftwareThreadProgress(feederStreamID);
            proceedNow = false;

            // If timed wait does not cause a deschedule, then we need
            // to return control to the PM and give it no
            // operation. We don't want to ask Mingo for another one
            // until the time expires.

            if (receivedTimedWait)
            {
                TRACE(Trace_Feeder,
                      cout << "Returning no operation due to timed "
                      << "wait" << endl);
                receivedTimedWait = false;
                return ASIM_INST_NONE;
            }
            
        }
        
        else
        {
            // proceedNow is false. There should be a pending operation
            // and maybe some preceding instructions. Make a SWARM_OP.
            // If there is nothing at this stage, something unexpected is 
            // going on, and we'll need to understand what could cause it.

            temp_op = PrepareToSend();
            if (temp_op != ASIM_INST_NONE)
            {
                return temp_op;
            }
            else
            {

                // We got an event from Mingo, and proceedNow is
                // false. Yet PrepareToSend did not generate an
                // operation to send. There is a flaw
                // in the interface between the lower-level routines and this
                // one. For now, get another event. This may
                // eventually lead to a crash.

                ASIMWARNING("SW_CONTEXT_CLASS::Fetch: After call to "
                    << " GetNextSoftwareThreadEvent, there was "
                      << "nothing to do" << endl);
                continue;
            }
        } // proceedNow is false
    } // while loop
}





/* ----------------- Commit  --------------------------------*/

/**
 * Called by ASIM_INST_CLASS::Commit when the timing model calls it.
 * Since ASIM_INST is a memory-managed object, the caller must set its
 * pointer to NULL when the object is no longer needed. This code does
 * not have to do that, because its pointer is temporary.


 */

void
SW_CONTEXT_CLASS::Commit(
    ASIM_INST committedOp)
 
{
    ASIM_INST_TYPE opType;
    
    // Report the state of things for debugging purposes

    TRACE(Trace_Feeder,
          cout << "SW_CONTEXT_CLASS::Commit called for " << endl);
    TRACE(Trace_Feeder, DumpID());
    TRACE(Trace_Feeder, DumpState());
    TRACE(Trace_Feeder, 
          cout << "Committing ");
    TRACE(Trace_Feeder, committedOp->DumpInfo());

    // See what kind of SWARM_OP this is (definitions are in swarm.h)
    
    opType = committedOp->GetOpType();

    // We don't count anonymous instructions as outstanding, because we
    // don't want to allow the thread to proceed until we have
    // committed the real operation that the anonymous instruction precedes.

    if (opType == ANONYMOUS)
    {
        return;
    }
    
    // If this is a context switch, record that it was committed.

    if (opType == CONTEXT_SWITCH)
    {
        contextSwitchState = COMMITTED;
    }

    else 
    {
        // *** Trying to fix context switch. Don't count them down,
        // but do call scheduler if committing one and there are no 
        // outstanding operations.
        outstandingOpCount--;
    }
    
    // If there are more outstanding operations, don't do anything
    // now. Wait until the count goes to zero.

    if (outstandingOpCount > 0)
    {
        return;
    }

    // Make sure we didn't go below zero. We've already checked for
    // positive, so the value should be zero.

    ASSERT(outstandingOpCount == 0,
        "ERROR: outstandingOpCount is  negative: " 
        << outstandingOpCount << endl);

    // We have no outstanding operation. Unless the thread
    // exited, tell Mingo to let it proceed.

    // *** Trying to fix context switch. Don't call Allow if committing a
    // context switch

    if (!streamEnded && (opType != CONTEXT_SWITCH))
    {
        TRACE(Trace_Feeder,
            cout << "Calling AllowSoftwareThreadProgress after " 
              << "committing operation." << endl);
        
        MingoFeederHandle->AllowSoftwareThreadProgress(feederStreamID);
    }
    
    // If a context switch has been committed (now or in a previous
    // call), tell the scheduler that the context switch is complete.
    // Reset the state so that the next time it runs, it won't look
    // like it is doing a context switch.

    if (contextSwitchState == COMMITTED)
    {
        contextSwitchState = NONE;    
        TRACE(Trace_Feeder,
              cout << "Calling ContextSwitchCompleted" << endl);
        contextSchedulerHandle->ContextSwitchCompleted(this);
    }

}


/************ Start of Private Member Functions ***************************/

/* ---------------- GetFeederEvent ---------------------------------------*/


MINGO_DATA
SW_CONTEXT_CLASS::GetFeederEvent(void)
{
    bool success = false;
    MINGO_DATA stream_event;
    INT32 timeout = FEEDER_TIMEOUT_VALUE;
    stream_event = new MINGO_DATA_CLASS;

// There used to be a timeout loop here.    
// Michael's V2 handles waiting for the next event. Therefore if we
// set the timeout high enough, a failure would indicate something
// seriously wrong. For now, pass the failure to the caller, which
// will decide what to do.

    TRACE(Trace_Feeder,
          cout << "Calling "
               << "MINGO_FEEDER_CLASS::GetNextSoftwareThreadEvent"
               << endl);
    
    success = MingoFeederHandle ->
        GetNextSoftwareThreadEvent
           (feederStreamID, stream_event, FEEDER_TIMEOUT_VALUE);

    if (success)
    {
        return (stream_event);
    }
    
    // We timed out.

    delete stream_event;
    stream_event = NULL;
    return (stream_event);
    
}



/* ---------------- ProcessStreamEvent ------------------------------------*/

/**
 * Java reader has given us a Java Event. Find out the event type and
 * call the appropriate routine
 * 
 * Assumptions made by Fetch about what this routine will do.
 *   If the Java Event included intervening instructions,
 *     pendingInstCount will be incremented by the number of them.
 *   If the Java Event included an operation that Swarm
 *     understands (or that it should eventually understand) 
 *     the SWARM_OP for that operation has been created
 *     and pendingOperation points to it.
 *   If the Java Event indicated a thread ended, streamEnded has been
 *      set to true, the scheduler has been notified, and 
 *      ContextSwitchState has been set to SENDING.
 *   If the Mingo Event indicated a thread paused (voluntarily gave up
 *      the CPU), the scheduler has been notified, and
 *      ContextSwitchState has been set to SENDING.
 *   For any event that does not generate a SWARM_OP or increment
 *      pendingInstCount, proceedNow will have been set. These include
 *      events that are meant only for the scheduler. The scheduler
 *      will have been notified.
 *   If the Mingo event is a timed wait, one of two things has
 *     happened:
 *     1) if TIMED_WAIT_CAUSES_UNSCHEDULE is false, proceedNow will be
 *        true (to cause a call to AllowSWTEvent), and
 *        receivedTimedWait will be true (to prevent a call to
 *        GetNextSWTEvent and cause return of ASIM_INST_NONE to the
 *        PM). The scheduler will have been called, so the swc will be
 *        marked as being in timed wait.
 *     2) If TIMED_WAIT_CAUSES_UNSCHEDULE is true, ContextSwitchState
 *        will be set to SENDING, and the scheduler will have been
 *        called. This will have caused the swc to be marked as in a
 *        timed wait and not runnable.
 * 
 * When this routine returns, the MINGO_DATA_CLASS object may be deleted.  
 */

void
SW_CONTEXT_CLASS::ProcessStreamEvent(MINGO_DATA event)
{
    MINGO_MSG event_type;
    event_type = event->get_msg();

    TRACE(Trace_Feeder,
          cout << "SW_CONTEXT_CLASS::ProcessStreamEvent received "
          << "event number " << event_type <<endl);
    TRACE(Trace_Feeder, event->DumpPacket(stdout));

    switch(event_type)
    {
      case MINGO_MSG_THREAD_END:
        TRACE(Trace_Feeder, cout << "Handling MINGO_MSG_THREAD_END" <<
              endl);
        HandleEndThread(event);
        break;
      case MINGO_MSG_THREAD_PRIORITY:
        TRACE(Trace_Feeder, cout << "Handling MINGO_MSG_THREAD_PRIORITY" <<
              endl);
        HandleSetPriority(event);
        break;
      case MINGO_MSG_PREFERRED_CPU:
        TRACE(Trace_Feeder, cout << "Handling MINGO_MSG_PREFERRED_CPU" <<
              endl);
        HandleSetPreferredCPU(event);
        break;
      case MINGO_MSG_MEMORY_READ:
        TRACE(Trace_Feeder, cout << "Handling MINGO_MSG_MEMORY_READ" <<
              endl);
        HandleMemoryRead(event);
        break;
      case MINGO_MSG_MEMORY_WRITE:
        TRACE(Trace_Feeder, cout << "Handling MINGO_MSG_MEMORY_WRITE" <<
              endl);
        HandleMemoryWrite(event);
        break;
      case MINGO_MSG_EXCHANGE:
        TRACE(Trace_Feeder, cout << "Handling MINGO_MSG_EXCHANGE" <<
              endl);
        HandleExchange(event);
        break;
      case MINGO_MSG_MEMORY_FENCE:
        TRACE(Trace_Feeder, cout << "Handling MINGO_MSG_MEMORY_FENCE" <<
              endl);
        HandleMemoryFence(event);
        break;
      case MINGO_MSG_DEPENDENT_READ:
        TRACE(Trace_Feeder, cout << "Handling MINGO_MSG_DEPENDENT_READ" <<
              endl);
        HandleDependentOp(event);
        break;
      case MINGO_MSG_BRANCH:
        TRACE(Trace_Feeder, cout << "Handling MINGO_MSG_BRANCH" <<
              endl);
        HandleBranch(event);
        break;
      case MINGO_MSG_SYSCALL:
        TRACE(Trace_Feeder, cout << "Handling MINGO_MSG_SYSCALL" <<
              endl);
        HandleWait(event);
        break;
      case MINGO_MSG_TIMED_WAIT:
        TRACE(Trace_Feeder, cout << "Handling MINGO_TIMED_WAIT" <<
              endl);
        HandleTimedWait(event);
        break;

      // Defined types that we don't expect here:
      //   MINGO_MSG_NONE (causes GetNextSoftwareThreadEvent to return false)
      //   MINGO_MSG_EXIT (causes EndOfData to return true)
      //   MINGO_MSG_THREAD_START (causes CheckForNewSoftwareThread to return  
      //     non-zero thread ID)

      default:
        ASIMWARNING("SW_CONTEXT_CLASS::ProcessStreamEvent: "
                   <<"Received unexpected Mingo event type: "
                   << event_type << endl;)
        HandleUnexpectedEvent(event);
        break;
    }
}

/*---------------------- HandleMemoryRead -------------------------------*/

/**
 * Mingo has given us a memory read. Make a Swarm operation for it, and
 * record the count of anonymous instructions that we have to feed to
 * Swarm before giving it the operation.
 */

void
SW_CONTEXT_CLASS::HandleMemoryRead(MINGO_DATA event)
{
    ASIM_INST swarm_op;                //contains operation passed to PM

    UINT64 mingo_ip;
    IADDR_CLASS asim_ip;
    UINT64 mingo_effective_addr;
    UINT32 mingo_access_size;
    UINT32 asim_access_size;
    bool mingo_lock_request;
    MINGO_REG mingo_target_register;
    UINT32 asim_target_register;
    MINGO_DATA_CLASS::LDTYPE mingo_load_type;
    
        
    // Make a Swarm operation, which will be sent eventually.

    swarm_op = new ASIM_INST_CLASS(MEMORY_READ, this);

    // Get IP. Convert from Mingo's type to Asim's type for
    // instruction addresses.

    mingo_ip = event->get_ip();
    asim_ip.Set(mingo_ip,0);
    
    swarm_op->SetFeederIP(asim_ip);

    // Get effective address. Mingo and Asim agree on type, so use Mingo's

    mingo_effective_addr = event->get_va();
    swarm_op->SetEffectiveAddress(mingo_effective_addr);

    // Get access size. Mingo records size as a power of 2. Asim
    // records the actual value.

    mingo_access_size = event->get_access_size();
    asim_access_size = 1 << mingo_access_size;
    swarm_op->SetAccessSize(asim_access_size);
    
    // Now that we have ea and access size, compute the byte mask.

    swarm_op->ComputeByteMask();
    
    // Find out if this is a lock request, and pass it on.

    mingo_lock_request = event->get_is_lock_request();
    swarm_op->SetIsLockRequest(mingo_lock_request);
    
    // Get target register. Convert from Mingo's type to Asim's type

    mingo_target_register = event->get_target_register();
    asim_target_register = mingo_target_register;
    swarm_op->SetReadTargetRegister(asim_target_register);
    
    // Get load type. For now we care only about acquire

    mingo_load_type = event->get_load_type();
    if (mingo_load_type == MINGO_DATA_CLASS::LDTYPE_ACQ)
    {
        swarm_op->SetReadIsAcquire(true);
    }
    

    // Record the count of preceding instructions, and save the new
    // operation until we are ready to send it.

    precedingInstCount += event->get_instruction_count();
    pendingOperation = swarm_op;
    
}

/*---------------------- HandleMemoryWrite -------------------------------*/

/**
 * Mingo has given us a memory write. Make a Swarm operation for it, and
 * record the count of anonymous instructions that we have to feed to
 * Swarm before giving it the operation.
 */

void 
SW_CONTEXT_CLASS::HandleMemoryWrite(MINGO_DATA event)
{
    
    ASIM_INST swarm_op;

    UINT64 mingo_ip;
    IADDR_CLASS asim_ip;
    UINT64 mingo_effective_addr;
    UINT32 mingo_access_size;
    UINT32 asim_access_size;
    bool mingo_lock_request;
    bool is_store_release;
    

    // Make a Swarm operation, which will be sent eventually.

    swarm_op = new ASIM_INST_CLASS(MEMORY_WRITE, this);

    // Get IP. Convert from Mingo's type to Asim's type for
    // instruction addresses.

    mingo_ip = event->get_ip();
    asim_ip.Set(mingo_ip,0);
    
    swarm_op->SetFeederIP(asim_ip);

    // Get effective address. Mingo and Asim agree on type, so use Mingo's

    mingo_effective_addr = event->get_va();
    swarm_op->SetEffectiveAddress(mingo_effective_addr);

    // Get access size. Mingo records size as a power of 2. Asim
    // records the actual value.

    mingo_access_size = event->get_access_size();
    asim_access_size = 1 << mingo_access_size;
    swarm_op->SetAccessSize(asim_access_size);
    
    // Now that we have ea and access size, compute the byte mask.

    swarm_op->ComputeByteMask();
    
    // Find out if this is a lock request, and pass it on.

    mingo_lock_request = event->get_is_lock_request();
    swarm_op->SetIsLockRequest(mingo_lock_request);

    is_store_release = event->get_is_store_release();
    swarm_op->SetWriteIsRelease(is_store_release);
 
    // Record the count of preceding instructions, and save the new
    // Swarm operation until we are ready to send it.

    precedingInstCount += event->get_instruction_count();
    pendingOperation = swarm_op;
}

/*---------------------- HandleExchange -------------------------------*/

/**
 * Mingo has given us an exchange. 
 * TEMPORARY: The processor doesn't know what to do with this, so
 * don't send it. Treat this like a message meant for the scheduler,
 * setting proceedNow so that Fetch will ask Mingo for the
 * next event
 */

void 
SW_CONTEXT_CLASS::HandleExchange(MINGO_DATA event)
{

    ASIM_INST swarm_op;                //contains operation passed to PM

    UINT64 mingo_ip;
    IADDR_CLASS asim_ip;
    UINT64 mingo_effective_addr;
    bool is_compare;
    UINT32 mingo_access_size;
    UINT32 asim_access_size;
    MINGO_REG mingo_target_register;
    UINT32 asim_target_register;
    bool is_acquire;
    bool is_release;
    UINT64 compare_value;
    
    // Make a Swarm operation, which will be sent eventually.

    swarm_op = new ASIM_INST_CLASS(EXCHANGE, this);

    // Get IP. Convert from Mingo's type to Asim's type for
    // instruction addresses.

    mingo_ip = event->get_ip();
    asim_ip.Set(mingo_ip,0);
    swarm_op->SetFeederIP(asim_ip);

    // Get effective address. Mingo and Asim agree on type, so use Mingo's

    mingo_effective_addr = event->get_va();
    swarm_op->SetEffectiveAddress(mingo_effective_addr);

    // Get access size. Mingo records size as a power of 2. Asim
    // records the actual value.

    mingo_access_size = event->get_access_size();
    asim_access_size = 1 << mingo_access_size;
    swarm_op->SetAccessSize(asim_access_size);
    
    // Now that we have ea and access size, compute the byte mask.

    swarm_op->ComputeByteMask();
    
    // Record whether this is compare-and-exchange (vs. exchange)

    is_compare = event->get_is_compare();
    swarm_op->SetExchangeIsCompare(is_compare);
    
    // if it is cmpxchng, record the compare value

    if (is_compare)
    {
        compare_value = event->get_compare_value();
        swarm_op->SetCompareValue(compare_value);
    }

    // Get target register. Convert from Mingo's type to Asim's type

    mingo_target_register = event->get_target_register();
    asim_target_register = mingo_target_register;
    swarm_op->SetReadTargetRegister(asim_target_register);
    
    // Find out whether this is an aquire request.

    is_acquire = event->get_is_acquire();
    swarm_op->SetReadIsAcquire(is_acquire);
    
    // Find out whether this is a release request.

    is_release = event->get_is_release();
    swarm_op->SetWriteIsRelease(is_release);
     

    // Record the count of preceding instructions, and save the new
    // Swarm operation until we are ready to send it.
    // TEMPORARY: Make these instructions real when one-stage supports 
    // exchange. The ASIM_INST is memory-managed, so it will go away
    // when this routine returns.

    precedingInstCount += event->get_instruction_count();
    pendingOperation = swarm_op;

}


/*---------------------- HandleMemoryFence -------------------------------*/

/**
 * Mingo has given us a memory fence. Make a Swarm operation for it, and
 * record the count of anonymous instructions that we have to feed to
 * Swarm before giving it the operation.
 */

void 
SW_CONTEXT_CLASS::HandleMemoryFence(MINGO_DATA event)
{
    ASIM_INST swarm_op;
    UINT64 mingo_ip;
    IADDR_CLASS asim_ip;
    MINGO_MEMORY_FENCE mingo_fence_type;
    FENCE_TYPE asim_fence_type;

    swarm_op = new ASIM_INST_CLASS(MEMORY_FENCE, this);

    // Get IP. Convert from Mingo's type to Asim's type for
    // instruction addresses.

    mingo_ip = event->get_ip();
    asim_ip.Set(mingo_ip,0);
    swarm_op->SetFeederIP(asim_ip);

    // Get type of fence. Convert from Mingo's type to Asim's type.

    mingo_fence_type = event->get_memory_fence_type();
    switch (mingo_fence_type)
    {
      case MINGO_FENCE_ORDERING:
        asim_fence_type = FENCE_ORDERING;
        break;
      case MINGO_FENCE_ACCEPTANCE:
        asim_fence_type = FENCE_ACCEPTANCE;
        break;
      default:
        //Had to do this because compiler thought it was uninitialized.
        asim_fence_type = FENCE_ORDERING;
        
        ASIMERROR("SW_CONTEXT_CLASS:HandleMemoryFence: Mingo passed "
                  <<"unexpected memory fence type" <<
                  mingo_fence_type);
        
    }
    swarm_op->SetFenceType(asim_fence_type);
    
    // Record the count of preceding instructions, and save the new
    // Swarm operation until we are ready to send it.

    precedingInstCount += event->get_instruction_count();
    pendingOperation = swarm_op;

}

/*---------------------- HandleDependentOp -------------------------------*/

/**
 * Mingo has given us a dependent operation. Make a Swarm operation
 * for it, and record the count of anonymous instructions that we have
 * to feed to Swarm before giving it the operation.
 */

void 
SW_CONTEXT_CLASS::HandleDependentOp(MINGO_DATA event)
{
    ASIM_INST swarm_op;
    UINT64 mingo_ip;
    IADDR_CLASS asim_ip;
    MINGO_REG mingo_source_register;
    UINT32 asim_source_register;

    swarm_op = new ASIM_INST_CLASS(DEPENDENT_OP, this);

    // Get IP. Convert from Mingo's type to Asim's type for
    // instruction addresses.

    mingo_ip = event->get_ip();
    asim_ip.Set(mingo_ip,0);
    swarm_op->SetFeederIP(asim_ip);

    // Get source register. Convert from Mingo's type to Asim's type
    // for registers

    mingo_source_register = event->get_source_register();
    asim_source_register = mingo_source_register;
    swarm_op->SetDependentSourceRegister(asim_source_register);
    
    // Record the count of preceding instructions, and save the new
    // Swarm operation until we are ready to send it.

    precedingInstCount += event->get_instruction_count();
    pendingOperation = swarm_op;
}


/*---------------------- HandleBranch -------------------------------*/

/**
 *
 * Mingo has given us a branch. Make a Swarm operation for it, and
 * record the count of anonymous instructions that we have to feed to
 * Swarm before giving it the operation.
 */

void 
SW_CONTEXT_CLASS::HandleBranch(MINGO_DATA event)
{
    ASIM_INST swarm_op;
    UINT64 mingo_ip;
    IADDR_CLASS asim_ip;
    UINT64 mingo_target_ip;
    IADDR_CLASS asim_target_ip;
    MINGO_REG mingo_target_register;
    UINT32 asim_target_register;
    MINGO_CONTROL_TRANSFER mingo_branch_type;
    CONTROL_TRANSFER_TYPE asim_branch_type;
    bool indirect_branch;

    swarm_op = new ASIM_INST_CLASS(BRANCH, this);

    // Get IP. Convert from Mingo's type to Asim's type for
    // instruction addresses.

    mingo_ip = event->get_ip();
    asim_ip.Set(mingo_ip,0);
    swarm_op->SetFeederIP(asim_ip);

    // Get target IP. Convert from Mingo's type to Asim's type for
    // instruction addresses.

    mingo_target_ip = event->get_branch_target_ip();
    asim_target_ip = mingo_target_ip;
    swarm_op->SetBranchTargetIP(asim_target_ip);

    // Get branch target register. Convert from Mingo's type to Asim's
    // type for registers.

    mingo_target_register = 
        event->get_branch_target_register();
    asim_target_register = mingo_target_register;
    swarm_op->SetBranchTargetRegister(asim_target_register);
    
    // Get branch type. Convert from Ming's type to Asim's type.

    mingo_branch_type = event->get_branch_type();
    switch (mingo_branch_type)
    {
      case MINGO_XFER_BRANCH:
        asim_branch_type = TRANSFER_BRANCH;
        break;
      case MINGO_XFER_CALL:
        asim_branch_type = TRANSFER_CALL;
        break;
      case MINGO_XFER_RETURN:
        asim_branch_type = TRANSFER_RETURN;
        break;
      default:
        //Had to do this because compiler thought this was uninitialized.
        asim_branch_type = TRANSFER_BRANCH;
        ASIMERROR("SW_CONTEXT_CLASS::HandleBranch: Mingo passed "
                  <<" unexpected branch type" <<
                  mingo_branch_type);
    }
    swarm_op->SetBranchType(asim_branch_type);
    
    // Find out if this is a register indirect branch, and pass it on.

    indirect_branch = event->get_branch_is_register_indirect();
    swarm_op->SetBranchIsRegisterIndirect(indirect_branch);
    
    // Record the count of preceding instructions, and save the new
    // Swarm operation until we are ready to send it.

    precedingInstCount += event->get_instruction_count();
    pendingOperation = swarm_op;
}

/*---------------------- HandleWait -------------------------------*/

/**
 * Java reader has said that a thread is waiting.
 * Tell the scheduler. Set contextSwitchState so that Fetch will
 * know to send a context switch.
 */

void 
SW_CONTEXT_CLASS::HandleWait(MINGO_DATA event)
{

    precedingInstCount += event->get_instruction_count();
    contextSwitchState = SENDING;
    contextSchedulerHandle -> SoftwareContextPaused(this);
}

/* ---------------------- HandleTimedWait--------------------------
 */

/**
 * Mingo has said that a thread is waiting for a specified time.
 * Depending on a global flag, this may require descheduling. If so, 
 * treat this like a syscall. 
 * Otherwise, mark it as waiting, but don't deschedule it.
 */

void
SW_CONTEXT_CLASS::HandleTimedWait(MINGO_DATA event)
{
    UINT64 time_in_ns;
    time_in_ns = event->get_timed_wait();
    TRACE(Trace_Feeder, 
          cout <<"Requested wait of " << time_in_ns
          << " nanoseconds" << endl);

    if (TIMED_WAIT_CAUSES_UNSCHEDULE)
    {
        // We want the scheduler to deschedule this stream. Set the
        // state so that Fetch will return a context
        // switch. Then tell the scheduler to start timing the wait
        // and deschedule (the 3rd argument requests the deschedule).

        if (contextSwitchState == NONE)
        {
            TRACE(Trace_Feeder,
                  cout << "Starting context switch for timed wait" << endl);
            contextSwitchState = SENDING;
            contextSchedulerHandle->StartTimedWait(this, time_in_ns, true);
        }
        else
        {
            // If this request comes in while we are doing a context
            // switch, we can't change the state. Since we don't ask
            // Mingo for events during a context switch, this isn't
            // expected to happen. If it does, we need to figure out
            // how to work around it. For now, drop the event by
            // signaling Fetch to allow the thread to
            // proceed. It will call Mingo for the next event as if
            // this one had not happened. This is the way that we
            // handle unsupported event types.

            // TO DO: Figure out whether this case can arise, and how
            // to handle it. Check other cases that modify
            // contextSwitchState for similar problems. In theory, we
            // should not be calling GetNextSWTEvent if we are in the
            // middle of a context switch, so we shouldn't have a problem.

            ASIMWARNING("Received timed_wait while context switch in "
                        << "progress. ContextSwitchState is "
                        << contextSwitchState << endl);
            proceedNow = true;
        }
    }
    else
    {
        // We don't want the scheduler to deschedule this stream. But
        // we do want it to time the wait. Set proceedNow to signal
        // the caller to tell Mingo to proceed. Set receivedTimedWait
        // to signal the caller to return no operation and NOT call
        // GetNextSWTEvent

        TRACE(Trace_Feeder,
              cout << "Starting timed wait without descheduling" <<
              endl);
        proceedNow = true;
        receivedTimedWait = true;

        contextSchedulerHandle->StartTimedWait(this, time_in_ns, false);
    }

}


/*---------------------- HandleEndThread -------------------------------*/

/**
 * Java reader has said that a thread was terminated.
 * Tell the scheduler. Set contextSwitchState so that Fetch will
 * know to send a context switch.
 * 
 */


void 
SW_CONTEXT_CLASS::HandleEndThread(MINGO_DATA event)
{    
    contextSwitchState = SENDING;
    contextSchedulerHandle -> SoftwareContextExited(this);
    streamEnded = true;
}

/*---------------------- HandleSetPriority  -----------------------------*/

/**
 * Mingo has said that a thread is setting its scheduling priority
 * At the moment the scheduler wouldn't know what to do with this. 
 * Future enhancement: Add a call to a scheduler routine.
 *
 */

void
SW_CONTEXT_CLASS::HandleSetPriority(MINGO_DATA event)
{
    INT32 priority;
    priority = event->get_priority();
    proceedNow = true;

}


/*---------------------- HandleSetPreferredCPU --------------------------*/

/**
 * Mingo has said that a thread is setting its preferred CPU.
 * At the moment the scheduler wouldn't know waht to do with this. 
 * Future enhancement: Add a call to a scheduler routine.
 */

void
SW_CONTEXT_CLASS::HandleSetPreferredCPU(MINGO_DATA event)
{
    INT32 cpu;
    cpu = event->get_cpu_id();    
    proceedNow = true;
}

/* --------------------- HandleUnexpectedEvent --------------------------- */

/**
 * Mingo has sent an event that sw_context is not prepared to handle.
 * Indicate that we should let the thread proceed, and skip over this
 * event.
 */

void
SW_CONTEXT_CLASS::HandleUnexpectedEvent(MINGO_DATA event)
{
    proceedNow = true;
}


/* --------------------- PrepareContextSwitch -------------------------------*/

/**
 * Make a Swarm operation representing a context switch. Increment count of
 * outstanding operations, and update contextSwitchState
 */

ASIM_INST
SW_CONTEXT_CLASS::PrepareContextSwitch(void)
{

    ASIM_INST temp_op;
    
    temp_op = new ASIM_INST_CLASS(CONTEXT_SWITCH,this);
// *** Trying to fix context switch problem. Don't count these up
// because we don't want to call Allow when they commit.

//    outstandingOpCount++;
    contextSwitchState = SENT;
    return temp_op;
}

/* ----------------- PrepareToSend ------------------------------------*/

/**
 * Called by Fetch when it believes it has either preceding
 * instructions or a pending operation. Make a SWARM_OP with an
 * ANONYMOUS op code if we have a preceding instruction. Otherwise, 
 * return the pendingOperation that we already have.
 *
 * This is a routine because it is
 * called from two places in Fetch . I'd rather have it all in one
 * piece of code, but I couldn't find a quick way to do that.
 */

ASIM_INST
SW_CONTEXT_CLASS::PrepareToSend(void)
{

    ASIM_INST temp_op = ASIM_INST_NONE;
    

    if (precedingInstCount > 0)
    {
        // There is at least one intervening anonymous
        // instruction. Send it, but do not count up the number of outstanding
        // Swarm operations.

        TRACE(Trace_Feeder,
              cout << "Sending an anonymous instruction" << endl);
        temp_op = new ASIM_INST_CLASS(ANONYMOUS, this);
        precedingInstCount --;
        return temp_op;
        }

    // No intervening instructions. See if we created a Swarm
    // operation. If so, send it, and mark that we don't have another
    // one pending for the next time this is called.

    if (pendingOperation != ASIM_INST_NONE)
    {

        TRACE(Trace_Feeder,
              cout << "Sending ");
        TRACE(Trace_Feeder, pendingOperation->DumpInfo());
        
        temp_op = pendingOperation;
        pendingOperation = ASIM_INST_NONE;
        outstandingOpCount++;
        return temp_op;
    }

    // We shouldn't get here. Let the caller decide what to do.

    return temp_op;

}

/* -------------- DumpContextSwitchState --------------------------------*/

/** Routine to print context switch state information for a software
 * context. Called from DumpState.
 *
 * Generally, the caller will probably call DumpID before calling this.
 */
void
SW_CONTEXT_CLASS:: DumpContextSwitchState(void)
{
    
    cout << "  Context Switch State is ";
    switch (contextSwitchState)
    {
      case NONE:
        cout << "NONE";
        break;
      case REQUESTED:
        cout << "REQUESTED";
        break;
      case SENDING:
        cout << "SENDING";
        break;
      case SENT:
        cout << "SENT";
        break;
      case COMMITTED:
        cout << "COMMITTED";
        break;
      default:
        cout << "*** unrecognized ***";
    }
    cout << endl;
}

