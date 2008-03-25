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
 * @author David Goodwin, Michael Adler
 * @brief Instruction feeder interface definition.
 */

//#ifndef _INSTFEEDERBASE_
//#define _INSTFEEDERBASE_

#include <vector>

// ASIM core
#include "asim/syntax.h"
#include "asim/alphaops.h"
#include "asim/disasm.h"
#include "asim/arch_register.h"
#include "asim/stateout.h"
#include "asim/memory_reference.h"


//----------------------------------------------------------------
// Anonymous handles
//----------------------------------------------------------------

typedef class WARMUP_CLIENTS_CLASS *WARMUP_CLIENTS;
typedef class WARMUP_INFO_CLASS *WARMUP_INFO;

//
// Handles specifying individual instruction streams (threads) inside
// a feeder are private to the feeder.  In order to export them,
// pass them around, and use them in this generic feeder class the
// private handle must be cast to a handle inside the feeder.
// 
typedef void* IFEEDER_STREAM_HANDLE;

/********************************************************************
 *
 * Feeder interface. The following function represent the interface
 * to the instruction feeders.
 */

//typedef class IFEEDER_BASE_CLASS *IFEEDER_BASE;

//
// Anonymous thread handle used by IFEEDER's thread control code.
// See thread management section of IFEEDER_BASE_CLASS.
//
typedef class IFEEDER_THREAD_CLASS *IFEEDER_THREAD;

//
// asimthread.h and instfeederbase.h are interdependent.  This must follow the
// handle declarations above.
//
#include "asimthread.h"

/********************************************************
 *
 *We have a very wierd include path to allow for inline functions in the
 *software context and hardware context. These both call feeder functions, but
 *they are also call by the instruction. So if we include isa here we end up
 *with a circular dependency. Instead we will include the software context and
 *let it call the isa.h file and then define the inst feederbase. Note that the
 *software context has an undefine because the synthesized header file also has
 *#define protections around a particular provides
 *
 *********************************************************/
#include "asim/provides/software_context.h"


#include "feedersystemevent.h"

#ifndef _INSTFEEDERBASE_
#define _INSTFEEDERBASE_

//----------------------------------------------------------------
//This is just a place holder for future work that may want to specify
//what gets dumped out and the file format.
//----------------------------------------------------------------

enum FUNCTIONAL_MODEL_DUMP_TYPE
{
   FUNCTIONAL_MODEL_DUMP_ALL_STATE,
   FUNCTIONAL_MODEL_DUMP_LAST
};

class IFEEDER_BASE_CLASS : public TRACEABLE_CLASS
{
    friend class IFEEDER_THREAD_CLASS;

  public:
    //----------------------------------------------------------------
    // Feeder type.  The type mainly indicates the fetch options
    // available.
    //----------------------------------------------------------------

    enum FEEDER_TYPE
    {
        IFEED_TYPE_UNKNOWN,             // Older feeders don't set a value
        IFEED_TYPE_MACRO,
        IFEED_TYPE_MICRO
    };

    //----------------------------------------------------------------
    // macro feeder type -- A bad idea
    //    Here a macro feeder can declare itself a specific type.  Why
    //    should the model above care?  If there really is something
    //    the model needs to know it should really be communicated in
    //    some generic way, instead of special casing each feeder.
    //    Right now only Archlib uses this.  Let's keep it that way and
    //    change Archlib.
    //
    enum MACRO_FEEDER_TYPE
    {
        IFEED_MACRO_UNKNOWN,
        IFEED_MACRO_LIT,
        IFEED_MACRO_SOFTSDV,
        IFEED_MACRO_NOP,
        IFEED_MACRO_SYNTHETIC,
        IFEED_MACRO_TRACE
    };


    // Constructor --
    //   A subfeeder is not added to the list of active feeders since
    //   its master will already be in it.
    //
    IFEEDER_BASE_CLASS(
        char *name = NULL,
        IFEEDER_BASE parentFeeder = NULL,
        FEEDER_TYPE fType = IFEED_TYPE_UNKNOWN);

    virtual ~IFEEDER_BASE_CLASS();
    
