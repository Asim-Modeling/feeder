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
 
//
// Authors:  Artur Klauser
//

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
#include "asim/cmd.h"

// ASIM local module
#include "null_feeder.h"

// ASIM public modules
#include "asim/provides/instfeeder_interface.h"

/********************************************************************
 *
 * Helper functions
 *
 *******************************************************************/

static bool 
Parse (UINT32 argc, char **argv)
/*
 * Parse command line arguments, return FALSE if there is
 * a parse error.
 */
{
  for (UINT32 i = 0; i < argc; i++) {
    if (!strcmp(argv[i], "-eventfile")) {
      if (i < argc) {
      } else {
	ASIMERROR("-eventfile commandline directive given without filename\n");
      }
      continue;
    }
  }

  return(true);
}

/********************************************************************
 *
 * FEEDer callbacks
 *
 *******************************************************************/

UINT64
FEED_Skip (ASIM_THREAD thread, UINT64 n, INT32 markerID)
{
  fprintf(stderr,"FEED_Skip is not supported on NULL feeder\n");
  exit(1);
  return 0;
}

void
FEED_Marker (ASIM_MARKER_CMD cmd, ASIM_THREAD thread,
             UINT32 markerID, UINT64 markerPC,
             UINT32 instBits, UINT32 instMask)
{
  fprintf(stderr,"FEED_Marker is not supported on NULL feeder\n");
  exit(1);
}

UINT64
FEED_Symbol (ASIM_THREAD thread, char* name)
{
  UINT32 tid = thread->Uid();
  return(FEED_Symbol(tid, name));
}

UINT64
FEED_Symbol (UINT32 tid, char* name)
{
  fprintf(stderr,"FEED_Symbol is not supported on NULL feeder\n");
  exit(1);
  return 0;
}

UINT64 
FEED_ITranslate(ASIM_THREAD thread, UINT64 vpc)
{
  return 0; 
}

void
FEED_Fetch (ASIM_THREAD thread, UINT64 predicted_pc, ASIM_INST inst)
/*
 * Fetch 'inst'...
 */
{
  //inst->SetThread(thread);
}


void
FEED_Issue (ASIM_INST inst)
/*
 * Issue 'inst'...
 */
{
}


void
FEED_DoRead (ASIM_INST inst)
/*
 * 'inst' is a load accessing the cache
 */
{
}

void
FEED_DoSpecWrite (ASIM_INST inst)
/*
 * Inst is a store making the stored value non-speculative
 */
{
}

void
FEED_DoWrite (ASIM_INST inst)
/*
 * Inst is a store making the stored value non-speculative
 */
{
}


void
FEED_Commit (ASIM_INST inst)
/*
 * 'inst' is committed...
 */
{
}


void
FEED_Kill (ASIM_INST inst, bool mispredict, bool killMe)
/*
 * kill 'inst'...
 */
{
}


void
FEED_Marker (ASIM_MARKER_CMD cmd, ASIM_THREAD thread, UINT32 markerID, IADDR_CLASS markerPC, UINT32 instBits, UINT32 instMask)
{
}

/********************************************************************
 *
 * Controller calls
 *
 *******************************************************************/

bool
FEED_Init (UINT32 argc, char **argv, char **envp)
/*
 * Initialize NULL instruction feeder. Return false
 * if we have an error during initialization.
 */
{
   ASIM_THREAD thread;
    //
    // Parse the command-line.
    //
    printf ("NULL feeder initializing ...\n");

    if (! Parse(argc, argv)) {
      return(false);
    }

    // start up one fake thread
    thread = new ASIM_THREAD_CLASS(/*tid*/0, /*pid*/0, /*vpc*/0);
         
    CMD_ThreadBegin(thread);

    return(true);
}


void
FEED_Done (void)
/*
 * Cleanup..
 */
{
}


void
FEED_Usage (FILE *file)
/*
 * Print usage...
 */
{
    fprintf(file, "\nFeeder usage: ... [-eventfile <filename>] ...\n");
}
