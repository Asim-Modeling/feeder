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
// Author:  Harish Patil
//


// generic
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/trace.h"

// ASIM local module
#include "icode.h"
#include "posix_thread.h"

#define _INIT_MUTEX_COUNT_ 100
#define _INIT_CONDITION_COUNT_ 100
#define _INIT_THREAD_COUNT_ MAX_NTHREADS

typedef enum posix_mutex_state {
        POSIX_MUTEX_UNINITIALIZED,
        POSIX_MUTEX_UNLOCKED,
        POSIX_MUTEX_LOCKED
} POSIX_MUTEX_STATE;

typedef enum posix_condition_state {
        POSIX_CONDITION_UNINITIALIZED,
        POSIX_CONDITION_UNSIGNALLED,
        POSIX_CONDITION_SIGNALLED,
        POSIX_CONDITION_BROADCASTED
} POSIX_CONDITION_STATE;


typedef enum aint_thread_state {
        POSIX_THREAD_UNKNOWN,
        POSIX_THREAD_RUNNING,
        POSIX_THREAD_BLOCKED,
        POSIX_THREAD_KILLED,
        POSIX_THREAD_DONE
} POSIX_THREAD_STATE;

typedef class aint_thread_item_class {
  private: 
     AINT_THREAD thread;
     POSIX_THREAD_STATE state;
  public: 
     AINT_THREAD get_aint_thread() { return thread; } 
     void set_aint_thread(AINT_THREAD t) { thread = t; } 
     POSIX_THREAD_STATE get_state() { return state; } 
     void set_state(POSIX_THREAD_STATE s) { state = s; } 
} *AINT_THREAD_ITEM, AINT_THREAD_ITEM_CLASS;
 
typedef class aint_thread_list_class {
  private: 
     int max_count;
     int current_count;
     AINT_THREAD_ITEM aint_threads;
      // If 't' exists in current list then return it's "index" [unique position marker]
      // return -1 if 't' is not found
     int get_position_for_thread(AINT_THREAD t) {
        int i;
        AINT_THREAD pt;
        for(i = 0; i < current_count; i++){
            pt = aint_threads[i].get_aint_thread();
            if (pt == t) return i;
        }
        return -1;
     }
  public: 
      // Constructor
     aint_thread_list_class(int mc) {
        int i;
        max_count = mc;
        current_count = 0;
        aint_threads = (AINT_THREAD_ITEM) malloc(mc*sizeof(AINT_THREAD_ITEM_CLASS));
        // Initialize newly created items
        for(i = 0; i < max_count; i++){
            aint_threads[i].set_aint_thread(NULL);
            aint_threads[i].set_state(POSIX_THREAD_UNKNOWN);
        }
     }  
      // Destructor
     ~aint_thread_list_class() {
        if(max_count > 0)free(aint_threads);
     }
      // Flush
     void flush_list() {
        if(max_count > 0)free(aint_threads);
        max_count = 0;
        current_count = 0;
     }
     void add_aint_thread(AINT_THREAD t) {
        if(current_count == max_count){
            int i;
            aint_threads = (AINT_THREAD_ITEM) realloc((void *)aint_threads, 2*max_count*sizeof(AINT_THREAD_ITEM_CLASS));
            // Initialize newly created items don't touch existing items
            for(i = max_count; i < 2*max_count; i++){
               aint_threads[i].set_aint_thread(NULL);
            }
            max_count = 2*max_count;
        }
        aint_threads[current_count].set_aint_thread(t);
        current_count++;
     }
     void delete_aint_thread(AINT_THREAD t) {
        int pos;
        pos = get_position_for_thread(t);
        if(pos == -1) return;
        ASSERTX(pos < current_count);
        aint_threads[pos].set_aint_thread(NULL);
     }
     AINT_THREAD  pick_and_delete_next_aint_thread() {
        int i;
        AINT_THREAD t;
        for(i = 0; i < current_count; i++){
           t = aint_threads[i].get_aint_thread();
           if(t != NULL) break;
        }
        if (i == current_count) return NULL;
        ASSERTX(i < current_count);
        aint_threads[i].set_aint_thread(NULL);
        return t;
     }
     AINT_THREAD_ITEM get_aint_thread_item(AINT_THREAD t) {
        int pos;

        pos = get_position_for_thread(t);
        if(pos == -1) return NULL;
        ASSERTX(pos < current_count);
        return &aint_threads[pos];
     }
} *AINT_THREAD_LIST, AINT_THREAD_LIST_CLASS;

