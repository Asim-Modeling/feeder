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

#ifndef _sim_h_
#define _sim_h_

#include "task.h"

		/* Prototype include a typedef name.
		   It should be moved after the typedef declaration */
extern void sim_checkpoint( );
		/* Prototype include a typedef name.
		   It should be moved after the typedef declaration */
extern void sim_done(double  , double );
extern void sim_deadlock();
extern int sim_exit( );
		/* Prototype include a typedef name.
		   It should be moved after the typedef declaration */
extern void sim_fork(task_ptr );
		/* Prototype include a typedef name.
		   It should be moved after the typedef declaration */
extern int sim_init(int  , char * []);
extern int sim_load_locked(task_ptr );
		/* Prototype include a typedef name.
		   It should be moved after the typedef declaration */
extern int sim_memory_barrier(task_ptr );
		/* Prototype include a typedef name.
		   It should be moved after the typedef declaration */
extern int sim_read(task_ptr );
		/* Prototype include a typedef name.
		   It should be moved after the typedef declaration */
extern int sim_store_conditional(task_ptr );
		/* Prototype include a typedef name.
		   It should be moved after the typedef declaration */
extern int sim_terminate(task_ptr );
extern void sim_unblock(task_ptr );
extern int sim_write(task_ptr );
extern int sim_user(task_ptr );
extern int sim_inst(task_ptr );
extern void sim_usage(int, char * []);

extern void sim_pthread_create(task_ptr);

extern int sim_null(task_ptr);    /* Simply returns T_ADVANCE */
#endif
