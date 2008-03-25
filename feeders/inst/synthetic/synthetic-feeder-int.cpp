/***************************************************************************
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
 ****************************************************************************/
 
/**
 * @file 
 * @author Mark Charney
 *
 * @brief A feeder for ASIM that generates synthetic instructions
 *
 * This file supports the generic FEED::* calls that are required by the
 * controller.
 *
 * There is the notion of the correct path and the wrong path. This code
 * generates junk syllables for the wrong path and does not remember
 * them. If wrong path instructions are ever replayed, we just make new
 * ones. For correct path instructions, we put them on the replay queue and
 * issue them from there. When they issue, the syllables are put in the
 * issue list. When they retire, we remove the syllables from the issued
 * list. If correct path instructions are killed, we copy them from the
 * issued list to the replay list. When instructions commit, they are
 * removed from the issued list. We track instructions on the correct path
 * with SYNTH_INST class instances.
 *
 * ASIM TWD models do not commit NOPs, so if there are NOPs on the issued
 * list, they get commited implicitly when the first non-NOP commits after
 * a sequence of one or more NOPs.
 *
 * There is a lot of non-object oriented code that could arguably be put
 * into the SYNTH_THREAD_CLASS or some other class that manufactures higher
 * level IPF bundles. Most can be sucked into THREAD_MGMT_CLASS with
 * a simple matter of typing. There is nothing fundamentally stopping that.
 *
 * Main Classes:
 *
 *     SYNTHETIC_FEEDER_CLASS : public IFEEDER_BASE_CLASS. The public
 *     interface to this stuff.
 *
 *     THREAD_MGMT_CLASS: builds the threads and tells the ASIM controller to
 *     stop/start them.  *** Also owns the threads[] array of
 *     SYNTH_THREAD_CLASS* pointers.  This owns the Memory Value Model
 *     (MVM).  [This is currently a file global and that will be probematic
 *     one day.]
 * 
 *     SYNTH_THREAD_CLASS: responsible for sequencing the instructions that
 *     we pass to ASIM. Handles replay, wrong-path tracking, etc. This class
 *     sets and controls the instruction pointer (pc).
 * 
 *     SYNTH_EVENT_CLASS: generates data addresses and reference types
 *     according to runtime parameters.
 *
 *     SYNTH_ROUTINE_CLASS: generates the program counter sequences
 *     according to runtime parameters.
 *
 *     SYNTH_INST_CLASS: internal representation of instructions.
 *
 *  Supporting Classes:
 *
 *     IPF_INST_BUILDER_CLASS: for creating bundles and selecting templates
 *     from syllable specifiers
 *
 *     VALUE_HISTORY_CLASS: Array of recent values for locality and
 *     sharing.
 *                           
 *     REGISTER_PROVIDER_CLASS: Hands out registers to use for different
 *     types of instructions, eg loads vs xors.
 *
 *  
 */

#include "synth-headers.h"

// ASIM local module
#include "synthetic-feeder.h"

/*FIXME: synth-events requires random_get_event() */
#include "synth-events.h"

#include "synth-thread-mgmt.h"

////////////////////////////////////////////////////////////////////////////


/* thread_mgmt is a file global that owns all the state. */
static THREAD_MGMT_CLASS thread_mgmt;

////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////

static SYNTH_PARAMS params = new SYNTH_PARAMS_CLASS();

////////////////////////////////////////////////////////////////////////////


/* FIXME: this is used by synth-event.h */
/* This is required to be here to access thread_mgmt. */
SYNTH_EVENT_CLASS*
random_get_event(void)
{
    UINT32 i = random_in_range(SYNTH_THREADS);
    assert(i < SYNTH_THREADS);
    //cerr << "picking node " << i <<endl;
    return thread_mgmt.get_thread(i)->get_event();
}
////////////////////////////////////////////////////////////////////////////