  public:

    //----------------------------------------------------------------
    // Feeder capability management.  A series of boolean descriptors
    // for operations conditionally supported by feeders.
    //----------------------------------------------------------------

    enum CAPABILITIES
    {
        IFEED_FAST_FORWARD,             // Mark/skip methods supported
        IFEED_SYMBOL_LOOKUP,            // Symbol lookup supported
        IFEED_GLOBAL_SYMBOLS,           // Symbol() with no thread handle allowed
        IFEED_MEM_CTL,                  // Load/store controllable
        IFEED_REGISTER_VALUES,          // Register values available
        IFEED_MEMORY_VALUES,            // Memory values associated with an instr
        IFEED_MEMORY_PEEK,              // Random access to memory, no instr needed
        IFEED_EXECUTE,                  // feeder can actually execute the
                                        // instruction
        IFEED_SYSTEM_EVENTS,            //Feeder can be probed for information
                                        //about system events
        IFEED_V2P_TRANSLATION,          //this feeder is able to do v2p translations
        IFEED_DUMP_FUNCTIONAL_STATE,    //this feeder can create a check point of the functional model
        IFEED_LAST                      // MUST BE LAST!!!
    };

  private:
    std::vector<bool> features;

    //
    // Since we can have multiple feeders we want to be able to
    // prefix the stats output with the feeder name.
    //
    char *feederName;
    IFEEDER_BASE parent;         // parent feeder in multi-level feeder config
    FEEDER_TYPE feederType;
    MACRO_FEEDER_TYPE macroType;
    
  protected:
    //
    // Feeder's call this in their constructors to indicate capabilities
    //
    void SetCapable(CAPABILITIES feature)
    {
        features[feature] = true;
    }

  public:
    //
    // Test a feeder's capabilities
    //
    inline bool
    IsCapable(CAPABILITIES feature) const
    {
        ASSERTX(feature < IFEED_LAST);
        return features[feature];
    }

    inline const char *GetFeederName() const { return feederName; }

    // Parent of this feeder in multi-level feeder model
    IFEEDER_BASE GetParentFeeder() const { return parent; }

    inline FEEDER_TYPE GetFeederType() const { return feederType; }

    inline MACRO_FEEDER_TYPE GetMacroType() const { return macroType; }
  protected:
    inline void SetMacroType(MACRO_FEEDER_TYPE t) { macroType = t; }

  public:

    //----------------------------------------------------------------
    // Startup / shutdown methods
    //----------------------------------------------------------------

    //
    // Init/Done functions.  These could be part of the constructor/destructor,
    // but they aren't.
    //
    virtual bool Init(UINT32 argc, char **argv, char **envp) = 0;
    virtual void Done(void) = 0;

    //
    // Force the feeder to terminate a thread, probably due to an error.
    // This is primarily used by a micro feeder to terminate a thread
    // owned by a macro feeder when the micro feeder needs to force an
    // abort.
    //
    virtual void ForceThreadExit(IFEEDER_STREAM_HANDLE stream) {}

    //----------------------------------------------------------------
    // Feeder state
    //----------------------------------------------------------------

    //
    // The number of active threads created by this feeder and all of its
    // children in the feeder hierarchy.
    //
    UINT32 NActiveThreads(void) const { return nActiveThreads; }

    //
    // SynchronizeFeederState is a hack.  It is a way around using
    // the defined interface between multi-level feeders to share
    // data between them in a private interface.  The call should go
    // away.
    //
    virtual void SynchronizeFeederState(void * state) {}

    //----------------------------------------------------------------
    // Pre-execution warm-up
    //----------------------------------------------------------------

    //
    // WarmUpClientInfo --
    //     Called by the warm-up manager before a group of calls to
    //     WarmUp().  The WARMUP_CLIENTS class describes the clients
    //     of the warm-up manager.  A feeder's warm-up code can take
    //     advantage of this information to reduce the amount of data
    //     passed back to the warm-up manager based on the type of
    //     information requested by the model.
    //
    virtual void
    WarmUpClientInfo(const WARMUP_CLIENTS clientInfo) {};

