//----------------------------------------------------------------------------
// Verifier.C - verification thread for committed instructions
//
// This file is part of the PolyPath architecture backend
// to the AINT microprocessor simulation tools developed by the
//
//                     Experimental Systems Lab
//                        Architecture Group
//                         (Dirk Grunwald)
//                 University of Colorado at Boulder
//
// Copyright (c) 1999   Artur Klauser <Artur.Klauser@computer.org>
//
// The verifier runs the same program as the pipeline simulator, but in
// a separate thread (and address space). The verifier thread is only
// simulated on a fast functional level (no pipeline, no timing). Each
// instruction committed in the pipeline level simulator is also executed
// by the verifier thread and the two instruction streams are compared to
// guarantee that the pipeline level simulator produces the correct results;
//
// $Id: Verifier.cpp 323 2002-07-15 19:48:18Z klauser $
//----------------------------------------------------------------------------

// generic
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

// ASIM local module
extern "C" {
#include "icode.h"
#include "pmint.h"
void informative(char *s, ...);
unsigned long PMINT_get_instr_issue_count();
}

#include "Verifier.h"

extern thread_ptr Threads;


//
// create the verifier thread
//
Verifier::Verifier(int tid, aint_addr_t startPC)
  : PC(startPC),
    TID(tid)
{
  // r2r: don't understand - no *_create_thread functionality in backend?
  //Path = AINT_create_path(TID, PC);   // create AINT main path
  informative("Verifier:: activated\n");
}

//
// verify next committed instruction
//
void
Verifier::Step( ASIM_INST instBackend, inflight_inst_ptr ifiBackend,
                VerifierMode mode )
{
  inflight_inst_ptr ifiVerifier;
  instid_t instidVerifier;
  thread_ptr threadBackend = &Threads[0];
  thread_ptr threadVerifier = &Threads[1];
  unsigned int iflagsVerifier;
  bool failed = false;

  //informative("Verifier::Step at PC " << (void *) PC << endl);
  //
  // check if we try to commit the correct instruction
  //
  failed |= verify( failed,
          (void*) PC, (void*) ifiBackend->picode->addr,
          PC, "commiting wrong instruction" );

  //
  // step (fetch + execute + commit) one instruction
  //
  instidVerifier = AINT_fetch_next_instr(TID, PC);

  //
  // issue (in all its various incarnations)
  //
  ifiVerifier = &threadVerifier->cbif[threadVerifier->instid];
  iflagsVerifier = ifiVerifier->picode->iflags;
  
  
  if (( iflagsVerifier & E_MEM_REF )&& !(iflagsVerifier & E_PREFETCH) ) {
    AINT_issue_other(TID, instidVerifier);
    if ( iflagsVerifier & (E_READ|E_LD_L)) {
      AINT_do_read(TID, instidVerifier);
    } else if ( iflagsVerifier & (E_WRITE|E_ST_C)) {
      AINT_do_spec_write(TID, instidVerifier);
      AINT_do_write(TID, instidVerifier);
    }
  } else {
    AINT_issue_other(TID, instidVerifier);
  }

  if (mode == VerifierCheck) {
    //
    // verify computed values and addresses
    //

    // get result register
    // verifier
    icode_ptr picodeVerifier = ifiVerifier->picode;
    Physical_RegNum newDestPhysVerifier;
    if (picodeVerifier->dest < MaxArgs) {
      // This instruction writes a register
      newDestPhysVerifier = ifiVerifier->args[picodeVerifier->dest];
    } else {
      newDestPhysVerifier = ZERO_REGISTER;
    }
    // backend
    icode_ptr picodeBackend = ifiBackend->picode;
    Physical_RegNum newDestPhysBackend;
    if (picodeBackend->dest < MaxArgs) {
      // This instruction writes a register
      newDestPhysBackend = ifiBackend->args[picodeBackend->dest];
    } else {
      newDestPhysBackend = ZERO_REGISTER;
    }

#ifdef NEW_TTL
    //
    // It is worth here to use an 'ifdef' to avoid paying any extra 'if' cost when 
    // running in pure Alpha mode. Remember that 'is_vector' is always defined in the
    // picode, so we could rely on it to be set to '0' when running in Alpha-only mode.
    // however, we would still be paying the 'if' every single time through this routine.
    //
    if ( picodeVerifier->is_vector ) {
     failed |= VectorStep(threadVerifier, ifiVerifier, threadBackend, ifiBackend,instBackend);
    }
#endif

    // check RA, RB, RC
    failed |= verify( failed,
            (void*) threadVerifier->Reg[ifiVerifier->args[0]].Int64,
            (void*) threadBackend->Reg[ifiBackend->args[0]].Int64,
            PC, "RA mismatch");
    failed |= verify( failed,
            (void*) threadVerifier->Reg[ifiVerifier->args[1]].Int64,
            (void*) threadBackend->Reg[ifiBackend->args[1]].Int64,
            PC, "RB mismatch");
    failed |= verify( failed,
            (void*) threadVerifier->Reg[ifiVerifier->args[2]].Int64,
            (void*) threadBackend->Reg[ifiBackend->args[2]].Int64,
            PC, "RC mismatch");
    if (iflagsVerifier & E_WRITE) {
      failed |= verify( failed,
              (void*) ifiVerifier->vaddr,
              (void*) ifiBackend->vaddr,
              PC, "wrong store address" );
//   old data of memory location does not exist @vssad
//      failed |= verify( failed,
//              (void*) ifiVerifier->olddata,
//              (void*) ifiBackend->olddata,
//              PC, "store olddata mismatch" );
      failed |= verify( failed,
              (void*) ifiVerifier->data,
              (void*) ifiBackend->data,
              PC, "store data mismatch" );
    } else if (iflagsVerifier & E_READ) {
      failed |= verify( failed,
              (void*) ifiVerifier->vaddr,
              (void*) ifiBackend->vaddr,
              PC, "wrong load address" );
      failed |= verify( failed,
              (void*) threadVerifier->Reg[newDestPhysVerifier].Int64,
              (void*) threadBackend->Reg[newDestPhysBackend].Int64,
              PC, "load data mismatch");
    } else {
      failed |= verify( failed,
              (void*) threadVerifier->Reg[newDestPhysVerifier].Int64,
              (void*) threadBackend->Reg[newDestPhysBackend].Int64,
              PC, "writeback data mismatch");
    }

    //
    // check if we got the same successor PC
    //
    failed |= verify( failed,
            (void*) ifiVerifier->nextpc,
            (void*) ifiBackend->nextpc,
            PC, "execution stream diverting" );

  } // mode == VerifierCheck

  if (failed) {
    if (instBackend) {
      cerr << "\t" << (void*) PC << ": "
           << instBackend->GetDisassembly() << endl;
    }
    cerr << endl;

    cerr << "\tVerifier:" << endl;
    dumpInsnInfo( ifiVerifier, threadVerifier );
    cerr << endl;

    cerr << "\tBackend:" << endl;
    dumpInsnInfo( ifiBackend, threadBackend );
    cerr << endl;

    cerr << "Verifier::Step terminating execution" << endl;
    cerr << endl;
    exit(123);
  }

  PC = ifiVerifier->nextpc;
  AINT_commit_instr(TID, instidVerifier);
}

