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

#include "aint.h"
#include "sim.h"
#include "pmint.h"
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <errno.h>

#include <aint_syscall.h>

#include <string.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <alpha/hal_sysinfo.h>
#include <alpha/fpu.h>
#include <sys/ioctl.h>
#include <sys/uswitch.h>
#include <sys/time.h>
#include <sys/mount.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/user.h>
struct rtentry; /* make compiler happy */
struct mbuf; /* make compiler happy */
#include <sys/table.h>

#include "icode.h"
#include "globals.h"
#include "opcodes.h"
#include "subst.h"
#include "protos.h"
#include "posix_thread.h"

#ifdef DEAD_CODE
static icode_t Iterminator1;
static icode_t I_pterminator1;
static icode_t Ifork;
static icode_t Ipthread_create;
#endif /* DEAD_CODE */

#define PTHREAD_DEBUG  /* Enables verbose messages on pthread_* calls */
/* #undef PTHREAD_DEBUG */ /* uncomment if no messages needed on pthread actions */

#ifndef __GNUC__
#  include <c_asm.h>
#endif

#define INSERT_F() {\
  struct icode pcopy; memcpy(&pcopy, picode, sizeof(struct icode)); \
  picode->next = pcopy; \
}

OP(aint_exit);
OP(aint_fork);
OP(aint_read);
OP(aint_write);
OP(aint_old_open);
OP(aint_close);
OP(aint_wait4);
OP(aint_old_creat);
OP(aint_link);
OP(aint_unlink);
OP(aint_execv);
OP(aint_chdir);
OP(aint_fchdir);
OP(aint_mknod);
OP(aint_chmod);
OP(aint_chown);
OP(aint_obreak);
OP(aint_getfsstat);
OP(aint_lseek);
OP(aint_getpid);
OP(aint_mount);
OP(aint_unmount);
OP(aint_setuid);
OP(aint_getuid);
OP(aint_exec_with_loader);
OP(aint_ptrace);
#ifdef	COMPAT_43
#error ??
OP(aint_nrecvmsg);
OP(aint_nsendmsg);
OP(aint_nrecvfrom);
OP(aint_naccept);
OP(aint_ngetpeername);
OP(aint_ngetsockname);
#else
OP(aint_recvmsg);
OP(aint_sendmsg);
OP(aint_recvfrom);
OP(aint_accept);
OP(aint_getpeername);
OP(aint_getsockname);
#endif
OP(aint_access);
OP(aint_chflags);
OP(aint_fchflags);
OP(aint_sync);
OP(aint_kill);
OP(aint_old_stat);
OP(aint_setpgid);
OP(aint_old_lstat);
OP(aint_dup);
OP(aint_pipe);
OP(aint_set_program_attributes);
OP(aint_profil);
OP(aint_open);
				/* 46 is obsolete osigaction */
OP(aint_getgid);
OP(aint_sigprocmask);
OP(aint_getlogin);
OP(aint_setlogin);
OP(aint_acct);
OP(aint_sigpending);
OP(aint_ioctl);
OP(aint_reboot);
OP(aint_revoke);
OP(aint_symlink);
OP(aint_readlink);
OP(aint_execve);
OP(aint_umask);
OP(aint_chroot);
OP(aint_old_fstat);
OP(aint_getpgrp);
OP(aint_getpagesize);
OP(aint_mremap);
OP(aint_vfork);
OP(aint_stat);
OP(aint_lstat);
OP(aint_sbrk);
OP(aint_sstk);
OP(aint_mmap);
OP(aint_ovadvise);
OP(aint_munmap);
OP(aint_mprotect);
OP(aint_madvise);
OP(aint_old_vhangup);
OP(aint_kmodcall);
OP(aint_mincore);
OP(aint_getgroups);
OP(aint_setgroups);
OP(aint_old_getpgrp);
OP(aint_setpgrp);
OP(aint_setitimer);
OP(aint_old_wait);
OP(aint_table);
OP(aint_getitimer);
OP(aint_gethostname);
OP(aint_sethostname);
OP(aint_getdtablesize);
OP(aint_dup2);
OP(aint_fstat);
OP(aint_fcntl);
OP(aint_select);
OP(aint_poll);
OP(aint_fsync);
OP(aint_setpriority);
OP(aint_socket);
OP(aint_connect);
#ifdef	COMPAT_43
OP(aint_accept);
#else
OP(aint_old_accept);
#endif
OP(aint_getpriority);
#ifdef	COMPAT_43
OP(aint_send);
OP(aint_recv);
#else
OP(aint_old_send);
OP(aint_old_recv);
#endif
OP(aint_sigreturn);
OP(aint_bind);
OP(aint_setsockopt);
OP(aint_listen);
OP(aint_plock);
OP(aint_old_sigvec);
OP(aint_old_sigblock);
OP(aint_old_sigsetmask);
OP(aint_sigsuspend);
OP(aint_sigstack);
#ifdef	COMPAT_43
OP(aint_recvmsg);
OP(aint_sendmsg);
#else
OP(aint_old_recvmsg);
OP(aint_old_sendmsg);
#endif
				/* 115 is obsolete vtrace */
OP(aint_gettimeofday);
OP(aint_getrusage);
OP(aint_getsockopt);
OP(aint_readv);
OP(aint_writev);
OP(aint_settimeofday);
OP(aint_fchown);
OP(aint_fchmod);
#ifdef	COMPAT_43
OP(aint_recvfrom);
#else
OP(aint_old_recvfrom);
#endif
OP(aint_setreuid);
OP(aint_setregid);
OP(aint_rename);
OP(aint_truncate);
OP(aint_ftruncate);
OP(aint_flock);
OP(aint_setgid);
OP(aint_sendto);
OP(aint_shutdown);
OP(aint_socketpair);
OP(aint_mkdir);
OP(aint_rmdir);
OP(aint_utimes);
				/* 139 is obsolete 4.2 sigreturn */
OP(aint_adjtime);
#ifdef	COMPAT_43
OP(aint_getpeername);
#else
OP(aint_old_getpeername);
#endif
OP(aint_gethostid);
OP(aint_sethostid);
OP(aint_getrlimit);
OP(aint_setrlimit);
OP(aint_old_killpg);
OP(aint_setsid);
OP(aint_quotactl);
OP(aint_oldquota);
#ifdef	COMPAT_43
OP(aint_getsockname);
#else
OP(aint_old_getsockname);
#endif
OP(aint_pid_block);
OP(aint_pid_unblock);
OP(aint_sigaction);
OP(aint_sigwaitprim);
OP(aint_nfssvc);
OP(aint_getdirentries);
OP(aint_statfs);
OP(aint_fstatfs);
OP(aint_async_daemon);
OP(aint_getfh);
OP(aint_getdomainname);
OP(aint_setdomainname);
OP(aint_exportfs);
OP(aint_alt_plock);
OP(aint_getmnt);
OP(aint_alt_sigpending);
OP(aint_alt_setsid);
OP(aint_swapon);
OP(aint_msgctl);
OP(aint_msgget);
OP(aint_msgrcv);
OP(aint_msgsnd);
OP(aint_semctl);
OP(aint_semget);
OP(aint_semop);
OP(aint_uname);
OP(aint_lchown);
OP(aint_shmat);
OP(aint_shmctl);
OP(aint_shmdt);
OP(aint_shmget);
OP(aint_mvalid);
OP(aint_getaddressconf);
OP(aint_msleep);
OP(aint_mwakeup);
OP(aint_msync);
OP(aint_signal);
OP(aint_utc_gettime);
OP(aint_utc_adjtime);
OP(aint_security);
OP(aint_kloadcall);
OP(aint_getpgid);
OP(aint_getsid);
OP(aint_sigaltstack);
OP(aint_waitid);
OP(aint_priocntlset);
OP(aint_sigsendset);
OP(aint_set_speculative);
OP(aint_msfs_syscall);
OP(aint_sysinfo);
OP(aint_uadmin);
OP(aint_fuser);
OP(aint_proplist_syscall);
OP(aint_pathconf);
OP(aint_fpathconf);
OP(aint_uswitch);
OP(aint_usleep_thread);
OP(aint_audcntl);
OP(aint_audgen);
OP(aint_sysfs);
OP(aint_subsys_info);
OP(aint_getsysinfo);
OP(aint_setsysinfo);
OP(aint_afs_syscall);
OP(aint_swapctl);
OP(aint_memcntl);

OP(aint_pthread_create);
OP(aint_pthread_join);
OP(aint_pthread_exit);
OP(aint_pthread_self);
OP(aint_pthread_mutex_init);
OP(aint_pthread_mutex_lock);
OP(aint_pthread_mutex_unlock);
OP(aint_pthread_condition_init);
OP(aint_pthread_condition_wait);
OP(aint_pthread_condition_broadcast);
OP(aint_pthread_condition_signal);

OP(aint_request_begin_skipping);
OP(aint_request_end_skipping);
OP(aint_request_end_simulation);
OP(aint_record_event);

OP(aint_quiesce_if_equal);
OP(aint_unquiesce_assign);


typedef struct textregion_s {
  ulong startaddr;
  ulong len;
} textregion_t;

textregion_t textregion[MAX_NPROCS][MAX_FDNUM];

/* This function initializes icodes used for implementing exit() */


void
subst_init()
{
#ifdef DEAD_CODE
    Iterminator1.func = terminator1;
    Iterminator1.cycles = 0;
    Iterminator1.opnum = 0;
    Iterminator1.next = NULL;
    Iterminator1.iflags = E_EXIT;

    I_pterminator1.func = pterminator1;
    I_pterminator1.cycles = 0;
    I_pterminator1.opnum = 0;
    I_pterminator1.next = NULL;
    I_pterminator1.iflags = E_EXIT;

    Ifork.func = event_fork;
    Ifork.cycles = 0;
    Ifork.opnum = 0;
    Ifork.next = 0;
    Ifork.iflags = E_FORK;

    Ipthread_create.func = event_pthread_create;
    Ipthread_create.cycles = 0;
    Ipthread_create.opnum = 0;
    Ipthread_create.next = NULL;
    Ifork.iflags = E_FORK;
#endif
    posix_thread_init();
}



/* The substitute functions are called from within the callsys_f function.
 */

/* This needs to be modified to call substitute functions through a pointer
 * rather than a switch statement 
 */
