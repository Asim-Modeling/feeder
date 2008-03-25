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

#ifndef AINT_SYSCALL_H
#define AINT_SYSCALL_H

/*
 * DigitalUnix system call return register usage:
 *   ERROR_REG ($r19) contains flag 0/1 for success/error
 *   RET_VAL_REG ($r0) contains correct/errno result
 */

  /* indicate error system call response */
#define SyscallError 1
  /* indicate success system call response */
#define SyscallSuccess 0

/* setup return registers as successful syscall */
#define SyscallSetSuccess(code) \
  MapReg(ERROR_REG) = SyscallSuccess; \
  MapReg(RET_VAL_REG) = (code);

/* setup return registers as failed syscall */
#define SyscallSetError(code) \
  MapReg(ERROR_REG) = SyscallError; \
  MapReg(RET_VAL_REG) = (code);

/* check for syscall success */
#define IsSyscallSuccess() (MapReg(ERROR_REG) == SyscallSuccess)

#define SyscallSetRegs(x) SyscallSetReturnRegs(pthread,(x))

/*
 * setup return registers from syscall;
 * return error for response == -1
 */
#define SyscallSetReturnRegs(pthread,response) \
{ \
  int resp_eval = response; \
  if (resp_eval == -1) { \
    SyscallSetError( errno ); \
  } else { \
    SyscallSetSuccess( resp_eval ); \
  } \
}

/* Aint "syscall" number definitions. */
#define AINT_PTHREAD_SYSCALL_BASE  0x42000000
#define SYS_aint_pthread_create (AINT_PTHREAD_SYSCALL_BASE + 1)
#define SYS_aint_pthread_join   (AINT_PTHREAD_SYSCALL_BASE + 2)
#define SYS_aint_pthread_exit   (AINT_PTHREAD_SYSCALL_BASE + 3)
#define SYS_aint_pthread_self   (AINT_PTHREAD_SYSCALL_BASE + 4)
#define SYS_aint_pthread_mutex_init   (AINT_PTHREAD_SYSCALL_BASE + 5)
#define SYS_aint_pthread_mutex_lock   (AINT_PTHREAD_SYSCALL_BASE + 6)
#define SYS_aint_pthread_mutex_unlock   (AINT_PTHREAD_SYSCALL_BASE + 7)
#define SYS_aint_pthread_condition_init   (AINT_PTHREAD_SYSCALL_BASE + 8)
#define SYS_aint_pthread_condition_wait   (AINT_PTHREAD_SYSCALL_BASE + 9)
#define SYS_aint_pthread_condition_broadcast   (AINT_PTHREAD_SYSCALL_BASE + 10)
#define SYS_aint_pthread_condition_signal   (AINT_PTHREAD_SYSCALL_BASE + 11)


#define SYS_aint_request_begin_skipping   (AINT_PTHREAD_SYSCALL_BASE + 100)
#define SYS_aint_request_end_skipping	  (AINT_PTHREAD_SYSCALL_BASE + 101)
#define SYS_aint_request_end_simulation   (AINT_PTHREAD_SYSCALL_BASE + 102)
#define SYS_aint_record_event             (AINT_PTHREAD_SYSCALL_BASE + 103)

#define SYS_aint_quiesce_if_equal         (AINT_PTHREAD_SYSCALL_BASE + 200)
#define SYS_aint_unquiesce_assign         (AINT_PTHREAD_SYSCALL_BASE + 201)


#endif
