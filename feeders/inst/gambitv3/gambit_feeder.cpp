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

//
// Authors:  Chris Weaver, Jim Vash
//

#include <stdlib.h>

#include "asim/provides/instfeeder_implementation.h"

#define V2P_WORKAROUND 1

GAMBIT_FEEDER_CLASS::GAMBIT_FEEDER_CLASS(void):
    IFEEDER_BASE_CLASS("Gambit Feeder v3")
{
    SetCapable(IFEED_FAST_FORWARD);
    SetCapable(IFEED_SYMBOL_LOOKUP);
    SetCapable(IFEED_GLOBAL_SYMBOLS);
    if (RECORD_REGISTERS)
    {
        SetCapable(IFEED_REGISTER_VALUES);
    }
}


/********************************************************************
 *
 * Helper functions that deal with HDB.. once the hacked interface
 * goes away these will need to be replaced with some other method of
 * doing these actions
 *
 *******************************************************************/
//change a U64 to a UINT64
UINT64 
u64touint64(U64 num)
{
    UINT64 tmp = num.high_w;
    tmp <<= 32;
    tmp |= num.low_w;
    return tmp;
}

UINT64 
GAMBIT_FEEDER_CLASS::UnrotatePreds(UINT64 pr, UINT64 cfm)
{
    UINT64 corrected_preds=pr;
    UINT64 rotate_amount= (cfm >> 32) & 0x03F;
    UINT64 static_pred=0;
    UINT64 rotate_pred=0;
    UINT64 rotated_off=0;
    UINT64 rotated_down=0;
    UINT64 rotated=0;

    if(rotate_amount)
    {
        TRACE(Trace_Feeder,
              cout <<"\t\tPreds before correction: "<<fmt("016x",corrected_preds)<<"Rotating them by"<<rotate_amount<<endl);
        static_pred=pr & 0x0ffff;
        rotate_pred=(pr & (0xFFFFFFFFFFFF0000LL))>>16;
        rotated_off= (rotate_pred)>> (48-rotate_amount);
        rotated_down= (rotate_pred) << (rotate_amount);
        rotated= rotated_off | rotated_down;
        corrected_preds=(rotated <<16)+static_pred;
    }
    else
    {
        corrected_preds=pr;
    }
        
    return corrected_preds;
}
//change the simulation mode to performance
void
GAMBIT_FEEDER_CLASS::switchtoperformance()
{
    hdb_set_oml_obj("notifier.vpc.kernel.simul_mode = 2");
}
//change the simulation mode to function
void
GAMBIT_FEEDER_CLASS::switchtofunctional()
{
    hdb_set_oml_obj("notifier.vpc.kernel.simul_mode = 4");
}

//restore from a file
void
GAMBIT_FEEDER_CLASS::restorefromfile(char * file)
{
      cout<<"Restoring simulation from: "<<file<<endl;
      char Buff[512] ;
      sprintf(Buff,"vpc.srmgr restore all f %s",file) ;
      hdb_platform(Buff);
}

//save to a file
void
GAMBIT_FEEDER_CLASS::savetofile(char *file)
{
      cout<<"Saving simulation to: "<<file<<endl;
      char Buff[512] ;
      sprintf(Buff,"vpc.srmgr save all f %s",file) ;
      hdb_platform(Buff);
}
//Print out the closest symbol to the address that we are at
void 
GAMBIT_FEEDER_CLASS::Current_Location()
{

    char* name;
    U64 tmp_offset;
    UINT64 offset;

    name=msl_label_name(hdb_get_ip(), 0, &tmp_offset);
    offset=u64touint64(tmp_offset);
    if(name!=NULL)
    {
        cout<<"Reference model now at: "<<offset<<" bytes into function: "<<name<<endl;
    }
    else
    {
        cout<<"Reference model now at: unknown location. Compile with a symbol table if you want this information"<<endl;
    }
}

/**************************************************************
 *Helper functions that call the cpu api
 *Get the bundlebits, register input and output values 
 **************************************************************/
//get the instruction count for the cpu
UINT64
GAMBIT_FEEDER_CLASS::ICNT(UINT32 CPU_NUM)
{
    UINT64 value=0;
    if(!asim_get_sim_litcount(CPU_NUM, &value))
    {
        cerr<<"Unable to get the instruction count"<<endl;
        return 0;
    }
    else
    {
        return value;
    }
}

//prints out the contents of the inst_info for debug purposes
void
GAMBIT_FEEDER_CLASS::print_inst_info(cpuapi_inst_info_t *inst_info)
{
    UINT64 bundle_bits[2];
    cout<<"instruction id: "<<inst_info->inst_id<<endl;
    cout<<"instruction count: "<<inst_info->icount<<endl;
    cout<<"New inst: "<<inst_info->new_inst<<endl;
    memcpy(&bundle_bits, &inst_info->inst_bytes[0],16);
    cout<<"The bundle bits are: ";
    cout<<fmt("016x", bundle_bits[1]);
    cout<<fmt("016x", bundle_bits[0])<<endl;    
    cout<<"The paddr is: "<<fmt("016x",inst_info->phy_addr)<<endl;
    cout<<"The vaddr is: "<<fmt("016x",inst_info->virt_addr)<<endl;
    cout<<"The flags are: "<<inst_info->eflags<<endl;
    cout<<"The repeat count is: "<<inst_info->cur_repeat_count<<endl;
    cout<<"Branch taken: "<<inst_info->branch_taken<<endl;
    cout<<"Branch vaddr: "<<fmt("016x",inst_info->branch_target_virt_addr)<<endl;
    cout<<"Store inst: "<<inst_info->store_inst<<endl;
    cout<<"Load inst: "<<inst_info->load_inst<<endl;
    cout<<"Branch inst: "<<inst_info->branch_inst<<endl;
    cout<<"mem paddr: "<<fmt("016x",inst_info->phy_mem_addr)<<endl;
    cout<<"mem vaddr: "<<fmt("016x",inst_info->virt_mem_addr)<<endl;
    cout<<"cfm: "<<fmt("016x",inst_info->cfm)<<endl;
    cout<<"bsp: "<<fmt("016x",inst_info->bsp)<<endl;
    cout<<"pred: "<<fmt("016x",inst_info->predicates)<<endl;
}

/***********************************************************************
 *This function will grab the instruction input register values
 *This is essential a copy of the code that is used in the cpu_inst to
 *create the depency information
 ************************************************************************/
void
GAMBIT_FEEDER_CLASS::set_feedbuf_input_reg_vals(UINT32 CPU_NUM, ASIM_INST inst)
{

    struct REGISTER_INFO
    {
        UINT64 value[2];
        INT32 regNum;
        ARCH_REGISTER_TYPE rType;
    };

    REGISTER_INFO srcGp[NUM_SRC_GP_REGS];
    REGISTER_INFO srcPred[NUM_SRC_PRED_REGS];

    UINT64 srcGpDepVal[NUM_SRC_GP_REGS][3];
    UINT64 srcPredDepVal[NUM_SRC_PRED_REGS];

    INT32 num;
    ARCH_REGISTER_TYPE reg_type;
    enum ia64_opnd opnd;

    INT32 p_reg = 0;
    INT32 g_reg = 0;

    //initialize the items to invalid
    for (UINT32 i = 0; i < NUM_SRC_PRED_REGS; i++)
    {
        srcPred[i].regNum=-1;
        srcPred[i].rType=REG_TYPE_INVALID;
        srcPred[i].value[0]=0;
        srcPred[i].value[1]=0;

    }
    for (UINT32 i = 0; i < NUM_SRC_GP_REGS; i++)
    {
        srcGp[i].regNum=-1;
        srcGp[i].rType=REG_TYPE_INVALID;
        srcGp[i].value[0]=0;
        srcGp[i].value[1]=0;

        srcGpDepVal[i][0]=0;
        srcGpDepVal[i][1]=0;
    }


    for (UINT32 i = 0; i < NUM_SRC_PRED_REGS + NUM_SRC_GP_REGS; i++)
    {
        num = -1;
        reg_type = REG_TYPE_INVALID;
        
        REG r = inst->GetRawInst()->RegExplicitRead(i);
        opnd = inst->GetRawInst()->RegOpndExplicitRead(i);
        TRACE(Trace_Feeder, cout << "\t\t\tGetting input reg num:"<<i);

        if (REG_is_pr(r))
        {
            reg_type = REG_TYPE_PREDICATE;
            num = r - REG_PBASE;
//            if (inst->IsValidDependency(num, reg_type))
//            {
                //ASSERTX(opnd != IA64_OPND_NIL);
                srcPred[p_reg].regNum=num;
                srcPred[p_reg].rType=REG_TYPE_PREDICATE;
                TRACE(Trace_Feeder, cout << "\tpr register: "<<num<<endl);
                if(!asim_get_preds(CPU_NUM,&srcPred[p_reg].value[0]))
                {
                    cerr<<"unable to get predicate registers"<<endl;
                }
//                p_reg++;
//            }
//            else
//            {
                TRACE(Trace_Feeder, cout << "\t invalid predicate register: "<<num<<endl); 
//            }
        }
        else 
        {
            if (REG_is_gr(r))
            {
                reg_type = REG_TYPE_INT;
                num = r - REG_GBASE;
                TRACE(Trace_Feeder, cout << "\tgr register: "<<num<<endl);
                if(!asim_get_gr(CPU_NUM, num,&srcGpDepVal[g_reg][0]))
                {
                    cerr<<"unable to get the gr"<<endl;
                }
            }
            else if (REG_is_fr(r))
            {
                reg_type = REG_TYPE_FP82;
                num = r - REG_FBASE;
                TRACE(Trace_Feeder, cout << "\tfr register: "<<num<<endl);
                if(!asim_get_fr(CPU_NUM, num,&srcGpDepVal[g_reg][0]))
                {
                    cerr<<"unable to get fr register"<<endl;
                }

            }
            else if (REG_is_ar(r))
            {
                reg_type = REG_TYPE_AR;
                num = r - REG_ABASE;
                TRACE(Trace_Feeder, cout << "\tar register: "<<num<<endl);
                if(!asim_get_ar(CPU_NUM, num,&srcGpDepVal[g_reg][0]))
                {
                    cerr<<"unable to get the ar register"<<endl;
                }
            }
            else if (REG_is_br(r))
            {
                reg_type = REG_TYPE_BR;
                num = r - REG_BBASE;
                TRACE(Trace_Feeder, cout << "\tbr register: "<<num<<endl);
                if(!asim_get_br(CPU_NUM, num,&srcGpDepVal[g_reg][0]))
                {
                    cerr<<"unable to get the br register"<<endl;
                }
            }
            else if (REG_is_cr(r))
            {
                reg_type = REG_TYPE_CR;
                num = r - REG_CBASE;
                TRACE(Trace_Feeder, cout << "\tcr register: "<<num<<endl);
                if(!asim_get_cr(CPU_NUM, num,&srcGpDepVal[g_reg][0]))
                {
                    cerr<<"unable to get the cr register"<<endl;
                }
            }
            else
            {
                TRACE(Trace_Feeder, cout << "\tno register: "<<num<<endl);
            }
            if (reg_type!=REG_TYPE_INVALID)
            {
 //               ASSERTX(opnd != IA64_OPND_NIL);
                srcGp[g_reg].regNum=num;
                srcGp[g_reg].rType=reg_type;
                srcGp[g_reg].value[0]=srcGpDepVal[g_reg][0];
                srcGp[g_reg].value[1]=srcGpDepVal[g_reg][1];
                g_reg++;
            }
        }

    }

    //
    // Convert REGISTER_INFO to ARCH_REGISTER_CLASS.  Some day we
    // should rewrite the code above to generic ARCH_REGISTER_CLASSes
    // directly.
    //
    ARCH_REGISTER_CLASS gpr[NUM_SRC_GP_REGS];
    for (int i = 0; i < NUM_SRC_GP_REGS; i++)
    {
        gpr[i] = ARCH_REGISTER_CLASS(srcGp[i].rType, srcGp[i].regNum,
                                        srcGp[i].value[0], srcGp[i].value[1]);
    }

    ARCH_REGISTER_CLASS predr[NUM_SRC_PRED_REGS];
    for (int i = 0; i < NUM_SRC_PRED_REGS; i++)
    {
        predr[i] = ARCH_REGISTER_CLASS(srcPred[i].rType, srcPred[i].regNum,
                                          srcPred[i].value[0], srcPred[i].value[1]);
    }

    FEEDER_BUFFER[CPU_NUM].set_input_dep(gpr, predr);

}
/***********************************************************************
 *This function will grab the instruction output register values
 *This is essential a copy of the code that is used in the cpu_inst to
 *create the depency information
 ************************************************************************/