OP(callsys_f)
{
  icode_ptr ret_addr;
  int wasVerbose = Verbose;

  Physical_RegNum RegMapSnapshot[TOTAL_LOGICALS];
  Physical_RegNum RegMapBackup[TOTAL_LOGICALS];
  
#ifdef VERBOSE_AINT_CALLSYS
  Verbose = 1;
#endif

  /* 
   * we need the register map corresponding to this instid 
   */
  memcpy(RegMapBackup, pthread->RegMap, sizeof(RegMapBackup));
  /* compute the snapshot */
  AINT_regmap_snapshot(RegMapSnapshot, pthread, IFI_INSTID(ifi, pthread));
  /* temporarily install the snapshot */
  memcpy(pthread->RegMap, RegMapSnapshot, sizeof(RegMapBackup));

  if (MapReg(0) == 0) {
    warning("[p%d.t%d] CALLSYS: %ld:%s [%lx %lx]\n",
	      pthread->process->pid,
	      pthread->tid,
	      MapReg(0),
	      syscall_name(MapReg(0)),
	      MapReg(A0),
	      MapReg(A1));
    /* find out the real syscall number */
    MapReg(0) = MapReg(A0);
    /* shift the arguments down one */
    MapReg(A0) = MapReg(A1);
    MapReg(A1) = MapReg(A2);
    MapReg(A2) = MapReg(A3);
    MapReg(A3) = MapReg(A4);
    MapReg(A4) = MapReg(A5);
  }

 informative("[p%d.t%d] CALLSYS: %ld:%s [%lx %lx %lx %lx] pc=%lx ra=%lx\n",
	      pthread->tid,
	      pthread->process->pid,
	      MapReg(0),
	      syscall_name(MapReg(0)),
	      MapReg(A0),
	      MapReg(A1),
	      MapReg(A2),
	      MapReg(A3),
	      picode->addr,
	      MapReg(RA_REG));

  if (0) fprintf(stderr, "[p%d.t%d] CALLSYS: %ld:%s [%lx %lx %lx %lx] pc=%lx ra=%lx\n",
	      pthread->tid,
	      pthread->process->pid,
	      MapReg(0),
	      syscall_name(MapReg(0)),
	      MapReg(A0),
	      MapReg(A1),
	      MapReg(A2),
	      MapReg(A3),
	      picode->addr,
	      MapReg(RA_REG));

  switch (MapReg(0)) {

#define SYSCASE(n_) case SYS_ ## n_: ret_addr = aint_ ## n_ (ifi, picode, pthread); break;
    
  SYSCASE(exit);
  SYSCASE(fork);
  SYSCASE(read);
  SYSCASE(write);
  SYSCASE(close);
  SYSCASE(wait4);
  SYSCASE(link);
  SYSCASE(unlink);
  SYSCASE(chdir);
  SYSCASE(fchdir);
  SYSCASE(mknod);
  SYSCASE(chown);

  case SYS_obreak:
    ret_addr = aint_brk(ifi, picode, pthread);
    break;

  SYSCASE(lseek);
  SYSCASE(getpid);
  SYSCASE(setuid);
  SYSCASE(getuid);
  SYSCASE(access);
  SYSCASE(kill);
  SYSCASE(dup);
  SYSCASE(set_program_attributes);

  SYSCASE(open);
  SYSCASE(getgid);
  SYSCASE(sigprocmask);
  SYSCASE(ioctl);
  
  SYSCASE(symlink);
  SYSCASE(readlink);
  SYSCASE(umask);
  SYSCASE(getpagesize);
  SYSCASE(stat);
  SYSCASE(lstat);

  SYSCASE(mmap);
  SYSCASE(munmap);

  SYSCASE(madvise);
  SYSCASE(table);
  SYSCASE(gethostname);

  SYSCASE(fstat);
  SYSCASE(fcntl);
  SYSCASE(rename);
  SYSCASE(truncate);
  SYSCASE(ftruncate);
  SYSCASE(flock);
  SYSCASE(sendto);
  SYSCASE(getdomainname);
  SYSCASE(socket);
  SYSCASE(setitimer);

  case SYS_gettimeofday:
    ret_addr = aint_gettimeofday(ifi, picode, pthread);
    break;

  SYSCASE(setrlimit);
  SYSCASE(getrlimit);
  SYSCASE(sigaction);
  SYSCASE(sigreturn);

  SYSCASE(getdirentries);
  SYSCASE(statfs);
  SYSCASE(fstatfs);

  case SYS_shmget:
    ret_addr = aint_shmget(ifi, picode, pthread);
    break;

  case SYS_shmat:
    ret_addr = aint_shmat(ifi, picode, pthread);
    break;
  
  case SYS_shmdt:
    ret_addr = aint_shmdt(ifi, picode, pthread);
    break;
  
  case SYS_shmctl:
    ret_addr = aint_shmctl(ifi, picode, pthread);
    break;

  case SYS_getdtablesize:
    ret_addr = aint_getdtablesize(ifi, picode, pthread);
    break;

  case SYS_getrusage:
    ret_addr = aint_getrusage(ifi, picode, pthread);
    break;

  SYSCASE(uswitch);
  SYSCASE(pathconf);
  SYSCASE(fpathconf);
  SYSCASE(usleep_thread);

  SYSCASE(set_speculative);

  SYSCASE(getsysinfo);
  SYSCASE(setsysinfo);

  case SYS_aint_pthread_create:
      ret_addr = aint_pthread_create(ifi, picode, pthread);
      break;

  case SYS_aint_pthread_join:
      ret_addr = aint_pthread_join(ifi, picode, pthread);
      break;

  case SYS_aint_pthread_exit:
      ret_addr = aint_pthread_exit(ifi, picode, pthread);
      break;

  case SYS_aint_pthread_self:
      ret_addr = aint_pthread_self(ifi, picode, pthread);
      break;

  case SYS_aint_pthread_mutex_init:
      ret_addr = aint_pthread_mutex_init(ifi, picode, pthread);
      break;

  case SYS_aint_pthread_mutex_lock:
      ret_addr = aint_pthread_mutex_lock(ifi, picode, pthread);
      break;

  case SYS_aint_pthread_mutex_unlock:
      ret_addr = aint_pthread_mutex_unlock(ifi, picode, pthread);
      break;

  case SYS_aint_pthread_condition_init:
      ret_addr = aint_pthread_condition_init(ifi, picode, pthread);
      break;

  case SYS_aint_pthread_condition_wait:
      ret_addr = aint_pthread_condition_wait(ifi, picode, pthread);
      break;

  case SYS_aint_pthread_condition_broadcast:
      ret_addr = aint_pthread_condition_broadcast(ifi, picode, pthread);
      break;

  case SYS_aint_pthread_condition_signal:
      ret_addr = aint_pthread_condition_signal(ifi, picode, pthread);
      break;

  case SYS_aint_request_begin_skipping:
      ret_addr = aint_request_begin_skipping(ifi, picode, pthread);
      break;
  case SYS_aint_request_end_skipping:
      ret_addr = aint_request_end_skipping(ifi, picode, pthread);
      break;
  case SYS_aint_request_end_simulation:
      ret_addr = aint_request_end_simulation(ifi, picode, pthread);
      break;

  case SYS_aint_record_event:
      ret_addr = aint_record_event(ifi, picode, pthread);
      break;

  case SYS_aint_quiesce_if_equal:
      ret_addr = aint_quiesce_if_equal(ifi, picode, pthread);
      break;
  case SYS_aint_unquiesce_assign:
      ret_addr = aint_unquiesce_assign(ifi, picode, pthread);
      break;


  default:
    fatal("Unimplemented syscall %d:%0x:%s at pc=%lx\n", 
	  MapReg(0), MapReg(0), syscall_name(MapReg(0)), picode->addr);
    ret_addr = picode->next;
    break;
  }

  informative("[p%d.t%d]          => %lx %lx\n",
	      pthread->tid,
	      pthread->process->pid,
	      MapReg(RET_VAL_REG),
	      MapReg(ERROR_REG));

  if (0) fprintf(stderr, "[p%d.t%d]          => %lx %lx\n",
	      pthread->tid,
	      pthread->process->pid,
	      MapReg(RET_VAL_REG),
	      MapReg(ERROR_REG));

  /* restore this threads register map */
  memcpy(pthread->RegMap, RegMapBackup, sizeof(RegMapBackup));

  Verbose = wasVerbose;
  return ret_addr;
}

/* this is *very* different from the original aint function.
 * since simulation is driven by pm, we don't need to go
 * through the scheduler
 */
OP(aint_exit)
{
  /* event_exit(pthread);  Calls task_exit calls sim_exit */
  pthread->exitcode = MapReg(A0);
  informative(
	  "Thread %d.%d exited with code %d\n", 
	  pthread->process->pid,
	  pthread->tid,
	  MapReg(A0));

  /* do everything pthread_exit() would do */
  return aint_pthread_exit(ifi, picode, pthread);
}

OP(aint_fork)
{
    int i;
    thread_ptr child;
    process_ptr childp;
    int pid, tid;
    event_ptr pevent;

    /* allocate a new thread */
    INLINE_DEQUEUE(&Free_q, child);
    INLINE_DEQUEUE(&Free_Process_q, childp);

    if (child == NULL)
        fatal("aint_fork: exceeded thread limit (%d)\n", Max_nthreads);

    if (childp == NULL)
        fatal("aint_fork: exceeded process limit (%d)\n", Max_nprocs);

    child->process = childp;
    childp->threads = child;
    childp->youngest_thread = child;

    /* if (childp->addr_space == 0) {
    if (1) { */

    /* Always allocate new event struct */
    NEW_ITEM(Event_free, sizeof(event_t), pevent, "aint_fork");

    /*} else
      pevent = child->pevent;
      */

    pid = childp->pid;
    if (Maxpid < pid)
        Maxpid = pid;

    tid = child->tid;
    if (Maxtid < tid)
	Maxtid = tid;

    Nprocs++;
    Nthreads++;

    if (Nprocs > Max_nprocs)
        fatal("aint_fork: exceeded process limit (%d)\n", Max_nprocs);

    if (Nthreads > Max_nthreads)
	fatal("aint_fork: exceeded thread limit (%d)\n", Max_nthreads);

    /* copy the parent's address space to the child */
    copy_addr_space(pthread, child);

    /* init_thread() must be called *after* copy_addr_space() since
     * init_thread() uses the mapping fields set up by copy_addr_space().
     */
    init_process(childp);
    init_thread(child, pevent);

    childp->thread_count = 1;

    child->tsibling = NULL;

    /* the child's start time is the parent's plus the time for the fork */
    child->time = pthread->time + picode_cycles(picode);
    if (First_fork_elapsed == 0.0) {
        First_fork_elapsed = child->time;
        First_fork_cpu = pthread->cpu_time + picode_cycles(picode);
    }
    childp->parent = pthread->process;

    /* link in the child to the parent's list of children */
    childp->sibling = pthread->process->youngest;
    pthread->process->youngest = childp;

    /* the first instruction for the child is to return 
    child->picode = T2I(pthread->reg[31]); */
    child->next_fetch_picode = picode->next;

    /* the return value for the parent is the child's pid */
    SyscallSetSuccess( pid );
    /* the return value for the child is 0 */
    MapReg_thr(child, RET_VAL_REG) = 0;
    MapReg_thr(child, ERROR_REG) = SyscallSuccess;

    /* Set child flags - r20 is 0 for parent and 1 for child */
    MapReg(CHILD_FLAG_REG) = 0;
    MapReg_thr(child, CHILD_FLAG_REG) = 1;

    /* copy the parent's signal handler vectors */
    for (i = 0; i < MAX_SIGNALS; i++) {
        childp->sigv[i].sv_handler = pthread->process->sigv[i].sv_handler;
        childp->sigv[i].sv_mask = pthread->process->sigv[i].sv_mask;
        childp->sigv[i].sv_flags = pthread->process->sigv[i].sv_flags;
    }
    child->sigblocked = pthread->sigblocked;


    pmint_thread_begin(child->tid);
    pmint_set_thread_type(child->tid, pmint_get_thread_type(pthread->tid));

    return picode->next;


    /* The following is not required; we are bypassing the scheduler */



    /* Return from the parent. If generating any events, then first
     * generate a fork event. The picode returned must be the one that
     * calls event_fork...
     */

}
OP(aint_read)
{

  process_ptr process = pthread->process;
  int pfd, vfd;
  long size = MapReg(A2);
  char *mybuf = (char *) calloc( 1, size+1);
  if (mybuf == NULL)
    fatal("aint_read failed to allocate a %d-byte buffer in which to perform a read\n", 
	  size);
	  
  vfd = MapReg(A0);
  pfd = pthread->process->fd[vfd];
  /* printf("aint_read: reading %d bytes from fd %d\n", MapReg(A2), MapReg(A0)); */

  SyscallSetRegs( read(pfd, mybuf, MapReg(A2)) );

  if (IsSyscallSuccess()) {
    memcpy_to_object_space(process, MapReg(A1), mybuf, MapReg(RET_VAL_REG));

    /***********
    if (Verbose) {
      if (MapReg(A2) > 70) {
        mybuf[70] = '\0';
      } else {
        mybuf[MapReg(A2)] = '\0';
      }
      fprintf(stderr, "%s\n", mybuf);
    }
    ************/
  }
  free((void *) mybuf);
  return picode->next;
} 

OP(aint_write)
{
  process_ptr process = pthread->process;
  int vfd, pfd;

  long size = MapReg(A2);
  char *mybuf = (char *) calloc( 1, size+1);
  if (mybuf == NULL)
    fatal("aint_write failed to allocate a %d-byte buffer in which to perform a read\n", 
	  size);

  vfd = MapReg(A0);
  pfd = pthread->process->fd[vfd];

  memcpy_from_object_space(process, mybuf, MapReg(A1), MapReg(A2));

  SyscallSetRegs( write(pfd, (caddr_t) mybuf, MapReg(A2)) );

  if (IsSyscallSuccess()) {
    /***********
    if (Verbose) {
      if (MapReg(A2) > 70) {
        mybuf[70] = '\0';
      } else {
        mybuf[MapReg(A2)] = '\0';
      }
      fprintf(stderr, "\n%s\n", mybuf);
    }
    ************/
  }
  free((void *) mybuf);
  return picode->next;
}

OP(aint_close)
{
  int vfd, dup_vfd, pfd;
  process_ptr process = pthread->process;

  vfd = MapReg(A0);
  pfd = process->fd[vfd];

  process->fd[vfd] = -1;

    /* printf("aint_close: closing %d\n", MapReg(A0)); */
  if (pfd < 3) {
    SyscallSetSuccess( 0 );
  } else {
    /* Search if there is another vfd dup'ed to the same pfd */
    for (dup_vfd = 0; dup_vfd < MAX_FDNUM; dup_vfd++) {
      if (process->fd[dup_vfd] == pfd) break;
    }
    if (dup_vfd == MAX_FDNUM) {
      /* no dup'ed synonyms - close host fd */
      SyscallSetRegs( close(pfd) );
    } else {
      SyscallSetSuccess( 0 );
    }
  }

  /* Crude hack - decode when /sbin/loader calls close */
  if (textregion[process->pid][vfd].startaddr != 0) {
    decode_text_region(process, 
		       textregion[process->pid][vfd].startaddr, 
		       textregion[process->pid][vfd].len);
    textregion[process->pid][vfd].startaddr = 0;
  }

  return picode->next;
}

OP(aint_wait4)
{
  process_ptr process = pthread->process;
  pid_t pid;
  int *status_location = NULL;
  int options;
  struct rusage *resource_usage = NULL;
  int childpid;
  long flags;

  pid = MapReg(A0);
  if (MapReg(A1))
    status_location = (int *) addr2phys(process, MapReg(A1), &flags);
  if ((flags & (PROT_READ|PROT_WRITE)) != (PROT_READ|PROT_WRITE)) {
    SyscallSetError( EINVAL );
    return picode->next;
  }
    
  options = MapReg(A2);
  if (MapReg(A3) != 0) {
    resource_usage = (struct rusage *) (long)addr2phys(process, MapReg(A3), &flags);
    if ((flags & (PROT_READ|PROT_WRITE)) != (PROT_READ|PROT_WRITE)) {
      SyscallSetError( EINVAL );
      return picode->next;
    }
  }

  if ((pid == -1) && (options == 0)) {
    /* This is the aint.pm wait4. There is no scheduler. 
     *  The blocking sequence has to be changed */
    childpid = wait_for_child(pthread);
    if (childpid == 0) {
      sleep_thr(pthread);
      ifi->runstate = R_WAIT;
      pthread->process->runstate = R_WAIT;
      return picode;
    }
    SyscallSetSuccess( childpid );
    if (childpid == -1) {
      SyscallSetError( ECHILD );
    } else if (status_location) {
	*status_location = 0;
    }
    return picode->next;
  } else {
    fatal("aint_wait4(): unimplemented option %d\n", options);
  }
  /* we never get here */
  fatal("Reached wrong path in aint_wait4!");
}