    //
    // Provide warm-up state for caches, branch predictors, etc.  Return true
    // if data exists.  The system will call WarmUp repeatedly until it returns
    // false.
    //
    // NOTE:  The feeder may choose to return true, indicating more warm-up
    // data is available while returning no useful data in the WARMUP_INFO.
    // This could happen if a feeder wanted to make sure it walked round-robin,
    // one instruction per CPU.
    //
    virtual bool
    WarmUp(IFEEDER_STREAM_HANDLE stream, WARMUP_INFO warmup)
    {
        return false;
    }

    //----------------------------------------------------------------
    // Basic control -- fetch, issue, commit, kill
    //----------------------------------------------------------------

    //
    // Fetch the next instruction from streamId at pc.  Fill in inst.
    // This method works for all types of feeders.  However, you MAY
    // NOT call it if you also use either FetchMacro or FetchMicro
    // for the feeder.
    //
    virtual bool
    Fetch(IFEEDER_STREAM_HANDLE stream,
          IADDR_CLASS pc,
          ASIM_INST inst,
          UINT64 cycle)
    {
        //
        // Feeder didn't supply this style.  Try calling the older
        // form without the cycle.
        //
        return Fetch(stream, pc, inst);
    }

    //
    // Fetch the next macro instruction.  This function may be exported
    // by either macro or micro instruction feeders.  A micro instruction
    // feeder would pass the call down to its macro instruction feeder.
    //
    virtual bool
    FetchMacro(IFEEDER_STREAM_HANDLE stream,
               IADDR_CLASS pc,
               ASIM_MACRO_INST inst,
               UINT64 cycle)
    {
        ASIMERROR("Feeder didn't supply a FetchMacro() routine");
        return false;
    }

    //
    // Fetch the next micro instruction based on a macro instruction
    // fetched earlier from the same feeder with FetchMacro().
    //
    virtual bool
    FetchMicro(IFEEDER_STREAM_HANDLE stream,
               IADDR_CLASS pc,
               ASIM_MACRO_INST macroInst,
               ASIM_INST inst,
               UINT64 cycle)
    {
        ASIMERROR("Feeder didn't supply a FetchMicro() routine");
        return false;
    }

    virtual UINT64
    GetFirstPC(IFEEDER_STREAM_HANDLE stream)
    {
        return 0;
    }
    
    
  protected:
    //
    // Old style fetch without cycle
    //
    virtual bool
    Fetch(IFEEDER_STREAM_HANDLE stream,
          IADDR_CLASS pc,
          ASIM_INST inst)
    {
        ASIMERROR("Feeder didn't supply a Fetch() routine");
        return false;
    }

  public:
    //
    // Inform simulator that 'inst' is being issued.
    //
    virtual void Issue(ASIM_INST inst) {};

    //
    // Inform simulator that 'inst' is being executed
    //
    virtual void Execute(ASIM_INST inst) {};

    //
    // Notifies simulator that instr 'inst' has been commited.
    // Requires: instruction 'inst' must be an instruction which has
    // already been fetched, but not yet committed or killed.
    //
    virtual void Commit(ASIM_INST inst) {};

    //
    // Kills all instructions younger than 'inst', and potentially kills 'inst'
    // itself.  If 'fetchNext' is true, then we will continue fetching after the
    // killed insn. Otherwise we will refetch the killed instruction; if
    // 'killMe' is true, the 'inst' will be the first one killed, otherwise
    // the instruction after 'inst' will be the first killed instruction;
    // if killMe is not set, it is !fetchNext;
    //
    // Requires: instruction 'inst' must be an
    // instruction which has already been fetched, but not yet committed or
    // killed.
    //
    virtual void
    Kill(ASIM_INST inst, bool fetchNext, bool killMe) {};

    //----------------------------------------------------------------
    // Fast forward
    //
    // Set IFEED_FAST_FORWARD capability
    //----------------------------------------------------------------