//
// verify next committed instruction, specific for Vector instructions
//
bool
Verifier::VectorStep(thread_ptr threadVerifier, inflight_inst_ptr ifiVerifier, 
                     thread_ptr threadBackend,  inflight_inst_ptr ifiBackend, ASIM_INST instBackend) 
{
#ifdef NEW_TTL
 UINT32		i,reg;
 UINT64		vl;
 bool		failed = false;
 unsigned int	iflagsVerifier = ifiVerifier->picode->iflags;
 icode_ptr	picodeVerifier = ifiVerifier->picode;
 icode_ptr	picodeBackend  = ifiBackend->picode;
 char		*mesg[3];

 // check if both instructions are vector 
 failed |= verify( failed,
         (void*) picodeVerifier->is_vector,
         (void*) picodeBackend->is_vector,
         PC, "IS_VECTOR mismatch");

 // check Vector control registers: VL, VS and VM
 failed |= verify( failed,
         (void*) threadVerifier->Reg[ifiVerifier->vl].Int64,
         (void*) threadBackend->Reg[ifiBackend->vl].Int64,
         PC, "VL mismatch");

 failed |= verify( failed,
         (void*) threadVerifier->Reg[ifiVerifier->vs].Int64,
         (void*) threadBackend->Reg[ifiBackend->vs].Int64,
         PC, "VS mismatch");

 // only if the instruction uses a mask!
 //if (picodeBackend->is_masked)
 //{ 
	 failed |= verify( failed,
        	 (void*) threadVerifier->Reg[ifiVerifier->vm].Mask[0],
        	 (void*) threadBackend->Reg[ifiBackend->vm].Mask[0],
        	 PC, "VM (half 0) mismatch");

	 failed |= verify( failed,
        	 (void*) threadVerifier->Reg[ifiVerifier->vm].Mask[1],
        	 (void*) threadBackend->Reg[ifiBackend->vm].Mask[1],
        	 PC, "VM (half 1) mismatch");
 //}
 
 // just to reuse the GETVM macro and keep all this stuff centralized...
 thread_ptr pthread = 	threadVerifier;
 inflight_inst_ptr ifi = ifiVerifier;
 
 //
 // check RA, RB, RC
 // To avoid a painful 'if' inside the checking loop, we always test at loop
 // entry time whether a certain register was 'enabled' or not (i.e., whether
 // it was the ZERO_REGISTER or not
 //
 mesg[0] = "Vector RA Mismatch";
 mesg[1] = "Vector RB Mismatch";
 mesg[2] = "Vector RC Mismatch";
 for ( reg = 0; reg <= 2; reg ++ ) {
  // Do not loop starting at the Z register.
  if ( ifiVerifier->args[reg] == ZERO_REGISTER ) continue;
  
  // if RC = VMASK (setvm/setnvm) we check the data in another way:
  if ((reg==2)&&(ifiVerifier->picode->is_setvm))
  {
	
	failed |= verify( failed,
           (void*) threadVerifier->Reg[ifiVerifier->args[reg]].Mask[0],
		   (void*) threadBackend->Reg[ifiBackend->args[reg]].Mask[0],
           PC, "Half 0 of dest. Vector mask mismatch", 0);

	failed |= verify( failed,
           (void*) threadVerifier->Reg[ifiVerifier->args[reg]].Mask[1],
		   (void*) threadBackend->Reg[ifiBackend->args[reg]].Mask[1],
           PC, "Half 1 of dest. Vector mask mismatch", 0);
		   
	continue;
  }
  
  //
  // Some vector instructions have a scalar register in position RB... For those,
  // you definetly do not want to do a loop up to VL!!!
  //
  vl = ifiVerifier->args[reg] < FIRST_VEC_LOGICAL ? 1 : threadVerifier->Reg[ifiVerifier->vl].Int64;
  for ( i = 0; i < vl; i++ ) {
  
   // if the instructions is masked and the mask is clear in this position => step over
   if ( ( picodeVerifier->is_masked ) && (!GETVM(i) ) ) continue;
   
   failed |= verify( failed,
           (void*) threadVerifier->Reg[ifiVerifier->args[reg]+i].Int64,
           (void*) threadBackend->Reg[ifiBackend->args[reg]+i].Int64,
           PC, mesg[reg], i);
  }
 }

 if (iflagsVerifier & E_WRITE) {
   failed |= verify( failed,
           (void*) ifiVerifier->vaddr,
           (void*) ifiBackend->vaddr,
           PC, "wrong store address" );
   failed |= verify( failed,
           (void*) ifiVerifier->data,
           (void*) ifiBackend->data,
           PC, "store data mismatch @ last element" );
 }
  else if (iflagsVerifier & E_READ) {
   failed |= verify( failed,
           (void*) ifiVerifier->vaddr,
           (void*) ifiBackend->vaddr,
           PC, "wrong load address" );
   failed |= verify( failed,
           (void*) ifiVerifier->data,
           (void*) ifiBackend->data,
           PC, "load data mismatch @ last element" );
 }

 return failed;
#else
 return 0; 
#endif 
}

