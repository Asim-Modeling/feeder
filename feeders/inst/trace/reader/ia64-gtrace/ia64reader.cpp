/*
 * Copyright (C) 2001-2006 Intel Corporation
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
 * @author Artur Klauser and Steven Wallace
 * @brief IA64-GTrace implementation of trace reader for trace feeder.
 */

// generic
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/trace.h"
#include "asim/thread.h"
#include "asim/ioformat.h"

// ASIM public modules
#include "asim/provides/traceinstruction.h"
#include "asim/provides/ipf_decoder.h"
#include "asim/provides/tracereader.h"

// ASIM local module
#include "ia64ciardibundle.h"
#include "ia64ciardi.h"
#include "bits.h"

namespace iof = IoFormat;
using namespace iof;

//--------------------------------------------------------------------------
// TraceReader Interface
//--------------------------------------------------------------------------

TRACE_READER_CLASS::TRACE_READER_CLASS ()
{
    SetTraceableName("TRACE_READER_CLASS");

    // clear pointers to readers
    for (UINT32 sid = 0; sid < MaxStreams; sid++) {
        stream[sid].ciardi = NULL;
        stream[sid].used = false;
        stream[sid].nRemainingWarmup = N_TRACE_WARMUP_INSTRS;
    }

    // instantiate a ciardi reader for each stream
    for (UINT32 sid = 0; sid < MaxStreams; sid++) {
        stream[sid].ciardi = new IA64_CIARDI_CLASS (sid);
        if (! stream[sid].ciardi)
        {
            ASSERT(false, "TraceReader: No memory for instruction reader." << endl);
        }
        stream[sid].bundlesFed = 0;
        stream[sid].lastSyllable = IA64_CIARDI_BUNDLE_CLASS::NumSyllables;
    }
}

bool
TRACE_READER_CLASS::Init (void)
{
    for(UINT32 s = 0; s < MaxStreams; s++) {
        stream[s].cfm = 96;
	stream[s].PRF = UINT64_MAX;
    }

    return true;
}

void
TRACE_READER_CLASS::Done (void)
{
    UINT64 total = 0;
    Format fmtLabel = Format("    ", "-25", " = ");

    for (UINT32 sid = 0; sid < MaxStreams; sid++)
    {
        if (stream[sid].used)
        {
            T1("stream " << sid << ":");
            T1(fmtLabel("num bundles fed")
                  << stream[sid].bundlesFed);
            total += stream[sid].bundlesFed;
        }

        if (stream[sid].ciardi)
        {
            delete stream[sid].ciardi;
        }
    }
    T1("total:");
    T1(fmtLabel("num bundles fed") << total);
}
  
bool
TRACE_READER_CLASS::FileOpen (
    INT32 streamId,
    const char * fileName)
{
    ASSERT(streamId < static_cast<INT32>(MaxStreams),
           "TraceReader trying to open too many concurrent streams" << endl);

    stream[streamId].used = true;
    return stream[streamId].ciardi->FileOpen(fileName);
}

void
TRACE_READER_CLASS::FileClose (
  INT32 streamId)
{
    ASSERT(streamId < static_cast<INT32>(MaxStreams),
           "TraceReader trying to close too many concurrent streams" << endl);

    stream[streamId].ciardi->FileClose();
}

/*
 * Return the next instruction in the trace (we initialize
 * 'inst' and return a pointer to it, we don't allocate a
 * new object), or return NULL if there are no more instructions.
 */
