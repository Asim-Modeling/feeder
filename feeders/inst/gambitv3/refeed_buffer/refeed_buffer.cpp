/*
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
 */


// ASIM local module
#include "asim/provides/refeed_buffer_v3.h"

FEEDER_QUEUE_CLASS::FEEDER_QUEUE_CLASS()
{
    size=0;
    head=0;
    tail=0;
    mispeculation_pt=0;
    issue_pt=0;
    misp_inst_left=0;

    //mark all of the elements in the buffer as invalid
    for(int i=0; i<REFEED_QUEUE_SIZE; i++)
    {
        instruction_buffer[i].valid=0;
        instruction_buffer[i].issued=0;
        // hmmm... don't see where all the fields are init'ed, so I'll do these
        // here right now.
        instruction_buffer[i].nop=false;
        instruction_buffer[i].longImm=false;
    }

}

void 
FEEDER_QUEUE_CLASS::set_input_dep(
    ARCH_REGISTER_CLASS src_Gp[NUM_SRC_GP_REGS],
    ARCH_REGISTER_CLASS src_Pred[NUM_SRC_PRED_REGS])
{
    //copy the local feeder variable to the refeed buffer
    for (int i = 0; i < NUM_SRC_GP_REGS; i++)
    {
        instruction_buffer[tail].src_Gp[i] = src_Gp[i];
    }
    
    for (int i = 0; i < NUM_SRC_PRED_REGS; i++)
    {
        instruction_buffer[tail].src_Pred[i] = src_Pred[i];
    }
}

void 
FEEDER_QUEUE_CLASS::set_old_bsp(UINT64 oldbsp)
{
    instruction_buffer[tail].oldbsp=oldbsp;
}

void 
FEEDER_QUEUE_CLASS::set_output_dep(
    ARCH_REGISTER_CLASS dst[NUM_DST_REGS])
{
    for (int i = 0; i < NUM_DST_REGS; i++)
    {
        instruction_buffer[tail].dst[i] = dst[i];
    }
}

void 
FEEDER_QUEUE_CLASS::set_bsp(UINT64 bsp)
{
    instruction_buffer[tail].bsp=bsp;
}

void 
FEEDER_QUEUE_CLASS::set_nop(bool nop)
{
    instruction_buffer[tail].nop=nop;
}

void 
FEEDER_QUEUE_CLASS::set_longImm(bool li)
{
    instruction_buffer[tail].longImm = li;
}

UINT64 FEEDER_QUEUE_CLASS::getheaduid()
{
    return instruction_buffer[head].Uid;
}

ARCH_REGISTER 
FEEDER_QUEUE_CLASS::getpred(UINT64 Uid)
{
    UINT64 startpt=head;
    while (Uid !=instruction_buffer[startpt].Uid )
    {

        startpt++;
        if (startpt==REFEED_QUEUE_SIZE)
        {
            startpt=0;
        }
        if(startpt==tail)
        {
            cout<<"You asked for an instruction that is not in the buffer"<<endl;
            return 0;
        }

    }
    if(REFEED_VERBOSE)
    {
        print_entry(&instruction_buffer[startpt]);
    }
    return &(instruction_buffer[startpt].src_Pred[0]);
}
bool
FEEDER_QUEUE_CLASS::getpred(UINT64 Uid, UINT32 slot,ARCH_REGISTER reg)
{
    UINT64 startpt=head;
    while (Uid !=instruction_buffer[startpt].Uid )
    {

        startpt++;
        if (startpt==REFEED_QUEUE_SIZE)
        {
            startpt=0;
        }
        if(startpt==tail)
        {
            cout<<"You asked for an instruction that is not in the buffer"<<endl;
            return 0;
        }

    }
    if(REFEED_VERBOSE)
    {
        print_entry(&instruction_buffer[startpt]);
    }
    *reg= (instruction_buffer[startpt].src_Pred[slot]);
    return 1;
}

ARCH_REGISTER
FEEDER_QUEUE_CLASS::getsrc(UINT64 Uid)
{
    UINT64 startpt=head;
    while (Uid !=instruction_buffer[startpt].Uid )
    {

        startpt++;
        if (startpt==REFEED_QUEUE_SIZE)
        {
            startpt=0;
        }
        if(startpt==tail)
        {
            cout<<"You asked for an instruction that is not in the buffer"<<endl;
            return 0;
        }
    }
    if(REFEED_VERBOSE)
    {
        print_entry(&instruction_buffer[startpt]);
    }
    return &(instruction_buffer[startpt].src_Gp[0]);
}

