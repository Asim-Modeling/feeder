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
 * @file refeed_buffer.h
 *
 * @brief This is the buffer that holds the instructions to be refeed on
 * a front end resteer
 *
 * @author Chris Weaver
 */
//#include "asim/provides/refeed_buffer_v3.h"
//#include "refeed_info.h"

class FEEDER_QUEUE_CLASS
{

  private:
    //the instruction buffer (circular queue)
    instruction_feed_info instruction_buffer[REFEED_QUEUE_SIZE];
    //the number of instructions in the buffer
    UINT32 size;
    //the number of the head element
    UINT32 head;

    //the number of the 
    UINT32 tail;
    //the number of the instruction that we are refeeding from
    UINT32 mispeculation_pt;
    UINT32 issue_pt;
    bool misp_inst_left;

  public:
    FEEDER_QUEUE_CLASS();

    void add(cpuapi_inst_info_t *inst_info,const UINT64 bundle_bits[2],
        IADDR_CLASS pc,IADDR_CLASS npc,UINT64 oldcfm,
        UINT64 cfm,UINT64 oldpr,UINT64 pr,UINT64 Uid);

    /* These will set the regsiters of the tail element
       DOES NOT UPDATE POINTER so must be followed by an add 
       split up so can easily be turned off*/
    void set_input_dep(
        ARCH_REGISTER_CLASS src_Gp[NUM_SRC_GP_REGS],
        ARCH_REGISTER_CLASS src_Pred[NUM_SRC_PRED_REGS]);
    void set_old_bsp(UINT64 oldbsp);
    void set_output_dep(ARCH_REGISTER_CLASS dst[NUM_DST_REGS]);
    void set_bsp(UINT64 bsp);
    void set_nop(bool nop);
    void set_longImm(bool li);

    //remove the head element
    void removehead();

    //check the Uid and remove the head element
    void removehead(UINT64 Uid);

    //return the Uid of the head element-used for commit inorder checking
    UINT64 getheaduid(); 
    ARCH_REGISTER getpred(UINT64 Uid);
    ARCH_REGISTER getsrc(UINT64 Uid);
    ARCH_REGISTER getdest(UINT64 Uid);

    bool getpred(UINT64 Uid,UINT32 slot,ARCH_REGISTER reg);
    bool getsrc(UINT64 Uid,UINT32 slot,ARCH_REGISTER reg);
    bool getdest(UINT64 Uid,UINT32 slot,ARCH_REGISTER reg);
    //return a pointer to the element at num -null if num is not valid
    instruction_fi getnum(UINT32 num); 

    //print ALL the information about an entry
    void print_entry(instruction_fi inst);

    /* The following functions are used to do the issue checking */
    UINT64 getissueuid(); 

    /* The following functions are used to do the issue checking */
    UINT64 getctissueuid(); 

    /*set the instruction as being issued */
    void setissue(); 
    /* done on a FEED_KILL to reset the issue point to a earlier element */
    void resetissuept();
    /* done on normal issue to set to the next element in the list */
    void incremetissuept();
    /* returns the value of issued at the instruction */
    bool hasbeenissued();

    /* The following functions allow the buffer to refeed instructions
       after an unforseen exception occurs */

    //changes the Uid of the instruction needed because we can
    //have exceptions within exceptions. This can only be done
    //to the instruction at the mispeculation point
    int rename(UINT64 newUid);

    //return a pointer to the mispeculation_pt - null if not valid or
    //misp_inst_left = FALSE
    instruction_fi getmispec();

    //returns true if we have instructions to refeed false otherwise
    bool get_misp_inst_left();

    //move the mispeculation point ahead one
    void increment_mispec_pt();
    
    //this will search the array for the Uid to start
    //refeeding from. If killme is true the instruction
    //that matches is refeed otherwise instructions after
    //the Uid are refeed
    int set_mispec_pt(UINT64 Uid,bool killMe);

    IADDR_CLASS get_mispec_pc();
  
};