    //
    // Marker manipulation: set / clear
    // A marker is a binary flag on a static instruction.
    //
    enum ASIM_MARKER_CMD {
        MARKER_CLEAR_PC = 0,  // clear marker on specific address
        MARKER_CLEAR_ALL,     // clear marker on all addresses
        MARKER_SET_PC,        // set marker on specific address
        MARKER_SET_INST       // set marker on all matching instructions
    };

    virtual void
    Marker(ASIM_MARKER_CMD cmd,
           IFEEDER_STREAM_HANDLE stream,
           UINT32 markerID, 
           IADDR_CLASS markerPC = IADDR_CLASS(0, 0),
           UINT32 instBits = 0,
           UINT32 instMask = 0)
    {
        ASSERTX(! IsCapable(IFEED_FAST_FORWARD));
    };

    //
    // Skip 'n' instructions in 'thread'. If 'thread' is NULL, then skip
    // 'n' instructions in each thread that exists.
    // The bool all is added so that if we want to advance all of the
    // streams handled by this feeder
    //
    virtual UINT64
    Skip(IFEEDER_STREAM_HANDLE stream,
         UINT64 n,
         INT32 markerID = -1)
    {
        ASSERTX(! IsCapable(IFEED_FAST_FORWARD));
        return 0;
    };

    //----------------------------------------------------------------
    // Virtual to physical translation
    //----------------------------------------------------------------

    //
    // Translates a virtual PC to a physical PC.  Provided by base class
    // and calls the virtual ITranslate without IADDR_CLASS.
    // 

    //
    // Translates a virtual PC to a physical PC for Istream.  This is the only
    // form callable from outside the feeder itself.  The form without
    // the stream handle is no longer part of the interface, but is supported
    // for older feeders.
    // 
    virtual bool ITranslate(IFEEDER_STREAM_HANDLE stream,
                            UINT32 hwcNum,
                            UINT64 va,
                            UINT64& pa)
    {
        //
        // Feeder didn't supply this style.  Try calling the older
        // form.
        //
        return ITranslate(hwcNum, va, pa);
    }

    //
    // This method handles virtual references that may cross physical
    // pages.  It returns the PA of the first byte and also the descriptor
    // of the untranslated portion of the original virtual region.
    // The base feeder implementation provides a default implementation
    // that picks a safe, small page size.  Individual feeders may provide
    // their own version.
    //
    virtual bool ITranslate(IFEEDER_STREAM_HANDLE stream,
                            const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
                            UINT64& pa,
                            PAGE_TABLE_INFO pt_info,
                            MEMORY_VIRTUAL_REFERENCE_CLASS& vNextRegion);

  protected:
    virtual bool ITranslate(UINT32 hwcNum,
                            UINT64 va,
                            UINT64& pa)
    {
        ASIMERROR("Feeder didn't supply an ITranslate() routine");
        return true;
    };

  public:
    //
    // Translates a virtual PC to a physical PC for data stream
    //
    // NOTE:  The va passed in here may not match the va to which the
    //        instruction actually refers!  This can happen in a variety
    //        of cases, such as during VHPT walks on IPF.  It is the
    //        responsibility of the callee to choose a reasonable, feeder
    //        specific, policy for handling the case that the instruction's
    //        virtual effective address doesn't match va.
    // 
    virtual bool DTranslate(ASIM_INST inst,
                            UINT64 va,
                            UINT64& pa) = 0;

    //
    // This method handles virtual references that may cross physical
    // pages.  It returns the PA of the first byte and also the descriptor
    // of the untranslated portion of the original virtual region.
    // The base feeder implementation provides a default implementation
    // that picks a safe, small page size.  Individual feeders may provide
    // their own version.
    //
    virtual bool DTranslate(ASIM_INST inst,
                            const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
                            UINT64& pa,
                            PAGE_TABLE_INFO pt_info,
                            MEMORY_VIRTUAL_REFERENCE_CLASS& vNextRegion);


   // this function just adds the twolit offset to PA and returns it back and hence it is called PTranslate since it is P to P translation.
// This is need for page walks were the multiple memlookups are all physical addresses and need to be twolit offset (with respect to different threads).
    virtual bool PTranslate(ASIM_INST inst,
                            UINT64* pa) 
    {
       //ASIMERROR("Feeder didn't supply an PTranslate routine");
       return true;
    }