OP(aint_link)
{
  process_ptr process = pthread->process;
  char *path1 = strdup_from_object_space(process, MapReg(A0));
  char *path2 = strdup_from_object_space(process, MapReg(A1));

  SyscallSetRegs( link(path1, path2) );

  free(path1);
  free(path2);
  return picode->next;
}


OP(aint_unlink)
{
  process_ptr process = pthread->process;
  char *path1 = strdup_from_object_space(process, MapReg(A0));

  SyscallSetRegs( unlink(path1) );

  free(path1);
  return picode->next;
}

OP(aint_execv);
OP(aint_chdir)
{
  process_ptr process = pthread->process;
  char *path1 = strdup_from_object_space(process, MapReg(A0));

  SyscallSetRegs( chdir(path1) );

  free(path1);
  return picode->next;
}

OP(aint_fchdir)
{
  extern int fchdir(int);

  SyscallSetRegs( fchdir(MapReg(A0)) );

  return picode->next;
}

OP(aint_mknod)
{
  extern int mknod (const char *path, int mode, dev_t	device );

  process_ptr process = pthread->process;
  char *path1 = strdup_from_object_space(process, MapReg(A0));

  SyscallSetRegs( mknod(path1, MapReg(A1), MapReg(A2)) );

  free(path1);
  return picode->next;
}

OP(aint_chmod)
{
  process_ptr process = pthread->process;
  char *path1 = strdup_from_object_space(process, MapReg(A0));

  SyscallSetRegs( chmod(path1, MapReg(A1)) );

  free(path1);
  return picode->next;
}

OP(aint_chown)
{
  process_ptr process = pthread->process;
  char *path1 = strdup_from_object_space(process, MapReg(A0));

  SyscallSetRegs( chown(path1, MapReg(A1), MapReg(A2)) );

  free(path1);
  return picode->next;
}

/* sys_obreak */

OP(aint_brk)
{
  SyscallSetSuccess( 0 );
  /* use brk-1 so that we just extend the current data segment, 
   * rather than adding a new one. -Jamey 6/19/96
   */
  process_add_segment(pthread->process, 
		      pthread->process->brk-1,
		      MapReg(A0)-pthread->process->brk+1,
		      PROT_READ|PROT_WRITE,
		      MAP_PRIVATE);
  pthread->process->brk = MapReg(A0);
  return picode->next;
}

OP(aint_getfsstat);

OP(aint_lseek)
{

  int vfd, pfd;

  vfd = MapReg(A0);
  pfd = pthread->process->fd[vfd];

  if (pfd > 2) {
    SyscallSetRegs( lseek(pfd, MapReg(A1), MapReg(A2)) );
  } else {
    SyscallSetRegs( lseek(pfd, 0, SEEK_CUR) );
  }
  
  /* printf("aint_lseek: lseeking %d offset %d from %d\n", MapReg(A0), 
	 MapReg(A1), MapReg(A2)); */
  return picode->next;
}

OP(aint_getpid)
{
  SyscallSetSuccess( pthread->process->pid );
  return(picode->next);
}

OP(aint_mount);
OP(aint_unmount);

OP(aint_setuid)
{
  SyscallSetRegs( setuid(MapReg(A0)) );
  return(picode->next);
}

OP(aint_getuid)
{
  SyscallSetRegs( getuid() );
  /* the obscure world of the getuid syscall also returns euid in a
   * non-standard return value register - ahrg! */
  MapReg(A4) = geteuid();
  return(picode->next);
}

OP(aint_exec_with_loader);
OP(aint_ptrace);

OP(aint_recvmsg);
OP(aint_sendmsg);
OP(aint_recvfrom);
OP(aint_accept);
OP(aint_getpeername);
OP(aint_getsockname);

OP(aint_access)
{
  process_ptr process = pthread->process;
  ulong upath = MapReg(A0);
  int access_mode = MapReg(A1);
  int status;
  char *path = strdup_from_object_space(process, upath);

  informative("access(%s, %d)\n", path, access_mode);

  SyscallSetRegs( access(path, access_mode) );
    
  free(path);
  return(picode->next);
}

OP(aint_chflags);
OP(aint_fchflags);
OP(aint_sync);

OP(aint_kill)
{
  ulong pid = MapReg(A0);
  ulong signal = MapReg(A1);
  informative("aint_kill: pid=%ld signal=%ld\n", pid, signal);
  if (pid < (ulong)Max_nprocs) {
    process_ptr oprocess = &Processes[pid];
    thread_ptr opthread = oprocess->threads;
    inflight_inst_ptr oifi = &opthread->cbif[opthread->instid];
    ifi->runstate = R_SIGNAL;
    ifi->signal = signal;
    opthread->runstate = R_SIGNAL;
    opthread->signal = signal;
  }
  return(picode->next);
}

OP(aint_setpgid);

OP(aint_dup)
{
  process_ptr process = pthread->process;
  int vfd, new_vfd;

  vfd = MapReg(A0);

  /* Search for an unused fd */
  for (new_vfd = 0; new_vfd < MAX_FDNUM; new_vfd++)
    if (process->fd[new_vfd] == -1) break;

  if (new_vfd < MAX_FDNUM) {
    process->fd[new_vfd] = process->fd[vfd];
    /* should clear close-on-exec flag of new_vfd */
    SyscallSetSuccess( new_vfd );
  } else {
    SyscallSetError( EBADF );
  }
  return picode->next;
}
 
OP(aint_pipe);
OP(aint_set_program_attributes)
{
  /* doesn't seem to do anything useful -Jamey 8/30/96 */
  /* sets text_start, text_size in ublock */
  /* sets data_start, data_size in ublock */
  
  SyscallSetSuccess( 0 );
  return picode->next;
}

OP(aint_profil);

OP(aint_open)
{
  process_ptr process = pthread->process;
  int pfd, vfd;

  /* Search for an unused fd */
  for (vfd = 0; vfd < MAX_FDNUM; vfd++)
    if (pthread->process->fd[vfd] == -1) break;

  if (vfd < MAX_FDNUM) {
    char *path = strdup_from_object_space(process, MapReg(A0));
    informative("open(%s, 0x%lx, 0x%lx) ", path, MapReg(A1), MapReg(A2));
    pfd = open(path, MapReg(A1), MapReg(A2));
    pthread->process->fd[vfd] = pfd;
    
    if (pfd == -1) {
      SyscallSetError( errno );
    } else {
      SyscallSetSuccess( vfd );
    }
    free(path);
  } else {
    SyscallSetError( ENOMEM );
  }
  return picode->next;
}

OP(aint_getgid)
{
  SyscallSetSuccess( getgid() );
  /* the obscure world of the getgid syscall also returns egid in a
   * non-standard return value register - ahrg! */
  MapReg(A4) = getegid();
  return(picode->next);
}

OP(aint_sigprocmask)
{
  /*
   * note: sigprocmask syscall interface differs from libc interface
   *   in: 
   *     A0 ... how
   *     A1 ... new mask VALUE!
   *   out:
   *     success: old mask VALUE!
   *     error:   errno code
   */

  int how = MapReg(A0);
  sigset_t new_mask = MapReg(A1);
  sigset_t old_mask;
  sigset_t aint_mask;
  int sim_err, sim_errno;

  /*
   * since there is some non-trivial masking going on in the OS (besides
   * the obvious one), we let the OS handle this operation by temporarily
   * swapping in the simulated signal mask into the simulator process
   */

  /* swap aint real mask with simulated mask */
  sigprocmask(SIG_SETMASK, &(pthread->process->signal_mask), &aint_mask);

  /* do the operation on simulated mask */
  sim_err = sigprocmask(how, &new_mask, &old_mask);
  sim_errno = errno;

  /* swap back */
  sigprocmask(SIG_SETMASK, &aint_mask, &(pthread->process->signal_mask));

  /* setup return registers */
  if (sim_err == 0) {
    SyscallSetSuccess( old_mask );
  } else {
    SyscallSetError( sim_errno );
  }
  return picode->next;
}

OP(aint_getlogin);
OP(aint_setlogin);
OP(aint_acct);
OP(aint_sigpending);

OP(aint_ioctl)
{
#if OSVER < 40
  extern int ioctl(int d, int request, void * arg );
#endif

  /* The fd is in a0, the command in a1, and the output buf in a2 */
  char mybuf[255];
  int vfd, pfd, len;
  process_ptr process = pthread->process;

  vfd = MapReg(A0);
  pfd = pthread->process->fd[vfd];

  len = (MapReg(A1) >> 16) & 0x1fff;

  if ((MapReg(A1)>> 28) & 0x2) {
    memcpy_from_object_space(process, mybuf, MapReg(A2), len);

  }

  SyscallSetRegs( ioctl(pfd, MapReg(A1), mybuf) );

  /* if out parameter, then fill in the results */
  if ( (MapReg(A1)>>28) && 0x4) {
    memcpy_to_object_space(process, MapReg(A2), mybuf, len);
  }
  
  return(picode->next);

}

OP(aint_reboot);
OP(aint_revoke);

OP(aint_symlink)
{
  process_ptr process = pthread->process;
  char *path1 = strdup_from_object_space(process, MapReg(A0));
  char *path2 = strdup_from_object_space(process, MapReg(A1));

  SyscallSetRegs( symlink(path1, path2) );

  free(path1);
  free(path2);
  return picode->next;
}

OP(aint_readlink)
{
  process_ptr process = pthread->process;
  char *path1 = strdup_from_object_space(process, MapReg(A0));
  char buffer[BUFSIZ];

  SyscallSetRegs( readlink(path1, buffer, BUFSIZ) );

  if (IsSyscallSuccess()) {
    if (strlen(buffer) < MapReg(A2)) {
      strcpy_to_object_space(process, MapReg(A1), buffer);
    } else {
      SyscallSetError( ERANGE );
    }
  }

  free(path1);
  return picode->next;
}

OP(aint_execve);

OP(aint_umask)
{
  SyscallSetRegs( umask(MapReg(A0)) );
  return picode->next;
}

OP(aint_chroot);

OP(aint_getpgrp);

OP(aint_getpagesize)
{
  SyscallSetRegs( getpagesize() );
  return picode->next;
}

OP(aint_mremap);
OP(aint_vfork);


OP(aint_stat)
{
  process_ptr process = pthread->process;
  char filename[256];
  struct stat statbuf = {0};

  /* Copy filename */
  strcpy_from_object_space(process, filename, MapReg(A0));

  SyscallSetRegs( stat(filename, &statbuf) );

  /* Copy struct stat */
  memcpy_to_object_space(process, MapReg(A1), &statbuf, sizeof(struct stat));
  if (Verbose)
    informative("[p%d.t%d] aint_stat(\"%s\", [%lx,%lx])\n", process->pid, pthread->tid, filename, 
	    MapReg(A1), MapReg(A1)+sizeof(struct stat));

  return picode->next;
}

OP(aint_lstat)
{
  process_ptr process = pthread->process;
  char filename[256];
  struct stat statbuf = {0};

  /* Copy filename */
  strcpy_from_object_space(process, filename, MapReg(A0));

  SyscallSetRegs( lstat(filename, &statbuf) );

  /* Copy struct stat */
  memcpy_to_object_space(process, MapReg(A1), &statbuf, sizeof(struct stat));
  if (Verbose)
    informative("[p%d.t%d] \t aint_lstat(\"%s\", [%lx,%lx])\n",
	    process->pid, pthread->tid, filename, 
	    MapReg(A1), MapReg(A1)+sizeof(struct stat));

  return picode->next;
}

OP(aint_sbrk);
OP(aint_sstk);

OP(aint_mmap)
{
  process_ptr process = pthread->process;
  ulong vaddr = MapReg(A0);
  ulong len = MapReg(A1);
  ulong prot = MapReg(A2);
  ulong flags = MapReg(A3);
  long vfd = MapReg(A4);
  ulong offset = MapReg(A5);
  int pfd;
  void *paddr;
  int shmid;
  long thread_errno;

  if (Verbose)
    informative("[p%d.t%d] \t aint_mmap(0x%lx, %ld, prot=0x%lx, flags=0x%lx, %ld/%d, off=0x%lx)\n",
	    process->pid, pthread->tid, 
	    vaddr, len, prot, flags, vfd, pthread->process->fd[vfd], offset);

  if (   ((vaddr & (TB_PAGESIZE-1)) != 0)
      || ((prot & ~(PROT_READ|PROT_WRITE|PROT_EXEC|PROT_NONE)) != 0)
      || ((flags & ~(MAP_FILE|MAP_ANONYMOUS|MAP_VARIABLE|MAP_FIXED|MAP_SHARED|MAP_PRIVATE)) != 0)
      ) {
    SyscallSetError( EINVAL );
    if (Verbose)
      informative("\t mmap() => EINVAL vaddr/prot/flags \n");
    return picode->next;
  }
  if (vfd < 0 || vfd >= MAX_FDNUM) {
    pfd = -1;
  } else {
    pfd = process->fd[vfd];
    if (pfd == -1) {
      SyscallSetError( EBADF );
      if (Verbose)
	informative("\t mmap() => EBADF fd \n");
      return picode->next;
    }
  }
  if ((flags & MAP_ANONYMOUS) && (pfd != -1)) {
    SyscallSetError( EINVAL );
    if (Verbose)
      informative("\t mmap() => EINVAL flags/pfd \n");
    return picode->next;
  }

  paddr = mmap(NULL, len, prot, flags, pfd, offset);
  SyscallSetRegs( (unsigned long) paddr );

  if (! IsSyscallSuccess()) {
    if (Verbose)
      informative("\t mmap() => errno %d from mmap \n", errno);
    return picode->next;
  }
  /* we use the shmseg structure so that tlb_miss() will fill in the TB entries for mmap()'ed pages */
  shmid = shmseg_allocate(0, len, 0, &thread_errno);
  informative("\t mmap() => %ld from shmseg_allocate\n", shmid);
  if (shmid == -1) {
    SyscallSetError( thread_errno );
    if (Verbose)
      informative("\t mmap() => %ld from shmseg_allocate \n", thread_errno);
    return picode->next;
  }
  Shmseg[shmid].paddr = paddr;
  vaddr = shmseg_attach(process, shmid, vaddr, flags, &thread_errno);
  if ((long)vaddr == -1ul) {
    SyscallSetError( thread_errno );
    if (Verbose)
      informative("\t mmap() => %ld from shmseg_attach \n", thread_errno);
    return picode->next;
  } else {
    SyscallSetSuccess( vaddr );
  }
  /* may have to decode new text upon close */
  if (   (prot&PROT_WRITE) == 0
      && vfd != -1ul
      && (flags&MAP_ANONYMOUS) == 0) {
    textregion[process->pid][vfd].startaddr = vaddr;
    textregion[process->pid][vfd].len = len;
  }
  return picode->next;
}

