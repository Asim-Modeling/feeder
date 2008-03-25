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
 * simcontrol-aint.c:
 *   Implementation of AINT-syscalls that control simulation and skipping
 */

#include "aint_pthread.h"
#include <stdio.h>

#define CALLSYS 131
/* for call_pal */

int request_begin_skipping(long how_many_to_skip)
{
    long callno = SYS_aint_request_begin_skipping;
    long ret_val;
    /* Copy the arguments into real registers */

#ifdef DEBUG
    printf("AINT request_begin_skipping: making syscall %d\n", callno);
    fflush(stdout);
#endif

    asm(  "\n"
	  "bis %1, $31, $0   \n"
	  "bis %2, $31, $16  \n"
	  "call_pal      131 \n"
	  "bis $19, $31, %0   \n"
	  : "=r" (ret_val)
	  : "r" (callno), "r" (how_many_to_skip)
	  : "$16", "$17", "$18", "$19", "$0"
	  );

    return ret_val;

}


int request_end_skipping()
{
    long callno = SYS_aint_request_end_skipping;
    long ret_val;
    /* Copy the arguments into real registers */

#ifdef DEBUG
    printf("AINT request_end_skipping: making syscall %d\n", callno);
    fflush(stdout);
#endif

    asm(  "\n"
	  "bis %1, $31, $0   \n"
	  "call_pal      131 \n"
	  "bis $19, $31, %0   \n"
	  : "=r" (ret_val)
	  : "r" (callno)
	  : "$16", "$17", "$18", "$19", "$0"
	  );

    return ret_val;

}


int request_end_simulation()
{
    long callno = SYS_aint_request_end_simulation;
    long ret_val;
    /* Copy the arguments into real registers */

#ifdef DEBUG
    printf("AINT request_end_simulation: making syscall %d\n", callno);
    fflush(stdout);
#endif

    asm(  "\n"
	  "bis %1, $31, $0   \n"
	  "call_pal      131 \n"
	  "bis $19, $31, %0   \n"
	  : "=r" (ret_val)
	  : "r" (callno)
	  : "$16", "$17", "$18", "$19", "$0"
	  );

    return ret_val;

}


int record_event(long event, long value)
{
    long callno = SYS_aint_record_event;
    long ret_val;
    /* Copy the arguments into real registers */

#ifdef DEBUG
    printf("AINT record_event: making syscall %d\n", callno);
    fflush(stdout);
#endif

    asm(  "\n"
	  "bis %1, $31, $0   \n"
	  "bis %2, $31, $16  \n"
	  "bis %3, $31, $17  \n"
	  "call_pal      131 \n"
	  "bis $19, $31, %0   \n"
	  : "=r" (ret_val)
	  : "r" (callno), "r" (event), "r" (value)
	  : "$16", "$17", "$18", "$19", "$0"
	  );

    return ret_val;

}