typedef class posix_mutex_resource_class {
  private: 
        void * key;
        POSIX_MUTEX_STATE state;
        AINT_THREAD owner_thread;
        AINT_THREAD_LIST waiting_threads;
  public: 
       AINT_THREAD get_owner_thread() { return owner_thread; }
       void set_owner_thread(AINT_THREAD owner) { owner_thread = owner; }
       void * get_key() { return key; }
       void set_key(void *k) { key = k; }
       void set_state(POSIX_MUTEX_STATE s) { state = s; }
       POSIX_MUTEX_STATE get_state() { return state; }
       AINT_THREAD_LIST get_waiting_threads() { return waiting_threads; }
       void set_waiting_threads(AINT_THREAD_LIST w) { waiting_threads = w; }
       AINT_THREAD get_next_waiting_thread() { 
           if(waiting_threads)
               return waiting_threads->pick_and_delete_next_aint_thread(); 
           else
               return NULL;
       }
       void add_waiting_thread(AINT_THREAD w) { 
           if(waiting_threads == NULL) {
              waiting_threads = new AINT_THREAD_LIST_CLASS(_INIT_THREAD_COUNT_);
           }
           waiting_threads->add_aint_thread(w); 
       }
} *POSIX_MUTEX_RESOURCE, POSIX_MUTEX_RESOURCE_CLASS;

typedef class posix_mutex_list_class {
  private: 
     int max_count;
     int current_count;
     POSIX_MUTEX_RESOURCE mutexes;
      // If mutex with key 'k' exists in current list then return it 
      // return NULL if not found
     int get_position_for_mutex (void *k) {
        int i;
        void *tk;
        for(i = 0; i < current_count; i++){
            tk = mutexes[i].get_key();
            if (tk == k) return i;
        }
        return -1;
     }
  public: 
     posix_mutex_list_class (int mc) {
        int i;
        max_count = mc;
        current_count = 0;
        mutexes = (POSIX_MUTEX_RESOURCE) malloc(mc*sizeof(POSIX_MUTEX_RESOURCE_CLASS));
        // Initialize newly created items
        for(i = 0; i < max_count; i++){
            mutexes[i].set_key(NULL);
            mutexes[i].set_owner_thread(NULL);
            mutexes[i].set_waiting_threads(NULL);
            mutexes[i].set_state(POSIX_MUTEX_UNINITIALIZED);
        }
     }  
     void add_mutex(void *k) {
        if(current_count == max_count){
            int i;
            mutexes = (POSIX_MUTEX_RESOURCE) realloc((void *)mutexes, 2*max_count*sizeof(POSIX_MUTEX_RESOURCE_CLASS));
            // Initialize newly created items don't touch existing items
            for(i = max_count; i < 2*max_count; i++){
               mutexes[i].set_key(NULL);
               mutexes[i].set_owner_thread(NULL);
               mutexes[i].set_waiting_threads(NULL);
               mutexes[i].set_state(POSIX_MUTEX_UNINITIALIZED);
            }
            max_count = 2*max_count;
        }
        mutexes[current_count].set_key(k);
        mutexes[current_count].set_owner_thread(NULL);
        mutexes[current_count].set_waiting_threads(NULL);
        mutexes[current_count].set_state(POSIX_MUTEX_UNINITIALIZED);
        current_count++;
     }
     void delete_mutex(void *k) {
        int pos;

        pos = get_position_for_mutex(k);
        if(pos == -1) return;
        ASSERTX(pos < current_count);
        //We shouldn't be deleting a mutex with an owner
        ASSERTX(mutexes[pos].get_owner_thread() == NULL);
        ASSERTX(mutexes[pos].get_waiting_threads() == NULL);
        mutexes[pos].set_key(NULL);
     }
     POSIX_MUTEX_RESOURCE get_posix_mutex_resource(void * key) {
        int pos;

        pos = get_position_for_mutex(key);
        if(pos == -1) return NULL;
        ASSERTX(pos < current_count);
        return &mutexes[pos];
     }
} *POSIX_MUTEX_LIST, POSIX_MUTEX_LIST_CLASS;