OP(aint_munmap)
{
  warning("AINT_MUNMAP: implemented as no-op\n");
  SyscallSetSuccess( 0 );
  return picode->next;
}

OP(aint_madvise)
{
  /* no functionality in OSF1 */
  SyscallSetSuccess( 0 );
  return picode->next;
}

OP(aint_table)
{
  process_ptr process = pthread->process;
  long id = MapReg(A0);
  long index = MapReg(A1);
  ulong addr = MapReg(A2);
  long nel = MapReg(A3);
  ulong lel = MapReg(A4);
  char* aint_buf = calloc(1, nel * lel);

  if (aint_buf == NULL) {
     fatal("failed to allocate aint_buf in aint_table");
  }

  /* copy buffer from object- to aint-address space */
  memcpy_from_object_space(pthread->process, aint_buf, addr, nel * lel);

  /* let real syscall fill buffer in aint-address space */
  SyscallSetRegs( table(id, index, (void*)addr, nel, lel) );

  /* copy buffer back from aint- to object-address space */
  memcpy_to_object_space(process, addr, aint_buf, nel * lel);
  free(aint_buf);

  return picode->next;
}

OP(aint_gethostname)
{
  process_ptr process = pthread->process;
  ulong name_ubuf = MapReg(A0);
  ulong namelen = MapReg(A1);
  char *namebuf = calloc( 1, namelen+1);

  if (namebuf == NULL)
     fatal("failed to allocate namebuf in aint_gethostname");

  SyscallSetRegs( gethostname(namebuf, namelen) );

  memcpy_to_object_space(process, name_ubuf, namebuf, namelen);
  free(namebuf);

  return picode->next;
}
OP(aint_getdomainname)
{
  process_ptr process = pthread->process;
  ulong name_ubuf = MapReg(A0);
  ulong namelen = MapReg(A1);
  char *namebuf = calloc( 1, namelen+1);

  if (namebuf == NULL)
     fatal("failed to allocate namebuf in aint_getdomainname");

  SyscallSetRegs( getdomainname(namebuf, namelen) );

  memcpy_to_object_space(process, name_ubuf, namebuf, namelen);
  free(namebuf);

  return picode->next;
}

OP(aint_fstat)
{
  process_ptr process = pthread->process;
  int vfd, pfd;
  struct stat statbuf = {0};

  vfd = MapReg(A0);
  pfd = pthread->process->fd[vfd];

  SyscallSetRegs( fstat(pfd, &statbuf) );

/* Copy struct stat */
  memcpy_to_object_space(process, MapReg(A1), &statbuf, sizeof(struct stat));
  if (Verbose)
    informative("[p%d.t%d] \t aint_fstat(%d/%d, [%lx,%lx])\n",
	    process->pid, pthread->tid, vfd, pfd, 
	    MapReg(A1), MapReg(A1)+sizeof(struct stat));

  return picode->next;
}

OP(aint_fcntl)
{
  process_ptr process = pthread->process;
  int vfd = MapReg(A0);
  int request = MapReg(A1);
  ulong arg = MapReg(A2);
  int pfd;
  int status;
  char *request_name = NULL;

  if ((vfd < 0) || (vfd >= MAX_FDNUM) || ((pfd = process->fd[vfd]) == -1)) {
    SyscallSetError( EBADF );
    return picode->next;
  }

  switch (request) {
  case F_GETLK: 
    request_name = "F_GETLK";
  case F_SETLK:
    request_name = (request_name ? request_name : "F_SETLK");
  case F_SETLKW: {
    struct flock flock_buf = {0};
    request_name = (request_name ? request_name : "F_SETLKW");
    
    memcpy_from_object_space(process, (char*)&flock_buf, arg, sizeof(struct flock));
    SyscallSetRegs( fcntl(pfd, request, &flock_buf) );
    if (IsSyscallSuccess())
      memcpy_to_object_space(process, arg, (char*)&flock_buf, sizeof(struct flock));

  } break;
  default:
    request_name = "??";
    SyscallSetRegs( fcntl(pfd, request, arg) );
  }
  if (Verbose)
    informative("[p%d.t%d] \t aint_fcntl(%d/%d, %d:%s, [%lx,%lx])\n",
	    process->pid, pthread->tid, vfd, pfd, request, request_name,
	    arg, arg+sizeof(struct flock));

  return picode->next;
}

/*
 * r2r: the following implementation is from CU; don't know if we are in sync
 * wrt. the data structures that are used here;
 * Note: this implementation is not complete; we just fix up enough of the
 * state for an AINT process/thread such that we cover the main functionality
 * for sigreturn, which libc has started to use to implement longjmp;
 * we don't worry much about more esoteric uses of sigreturn;
 */
OP(aint_sigreturn)
{
  static int first = TRUE;
  process_ptr process = pthread->process;
  struct sigcontext sc;
  int reg;
      
  if (first) {
    /*
     * this implementation might not be completely correct, but it seems
     * to be good enough to handle the longjmp return functionality
     */
    fprintf(stderr, "partially supported sigreturn() call\n");
    first = FALSE;      
  }

  memcpy_from_object_space(process, (char*)&sc, MapReg(A0), sizeof(sc));

  process->signal_mask = sc.sc_mask;

  for (reg = 0; reg < 31; reg++) { /* skip zero-register */
    /* int registers */
    MapReg(reg) = sc.sc_regs[reg];
  }
  for (reg = 0; reg < 31; reg++) { /* skip zero-register */
    /* fp registers */
    MapFP(reg) = sc.sc_fpregs[reg];
  }
  pthread->fpcr = sc.sc_fpcr;

  return (addr2iphys(process, sc.sc_pc, NULL));
}

OP(aint_rename)
{
  process_ptr process = pthread->process;
  char *path_from = strdup_from_object_space(process, MapReg(A0));
  char *path_to = strdup_from_object_space(process, MapReg(A1));

  SyscallSetRegs( rename(path_from, path_to) );

  free(path_from);
  free(path_to);
  return picode->next;
}

OP(aint_truncate)
{
  process_ptr process = pthread->process;
  char *path = strdup_from_object_space(process, MapReg(A0));
  off_t length = MapReg(A1);

  SyscallSetRegs( truncate(path, length) );

  free(path);
  return picode->next;
}

OP(aint_ftruncate)
{
  int vfd = MapReg(A0);
  off_t length = MapReg(A1);

  if (vfd < 0 || vfd >= MAX_FDNUM) {
    SyscallSetError( EBADF );
    return picode->next;
  }

  SyscallSetRegs( ftruncate(pthread->process->fd[vfd], length) );

  return picode->next;
}

OP(aint_flock)
{
  process_ptr process = pthread->process;
  int vfd = MapReg(A0);
  int operation = MapReg(A1);
  int pfd;
  if (operation & LOCK_NB == 0) {
     warning("aint_flock: blocking lock, downgrading to nonblocking\n");
     operation |= LOCK_NB;
  }
  if ((vfd < 0) || (vfd >= MAX_FDNUM) || ((pfd = process->fd[vfd]) == -1)) {
    SyscallSetError( EBADF );
    return picode->next;
  }

  SyscallSetRegs( flock(pfd, operation) );

  return picode->next;
}

OP(aint_sendto)
{
  process_ptr process = pthread->process;
  int vfd = MapReg(A0);
  int pfd;
  int flags = MapReg(A3);
  long msgsize = MapReg(A2);
  long addrsize = MapReg(A5);
  char *mymsg;
  struct sockaddr myaddr;

  mymsg = (char *) calloc( 1, msgsize);
  if (mymsg == NULL) {
    fatal("aint_sendto failed to allocate a %d-byte msg buffer\n", msgsize);
  }

  pfd = pthread->process->fd[vfd];

  memcpy_from_object_space(process, mymsg, MapReg(A1), msgsize);
  memcpy_from_object_space(process, (char*)&myaddr, MapReg(A4), addrsize);

  SyscallSetRegs( sendto(pfd, mymsg, msgsize, flags, &myaddr, addrsize) );

  free((void *) mymsg);
  return picode->next;
}

extern unsigned long PMINT_get_instr_issue_count();

/* We try to simulate the system clock */
/* We assume 1usec passes every 1K instr issues */
OP(aint_gettimeofday)
{
  process_ptr process = pthread->process;
  static struct timeval init_tval = {0};
  struct timeval tval = {0};
  static struct timezone tzone = {0};
  static int already_called=0;
  int ret_val;
  int icount;

  if(!already_called){
    ret_val = gettimeofday(&init_tval, &tzone);
    if (ret_val!=0)
      error("Call of gettimeofday failed. Why??????");
    init_tval.tv_usec=0;
  }

  /* We want to return the original tzone, and the init_tval incremented
   * based on num instrs executed.
   * For simplicity, we initialize tv_usec to 0 on the first call.
   */
  tval = init_tval;

  /*
   * (r2r): Ahhhhrg! This is great for getting non-repeatable behavior. If
   * the ISSUE count changes in the timing simulator, the syscall will
   * return different results !?
   * I think this is a _very_ bad idea.  * FIXME *
   */
  icount = PMINT_get_instr_issue_count();
  icount = icount/1024;
  tval.tv_usec += icount%1000000;
  tval.tv_sec +=  icount/1000000;
  
  /* Copy the timeval and timezone structs */
  memcpy_to_object_space(process, MapReg(A0), (char*)&tval, sizeof(struct timeval));

  if ( MapReg(A1) != 0 ) {
    unsigned long p_a1 = MapReg(A1);
    memcpy_to_object_space(process, p_a1, (char*)&tzone, sizeof(struct timezone));
  }

  SyscallSetSuccess( 0 );

  return picode->next;
}

OP(aint_getrlimit)
{
  process_ptr process = pthread->process;
  int resource = MapReg(A0);
  struct rlimit rl = {0};
  if (resource == RLIMIT_NOFILE) {
    SyscallSetSuccess( 0 );
    rl.rlim_cur = MAX_FDNUM;
    rl.rlim_max = MAX_FDNUM;
    memcpy_to_object_space(process, MapReg(A1), (char*)&rl, sizeof(struct rlimit));
  } else {
    SyscallSetRegs( getrlimit(resource, &rl) );
    if (IsSyscallSuccess()) {
      memcpy_to_object_space(process, MapReg(A1), (char*)&rl, sizeof(struct rlimit));
    }
  }
  return picode->next;
}

OP(aint_setrlimit)
{
  process_ptr process = pthread->process;
  int resource = MapReg(A0);
  struct rlimit rl = {0};
  memcpy_from_object_space(process, (char*)&rl, MapReg(A1), sizeof(struct rlimit));
  if (resource == RLIMIT_NOFILE) {
    if (rl.rlim_max > MAX_FDNUM) {
      SyscallSetError( EPERM );
    } else if (rl.rlim_cur > rl.rlim_max) {
      SyscallSetError( EINVAL );
    } else {
      SyscallSetSuccess( 0 );
    }
  } else {
    SyscallSetRegs( setrlimit(resource, &rl) );
  }
  return picode->next;
}

OP(aint_sigaction)
{
  process_ptr process = pthread->process;
  int signal = MapReg(A0);

  if (   signal >= 0
      && signal <= SIGUSR2
      && signal != SIGKILL
      && signal != SIGSTOP
      && signal != SIGCONT) {
    struct sigaction oaction = {0};

    oaction.sa_handler = (void*)process->sigv[signal].sv_handler;
    oaction.sa_mask = process->sigv[signal].sv_mask;
    oaction.sa_flags = process->sigv[signal].sv_flags;

    if (MapReg(A1)) {
       struct sigaction action = {0};
       memcpy_from_object_space(process, (char*)&action, MapReg(A1), sizeof(action));

       process->sigv[signal].sv_handler = (void*)action.sa_handler;
       process->sigv[signal].sv_mask = action.sa_mask;
       process->sigv[signal].sv_flags = action.sa_flags;
    }
    if (MapReg(A2))
      memcpy_to_object_space(process, MapReg(A2), (char*)&oaction, sizeof(oaction));

    SyscallSetSuccess( 0 );
  } else {
    SyscallSetError( EINVAL );
  }
  return picode->next;
}


extern int getdirentries(int fd, char *buf, int nbytes, long *basep );

/*
 *
 * The goal of cleanup_dirent_padding is to clean up the padding bytes
 * between the different 'dirent' structures that are returned by the OS
 * in a call to 'getdirentries'.
 *
 * The 'dirent' structure has three fixed size fields 
 *
 *  . inode number
 *  . record length of the direntry (i.e. total bytes of this entry)
 *  . number of characters in the file name (without the terminating null byte)
 *
 * and then a variable number of bytes that contain
 *
 *  . a file name
 *  . padding with null bytes up to a 4-byte boundary
 *  . padding with random bytes up to the end of the record
 *
 */