void 
GAMBIT_FEEDER_CLASS::set_feedbuf_output_reg_vals(UINT32 CPU_NUM, ASIM_INST inst)
{
    struct REGISTER_INFO
    {
        UINT64 value[2];
        INT32 regNum;
        ARCH_REGISTER_TYPE rType;
    };

    REGISTER_INFO dst[NUM_DST_REGS];
    UINT64 dstDepVal[NUM_DST_REGS][3];
    
    U128 tmp128;    
    INT32 num;
    ARCH_REGISTER_TYPE reg_type;
    enum ia64_opnd opnd;

    for (UINT32 i = 0; i < NUM_DST_REGS; i++)
    {
        dst[i].regNum=-1;
        dst[i].rType=REG_TYPE_INVALID;
        dst[i].value[0]=0;
        dst[i].value[1]=0;
        num = -1;
        reg_type = REG_TYPE_INVALID;

        REG r = inst->GetRawInst()->RegExplicitWrite(i);
        opnd = inst->GetRawInst()->RegOpndExplicitWrite(i);
        TRACE(Trace_Feeder, cout << "\t\t\tGetting dst reg num:"<<i);

        if (REG_is_pr(r))
        {
            reg_type = REG_TYPE_PREDICATE;
            num = r - REG_PBASE;
            TRACE(Trace_Feeder, cout << "\tpr register: "<<num<<endl);
            if(!asim_get_preds(CPU_NUM,&dstDepVal[i][0]))
            {
                cerr<<"unable to get predicate registers"<<endl;
            }
            dstDepVal[i][1]=0;
        }
        else if (REG_is_gr(r))
        {
            reg_type = REG_TYPE_INT;
            num = r - REG_GBASE;
            TRACE(Trace_Feeder, cout << "\tgr register: "<<num<<endl);

            if(!asim_get_gr(CPU_NUM, num, &dstDepVal[i][0]))
            {
                cerr<<"unable to get gr registers"<<endl;
            }
            dstDepVal[i][1]=0;

        }
        else if (REG_is_fr(r))
        {
            reg_type = REG_TYPE_FP82;
            num = r - REG_FBASE;
            TRACE(Trace_Feeder, cout << "\tfr register: "<<num<<endl);

            if(!asim_get_fr(CPU_NUM, num, &dstDepVal[i][0]))
            {
                cerr<<"unable to get fr registers"<<endl;
            }
        }
        else if (REG_is_ar(r))
        {
            reg_type = REG_TYPE_AR;
            num = r - REG_ABASE;
            TRACE(Trace_Feeder, cout << "\tar register: "<<num<<endl);

            if(!asim_get_ar(CPU_NUM, num, &dstDepVal[i][0]))
            {
                cerr<<"unable to get ar registers"<<endl;
            }
            dstDepVal[i][1]=0;

        }
        else if (REG_is_br(r))
        {
            reg_type = REG_TYPE_BR;
            num = r - REG_BBASE;
            TRACE(Trace_Feeder, cout << "\tbr register: "<<num<<endl);

            if(!asim_get_br(CPU_NUM, num, &dstDepVal[i][0]))
            {
                cerr<<"unable to get br registers"<<endl;
            }
            dstDepVal[i][1]=0;
        }
        else if (REG_is_cr(r))
        {
            reg_type = REG_TYPE_CR;
            num = r - REG_CBASE;
            TRACE(Trace_Feeder, cout << "\tcr register: "<<num<<endl);

            if(!asim_get_cr(CPU_NUM, num, &dstDepVal[i][0]))
            {
                cerr<<"unable to get cr registers"<<endl;
            }
            dstDepVal[i][1]=0;

        }
        else
        {
            TRACE(Trace_Feeder, cout << "\tno register"<<num<<endl);            
        }

//        if (inst->IsValidDependency(num, reg_type))
        if(reg_type != REG_TYPE_INVALID)
        {
            dst[i].regNum=num;
            dst[i].rType=reg_type;
            dst[i].value[0]=dstDepVal[i][0];
            dst[i].value[1]=dstDepVal[i][1];
            //TRACE(Trace_Sys, this->dstDep[i]->DumpTrace());                
        }
    }

    ARCH_REGISTER_CLASS dstr[NUM_DST_REGS];
    for (int i = 0; i < NUM_DST_REGS; i++)
    {
        dstr[i] = ARCH_REGISTER_CLASS(dst[i].rType, dst[i].regNum,
                                         dst[i].value[0], dst[i].value[1]);
    }

    FEEDER_BUFFER[CPU_NUM].set_output_dep(dstr);
}
/**********************************************************
 *Read the bundle bits from memory
 *********************************************************/
bool
GAMBIT_FEEDER_CLASS::Read_BB(UINT32 CPU_NUM, IADDR_CLASS address, UINT64 *bb)
{

    UINT64 paddr;
    if(!asim_v2p(CPU_NUM,address.GetBundleAddr(), &paddr ))
    {
        cerr<<"Unable to get v2p for "<<fmt("016x",address.GetBundleAddr())<<endl;
        return 0;
    }
    if(!asim_mem_read(CPU_NUM,paddr,16,bb))
    {
        cerr<<"Unable to read memory"<<fmt("016x",paddr)<<endl;
        return 0;
    }
    return 1;
}
/*************************************************************
 *We want to step the model in chunks of instructions to increase
 *The performance speed
 ************************************************************/
bool
GAMBIT_FEEDER_CLASS::StepExecuteGroup (UINT32 CPU_NUM)
{
    //increment the buffer position
    buffer_position[CPU_NUM]++;

    //there are no more instructions left in the buffer
    //execute some more
    if(buffer_position[CPU_NUM]>=EXECUTE_GROUP_SIZE)
    {
        if(!asim_step_ref(CPU_NUM,&all_inst_info[CPU_NUM][0],EXECUTE_GROUP_SIZE))
        {
            cerr<<"Unable to step any more instructions"<<endl;
            return 0;
        }
        else
        {
            buffer_position[CPU_NUM]=0;
            return 1;
        }
    }
    
    return 1;

}

/*************************************************************
 *Return the pointer to the current instruction in the execute group
 ************************************************************/
cpuapi_inst_info_t *
GAMBIT_FEEDER_CLASS::GetCurrentInExecuteGroup(UINT32 CPU_NUM)
{
    UINT32 position=buffer_position[CPU_NUM];
    ASSERTX(position<EXECUTE_GROUP_SIZE);
    return (&all_inst_info[CPU_NUM][position]);
}

/*************************************************************
 *Get the value of the cfm after this instruction executed
 *This is a work around a bug in which the cfm appears not to be
 *updated when an alloc occurs
 ************************************************************/
UINT64
GAMBIT_FEEDER_CLASS::GetNextCFM(UINT32 CPU_NUM)
{
    UINT32 position=buffer_position[CPU_NUM];
    ASSERTX(position<EXECUTE_GROUP_SIZE);
    UINT64 cfm=0;
    if(position==(EXECUTE_GROUP_SIZE-1))
    {
        if(!asim_get_cfm(CPU_NUM, &cfm))
        {
            cerr<<"Unable to get the cfm"<<endl;
            
        }        
        return cfm;
    }
    else
    {
        return all_inst_info[CPU_NUM][position+1].cfm;
    }

}
/*************************************************************
 *Get the next PC either from the next instruction
 *or by looking it up
 ************************************************************/
UINT64
GAMBIT_FEEDER_CLASS::GetNextPC(UINT32 CPU_NUM)
{
    UINT32 position=buffer_position[CPU_NUM];
    ASSERTX(position<EXECUTE_GROUP_SIZE);
    UINT64 pc=0;
    if(position==(EXECUTE_GROUP_SIZE-1))
    {
        if(!asim_get_pc(CPU_NUM, &pc))
        {
            cerr<<"Unable to get the pc"<<endl;
            
        }        
        return pc;
    }
    else
    {
        return all_inst_info[CPU_NUM][position+1].virt_addr;
    }

}

/************************************************************
 *
 *SOURCES FOR THE INSTRUCTION INFORMATION
 *
 *Refeed- when front end is resteered correct instructions are refeed into the model
 *Convert-Correct Path - deprecated ..used hdb functions
 *Convert-Correct Path Speed - model stepped in groups specified in awb file
 *Convert-Wrong Path  - model not stepped bundle bits read from memory
 *
 *************************************************************/

