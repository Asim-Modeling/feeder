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
 *
 * pthread_stubs.c
 *
 *
 */
#include <stdio.h>

extern int __warning(const char *fmt, ...);

/* Implementation of AINT-decthread stubs that generate "fake" pthread syscalls
 * which are trapped by AINT.
 */


/*
 * An attributes object is created to specify the attributes of other CMA
 * objects that will be created.
 */


int pthread_attr_create (pthread_attr_t *attr)
{
  return 0;
}

/*
 * An attributes object can be deleted when it is no longer needed.
 */

int pthread_attr_delete (pthread_attr_t *attr)
{
  return 0;
}

int pthread_attr_setinheritsched(pthread_attr_t *attr, int inherit)
{
  __warning("pthread_attr_setinheritsched: inherit=%d\n", inherit);
  return 0;
}

int pthread_attr_setprio(pthread_attr_t *attr, int priority)
{
  __warning("pthread_attr_setprio: inherit=%d\n", priority);
  return 0;
}

int pthread_attr_setsched(pthread_attr_t *attr, int scheduler)
{
  __warning("pthread_attr_setsched: scheduler=%d\n", scheduler);
  return 0;
}
int pthread_attr_setstacksize(pthread_attr_t *attr, long stacksize)
{
  __warning("pthread_attr_setstacksize: stacksize=%d\n", stacksize);
  return 0;
}



pthread_condattr_t pthread_condattr_default;

int pthread_cond_init(pthread_cond_t *cond, pthread_condattr_t attr)
{
  __pthread_cond_init(cond);
  return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
  __warning("pthread_cond_destroy: cond=%lx\n", cond);
  return 0;
}

int pthread_cond_broadcast(pthread_cond_t *cond)
{
  __pthread_cond_broadcast(cond);
  return 0;
}

int pthread_cond_signal(pthread_cond_t *cond)
{
  __pthread_cond_signal(cond);
  return 0;
}



int pthread_create(pthread_t *pthread, pthread_attr_t attr, 
		   void *                   (*startroutine)(void *),
		   void * arg)
{
  int rc; 
  printf("pthead_stubs:pthread_create pthread 0x%lx attr 0x%lx startroutine 0x%lx arg 0x%lx\n",pthread, attr, startroutine, arg);
  rc =  __pthread_create(pthread,
			    attr,
			    startroutine,
			    arg);
 return rc;
}

int pthread_detach(pthread_t pthread)
{
  __warning("pthread_detach: thread=%lx\n", pthread);
  return 0;
}

int pthread_exit(void * status)
{
  return __pthread_exit(status);
}

int pthread_join(pthread_t pthread, void *status)
{
  printf("pthead_stubs:pthread_join %d\n",pthread);
  return __pthread_join((void*)pthread, (void*)status);
}

pthread_t pthread_self()
{
  extern pthread_t __pthread_self();
  pthread_t thr;
  thr = __pthread_self();
  return thr;
}

/*
 * The current thread can request that a thread terminate its execution.
 */
int pthread_cancel(pthread_t thread)
{
  __warning("pthread_cancel: thread =%lx\n", thread);
  return 0;
}

/*
 * The current thread can enable or disable alert delivery (PTHREAD
 * "cancels"); it can control "general cancelability" (CMA "defer") or
 * just "asynchronous cancelability" (CMA "asynch disable").
 */
int pthread_setasynccancel (int	state)
{
  __warning("pthread_setasynccancel: state=%d\n", state);
  return 0;
}


/* 
 * The following routines create, delete, lock and unlock mutexes.
 */

pthread_mutexattr_t pthread_mutexattr_default;

int pthread_mutex_init (pthread_mutex_t *mutex,	pthread_mutexattr_t attr)
{
  __pthread_mutex_init(mutex);
  return 0;
}

int pthread_mutex_destroy (pthread_mutex_t *mutex)
{
  __warning("pthread_mutex_destroy: mutex=%lx\n", mutex);
  return 0;
}

int pthread_mutex_lock (pthread_mutex_t *mutex)
{
  __pthread_mutex_lock(mutex);
  return 0;
}

int pthread_mutex_unlock (pthread_mutex_t *mutex)
{
  __pthread_mutex_unlock(mutex);
  return 0;
}

int pthread_bind_to_cpu_np(pthread_t *thread, unsigned long *cpu_mask)
{
  return 0;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
  __pthread_cond_wait(cond, mutex);
  return 0;
}
