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
 * Definition of the event structure and bit field mnemonics
 */

#ifndef TASK_H
#define TASK_H

#include <values.h>
#include "event.h"
#include "export.h"

/* The order of the first few fields is important.
 * The time wheel defines a struct that contains only next, prev, time
 * and priority.
 */
typedef struct task {
  struct task *next;
  struct task *prev;
  aint_time_t time;
  int priority;      /* larger number == higher priority */
  int tid;
  int pid;
  int (*ufunc)(struct task *);    /* user-defined function to call */
  struct task *tstack;    /* stack of tasks to execute next */
  struct event *pevent;    /* pointer to an event structure */
  long ival1;		  /* user-defined integer value */
  long ival2;		  /* user-defined integer value */
  long ival3;		  /* user-defined integer value */
  long ival4;		  /* user-defined integer value */
  long ival5;		  /* user-defined integer value */
  long ival6;		  /* user-defined integer value */
  double dval1;		 /* user-defined double value */
  double dval2;		 /* user-defined double value */
  aint_time_t tval1;	 /* user-defined time value */
  aint_time_t tval2;	 /* user-defined time value */
  aint_time_t tval3;	 /* user-defined time value */
  void *uptr;		 /* user-defined pointer */
  void *uptr2;		 /* user-defined pointer */
  void *uptr3;		 /* user-defined pointer */
  void *uptr4;		 /* user-defined pointer */
  long virtual_address;  /* user-defined to contain value of ptask->pevent->vaddr */
  long event_type;       /* user-defined to contain value of ptask->pevent->type */
  int (*ufunc2)(struct task *);	  /* user-defined function pointer */
  struct task *unext;	  /* user-defined forward link */
  struct task *uprev;   /* user-defined backward link */

  /* There are supposed to be more things in here, that the user may define.
   * Will deal with them later
   */
} task_t, *task_ptr;

typedef int (*PFTASK)(struct task *ptask);

/* the default priority */
#define DEF_PRIORITY 0

#define PRIORITY_HIGH (~(1 << 31))
#define PRIORITY_LOW  (1 << 31)

#define T_ADVANCE 0
#define T_FREE    1
#define T_NO_ADVANCE T_FREE    /* For backwards compatibility? */
#define T_YIELD   2
#define T_CONTINUE 3
#define T_MAX     5    /* Must be last */


#ifdef __cplusplus
extern "C" {
#endif

/* sim_* function prototypes */
void sim_block(task_ptr ptask);
void sim_checkpoint(task_ptr ptask);
void sim_deadlock();
void sim_done(double elapsed, double cpu_time);
int sim_exit(task_ptr ptask);
void sim_fork(task_ptr ptask);
int sim_init(int argc, char *argv[]);
int sim_inst(task_ptr ptask);
int sim_load_locked(task_ptr ptask);
int sim_memory_barrier(task_ptr ptask);
int sim_null(task_ptr ptask);
int sim_read(task_ptr ptask);
int sim_store_conditional(task_ptr ptask);
int sim_terminate(task_ptr ptask);
void sim_unblock(task_ptr ptask);
void sim_usage(int argc, char **argv);
int sim_write(task_ptr ptask);
void sim_pthread_create(task_ptr ptask);

task_ptr unblock_proc(int pid, aint_time_t tval);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif
