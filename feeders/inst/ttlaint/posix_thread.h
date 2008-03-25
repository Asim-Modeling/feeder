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
 * Author:  Harish Patil
 */

#ifndef _POSIX_THREAD_
#define _POSIX_THREAD_

#define MAX_NTHREADS 4

typedef struct thread *AINT_THREAD;

#ifdef __cplusplus
extern "C" void posix_thread_init();
extern void posix_register_aint_thread(AINT_THREAD t); // This should not be called from C code
extern "C" void posix_register_mutex(void *key);
extern "C" int is_posix_mutex_locked(void *key);
extern "C" void posix_mutex_lock(void *key, AINT_THREAD owner);
extern "C" void posix_mutex_unlock(void *key, AINT_THREAD unlocker);
extern "C" void posix_mutex_add_waiting_thread(void *key, AINT_THREAD waiter);
extern "C" AINT_THREAD posix_mutex_get_next_waiting_thread(void *key);
extern "C" void posix_condition_add_waiting_thread(void *cond_key, void *mutex_key, AINT_THREAD waiter);
extern "C" void * posix_condition_get_mutex_key(void *cond_key);
extern "C" AINT_THREAD posix_condition_get_next_waiting_thread(void *cond_key);
extern "C" void posix_condition_signalled(void *cond_key);
extern "C" void posix_condition_broadcasted(void *cond_key);
#else
extern void posix_thread_init();
extern void posix_register_mutex(void *key);
extern int is_posix_mutex_locked(void *key);
extern  void posix_mutex_lock(void *key, AINT_THREAD owner);
extern  void posix_mutex_unlock(void *key, AINT_THREAD unlocker);
extern  void posix_mutex_add_waiting_thread(void *key, AINT_THREAD waiter);
extern AINT_THREAD posix_mutex_get_next_waiting_thread(void *key);
extern void posix_condition_add_waiting_thread(void *cond_key, void *mutex_key, AINT_THREAD waiter);
extern  void * posix_condition_get_mutex_key(void *cond_key);
extern  AINT_THREAD posix_condition_get_next_waiting_thread(void *cond_key);
extern void posix_condition_signalled(void *cond_key);
extern void posix_condition_broadcasted(void *cond_key);
#endif
#endif /* _POSIX_THREAD_ */