    //----------------------------------------------------------------
    // Execution mode.  Somewhat ISA specific.
    //----------------------------------------------------------------

    //
    // Current execution mode.  The mode should be what will be in effect when
    // the next instruction is fetched.
    //
    virtual CPU_MODE_TYPE GetCurrentCPUMode(void)
    {
        return CPU_MODE_UNKNOWN;
    }

    //----------------------------------------------------------------
    // Register value queries -- set IFEED_REGISTER_VALUES
    //----------------------------------------------------------------

    //
    // There are two interface styles for getting register values.  A feeder
    // is expected to support both.  In the first style, the caller requests
    // the value for a specific register and it is returned as an ARCH_REGISTER.
    // The calls return false if the register is not used by the instruction.
    // The function may return a value for a register not actually used by
    // the instruction if it happens to be known.
    //
    // Here are the functions for the first style:
    //

    virtual bool GetInputRegisterValue(
        ASIM_INST inst,
        ARCH_REGISTER_TYPE rType,
        INT32 regNum,
        ARCH_REGISTER reg)
    {
        ASSERTX(! IsCapable(IFEED_REGISTER_VALUES));
        return false;
    };

    virtual bool GetArchRegisterValue(
        IFEEDER_STREAM_HANDLE stream, 
        ARCH_REGISTER_TYPE rType,
        INT32 regNum,
        ARCH_REGISTER reg)
    {
        ASSERTX(! IsCapable(IFEED_REGISTER_VALUES));
        return false;
    };
 

    virtual bool GetOutputRegisterValue(
        ASIM_INST inst,
        ARCH_REGISTER_TYPE rType,
        INT32 regNum,
        ARCH_REGISTER reg)
    {
        ASSERTX(! IsCapable(IFEED_REGISTER_VALUES));
        return false;
    };

    //
    // For the second style, the caller doesn't need to know which registers
    // are used by the instruction.  The feeder will enumerate all input and
    // output registers.
    //
    // To find out about all registers start with slot = 0
    // and increase it monotonically.  When GetInputRegVal() returns false
    // you have found all the registers and the returned register
    // information is undefined.  Caller must pass a pointer to
    // an ARCH_REGISTER that will be filled in by GetInputRegVal().
    //
    // Note that the type of a returned register may be REG_TYPE_INVALID
    // if the slot is not used.  This does not mean that the slot is the
    // last valid slot.  You must keep incrementing slot until the function
    // returns false.
    //
    // There is no guaranteed order of registers.
    //
    virtual bool GetInputRegVal(
        ASIM_INST inst,
        UINT32 slot,
        ARCH_REGISTER reg)
    {
        ASSERTX(! IsCapable(IFEED_REGISTER_VALUES));
        return false;
    };

    //
    // Get predicates controlling the instruction.  These are separated
    // from input registers because some analysis within Asim handles
    // predicates differently.  Usage is similar to GetInputRegVal().
    //
    virtual bool GetPredicateRegVal(
        ASIM_INST inst,
        UINT32 slot,
        ARCH_REGISTER reg)
    {
        ASSERTX(! IsCapable(IFEED_REGISTER_VALUES));
        return false;
    };

    //
    // Get instruction's output register values.  Usage is similar to
    // GetInputRegVal() above.
    //
    virtual bool GetOutputRegVal(
        ASIM_INST inst,
        UINT32 slot,
        ARCH_REGISTER reg)
    {
        ASSERTX(! IsCapable(IFEED_REGISTER_VALUES));
        return false;
    };

    //
    // Return the contents of one of "inst"'s register operands (src or dest)
    //
#if TTL_EXTENSION == 1
//    virtual bool
//    GetRegValue(ASIM_INST inst,
//                RegOps regOp,
//                UINT64* varray,
//                UINT64 fillVL) {};
#else
//    virtual UINT64 GetRegValue(ASIM_INST inst, RegOps regOp) { return 0; };
#endif