bool
FEEDER_QUEUE_CLASS::getsrc(UINT64 Uid,UINT32 slot,ARCH_REGISTER reg)
{
    UINT64 startpt=head;
    while (Uid !=instruction_buffer[startpt].Uid )
    {

        startpt++;
        if (startpt==REFEED_QUEUE_SIZE)
        {
            startpt=0;
        }
        if(startpt==tail)
        {
            cout<<"You asked for an instruction that is not in the buffer"<<endl;
            return 0;
        }
    }
    if(REFEED_VERBOSE)
    {
        print_entry(&instruction_buffer[startpt]);
    }
    *reg= (instruction_buffer[startpt].src_Gp[slot]);
    return 1;
}

ARCH_REGISTER 
FEEDER_QUEUE_CLASS::getdest(UINT64 Uid)
{ 

    UINT64 startpt=head;
    while (Uid !=instruction_buffer[startpt].Uid )
    {

        startpt++;
        if (startpt==REFEED_QUEUE_SIZE)
        {
            startpt=0;
        }
        if(startpt==tail)
        {
            cout<<"You asked for an instruction that is not in the buffer"<<endl;
            return 0;
        }
    }
    if(REFEED_VERBOSE)
    {
        print_entry(&instruction_buffer[startpt]);
    }
    return &(instruction_buffer[startpt].dst[0]);
}
bool 
FEEDER_QUEUE_CLASS::getdest(UINT64 Uid,UINT32 slot, ARCH_REGISTER reg)
{ 

    UINT64 startpt=head;
    while (Uid !=instruction_buffer[startpt].Uid )
    {

        startpt++;
        if (startpt==REFEED_QUEUE_SIZE)
        {
            startpt=0;
        }
        if(startpt==tail)
        {
            cout<<"You asked for an instruction that is not in the buffer"<<endl;
            return 0;
        }
    }
    if(REFEED_VERBOSE)
    {
        print_entry(&instruction_buffer[startpt]);
    }
    *reg= (instruction_buffer[startpt].dst[slot]);
    return 1;
}
instruction_fi 
FEEDER_QUEUE_CLASS::getnum(UINT32 num) 
{

    return &instruction_buffer[num];
}


/* The following functions are used to do the issue checking */
UINT64 
FEEDER_QUEUE_CLASS::getissueuid()
{
    return instruction_buffer[issue_pt].Uid;
}
UINT64 
FEEDER_QUEUE_CLASS::getctissueuid()
{
    return instruction_buffer[head].Uid;
}
/*set the instruction as being issued */
void 
FEEDER_QUEUE_CLASS::setissue()
{
    instruction_buffer[issue_pt].issued=1;    
}

/* done on a FEED_KILL to reset the issue point to a earlier element 
   check to see if the mispeculation point is before the issue point*/
void 
FEEDER_QUEUE_CLASS::resetissuept()
{
    UINT32 tmp_misp_num;
    UINT32 tmp_issue_num;
    //if we are in the process of wrapping around
    if(mispeculation_pt<head)
        tmp_misp_num=mispeculation_pt+REFEED_QUEUE_SIZE;
    else
        tmp_misp_num= mispeculation_pt;

    if(issue_pt<head)
        tmp_issue_num=issue_pt+REFEED_QUEUE_SIZE;
    else
        tmp_issue_num=issue_pt;

    if (tmp_issue_num>tmp_misp_num)
        issue_pt=mispeculation_pt;

}

/* done on normal issue to set to the next element in the list */
void 
FEEDER_QUEUE_CLASS::incremetissuept()
{
    issue_pt++;
    if(issue_pt==REFEED_QUEUE_SIZE)
        issue_pt=0;
}

bool 
FEEDER_QUEUE_CLASS::hasbeenissued()
{

    return instruction_buffer[head].issued;
}



/* The following functions allow the buffer to refeed instructions
   after an unforseen exception occurs */

//changes the Uid of the instruction needed because we can
//have exceptions within exceptions. This can only be done
//to the instruction at the mispeculation point
int 
FEEDER_QUEUE_CLASS::rename(UINT64 newUid)
{
    if (size==0 ||
        instruction_buffer[mispeculation_pt].valid==0)
        return 0;
    
    else
        instruction_buffer[mispeculation_pt].Uid=newUid;
    
    return 1;
}