void cleanup_dirent_padding(struct dirent *de,int nbytes)
{
 int nhead;
 int npad;
 char *pad;
 char *end = ((char *)de) + nbytes;

 /*
  * Size of record head
  */
 nhead = sizeof(de->d_ino) + sizeof(de->d_reclen) + sizeof(de->d_namlen);

 /*
  * We start padding each record after the last valid character of
  * the file name.
  */
 while ( de->d_ino != 0 && de->d_reclen != 0 && de->d_namlen != 0) {
  npad = de->d_reclen - nhead - de->d_namlen;
  pad = &de->d_name[de->d_namlen];
  while ( npad > 0 ) {
   assert(pad < end);
   *pad = '\0';
   pad++;
   npad--;
  }

  /*
   * Advance to next record
   */
  de =(struct dirent *) (((char *)de) + de->d_reclen);
 }
}

OP(aint_getdirentries)
{
  process_ptr process = pthread->process;
  long vfd = MapReg(A0);
  ulong object_buf = MapReg(A1);
  ulong nbytes = MapReg(A2);
  ulong object_basep = MapReg(A3);
  int pfd;
  char *buf = calloc( 1, nbytes);
  long basep;
  int status;

  if (buf == NULL)
    fatal("aint_getdirentries failed to allocate %d bytes\n", nbytes);

  if ((vfd < 0) || (vfd >= MAX_FDNUM) || ((pfd = process->fd[vfd]) == -1)) {
    SyscallSetError( EBADF );
    return picode->next;
  }

  SyscallSetRegs( getdirentries(pfd, buf, nbytes, &basep) );

  /*
   * Roger: this is a problem that Bobbie spotted that created undeterministic
   * behaviour and verifier failures. The problem is that the 'dirent's returned
   * by the OS in the memory area pointed by 'buf' contain padding bytes that
   * are not completly clean. So, on repeated calls to getdirentries, the main 
   * thread and the verifier get slightly different memory buffers, yielding
   * problems when comparing the name strings used inside 'dirent' using
   * full quadwords (as the strlen routine does...).
   * The solution is to cleanup those padding bytes manually.
   */
  cleanup_dirent_padding((struct dirent *) buf,nbytes);

  if (IsSyscallSuccess()) {
    memcpy_to_object_space(process, object_buf, (void*)buf, nbytes);
    *(long *)addr2phys(process, object_basep, NULL) = basep;
  }
  if (Verbose) {
    informative("[p%d.t%d] \t aint_getdirentries(%ld/%d, [%lx,%lx], %ld, %lx)\n",
	    process->pid, pthread->tid, vfd, pfd, 
	    object_buf, object_buf+nbytes, nbytes,
	    object_basep);
  }

  free(buf);
  return picode->next;
}


extern int statfs(char *path, struct statfs *buffer, int length );
extern int fstatfs(int file_descriptor, struct statfs *buffer, int length );

OP(aint_statfs)
{
  process_ptr process = pthread->process;
  char *path;
  ulong path_ptr = MapReg(A0);
  ulong object_buf = MapReg(A1);
  ulong length = MapReg(A2);
  struct statfs aint_buf = {0};

  path = strdup_from_object_space(process, path_ptr);

  SyscallSetRegs( statfs(path, &aint_buf, sizeof(struct statfs)) );

  if (IsSyscallSuccess()) {
    memcpy_to_object_space(process, object_buf, (char*)&aint_buf,  sizeof(struct statfs));
  }
  if (Verbose)
    informative("[p%d.t%d] \t aint_statfs(\"%s\", [%lx,%lx], %ld)\n",
	    process->pid, pthread->tid, path, 
	    object_buf, object_buf+sizeof(struct statfs),
	    length);

  free(path);
  return picode->next;
}

OP(aint_fstatfs)
{
  process_ptr process = pthread->process;
  int vfd = MapReg(A0);
  ulong object_buf = MapReg(A1);
  struct statfs aint_buf = {0};
  int pfd;

  if ((vfd < 0) || (vfd >= MAX_FDNUM) || ((pfd = process->fd[vfd]) == -1)) {
    SyscallSetError( EBADF );
    return picode->next;
  }

  SyscallSetRegs( fstatfs(pfd, &aint_buf, sizeof(struct statfs)) );

  if (IsSyscallSuccess()) {
    memcpy_to_object_space(process, object_buf, (char*)&aint_buf, sizeof(struct statfs));
  }
  return picode->next;
}


OP(aint_getdtablesize)
{
  SyscallSetSuccess( MAX_FDNUM );
  return picode->next;
}

OP(aint_getrusage)
{
  process_ptr process = pthread->process;
  struct rusage mybuf = {0};

  SyscallSetRegs( getrusage(MapReg(A0), &mybuf) );

  /* Copy the struct rusage */
  memcpy_to_object_space(process, MapReg(A1), (char*)&mybuf, sizeof(mybuf));

  return picode->next;
}

OP(aint_pathconf)
{
  process_ptr process = pthread->process;
  char *path;
  ulong path_ptr = MapReg(A0);
  int name = MapReg(A1);
  long result;

  path = strdup_from_object_space(process, path_ptr);

  result = pathconf(path, name);

  if (result != -1) {
    SyscallSetSuccess( result );
  } else {
    SyscallSetError( errno );
  }

  if (Verbose) {
    informative("[p%d.t%d] \t aint_pathconf(\"%s\", %d)\n",
	    process->pid, pthread->tid, path, name);
  }

  free(path);
  return picode->next;
}

OP(aint_fpathconf)
{
  process_ptr process = pthread->process;
  char *path;
  int filedes = MapReg(A0);
  int name = MapReg(A1);
  long result;

  result = fpathconf(filedes, name);

  if (result != -1) {
    SyscallSetSuccess( result );
  } else {
    SyscallSetError( errno );
  }

  if (Verbose) {
    informative("[p%d.t%d] \t aint_fpathconf(%d, %d)\n",
	    process->pid, pthread->tid, filedes, name);
  }

  return picode->next;
}

OP(aint_uswitch)
{
  extern long uswitch (long cmd, long mask);
  process_ptr process = pthread->process;

  long cmd = MapReg(A0);
  long mask = MapReg(A1);
  long result = 0;

  if ((mask & ~USW_NULLP) == 0) {
    segment_t *null_seg = process_find_segment(process, 0);

    if (cmd == USC_SET) {
      if (mask & USW_NULLP) {
	process_add_segment(process, 0, 1, PROT_READ, MAP_PRIVATE);
      } else {
	/* disable null pointer references */
	if (null_seg)
	  null_seg->prot &= ~(PROT_READ|PROT_WRITE);
      }
      result = mask;
    } else {
      if (null_seg && (null_seg->prot & PROT_READ))
	result |= USW_NULLP;
    }

    SyscallSetSuccess( result );

  } else {
    warning("USWITCH: %lx %lx -- please report\n", cmd, mask);
    SyscallSetRegs( uswitch(cmd, mask) );
  }
  return picode->next;
}


/* Other socket related calls are needed */
/* Can we just pass them all through? */
OP(aint_socket)
{
  int domain = MapReg(A0);
  int type = MapReg(A1);
  int protocol = MapReg(A2);


  SyscallSetRegs( socket(domain,type,protocol) );

  return picode->next;
}


/* This probably doesn't do the right thing! */
OP(aint_setitimer)
{
  int which = MapReg(A0);
  const struct itimerval *value_orig = (struct itimerval *) MapReg(A1);
  struct itimerval *ovalue_orig = (struct itimerval *)MapReg(A2);

 struct itimerval *value = calloc( 1, sizeof(struct itimerval));
 struct itimerval *ovalue = calloc( 1, sizeof(struct itimerval));

  memcpy_from_object_space(pthread->process, (char*)value, (ulong)value_orig, sizeof(struct itimerval));
  
  fprintf(Aint_output,"setitimer called, but .... (%d %d %d %d)\n",
	  value->it_interval.tv_sec,value->it_interval.tv_usec,
	  value->it_value.tv_sec,value->it_value.tv_usec);

  SyscallSetRegs( setitimer(which,value,ovalue) );

  if(ovalue_orig!=NULL)
    memcpy_to_object_space(pthread->process, (ulong)ovalue_orig, (char*)ovalue, sizeof(struct itimerval));

  return picode->next;
}




OP(aint_usleep_thread)
{
  SLEEP(pthread);
  SyscallSetSuccess( R_SLEEP );
  return(picode->next);
}

extern int set_speculative(void *, long, long, long, long);
OP(aint_set_speculative)
{
  static int first = TRUE;

  /* version difference between v1 and v2 requires this construct */
  struct args {
    union {
      vm_offset_t  addr;
      long         type;
    } arg0;
    union {
      vm_size_t    size;
      vm_offset_t  addr;
    } arg1;
    union {
      long         segv_action;
      long         fault_pc;
    } arg2;
    union {
      long         fpe_action;
      long         fault_code;
    } arg3;
    long    version;
  } uap;
  process_ptr process = pthread->process;

  uap.arg0.type = MapReg(A0);
  uap.arg1.addr = MapReg(A1);
  uap.arg2.segv_action = MapReg(A2);
  uap.arg3.fpe_action = MapReg(A3);
  uap.version = MapReg(A4);

  if (first) {
    /* this implementation is not here */
    fprintf(stderr, "CALLSYS: partially supported set_speculative() call\n");
    first = FALSE;      
  }

  if (uap.version > 2) {
    SyscallSetError( EINVAL );
  } else if (! (uap.arg2.segv_action >= 0 || uap.arg2.segv_action <= 2)) {
    SyscallSetError( EINVAL );
  } else if (! (uap.arg3.fpe_action >= 0 || uap.arg3.fpe_action <= 2)) {
    SyscallSetError( EINVAL );
  } else {
    informative("SET_SPECULATIVE: %lx %lx %lx %lx %ld\n",
                (void *) uap.arg0.type,
		(void *) uap.arg1.addr,
		uap.arg2.segv_action,
		uap.arg3.fpe_action,
		uap.version);

    /* (r2r) this is all we implement for now:
     * if the see the initialize call, we copy the action field into AINT per
     * process data structures; they get used from there by the signal handler
     * that is called when AINT_issue traps; we determine there if we can
     * actually handle the action type that is set at that point; (lazy eval)
     */
    if ((uap.version == 1) ||
        ((uap.version == 2) &&
         (uap.arg0.type == SPEC_ACTION_INITIALIZE)))
    {
      process->spec_segv_action = uap.arg2.segv_action;
      process->spec_fpe_action = uap.arg3.fpe_action;

      SyscallSetSuccess( 0 );
    } else {
      SyscallSetError( EINVAL );
    }
  }

  return picode->next;
}

OP(aint_getsysinfo)
{
  ulong op = MapReg(A0);
  ulong buffer = MapReg(A1);
  ulong nbytes = MapReg(A2);
  ulong start = MapReg(A3);
  ulong arg = MapReg(A4);

  char *buf = calloc( 1, nbytes);
  if (buf == NULL)
    fatal("aint_getsysinfo failed to allocate %d bytes\n", nbytes);

  SyscallSetRegs( getsysinfo(op,
			       buf,
			       nbytes,
			       (start ? (int*)PMAP(start) : 0),
			       (arg ? (void*)PMAP(arg) : 0)) );

  if (IsSyscallSuccess()) {
    if (buffer)
      memcpy_to_object_space(pthread->process, buffer, buf, nbytes);
  }
  free (buf);

  return(picode->next);
}

OP(aint_setsysinfo)
{
  ulong op = MapReg(A0);
  ulong buffer = MapReg(A1);
  ulong nbytes = MapReg(A2);
  ulong start = MapReg(A3);
  ulong arg = MapReg(A4);
  ulong flag = MapReg(A5);
  ulong retval = 0;
  ulong errval = 0;

  switch (op) {
  case SSI_NVPAIRS: {
    /* nbytes is actually nelts, for int elts -Jamey, 7/15/96 */
    int npairs = nbytes;
    int bufsize = npairs*2*sizeof(int);
    int *buf = (int*)calloc( 1, bufsize);
    int i;

    informative("aint_setsysinfo(SSI_NVPAIRS buf=%lx nbytes=%ld start=%ld arg=%lx flag=%lx)\n",
	    buffer, nbytes, start, arg, flag);
    if (buf == NULL)
      fatal("aint_setsysinfo failed to allocate %d bytes\n", bufsize);
    memcpy_from_object_space(pthread->process, (char*)buf, buffer, bufsize);
    for (i = 0; i < npairs; i++) {
      int op = buf[2*i];
      switch(op) {
      case SSIN_UACPROC: {/* set unaligned access processing on/off */
	int buf[2];
	int rc = setsysinfo(SSI_NVPAIRS, buf, sizeof(buf), 0);
	informative("\t SSIN_UACPROC rc=%d errno=%d\n", rc, (rc ? errno : 0));
      } break;
      default:
	warning("Encountered name=%d in setsysinfo(SSI_NVPAIRS, ...)\n", op);
      }
    }
    free(buf);
  } break;
  case SSI_IEEE_FP_CONTROL: {
    long buf;
    double fpcr = *(double*)&pthread->fpcr;

#ifdef __GNUC__
    asm ("trapb; mf_fpcr %0; trapb" : : "f" (fpcr));
#else
    fpcr = dasm ("trapb; mf_fpcr $f0; trapb");
#endif

    informative("setsysinfo: SSI_IEEE_FP_CONTROL -- fpcr was %lx\n", 
		*(ulong*)&fpcr);

    memcpy_from_object_space(pthread->process, (char*)&buf, buffer, 
			     sizeof(buf));
    retval = setsysinfo(SSI_IEEE_FP_CONTROL, &buf, sizeof(buf), start, arg, flag);
    if (!retval)
      errval = errno;
    informative("setsysinfo: SSI_IEEE_FP_CONTROL -- set IEEE_FP_CONTROL set to %lx\n", buf);

    {
      /* testing */
      getsysinfo(GSI_IEEE_FP_CONTROL, (char*)&buf, sizeof(buf), 0, 0, 0);
      informative("setsysinfo: GSI_IEEE_FP_CONTROL -- IEEE_FP_CONTROL is %lx\n", buf);
    }

#ifdef __GNUC__
    asm ("trapb; mf_fpcr %0; trapb" : "=f" (fpcr));
#else
    fpcr = dasm ("trapb; mf_fpcr $f0; trapb");
#endif

    pthread->fpcr |= *(ulong*)&fpcr;
    if (pthread->fpcr & FPCR_UNDZ)
      pthread->fpcr |= FPCR_UNFD;

    informative("setsysinfo: SSI_IEEE_FP_CONTROL -- fpcr now %lx\n", 
		*(ulong*)&fpcr);
  } break;
  default: {
    warning("Encountered op=%d in setsysinfo(%d, ...)\n", op, op);
  }
  }

  errno = errval;
  SyscallSetRegs( retval );

  return(picode->next);
}