    //----------------------------------------------------------------
    // Memory value queries -- set IFEED_MEMORY_VALUES
    //----------------------------------------------------------------

    virtual bool ReadMemory(
        ASIM_INST inst,
        void *buffer,
        UINT32 size,
        UINT64 pAddr)
    {
        ASSERTX(! IsCapable(IFEED_MEMORY_VALUES));
        return false;
    }

    virtual bool ReadMemory(
        IFEEDER_STREAM_HANDLE stream,
        void *buffer,
        UINT32 size,
        UINT64 pAddr)
    {
        ASSERTX(! IsCapable(IFEED_MEMORY_PEEK));
        return false;
    }


    //----------------------------------------------------------------
    // I/O port value queries -- set IFEED_MEMORY_VALUES
    //----------------------------------------------------------------

    virtual bool ReadIOPort(
        ASIM_INST inst,
        void *buffer,
        UINT32 size,
        UINT32 port)
    {
        ASSERTX(! IsCapable(IFEED_MEMORY_VALUES));
        return false;
    }

    //----------------------------------------------------------------
    // System Activity Queries
    //----------------------------------------------------------------

    // The verb "handle" is used because the system event can have side
    // effects in both the feeder and performance model.
    virtual FEEDER_SYSTEM_EVENT_CLASS HandleSystemEvent(
        ASIM_INST inst,
        FEEDER_SYSTEM_EVENT_TYPES type)
	{
        return FEEDER_SYSTEM_EVENT_CLASS(NULL_FEEDER_SYSTEM_EVENT, 0, 0);
	}

    // Ask the feeder to return the type of the next event to handle
    virtual FEEDER_SYSTEM_EVENT_TYPES GetNextSystemEventType(ASIM_INST inst)
    {
        return NULL_FEEDER_SYSTEM_EVENT;
    }

    // Query if a specific type of event is pending. Useful for looping through
    // all events and/or handling system events in a specific order.
    virtual bool IsSystemEventTypePending(
        ASIM_INST inst,
        FEEDER_SYSTEM_EVENT_TYPES type)
    {
        if (type == NULL_FEEDER_SYSTEM_EVENT) return false;
        return (type == GetNextSystemEventType(inst));
    }

    //----------------------------------------------------------------
    // Symbol table queries.  Set IFEED_SYMBOL_LOOKUP if feeder
    // supports symbol lookup.
    //----------------------------------------------------------------

    virtual UINT64 Symbol(IFEEDER_STREAM_HANDLE stream, char* name);

    //
    // Set IFEED_GLOBAL_SYMBOLS capability for this one...
    // 
    virtual UINT64 Symbol(char *name);

    virtual char*
    Symbol_Name(IFEEDER_STREAM_HANDLE stream,
                UINT64 address,
                UINT64& offset);

    //
    // Hack!  AWB's TCL code needs a method of supporting the "PmSymbol find"
    // method in old scripts.  That method expected easy access to a feeder
    // handle and simple thread numbering.  SingleFeederSymbol() succeeds if
    // exactly one feeder is active and it supports IFEED_GLOBAL_SYMBOLS.
    //
    static UINT64 SingleFeederSymbolHack(char *name);

    //----------------------------------------------------------------
    // Load/Store adjustment
    //
    // Set IFEED_MEM_CTL capability in feeder if these are supported.
    //
    //----------------------------------------------------------------

    //
    // Force Feeder to use Address supplied by the performance model.
    // Set IFEED_SET_EA capability.
    //
    virtual void SetEA(ASIM_INST inst, char vf, UINT64 vea)
    {
        ASSERTX(! IsCapable(IFEED_MEM_CTL));
    }

    //
    // This informs simulator that the load described by instruction 'inst'
    // has been fulfilled.
    // Note:This action is separate from the issue of the load (which
    //      computes the effective address) and the commit of the load
    //      (which allows other threads to see the stored value).
    //
    // Requires: instruction 'inst' must be a load instruction which has
    // already been issued, but not killed and for which the read has not 
    // yet been done. (Note that the instruction may already have committed!)
    // This must be called for every fetched read instruction that is not
    // killed.
    //
    virtual bool DoRead(ASIM_INST inst)
    {
        ASSERTX(! IsCapable(IFEED_MEM_CTL));
        return true;
    }

