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
 * pthread.c
 */


/* Implementation of AINT-pthread stubs that generate "fake" pthread syscalls
 * which are trapped by AINT.
 */


#include <stdio.h>
#include "aint_pthread.h"

#if (OSVER == 32)
typedef void *pthread_t;
typedef void *pthread_attr_t;
typedef void *pthread_mutex_t;
typedef void *pthread_cond_t;
typedef void *pthread_mutexattr_t;
#endif

extern char *malloc(size_t);

#define CALLSYS 131
/* for call_pal */

#define DEBUG 1
/* #undef DEBUG */

typedef struct {
    void *                   (*start_routine)(void *); 
    void                     *arg;
} pif_t;

static void *(__pthread_initial_fcn)(void *); 
/* static pthread_startroutine_t __pthread_initial_fcn; */

int __pthread_equal(pthread_t t1, pthread_t t2)
{
  return t1 == t2;
}

int __pthread_create(
		   pthread_t                *thread,
		   const pthread_attr_t     *attr,
		   void *                   (*start_routine)(void *), 
		   void                     *arg)
{

    long callno = SYS_aint_pthread_create;
    long ret_val;
    pthread_t *newthread = thread;
    pif_t *ppif = (pif_t*)malloc(sizeof(pif_t));
    /*
     * WARNING:
     * Malloc is not thread safe. 
     * This could cause problems if all calls to pthread_create aren't serialized.
     */
    ppif->start_routine = start_routine;
    ppif->arg = arg;
    /* Copy the arguments into real registers */

#ifdef DEBUG
    printf("AINT __pthread_create: making syscall:%d thread:0x%p attr:0x%p start_routine:0x%p arg:0x%p\n", 
           callno,thread,attr,start_routine,arg);
    fflush(stdout);
#endif

    asm(  "\n"
	  "bis %1, $31, $0   \n"
	  "bis %2, $31, $16  \n"
	  "bis %3, $31, $17  \n"
	  "bis %5, $31, $19  \n"
	  "bis %4, $31, $18  \n"
	  "call_pal      131 \n"
	  "bis $19, $31, %0   \n"
	  : "=r" (ret_val)
	  : "r" (callno), "r" (newthread), "r" (attr), "r" (__pthread_initial_fcn), "r" (ppif)
	  : "$16", "$17", "$18", "$19", "$0"
	  );

    return ret_val;

}


pthread_t __pthread_self()
{

    pthread_t ret_val;
    long callno = SYS_aint_pthread_self;

  asm( "bis %1, $31, $0 \n"
       "call_pal     131 \n"
       "bis $0, $31, %0 \n"
       : "=r" (ret_val)
       : "r" (callno)
       : "$16", "$17", "$18", "$19", "$0"
       );
  return (ret_val);
}


int __pthread_exit(void *value_ptr)
{

    long ret_val;
    long callno = SYS_aint_pthread_exit;

#ifdef DEBUG
  printf("AINT __pthread_exit: making syscall %d\n", callno); fflush(stdout);
#endif

  asm( "bis %1, $31, $0 \n"
       "bis %2, $31, $16 \n"
       "call_pal     131 \n"
       "bis $19, $31, %0 \n"
       : "=r" (ret_val)
       : "r" (callno), "r" (value_ptr)
       : "$16", "$17", "$18", "$19", "$0"
       );
  return (ret_val);
}



int __pthread_join (pthread_t thread, void **value_ptr)
{
    long ret_val;
    long callno = SYS_aint_pthread_join;

#ifdef DEBUG
  printf("AINT pthread_join (%d): making syscall %d\n", thread, callno); fflush(stdout);
#endif
  asm( "bis %1, $31, $0 \n"
       "bis %2, $31, $16 \n"
       "bis %3, $31, $17 \n"
       "call_pal     131 \n"
       "bis $19, $31, %0 \n"
       : "=r" (ret_val)
       : "r" (callno), "r" (thread), "r" (value_ptr)
       : "$16", "$17", "$18", "$19", "$0"
       );
  return (ret_val);

}

static void *(__pthread_initial_fcn)(void *pv) 
{
  pif_t *ppif = (pif_t *)pv;
  if (ppif != NULL && ppif->start_routine != NULL){
#ifdef DEBUG
  printf("AINT __pthread_initial_fnc being called with start_routine:0x%p arg:0x%p\n", 
         ppif->start_routine,ppif->arg); fflush(stdout);
#endif
    (ppif->start_routine)(ppif->arg);
  }
  __pthread_exit(NULL);
}