//return a pointer to the mispeculation_pt - null if not valid or
//misp_inst_left = FALSE
instruction_fi 
FEEDER_QUEUE_CLASS::getmispec()
{
    if (size==0 ||
        instruction_buffer[mispeculation_pt].valid==0)
    {
        return NULL; 
    }    
    else
    {
        return &instruction_buffer[mispeculation_pt];
    }
}

//returns true if we have instructions to refeed false otherwise
bool 
FEEDER_QUEUE_CLASS::get_misp_inst_left()
{
    return misp_inst_left;
}


//move the mispeculation point ahead one
void 
FEEDER_QUEUE_CLASS::increment_mispec_pt()
{
    mispeculation_pt++;
    if(mispeculation_pt==REFEED_QUEUE_SIZE)
    {
        mispeculation_pt=0;
    }
    if(mispeculation_pt==tail)
    {
        misp_inst_left=0;            
    }
}
    


IADDR_CLASS 
FEEDER_QUEUE_CLASS::get_mispec_pc()
{ 
    return instruction_buffer[mispeculation_pt].pc;
}


void 
FEEDER_QUEUE_CLASS::print_entry(instruction_fi inst)
{
    
    cpuapi_inst_info_t *inst_info=&(inst->inst_info);
    UINT64 bundle_bits[2];
    TRACE(Trace_Feeder,cout<<"instruction id: "<<inst_info->inst_id<<endl);
    TRACE(Trace_Feeder,cout<<"instruction count: "<<inst_info->icount<<endl);
    TRACE(Trace_Feeder,cout<<"New inst: "<<inst_info->new_inst<<endl);

    //copy the bundle bits so that it is easier to print
    memcpy(&bundle_bits, &inst_info->inst_bytes[0],16);
    TRACE(Trace_Feeder,cout<<"The bundle bits are: ");
    TRACE(Trace_Feeder,cout<<fmt("016x", bundle_bits[1]));
    TRACE(Trace_Feeder,cout<<fmt("016x", bundle_bits[0])<<endl);    
    TRACE(Trace_Feeder,cout<<"The paddr is: "<<fmt("016x",inst_info->phy_addr)<<endl);
    TRACE(Trace_Feeder,cout<<"The vaddr is: "<<fmt("016x",inst_info->virt_addr)<<endl);
    TRACE(Trace_Feeder,cout<<"The flags are: "<<inst_info->eflags<<endl);
    TRACE(Trace_Feeder,cout<<"The repeat count is: "<<inst_info->cur_repeat_count<<endl);
    TRACE(Trace_Feeder,cout<<"Branch taken: "<<inst_info->branch_taken<<endl);
    TRACE(Trace_Feeder,cout<<"Branch vaddr: "<<fmt("016x",inst_info->branch_target_virt_addr)<<endl);
    TRACE(Trace_Feeder,cout<<"Store inst: "<<inst_info->store_inst<<endl);
    TRACE(Trace_Feeder,cout<<"Load inst: "<<inst_info->load_inst<<endl);
    TRACE(Trace_Feeder,cout<<"Branch inst: "<<inst_info->branch_inst<<endl);
    TRACE(Trace_Feeder,cout<<"mem paddr: "<<fmt("016x",inst_info->phy_mem_addr)<<endl);
    TRACE(Trace_Feeder,cout<<"mem vaddr: "<<fmt("016x",inst_info->virt_mem_addr)<<endl);
    TRACE(Trace_Feeder,cout<<"cfm: "<<fmt("016x",inst_info->cfm)<<endl);
    TRACE(Trace_Feeder,cout<<"bsp: "<<fmt("016x",inst_info->bsp)<<endl);
    TRACE(Trace_Feeder,cout<<"pred: "<<fmt("016x",inst_info->predicates)<<endl);

    TRACE(Trace_Feeder,cout<<"The bundle bits are: ");
    TRACE(Trace_Feeder,cout<<fmt("016x", inst->bundle_bits[1]));
    TRACE(Trace_Feeder,cout<<fmt("016x", inst->bundle_bits[0])<<endl);    
    TRACE(Trace_Feeder,cout<<inst->pc<<endl);
    TRACE(Trace_Feeder,cout<<inst->npc<<endl);
    TRACE(Trace_Feeder,cout<<"oldcfm: "<<fmt("016x", inst->oldcfm)<<endl);
    TRACE(Trace_Feeder,cout<<"cfm: "<<fmt("016x", inst->cfm)<<endl);
    TRACE(Trace_Feeder,cout<<"oldpr: "<<fmt("016x", inst->oldpr)<<endl);
    TRACE(Trace_Feeder,cout<<"pr :"<<fmt("016x", inst->pr)<<endl);
    TRACE(Trace_Feeder,cout<<"oldbsp :"<<fmt("016x", inst->oldbsp)<<endl);
    TRACE(Trace_Feeder,cout<<"bsp :"<<fmt("016x", inst->bsp)<<endl);
    TRACE(Trace_Feeder,cout<<"Uid :"<<inst->Uid<<endl);
    TRACE(Trace_Feeder,cout<<"Valid :"<<inst->valid<<endl);
    TRACE(Trace_Feeder,cout<<"issued :"<<inst->issued<<endl);
    TRACE(Trace_Feeder,cout<<"nop :"<<inst->nop<<endl);
    TRACE(Trace_Feeder,cout<<"longImm :"<<inst->longImm<<endl);


    for(UINT32 i=0; i<NUM_SRC_GP_REGS; i++)
    {
        TRACE(Trace_Feeder,cout<<"Src Register input: "<<i<<endl);
        TRACE(Trace_Feeder,cout<<"\tvalid: "<<inst->src_Gp[i].IsValid()<<" type: "<<inst->src_Gp[i].GetType()<<" number"<<inst->src_Gp[i].GetNum()<<endl);
        
            
    }
    for(UINT32 i=0; i<NUM_SRC_PRED_REGS; i++)
    {
        TRACE(Trace_Feeder,cout<<"Src Pred Register input: "<<i<<endl);
        TRACE(Trace_Feeder,cout<<"\tvalid: "<<inst->src_Pred[i].IsValid()<<" type: "<<inst->src_Pred[i].GetType()<<" number"<<inst->src_Pred[i].GetNum()<<endl);

    }
    for(UINT32 i=0; i<NUM_DST_REGS; i++)
    {
        TRACE(Trace_Feeder,cout<<"Dst Register input: "<<i<<endl);
        TRACE(Trace_Feeder,cout<<"\tvalid: "<<inst->dst[i].IsValid()<<" type: "<<inst->dst[i].GetType()<<" number"<<inst->dst[i].GetNum()<<endl);

    }



}
void
FEEDER_QUEUE_CLASS::add(cpuapi_inst_info_t *inst_info,const UINT64 *bundle_bits,
        IADDR_CLASS pc,IADDR_CLASS npc,UINT64 oldcfm,
        UINT64 cfm,UINT64 oldpr,UINT64 pr,UINT64 Uid)
{
    instruction_fi insert_pt= &instruction_buffer[tail];

    memcpy(&(insert_pt->inst_info),inst_info,sizeof(cpuapi_inst_info_t));
    insert_pt->bundle_bits[0]=bundle_bits[0];
    insert_pt->bundle_bits[1]=bundle_bits[1];
 //   insert_pt->pc.Set(pc,pc);
 //   insert_pt->npc.Set(npc,npc);
    insert_pt->pc=pc;
    insert_pt->npc=npc;
    insert_pt->oldcfm=oldcfm;
    insert_pt->cfm=cfm;
    insert_pt->oldpr=oldpr;
    insert_pt->pr=pr;
    insert_pt->Uid=Uid;
    insert_pt->valid=1;

    TRACE(Trace_Feeder, cout<<"\tREFEED BUFFER:"<<" Adding Uid: "<<insert_pt->Uid<<
          " pc: "<<insert_pt->pc<<" npc: "<<insert_pt->npc<<endl);

    //IF the verbose is on print out all the details
    if(REFEED_VERBOSE)
    {
        print_entry(insert_pt);
    }
    size++;

    //we do the space check after the insert fixes the first case
    //problem. but means that there always has to be one empty in the
    //queue
    tail++;

    //if wrapped around (you can also do the modulo trick but I think
    //that is slower)
    if(tail==REFEED_QUEUE_SIZE)
    {
        tail=0;
    }

    if (tail==head)
    {
        ASIMERROR("Gambit_FEEDER buffer queue is full"<<endl);
    }

}

