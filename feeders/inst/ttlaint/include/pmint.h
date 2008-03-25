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
 * $Header$
 * $Log$
 * Revision 1.1  2001/11/15 16:09:43  klauser
 * - moving all (instruction) feeders one level down in directory hierarchy
 * - changed atf feeder to more generic trace feeder + atf reader
 *
 * Revision 1.5  2000/02/17 00:35:25  klauser
 * CSN-feeder-71
 *
 * Revision 1.4  1999/11/18 20:35:11  ahuja
 * CSN-feeder-61
 *
 * Revision 1.3  1999/10/19 20:50:33  shubu
 * CSN-feeder-47
 *
 * Revision 1.2  1999/08/19 13:25:34  espasa
 * CSN-feeder-28
 *
 * Revision 1.1  1999/07/02 00:37:11  espasa
 * *** empty log message ***
 *
 * Revision 1.1.1.1  1999/03/05  21:52:45  swallace
 * AINT code that came with the nov-24-98 version of the EV8 simulator
 *
 * Revision 1.12  1998/07/21  19:28:04  jamey
 * elaborated the comment about pmint_init
 *
 * Revision 1.11  1998/02/03  16:53:09  jamey
 * added pmint_get_thread_type
 *
 * Revision 1.10  1997/12/13  18:07:34  snyder
 * Added -atf (arana trace format) to aint trace reader
 *
 * Revision 1.9  1997/11/13  19:44:05  cfj
 * Added additional syscalls to aint.
 *
 * Revision 1.8  1997/09/03  20:05:47  cfj
 * Changes to AINT to support load_locked & store_conditional
 * See src/exec.c for more detailed comments.
 *
 * Revision 1.7  1997/07/08  14:03:41  cfj
 * Updates to have RPCC return SIM_cycle.
 *
 * Revision 1.6  1997/04/21  14:56:37  jamey
 * made multi-snapshot restore work
 *
 * Revision 1.5  1997/04/21  14:00:18  jamey
 * added snapshot/restore capability
 *
 * Revision 1.4  1997/03/04  21:52:19  jamey
 * added skipping/simulation control
 *
 * Revision 1.3  1996/08/12  18:48:52  jamey
 * folded in Vegas storebuf fixes
 *
 */
/* AINT-PM Interface definition */

/* int main(int argc, char **argv, char **envp)
 * Defined in AINT
 */
#ifndef PMINT_H
#define PMINT_H



/* Global flag to indicate error status upon return from pm interface functions */
extern int pmint_errno;

/* Interface functions  */

/* Called at initialization time, after aint has loaded code.
 * Passes to the PM the following:
 *
 * -- Maximum number of threads
 * -- Execution flags (trace option etc.)
 * -- Number of initial ready contexts (threads)
 */
void pmint_init(int max_nthreads, int cur_nthreads);

/* Indicates to PM that a new thread has begun, along with which
 * instruction stream it will execute
 */
void pmint_thread_begin(int tid);

/* Indicates to PM that the thread has died, and will not produce any
 * more instructions
 */
void pmint_thread_end(int tid);

/* Indicates to PM that the thread will not produce any more instructions
 * until it is unblocked
 */
void pmint_thread_block(int tid);

/* Indicates that the thread is ready to resume execution */
void pmint_thread_unblock(int tid);

void pmint_quiesce_thread(int tid, void *picode, unsigned long addr);

/* simulation control */
void pmint_request_begin_skipping (long how_many_to_skip);
void pmint_request_end_skipping ();
void pmint_request_end_simulation ();

void pmint_set_thread_type (int tid, /*thread_type_t*/int type);
/*thread_type_t */ int pmint_get_thread_type (int tid);
void pmint_record_event (int tid, long event, long value);

/* pass control to pmint, which will schedule instructions */
void pmint_execute();

/* give PMINT a chance to clean up */
void pmint_done();

