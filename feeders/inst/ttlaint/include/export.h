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

/*
 * Definitions of exported global variables.
 */

#ifndef EXPORT_H
#define EXPORT_H

#include <stdio.h>
#include "event.h"
#include "macros.h"

/* The maximum number of processes that can be simulated. This is used
 * to allocate space. This value can be changed with the "-p" command
 * line option up to a maximum of MAXPROC.
 */
extern int Max_nprocs;
extern int Max_nthreads;

/* The absolute maximum number of processes that can be simulated. */
#define MAXPROC 256

/* the number of existing processes */
extern int Nprocs;
extern int Nthreads;

/* the maximum process id ever used (process ids start at zero) */
extern int Maxpid;
extern int Maxtid;

/* The file descriptor used by AINT for its output. Can be assigned by
 * the user program. Defaults to stderr. If NULL, no output from MINT
 * is generated.
 */
extern char *Aint_output_filename;
extern FILE *Aint_output;

/* marker statistics output file */
extern char *Aint_marker_filename;
extern FILE *Aint_marker_output;

/* Ckpoint_freq controls the frequency of calls to sim_checkpoint().
 * This can be set on the command line as well as changed by the user while
 * the program is running. If set to zero, no further calls to sim_checkpoint()
 * will occur.
 */
extern unsigned int Ckpoint_freq;

/* If true, then the simulator must supply a value on a read by using
 * the pointer "value" in the event structure to write the value that
 * should be used. On a write, the "value" pointer points to the value
 * being written by the object program. This flag is set by the "-V"
 * command line option.
 *
 extern int Verify_protocol; */

/* If true, then the simulator must explicitly release a lock or
 * semaphore by calling RC_unlock() and RC_vsema().
 *
 extern int Release_consist; */

/* If true, then a connection to a remote client has been established 
extern int Connection_established; */

/* The time of the first fork, or zero if no forks yet */
extern aint_time_t First_fork_elapsed;

/* The accumulated cpu time of processor 0 at the time of the first fork */
extern aint_time_t First_fork_cpu;

/* If true, then an optimization is enabled where the back-end function
 * prmcache_read() is called to determined if a reschedule should occur.
 * An event pointer is passed in as the only argument. A return value
 * of 0 means "no reschedule"; any other return value means "reschedule".
 * Similarly for prmcache_write(). These variables default to zero and
 * must be set to 1 in sim_init() in order to enable this optimization.
 *
extern int Use_prmcache_read;
extern int Use_prmcache_write; */

#endif /* !EXPORT_H */