void
FEEDER_QUEUE_CLASS::removehead()
{
    if (size==0 || instruction_buffer[head].valid==0)
    {
        cout<<"Size is: "<<size<<endl;
        ASIMERROR("Gambit_FEEDER: Buffer is empty or head is invalid"<<endl);
    }
    else
    {
        size--;
        TRACE(Trace_Feeder,cout<<"\t\tRemoving Uid: "<<instruction_buffer[head].Uid<<endl);
        instruction_buffer[head].valid=0;
        instruction_buffer[head].issued=0;
        //yes I know the modulo trick but I think it is slower.
        head++;
        if (head==REFEED_QUEUE_SIZE)
        {
            head=0;
        }

    }
}
//Remove the 
void
FEEDER_QUEUE_CLASS::removehead(UINT64 Uid)
{


    //pull of any leading nops and longImms if they don't match the
    //Uid we were given
    while ((Uid != instruction_buffer[head].Uid)&&
           (instruction_buffer[head].nop ||
            instruction_buffer[head].longImm) &&
           (size))
    {
        
        size--;
        instruction_buffer[head].valid=0;
        instruction_buffer[head].issued=0;
        TRACE(Trace_Feeder,cout<<"Removing uid"<<
              instruction_buffer[head].Uid<<endl);
        //yes I know the modulo trick but I think it is slower.
        head++;
        if (head==REFEED_QUEUE_SIZE)
        {
            head=0;
        }
    }
    if (size==0 || instruction_buffer[head].valid==0)
    {
        cout<<"Size is: "<<size<<endl;
        cout <<"Uid given is: "<<Uid<<endl;
        ASIMERROR("Gambit_FEEDER: Buffer is empty or head is invalid"<<endl);
    }

    else if(Uid != instruction_buffer[head].Uid)
    {
        cout <<"Uid given is: "<<Uid<<" Uid of queue is: "<<
            instruction_buffer[head].Uid<<endl;
        ASIMERROR("Gambit_FEEDER: Uid of commit does not match buffer head"<<endl);
    }

    else
    {
        size--;
        
        instruction_buffer[head].valid=0;
        instruction_buffer[head].issued=0;
        TRACE(Trace_Feeder,cout<<"Removing uid"<<
              instruction_buffer[head].Uid<<endl);

        //yes I know the modulo trick but I think it is slower.
        head++;
        if (head==REFEED_QUEUE_SIZE)
        {
            head=0;
        }
    }
    
}


