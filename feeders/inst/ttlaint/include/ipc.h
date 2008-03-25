#ifndef _ipc_h_
#define _ipc_h_
/*
 * Structure definitions for supporting System V semaphores and
 * shared memory.
 *
 * Copyright (C) 1993 by Jack E. Veenstra (veenstra@cs.rochester.edu)
 * 
 * This file is part of MINT, a MIPS code interpreter and event generator
 * for parallel programs.
 * 
 * MINT is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 * 
 * MINT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MINT; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include "event.h"
#include "queue.h"
#include "icode.h"

struct sysv_semid_ds {
    struct ipc_perm sem_perm;
    struct sysv_sem *sem_base;
    ushort sem_nsems;
    aint_time_t sem_otime;
    aint_time_t sem_ctime;
    int inuse;
};

struct sysv_sem {
    ushort semval;
    short sempid;
    short semncnt;
    short semzcnt;
    qnode_t nwait_q;
    qnode_t zwait_q;
};

struct sysv_shm {
  struct ipc_perm shm_perm;
  char *paddr; /* address of mmap()'d segment to for this shm */
  int inuse;
  /* ushort real_seg;  true if a system V segment was allocated */
  int size;
};

#define MAX_SEMASET 1024
#define MAX_SHMSEG   256

struct  sysv_semid_ds;
extern icode_ptr aint_semget(inflight_inst_ptr, icode_ptr  , thread_ptr );
		/* Prototype include a typedef name.
		   It should be moved after the typedef declaration */
extern void semcpin(struct sysv_semid_ds *  , struct sysv_semid_ds * );
extern void semcpout(struct sysv_semid_ds *  , struct sysv_semid_ds * );
extern icode_ptr aint_semop(inflight_inst_ptr, icode_ptr  , thread_ptr );
		/* Prototype include a typedef name.
		   It should be moved after the typedef declaration */
extern icode_ptr aint_semctl(inflight_inst_ptr,  icode_ptr  , thread_ptr );
		/* Prototype include a typedef name.
		   It should be moved after the typedef declaration */
/* shmseg_allocate does the work for aint_shmget() */
extern int shmseg_allocate (long key, long size, long flags, long *perrno);
extern icode_ptr aint_shmget(inflight_inst_ptr, icode_ptr  , thread_ptr );
		/* Prototype include a typedef name.
		   It should be moved after the typedef declaration */
extern int is_valid(long );
/* shmseg_attach() does the work for aint_shmat() */
ulong shmseg_attach(process_ptr process, int shmid, ulong addr, ulong flags, long *perrno);
extern icode_ptr aint_shmat(inflight_inst_ptr, icode_ptr  , thread_ptr );
		/* Prototype include a typedef name.
		   It should be moved after the typedef declaration */
extern icode_ptr aint_shmdt(inflight_inst_ptr, icode_ptr  , thread_ptr );
		/* Prototype include a typedef name.
		   It should be moved after the typedef declaration */
extern icode_ptr aint_shmctl(inflight_inst_ptr, icode_ptr  , thread_ptr );
		/* Prototype include a typedef name.
		   It should be moved after the typedef declaration */
#endif