void 
GAMBIT_FEEDER_CLASS::Refeed(UINT32 CPU_NUM,ASIM_INST inst)
{
    instruction_fi q_inst=FEEDER_BUFFER[CPU_NUM].getmispec();
    IADDR_CLASS tmp_pc;
    bool trap=false;

    if(!q_inst)
    {
        ASIMERROR("Gambit_FEEDER: Invalid Pointer to instruction info on refeed"<<endl);
    }
    //hand the pc and bundle bits to asim for decode
    TRACE(Trace_Inst, cout<<"\t");
    inst->Init(q_inst->bundle_bits, q_inst->pc);

    if (inst->IsNOP())
    {
        q_inst->nop =true;
    }

    if (inst->IsTypeL())
    {
        q_inst->longImm =true;
    }

    if (inst->HasEffAddress())
    {
        
        inst->SetVirtualEffAddress(q_inst->inst_info.virt_mem_addr);
        inst->SetPhysicalEffAddress(q_inst->inst_info.phy_mem_addr);
        
        /* recording the instructions to use on the wrong path*/
        LAST_PHY_ADDRESS[CPU_NUM]=q_inst->inst_info.phy_mem_addr;
        LAST_VIR_ADDRESS[CPU_NUM]=q_inst->inst_info.virt_mem_addr;
    }
 
    if(!(((q_inst->bundle_bits[0] & 0x1e) == 0x04) && (q_inst->pc.GetSyllableIndex() == 1)))
    {
        LAST_INST_WAS_MLX[CPU_NUM]=FALSE;
    }
    else
    {
       LAST_INST_WAS_MLX[CPU_NUM]=TRUE;
    }

    // regular synchronous control flow changes
    if (inst->IsControlOp())
    {
        inst->SetActualTarget(q_inst->npc);
        //If it is a not take branch deal with the 
        if(!(q_inst->inst_info.branch_taken))
        {
            //check to see if the address is the next ip
            if(q_inst->npc==q_inst->pc.Next())
            {
                inst->SetActualTaken(false);
            }
            //else it is an interrupt
            else
            {
                inst->SetActualTaken(true);
                inst->SetFeederContextSwitch(true);
                inst->SetNonSequentialPc(q_inst->npc);
            }
        }
        //if the branch was taken look to see if the target lines up with the npc
        else
        {
            //ignore the lower 4 bits
            if(((q_inst->inst_info.branch_target_virt_addr)|0x0F )==((q_inst->npc.GetBundleAddr())|0x0F))
            {
               inst->SetActualTaken(true); 
            }
            else
            {
                inst->SetActualTaken(true);
                inst->SetFeederContextSwitch(true);
                inst->SetNonSequentialPc(q_inst->npc);

            }
        }
            LAST_INST_WAS_CONTROL[CPU_NUM]=TRUE;
    }
    else
    {
        LAST_INST_WAS_CONTROL[CPU_NUM]=FALSE;
    }


    // set CFM regardless of instruction type
    inst->SetCFM(q_inst->oldcfm, q_inst->cfm);

    // set the predicate register file
    inst->SetPRF(q_inst->oldpr,q_inst->pr);
   
    //set the BSP
    inst->SetBSP(q_inst->oldbsp,q_inst->bsp);


    LAST_CFM[CPU_NUM]=q_inst->cfm;
    LAST_BSP[CPU_NUM]=q_inst->bsp;
    LAST_PRED[CPU_NUM]=q_inst->pr;

    //cout<<"Refeed instruction id: "<<q_inst->Uid<<"with id: "<<inst->GetUid()<<endl; 
    TRACE(Trace_Feeder, cout<<"\t\tNPC: "<<q_inst->npc<<endl);

    /* UPDATE THE UID if we have to roll back! */
    if(!FEEDER_BUFFER[CPU_NUM].rename(inst->GetUid()))    
    {
        ASIMERROR("Gambit_FEEDER: Could not reset the Uid on refeed"<<endl);
    } 
    
    FEEDER_BUFFER[CPU_NUM].increment_mispec_pt();
    if(FEEDER_BUFFER[CPU_NUM].get_misp_inst_left())
    {
        //we may have inserted a patch up branch for the interrupt
        q_inst=FEEDER_BUFFER[CPU_NUM].getmispec();
        NEXT_IP[CPU_NUM]=q_inst->pc; 
    }
    else
    {
        NEXT_IP[CPU_NUM]=q_inst->npc; 
    }
}

void 
GAMBIT_FEEDER_CLASS::ConvertWrongPath (UINT32 CPU_NUM, ASIM_INST inst,const IADDR_CLASS predicted_pc)
{
    UINT64 bundle_bits[2];
    UINT64 cfm;
    UINT64 pr;
    UINT64 bsp;
    UINT64 bundleaddress= predicted_pc.GetBundleAddr();
    UINT64 paddr=0;

    if(!Read_BB(CPU_NUM, predicted_pc, &bundle_bits[0]))
    {
        cerr<<"Unable to get the bundle bits!!"<<endl;
        //if we could not get the bundle bits then make it a NOP
        memcpy(&bundle_bits,&NOP,16);
    }
    

    //hand the pc and bundle bits to asim for decode
    inst->Init(bundle_bits, predicted_pc);
    
    if (inst->HasEffAddress())
    {
        inst->SetVirtualEffAddress(LAST_PHY_ADDRESS[CPU_NUM]);
        inst->SetPhysicalEffAddress(LAST_VIR_ADDRESS[CPU_NUM]);
    }

    // THESE WILL BE BOGUS ON THE WRONG PATH
    if (inst->IsControlOp())
    {
        inst->SetActualTarget(predicted_pc.Next());
        inst->SetActualTaken(false);        
    }
    //Get the CFM, BSP and PREDS
    if(!asim_get_cfm(CPU_NUM, &cfm))
    {
        cerr<<"Unable to get the cfm"<<endl;
    }
    if(!asim_get_preds(CPU_NUM, &pr))
    {
        cerr<<"Unable to get the pr"<<endl;
    }
    //pr=UnrotatePreds(pr,cfm);
    if(!asim_get_bsp(CPU_NUM, &bsp))
    {
        cerr<<"Unable to get the bsp"<<endl;
    }

    // set CFM regardless of instruction type
    inst->SetCFM(cfm, cfm);

    // set the predicate register file
    inst->SetPRF(pr,pr);
    
    //set the BSP
    inst->SetBSP(bsp,bsp);
    
}  


/***************************************************
 *making the instruction for the correct path
 *PC oldcfm and oldpr should actually be generated in
 *here.  This returns false if the thread has ended.
 ******************************************************/

bool
GAMBIT_FEEDER_CLASS::ConvertCorrectPath (UINT32 CPU_NUM, ASIM_INST inst,
                                         IADDR_CLASS PC)
{
    ASSERT(0,"depricated function");
    return 0;
}

/***************************************************
 *
 ******************************************************/

bool
GAMBIT_FEEDER_CLASS::ConvertCorrectPathSpeed (UINT32 CPU_NUM, ASIM_INST inst,
                                         IADDR_CLASS PC)
{
    UINT64 bundle_bits[2]; //the instruction bundle bits
    IADDR_CLASS next_pc; //the next pc
    UINT64 cfm; //the cfm after the step
    UINT64 oldcfm; //the cfm before the step
    UINT64 oldpr; //the predicate register file before the step
    UINT64 oldbsp; //the bsp before the step
    cpuapi_inst_info_t* inst_info; //information about the instruction
    UINT32 retval = false; //the return value of the step


/*******************************************************************
* BEFORE STEP REFERENCE!
* Before the step. Then get the values of the old cfm, bsp and preds
*********************************************************************/

    oldcfm=LAST_CFM[CPU_NUM];
    oldbsp=LAST_BSP[CPU_NUM];
    oldpr=LAST_PRED[CPU_NUM];

    //if we are doing checks then make sure the cfm and preds look ok
    if (CONFIG_CHECK)
    {
        //check the cfm and preds
        UINT64 TESTCFM;
        UINT64 TESTPR;
        if(!asim_get_cfm(CPU_NUM, &TESTCFM))
        {
            cerr<<"Unable to get the cfm"<<endl;
        }
        if(!asim_get_preds(CPU_NUM, &TESTPR))
        {
            cerr<<"Unable to get the pr"<<endl;
        }
        if(oldcfm!=TESTCFM)
        {
            POSSIBLE_ERROR_IN_CFM[CPU_NUM]++;
            cout<<"The cfm doesn't match"<<endl;
        }
        if(oldpr!=TESTPR)
        {
            POSSIBLE_ERROR_IN_PR[CPU_NUM]++;
            cout<<"The preds don't match"<<endl;
        }
    }

    FEEDER_BUFFER[CPU_NUM].set_old_bsp(oldbsp);

#ifndef V2P_WORKAROUND
    if(RECORD_REGISTERS)
    {
        TRACE(Trace_Feeder,
             cout << "\t\tRecording Input registers"<< endl); 
        UINT64 local_bb[2];
        if(!Read_BB(CPU_NUM, PC, &local_bb[0]))
        {
           local_bb[0]=NOP[0];
           local_bb[1]=NOP[1];
        }
        TRACE(Trace_Inst,cout<<"\t");
        inst->Init( (&local_bb[0]), PC);
        set_feedbuf_input_reg_vals(CPU_NUM,inst);
    }
    
#endif

/*******************************************************************
* STEP REFERENCE!
* Step the reference if we are not on the second syllable of a long
* syllable instruction. 
* SET the return value
* and End the thread if we have to
*********************************************************************/
    if(!LAST_INST_WAS_MLX[CPU_NUM])
    {

        if(!StepExecuteGroup(CPU_NUM))
        {
            retval=false;
            TRACE(Trace_Feeder,
                  cout << "\t\tEnding thread, uid = " << CPU_NUM << endl); 
            thread[CPU_NUM]->ThreadEnd();
            inst->Init(NOP,PC);    
            FEEDER_BUFFER[CPU_NUM].set_nop(true);
            Fabricate_NOP(PC,inst,CPU_NUM);            
            return false;
        }
        else
        {
            retval=true;
            TRACE(Trace_Feeder,
                  cout << "\t\tStepped cpu # "<<CPU_NUM << endl);              
        }
    }
    else
    {
        retval=true;
        TRACE(Trace_Feeder,cout<<"\t\tDIDN'T STEP REFERENCE BECAUSE MLX"<<endl);
    }

    //grab the instruction information from the execute group buffer
    inst_info=GetCurrentInExecuteGroup(CPU_NUM);

/*****************************************************************
 *AFTER STEP
 * Process the instruction information structure
 *****************************************************************/
    
#ifndef V2P_WORKAROUND
    //If we are recording the registers then we should have already
    //inited the instruction so that we could have found out the 
    //registers that it used
    if(RECORD_REGISTERS)
    {
        TRACE(Trace_Feeder,cout<<"\t\tRecording Output Registers"<<endl);
        set_feedbuf_output_reg_vals(CPU_NUM,inst);
    }
    else
    {
        TRACE(Trace_Inst,cout<<"\t");
        inst->Init((UINT64*) &(inst_info->inst_bytes[0]), PC);
    }
#else
    //If we are recording the registers then we should have already
    //inited the instruction so that we could have found out the 
    //registers that it used
    if(RECORD_REGISTERS)
    {
        TRACE(Trace_Feeder,cout<<"\t\tRecording Output Registers"<<endl);
        inst->Init((UINT64*) &(inst_info->inst_bytes[0]), PC);
        set_feedbuf_input_reg_vals(CPU_NUM,inst);
        set_feedbuf_output_reg_vals(CPU_NUM,inst);
    }
    else
    {

        TRACE(Trace_Inst,cout<<"\t");
        inst->Init((UINT64*) &(inst_info->inst_bytes[0]), PC);
    }

#endif

    /********Decide what the next pc should be ***********************/

    next_pc.Set(GetNextPC(CPU_NUM),GetNextPC(CPU_NUM));
    TRACE(Trace_Feeder,cout<<"\t\tReference Model NPC:"<<next_pc<<endl);
        

    //Find long syllables
    if (!(((inst_info->inst_bytes[0] & 0x1e) == 0x04) && (PC.GetSyllableIndex() == 1)))
    {
        //TRACE(Trace_Feeder,cout<<"\tnot a long syllable"<<endl);
        LAST_INST_WAS_MLX[CPU_NUM]=FALSE;
    }
    else //This instruction is a MLX so make sure we do a feed fetch
         //on the data portion
    {
        
        next_pc=PC.Next();
        TRACE(Trace_Feeder,cout<<"\t\tThis is a long syllable instruction so overriding pc with"<<next_pc<<endl);
        LAST_INST_WAS_MLX[CPU_NUM]=TRUE;
    }





/***************************************
*Instruction Specific Actions
*****************************************/

    /**************************************************
     *Look at the Instruction Type
     *FILL in required fields for type:
     *a)MEMOPS - fill in the address
     *b)Control OPS - fill in the target address
     *Handle Special Cases:
     *a)Interrupt on ALLOC- we get the cfm AFTER the interrupt, 
     *so
     *b)NOP - tell the refeed buffer that this is a NOP so that
     *it can be skipped if we are not committing these 
     *c)Long immediate- same as above
     *d)Control OPS- handle interrupts
     ***************************************************/
    //alloc
    if(inst->IsAlloc())
    {
        if (next_pc!=PC.Next())
        {
        
            TRACE(Trace_Feeder,cout<<"\t\tInterrupt on alloc"<< 
                  "use a fake huge frame"<<endl);
            cfm= oldcfm | 0x03fff;
        }
        else
        {
            cfm=GetNextCFM(CPU_NUM);
        }
    }
    else
    {
        cfm=inst_info->cfm;
    }

    //NOPs
    if (inst->IsNOP())
    {
        FEEDER_BUFFER[CPU_NUM].set_nop(true);
    }

    //Long Immediates
    if (inst->IsTypeL())
    {
        FEEDER_BUFFER[CPU_NUM].set_longImm(true);
    }

    //Memory OPs
    if (inst->HasEffAddress())
    {
        //in user mode set the pa with the page coloring 
        if(!os_mode)
        {    
            //Make sure they have not enhanced softsdv to have a pa in user mode
           ASSERTX(inst_info->virt_mem_addr==inst_info->phy_mem_addr)        
           inst->SetVirtualEffAddress(inst_info->virt_mem_addr);  
           UINT64 pa;
           DTranslate(inst,inst_info->virt_mem_addr,pa); 
           inst->SetPhysicalEffAddress(pa);
        }
        else
        {
            inst->SetVirtualEffAddress(inst_info->virt_mem_addr);
            inst->SetPhysicalEffAddress(inst_info->phy_mem_addr);
        }

        // recording the instructions to use on the wrong path
        LAST_PHY_ADDRESS[CPU_NUM]=inst_info->phy_mem_addr;
        LAST_VIR_ADDRESS[CPU_NUM]=inst_info->virt_mem_addr;
    }

    // Control OPs
    if (inst->IsControlOp())
    {
        inst->SetActualTarget(next_pc);

        if(!(inst_info->branch_taken))
        {
            //check to see if the address is the next ip
            if(next_pc==PC.Next())
            {
                inst->SetActualTaken(false);
            }
            //else it is an interrupt
            else
            {
                inst->SetActualTaken(true);
                inst->SetFeederContextSwitch(true);
                inst->SetNonSequentialPc(next_pc);
            }            
        }
        //if it was taken make sure the address is correct
        else
        {
            //ignore the lower 4 bits
            if(((inst_info->branch_target_virt_addr)|0x0F )==((next_pc.GetBundleAddr())|0x0F))
            {
                inst->SetActualTaken(true); 
            }
            else
            {
                inst->SetActualTaken(true);
                inst->SetFeederContextSwitch(true);
                inst->SetNonSequentialPc(next_pc);
            }
        }
        LAST_INST_WAS_CONTROL[CPU_NUM]=TRUE;
    }
    else
    {
       LAST_INST_WAS_CONTROL[CPU_NUM]=FALSE;
    }

/***************************************
*General Instruction Fields
*****************************************/

    TRACE(Trace_Feeder,
          cout <<"\t\tNew CFM: "<<fmt("016x",cfm)<<" Old CFM: "<<fmt("016x",oldcfm)<<endl);

    //Store these values for the next time we come in here (quicker)
    LAST_CFM[CPU_NUM]=cfm;
    LAST_BSP[CPU_NUM]=inst_info->bsp;
    
    UINT64 corrected_preds=UnrotatePreds(inst_info->predicates,cfm);
    LAST_PRED[CPU_NUM]=corrected_preds;

     // set CFM regardless of instruction type
    inst->SetCFM(oldcfm,cfm);

    // set the predicate register file
    inst->SetPRF(oldpr,corrected_preds);
    TRACE(Trace_Feeder,
          cout <<"\t\tNew Preds: "<<fmt("016x",corrected_preds)<<" Old Preds: "<<fmt("016x",oldpr)<<endl);

//   UINT64 preds_from_vis;
//   asim_get_preds(CPU_NUM,&preds_from_vis);
//   TRACE(Trace_Feeder,
//          cout <<"\t\tPreds from visability: "<<fmt("016x",preds_from_vis)<<endl);


    //check to see if there is an address 
    if(CONFIG_CHECK&&inst->GetQP()&&inst->HasEffAddress()&&!inst->IsPrefetch())
    {
       if(!(inst_info->virt_mem_addr)||!(inst_info->phy_mem_addr))
       {
           ZERO_ADDRESS_MEM_OPS[CPU_NUM]++;

           cout<<"Warning zero address on valid memop"<<endl;
           cout<<"The instruction was"<<inst->GetRawInst()->DisasmString()<<endl;
       }
    }


    FEEDER_BUFFER[CPU_NUM].set_bsp(inst_info->bsp);
    inst->SetBSP(oldbsp,inst_info->bsp);
    TRACE(Trace_Feeder,
          cout << "\t\tNew BSP: "<<fmt("016x",inst_info->bsp)<<" Old BSP: "<<fmt("016x",oldbsp)<< endl);

/*******************************
*Add to refeed buffer 
*******************************/
//    cout<<"The QP is"<<inst->GetQP()<<endl;
    //This must be the last feed_buffer action since it moves
    //the tail position
    FEEDER_BUFFER[CPU_NUM].add(inst_info,(UINT64*) &inst_info->inst_bytes,
        PC,next_pc,oldcfm,cfm,oldpr,corrected_preds,inst->GetUid());
   
    NEXT_IP[CPU_NUM]=next_pc; 

    return retval;
}




/******************************************************
 *Seperate the softsdv and asim arguments
 ******************************************************/
bool 
GAMBIT_FEEDER_CLASS::Parse (UINT32 &argc, char **argv)
/*
 * Parse command line arguments, return FALSE if there is
 * a parse error.
 */
{
    bool cfg_file_seen=0;
    bool executable_seen=0;
    bool stdout_seen=0;
    bool stderr_seen=0;
    bool  stdin_seen=0;
    if(argc<2)
      {
          fprintf(stderr,"you do not appear to have enough arguments");
          IFEEDER_Usage (stderr);
          exit(1);
      }
    
    //this is going to be some simple checks.. and take out the asim feeder options   
    for (UINT32 i = 0; i < argc; i++) 
    {
        //os and user mode args
        if (!strcmp(argv[i], "-rc_file=")) 
        {
            cfg_file_seen=1; 
        }
        //User mode args
        if(!strcmp(argv[i], "-stdout="))
        {
            stdout_seen=1;
        }
        else if(!strcmp(argv[i], "-stderr="))
        {
            stderr_seen=1;
        }
        else if(!strcmp(argv[i], "-stdin="))
        {
            stdin_seen=1;
        }
        //these valeus are passed to softsdv, but the feeder needs to know
        //about them
        else if(!strcmp(argv[i], "-os"))
        {
            os_mode=1;
        }
        else if(!strncmp(argv[i], "-num_cpus=",10))
        {
            number_of_cpus=0;
            //if there is a space then look at argv[i++]
            if (argv[i][10]=='\0')
            {
                
                if((argv[i+1][0]>='0')&&(argv[i+1][0]<='9'))
                {
                    i++;
                    for(UINT32 j=0; (argv[i][j]!=0)&& (j<2); j++)
                    {
                        number_of_cpus*=10;
                        number_of_cpus+=(UINT32)argv[i][j]-'0';
                    }
                }                
                else
                {
                    ASIMERROR("There is not a valid number of cpus");
                }   
            }
            else if((argv[i][10]>='0')&&(argv[i][10]<='9'))
            {
                for(UINT32 j=10; (argv[i][j]!=0)&& (j<12); j++)
                {
                    number_of_cpus*=10;
                    number_of_cpus+=(UINT32)argv[i][j]-'0';
                } 
            }
            else
            {
                ASIMERROR("There is not a valid number of cpus");
            }   
        }
        //feeder only args
        else if(!strcmp(argv[i], "-help"))        
        {
            Usage_Verbose();
        }
        else if(!strncmp(argv[i], "-restore=",9))
        {
            //grab the file name from the end of this string or the next one
            if(argv[i][9]!='\0')
            {
                restore_file = new char[strlen(argv[i])-8];
                for(UINT32 j=0;j< (strlen(argv[i])-8); j++)
                    restore_file[j]=argv[i][j+9];

                argv[i]=NULL;
                for(UINT32 j=i;j< (argc-1); j++)
                {
                    argv[j]=argv[j+1];
                    
                }
                argc=argc-1;
                i--;
            }
            else
            {
                restore_file = new char[strlen(argv[i+1])+1];
                strcpy(restore_file,argv[i+1]);
                argv[i]=NULL;
                for(UINT32 j=i;j< (argc-2); j++)
                {
                    argv[j]=argv[j+2];
                    
                }
                argc=argc-2;
                i--;
            }
        }
        else if(!strncmp(argv[i], "-hdb_script=",12))
        {
            //grab the file name from the end of this string or the next one
            if(argv[i][12]!='\0')
            {
                include_file = new char[strlen(argv[i])-11];
                for(UINT32 j=0;j< (strlen(argv[i])-11); j++)
                    include_file[j]=argv[i][j+12];

                argv[i]=NULL;
                for(UINT32 j=i;j< (argc-1); j++)
                {
                    argv[j]=argv[j+1];
                    
                }
                argc=argc-1;
                i--;
            }
            else
            {
                include_file = new char[strlen(argv[i+1])+1];
                strcpy(include_file,argv[i+1]);
                argv[i]=NULL;
                for(UINT32 j=i;j< (argc-2); j++)
                {
                    argv[j]=argv[j+2];
                    
                }
                argc=argc-2;
                i--;
            }
        }
        else if(!strcmp(argv[i], "-checkpoint"))
        {
            save_file=new char[strlen(argv[i+1])+1];
            strcpy(save_file,argv[i+1]);
            first_save_num=atoll(argv[i+2]);
            inc_save_num=atoll(argv[i+3]);

            argv[i]='\0';
            for(UINT32 j=i;j< (argc-4); j++)
            {
                argv[j]=argv[j+4];

            }
            argc=argc-4;
            i--;
        }
        else
        {
        }
        
    }
    
    UINT32 tmp_count=cfg_file_seen+stdout_seen+stderr_seen+stdin_seen+1;

    //Simple error checking for user mode
    if ((argc<(tmp_count))&&!os_mode)
    {
        cerr<<"you do not appear to have enough arguments for user mode"<<endl;
        IFEEDER_Usage(stderr);
        exit(1);
    }

  return(true);
}




/********************************************************************
 *
 * FEEDer callbacks
 *
 *******************************************************************/

UINT64
GAMBIT_FEEDER_CLASS::Skip(
    IFEEDER_STREAM_HANDLE stream,
    UINT64 n,
    INT32 markerID)
{
    int retval;
    UINT64 skipped = 0;
    U64 z = U64_ZERO;
    U64 gambit_pc, marker_pc;

    UINT32 tid = STREAM_HANDLE(stream);

    // get the current instruction count in gambit
    UINT64 start_icount = ICNT(tid);
    UINT64 last_icount = 0;
    UINT64  this_icount=0;
    // if n is zero then we want to skip to the end
    if (n == 0)
    {
        n = UINT64_MAX;
        cout << "Skipping all instructions" << endl;
    }
    else
    {
        cout << "Skipping " << n << " instructions" << endl;
    }


    // if there is a marker then set a break point and run
    if (markerID >= 0)
    {
        // this is the hack to construct the pc using the upper bits of
        // the current pc and the lower bits from the marker
        gambit_pc = hdb_get_ip();
        UINT64 tmp_marker_pc = (marker.GetMarkerPc(tid, markerID)) >> 32;
        marker_pc.high_w = tmp_marker_pc;
        marker_pc.low_w = marker.GetMarkerPc(tid, markerID);

        U64 gambit_u64 = hdb_get_ip();
        //this is the hack to construct the pc using the upper bits of
        //the current pc and the lower 
        U64 skip_2_pc;
        skip_2_pc.high_w = gambit_u64.high_w;
        skip_2_pc.low_w = marker.GetMarkerPc(tid, markerID);

        cout << "...or until marker " << markerID << endl;
        cout << "at IP: 0x"<< fmt("016x", u64touint64(marker_pc)) << endl;
        // set the break point at this pc
        hdb_set_break(U64_ZERO, marker_pc, GE_BRKPT_FETCH, HDB_MODE_VIRTUAL, 0);
    }
    switchtofunctional();

    // skipping loop - only skip 10 million at a time to give status
    while (skipped < n)
    {
        UINT64 interval;
 
        //check to see if there is less then 10 million instructions left
        interval = ((n-skipped) < 10000000) ? (n-skipped) : 10000000;
 
        // make a U64 from the UINT64
        z.low_w = (UINT32) interval;
        z.high_w = (UINT32) (interval >> 32);       
        retval = asim_step(z);

        // figure out how far we skipped this iteration
        this_icount = ICNT(tid);
        skipped = this_icount - start_icount;

        cout << "Skipped " << skipped << " instructions" << endl;
 
        //did we hit the pc?
        if (markerID >= 0)
        {
            gambit_pc = hdb_get_ip();
            if ((gambit_pc.high_w == marker_pc.high_w) &&
                (gambit_pc.low_w == marker_pc.low_w))
            {
                break;
            }
        }
        //if we didn't do the whole interval then gambit had an issue or the
        //program is done. so break
        if ((this_icount - last_icount) < interval)
        {
            break;
        }
        last_icount = this_icount;

    }

    // clear any previous break points
    hdb_remove_all_brk(GE_BRKPT_FETCH);

    // let the fetch know that we have not started a correct path yet
    for(UINT32 j=0; j<FEED_MAX_CPU; j++)
    {
        justSkipped[j] = true;
    }

    // let the user know where they are
    Current_Location();

    switchtoperformance();
    //reset the stream variables so that the performance model re-syncs
    for(UINT32 t = 0; t < number_of_cpus; t++) {
        UINT64 NPC;
        UINT64 cfm=0;
        UINT64 pr=0;;
        UINT64 bsp=0;
        if(!asim_get_pc(t,&NPC))
        {
            ASIMERROR("Gambit_FEEDER: could not get the ip"<<endl);
        }
        NEXT_IP[t].Set(NPC,NPC);
        justSkipped[t] = true;
        wrongPath[t] = false;
        LAST_INST_WAS_CONTROL[t]=false;  
        LAST_INST_WAS_MLX[t]=false;

        if(!asim_get_cfm(t, &cfm))
        {
            cerr<<"Unable to get the cfm"<<endl;
        }
        if(!asim_get_preds(t, &pr))
        {
            cerr<<"Unable to get the pr"<<endl;
        }
        //pr=UnrotatePreds(pr,cfm);
        if(!asim_get_bsp(t, &bsp))
        {
            cerr<<"Unable to get the bsp"<<endl;
        }
        LAST_CFM[t]=cfm;
        LAST_BSP[t]=bsp;
        LAST_PRED[t]=pr;
        //force it to step a new group of instructions
        buffer_position[t]=EXECUTE_GROUP_SIZE;
    }
    return skipped;

}



/****************************************************************
 *create a branch instruction to the address at tmp_pc
 ****************************************************************/
void
GAMBIT_FEEDER_CLASS::Fabricate_Branch(const IADDR_CLASS tmp_pc, ASIM_INST inst,const UINT64 sid)
{
    UINT64 cfm,bsp,pr;
    // now generate a valid ASIM branch instruction 
    //FIXME: right now we are nulling the lower bits so the branch bundle
    //decodes.. really need to make a different bundle
    inst->Init(THREE_BRANCH_BUNDLE, tmp_pc);        

    cpuapi_inst_info_t inst_info;
    memset(&inst_info,0,sizeof(cpuapi_inst_info_t));
    inst_info.branch_taken=1;

    ASSERTX(inst->IsControlOp());
    inst->SetActualTarget(NEXT_IP[sid]);
    inst->SetActualTaken(true);
    TRACE(Trace_Feeder, cout << "\tThe Branch target is:"<<NEXT_IP[sid]<<endl);
    //   ... and set the trace ID (for interface betwen PM and FEEDER)
    inst->SetTraceID(traceArtificialInstId);

    //Get the CFM, BSP and PREDS
    if(!asim_get_cfm(sid, &cfm))
    {
        cerr<<"Unable to get the cfm"<<endl;
    }
    if(!asim_get_preds(sid, &pr))
    {
        cerr<<"Unable to get the pr"<<endl;
    }
    //pr=UnrotatePreds(pr,cfm);
    if(!asim_get_bsp(sid, &bsp))
    {
        cerr<<"Unable to get the bsp"<<endl;
    }

    inst->SetBSP(bsp,bsp);
    inst->SetPRF(pr,pr);
    inst->SetCFM(cfm, cfm);

    FEEDER_BUFFER[sid].add(&inst_info,BRANCH_BUNDLE,
                               tmp_pc,NEXT_IP[sid],cfm,cfm,pr,pr,inst->GetUid());


    LAST_CFM[sid]=cfm;
    LAST_BSP[sid]=bsp;
    LAST_PRED[sid]=pr;

    LAST_INST_WAS_CONTROL[sid]=TRUE;
    LAST_INST_WAS_MLX[sid]=FALSE;

    if(TRACE_INST_LEVEL&0x010)
    {
        outfile<<"CPUNUM: "<<sid<<" ,type: FB ,inst: "<<inst->GetRawInst()->DisasmString()<<endl;
    }
}

/*******************************************************
 *Fabricate Context switch instruction
 *******************************************************/
void
GAMBIT_FEEDER_CLASS::Fabricate_CS(const IADDR_CLASS tmp_pc, ASIM_INST inst,const UINT64 sid)
{
    UINT64 cfm,bsp,pr;

    TRACE(Trace_Feeder, cout << "Context switch requested.  Creating a NOP to represent a context switch." << endl);
    inst->Init(NOP,tmp_pc);        
 

    inst->SetTraceID(traceArtificialInstId);
    //Get the CFM, BSP and PREDS
    if(!asim_get_cfm(sid, &cfm))
    {
        cerr<<"Unable to get the cfm"<<endl;
    }
    if(!asim_get_preds(sid, &pr))
    {
        cerr<<"Unable to get the pr"<<endl;
    }
    //pr=UnrotatePreds(pr,cfm);
    if(!asim_get_bsp(sid, &bsp))
    {
        cerr<<"Unable to get the bsp"<<endl;
    }
    inst->SetBSP(bsp,bsp);
    inst->SetPRF(pr,pr);
    inst->SetCFM(cfm, cfm);
    LAST_CFM[sid]=cfm;
    LAST_BSP[sid]=bsp;
    LAST_PRED[sid]=pr;
    LAST_INST_WAS_CONTROL[sid]=FALSE;
    LAST_INST_WAS_MLX[sid]=FALSE;
    if(TRACE_INST_LEVEL&0x008)
    {
        outfile<<"CPUNUM: "<<sid<<" ,type: FC ,inst: "<<inst->GetRawInst()->DisasmString()<<endl;
    }
}

/***************************************************
 *If the thread has ended send the performance model NOPs
 *until it stops asking
 ***************************************************/