void
TRACE_READER_CLASS::GetNextInst (
    INT32 streamId,
    TRACE_INST traceInst)
{
    bool newBundle = false;

    ASSERT(streamId < static_cast<INT32>(MaxStreams),
           "TraceReader streamId out of range - too many concurrent streams" << endl);

    stream[streamId].lastSyllable++;
    // check if we need to fetch a new bundle
    if (stream[streamId].lastSyllable >= IA64_CIARDI_BUNDLE_CLASS::NumSyllables) {
        stream[streamId].currentBundle = stream[streamId].ciardi->GetNextBundle();
        newBundle = true;
        stream[streamId].lastSyllable = 0;
    }

    //--- yank syllable info out of bundle and put into TRACE_INST ---
    IA64_CIARDI_BUNDLE bundle = stream[streamId].currentBundle;
    UINT8 sNum = stream[streamId].lastSyllable;

    if (bundle) {
        if (newBundle) {
            stream[streamId].bundlesFed++;
        }

        IADDR_CLASS pc = IADDR_CLASS(bundle->vpc, sNum);

        // init general info
        traceInst->Init(
            pc,
            & (bundle->bits[0]),
            bundle->syllable[sNum].wrongpath);

        UINT64 warmUp = stream[streamId].nRemainingWarmup;
        if (warmUp != 0)
        {
            traceInst->SetIsWarmUp();
        }

        if (bundle->syllable[sNum].wrongpath) {
            return;
        }

        if (warmUp != 0)
        {
            stream[streamId].nRemainingWarmup = warmUp - 1;
        }

        // setup additional syllable info
        traceInst->SetTarget(bundle->syllable[sNum].target);
        traceInst->SetTaken(bundle->syllable[sNum].taken);
        traceInst->SetTrap(bundle->syllable[sNum].trap);
        traceInst->SetVirtEffAddr(bundle->syllable[sNum].addr);

        UINT64 newprf = stream[streamId].PRF;  // new PRF = old PRF

        // translate the virtual predicate register number into the
        // physical number and then write a '1' or '0' bit into the
        // 64-bit predicate register file depending on the result value
        INT8 ppr;

        // check and update first predicate result
        ppr = pr_v2p(bundle->syllable[sNum].vpr[0], stream[streamId].cfm);
        if (ppr > 0) {
            if (bundle->syllable[sNum].value[0])
		newprf |= (UINT64)(1) << ppr;
            else
		newprf &= ~((UINT64)(1) << ppr);
        }

        // check and update second predicate result
        ppr = pr_v2p(bundle->syllable[sNum].vpr[1], stream[streamId].cfm);
        if (ppr > 0) {
            if (bundle->syllable[sNum].value[1])
		newprf |= (UINT64)(1) << ppr;
            else
		newprf &= ~((UINT64)(1) << ppr);
        }

        // convert virtual PRs to physical
        if (bundle->syllable[sNum].movpr) {
	    newprf = pregs_v2p(bundle->syllable[sNum].pregs, stream[streamId].cfm);
        }

	// record old and new PRFs to trace
	traceInst->SetPredRF(stream[streamId].PRF, newprf);
	stream[streamId].PRF = newprf; // update new PRF for this stream

	// record and update the CFM for this stream
        UINT64 newcfm = FixCFM(streamId, traceInst);
        traceInst->SetCFM(stream[streamId].cfm, newcfm);
	stream[streamId].cfm = newcfm;

        //if we are not ignoring the register values then copy any that exist
        if(!IGNORE_REG_VALUES)
        {
            //record and update the bsp for this stream
            if(bundle->syllable[sNum].bsp)
            {
                traceInst->SetBSP(stream[streamId].bsp,bundle->syllable[sNum].bsp );
                stream[streamId].bsp=bundle->syllable[sNum].bsp;
            }
            else
            {
                traceInst->SetBSP(stream[streamId].bsp,stream[streamId].bsp);
            }
 
            //put the input and output registers into the trace instruction
            //if the register did not exist it should be an invalid register type
            traceInst->SetInputDep(bundle->syllable[sNum].input_registers);
            traceInst->SetOutputDep(bundle->syllable[sNum].output_registers);

        }
    } else {
        traceInst->SetEof();
    }
}


// from achebe:rename.cxx
// convert virtual predicate number into physical number using
// CFM's rrb.pr predicate register base for rotating registers
INT8
TRACE_READER_CLASS::pr_v2p (
    INT8   virt,
    UINT64 cfm)
{
    INT8 phys = virt;
    INT8 pr_base = (cfm >> 32) & 0x3f;
    ASSERTX(pr_base < 48);
    if (virt < 1) return -1;
    if (phys > 15) {
      phys -= 16;
      if (pr_base != 0) {
	phys = (phys + pr_base) % 48;
      }
      phys += 16;
    }
    return phys;
}

UINT64
TRACE_READER_CLASS::pregs_v2p (
    UINT64 vpregs,
    UINT64 cfm)
{
    INT8 pr_base = (cfm >> 32) & 0x3f;
    ASSERTX(pr_base < 48);

    // common case - no rotation
    if (pr_base == 0)
    {
        return vpregs;
    }
    // rotate p16-p63 to their physical positions
    else
    {
#if 0 /* CIARDI_DUMP */
        stream[0].ciardi->HackSPRecord(
            ((vpregs << 48) >> 48) |
            ((vpregs >> (pr_base + 16)) << 16) |
            ((vpregs >> 16) << (64 - pr_base)));
#endif
        return
            ((vpregs << 48) >> 48) |
            ((vpregs >> 16) << (pr_base + 16)) |
            ((vpregs >> (64 - pr_base)) << 16);
    }
}