OP(aint_pthread_create)
{
  process_ptr process = pthread->process;
    thread_ptr child;
    int tid;
    event_ptr pevent;

  ulong pthread_ptr = MapReg(A0);
  /* ulong pthread_attr_ptr = MapReg(A1); */
  ulong child_pc = MapReg(A2);
  ulong child_fp = MapReg(A3);

    /* allocate a new thread */
    INLINE_DEQUEUE(&Free_q, child);

    if (child == NULL)
        fatal("\taint_pthread_create: exceeded thread limit (%d)\n", Max_nthreads);

    child->process = pthread->process;
    /* Insert child in the process' thread list */
    pthread->process->youngest_thread->tsibling = child;
    pthread->process->youngest_thread = child;

    pthread->process->thread_count++;

    /* if (childp->addr_space == 0) {
    if (1) { */

    /* Always allocate new event struct */
    NEW_ITEM(Event_free, sizeof(event_t), pevent, "aint_fork");

    /*} else
      pevent = child->pevent;
      */

    tid = child->tid;
    if (Maxtid < tid)
	Maxtid = tid;

    Nthreads++;

    if (Nthreads > Max_nthreads)
	fatal("pthread_create: exceeded thread limit (%d)\n", Max_nthreads);

    /* copy the parent's address space to the child 
    copy_addr_space(pthread, child); */

    /* Allocate stack space for child HERE */
    init_thread_addr_space(pthread, child);

    /* init_thread() must be called *after* copy_addr_space() since
     * init_thread() uses the mapping fields set up by copy_addr_space().
     */
    init_thread(child, pevent);
    child->tsibling = NULL;

    /* the child's start time is the parent's plus the time for the fork */
    child->time = pthread->time + picode_cycles(picode);

    /* 
    if (First_fork_elapsed == 0.0) {
        First_fork_elapsed = child->time;
        First_fork_cpu = pthread->cpu_time + picode_cycles(picode);
    }
    */

    /* the first instruction for the child is the start routine */
    child->next_fetch_picode = IV2R(process, child_pc);
    
    /* the return value for the parent is 0 */
    SyscallSetSuccess( 0 );
    /* the return value for the child doesn't matter */
    MapReg_thr(child, RET_VAL_REG) = 0;
    MapReg_thr(child, ERROR_REG) = SyscallSuccess;

    /* Pass the correct procedure value */
    MapReg_thr(child, PROCEDURE_VALUE_REG) = child_pc;
    MapReg_thr(child, A0) = child_fp;

    /* Assign the correct thread structure to the pthread_t argument. */

  * (int *) addr2phys(process, pthread_ptr, NULL) = child->tid;

    child->sigblocked = pthread->sigblocked;

#ifdef PTHREAD_DEBUG
    fprintf(stderr, "\taint_pthread_create: creating %d.\n", child->tid); 
#endif
    pmint_thread_begin(child->tid);

    return picode->next;

    /* Remainder not used in pmint: */

    /* Return from the parent. If generating any events, then first
     * generate a fork event. The picode returned must be the one that
     * calls event_fork...
     */

}

OP(aint_pthread_self)
{
  SyscallSetSuccess( (unsigned long)pthread->tid );
  return picode->next;
}

OP(aint_pthread_join)
{
  process_ptr process = pthread->process;
  int joinee_tid;
  thread_ptr thread;
  ulong value_ptr = MapReg(A1);

  joinee_tid = MapReg(A0);
#ifdef PTHREAD_DEBUG
  fprintf(stderr, "\taint_pthread_join: thread 0x%lx would like to join thread  0x%lx.\n", pthread->tid, joinee_tid); 
#endif
  thread = &Threads[joinee_tid];
  /* Look at the state of the joinee. If it's done, detach it and return */
  if (thread->runstate == R_DONE) {
    /* save value */
    if (value_ptr)
      *(long*)addr2phys(process, value_ptr, NULL) = MapReg_thr(thread, A0);

    pthread->cpu_time += thread->cpu_time;
    thread->runstate = R_FREE;
    if (thread->process->thread_count == 0) thread->process->runstate = R_FREE;

    /* Return thread to free queue */
    INLINE_REMOVE(thread);
    if (Recycle_threads) {
      Nthreads--;
      INLINE_INSERT_AFTER(&Free_q, thread);
    }

    SyscallSetSuccess( 0 );
    return (picode->next);
  }

  SyscallSetSuccess( 0 );
  /* block thread */
  pmint_thread_block(pthread->tid);
  ifi->runstate = R_WAIT;
  /* let the other thread know that we're blocked */
  thread->wthread = pthread;
  return picode;
}

OP(aint_pthread_exit)
{
  pthread->exitcode = MapReg(A0);

#ifdef PTHREAD_DEBUG
  fprintf(stderr, "\taint_pthread_exit: thread 0x%lx \n", pthread->tid); 
#endif
  if (pthread->wthread) {
    thread_ptr wthread = pthread->wthread;
    ulong value_ptr = MapReg_thr(wthread, A1);
    /* save value */
    if (value_ptr)
      *(long*)addr2phys(wthread->process, value_ptr, NULL) = MapReg(A0);
    /* Wakeup waiting thread */
    pmint_thread_unblock(wthread->tid);
    ifi->runstate = R_FREE;
  } else {
    ifi->runstate = R_DONE;
  }
  pmint_thread_end(pthread->tid);

  if (ifi->runstate == R_FREE) {
    /* Return thread to free queue */
    INLINE_REMOVE(pthread);
    if (Recycle_threads) {
      Nthreads--;
      INLINE_INSERT_AFTER(&Free_q, pthread);
    }
  }
  SyscallSetSuccess( 0 );
  return NULL;
}

OP(aint_pthread_mutex_init)
{
  int tid = pthread->tid;
  process_ptr process = pthread->process;
  ulong mutex_ptr = MapReg(A0);
  unsigned int *paddr = NULL;
  if (mutex_ptr){
      paddr = (unsigned int *)addr2phys(pthread->process, mutex_ptr, NULL);
#ifdef PTHREAD_DEBUG
     fprintf(stderr, "\taint_pthread_mutex_init: thread 0x%lx mutex  0x%lx.\n", pthread->tid, paddr); 
#endif
      posix_register_mutex((void *)paddr);
  }else {
      fatal("\taint_pthread_mutex_init: mutex_ptr is NULL\n");
  }
  SyscallSetSuccess( 0 );
  return picode->next;
}

OP(aint_pthread_mutex_lock)
{
  int tid = pthread->tid;
  process_ptr process = pthread->process;
  ulong mutex_ptr = MapReg(A0);
  unsigned int *paddr = NULL;
  if (mutex_ptr){
      int locked = 0;
      paddr = (unsigned int *)addr2phys(pthread->process, mutex_ptr, NULL);
#ifdef PTHREAD_DEBUG
      fprintf(stderr, "\taint_pthread_mutex_lock: thread 0x%lx mutex  0x%lx.\n", pthread->tid, paddr); 
#endif
      locked  = is_posix_mutex_locked((void *)paddr);
      if(locked){
          AINT_THREAD waiter;
          waiter = pthread;
          SyscallSetSuccess( 0 );
          /* block thread */
          pmint_thread_block(pthread->tid);
          ifi->runstate = R_WAIT;
          /* mark that this thread is waiting for the mutex */
          posix_mutex_add_waiting_thread( (void *) paddr, waiter);
#ifdef PTHREAD_DEBUG
          fprintf(stderr, "\taint_pthread_mutex_lock: thread 0x%lx blocked on mutex (0x%lx).\n", waiter->tid, paddr); 
#endif
          return picode;
      }else{
          AINT_THREAD owner;
          owner = pthread;
#ifdef PTHREAD_DEBUG
          fprintf(stderr, "\taint_pthread_mutex_lock: thread 0x%lx locking mutex (0x%lx).\n", owner->tid, paddr); 
#endif
          SyscallSetSuccess( 0 );
          posix_mutex_lock((void *)paddr, owner);
      }
  }else {
      fatal("\taint_pthread_mutex_lock: mutex_ptr is NULL\n");
  }
  return picode->next;
}

OP(aint_pthread_mutex_unlock)
{
  int tid = pthread->tid;
  process_ptr process = pthread->process;
  ulong mutex_ptr = MapReg(A0);
  unsigned int *paddr = NULL;
  if (mutex_ptr){
      int locked = 0;
      paddr = (unsigned int *)addr2phys(pthread->process, mutex_ptr, NULL);
      locked  = is_posix_mutex_locked((void *)paddr);
      if(locked){
          AINT_THREAD unlocker;
          AINT_THREAD waiter;
          unlocker = pthread;
#ifdef PTHREAD_DEBUG
          fprintf(stderr, "\taint_pthread_mutex_unlock: thread 0x%lx  un-locking mutex (0x%lx).\n", unlocker->tid, paddr); 
#endif
          SyscallSetSuccess( 0 );
          posix_mutex_unlock((void *)paddr, unlocker);
          waiter = posix_mutex_get_next_waiting_thread( (void *) paddr);
          if(waiter != NULL) {
              /* Grant mutex to the waiter */
#ifdef PTHREAD_DEBUG
              fprintf(stderr, "\taint_pthread_mutex_unlock: thread 0x%lx now owns mutex (0x%lx).\n", waiter->tid, paddr);
#endif
              posix_mutex_lock((void *)paddr, waiter);
              /* Wake up waiter */
#ifdef PTHREAD_DEBUG
              fprintf(stderr, "\taint_pthread_mutex_unlock: waking up thread 0x%lx \n", waiter->tid);
#endif
              SyscallSetSuccess( 0 );
              pmint_thread_unblock(waiter->tid);
          }
      }else{
          fatal("\taint_pthread_mutex_unlock: Attempting to un-lock an unlocked mutex (0x%lx)!\n", paddr);
      }
  }else {
      fatal("\taint_pthread_mutex_unlock: mutex_ptr is NULL\n");
  }
  return picode->next;
}

OP(aint_pthread_condition_init)
{
  int tid = pthread->tid;
  process_ptr process = pthread->process;
  ulong cond_ptr = MapReg(A0);
  unsigned int *paddr = NULL;
  if (cond_ptr){
      paddr = (unsigned int *)addr2phys(pthread->process, cond_ptr, NULL);
      SyscallSetSuccess( 0 );
#ifdef PTHREAD_DEBUG
      fprintf(stderr, "\taint_pthread_cond_init: thread 0x%lx condition  0x%lx.\n", 
               pthread->tid, paddr); 
#endif
      posix_register_condition((void *)paddr);
  }else {
      fatal("\taint_pthread_cond_init: cond_ptr is NULL\n");
  }
  return picode->next;
}

/* int pthread_cond_wait(
                    pthread_cond_t      *cond,
                    pthread_mutex_t     *mutex);

  Release the mutex. Make the caller wait on cond.
*/
OP(aint_pthread_condition_wait)
{
  int tid = pthread->tid;
  process_ptr process = pthread->process;
  ulong cond_ptr = MapReg(A0);
  ulong mutex_ptr = MapReg(A1);
  unsigned int *cond_paddr = NULL;
  unsigned int *mutex_paddr = NULL;
  if (cond_ptr){
      cond_paddr = (unsigned int *)addr2phys(pthread->process, cond_ptr, NULL);
      if( mutex_ptr ) {
         int locked = 0;
         mutex_paddr = (unsigned int *)addr2phys(pthread->process, mutex_ptr, NULL);
#ifdef PTHREAD_DEBUG
         fprintf(stderr, "\taint_pthread_cond_wait: thread 0x%lx condition  0x%lx mutex 0x%lx.\n", 
              pthread->tid, cond_paddr, mutex_paddr); 
#endif
         locked  = is_posix_mutex_locked((void *)mutex_paddr);
         if(locked){
             AINT_THREAD unlocker;
             AINT_THREAD waiter;
             unlocker = pthread;
#ifdef PTHREAD_DEBUG
            fprintf(stderr, "\taint_pthread_condition_wait: thread 0x%lx  un-locking mutex (0x%lx).\n"
                  , unlocker->tid, mutex_paddr); 
#endif
              posix_mutex_unlock((void *)mutex_paddr, unlocker);
              waiter = posix_mutex_get_next_waiting_thread( (void *) mutex_paddr);
              if(waiter != NULL) {
                  /* Grant mutex to the waiter */
#ifdef PTHREAD_DEBUG
                  fprintf(stderr, "\taint_pthread_mutex_condition_wait: thread 0x%lx now owns mutex (0x%lx).\n", waiter->tid, mutex_paddr);
#endif
                  SyscallSetSuccess( 0 );
                  posix_mutex_lock((void *)mutex_paddr, waiter);
                  /* Wake up waiter */
#ifdef PTHREAD_DEBUG
                  fprintf(stderr, "\taint_pthread_condition_wait: waking up thread 0x%lx \n", waiter->tid);
#endif
                  pmint_thread_unblock(waiter->tid);
               }

             /* Now make the thread wait for condition */
             SyscallSetSuccess( 0 );
             /* block thread */
             pmint_thread_block(pthread->tid);
             ifi->runstate = R_WAIT;
             /* mark that this thread is waiting for the condition */
             posix_condition_add_waiting_thread( (void *) cond_paddr, (void *)mutex_paddr, pthread);
#ifdef PTHREAD_DEBUG
             fprintf(stderr, "\taint_pthread_condition_wait: thread 0x%lx blocked on condition (0x%lx).\n", pthread->tid, cond_paddr); 
#endif
             return picode;
         } else {
            fatal("\taint_pthread_cond_wait: mutex is not locked!\n");
         }
      } else {
          fatal("\taint_pthread_cond_wait: mutex_ptr is NULL\n");
      }
  }else {
      fatal("\taint_pthread_cond_wait: cond_ptr is NULL\n");
  }
  return picode->next;
}

