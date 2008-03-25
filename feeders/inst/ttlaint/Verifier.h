//----------------------------------------------------------------------------
// Verifier.h - verification thread for commited instructions
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
// $Id: Verifier.h 466 2003-02-24 20:51:39Z klauser $
//----------------------------------------------------------------------------

#ifndef _Verifier_h
#define _Verifier_h


// ASIM public modules
#include "asim/provides/instfeeder_interface.h"

// ASIM local module
#include "aint.h"

// #include "Scheduler.h"
typedef long SimTimeUnit;
typedef long aint_addr_t;

enum VerifierMode {
  VerifierCheck = 0,
  VerifierSkip
};

class Verifier {
private:
  aint_addr_t PC;     // current PC
  int TID;            // AINT thread ID
  //path_ptr Path;      // pointer to AINT path object

public:

  // Constructor
  Verifier( int tid, aint_addr_t startPC );

  //
  // This is the function called each time we commit a Backend instruction. This
  // function will do one step on the Verifier thread and then compare the register
  // contents of the Backend and Verifier instructions. If they do not match, the verifier
  // flags an error
  //
  void Step( ASIM_INST instBackend, inflight_inst_ptr ifiBackend, VerifierMode mode );

  //
  // Function called for vector instructions. This function is called withing 'Step'. It works
  // following the same scheme as the main 'Step' function, but we need to  be able to 
  // check every single value inside a vector register
  //
  bool VectorStep(thread_ptr threadVerifier, inflight_inst_ptr ifiVerifier, thread_ptr threadBackend,  inflight_inst_ptr ifiBackend, ASIM_INST instBackend);

  //
  // Auxiliary routine use to compare to 64 bit quantities (typeless comparison). Dumps an error if they differ.
  // The last parameter, 'pos', is only used when 'verify' is called from the VectorStep function. It is only
  // useful when comparing elements inside a vector register
  //
  inline bool verify ( bool failedAlready, void* thisData, void* backendData, aint_addr_t PC, char* errorMsg, int pos = -1);

  //
  // Dumps the instruction information
  //
  inline void dumpInsnInfo ( inflight_inst_ptr ifi, thread_ptr pthread);
};

#endif // _Verifier_h