//
// verify one data item
//
inline bool
Verifier::verify (bool failedAlready, void* thisData, void* backendData, aint_addr_t pc, char* errorMsg, int pos)
{
  if (thisData != backendData) {
    if (!failedAlready) {
      // print out header for first mismatch
      fflush(stdout);
      fflush(stderr);
      cerr << endl;
      cerr << "Verifier::Step at cycle " << pmint_get_sim_cycle()
           << " after " << PMINT_get_instr_issue_count()
           << " committed instructions "
           << " at PC " << (void*) pc << endl;
    }
    if ( pos != -1 ) {
     cerr << "\t" << errorMsg << " at element " << pos << endl;
    }
    else {
     cerr << "\t" << errorMsg << endl;
    }
    cerr << "\t" << "Verifier has " << thisData << endl;
    cerr << "\t" << "Backend  has " << backendData << endl;
    cerr << endl;
    return (true);
  } else {
    return (false);
  }
}

inline void
Verifier::dumpInsnInfo (inflight_inst_ptr ifi, thread_ptr pthread)
{
  fprintf(stderr, "\t [RA]=%016lx\n\t [RB]=%016lx\n\t [RC]=%016lx\n",
          REG(RA), REG(RB), REG(RC));

  if (ifi->picode->iflags & E_MEM_REF) {
    fprintf(stderr, "\n");
    fprintf(stderr, "\t vaddr=%016lx\n\t paddr=%016lx\n",
            ifi->vaddr, ifi->paddr);
    fprintf(stderr, "   Memory    ");
    for (int i = 0; i <= 0x0f; i++) {
      fprintf(stderr, " %02x", i);
    }
    fprintf(stderr, "\n");
    for (uchar* addr = (uchar*) ((ifi->paddr - 0x0f) & ~0xf);
         addr < (uchar*) ((ifi->paddr + 0x1f) & ~0xf);
         addr++)
    {
      if (((unsigned long) addr & 0x0f) == 0) {
        fprintf(stderr, "0x%p: ", addr);
      }
      fprintf(stderr, " %02x", *addr);
      if (((unsigned long) addr & 0x0f) == 0x0f) {
        fprintf(stderr, "\n");
      }
    }
  }
}
