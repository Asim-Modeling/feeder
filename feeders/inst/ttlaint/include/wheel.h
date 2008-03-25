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

#ifndef WHEEL_H
#define WHEEL_H

#include "protos.h"
#include "task.h"

/* WHEEL_SIZE must be a power of two */
#define WHEEL_SIZE 32768
#define WHEEL_MASK (WHEEL_SIZE-1)

/* use only the fields we need and pad the size until it is a power of 2 */
typedef struct wheel_ask {
  struct task *next;
  struct task *prev;
  aint_time_t time;
  int priority;
} wheel_task_t, *wheel_task_ptr;

extern wheel_task_ptr Wheel;
extern wheel_task_ptr Curptr;
extern wheel_task_ptr Endptr;
extern aint_time_t Start_time;
extern aint_time_t End_time;
extern task_ptr Future_q;

extern void wheel_init();
extern int wheel_empty();
extern int task_cleanup();

/* Define macros */
#define INLINE_TASK_EXTRACT(pnew) pnew = task_extract()
#define INLINE_TASK_INSERT(pnew) task_insert(pnew)

extern task_ptr task_pull_from_future();

#ifdef DO_INLINING
#ifdef __GNUC__
   inline
#else
#  pragma inline(task_extract)
#endif
#endif /* INLINE */
static
task_ptr task_extract()
{
  task_ptr pnew;

  /* search the time wheel first */
  pnew = Curptr->next;
  if (pnew != (task_ptr) Curptr) {
    INLINE_REMOVE(pnew);
    return pnew;
  } else {

    task_ptr tptr, next;

    while (1) {
      /* Search the queues until we get to the end. Since we put a dummy item
       * in the last (unused) queue, we don't need to check for overflow of the
       * Curptr in each iteration
       */
      do {
	Curptr++;
	pnew = Curptr->next;
      } while (pnew == (task_ptr) Curptr);
    
      /* if we found a tsk, remove it and return */
      if (Curptr != Endptr) {
	INLINE_REMOVE(pnew);
	return(pnew);
      }

      /* Wrap around processing */
      Curptr = &Wheel[0];
      End_time += WHEEL_SIZE;
      Start_time += WHEEL_SIZE;
      pnew = Future_q->next;
      if (pnew == Future_q)
	return NULL;

      for (; pnew != Future_q; pnew = next) {
	int diff = pnew->time - Start_time;
	next = pnew->next;

	if (diff < WHEEL_SIZE) {
	  /* insert in priority order */
	  for (tptr = Wheel[diff].prev; tptr->priority < pnew->priority;
	       tptr = tptr->prev)
	    ;

	  INLINE_REMOVE(pnew);
	  INLINE_INSERT_AFTER(tptr, pnew);
	}
      }

      pnew = Curptr->next;
      if (pnew != (task_ptr) Curptr) {
	INLINE_REMOVE(pnew);
	return pnew;
      }
    }
  }
}

#ifdef DO_INLINING
#ifdef __GNUC__
  inline
#else
# pragma inline(task_insert)
#endif
#endif /* INLINE */
static
void
task_insert(task_ptr ptask)
{
  int index;
  task_ptr tptr;

  /* printf("task_insert: time is %d\n", (int) ptask->time); */
  index = ptask->time - Start_time;

  if (index < WHEEL_SIZE) {
    /*
     * insert in priority order
     */
    for (tptr = Wheel[index].prev;
	 tptr->priority < ptask->priority;
	 tptr = tptr->prev) {
      /*
       * Empty statement
       */
    }
    INLINE_INSERT_AFTER(tptr, ptask);
  } else {
    INLINE_ENQUEUE(Future_q, ptask);
  }
}

#endif