int 
FEEDER_QUEUE_CLASS::set_mispec_pt(UINT64 Uid,bool killMe)
{
    UINT32 i;
    UINT32 tailminusone;


//	cout<<"Set mispec pt"<<endl;
    //if we already knew the front end was
    //going to mispeculate then we should have
    //stop stepping gambit therefore the end
    //should be closer then the head to this
    //Uid
    if(size==0)
    {
        return 0;
    }

    if (tail==0)
    {
        tailminusone=REFEED_QUEUE_SIZE-1;
    }
    else
    {
        tailminusone=tail-1;
    }
//	cout<<"Tailminusone "<<tailminusone<<endl;
    i=tailminusone;
    while (i!=head&&
           instruction_buffer[i].Uid!=Uid)
    {     
//	cout<<"Iteration"<<endl ;
        if (i==0)
        {
            i=REFEED_QUEUE_SIZE-1;
        }
        else
        {
            i--;  
        }
    }
    //if we found it set the mispec pt
    if(instruction_buffer[i].Uid==Uid)
    {
//        cout<<"Here in mispeculation point"<<endl;
        if(killMe){
//	    cout<<"if"<<endl;
            mispeculation_pt=i;
            misp_inst_left=1;
        }
        else
        {

            //if we knew the mispeculation was coming then the
            //isntruction should be the last in the queue if not set
            //the mispeculation pt and misp_inst_left flag
            if(i==tailminusone)
            {
                mispeculation_pt=i;
                    misp_inst_left=0;
	//	cout<<"No instructions left"<<endl;
            }
            else
            {
	//	cout<<"else else mispeculation pt"<<mispeculation_pt<<" tail "<<
	//		tail<<endl;
                misp_inst_left=1;
                if(i==REFEED_QUEUE_SIZE-1)
                {
                    mispeculation_pt=0;
                }
                else
                {
                    mispeculation_pt=i+1;
                }
            }
        }
        return 1;
    }
    else
    {
//	cout<<"RETURNING ZERO"<<endl;
        return 0;
    }


}