/*
  int pthread_cond_broadcst(
                    pthread_cond_t    *cond);

  Wakes up all threads waiting on the
  specified condition variable.
  The woken up threads try to lock the mutex associated with cond.
*/
OP(aint_pthread_condition_broadcast)
{
  int tid = pthread->tid;
  process_ptr process = pthread->process;
  ulong cond_ptr = MapReg(A0);
  unsigned int *cond_paddr = NULL;
  unsigned int *mutex_paddr = NULL;
  if (cond_ptr){
      AINT_THREAD waiter;
      int done = 0;
      cond_paddr = (unsigned int *)addr2phys(pthread->process, cond_ptr, NULL);
#ifdef PTHREAD_DEBUG
      fprintf(stderr, "\taint_pthread_cond_broadcast: thread 0x%lx condition  0x%lx \n", 
              pthread->tid, cond_paddr); 
#endif
      SyscallSetSuccess( 0 );
      mutex_paddr =  posix_condition_get_mutex_key((void *) cond_paddr);
      posix_condition_broadcasted( (void *) cond_paddr);
      while(!done) {
          waiter = posix_condition_get_next_waiting_thread( (void *) cond_paddr);
          if(waiter != NULL) {
               int locked = 0;
              /* The waiter tries to lock mutex */

               locked  = is_posix_mutex_locked((void *)mutex_paddr);
               if(locked){
                    /* The thread continues to be blocked : earlier it was
                        blocked on the condition now it blocks on the mutex*/
                    /* Mark that this thread is waiting for the mutex */
                    posix_mutex_add_waiting_thread( (void *) mutex_paddr, waiter);
#ifdef PTHREAD_DEBUG
                    fprintf(stderr, "\taint_pthread_condition_broadcast: thread 0x%lx was blocked on condition (0x%lx) is now blocked on mutex (0x%lx).\n", waiter->tid, cond_paddr, mutex_paddr); 
#endif
                    SyscallSetSuccess( 0 );
               }else{
                    AINT_THREAD owner;
                    owner = waiter;
#ifdef PTHREAD_DEBUG
                    fprintf(stderr, "\taint_pthread_condition_broadcast: thread 0x%lx locking mutex (0x%lx).\n", owner->tid, mutex_paddr); 
#endif
                    posix_mutex_lock((void *)mutex_paddr, owner);
                    /* Wake up waiter */
#ifdef PTHREAD_DEBUG
                    fprintf(stderr, "\taint_pthread_condition_broadcast: waking up thread 0x%lx \n", waiter->tid);
#endif
                    pmint_thread_unblock(waiter->tid);
                }
          } else {
               /* no more waiting threads */
               done = 1;
          }
     }
  }else {
      fatal("\taint_pthread_cond_broadcast: cond_ptr is NULL\n");
  }
  return picode->next;
}


/*
  int pthread_cond_signal(
                    pthread_cond_t    *cond);

  Wakes at least one thread that is waiting on the
  specified condition variable.
  The woken up thread tries to lock the mutex associated with cond.
*/

OP(aint_pthread_condition_signal)
{
  int tid = pthread->tid;
  process_ptr process = pthread->process;
  ulong cond_ptr = MapReg(A0);
  unsigned int *cond_paddr = NULL;
  unsigned int *mutex_paddr = NULL;
  if (cond_ptr){
      AINT_THREAD waiter;
      cond_paddr = (unsigned int *)addr2phys(pthread->process, cond_ptr, NULL);
#ifdef PTHREAD_DEBUG
      fprintf(stderr, "\taint_pthread_cond_signal: thread 0x%lx condition  0x%lx \n", 
              pthread->tid, cond_paddr); 
#endif
      SyscallSetSuccess( 0 );
      mutex_paddr =  posix_condition_get_mutex_key((void *) cond_paddr);
      posix_condition_signalled( (void *) cond_paddr);
      waiter = posix_condition_get_next_waiting_thread( (void *) cond_paddr);
      if(waiter != NULL) {
           int locked = 0;
          /* The waiter tries to lock mutex */

           locked  = is_posix_mutex_locked((void *)mutex_paddr);
           if(locked){
                SyscallSetSuccess( 0 );
                /* The thread continues to be blocked : earlier it was
                    blocked on the condition now it blocks on the mutex*/
                /* Mark that this thread is waiting for the mutex */
                posix_mutex_add_waiting_thread( (void *) mutex_paddr, waiter);
#ifdef PTHREAD_DEBUG
                fprintf(stderr, "\taint_pthread_condition_signal: thread 0x%lx was blocked on condition (0x%lx) is now blocked on mutex (0x%lx).\n", waiter->tid, cond_paddr, mutex_paddr); 
#endif
                return picode;
           }else{
                AINT_THREAD owner;
                owner = waiter;
#ifdef PTHREAD_DEBUG
                fprintf(stderr, "\taint_pthread_condition_signal: thread 0x%lx locking mutex (0x%lx).\n", owner->tid, mutex_paddr); 
#endif
                posix_mutex_lock((void *)mutex_paddr, owner);
                /* Wake up waiter */
#ifdef PTHREAD_DEBUG
                fprintf(stderr, "\taint_pthread_condition_signal: waking up thread 0x%lx \n", waiter->tid);
#endif
                pmint_thread_unblock(waiter->tid);
            }
      }
  }else {
      fatal("\taint_pthread_cond_signal: cond_ptr is NULL\n");
  }
  return picode->next;
}


OP(aint_request_begin_skipping)
{
  /* request A0 cycles of fast-mode execution */
  pmint_request_begin_skipping(MapReg(A0));
  SyscallSetSuccess( 0 );

  return picode->next;
}

extern int was_ldq_u_count;
OP(aint_request_end_skipping)
{
  /* end fast-mode execution */
  pmint_request_end_skipping();
  was_ldq_u_count = 0;
  SyscallSetSuccess( 0 );

  return picode->next;
}

OP(aint_request_end_simulation)
{
  /* end simulation */
  pmint_request_end_simulation();
  SyscallSetSuccess( 0 );

  return picode->next;
}

OP(aint_record_event)
{
  /* end simulation */
  pmint_record_event(pthread->tid, MapReg(A0), MapReg(A1));
  SyscallSetSuccess( 0 );

  return picode->next;
}

typedef struct {
  ulong vaddr;
  ulong val;
} qsemaphore_t;

static qsemaphore_t qsemaphores[MAX_NTHREADS];

OP(aint_quiesce_if_equal)
{
  int tid = pthread->tid;
  ulong vaddr = MapReg(A0);
  unsigned int *paddr = (unsigned int *)addr2phys(pthread->process, vaddr, NULL);
  /* fatal("aint_quiesce_if_equal is not implemented! \n"); */
  ulong val = MapReg(A1);
#ifdef VERBOSE_AINT_QUIESCE
  fprintf(Aint_output, "aint_quiesce_if_equal: tid=%d vaddr=%lx paddr=%lx *paddr=%d val=%d\n",
	  pthread->tid, vaddr, paddr, (paddr != NULL ? *paddr : 0), val);
#endif
  if (paddr != NULL) {
    if ((*paddr) == val) {
      if (0) pmint_quiesce_thread(pthread->tid, picode, vaddr);
#ifdef VERBOSE_AINT_QUIESCE
      fprintf(Aint_output, "\t blocking thread %d\n", tid);
#endif      
      qsemaphores[tid].vaddr = vaddr;
      qsemaphores[tid].val = val;
      pmint_thread_block(tid);
      ifi->runstate = R_BLOCKED;
      pthread->runstate = R_BLOCKED;
    } else {
      /* proceed */
      qsemaphores[tid].vaddr = -1;
    }
    SyscallSetSuccess( 0 );
  } else {
    SyscallSetError( EINVAL );
  }
  return picode->next;
}

OP(aint_unquiesce_assign)
{
  int tid = pthread->tid;
  ulong vaddr = MapReg(A0);
  unsigned int *paddr = (unsigned *)addr2phys(pthread->process, vaddr, NULL);
  ulong val = MapReg(A1);
  /* fatal("aint_unquiesce_assign is not implemented!\n"); */
#ifdef VERBOSE_AINT_QUIESCE
  fprintf(Aint_output, "aint_unquiesce: tid=%d vaddr=%lx paddr=%lx *paddr=%ld val=%d\n",
	  tid, vaddr, paddr, (paddr != NULL ? *paddr : 0), val);
#endif
  if (paddr != NULL) {
    int otid;
    *paddr = val;
    for (otid = 0; otid < MAX_NTHREADS; otid++) {
      if (qsemaphores[otid].vaddr == vaddr) {
	if (qsemaphores[otid].vaddr != val) {
#ifdef VERBOSE_AINT_QUIESCE
	  fprintf(Aint_output, "\t unblocking thread %d\n", otid);
#endif
	  ifi->runstate = R_RUN;
	  pthread->runstate = R_RUN;
	  pmint_thread_unblock(otid);
	  qsemaphores[otid].vaddr = -1;
	}
      }
    }
    SyscallSetSuccess( 0 );
  } else {
    SyscallSetError( EINVAL );
  }
  return picode->next;
}