/* Called to get current simulation cycle */
unsigned long pmint_get_sim_cycle();

/* Called when a thread calls usleep */
void pmint_thread_sleep(int tid);



/* interface functions provided for PMINT */


/*** FETCH ***/

/* returns instid */
unsigned long AINT_fetch_next_instr(int tid, unsigned long predicted_pc);
/* Causes AINT to fetch the next instruction from thread 'tid' at address
 * predicted_pc.  AINT will call the appropriate EVSIM_queue... procedure
 * to enqueue the instruction.  * Otherwise returns the unique id of the
 * dynamic instance of the fetched instruction.  */

/*** ISSUE and EXECUTE ***/

unsigned long AINT_issue_branch(int tid, unsigned long instid); 
/* Executes a jump or conditional branch instruction.
 * Returns the actual PC of the target instruction.
 *
 * Requires: instruction 'i' must be a branch which has previously
 * been fetched but not yet issued or killed.
 */

void AINT_issue_other(int tid, unsigned long instid); 
/* Executes instruction 'i'.
 *
 * Requires: instruction 'i' should not be a branch or memop. 
 * Instruction 'i' must have been previously fetched but not yet
 * issued or killed.
 *
 * Note: For certain instructions, such as div and sqrt, the latency
 *   until they can retire is data dependent.  If we want to model this
 *   precisely, this function could return the appropriate latency.
 */


unsigned long AINT_get_reg_value(int tid, instid_t instid, int reg_arg);
/* Returns the value of register 'reg_arg' (which is RA, RB, or RC) of
 * the given inst.
 *
 * Attempts to read an operand that does not exist (eg, RC for a load)
 * will cause an assertion failure.
 */ 

unsigned long AINT_read_reg(int tid, int log_reg);
/* Returns the value of register 'log_reg' (which is a logical register number)
 * at this point in time.
 */ 

void AINT_do_read(int tid, unsigned long instid); 
/* This informs AINT that the load described by instruction 'i'
 * has been fulfilled.
 * Note:This action is separate from the issue of the load (which
 *      computes the effective address) and the commit of the load
 *      (which allows other threads to see the stored value).
 *
 * Requires: instruction 'i' must be a load instruction which has
 * already been issued, but not killed and for which the read has not 
 * yet been done. (Note that the instruction may already have committed!)
 * This must be called for every fetched read instruction that is not killed.
 */

void AINT_do_spec_write(int tid, unsigned long instid);
void AINT_do_write(int tid, unsigned long instid);

/* Called when pm decides a store_cond instr fails */
void AINT_stc_failed(int tid, unsigned long instid);

/*** COMMIT ***/

void AINT_commit_instr(int tid, unsigned long instid);
/* Notifies AINT that instr 'i' has been commited.
 * Requires: instruction 'i' must be an instruction which has
 * already been fetched, but not yet committed or killed.
 */

/*** FETCH, ISSUE, COMMIT ***/
unsigned long AINT_fetch_issue_commit_next_instr(int tid, unsigned long predicted_pc);

typedef enum {KT_mispredict, KT_trap} Kill_Type;

void AINT_kill_instrs(int tid, unsigned long instid, Kill_Type kill_reason);
/* Kills instruction 'i', and all instructions following 'i' in 
 *   the same thread. 
 * AINT unwinds its state to erase any effects of the killed instructions.
 * 'kill_reason' is used to reset where AINT will fetch from.
 * If 'kill_reason' is:
 *    KT_mispredict:  Compute next fetch by examining instruction
 *        prior to 'i'.  This instruction must be a branch.  Next fetch 
 *        will follow the true path for this branch.
 *    KT_trap:  next fetch will be same instruction as instr 'i'.
 *
 * Requires: instruction 'i' must be an instruction which has
 *  already been fetched, but not yet committed or killed.
 *  If kill_type is KT_mispredict, inst prior to 'i' must be a branch.
 */


#endif /* PMINT_H */