void
GAMBIT_FEEDER_CLASS::Fabricate_NOP(const IADDR_CLASS tmp_pc, ASIM_INST inst,const UINT64 sid)
{
    UINT64 cfm,bsp,pr;
    cpuapi_inst_info_t inst_info;
    memset(&inst_info,0,sizeof(cpuapi_inst_info_t));

    TRACE(Trace_Feeder, cout << "Thread ending Creating a NOP" << endl);
    inst->Init(NOP,tmp_pc);        
 
    inst->SetTraceID(traceArtificialInstId);

    //Get the CFM, BSP and PREDS
    if(!asim_get_cfm(sid, &cfm))
    {
        cerr<<"Unable to get the cfm"<<endl;
    }
    if(!asim_get_preds(sid, &pr))
    {
        cerr<<"Unable to get the pr"<<endl;
    }
    //pr=UnrotatePreds(pr,cfm);
    if(!asim_get_bsp(sid, &bsp))
    {
        cerr<<"Unable to get the bsp"<<endl;
    }
    inst->SetBSP(bsp,bsp);
    inst->SetPRF(pr,pr);
    inst->SetCFM(cfm, cfm);
    LAST_CFM[sid]=cfm;
    LAST_BSP[sid]=bsp;
    LAST_PRED[sid]=pr;
    LAST_INST_WAS_CONTROL[sid]=FALSE;
    LAST_INST_WAS_MLX[sid]=FALSE;
    if(TRACE_INST_LEVEL&0x008)
    {
        outfile<<"CPUNUM: "<<sid<<" ,type: FC ,inst: "<<inst->GetRawInst()->DisasmString()<<endl;
    }    
    FEEDER_BUFFER[sid].add(&inst_info, NOP,
                               tmp_pc,tmp_pc,cfm,cfm,pr,pr,inst->GetUid());
 
}


/*******************************************************************
 *Feed Fetch is responsible for taking the prediction and returning the bundle
 *bits and other asim necessary information. The feeder treats the instruction
 *with this priority 
 *a) is it a context switch-return a nop
 *b) is it a thread start-up, immediate after a skip or interrupt - fabricate
 *branch to new location
 *c) is it a non-long syllable good path instruction - step reference and return
 *data
 *d) is it a long syllable instruction
 *e)wrong path - either look up address or return a fabricated
 *instruction
 * Return  0  on then end of a thread and 1 if fetch ok
 *******************************************************************/
bool
GAMBIT_FEEDER_CLASS::Fetch(
    IFEEDER_STREAM_HANDLE stream,
    IADDR_CLASS predicted_pc,
    ASIM_INST inst)
/*
 * Fetch 'inst'...
 */
{
    UINT32 sid = STREAM_HANDLE(stream);

    TRACE(Trace_Feeder,cout << "\tFEED_Fetch PC-" << sid
          << ":" << predicted_pc << endl); 

    TRACE(Trace_Feeder,cout << "\tREFERENCE IP-"<<sid
          << ":" << NEXT_IP[sid]<< endl); 

    TOTAL_FETCH_COUNT[sid]++;
    
    // if the context switch flag is set, then all the pm is asking for is for
    // the feeder to create a nop inst.  So, check for this first, and if
    // found, return the nop.
    if (inst->GetAsimSchedulerContextSwitch() == true)
    {
 
        Fabricate_CS(predicted_pc,inst,sid);
        return true;
    }

    //Create a fake branch on:
    //a) thread start up and address zero was requested
    //b) we skipped instructions and the performance and reference models are
    //not out of sync
    //c) when the reference model takes an interrupt. This should only
    //happen on OS mode.
    else if ((predicted_pc.GetAddr() == 0 && predicted_pc.GetSyllableIndex() == 0)
        || justSkipped[sid]
        || (!LAST_INST_WAS_CONTROL[sid]&&!wrongPath[sid] &&
              !LAST_INST_WAS_MLX[sid] &&predicted_pc!=NEXT_IP[sid]) )
    {
        IADDR_CLASS tmp_pc;
        inst->SetFeederContextSwitch(true);

        if(!justSkipped[sid] && (predicted_pc.GetAddr() == 0 && predicted_pc.GetSyllableIndex() == 0))
        {
            CS_COUNT[sid]++;
            tmp_pc=predicted_pc;
            TRACE(Trace_Feeder, cout << "\tAttempt to fetch IP 0 due to thread startup.  Creating BR starting trace IP." << endl);
        }
        else if(justSkipped[sid])
        {
            CORRECT_FETCH_COUNT[sid]++;
            tmp_pc=predicted_pc;
            TRACE(Trace_Feeder, cout << "\tJust skipped.  Creating BR to the Gambit IP." << endl);
        }
        else
        {  
            INTERRUPT_COUNT[sid]++;
            ASSERT(os_mode,"You appear to have an interrupt but that should not happen in user mode!!!");
            //tmp_pc.Set(predicted_pc.GetAddr(),0);
            tmp_pc=predicted_pc;
            TRACE(Trace_Feeder, cout << "\tReference model appears to have had an interrupt." <<endl);
            inst->SetNonSequentialPc(NEXT_IP[sid]); 
        }
        Fabricate_Branch(tmp_pc,inst,sid);        

        justSkipped[sid]=false;
        return true;
    }
    //Correct path instruction
    else if((NEXT_IP[sid]==predicted_pc)&&
            (!wrongPath[sid]))
    {

        inst->SetTraceID(sid);
        if (!FEEDER_BUFFER[sid].get_misp_inst_left())
        {
            CORRECT_FETCH_COUNT[sid]++;
            bool NOT_eoThread;
            TRACE(Trace_Feeder, cout << "\tStepping Reference"<<endl);
            NOT_eoThread =ConvertCorrectPathSpeed(sid,inst,NEXT_IP[sid]);

            if(TRACE_INST_LEVEL&0x001)
            {
                outfile<<"s CPU"<<sid<<" 1 "<<endl;
                outfile<<"CPUNUM: "<<sid<<" ,type: GP ,inst: "<<inst->GetRawInst()->DisasmString()<<endl;
            }
            return NOT_eoThread;
        }
        else
        {
            REFEED_FETCH_COUNT[sid]++;
            TRACE(Trace_Feeder, cout << "\tRefeeding Inst"<<endl);
            Refeed(sid,inst);
            if(TRACE_INST_LEVEL&0x004)
            {
                outfile<<"CPUNUM: "<<sid<<" ,type: RF ,inst: "<<inst->GetRawInst()->DisasmString()<<endl;
            }
            return true;
        }
    }
    
    //Long syllable special case (asim feed fetches data portion seperately)
    //We now check for this in the convertcorrect path speed
    else if( (NEXT_IP[sid].GetBundleAddr() == predicted_pc.GetBundleAddr() ) &&
        (predicted_pc.GetSyllableIndex() == 2) &&
         (LAST_INST_WAS_MLX[sid]) && (!wrongPath[sid]) &&
        (NEXT_IP[sid].GetSyllableIndex() == 1))
    {

       ASSERT(0, "This should not be called anymore!!!");
       return false;
    }

    //wrong path instruction
    else
    {
        WRONG_FETCH_COUNT[sid]++;
        //mark the instruction as wrong path
        inst->SetTraceID(traceWrongpathId);

        //set the wrongpath flag so that future fetches do not step until
        //the branch misprediction is cleaned up
        wrongPath[sid] = true;

        //select whether to fetch the predicted pc from the memory space or just
        //add nops
        if (PSEUDO_WRONGPATH)
        {
            TRACE(Trace_Feeder,
                  cout << "\tGoing on pseudo wrong path " << sid << endl);
            ConvertWrongPath(sid,inst,predicted_pc);
        }
        else
	{
            TRACE(Trace_Feeder,
                  cout << "\tWrong path with NOPish type instruction " << sid << endl);
            inst->Init(NOP,  predicted_pc);        
	}
        if(TRACE_INST_LEVEL&0x002)
        {
            outfile<<"CPUNUM: "<<sid<<" ,type: WR ,inst: "<<inst->GetRawInst()->DisasmString()<<endl;
        }
        LAST_INST_WAS_MLX[sid]=false;
        LAST_INST_WAS_CONTROL[sid]=false;
        return true;
    }
}






void
GAMBIT_FEEDER_CLASS::Commit(ASIM_INST inst)
/*
 * 'inst' is committed...
 */
{
    UINT32 sid = STREAM_HANDLE(inst->GetSWC()->GetFeederStreamHandle());

    //check to see if the instruction is on the wrong path
    if (inst->GetTraceID() == traceWrongpathId) {
        ASIMERROR(
            "Gambit_FEEDER::Commit: Attempt to commit wrongpath instruction\n"
            << "\t" << inst->GetVirtualPC() << ": "
            << inst->GetDisassembly() << endl);
    }

    //if it is a manufactuered instruction other then a context switch
    //then ignore it because we don't put non-context switch
    //instructions into the refeed buffer    
    if ((inst->GetTraceID() == traceArtificialInstId)&&
        (!inst->GetFeederContextSwitch()))
    {
        // we're committing an instruciton that was artificial, i.e., we
        // manufactured it and it doesn't exist on the trace, so just return.
        return;
    }
    
    //if we assume an inorder commit then make sure that the front of the queue
    //and this instruction match
    if(INORDER_COMMIT)
    {
        FEEDER_BUFFER[sid].removehead(inst->GetUid());        
    }
    else
    {
        //this will make sure that the head is valid
        FEEDER_BUFFER[sid].removehead();
    }

}

void 
GAMBIT_FEEDER_CLASS::Issue(ASIM_INST inst)
{

   UINT32 sid = STREAM_HANDLE(inst->GetSWC()->GetFeederStreamHandle());
   if (inst->GetTraceID() == traceWrongpathId) 
   {
       WRONG_ISSUE_COUNT[sid]++;
   }
   else if (inst->GetTraceID() == traceArtificialInstId)
   {
       FAKE_ISSUE_COUNT[sid]++;
   }
   else
   {
       CORRECT_ISSUE_COUNT[sid]++;
   }


}
void
GAMBIT_FEEDER_CLASS::Kill(
    ASIM_INST inst,
    const bool fetchNext,
    const bool killMe)
/*
 * kill 'inst'...
 */
{
    UINT32 sid = STREAM_HANDLE(inst->GetSWC()->GetFeederStreamHandle());

    TRACE(Trace_Feeder,
          cout << "\tFEED_Kill id=" << inst->GetUid()
          << "\t fetchNext=" << fetchNext
          << " killMe=" << killMe << endl);

    //On all non-wrong path instructions look to see if we should refeed
    //but only assert if not a fake instruction
    if ((inst->GetTraceID() != traceWrongpathId))
    {
        TRACE(Trace_Feeder,cout<<"The instruction was not flagged as bad path, so"<<
              " it should be in the refeed buffer"<<endl);

        //set a flag at the instruction that mispeculated so that 
        //we can refeed the instructions if we didn't detect the
        //mispeculation and stepped the reference (Gambit)
        if (!FEEDER_BUFFER[sid].set_mispec_pt(inst->GetUid(),killMe))
        {
            if(inst->GetTraceID() == traceArtificialInstId)
            {
                TRACE(Trace_Feeder,cout<<"The instruction was fake, so"<<
              " it is ok that it was not found"<<endl); 
            }
            else
            {
                ASIMERROR("Gambit_FEEDER::Kill:You are trying to kill an element but"
                          <<"cannot find it in the buffer"); 
            }
        }
        // cleanup the wrongpath flag for when we start fetching again
        wrongPath[sid] = false;

        //look to see if we will be refeeding and if so grab what the pc should be
        if (FEEDER_BUFFER[sid].get_misp_inst_left())
        {
           instruction_fi q_inst=FEEDER_BUFFER[sid].getmispec();
           NEXT_IP[sid]=q_inst->pc;
        }
    }
    else
    {

        //if the instruction was marked as traceWrongpathId it was
        //never put in the refeed buffer
    }

}

void
GAMBIT_FEEDER_CLASS::Marker(
    const ASIM_MARKER_CMD cmd,
    const IFEEDER_STREAM_HANDLE stream,
    const UINT32 markerID,
    const IADDR_CLASS markerPC,
    const UINT32 instBits,
    const UINT32 instMask)
{
    ASSERT (markerID < MAX_MARKER, "marker ID outside valid range");

    UINT32 tid = STREAM_HANDLE(stream);

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
        ASIMERROR("GAMBIT_FEEDER::Marker: marker command MARKER_SET_INST "
                  "not supported on Gambit Feeder\n");
        break;
      default:
        ASIMERROR("GAMBIT_FEEDER::Marker: invalid marker command <"
            << cmd << ">" << endl);
    }
}


/*
 * Virtual to physical translation.
 */

bool
GAMBIT_FEEDER_CLASS::ITranslate(
    UINT32 hwcNum,
    UINT64 va,
    UINT64& pa)
{
    return (addrTrans.ITranslate(hwcNum, va, pa));
}

bool
GAMBIT_FEEDER_CLASS::DTranslate(
    ASIM_INST inst,
    UINT64 va,
    UINT64& pa)
{
    return (addrTrans.DTranslate(inst, va, pa));
}


/********************************************************************
 *
 * Controller calls
 *
 *******************************************************************/
void
GAMBIT_FEEDER_CLASS::ThreadVariablesInit(UINT32 t)
{
    UINT64 startpc=0;
    if(!asim_init_handles(t))
    {
       ASIMERROR("Gambit_FEEDER: could not init the handles"<<endl); 
    }

    if(!asim_get_pc(t,&startpc))
    {
        ASIMERROR("Gambit_FEEDER: could not get the ip"<<endl);
    }
    
    TRACE(Trace_Feeder, cout << "Starting cpu: "<<t<<" at ip(hex) : "<<fmt("016x",startpc)<<endl);
    
    //The next correct ip should be where we loaded the sim from
    NEXT_IP[t].Set(startpc,startpc);
    
    //clear thread specific variables
    LAST_PHY_ADDRESS[t]=0;
    LAST_VIR_ADDRESS[t]=0;     
    LAST_INST_WAS_CONTROL[t]=false;
    LAST_INST_WAS_MLX[t]=false;
    justSkipped[t] = false;
    wrongPath[t] = false;
    buffer_position[t]=EXECUTE_GROUP_SIZE+1;

    /* stats */
    TOTAL_FETCH_COUNT[t]=0;
    WRONG_FETCH_COUNT[t]=0;
    CORRECT_FETCH_COUNT[t]=0;
    REFEED_FETCH_COUNT[t]=0;
    INTERRUPT_COUNT[t]=0;
    CS_COUNT[t]=0;
    WRONG_ISSUE_COUNT[t]=0;
    CORRECT_ISSUE_COUNT[t]=0;
    FAKE_ISSUE_COUNT[t]=0;
    ZERO_ADDRESS_MEM_OPS[t]=0;
    POSSIBLE_ERROR_IN_CFM[t]=0;
    POSSIBLE_ERROR_IN_PR[t]=0;
    // Make a new thread
    const IFEEDER_STREAM_HANDLE handle = STREAM_HANDLE(t);
    thread[t] = new IFEEDER_THREAD_CLASS(this, handle, NEXT_IP[t]);
}

void
GAMBIT_FEEDER_CLASS::CommonVariablesInit()
{


   //we handle things a little differently in OS mode right now
   os_mode=0;

   //a total count of every step on all cpus
   all_streams_icount=0;
   
   number_of_cpus=1;
   //the file to do the restore from
   restore_file=NULL;
   
   //the file to include with hdb commands
   include_file=NULL;
   
   //following variables are used for generating check points
   checkpoints_on=0;
   first_save_num=0;
   inc_save_num=0;
   save_file=NULL;

}

void
GAMBIT_FEEDER_CLASS::Stream_Trace_Init()
{
    //TRACE_INST_LEVEL
    cout<<"Opening inst_trace.out for fetch stream trace..recording the following"<<endl;
    outfile.open("inst_trace.out");
    if (!outfile)
    {
        ASIMERROR("unable to open file inst_trace.out"<<endl);
    }

    if(TRACE_INST_LEVEL&0x01)
    {
        cout<<"good instructions"<<endl;
    }

    if(TRACE_INST_LEVEL&0x02)
    {
        cout<<"bad path instructions"<<endl;
    }

    if(TRACE_INST_LEVEL&0x04)
    {
        cout<<"refeed instructions"<<endl;
    }

    if(TRACE_INST_LEVEL&0x08)
    {
        cout<<"context switch instructions"<<endl;
    }

    if(TRACE_INST_LEVEL&0x010)
    {
        cout<<"fabriacted branch instructions"<<endl;
    }


}

bool
GAMBIT_FEEDER_CLASS::Init(UINT32 argc, char **argv, char **envp)
/*
 * Initialize NULL instruction feeder. Return false
 * if we have an error during initialization.
 */
{
   bool retval;
   char prog_string[10]="asim-ipf ";
   prog_string[8]=0;
   char **HDBArgv;
   UINT32 HDBargc=argc+1;


   cout<<"GAMBIT feeder initializing ..."<<endl;

   CommonVariablesInit();
   TRACE(Trace_Feeder, cout << "Did an init on the common variables" << endl);

   //if we are output an instruction trace
   if(TRACE_INST_LEVEL)
   {
       Stream_Trace_Init();
   }

   //turn off the messaging from the cpuapi
   asim_set_verbose(false);

   //if we are recording the registers then we can only step one
   //instruction at a time
   if(RECORD_REGISTERS)
   {
       ASSERT(EXECUTE_GROUP_SIZE==1,"You can only execute groups of one instruction when recording register values");      
   }

   /* Make the argvs compatible with gambit */
   HDBArgv = new char *[argc+2];
   for(UINT32 i=0; i<argc; i++)
   {
       HDBArgv[i+1]=argv[i];
   }
   //the initialization expects to have a program
   //string when it initializes so give it one
   HDBArgv[0]=&prog_string[0];
   HDBArgv[argc+1]=0;


   //parse the arguments and pull out the asim options
   if (! Parse(HDBargc,HDBArgv )) {
       return(false);
   }

   //call the asim interface init function for gambit//
   retval= asim_init(HDBargc,HDBArgv,envp);

   TRACE(Trace_Feeder, cout << "HDB initialized " << endl);

   //set the simulation mode to hot
   //I think this turns on all the instruction information
   switchtoperformance();

   TRACE(Trace_Feeder, cout << "Switched to performance mode " << endl);

   //do the restore command
   if(restore_file!=NULL)
   {
       restorefromfile(restore_file);
       TRACE(Trace_Feeder, cout << "Restored reference from file "<<restore_file<< endl);
   
       switchtoperformance();
      //reset the simulation mode to hot if it was something else in the restore
   }


   //check to make sure that we can handle the number of cpus requested
   if(number_of_cpus>FEED_MAX_CPU)
   {
       cerr<<"feeder only compiled to support "<<FEED_MAX_CPU<<" cpus, recompile with larger number"<<endl;
       ASSERT(0,"");
   }
   
   //initialize all the threads
   for(UINT32 t = 0; t < number_of_cpus; t++) 
   {
       TRACE(Trace_Feeder, cout << "About to Initialized CPU # "<<t<< endl);
       ThreadVariablesInit(t);
       TRACE(Trace_Feeder, cout << "Initialized CPU # "<<t<< endl);
   }

   //make sure we are in performance mode
   switchtoperformance();

   cout<<"\n....GAMBIT feeder initialization done"<<endl;
   
   return(true);
}

void
GAMBIT_FEEDER_CLASS::Done(void)
{}

void
GAMBIT_FEEDER_CLASS::DumpStats(STATE_OUT state_out)
{
    char cpunum[10];
    for(UINT32 i=0; i<number_of_cpus; i++)
    {
        sprintf(cpunum,"%d",i);
        state_out->AddCompound("STREAM: ", cpunum);
        state_out->AddScalar ("UINT64", "total_fetch","The total number of feed fetches",TOTAL_FETCH_COUNT[i]);
        state_out->AddScalar ("UINT64", "wrong_fetch","The number of wrong path feed fetches",WRONG_FETCH_COUNT[i] );
        state_out->AddScalar ("UINT64", "correct_fetch","The number of correct path feed fetches",CORRECT_FETCH_COUNT[i] );
        state_out->AddScalar ("UINT64", "refeed_fetch","The number of refeed path feed fetches",REFEED_FETCH_COUNT[i] );
        state_out->AddScalar ("UINT64", "interrupt_fetch","The number of interrupt path feed fetches",INTERRUPT_COUNT[i] );
        state_out->AddScalar ("UINT64", "context_switch_fetch","The number of context_switch_fetch  feed fetches",CS_COUNT[i] );


        state_out->AddScalar ("UINT64", "wrong_issue","The number of wrong  path feed issues",WRONG_ISSUE_COUNT[i] );
        state_out->AddScalar ("UINT64", "correct_issue","The number of correct path feed issues",CORRECT_ISSUE_COUNT[i] );
        state_out->AddScalar ("UINT64", "fake_issue","The number of fake feed fetches",FAKE_ISSUE_COUNT[i] );

        state_out->AddScalar ("UINT64", "zero_address_mem_op","The number of memory ops fetch with zero address",ZERO_ADDRESS_MEM_OPS[i] );
        state_out->AddScalar ("UINT64", "possible_bad_cfm","The number of time the cfm didn't match",POSSIBLE_ERROR_IN_CFM[i] );
        state_out->AddScalar ("UINT64", "possible_bad_pr","The number of times the pr didn't match",POSSIBLE_ERROR_IN_PR[i] );

        state_out->CloseCompound();
        state_out->CloseCompound(); // cpu
    }


}
bool
GAMBIT_FEEDER_CLASS::GetInputRegVal(
    ASIM_INST inst,
    UINT32 slot,
    ARCH_REGISTER reg)
{

    bool ret_val;
    ASSERTX(RECORD_REGISTERS);

    if (slot >= NUM_SRC_GP_REGS) return false;

    UINT32 streamId = STREAM_HANDLE(inst->GetSWC()->GetFeederStreamHandle());
    ret_val = FEEDER_BUFFER[streamId].getsrc(inst->GetUid(),slot,reg);

    if(!ret_val)
    {
        cout<<"Unable to get the register values for: "<<inst->GetRawInst()->DisasmString()<<endl;
        cout<<"Uid: "<<inst->GetUid()<<endl;
        ASSERT(0,"Failure to get input register values");
    }
    TRACE(Trace_Feeder,cout<<"\tcheck in feeder valid: "<<reg->IsValid()<<" type: "<<reg->GetType()<<" number"<<reg->GetNum()<<endl);

    return true;
};