int __pthread_mutex_init(pthread_mutex_t	  *mutex_ptr)
{

    long ret_val;
    long callno = SYS_aint_pthread_mutex_init;

#ifdef DEBUG
  printf("AINT __pthread_mutex_init: making syscall %d\n", callno); fflush(stdout);
#endif

  asm( "bis %1, $31, $0 \n"
       "bis %2, $31, $16 \n"
       "call_pal     131 \n"
       "bis $19, $31, %0 \n"
       : "=r" (ret_val)
       : "r" (callno), "r" (mutex_ptr)
       : "$16", "$17", "$18", "$19", "$0"
       );
  return (ret_val);
}


int __pthread_mutex_lock(pthread_mutex_t	  *mutex_ptr)
{

    long ret_val;
    long callno = SYS_aint_pthread_mutex_lock;

#ifdef DEBUG
  printf("AINT __pthread_mutex_lock: making syscall %d\n", callno); fflush(stdout);
#endif

  asm( "bis %1, $31, $0 \n"
       "bis %2, $31, $16 \n"
       "call_pal     131 \n"
       "bis $19, $31, %0 \n"
       : "=r" (ret_val)
       : "r" (callno), "r" (mutex_ptr)
       : "$16", "$17", "$18", "$19", "$0"
       );
  return (ret_val);
}

int __pthread_mutex_unlock(pthread_mutex_t	  *mutex_ptr)
{

    long ret_val;
    long callno = SYS_aint_pthread_mutex_unlock;

#ifdef DEBUG
  printf("AINT __pthread_mutex_unlock: making syscall %d\n", callno); fflush(stdout);
#endif

  asm( "bis %1, $31, $0 \n"
       "bis %2, $31, $16 \n"
       "call_pal     131 \n"
       "bis $19, $31, %0 \n"
       : "=r" (ret_val)
       : "r" (callno), "r" (mutex_ptr)
       : "$16", "$17", "$18", "$19", "$0"
       );
  return (ret_val);
}

int __pthread_cond_init(pthread_cond_t *condition_ptr)
{

    long ret_val;
    long callno = SYS_aint_pthread_condition_init;

#ifdef DEBUG
  printf("AINT __pthread_cond_init: making syscall %d\n", callno); fflush(stdout);
#endif

  asm( "bis %1, $31, $0 \n"
       "bis %2, $31, $16 \n"
       "call_pal     131 \n"
       "bis $19, $31, %0 \n"
       : "=r" (ret_val)
       : "r" (callno), "r" (condition_ptr)
       : "$16", "$17", "$18", "$19", "$0"
       );
  return (ret_val);
}


int __pthread_cond_wait (pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    long ret_val;
    long callno = SYS_aint_pthread_condition_wait;

#ifdef DEBUG
  printf("AINT pthread_cond_wait: making syscall %d\n", callno); fflush(stdout);
#endif
  asm( "bis %1, $31, $0 \n"
       "bis %2, $31, $16 \n"
       "bis %3, $31, $17 \n"
       "call_pal     131 \n"
       "bis $19, $31, %0 \n"
       : "=r" (ret_val)
       : "r" (callno), "r" (cond), "r" (mutex)
       : "$16", "$17", "$18", "$19", "$0"
       );
  return (ret_val);

}

int __pthread_cond_signal(pthread_cond_t *condition_ptr)
{

    long ret_val;
    long callno = SYS_aint_pthread_condition_signal;

#ifdef DEBUG
  printf("AINT __pthread_cond_signal: making syscall %d\n", callno); fflush(stdout);
#endif

  asm( "bis %1, $31, $0 \n"
       "bis %2, $31, $16 \n"
       "call_pal     131 \n"
       "bis $19, $31, %0 \n"
       : "=r" (ret_val)
       : "r" (callno), "r" (condition_ptr)
       : "$16", "$17", "$18", "$19", "$0"
       );
  return (ret_val);
}


int __pthread_cond_broadcast(pthread_cond_t *condition_ptr)
{

    long ret_val;
    long callno = SYS_aint_pthread_condition_broadcast;

#ifdef DEBUG
  printf("AINT __pthread_cond_broadcast: making syscall %d\n", callno); fflush(stdout);
#endif

  asm( "bis %1, $31, $0 \n"
       "bis %2, $31, $16 \n"
       "call_pal     131 \n"
       "bis $19, $31, %0 \n"
       : "=r" (ret_val)
       : "r" (callno), "r" (condition_ptr)
       : "$16", "$17", "$18", "$19", "$0"
       );
  return (ret_val);
}
int __pthread_mutex_destroy(pthread_mutex_t   *mutex)
{

}

int pthread_attr_init(pthread_attr_t   *attr)
{

}

int __pthread_attr_setstacksize()
{

}


/* mutexes */