typedef class posix_condition_resource_class {
  private: 
        void * key;
        void * mutex_key;
        POSIX_CONDITION_STATE state;
        AINT_THREAD_LIST waiting_threads;
  public: 
       void * get_key() { return key; }
       void set_key(void *k) { key = k; }
       void * get_mutex_key() { return mutex_key; }
       void set_mutex_key(void *k) { mutex_key = k; }
       void set_state(POSIX_CONDITION_STATE s) { state = s; }
       POSIX_CONDITION_STATE get_state() { return state; }
       AINT_THREAD_LIST get_waiting_threads() { return waiting_threads; }
       void set_waiting_threads(AINT_THREAD_LIST w) { waiting_threads = w; }
       AINT_THREAD get_next_waiting_thread() { 
           if(waiting_threads)
               return waiting_threads->pick_and_delete_next_aint_thread(); 
           else
               return NULL;
       }
       void add_waiting_thread(AINT_THREAD w) { 
           if(waiting_threads == NULL) {
              waiting_threads = new AINT_THREAD_LIST_CLASS(_INIT_THREAD_COUNT_);
           }
           waiting_threads->add_aint_thread(w); 
       }
} *POSIX_CONDITION_RESOURCE, POSIX_CONDITION_RESOURCE_CLASS;


typedef class posix_condition_list_class {
  private: 
     int max_count;
     int current_count;
     POSIX_CONDITION_RESOURCE conditions;
      // If condition with key 'k' exists in current list then return it 
      // return NULL if not found
     int get_position_for_condition (void *k) {
        int i;
        void *tk;
        for(i = 0; i < current_count; i++){
            tk = conditions[i].get_key();
            if (tk == k) return i;
        }
        return -1;
     }
  public: 
     posix_condition_list_class (int mc) {
        int i;
        max_count = mc;
        current_count = 0;
        conditions = (POSIX_CONDITION_RESOURCE) malloc(mc*sizeof(POSIX_CONDITION_RESOURCE_CLASS));
        // Initialize newly created items
        for(i = 0; i < max_count; i++){
            conditions[i].set_key(NULL);
            conditions[i].set_waiting_threads(NULL);
            conditions[i].set_state(POSIX_CONDITION_UNINITIALIZED);
        }
     }  
     void add_condition(void *k) {
        if(current_count == max_count){
            int i;
            conditions = (POSIX_CONDITION_RESOURCE) realloc((void *)conditions, 2*max_count*sizeof(POSIX_CONDITION_RESOURCE_CLASS));
            // Initialize newly created items don't touch existing items
            for(i = max_count; i < 2*max_count; i++){
               conditions[i].set_key(NULL);
               conditions[i].set_waiting_threads(NULL);
               conditions[i].set_state(POSIX_CONDITION_UNINITIALIZED);
            }
            max_count = 2*max_count;
        }
        conditions[current_count].set_key(k);
        conditions[current_count].set_waiting_threads(NULL);
        conditions[current_count].set_state(POSIX_CONDITION_UNINITIALIZED);
        current_count++;
     }
     void delete_condition(void *k) {
        int pos;

        pos = get_position_for_condition(k);
        if(pos == -1) return;
        ASSERTX(pos < current_count);
        //We shouldn't be deleting a condition with an owner
        ASSERTX(conditions[pos].get_waiting_threads() == NULL);
        conditions[pos].set_key(NULL);
     }
     POSIX_CONDITION_RESOURCE get_posix_condition_resource(void * key) {
        int pos;

        pos = get_position_for_condition(key);
        if(pos == -1) return NULL;
        ASSERTX(pos < current_count);
        return &conditions[pos];
     }
} *POSIX_CONDITION_LIST, POSIX_CONDITION_LIST_CLASS;

static POSIX_CONDITION_LIST posix_condition_list = NULL;
static POSIX_MUTEX_LIST posix_mutex_list = NULL;
static AINT_THREAD_LIST aint_thread_list = NULL;

static void posix_mutex_list_init(){
       ASSERTX(posix_mutex_list == NULL); 
       posix_mutex_list = new POSIX_MUTEX_LIST_CLASS(_INIT_MUTEX_COUNT_);
}