bool
GAMBIT_FEEDER_CLASS::GetPredicateRegVal(
    ASIM_INST inst,
    UINT32 slot,
    ARCH_REGISTER reg)
{
    bool ret_val;

    ASSERTX(RECORD_REGISTERS);

    if (slot >= NUM_SRC_PRED_REGS) return false;

    UINT32 streamId = STREAM_HANDLE(inst->GetSWC()->GetFeederStreamHandle());
    ret_val = FEEDER_BUFFER[streamId].getpred(inst->GetUid(),slot,reg);
    if(!ret_val)
    {
        cout<<"Unable to get the register values for: "<<inst->GetRawInst()->DisasmString()<<endl;
        cout<<"Uid: "<<inst->GetUid()<<endl;
        ASSERT(0,"Failure to get pred register values");
    }
    TRACE(Trace_Feeder,cout<<"\tcheck in feeder valid: "<<reg->IsValid()<<" type: "<<reg->GetType()<<" number"<<reg->GetNum()<<endl);

    return true;
};

bool
GAMBIT_FEEDER_CLASS::GetOutputRegVal(
    ASIM_INST inst,
    UINT32 slot,
    ARCH_REGISTER reg)
{
    bool ret_val;

    ASSERTX(RECORD_REGISTERS);


    if (slot >= NUM_DST_REGS) return false;

    UINT32 streamId = STREAM_HANDLE(inst->GetSWC()->GetFeederStreamHandle());
    ret_val = FEEDER_BUFFER[streamId].getdest(inst->GetUid(),slot,reg);
    if(!ret_val)
    {
        cout<<"Unable to get the register values for: "<<inst->GetRawInst()->DisasmString()<<endl;
        cout<<"Uid: "<<inst->GetUid()<<endl;
        ASSERT(0,"Failure to get dest register values");
    }
    TRACE(Trace_Feeder,cout<<"\tcheck in feeder valid: "<<reg->IsValid()<<" type: "<<reg->GetType()<<" number"<<reg->GetNum()<<endl);

    return true;
};


UINT64 
GAMBIT_FEEDER_CLASS::Symbol(IFEEDER_STREAM_HANDLE stream, char* name)
{

    return  u64touint64(msl_label_addr(name));
}

UINT64 
GAMBIT_FEEDER_CLASS::Symbol(char* name)
{

    return  u64touint64(msl_label_addr(name));
}

char* 
GAMBIT_FEEDER_CLASS::Symbol_Name(
    IFEEDER_STREAM_HANDLE stream,
    UINT64 address,
    UINT64& offset)
{

    U64 tmp_offset;
    char* name;
    U64 addr;
    addr.low_w=address & 0x0FFFFFFFF;
    addr.high_w=address>>32;
    name=msl_label_name(addr, 0, &tmp_offset);
    offset=u64touint64(tmp_offset);
    return  name;

}

void
IFEEDER_Usage(FILE *file)
    /*
     * Print usage...
     */
{
    fprintf(file, "\nCommon usage: (use --feeder -help to get all options)");
    fprintf(file, "\nUser mode (syscalls are emulated");
    fprintf(file, "\nFeeder usage: -rc_file=<filename> [-stdout=<filename>] [-stderr=<filename>] [-stdin=<filename>] \"executable\" \"executable arguments\" \n");
    fprintf(file, "\nOs Mode (operating system load on asim)");
    fprintf(file, "\nFeeder usage: -os -rc_file=<filename> -num_cpus=<n> -d -vga=<max|min|hidden> -max_mem=80 -restore=<filename>\n");
}

void
GAMBIT_FEEDER_CLASS::Usage_Verbose()
{
    cerr<<"ASIM OPTIONS: \n\n"<<endl;
    cerr<<"-restore=<filename>  load the simulation starting point from <filename>"<<endl;
    cerr<<"-checkpoint <filename prefix> first_time incr_after "<<endl;
    cerr<<"-hdb_script=<filename> execute the hdb script before running the simulation "<<endl;
    cerr<<"(gambitdb.rc is automatically included in OS mode\n\n"<<endl;
	    
    cerr<<"SOFTSDV OPTIONS: \n\n"<<endl;
    cerr<<" Command line switches defined in table /legacy/traces/asim/softsdv/release/v20021127_0/bin/bin.linux/ia64_cfg.csv:"<<endl;
    cerr<<"-trg_proc=<itanium|mckinley>   Set the target processor to be Itanium or McKinley"<<endl;
    cerr<<"-emrl=<s|f|h>            s - EMerald in stub mode"<<endl;
    cerr<<"			 f - EMerald in full mode"<<endl;
    cerr<<"			h - EMerald in DVLoc mode"<<endl;
    cerr<<"			local_fd                Use local numbering of file descriptors"<<endl;
    cerr<<"-ptr32                   Force ILP32 argv model"<<endl;
    cerr<<"-eas=<eas26:eas25|eas24|eas23|eas23+|eas23++>   "<<endl;
    cerr<<"-exact_fp                Use exact FP calculation using fp82 library"<<endl;
    cerr<<"-exact_fma               Generate exact FMA results"<<endl;
    cerr<<"-alat                    Simulate accurate Itanium ALAT"<<endl;
    cerr<<"-mp_start_with_bsp_only   Upon Startup of MP simulation only the BSP is alive"<<endl;
    cerr<<"-no_code_protect         Turn off code protection"<<endl;
    cerr<<"-sof_protect             Disable read of registers above sof"<<endl;
    cerr<<"-no_ac                   Turn off data access alignment checks (os mode)"<<endl;
    cerr<<"-strict_ac               abort if access alignment (app mode)"<<endl;
    cerr<<"-no_nat                  Turn off NaT generation (application mode only)"<<endl;
    cerr<<"-no_seg                  Turn off memory protection (application mode only)"<<endl;
    cerr<<"-ctrl_c                  Enable correct control-c handling"<<endl;
    cerr<<"-no_envp                 Environment variables are not allocated on the stack"<<endl;
    cerr<<"-trash_NAT_bits          Trash NAT bits of stacked integer registers"<<endl;
    cerr<<"			 or fp registers"<<endl;
    cerr<<"-trash_ive_regs          Trash ive registers value on transfer to IA mode"<<endl;
    cerr<<"-force_chk_a_failure=<n>   Force chk_a to fail every <n> instructions"<<endl;
    cerr<<"-force_chk_s_failure=<n>   Force chk_s to fail every <n> instructions"<<endl;
    cerr<<"-ba=<file_name>          Batch file to execute in batch mode"<<endl;
    cerr<<"-no_dlg                  Turn off dialog box for vpc messages"<<endl;
    cerr<<"-no_tag                  Do not load compiler generated TAG symbols"<<endl;
    cerr<<"-trace=<s|w|r|i|f|pred|c|a|e>    ; s - short virtual address branch trace ; w - memory write value trace ; r - Generate memory read value trace ; i - Instructions bundle byte trace ; f -  Code Fetch trace ; pred - predicates trace ; c - instruction count trace ; a - RSE activity (fill/spill information) ;  e - Execution driven instruction info trace"<<endl;
    cerr<<"-emrl_cfg=<file>         EMerald Configuration File Name"<<endl;
    cerr<<""<<endl;
    cerr<<" Command line switches defined in table common_cfg.csv:"<<endl;
    cerr<<"-os                      Turn os mode on"<<endl;
    cerr<<"-max_mem                 Set the maximum memory size to be used by vpc"<<endl;
    cerr<<"			 n is the percentage of the total machine memory size"<<endl;
    cerr<<"-trc_def=<file>          Specify Trace Forms Definition file name"<<endl;
    cerr<<"-trc_to=<n>              Specify number of seconds for trace generation time out"<<endl;
    cerr<<"-rc_file=<file>          vpc initialization file"<<endl;
    cerr<<"-dbrc=<file>             hdb input file"<<endl;
    cerr<<"-text_table=<file>       Main configuration table"<<endl;
    cerr<<"-stdin=<file>            Redirect application standard input to <file>"<<endl;
    cerr<<"-stdall=<file>           Redirect application standard input/output/error"<<endl;
    cerr<<"			 to <file>"<<endl;
    cerr<<"-stdout=<file>           Redirect application standard output to <file>"<<endl;
    cerr<<"-stderr=<file>           Redirect application standard error to <file>"<<endl;
    cerr<<"-delta_in=<dir>          The working path for previous delta_file"<<endl;
    cerr<<"-delta_out=<dir>         The working path for the current delta-file"<<endl;
    cerr<<"-no_signon               Do not display signon and information messages"<<endl;
    cerr<<"-no_warn                 Do not output warnings"<<endl;
    cerr<<"-noisy                   Turn on the extended warning/debug messages"<<endl;
    cerr<<"-d                       Turn debug mode on"<<endl;
    cerr<<"-g                       Turn debug mode on"<<endl;
    cerr<<"-random_time_slice       Time slice in the current MP execution"<<endl;
    cerr<<"			 is randomly selected"<<endl;
    cerr<<"-num_cpus=<n>            Sets number of processors in the current"<<endl;
    cerr<<"			 execution to be n"<<endl;
    cerr<<"-time_slice=<n>          Sets time slice in the current multiprocessor"<<endl;
    cerr<<"			 execution to be n"<<endl;
    cerr<<"-vga=<max|min|hidden>    Start the vga window"<<endl;
    cerr<<"			 max - open regular vga window (Default value)"<<endl;
    cerr<<"			 min - Open minimized vga window"<<endl;
    cerr<<"			 hidden - Open hidden vga window - for batch execution"<<endl;
    cerr<<"-v2h_size                Set V2H actual table size"<<endl;
    cerr<<"-affin=<N>               Set affinity of vpc thread to processor N (if possible)"<<endl;
    cerr<<"-sysapi                  OS emulation in application mode: Posix"<<endl;
    cerr<<"-kbd                     keyboard & mouse state: enable/disable"<<endl;
    cerr<<"-port_connect            Isis port connection configuration file"<<endl;
    cerr<<"-tran_tick               if >0 print tick every creation of such transactions"<<endl;
    cerr<<"-tran_end                if >0 print exit after creation of such number of transactions"<<endl;
    cerr<<"-tran_watch              if >0 exit after that many nanosec without creation of any transaction"<<endl;
    cerr<<"-mm_active               if TRUE Memory Manager is active"<<endl;
    exit(1);
}


IFEEDER_BASE IFEEDER_New(void)
{
    return new GAMBIT_FEEDER_CLASS();
}
