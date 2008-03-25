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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "icode.h"
#include "globals.h"
#include "task.h"
#include "wheel.h"
#include "symtab.h"

/* This points to the current position withing the tim ewheel */
wheel_task_ptr Curptr;

/* The time wheel is an array of head-nodes to doubly linked lists.
 * The array is indexed by the time value of a task. If the time
 * value is too large, then a task is added to a Future queue
 * and processed when the current index into the time wheel wraps around.
 * Curptr is a pointer to the head node for the current time wheel entry.
 * When all the tasks for the current time have been executed, Curptr is 
 * incremented to the next time wheel entry.
 */

aint_time_t Start_time;
aint_time_t End_time;

/* used in wheel_ptrin() to lookup function names */
extern hash_tab_ptr Func_hash_tab;
extern int Num_tasks;

/* if more than this number of tasks is allocated, it is probably a mistake */
#define TOO_MANY_TASKS 10000

wheel_task_ptr Wheel;
task_ptr Future_q;
wheel_task_ptr Curptr;
wheel_task_ptr Endptr;