static void posix_condition_list_init(){
       ASSERTX(posix_condition_list == NULL); 
       posix_condition_list = new POSIX_CONDITION_LIST_CLASS(_INIT_CONDITION_COUNT_);
}

static void aint_thread_list_init(){
       ASSERTX(aint_thread_list == NULL); 
       aint_thread_list = new AINT_THREAD_LIST_CLASS(_INIT_THREAD_COUNT_);
}

extern "C" void posix_thread_init(){
#ifdef PTHREAD_DEBUG
   fprintf(stderr, "posix_thread_init() called \n");
#endif
   posix_mutex_list_init();
   posix_condition_list_init();
   aint_thread_list_init();
}

extern void posix_register_aint_thread(AINT_THREAD t){
   AINT_THREAD_ITEM item;
#ifdef PTHREAD_DEBUG
   fprintf(stderr, "Registering aint thread: tid = %d \n", t->tid);
#endif
   aint_thread_list->add_aint_thread(t);
   item  = aint_thread_list->get_aint_thread_item(t);
   item->set_state(POSIX_THREAD_RUNNING);
}

extern "C" void posix_register_mutex(void *key){
   POSIX_MUTEX_RESOURCE mutex_resource;
#ifdef PTHREAD_DEBUG
   fprintf(stderr, "Registering posix mutex: key = 0x%lx \n", key);
#endif
   posix_mutex_list->add_mutex(key);
   mutex_resource  = posix_mutex_list->get_posix_mutex_resource(key);
   if(mutex_resource->get_state() != POSIX_MUTEX_UNINITIALIZED){
       fprintf(stderr, "WARNING: posix mutex: key = 0x%p already registered! \n", key);
   }
   mutex_resource->set_state(POSIX_MUTEX_UNLOCKED);
}

extern "C" void posix_register_condition(void *key){
   POSIX_CONDITION_RESOURCE condition_resource;
#ifdef PTHREAD_DEBUG
   fprintf(stderr, "Registering posix condition: key = 0x%lx \n", key);
#endif
   posix_condition_list->add_condition(key);
   condition_resource  = posix_condition_list->get_posix_condition_resource(key);
   if(condition_resource->get_state() != POSIX_CONDITION_UNINITIALIZED){
       fprintf(stderr, "WARNING: posix condition: key = 0x%p already registered! \n", key);
   }
   condition_resource->set_state(POSIX_CONDITION_UNSIGNALLED);
}

extern "C" int is_posix_mutex_locked(void *key){
   POSIX_MUTEX_RESOURCE mutex_resource;

#ifdef PTHREAD_DEBUG
   fprintf(stderr, "Looking up posix mutex: key = 0x%lx \n", key);
#endif
   mutex_resource  = posix_mutex_list->get_posix_mutex_resource(key);
   if(mutex_resource->get_state() == POSIX_MUTEX_LOCKED){
        return true;
   }
   return false;
}

extern "C" void posix_mutex_lock(void *key, AINT_THREAD owner){
   POSIX_MUTEX_RESOURCE mutex_resource;
#ifdef PTHREAD_DEBUG
   fprintf(stderr, "Locking posix mutex: key = 0x%lx thread 0x%lx \n", key, owner->tid);
#endif
   mutex_resource  = posix_mutex_list->get_posix_mutex_resource(key);
   if(mutex_resource->get_state() != POSIX_MUTEX_UNLOCKED){
       fprintf(stderr, "posix mutex: key = 0x%p already locked! \n", key);
       ASSERTX(0);
   }
   mutex_resource->set_state(POSIX_MUTEX_LOCKED);
   mutex_resource->set_owner_thread(owner);
}

extern "C" void posix_condition_add_waiting_thread(void *cond_key, void *mutex_key, 
AINT_THREAD waiter){
   POSIX_CONDITION_RESOURCE condition_resource;

#ifdef PTHREAD_DEBUG
   fprintf(stderr, "Waiting on  posix condition: cond_key = 0x%lx mutex_key = 0x%lx thread 0x%lx \n", cond_key, mutex_key, waiter->tid);
#endif
   condition_resource  = posix_condition_list->get_posix_condition_resource(cond_key);
   condition_resource->set_mutex_key(mutex_key);
   condition_resource->add_waiting_thread(waiter);
   condition_resource->set_state(POSIX_CONDITION_UNSIGNALLED);
}

