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
 * @author Artur Klauser
 *
 * Trace reader for IA64 UCSD traces.
 * This is an adaptor for Weihaw Chuang's UCSD IA64 trace reader.
 * It is intended to be used as part of the code to implement an ASIM
 * Trace Reader plugin module for the ASIM Trace Feeder set of
 * modules.
 */

// generic
#include <stdio.h>
#include <stdarg.h>

// ASIM core
#include "asim/syntax.h"
#include "asim/trace.h"
#include "asim/mesg.h"
#include "asim/ioformat.h"

// ASIM local module
#include "ia64ucsd.h"

// UCSD tools and GNU (binutils) based tools
extern "C" {
#include "ssdis.h"
#include "sstr.h"
}

namespace iof = IoFormat;
using namespace iof;
using namespace std;

/**
 * Default constructor: initialize this object
 */
IA64_UCSD_CLASS::IA64_UCSD_CLASS (
    UINT32 id)    ///< ID of this trace stream
    : traceID(id)
{
    ss_init();
}

/**
 * Destuctor: cleanup this object
 */
IA64_UCSD_CLASS::~IA64_UCSD_CLASS()
{
    ss_done();
    tr64_finish_tlite();

    Format fmtLabel = Format("    ", "-25", " = ");
    //cout << fmtLabel("num trace lines read") << lineNum << endl;
}

/**
 * Open a file associated with this reader.
 * @returns true on success, false on failure
 */
bool
IA64_UCSD_CLASS::FileOpen (
    const char * fileName)  ///< file to open
{
    FILE *infile;

    // Note: the file name we get passed is the name of a trace
    // descriptor file; that file in turn contains 2 lines, the first
    // one is the file name of the binary and the second one is the
    // file name of the trace file;
    infile = fopen(fileName, "r");
    if(! infile)
    {
        return false;
    }

    const int BufSize = 256;
    char binaryName[BufSize];
    char traceName[BufSize];
    UINT32 lastChar;

    // parse for binary file name
    if (! fgets(binaryName, BufSize, infile))
    {
        ASIMERROR("can't parse UCSD trace descriptor file " << fileName);
    }
    lastChar = strlen(binaryName) - 1;
    if (binaryName[lastChar] == '\n')
    {
        binaryName[lastChar] = '\0';
    }

    // parse for trace file name
    if (! fgets(traceName, BufSize, infile))
    {
        ASIMERROR("can't parse UCSD trace descriptor file " << fileName);
    }
    lastChar = strlen(traceName) - 1;
    if (traceName[lastChar] == '\n')
    {
        traceName[lastChar] = '\0';
    }

    fclose(infile);

    bool  binaryLoaded = false;
    binaryLoaded = ss_load(binaryName);
    if(! binaryLoaded)
    {
        ASIMERROR("Loading binary " << binaryName << " failed" << endl);
        return false;
    }

    INT32 traceLoaded = TR64_ERROR;
    UINT64 init_pc;

    sm_initialize();
    tr64_init_tlite(traceName);
    traceLoaded = trace_initialize(&init_pc);
    switch(traceLoaded)
    {
      case TR64_DONE:  
        ASIMWARNING("Possibly empty trace:" << traceName
            << " finished early" << endl);
        return false;
      case TR64_ERROR:  
        ASIMERROR("Loading trace " << traceName << " failed");
        return false;
        // default:  
    }

    return true;
}

/**
 * Close the file associated with this reader.
 */
void
IA64_UCSD_CLASS::FileClose (void)
{
    // nada
}

/**
 * Get the next bundle from the trace file.
 */
bool  ///< @return true on success
IA64_UCSD_CLASS::GetNextInst (
    TRACE_INST traceInst)  ///< trace instruction to fill in
{
    UINT64            trace_this_pc;
    UINT64            trace_next_pc;
    INT32             br_taken = 0;
    UINT64            ea;
    UINT64            pr;
    UINT64            next_pr;
    UINT64            cfm_pfs;
    ss_ia64_inst      *sinst;
    INT32             status;

    // trace_single_step also decodes ss_ia64_inst which is expensive.
    // Should save time later by creating trace interface that's more
    // light weight
    status = trace_single_step_pr(&trace_this_pc,
                                  &trace_next_pc,
                                  &br_taken,
                                  &ea,
                                  &pr,
                                  &next_pr,
                                  &cfm_pfs,
                                  &sinst);


    if (status == TR64_VALID) 
    {
        sm_delete(&sinst); // free sinst inside UCSD tracer

        // if instruction available, parse, and fill traceInst
        status = GetInstBits (trace_this_pc, traceInst);

        const UINT64 next_bundleAddr = (trace_next_pc & ~UINT64_CONST(0x0f));
        const UINT8 next_sNum = trace_next_pc & 0x03;
        IADDR_CLASS next_pc = IADDR_CLASS(next_bundleAddr, next_sNum);

        traceInst->SetTarget(next_pc);
        traceInst->SetTaken(br_taken);
        traceInst->SetPredRF(pr, next_pr);
        // FIXME: cfm_pfs is sometimes old, sometimes new...need both
        traceInst->SetCFM(cfm_pfs, cfm_pfs);
        traceInst->SetVirtEffAddr(ea);

        // MLX bundle: L syllable "jumps" to next bundle
        bool isMLXBundle = ((traceInst->GetBundleBits()[0] & 0x1e) == 0x04);
        bool is2ndSyllable = ((trace_this_pc & 0x03) == 0x01);
        if (isMLXBundle & is2ndSyllable) {
            traceInst->SetNonSeqPc(next_pc);
        }
    }

    // If we're at the end of the trace, then signal the end
    // of thread, and feed nops like going down a wrongpath.
    if(status == TR64_DONE)
    {
      TRACE(Trace_Feeder, cout << "UCSD trace done" << endl); 
      return false;
    }
    else if(status == TR64_ERROR)
    {
      ASIMERROR("FEED_Fetch: failed to read trace");
      return false; // make compiler happy
    }

    return true;
}

/**
 * Get the instruction bits from the trace.
 */
INT32  ///< @return error/success code TR64_*
IA64_UCSD_CLASS::GetInstBits (
    UINT64     vpc,       ///< virtual PC
    TRACE_INST traceInst) ///< trace instruction to fill in
{
    bfd_byte bundle[16];         // bundle opcodes

    /* mask off the slot bits */
    const UINT64 bundleAddr = (vpc & ~UINT64_CONST(0x0f));
    const UINT8 sNum = vpc & 0x03;

    // fill bundle in with opcode info
    if (fast_get_bundle_bytes(bundleAddr, bundle) == 0)
    {
        // bundle not found, failed
        traceInst->SetEof();
        cerr << "fast_get_bundle_bytes failed in ucsd_parse_inst\n";
        return TR64_ERROR;
    }

    IADDR_CLASS pc = IADDR_CLASS(bundleAddr, sNum);
    // init general info
    traceInst->Init(
            pc,
            (UINT64 *) & (bundle[0]),
            false, // wrongpath
            0); //streamId

    return TR64_VALID;
}
