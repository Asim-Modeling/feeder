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

#ifndef _subst_h_
#define _subst_h_

void subst_init();
extern OP(callsys_f);
extern OP(aint_ioctl);
extern OP(aint_close);
extern OP(aint_write);
extern OP(aint_read);
extern OP(aint_lseek);
extern OP(aint_exit);
extern OP(aint_fork);
extern OP(aint_open);
extern OP(aint_stat);
extern OP(aint_fstat);
extern OP(aint_brk);
extern OP(aint_getpagesize);
extern OP(aint_gettimeofday);
extern OP(aint_wait4);
extern OP(aint_getdtablesize);
extern OP(aint_getrusage);
extern OP(aint_getpid);

extern void subst_init(void);
extern const char *syscall_name(int);
#endif
