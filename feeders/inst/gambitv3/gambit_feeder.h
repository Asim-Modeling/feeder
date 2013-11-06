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
 * @file gambit_feeder.h
 *
 * @brief This is the feeder interface to gambit
 *
 * @author Chris Weaver
 */

#ifndef _GAMBIT_CLASS_H
#define _GAMBIT_CLASS_H

/* stdlib stuff */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>




/* asim stuff */
#include "asim/provides/gambit_marker_v3.h"
#include "asim/provides/addrtranslator.h"
#include "asim/provides/refeed_buffer_v3.h"
#include "asim/provides/isa.h"


//#include "asim/provides/feeder.h"
// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/trace.h"
#include "asim/thread.h"
//#include "asim/instfeeder.h"

// ASIM public modules
#include "asim/provides/cpu_inst.h"


#include "asim/provides/softsdv_import_include.h"
#include "asim/provides/softsdv_interface.h"


//for some reason the compiler didn't like me making
//this a static constant in the class???
const UINT64 NOP[2] =
        {
            UINT64_CONST(0x0810200e02040800), // low bits
            UINT64_CONST(0x8038081020401c04)  // high bits
            
        };

const UINT64 BRANCH_BUNDLE[2] =
        { 
            // @#$$! little endian
            // change this to reflect an unconditional long imm. (probably) branch
            UINT64_CONST(0x0000002000000016), // low bits
            UINT64_CONST(0x4000000000100000)  // high bits
        };

const UINT64 THREE_BRANCH_BUNDLE[2] =
        { 
            // @#$$! little endian
            // change this to reflect an unconditional long imm. (probably) branch
            UINT64_CONST(0x0000002000000016), // low bits
            UINT64_CONST(0x0080000000004000)  // high bits
        };                 


class GAMBIT_FEEDER_CLASS : public IFEEDER_BASE_CLASS
{
  public:
    GAMBIT_FEEDER_CLASS(void);
    ~GAMBIT_FEEDER_CLASS() {};

  private:
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


/************************************************
 *
 * Thread specific variable
 *
 **********************************************/
    //refeed buffer- used to track the instructions fetch
    //in case the front end is resteered
    FEEDER_QUEUE_CLASS FEEDER_BUFFER[FEED_MAX_CPU];

    //the actual thread for each cpu
    IFEEDER_THREAD thread[FEED_MAX_CPU];

    // we just finished skipping
    // so send the model back to the correct path
    bool justSkipped[FEED_MAX_CPU];

    // we have detected wrongpath
    bool wrongPath[FEED_MAX_CPU];

    //These are values given for the memory
    //addresses on the pseudo-wrong path execution
    UINT64        LAST_PHY_ADDRESS[FEED_MAX_CPU];
    UINT64        LAST_VIR_ADDRESS[FEED_MAX_CPU];

    //quicker to record the old values then look them up before stepping
    UINT64 LAST_CFM[FEED_MAX_CPU];
    UINT64 LAST_BSP[FEED_MAX_CPU];
    UINT64 LAST_PRED[FEED_MAX_CPU];

    IADDR_CLASS NEXT_IP[FEED_MAX_CPU];

    bool LAST_INST_WAS_CONTROL[FEED_MAX_CPU];
    bool LAST_INST_WAS_MLX[FEED_MAX_CPU];

    //For the speed convert correct path
    cpuapi_inst_info_t all_inst_info[FEED_MAX_CPU][EXECUTE_GROUP_SIZE];
    UINT32  buffer_position[FEED_MAX_CPU]; 

    //stats 
    UINT64 TOTAL_FETCH_COUNT[FEED_MAX_CPU];
    UINT64 WRONG_FETCH_COUNT[FEED_MAX_CPU];
    UINT64 CORRECT_FETCH_COUNT[FEED_MAX_CPU];
    UINT64 REFEED_FETCH_COUNT[FEED_MAX_CPU];
    UINT64 INTERRUPT_COUNT[FEED_MAX_CPU];
    UINT64 CS_COUNT[FEED_MAX_CPU];
    UINT64 WRONG_ISSUE_COUNT[FEED_MAX_CPU];
    UINT64 CORRECT_ISSUE_COUNT[FEED_MAX_CPU];
    UINT64 FAKE_ISSUE_COUNT[FEED_MAX_CPU];
    UINT64 ZERO_ADDRESS_MEM_OPS[FEED_MAX_CPU];
    UINT64 POSSIBLE_ERROR_IN_CFM[FEED_MAX_CPU];
    UINT64 POSSIBLE_ERROR_IN_PR[FEED_MAX_CPU];
/***********************************************************
 *
 *Shared constants/configuration variables
 *
 *************************************************************/
    //when in os_mode there are different options to gambit
    //we also do not allow any interrupts in user mode
    bool os_mode;

    //the markers for awb commands
    TRACE_MARKER_CLASS marker;

    //a total count of every step on all cpus
    UINT64 all_streams_icount;

    //the number of active cpus in the reference model
    UINT64 number_of_cpus;

    //the file to do the restore from
    char *restore_file;

    //the file to include with hdb commands
    char *include_file;

    //following variables are used for generating check points
    bool checkpoints_on;
    UINT64 first_save_num;
    UINT64 inc_save_num;
    char *save_file;

    //the stream for the instruction stream output
    ofstream outfile;

    //Used to flag wrong path instructions
    static const UINT32 traceWrongpathId = UINT32_MAX;
    static const UINT32 traceArtificialInstId = UINT32_MAX - 1;
    