static void
init_synth_feeder(IFEEDER_BASE ifeeder)
{
    if (SYNTH_PRINT)
    {
        itrace = true; // turn on tracing/disassembly of events passed->ASIM
    }

    params->init_params();
    
    init_random_number_generator();

    XMSG("SYNTH_EVEN_BUNDLE = " << SYNTH_EVEN_BUNDLE);
    XMSG("SYNTH_ODD_BUNDLE = " << SYNTH_ODD_BUNDLE);
    XMSG("SYNTH_MEMOP_BUNDLE = " << SYNTH_MEMOP_BUNDLE);


    thread_mgmt.make_threads(ifeeder, params);
}


/********************************************************************
 *
 * Helper functions
 *
 *******************************************************************/

static bool 
parse_args(UINT32 argc,
           char **argv)
{
/*
 * Parse command line arguments, return FALSE if there is
 * a parse error.
 */

    for (UINT32 i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-d")==0)
        {
            itrace = true; // turn on tracing/disassembly of events passed->ASIM
        }
    }

    return true;
}

/********************************************************************
 *
 * FEEDer callbacks
 *
 *******************************************************************/

bool
SYNTHETIC_FEEDER_CLASS::DTranslate(ASIM_INST inst,
                                   UINT64 vpc,
                                   UINT64& pa)
{
    UINT32 streamId = get_stream_id(inst);
    // mask because HW is sensitive to upper unimplmented bits
    pa = data_address_space_mask & vpc; // virtual equals real for now 
    SMSG("DTranslating for stream: " << streamId << ": " << hex << vpc << " -> " << pa << dec);
    return true;  // page mapping exists 
}


bool
SYNTHETIC_FEEDER_CLASS::ITranslate(UINT32 hwcNum, UINT64 vpc, UINT64& pa)
{
    // mask because HW is sensitive to upper unimplmented bits
    pa = inst_address_space_mask & vpc; // virtual equals real for now 
    return true;  // page mapping exists 
}

/****************************************************************/
/* Stuff we really use in the feeder interface */
/****************************************************************/

bool
SYNTHETIC_FEEDER_CLASS::Fetch(IFEEDER_STREAM_HANDLE stream,
                              IADDR_CLASS predicted_pc,
                              ASIM_INST inst) 
{
    UINT32 streamId = STREAM_HANDLE(stream);
    // compute the bundle bits by talking to inst-generator
    thread_mgmt.generate_asim_inst(streamId, predicted_pc, inst);

    return true;
}

void
SYNTHETIC_FEEDER_CLASS::Issue(ASIM_INST inst)
{
    /* set the qualifying predicate values to their real values,
       set the branch targets, and also the branch direction. */
    UINT32 streamId = get_stream_id(inst);
    m_debug("FEED::Issue", inst);
    thread_mgmt.issue(inst, streamId);
}

void
SYNTHETIC_FEEDER_CLASS::Commit(ASIM_INST inst)
{
    UINT32 streamId = get_stream_id(inst);
    SMSG("FEED::Commit uid = " << inst->GetUid() << "  stream = " << streamId);
    thread_mgmt.commit(inst, streamId);

}


void
SYNTHETIC_FEEDER_CLASS::Kill(ASIM_INST inst,
                             bool fetchNext,
                             bool killMe)
{
    UINT32 streamId = get_stream_id(inst);
    thread_mgmt.kill(inst, streamId, fetchNext, killMe);
}

bool
SYNTHETIC_FEEDER_CLASS::Init(UINT32 argc,
                             char **argv,
                             char **envp)
{
/*
 * Initialize Synthetic instruction feeder. Return false
 * if we have an error during initialization.
 */
    printf ("synthetic feeder initializing ...\n");

    // Parse the command-line.
    if (! parse_args(argc, argv) )
    {
        return false;
    }

    SetCapable(IFEED_MEM_CTL);
    SetCapable(IFEED_V2P_TRANSLATION);
    init_synth_feeder(this);
    
    return true;
}


