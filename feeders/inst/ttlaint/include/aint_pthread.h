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

#ifndef AINT_PTHREAD_H
#define AINT_PTHREAD_H

/* Header file for "fake" pthreads */

/* Include the syscall number definitions */
#include <aint_syscall.h>

/* This defines the interface to DU 4.0-like pthreads (POSIX 1003.1c-1995) */

#ifndef _THREAD_SAFE
typedef void * pthread_t;
typedef void * pthread_attr_t;
typedef void * pthread_mutex_t;
typedef void * pthread_mutexattr_t;
typedef void * pthread_cond_t;

int pthread_create(pthread_t *, const pthread_attr_t *, void * (*)(void *), void *);

int pthread_join(pthread_t, void **);

int pthread_mutex_init(pthread_mutex_t, void **);
int pthread_condition_init(pthread_cond_t, void **);
int pthread_exit(void *);
#endif /* _THREAD_SAFE */

#endif
