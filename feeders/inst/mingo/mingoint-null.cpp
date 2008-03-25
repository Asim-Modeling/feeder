// 5/10/2002: Added unimplemented entry points to match instfeeder.h
// 5/10/2002: Changed calculation of number of CPUs for Mingo feeder's
// constructor to include number of TPUs per CPU
// 5/9/2002: Changed include of mingo_feeder.h to point to correct
// directory. Added include of instfeeder.h, which should get other
// things that we need. Temporarily made FEED_Init do what Judy's test
// code did to instantiate MINGO_FEEDER_CLASS, so we can compile.
// Fixed include to be of provides/feeder.h. Removed mistaken return
// value from FEED_Marker.


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
 * @author Judy Hall
 *
 * @brief Interface between the performance model and the Mingo feeder
 *
 * This file supports the generic FEED_* calls that are required by the
 * controller.
 */

// generic
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"

// ASIM local module
#include "asim/provides/instfeeder_interface.h"
// instfeeder_implementation.h is a generated header file
#include "asim/provides/instfeeder_implementation.h" 

MINGO_FEEDER MingoFeederHandle;

// Required entry points for the controller to call the feeder

/**
 * @brief Initialize Mingo feeder.
 * @return false if we have an error during initialization.
 */

// NOTE: The arguments passed via FEED_Init do not match what
// Mingo is expecting. Mingo wants:
//   path/name of program to run
//   argv
//   number of hardware contexts
//   stdin (defaults to NULL)
//   stdout (defaults to NULL)
//   stderr (defaults to NULL)

// FEED_Init supplies
//    argc
//    argv
//    envp

// Michael can decide what to do with envp. 
// Need to find a way to pass the number of CPUs.

    
// Mingo expects to be told the number of CPUs. This also
// controls the number that we create and loop through.


bool
FEED_Init (
    UINT32 argc,  ///< argument count of feeder specific arguments
    char **argv,  ///< feeder specific arguments
    char **envp)  ///< global program environment
{
    if ((argc == 0) || (argv[0] == NULL))
    {
        cerr << "FEED_Init:  Must specify client program with --feeder <prog>" << endl;
        exit(1);
    }

    //
    // Still need to fix this...

    // FIXME: Instead of passing MAX_TOTAL_NUM_HWCS to mingo, this should call
    // the static function of HWC class to query how many hwc there really are
    // and pass that number.
    MingoFeederHandle = new MINGO_FEEDER_CLASS(
        argv[0], argv, envp, MAX_TOTAL_NUM_HWCS,
        "/dev/null", "/dev/tty");

    ASSERTX (MingoFeederHandle);
    return true;
}

/**
 * @brief Simulation has finished. Clean up Mingo feeder.
 */

void
FEED_Done (void)
{
    MingoFeederHandle->Done();
    delete MingoFeederHandle;
}

/**
 * @brief Tell user how to invoke the feeder
 */

void
FEED_Usage (FILE *file)

// Michael should make this correct.

{
    ostringstream os;

    os << "\nFeeder usage: ... <path/program name> [<program args> ...] "
       <<"[stdin] [stdout] [stderr] \n";
    fputs(os.str().c_str(), file);
}

/* ---------------------- Operations not supported for Mingo -------- */

// NOTE for future enhancements:  Calls that take ASIM_INST should
// invoke methods of ASIM_INST

/**
 * @brief Marker manipulation 
 * Not implemented for Mingo feeder
 */
void
FEED_Marker (
    ASIM_MARKER_CMD cmd,  ///< specific marker command to perform
    ASIM_THREAD thread,   ///< thread to perform operation in
    UINT32 markerID,      ///< marker unique ID of this operation
    IADDR_CLASS markerPC, ///< marker PC of this operation
    UINT32 instBits,      ///< marker instruction bits for operation
    UINT32 instMask)      ///< marker instruction bit mask for operation
{
    cerr << "FEED_Marker is not supported for Mingo feeder" << endl;
    exit(1);
}

/**
 * @brief Symbol table query using thread ID 
 * Mot implemented for Mingo feeder
 */

UINT64
FEED_Symbol (
    UINT32 tid,  ///< thread ID to lookup symbol in
    char* name)  ///< symbol name to look up
{
    cerr << "FEED_Symbol is not supported on Mingo Feeder" << endl;
    exit(1);
    return 0;
}


/**
 * @brief Symbol table query using ASIM_THREAD 
 * Not implemented for Mingo feeder
 */