// modled after achebe:rename.cxx::CheckRSE()
UINT64
TRACE_READER_CLASS::FixCFM (
    INT32 streamId,
    TRACE_INST traceInst)
{
    Bits<38> cfm;

    const UINT64* bundle_bits = traceInst->GetBundleBits();

    UINT64 syllables[SYLLABLES_PER_BUNDLE];
    const TEMPLATE_DESCRIPTOR *td = BundleSplit(bundle_bits, syllables);
    int syl_idx = traceInst->VirtualPc().GetSyllableIndex();

    cfm = stream[streamId].cfm;
    UINT64 newcfm = stream[streamId].currentBundle->syllable[stream[streamId].lastSyllable].cfm;

    // alloc
    if ((td->units[syl_idx] == TYPE_UNIT_M) &&
        ((syllables[syl_idx] & 0x1ee00000000ULL) == 0x02c00000000ULL))
    {
        cfm(17,14) = ((syllables[syl_idx] >> 27) & 0xf);
        cfm(13,7) = ((syllables[syl_idx] >> 20) & 0x7f);
        cfm(6,0) = ((syllables[syl_idx] >> 13) & 0x7f);
    }
    // br.call/brl.call
    else if ((((td->units[syl_idx] == TYPE_UNIT_B) &&
               ((syllables[syl_idx] & 0x16000000000ULL) == 0x02000000000ULL)) ||
              ((td->units[syl_idx] == TYPE_UNIT_X) &&
               ((syllables[syl_idx] & 0x1c000000000ULL) == 0x18000000000ULL))) &&
             traceInst->Taken())
    {
        cfm(6,0) = cfm(6,0) - cfm(13,7);
        cfm(37,7) = 0;
    }
    // br.ret
    else if (((td->units[syl_idx] == TYPE_UNIT_B) &&
              ((syllables[syl_idx] & 0x1e1f80001c0ULL) == 0x00108000100ULL)) &&
             traceInst->Taken())
    {
        if (newcfm == UINT64_MAX)
        {
            if (stream[streamId].ciardi->GetNextBundle() != NULL)
            {
                ASSERT(false, "br.ret does not have valid CFM in trace");
            }
        }
        // PFM = CFM from trace
        cfm = newcfm; // set new CFM to PFM
    }
    // rfi/rfi.x
    else if ((td->units[syl_idx] == TYPE_UNIT_B) &&
             ((syllables[syl_idx] & 0x1e1f0000000ULL) == 0x00040000000ULL))
    {
        if (newcfm == UINT64_MAX)
        {
            if (stream[streamId].ciardi->GetNextBundle() != NULL)
            {
                ASSERT(false, "rfi does not have valid CFM in trace");
            }
        }
        // IFM = CFM from trace
        cfm = newcfm; // set new CFM to IFM
    }
    // cover
    else if ((td->units[syl_idx] == TYPE_UNIT_B) &&
             ((syllables[syl_idx] & 0x1e1f8000000ULL) == 0x00010000000ULL))
    {
        cfm = 0;
    }
    // br.ctop/br.cexit/br.wtop/br.wexit
    else if ((td->units[syl_idx] == TYPE_UNIT_B) &&
             ((syllables[syl_idx] & 0x1e000000080ULL) == 0x08000000080ULL))
    {
#if 0 /* CIARDI_DUMP */
        stream[0].ciardi->HackC1Record();
        stream[streamId].currentBundle->syllable[stream[streamId].lastSyllable].vpr[0] = 63;
#endif
        // check for missing c1 record trace bug
        if (stream[streamId].currentBundle->syllable[stream[streamId].lastSyllable].vpr[0] != 63)
        {
            if (stream[streamId].ciardi->GetNextBundle() != NULL)
            {
                cout << "stream Id here=" << streamId << endl;
                ASSERT(false, "mod-sched br does not have valid C1 record in trace");
            }
        }
        if (newcfm == UINT64_MAX)
        {
            if (stream[streamId].ciardi->GetNextBundle() != NULL)
            {
                ASSERT(false, "mod-sched br does not have valid CFM in trace");
            }
        }
        // (potentially) rotated CFM = CFM from trace
        cfm = newcfm;
    }
    // clrrrb
    else if ((td->units[syl_idx] == TYPE_UNIT_B) &&
             ((syllables[syl_idx] & 0x1e1f8000000ULL) == 0x00020000000ULL))
    {
        // reset rrb_gr, rrb_fr, rrb_pr
        cfm(37,18) = 0;
    }
    // clrrrb.pr
    else if ((td->units[syl_idx] == TYPE_UNIT_B) &&
             ((syllables[syl_idx] & 0x1e1f8000000ULL) == 0x00028000000ULL))
    {
        // reset just rrb_pr
        cfm(37,32) = 0;
    }

    return cfm;
}