    virtual bool DoSpecWrite(ASIM_INST inst)
    {
        ASSERTX(! IsCapable(IFEED_MEM_CTL));
        return true;
    }

    virtual bool DoWrite(ASIM_INST inst)
    {
        ASSERTX(! IsCapable(IFEED_MEM_CTL));
        return true;
    }

    //
    // Called when pm decides a store_cond instr fails.
    //
    virtual void StcFailed(ASIM_INST inst)
    {
        ASSERTX(! IsCapable(IFEED_MEM_CTL));
    }

    //
    // Dump the stats for the feeder
    //
    virtual void DumpStats(STATE_OUT state_out) {}

    //
    // Clear the stats for the feeder
    //
    virtual void ClearStats() {}
        
    //----------------------------------------------------------------
    // Feeder management
    //----------------------------------------------------------------
  public:
    //
    // Call Done() method on all active feeders
    //
    static void AllDone(void);

    //
    // Delete all active feeders.  Assumes Done() methods have already been
    // called.
    //
    static void DeleteAllFeeders(void);

    //dump the stats from all the feeders
    static void DumpAllFeederStats(STATE_OUT state_out);

    static void DumpAllFeederState(const char * file_name, FUNCTIONAL_MODEL_DUMP_TYPE type);
    static void LoadAllFeederState(const char * file_name, FUNCTIONAL_MODEL_DUMP_TYPE type);

    //create a check point of the state
    virtual bool DumpState(const char * file_name, FUNCTIONAL_MODEL_DUMP_TYPE type)
    {
        ASSERTX(! IsCapable(IFEED_DUMP_FUNCTIONAL_STATE));
        return false;
    }
    
    //load a check point of the state
    virtual bool LoadState(const char * file_name, FUNCTIONAL_MODEL_DUMP_TYPE type)
    {
        ASSERTX(! IsCapable(IFEED_DUMP_FUNCTIONAL_STATE));
        return false;
    }

    //clear the stats from all the feeders
    static void ClearAllFeederStats();

  private:
    //
    // Private values for maintaining a linked list of all allocated feeders.
    //
    static IFEEDER_BASE activeFeederHead;
    IFEEDER_BASE nextActiveFeeder;

    //
    // The IFEEDER_THREAD_CLASS automatically maintains a count of active
    // threads in the feeder hierarchy.  These methods should be called
    // only by IFEEDER_THREAD_CLASS.
    //

    // A thread was started for this feeder or one of its children
    void NoteThreadActivated(void) { nActiveThreads += 1; }
    void NoteThreadDeactivated(void) { ASSERTX(nActiveThreads); nActiveThreads -= 1; }
    UINT32 nActiveThreads;
};


//----------------------------------------------------------------
// Thread management
//----------------------------------------------------------------

//
// A simple thread class for use by feeders to start and stop threads.
// The goal is to hide Asim's actual thread control mechanism from the
// feeders so we can avoid changing feeders when the thread implementation
// changes.
//
class IFEEDER_THREAD_CLASS : public TRACEABLE_CLASS
{
  public:
    IFEEDER_THREAD_CLASS(
        IFEEDER_BASE feeder,
        IFEEDER_STREAM_HANDLE stream,
        IADDR_CLASS startPC);

    ~IFEEDER_THREAD_CLASS();

    //
    // ThreadEnd() is separate from the destructor because the thread object
    // may need to survive for a while while the termination message is
    // being passed around.
    //
    void ThreadEnd(void);

    void SetVirtualPc(IADDR_CLASS vpc);

  private:
    ASIM_THREAD thread;
    const IFEEDER_BASE feeder;
};

        
//
// Call this to get a specific feeder.  Must be defined by the module that
// provides the feeder.
//
IFEEDER_BASE IFEEDER_New(void);

//
// Print command-line usage
//
void IFEEDER_Usage(FILE *file);




#endif /* _INSTFEEDERBASE_ */