const char *syscall_name(int syscall)
{
  switch (syscall) {
  case SYS_syscall: return "SYS_syscall"; break;
  case SYS_exit: return "SYS_exit"; break;
  case SYS_fork: return "SYS_fork"; break;
  case SYS_read: return "SYS_read"; break;
  case SYS_write: return "SYS_write"; break;
  case SYS_old_open: return "SYS_old_open"; break;
  case SYS_close: return "SYS_close"; break;
  case SYS_wait4: return "SYS_wait4"; break;
  case SYS_old_creat: return "SYS_old_creat"; break;
  case SYS_link: return "SYS_link"; break;
  case SYS_unlink: return "SYS_unlink"; break;
  case SYS_execv: return "SYS_execv"; break;
  case SYS_chdir: return "SYS_chdir"; break;
  case SYS_fchdir: return "SYS_fchdir"; break;
  case SYS_mknod: return "SYS_mknod"; break;
  case SYS_chmod: return "SYS_chmod"; break;
  case SYS_chown: return "SYS_chown"; break;
  case SYS_obreak: return "SYS_obreak"; break;
  case SYS_getfsstat: return "SYS_getfsstat"; break;
  case SYS_lseek: return "SYS_lseek"; break;
  case SYS_getpid: return "SYS_getpid"; break;
  case SYS_mount: return "SYS_mount"; break;
  case SYS_unmount: return "SYS_unmount"; break;
  case SYS_setuid: return "SYS_setuid"; break;
  case SYS_getuid: return "SYS_getuid"; break;
  case SYS_exec_with_loader: return "SYS_exec_with_loader"; break;
  case SYS_ptrace: return "SYS_ptrace"; break;
#ifdef	COMPAT_43
  case SYS_nrecvmsg: return "SYS_nrecvmsg"; break;
  case SYS_nsendmsg: return "SYS_nsendmsg"; break;
  case SYS_nrecvfrom: return "SYS_nrecvfrom"; break;
  case SYS_naccept: return "SYS_naccept"; break;
  case SYS_ngetpeername: return "SYS_ngetpeername"; break;
  case SYS_ngetsockname: return "SYS_ngetsockname"; break;
#else
  case SYS_recvmsg: return "SYS_recvmsg"; break;
  case SYS_sendmsg: return "SYS_sendmsg"; break;
  case SYS_recvfrom: return "SYS_recvfrom"; break;
  case SYS_accept: return "SYS_accept"; break;
  case SYS_getpeername: return "SYS_getpeername"; break;
  case SYS_getsockname: return "SYS_getsockname"; break;
#endif
  case SYS_access: return "SYS_access"; break;
  case SYS_chflags: return "SYS_chflags"; break;
  case SYS_fchflags: return "SYS_fchflags"; break;
  case SYS_sync: return "SYS_sync"; break;
  case SYS_kill: return "SYS_kill"; break;
  case SYS_old_stat: return "SYS_old_stat"; break;
  case SYS_setpgid: return "SYS_setpgid"; break;
  case SYS_old_lstat: return "SYS_old_lstat"; break;
  case SYS_dup: return "SYS_dup"; break;
  case SYS_pipe: return "SYS_pipe"; break;
  case SYS_set_program_attributes: return "SYS_set_program_attributes"; break;
  case SYS_profil: return "SYS_profil"; break;
  case SYS_open: return "SYS_open"; break;
  case SYS_getgid: return "SYS_getgid"; break;
  case SYS_sigprocmask: return "SYS_sigprocmask"; break;
  case SYS_getlogin: return "SYS_getlogin"; break;
  case SYS_setlogin: return "SYS_setlogin"; break;
  case SYS_acct: return "SYS_acct"; break;
  case SYS_sigpending: return "SYS_sigpending"; break;
  case SYS_ioctl: return "SYS_ioctl"; break;
  case SYS_reboot: return "SYS_reboot"; break;
  case SYS_revoke: return "SYS_revoke"; break;
  case SYS_symlink: return "SYS_symlink"; break;
  case SYS_readlink: return "SYS_readlink"; break;
  case SYS_execve: return "SYS_execve"; break;
  case SYS_umask: return "SYS_umask"; break;
  case SYS_chroot: return "SYS_chroot"; break;
  case SYS_old_fstat: return "SYS_old_fstat"; break;
  case SYS_getpgrp: return "SYS_getpgrp"; break;
  case SYS_getpagesize: return "SYS_getpagesize"; break;
  case SYS_mremap: return "SYS_mremap"; break;
  case SYS_vfork: return "SYS_vfork"; break;
  case SYS_stat: return "SYS_stat"; break;
  case SYS_lstat: return "SYS_lstat"; break;
  case SYS_sbrk: return "SYS_sbrk"; break;
  case SYS_sstk: return "SYS_sstk"; break;
  case SYS_mmap: return "SYS_mmap"; break;
  case SYS_ovadvise: return "SYS_ovadvise"; break;
  case SYS_munmap: return "SYS_munmap"; break;
  case SYS_mprotect: return "SYS_mprotect"; break;
  case SYS_madvise: return "SYS_madvise"; break;
  case SYS_old_vhangup: return "SYS_old_vhangup"; break;
  case SYS_kmodcall: return "SYS_kmodcall"; break;
  case SYS_mincore: return "SYS_mincore"; break;
  case SYS_getgroups: return "SYS_getgroups"; break;
  case SYS_setgroups: return "SYS_setgroups"; break;
  case SYS_old_getpgrp: return "SYS_old_getpgrp"; break;
  case SYS_setpgrp: return "SYS_setpgrp"; break;
  case SYS_setitimer: return "SYS_setitimer"; break;
  case SYS_old_wait: return "SYS_old_wait"; break;
  case SYS_table: return "SYS_table"; break;
  case SYS_getitimer: return "SYS_getitimer"; break;
  case SYS_gethostname: return "SYS_gethostname"; break;
  case SYS_sethostname: return "SYS_sethostname"; break;
  case SYS_getdtablesize: return "SYS_getdtablesize"; break;
  case SYS_dup2: return "SYS_dup2"; break;
  case SYS_fstat: return "SYS_fstat"; break;
  case SYS_fcntl: return "SYS_fcntl"; break;
  case SYS_select: return "SYS_select"; break;
  case SYS_poll: return "SYS_poll"; break;
  case SYS_fsync: return "SYS_fsync"; break;
  case SYS_setpriority: return "SYS_setpriority"; break;
  case SYS_socket: return "SYS_socket"; break;
  case SYS_connect: return "SYS_connect"; break;
#ifdef	COMPAT_43
  case SYS_accept: return "SYS_accept"; break;
#else
  case SYS_old_accept: return "SYS_old_accept"; break;
#endif
  case SYS_getpriority: return "SYS_getpriority"; break;
#ifdef	COMPAT_43
  case SYS_send: return "SYS_send"; break;
  case SYS_recv: return "SYS_recv"; break;
#else
  case SYS_old_send: return "SYS_old_send"; break;
  case SYS_old_recv: return "SYS_old_recv"; break;
#endif
  case SYS_sigreturn: return "SYS_sigreturn"; break;
  case SYS_bind: return "SYS_bind"; break;
  case SYS_setsockopt: return "SYS_setsockopt"; break;
  case SYS_listen: return "SYS_listen"; break;
  case SYS_plock: return "SYS_plock"; break;
  case SYS_old_sigvec: return "SYS_old_sigvec"; break;
  case SYS_old_sigblock: return "SYS_old_sigblock"; break;
  case SYS_old_sigsetmask: return "SYS_old_sigsetmask"; break;
  case SYS_sigsuspend: return "SYS_sigsuspend"; break;
  case SYS_sigstack: return "SYS_sigstack"; break;
#ifdef	COMPAT_43
  case SYS_recvmsg: return "SYS_recvmsg"; break;
  case SYS_sendmsg: return "SYS_sendmsg"; break;
#else
  case SYS_old_recvmsg: return "SYS_old_recvmsg"; break;
  case SYS_old_sendmsg: return "SYS_old_sendmsg"; break;
#endif
    /* 115 is obsolete vtrace */
  case SYS_gettimeofday: return "SYS_gettimeofday"; break;
  case SYS_getrusage: return "SYS_getrusage"; break;
  case SYS_getsockopt: return "SYS_getsockopt"; break;
  case SYS_readv: return "SYS_readv"; break;
  case SYS_writev: return "SYS_writev"; break;
  case SYS_settimeofday: return "SYS_settimeofday"; break;
  case SYS_fchown: return "SYS_fchown"; break;
  case SYS_fchmod: return "SYS_fchmod"; break;
#ifdef	COMPAT_43
  case SYS_recvfrom: return "SYS_recvfrom"; break;
#else
  case SYS_old_recvfrom: return "SYS_old_recvfrom"; break;
#endif
  case SYS_setreuid: return "SYS_setreuid"; break;
  case SYS_setregid: return "SYS_setregid"; break;
  case SYS_rename: return "SYS_rename"; break;
  case SYS_truncate: return "SYS_truncate"; break;
  case SYS_ftruncate: return "SYS_ftruncate"; break;
  case SYS_flock: return "SYS_flock"; break;
  case SYS_setgid: return "SYS_setgid"; break;
  case SYS_sendto: return "SYS_sendto"; break;
  case SYS_shutdown: return "SYS_shutdown"; break;
  case SYS_socketpair: return "SYS_socketpair"; break;
  case SYS_mkdir: return "SYS_mkdir"; break;
  case SYS_rmdir: return "SYS_rmdir"; break;
  case SYS_utimes: return "SYS_utimes"; break;
    /* 139 is obsolete 4.2 sigreturn */
  case SYS_adjtime: return "SYS_adjtime"; break;
#ifdef	COMPAT_43
  case SYS_getpeername: return "SYS_getpeername"; break;
#else
  case SYS_old_getpeername: return "SYS_old_getpeername"; break;
#endif
  case SYS_gethostid: return "SYS_gethostid"; break;
  case SYS_sethostid: return "SYS_sethostid"; break;
  case SYS_getrlimit: return "SYS_getrlimit"; break;
  case SYS_setrlimit: return "SYS_setrlimit"; break;
  case SYS_old_killpg: return "SYS_old_killpg"; break;
  case SYS_setsid: return "SYS_setsid"; break;
  case SYS_quotactl: return "SYS_quotactl"; break;
  case SYS_oldquota: return "SYS_oldquota"; break;
#ifdef	COMPAT_43
  case SYS_getsockname: return "SYS_getsockname"; break;
#else
  case SYS_old_getsockname: return "SYS_old_getsockname"; break;
#endif
  case SYS_pid_block: return "SYS_pid_block"; break;
  case SYS_pid_unblock: return "SYS_pid_unblock"; break;
  case SYS_sigaction: return "SYS_sigaction"; break;
  case SYS_sigwaitprim: return "SYS_sigwaitprim"; break;
  case SYS_nfssvc: return "SYS_nfssvc"; break;
  case SYS_getdirentries: return "SYS_getdirentries"; break;
  case SYS_statfs: return "SYS_statfs"; break;
  case SYS_fstatfs: return "SYS_fstatfs"; break;
  case SYS_async_daemon: return "SYS_async_daemon"; break;
  case SYS_getfh: return "SYS_getfh"; break;
  case SYS_getdomainname: return "SYS_getdomainname"; break;
  case SYS_setdomainname: return "SYS_setdomainname"; break;
  case SYS_exportfs: return "SYS_exportfs"; break;
  case SYS_alt_plock: return "SYS_alt_plock"; break;
  case SYS_getmnt: return "SYS_getmnt"; break;
  case SYS_alt_sigpending: return "SYS_alt_sigpending"; break;
  case SYS_alt_setsid: return "SYS_alt_setsid"; break;
  case SYS_swapon: return "SYS_swapon"; break;
  case SYS_msgctl: return "SYS_msgctl"; break;
  case SYS_msgget: return "SYS_msgget"; break;
  case SYS_msgrcv: return "SYS_msgrcv"; break;
  case SYS_msgsnd: return "SYS_msgsnd"; break;
  case SYS_semctl: return "SYS_semctl"; break;
  case SYS_semget: return "SYS_semget"; break;
  case SYS_semop: return "SYS_semop"; break;
  case SYS_uname: return "SYS_uname"; break;
  case SYS_lchown: return "SYS_lchown"; break;
  case SYS_shmat: return "SYS_shmat"; break;
  case SYS_shmctl: return "SYS_shmctl"; break;
  case SYS_shmdt: return "SYS_shmdt"; break;
  case SYS_shmget: return "SYS_shmget"; break;
  case SYS_mvalid: return "SYS_mvalid"; break;
  case SYS_getaddressconf: return "SYS_getaddressconf"; break;
  case SYS_msleep: return "SYS_msleep"; break;
  case SYS_mwakeup: return "SYS_mwakeup"; break;
  case SYS_msync: return "SYS_msync"; break;
  case SYS_signal: return "SYS_signal"; break;
  case SYS_utc_gettime: return "SYS_utc_gettime"; break;
  case SYS_utc_adjtime: return "SYS_utc_adjtime"; break;
  case SYS_security: return "SYS_security"; break;
  case SYS_kloadcall: return "SYS_kloadcall"; break;
  case SYS_getpgid: return "SYS_getpgid"; break;
  case SYS_getsid: return "SYS_getsid"; break;
  case SYS_sigaltstack: return "SYS_sigaltstack"; break;
  case SYS_waitid: return "SYS_waitid"; break;
  case SYS_priocntlset: return "SYS_priocntlset"; break;
  case SYS_sigsendset: return "SYS_sigsendset"; break;
  case SYS_set_speculative: return "SYS_set_speculative"; break;
  case SYS_msfs_syscall: return "SYS_msfs_syscall"; break;
  case SYS_sysinfo: return "SYS_sysinfo"; break;
  case SYS_uadmin: return "SYS_uadmin"; break;
  case SYS_fuser: return "SYS_fuser"; break;
  case SYS_proplist_syscall: return "SYS_proplist_syscall"; break;
  case SYS_pathconf: return "SYS_pathconf"; break;
  case SYS_fpathconf: return "SYS_fpathconf"; break;
  case SYS_uswitch: return "SYS_uswitch"; break;
  case SYS_usleep_thread: return "SYS_usleep_thread"; break;
  case SYS_audcntl: return "SYS_audcntl"; break;
  case SYS_audgen: return "SYS_audgen"; break;
  case SYS_sysfs: return "SYS_sysfs"; break;
  case SYS_subsys_info: return "SYS_subsys_info"; break;
  case SYS_getsysinfo: return "SYS_getsysinfo"; break;
  case SYS_setsysinfo: return "SYS_setsysinfo"; break;
  case SYS_aint_pthread_create: return "SYS_pthread_create"; break;
  case SYS_aint_pthread_join: return "SYS_pthread_join"; break;
  case SYS_aint_pthread_exit: return "SYS_pthread_exit"; break;
  case SYS_aint_pthread_self: return "SYS_pthread_self"; break;
  case  SYS_aint_pthread_mutex_init: return  "SYS_aint_pthread_mutex_init"; break;
  case  SYS_aint_pthread_mutex_lock: return  "SYS_aint_pthread_mutex_lock"; break;
  case  SYS_aint_pthread_mutex_unlock: return  "SYS_aint_pthread_mutex_unlock"; break;
  case  SYS_aint_pthread_condition_init: return  "SYS_aint_pthread_condition_init"; break;
  case  SYS_aint_pthread_condition_wait: return  "SYS_aint_pthread_condition_wait"; break;
  case  SYS_aint_pthread_condition_broadcast: return  "SYS_aint_pthread_condition_broadcast"; break;
  case  SYS_aint_pthread_condition_signal: return  "SYS_aint_pthread_condition_signal"; break;

  case SYS_aint_request_begin_skipping: return "SYS_aint_request_begin_skipping"; break;
  case SYS_aint_request_end_skipping: return "SYS_aint_request_end_skipping"; break;
  case SYS_aint_request_end_simulation: return "SYS_aint_request_end_simulation"; break;
  case SYS_aint_quiesce_if_equal: return "SYS_aint_quiesce_if_equal"; break;
  case SYS_aint_unquiesce_assign: return "SYS_aint_unquiesce_assign"; break;
  case SYS_afs_syscall: return "SYS_afs_syscall"; break;
  case SYS_swapctl: return "SYS_swapctl"; break;
  case SYS_memcntl: return "SYS_memcntl"; break;
  default: return "??"; break;
  }
}



void snapshot_subst(FILE *out)
{
  fwrite(qsemaphores, sizeof(qsemaphore_t)*MAX_NTHREADS, 1, out);
}

void restore_subst(FILE *in)
{
  fread(qsemaphores, sizeof(qsemaphore_t)*MAX_NTHREADS, 1, in);
}