UINT64
FEED_Symbol (
    ASIM_THREAD thread,  ///< thread to lookup symbol in
    char* name)          ///< symbol name to look up
{
    cerr << "FEED_Symbol is not supported on Mingo Feeder" << endl;
    exit(1);
    return 0;
}


/**
 * @brief Skip numInst instructions or up to markerID, whatever occurs first.
 * Not implemented for Mingo feeder.
 */

UINT64
FEED_Skip (
    ASIM_THREAD thread,  ///< thread to skip instructions for
    UINT64 numInst,      ///< number of instructions to skip
    INT32 markerID)      ///< marker ID to skip to
{
    cerr << "FEED_Skip is not supported on Mingo Feeder" << endl;
    exit(1);
    return 0;
}

/**
 * @brief Translates virtual PC to physical PC
 * Not implemented for Mingo feeder
 */

UINT64 
FEED_ITranslate(ASIM_THREAD thread, UINT64 vpc)
{
    cerr << "FEED_ITranslate is not supported on Mingo Feeder" << endl;
    exit(1);
    return 0; 
}

/**
 * @brief Initialize "inst" with instruction information for
 * instruction from "thread" at "pc".
 * Not implemented for Mingo feeder.
 * Equivalent function is in HW_CONTEXT_CLASS::FetchOperation.
 *
 */

void
FEED_Fetch (ASIM_THREAD thread, UINT64 predicted_pc, ASIM_INST inst)
{
    cerr << "FEED_Fetch is not supported for Mingo Feeder. See " 
         << " HW_CONTEXT_CLASS::FetchOperation." << endl;
    exit(1);
}

void
FEED_Fetch2 (const UINT64 streamId, UINT64 predicted_pc, ASIM_INST inst)
{
    cerr << "FEED_Fetch2 is not supported for Mingo Feeder. See " 
         << " HW_CONTEXT_CLASS::FetchOperation." << endl;
    exit(1);
}

/**
 * @brief Return the contents of one of inst's register operands.
 * Not implemented for Mingo feeder
 */

UINT64
FEED_GetRegValue (ASIM_INST inst, RegOps regOp)
{
    cerr << "FEED_GetRegValue is not supported for Mingo Feeder" << endl;
    exit(1);
    return 0;
}

/**
 * @brief Force feeder to use address supplied by performance model.
 * Not implemented for Mingo feeder.
 */

void
FEED_SetEA(ASIM_INST inst, char vf, UINT64 vea)
{
    cerr << "FEED_SetEA is not supported for Mingo Feeder" << endl;
    exit(1);
}

/**
 * Inform feeder that instruction is being issued
 * Not implemented for Mingo feeder.
 */

void
FEED_Issue (ASIM_INST inst)
/*
 * Issue 'inst'...
 */
{
    cerr << "FEED_Issue is not supported for Mingo Feeder" << endl;
    exit(1);
}

/**
 * Do cache access for load
 * Not implemented for Mingo feeder.
 */

void
FEED_DoRead (ASIM_INST inst)
{
    cerr << "FEED_DoRead is not supported for Mingo Feeder" << endl;
    exit(1);
}

/**
 * Do speculative write for store.
 * Not implemented for Mingo feeder. 
 */

void
FEED_DoSpecWrite (ASIM_INST inst)
{
    cerr << "FEED_DoSpecWrite is not supported for Mingo Feeder" << endl;
    exit(1);
}

/**
 * Commit speculative write data.
 * Not implemented for Mingo feeder.
 */

void
FEED_DoWrite (ASIM_INST inst)
{    
    cerr << "FEED_DoWrite is not supported for Mingo Feeder" << endl;
    exit(1);
}

/**
 * brief Store conditional failed
 * Not implemented for Mingo feeder
 */

void
FEED_StcFailed (ASIM_INST inst)
{
    cerr << "FEED_StcFailed is not supported for Mingo Feeder" << endl;
    exit(1);
}

/**
 * Commit the instruction.
 * Not implemented for Mingo feeder. Equivalent function is in
 * ASIM_INST_CLASS::CommitOperation
 */

void
FEED_Commit (ASIM_INST inst)
{
    cerr << "FEED_Commit is not supported for Mingo Feeder. See" 
         << " ASIM_INST_CLASS::CommitOperation" << endl;
    exit(1);
}

/**
 * Kill instruction and all younger instructions
 * Not implemented for Mingo feeder
 */

void
FEED_Kill (ASIM_INST inst, bool mispredict, bool killMe)
{
    cerr << "FEED_Kill is not supported for Mingo Feeder" << endl;
    exit(1);

}