void
SYNTHETIC_FEEDER_CLASS::Done (void)
{
    MSG("FEED::Done() called.");
}


SYNTHETIC_FEEDER_CLASS::SYNTHETIC_FEEDER_CLASS() //CONS
    : IFEEDER_BASE_CLASS("Synthetic Feeder") ,
      data_address_space_mask(((UINT64) (1ULL << SYNTH_DATA_SPACE)) - 1),
      inst_address_space_mask(((UINT64) (1ULL << SYNTH_CODE_SPACE)) - 1),
      warm_instr(0),
      warm_addr(SYNTH_WARMUP_INITIAL_ADDR)
{

    ASSERTX(SYNTH_CODE_SPACE > 0 && SYNTH_CODE_SPACE <= 64);
    ASSERTX(SYNTH_DATA_SPACE > 0 && SYNTH_CODE_SPACE <= 64);
}

UINT32
SYNTHETIC_FEEDER_CLASS::get_stream_id(ASIM_INST inst)
{
    /* convert the external stream id's to our internal stream ids */
    UINT32 s = STREAM_HANDLE(inst->GetSWC()->GetFeederStreamHandle());
    return s;
}


////////////////////////////////////////////////////////////////////////////

/*
 * Talk to them MVM object (via the thread_mgmt object.)
 */

bool
SYNTHETIC_FEEDER_CLASS::DoRead(ASIM_INST inst)
{
    const UINT32 streamId = get_stream_id(inst);
    m_debug("FEED::DoRead", inst);
    if (params -> getSynth_Synchronization(streamId))
    {
        thread_mgmt.handle_mvm_read(inst, streamId);
    }
    return true;
}

bool
SYNTHETIC_FEEDER_CLASS::DoSpecWrite(ASIM_INST inst)
{

    /*
      Bypassing of locally visible stuff is not allowed for
      semaphores (cmpxch, fetchadd, etc.) so we skip the
      locally visible step for them.
     */
    const UINT32 streamId = get_stream_id(inst);
    
    if (params -> getSynth_Synchronization(streamId))
    {
	
	    if (!inst->IsSemaphore())
        {
            const UINT32 streamId = get_stream_id(inst);
            m_debug("FEED::DoSpecWrite", inst);
            thread_mgmt.handle_mvm_writes(inst,
					  streamId,
                      MVM_LOCALLY_VISIBLE,
                      "LocalVisStore");
        }
    }
    return true;
}

bool
SYNTHETIC_FEEDER_CLASS::DoWrite(ASIM_INST inst)
{
    const UINT32 streamId = get_stream_id(inst);
    m_debug("FEED::DoWrite", inst);
    if (params -> getSynth_Synchronization(streamId))
    {

        thread_mgmt.handle_mvm_writes(inst,
		    	      streamId,
				      MVM_GLOBALLY_VISIBLE,
				      "Attempting GlobalVisStore");
    }
    return true;
}
////////////////////////////////////////////////////////////////////////////

bool
SYNTHETIC_FEEDER_CLASS::WarmUp(
    IFEEDER_STREAM_HANDLE stream,
    WARMUP_INFO warmup)
{

    if(warm_instr >= SYNTH_WARMUP_INSTR)
    {
        return false;
    }

    warmup->NoteLoad(warm_addr, warm_addr, 1);
    
    warm_instr++;
    warm_addr = (warm_addr + SYNTH_WARMUP_STRIDE) & data_address_space_mask;
    
    if(warm_instr % 5000 == 0) cout << "Warming up caches... Status: " << warm_instr << " of " << SYNTH_WARMUP_INSTR << "." << endl;
    
    return true;
}


void
IFEEDER_Usage (FILE *file)
{
    /* * Print usage... */
    ostringstream os;
    os  << endl << "Feeder usage: ... [-d] ..." << endl;
    fputs(os.str().c_str(), file);
}

IFEEDER_BASE
IFEEDER_New(void)
{
    return new SYNTHETIC_FEEDER_CLASS();
}