extern "C" void * posix_condition_get_mutex_key(void *cond_key)
{
   POSIX_CONDITION_RESOURCE condition_resource;
   void * retval;
#ifdef PTHREAD_DEBUG
   fprintf(stderr, "Waiting on  posix condition: cond_key = 0x%lx mutex_key = 0x%lx thread 0x%lx \n", cond_key, mutex_key, waiter->tid);
#endif
   condition_resource  = posix_condition_list->get_posix_condition_resource(cond_key);
   retval = condition_resource->get_mutex_key();
   return retval;
}

extern "C" void posix_mutex_unlock(void *key, AINT_THREAD unlocker){
   POSIX_MUTEX_RESOURCE mutex_resource;
   AINT_THREAD owner;
#ifdef PTHREAD_DEBUG
   fprintf(stderr, "Unlocking posix mutex: key = 0x%lx thread 0x%lx \n", key, unlocker->tid);
#endif
   mutex_resource  = posix_mutex_list->get_posix_mutex_resource(key);
   if(mutex_resource->get_state() != POSIX_MUTEX_LOCKED){
       fprintf(stderr, "posix mutex: key = 0x%p is not locked! \n", key);
       ASSERTX(0);
   }
   owner = mutex_resource->get_owner_thread();
   if(owner->tid != unlocker->tid){
       fprintf(stderr, "Posix mutex: key = 0x%p is locked by thread 0x"FMT32X" not thread 0x"FMT32X"! \n", key, owner->tid, unlocker->tid);
       ASSERTX(0);
   }
   mutex_resource->set_state(POSIX_MUTEX_UNLOCKED);
}


extern "C" void posix_mutex_add_waiting_thread(void *key, AINT_THREAD waiter){
   POSIX_MUTEX_RESOURCE mutex_resource;
#ifdef PTHREAD_DEBUG
   fprintf(stderr, "Making thread wait on posix mutex: key = 0x%lx thread 0x%lx \n", key, waiter->tid);
#endif
   mutex_resource  = posix_mutex_list->get_posix_mutex_resource(key);
   if(mutex_resource->get_state() != POSIX_MUTEX_LOCKED){
       fprintf(stderr, "posix mutex: key = 0x%p is not locked yet! \n", key);
       ASSERTX(0);
   }
   mutex_resource->add_waiting_thread(waiter);
}

extern "C" AINT_THREAD posix_mutex_get_next_waiting_thread(void *key){
   AINT_THREAD waiter;
   POSIX_MUTEX_RESOURCE mutex_resource;
#ifdef PTHREAD_DEBUG
   fprintf(stderr, "Getting next thread waiting on posix mutex: key = 0x%lx \n", key);
#endif
   mutex_resource  = posix_mutex_list->get_posix_mutex_resource(key);
   waiter = mutex_resource->get_next_waiting_thread();
   return waiter;
}

extern "C" AINT_THREAD posix_condition_get_next_waiting_thread(void *cond_key){
   AINT_THREAD waiter;
   POSIX_CONDITION_RESOURCE condition_resource;
#ifdef PTHREAD_DEBUG
   fprintf(stderr, "Getting next thread waiting on posix condition: key = 0x%lx \n", cond_key);
#endif
   condition_resource  = posix_condition_list->get_posix_condition_resource(cond_key);
   waiter = condition_resource->get_next_waiting_thread();
   return waiter;
}

extern "C" void posix_condition_signalled(void *cond_key)
{
   POSIX_CONDITION_RESOURCE condition_resource;
#ifdef PTHREAD_DEBUG
   fprintf(stderr, "Marking posix condition: key = 0x%lx  as signalled\n", cond_key);
#endif
   condition_resource  = posix_condition_list->get_posix_condition_resource(cond_key);
   condition_resource->set_state(POSIX_CONDITION_SIGNALLED);
}

extern "C" void posix_condition_broadcasted(void *cond_key)
{
   POSIX_CONDITION_RESOURCE condition_resource;
#ifdef PTHREAD_DEBUG
   fprintf(stderr, "Marking posix condition: key = 0x%lx  as broadcasted\n", cond_key);
#endif
   condition_resource  = posix_condition_list->get_posix_condition_resource(cond_key);
   condition_resource->set_state(POSIX_CONDITION_BROADCASTED);
}