    ADDRESS_TRANSLATOR_CLASS addrTrans;

/*************************************************************
 *
 * Register information -functions for setting and getting the register
 * values that are stored in the refeed buffer
 *
 ***************************************************************/
  private:
    void set_feedbuf_input_reg_vals(UINT32 CPU_NUM,ASIM_INST inst);
    void set_feedbuf_output_reg_vals(UINT32 CPU_NUM,ASIM_INST inst);

  public:
    


    virtual bool GetInputRegVal(
        ASIM_INST inst,
        UINT32 slot,
        ARCH_REGISTER reg);

    virtual bool GetPredicateRegVal(
        ASIM_INST inst,
        UINT32 slot,
        ARCH_REGISTER reg);

    virtual bool GetOutputRegVal(
        ASIM_INST inst,
        UINT32 slot,
        ARCH_REGISTER reg);


/**************************************************************
 *
 *Instruction Information Sources
 *
 **************************************************************/
  private:
    bool ConvertCorrectPath (UINT32 CPU_NUM, ASIM_INST inst,
                         IADDR_CLASS PC);

    //The instruction is on the correct path .. execute a group of 
    //instructions and get the information
    bool ConvertCorrectPathSpeed (UINT32 CPU_NUM, ASIM_INST inst,
                                  IADDR_CLASS PC);

    //We had a false path that was the same of the good path... refeed
    //instruction after the mispeculation is declared    
    void Refeed(UINT32 CPU_NUM, ASIM_INST inst);

    //Create a wrong path instruction by looking up the memory location
    void ConvertWrongPath (UINT32 CPU_NUM, ASIM_INST inst,const IADDR_CLASS predicted_pc);

    //make a branch for a given stream at the given tmp_pc to the next reference
    //model ip
    void Fabricate_Branch(const IADDR_CLASS tmp_pc, ASIM_INST inst,const UINT64 sid );
    //make a nop for the context switch
    void Fabricate_CS(const IADDR_CLASS tmp_pc, ASIM_INST inst,const UINT64 sid);
    //make a nop (right now just used at the end of a thread
    void Fabricate_NOP(const IADDR_CLASS tmp_pc, ASIM_INST inst,const UINT64 sid);



    /***********************************
     *Common Feeder functions 
     ***********************************/
    bool Parse(UINT32 &argc, char **argv);
    virtual UINT64 Skip(IFEEDER_STREAM_HANDLE stream,
                        UINT64 n,
                        INT32 markerID);

    //get the instruction from one of the sources about
    virtual bool Fetch(IFEEDER_STREAM_HANDLE stream,
                       IADDR_CLASS predicted_pc,
                       ASIM_INST inst);

    //commit the instruction
    virtual void Commit(ASIM_INST inst);
    //kill an instruction or sequence of instructions
    virtual void Kill(ASIM_INST inst, bool fetchNext, bool killMe);
    //issue the instruction
    virtual void Issue(ASIM_INST inst);
    //initialize the feeder
    virtual bool Init(UINT32 argc, char **argv, char **envp);
    //done with the feeder
    virtual void Done(void);
    //dump the feeder stats
    virtual void DumpStats(STATE_OUT state_out);



    /**********************************
     *   Marker Functions 
     **********************************/
    //look up the addres of a symbol
    virtual UINT64 Symbol(IFEEDER_STREAM_HANDLE stream, char* name);
    virtual UINT64 Symbol(char* name);
    //get the closest symbol name from a PC (also returns the offset)
    virtual char* Symbol_Name(IFEEDER_STREAM_HANDLE stream,
                              UINT64 address,
                              UINT64& offset);

    //interface to the markers
    virtual void
    Marker(ASIM_MARKER_CMD cmd,
           IFEEDER_STREAM_HANDLE stream,
           UINT32 markerID, 
           IADDR_CLASS markerPC,
           UINT32 instBits,
           UINT32 instMask);


    /******************************************
     * Virtual to physical translation     
     **********************************************/

    virtual bool ITranslate(UINT32 hwcNum,
                            UINT64 va,
                            UINT64& pa);

    virtual bool DTranslate(ASIM_INST inst,
                            UINT64 va,
                            UINT64& pa);

/************************************************
*Helper functions 
************************************************/
  private:
    //prints a more verbose version of the usage
    void Usage_Verbose();

    //read the bundle bits from a certain location. Used when recording the
    //register values to determine the instruction type and therefore its
    //inputs before the instruction is stepped
    bool Read_BB(UINT32 CPU_NUM,const IADDR_CLASS  address, UINT64* bb);

    //The following four functions are used for batching instructions and
    //maintaining the     
    UINT64 GetNextCFM(UINT32 CPU_NUM);
    UINT64 GetNextPC(UINT32 CPU_NUM);
    UINT64 UnrotatePreds(UINT64 pr, UINT64 cfm);

    cpuapi_inst_info_t * GetCurrentInExecuteGroup(UINT32 CPU_NUM);
    bool StepExecuteGroup (UINT32 CPU_NUM);

    //switch performance modes -GEAR shifting
    void switchtoperformance();
    void switchtofunctional();

    //snr calls
    void restorefromfile(char * file);
    void savetofile(char *file);

    //get the current instruction count
    UINT64 ICNT(UINT32 CPU_NUM);

    //print out where in the program we are
    void Current_Location();

    //initialization routines
    void CommonVariablesInit();
    void ThreadVariablesInit(UINT32 t);
    void Stream_Trace_Init();

    //debug routine
    void print_inst_info(cpuapi_inst_info_t *inst_info);
};


#endif /*_GAMBIT_FEEDER_H*/

