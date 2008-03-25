/*
 * Routines for supporting System V semaphores and shared memory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <errno.h>
#include "icode.h"
#include "aint_syscall.h"
#include "opcodes.h"
#include "globals.h"
#include "ipc.h"
#include "task.h"
#include "protos.h"

static struct sysv_semid_ds Semset[MAX_SEMASET];
/* struct sysv_shm Shmseg[MAX_SHMSEG]; */

int Shm_max_inuse = -1;

static void alloc_semset(struct sysv_semid_ds *  , int  , int  , int );
static void free_semset(struct sysv_semid_ds * );


/* ARGSUSED */
OP(aint_semget)
{
    int key, nsems, flags;
    int i;

    key =  MapReg(A0);
    nsems = MapReg(A1);
    flags = MapReg(A2);
#ifdef DEBUG_VERBOSE
    printf("mint_semget( key = %d, nsems = %d, flags = 0x%x )\n",
           key, nsems, flags);
#endif

    if (key == 0) {
        /* search for a free slot */
        for (i = 0; i < MAX_SEMASET; i++)
            if (Semset[i].inuse == 0)
                break;

        /* out of semaphore sets, return an error */
        if (i == MAX_SEMASET) {
            *pthread->perrno = ENOSPC;
	    SyscallSetError( ENOSPC );
            return picode->next;
        }

        /* found a free slot */
        alloc_semset(&Semset[i], key, nsems, pthread->tid);

        /* return the semid */
        SyscallSetSuccess( i );
        return picode->next;
    }
    
    /* search for a matching key */
    for (i = 0; i < MAX_SEMASET; i++)
        if (Semset[i].sem_perm.key == key)
            break;
    if (Semset[i].sem_perm.key == key) {
        /* if they wanted a new exclusive semaphore, then it's an error */
        if ((flags & (IPC_EXCL | IPC_CREAT)) == (IPC_EXCL | IPC_CREAT)) {
            *pthread->perrno = EEXIST;
            SyscallSetError( EEXIST );
            return picode->next;
        }
        /* return the semid */
        SyscallSetSuccess( i );
        return picode->next;
    }

    /* semid matching key not found */
    if (flags & IPC_CREAT) {
        /* search for a free slot */
        for (i = 0; i < MAX_SEMASET; i++)
            if (Semset[i].inuse == 0)
                break;

        /* out of semaphore sets, return an error */
        if (i == MAX_SEMASET) {
            *pthread->perrno = ENOSPC;
            SyscallSetError( ENOSPC );
            return picode->next;
        }

        /* found a free slot */
        alloc_semset(&Semset[i], key, nsems, pthread->tid);

        /* return the semid */
        SyscallSetSuccess( i );
        return picode->next;
    }

    /* no match and IPC_CREAT not set, so it's an error */
    *pthread->perrno = ENOENT;
    SyscallSetError( ENOENT );
    return picode->next;
}

static void
alloc_semset(struct sysv_semid_ds *psemset, int key, int nsems, int pid)
{
    int i;
    struct sysv_sem *psem;

    psemset->inuse = 1;
    psemset->sem_perm.key = key;
    psemset->sem_nsems = nsems;
    psem = (struct sysv_sem *) malloc(nsems * sizeof(struct sysv_sem));
    if (psem == NULL)
        fatal("alloc_semset: cannot allocate 0x%x bytes for semaphores.\n",
              nsems * sizeof(struct sysv_sem));
    psemset->sem_base = psem;

    for (i = 0; i < nsems; i++) {
        psem[i].semval = 0;
        psem[i].sempid = pid;
        psem[i].semncnt = 0;
        psem[i].semzcnt = 0;
        INLINE_INIT_Q(&psem[i].nwait_q);
        INLINE_INIT_Q(&psem[i].zwait_q);
    }
}

static void
free_semset(struct sysv_semid_ds *psemset)
{
    psemset->inuse = 0;
    psemset->sem_perm.key = 0;
    free(psemset->sem_base);
}

void
semcpin(struct sysv_semid_ds *psemset, struct sysv_semid_ds *buf)
{
    int mode;

    psemset->sem_perm.uid = buf->sem_perm.uid;
    psemset->sem_perm.gid = buf->sem_perm.gid;
    
    mode = buf->sem_perm.mode & 0777;
    psemset->sem_perm.mode = (psemset->sem_perm.mode & ~0777) | mode;
}

void
semcpout(struct sysv_semid_ds *buf, struct sysv_semid_ds *psemset)
{
    buf->sem_perm.uid = psemset->sem_perm.uid;
    buf->sem_perm.gid = psemset->sem_perm.gid;
    buf->sem_perm.cuid = psemset->sem_perm.cuid;
    buf->sem_perm.cgid = psemset->sem_perm.cgid;
    buf->sem_perm.mode = psemset->sem_perm.mode & 0777;
    buf->sem_perm.seq = psemset->sem_perm.seq;
    buf->sem_perm.key = psemset->sem_perm.key;
    buf->sem_nsems = psemset->sem_nsems;
    buf->sem_otime = psemset->sem_otime;
    buf->sem_ctime = psemset->sem_ctime;
}

OP(aint_semop)
{
    int i, sem_num, sem_op, sem_flg;
    int semid, r5, nsops;
    struct sembuf *sops;
    struct sysv_semid_ds *psemset;
    struct sysv_sem *psem;
    thread_ptr pthr, tnext;
    aint_time_t duration;
    event_ptr pevent;

    semid = MapReg(A0);
    r5 = MapReg(A1);
    nsops = MapReg(A2);
#ifdef DEBUG_VERBOSE
    printf("mint_semop( semid = %d, sops = 0x%x, nsops = %d )\n",
           semid, r5, nsops);
#endif

    sops = (struct sembuf *) (long) MAP(r5);
    psemset = &Semset[semid];

    /* First loop through all the semaphores to check that all requested
     * operations can be performed.
     */
    for (i = 0; i < nsops; i++) {
        sem_num = sops[i].sem_num;
        sem_op = sops[i].sem_op;
        sem_flg = sops[i].sem_flg;
        if (sem_flg & SEM_UNDO)
            fatal("mint_semop: SEM_UNDO not supported yet.\n");
        psem = &psemset->sem_base[sem_num];
        if (sem_op < 0) {
            if (psem->semval < -sem_op) {
                if (sem_flg & IPC_NOWAIT) {
                  *pthread->perrno = EAGAIN;
                  SyscallSetError( *pthread->perrno );
		  return picode->next;
                }
                /* need to suspend this process */
                pthread->semval = -sem_op;
                psem->semncnt++;
                block_thr(&psem->nwait_q, pthread);
                /* return to this routine to try the semaphores again */
                return picode;
            }
        } else if (sem_op == 0) {
            if (psem->semval) {
                if (sem_flg & IPC_NOWAIT) {
                    *pthread->perrno = EAGAIN;
                    SyscallSetError( *pthread->perrno );
                    return picode->next;
                }
                /* need to suspend this process */
                psem->semzcnt++;
                block_thr(&psem->zwait_q, pthread);
                /* return to this routine to try the semaphores again */
                return picode;
            }
        }
    }

    /* Loop through the operations again, this time performing them,
     * since we know that none will block.
     */
    for (i = 0; i < nsops; i++) {
        sem_num = sops[i].sem_num;
        sem_op = sops[i].sem_op;
        sem_flg = sops[i].sem_flg;
        psem = &psemset->sem_base[sem_num];
        /* need to check for blocked processes */
        if (sem_op < 0)
            psem->semval += sem_op;
        else if (sem_op > 0) {
            psem->semval += sem_op;
            /* If there are processes waiting for this semaphore,
             * we need to check if they should be awakened.
             */
            if (psem->semval > 0 && psem->semncnt) {
                /* Wake up everyone waiting for this to be large enough.
                 * We don't know who will be able to proceed since they
                 * may have to atomically change other semaphores, so we
                 * wake them all up and let them fight it out.
                 */
                pthr = (thread_ptr) psem->nwait_q.next;
                while (pthr != (thread_ptr) &psem->nwait_q) {
                    tnext = (thread_ptr) pthr->next;
                    if (pthr->runstate != R_BLOCKED)
                        fatal("mint_semop: runstate (%d) not R_BLOCKED for thread %d\n",
                              pthr->runstate, pthr->tid);
                    if (psem->semval >= pthr->semval) {
                        /* wake up this thread */
                        wakeup_thr(pthr);
                        duration = pthread->time + picode_cycles(picode) - pthr->time;
                        if (duration < 0)
                            fatal("mint_usvsema: negative wait duration (%.1f) at time %.0f\n",
                                  duration, pthread->time + picode_cycles(picode));
                        pthr->time += duration;
			fatal("this is not implemented");
                        psem->semncnt--;
                    }
                    pthr = tnext;
                }
            }
        }

        /* If this semaphore is now zero, and there are waiting processes,
         * then wake them up.
         */
        if (psem->semval == 0 || psem->semzcnt) {
            /* wake up everyone waiting for this to be zero */
            for (i = 0; i < psem->semzcnt; i++) {
                pthr = unblock_thr((thread_ptr) &psem->zwait_q);
                duration = pthread->time + picode_cycles(picode) - pthr->time;
                if (duration < 0)
                    fatal("mint_usvsema: negative wait duration (%.1f) at time %.0f\n",
                          duration, pthread->time + picode_cycles(picode));
                pthr->time += duration;
		fatal("event_unblock_list not implemented");
                pevent->duration = duration;
                pevent->utype = E_PSEMA_ATTEMPT;
            }
            psem->semzcnt = 0;
        }
    }

    SyscallSetSuccess( 0 );
    return picode->next;
}

/* ARGSUSED */
OP(aint_semctl)
{
    int i;
    int semid, semnum, cmd;
    long arg;			/* really "union semun" */
    ushort *array;
    /* struct semid_ds *axpbuf; */
    struct sysv_semid_ds *buf;
    struct sysv_semid_ds *psemset;
    struct sysv_sem *psem;

    semid = MapReg(A0);
    semnum = MapReg(A1);
    cmd = MapReg(A2);
    arg = MapReg(A3);
#ifdef DEBUG_VERBOSE
    printf("mint_semctl( semid = %d, semnum = %d, cmd = 0x%x, arg = 0x%x )\n",
           semid, semnum, cmd, arg);
#endif

    /* check for invalid semid */
    if (semid >= MAX_SEMASET || Semset[semid].inuse == 0) {
        *pthread->perrno = EINVAL;
        SyscallSetError( *pthread->perrno );
        return picode->next;
    }
    psemset = &Semset[semid];
    if (semnum < 0 || semnum >= psemset->sem_nsems) {
        *pthread->perrno = EINVAL;
        SyscallSetError( *pthread->perrno );
        return picode->next;
    }
    psem = &psemset->sem_base[semnum];

    switch (cmd) {
      case GETVAL:
        SyscallSetSuccess( psem->semval );
        break;
      case SETVAL:
        psem->semval = (int) arg;
        SyscallSetSuccess( 0 );
        break;
      case GETPID:
        SyscallSetSuccess( psem->sempid );
        break;
      case GETNCNT:
        SyscallSetSuccess( psem->semncnt );
        break;
      case GETZCNT:
        SyscallSetSuccess( psem->semzcnt );
        break;
      case GETALL:
        array = (ushort *) (long) MAP(arg);
        for (i = 0; i < psemset->sem_nsems; i++)
            array[i] = psemset->sem_base[i].semval;
        SyscallSetSuccess( 0 );
        break;
      case SETALL:
        array = (ushort *) (long) MAP(arg);
        for (i = 0; i < psemset->sem_nsems; i++)
            psemset->sem_base[i].semval = array[i];
        SyscallSetSuccess( 0 );
        break;
      case IPC_STAT:
        buf = (struct sysv_semid_ds *) (long) MAP(arg);
        semcpout(buf, psemset);
        SyscallSetSuccess( 0 );
        break;
      case IPC_SET:
        buf = (struct sysv_semid_ds *) (long) MAP(arg);
        semcpin(psemset, buf);
        SyscallSetSuccess( 0 );
        break;
      case IPC_RMID:
        /* Need to check here if any process is blocked on a semaphore
         * in this set.
         */
        free_semset(psemset);
        SyscallSetSuccess( 0 );
        break;
      default:
        *pthread->perrno = EINVAL;
        SyscallSetError( *pthread->perrno );
        break;
    }

    return picode->next;
}

int shmseg_allocate (long key, long size, long flags, long *perrno)
{
    int shmid;

    /* round size up to a page boundary */
    size = BASE2ROUNDUP(size, TB_PAGESIZE);

    /* If a non-zero key was specified, then we have to search for a
     * segment with a matching key.
     */
    if (key) {
        /* search for a matching key */
        for (shmid = 0; shmid < MAX_SHMSEG; shmid++) {
            if (Shmseg[shmid].shm_perm.key == key) {
                /* If this process requested a new exclusive copy,
                 * then it's an error.
                 */
                if ((flags & (IPC_EXCL | IPC_CREAT)) == (IPC_EXCL | IPC_CREAT)) {
                    /* return an error */
		  *perrno = EEXIST;
		  return -1;
                } else {
		  *perrno = 0;
		  return shmid;
                }
            }
        }

        /* if we get here then we did not find a matching key */
        if ((flags & IPC_CREAT) == 0) {
	    *perrno = ENOENT;
	    return -1;
        }
    }

    /* search for an unused shmid */
    for (shmid = 0; shmid < MAX_SHMSEG; shmid++)
        if (Shmseg[shmid].inuse == 0)
            break;

    /* Out of shared memory segments, return an error */
    if (shmid == MAX_SHMSEG) {
        *perrno = ENOSPC;
        return -1;
    }
    /* the return value is the shmid */
    *perrno = 0;

    if (shmid > Shm_max_inuse)
        Shm_max_inuse = shmid;

    /* make sure there is enough space */
    if (Shmem_end + size > Shmem_start + Shmem_size)
        fatal("aint_shmget: not enough shared memory (requested size = 0x%x, used = 0x%x)\n",
              size, Shmem_end - Shmem_start);

    /* initialize the shared memory segment */

    Shmseg[shmid].paddr = NULL;
    Shmseg[shmid].size = size;
    Shmseg[shmid].inuse = 1;
    Shmseg[shmid].shm_perm.key = key;

    /* Update size of used shared-space */
    Shmem_end += size;

    return shmid;
}

/* ARGSUSED */
OP(aint_shmget)
{
    int shmid;
    long key, size, flags, thread_errno;

    key =  MapReg(A0);
    size = MapReg(A1);
    flags = MapReg(A2);

    if ((shmid = shmseg_allocate(key, size, flags, &thread_errno)) != -1) {
      SyscallSetSuccess( shmid );
    } else {
      SyscallSetError( thread_errno );
    }
    return picode->next;
}

/*
 * returns: 0 if addr is not within an allocated shared memory segment
 *          1 if addr is within an allocated shared memory segment
 */
int
is_valid(long addr)
{
  /*
    int i;

    for (i = 0; i <= Shm_max_inuse; i++)
        if (Shmseg[i].inuse && Shmseg[i].real_seg)
            if (addr >= Shmseg[i].addr && addr < Shmseg[i].addr + Shmseg[i].size)
                return 1;
    return 0;
    */
  return(1);
}


ulong shmseg_attach(process_ptr process, int shmid, ulong addr, ulong flags, long *perrno)
{
    int size;
    struct Shm_ds *new_shm_ds;

    /* check that the shmid is a valid index */
    if (shmid < 0 || shmid > MAX_SHMSEG) {
        *perrno = EINVAL;
        return -1;
    }

    /* check that this shared memory segment has been allocated */
    if (Shmseg[shmid].inuse == 0) {
        *perrno = EINVAL;
        return -1;
    }

    size = Shmseg[shmid].size;
    assert(size == BASE2ROUNDDOWN(size, TB_PAGESIZE));
    if (Shmseg[shmid].paddr == NULL) {
      /* if we're the first ones to attach to this shmseg, then allocate its pages */
      Shmseg[shmid].paddr = allocate_pages(size);
      if (Shmseg[shmid].paddr == (char *)-1)
	fatal("shmseg_attach failed to allocate physical page\n");
    }

    /* If addr is zero, get the next valid addr: Unsp_Shmat_current upto Unsp_Shmat_current+size */
    if (addr == 0) {
      process->Unsp_Shmat_Current = 
	BASE2ROUNDUP(process->Unsp_Shmat_Current, TB_PAGESIZE);
      addr = process->Unsp_Shmat_Current;
      process->Unsp_Shmat_Current += size;
      if (process->Unsp_Shmat_Current >= UNSP_SHMAT_END) { 
	/* Cannot attach - full */
	*perrno = ENOMEM;
	addr = -1;
      } else {
	*perrno = 0;
      }
    } else {
      addr = BASE2ROUNDDOWN(addr, TB_PAGESIZE);
      *perrno = 0;
    }
    if (*perrno == 0) {
      /* Plug in the address in the per-process table */
      new_shm_ds = (struct Shm_ds *) malloc(sizeof(struct Shm_ds));
      if (new_shm_ds == NULL)
	fatal("shmseg_attach could not allocate space for new_shm_ds\n");
      new_shm_ds->shmid = shmid;
      new_shm_ds->size = size;
      new_shm_ds->addr = addr;
      /* Enter new_shm_ds in the per-process shared-region descriptor */
      new_shm_ds->next = process->Shmem_regions;
      process->Shmem_regions = new_shm_ds;

      /* success: map the segment */
      process_add_segment(process, addr, size, PROT_READ|PROT_WRITE, MAP_SHARED);
    }
    return addr;
}


/* ARGSUSED */
OP(aint_shmat)
{
  int shmid, flags;
  long addr;
  process_ptr process;
  long thread_errno;

  process = pthread->process;

  shmid =  MapReg(A0);
  addr = MapReg(A1);
  flags = MapReg(A2);

  if ((addr = shmseg_attach(process, shmid, addr, flags, &thread_errno)) != -1) {
    if (Verbose)
      fprintf(Aint_output, "AINT_SHMAT: pid=%d, shmid=%x, vaddr=%lx, paddr=%lx\n",
	      process->pid, shmid, addr, PMAP(addr));
    SyscallSetSuccess( addr );
  } else {
    SyscallSetError( thread_errno );
  }
  return picode->next;
}



/* ARGSUSED */
OP(aint_shmdt)
{
#ifdef DEBUG_VERBOSE
    int addr;

    addr = MapReg(A0);
    printf("aint_shmdt( addr = 0x%x )\n", addr);
#endif

    SyscallSetSuccess( 0 );
    return picode->next;
}

/* ARGSUSED */
OP(aint_shmctl)
{
#ifdef DEBUG_VERBOSE
    int shmid, cmd;
    struct shmid_ds *buf;

    shmid = REGNUM(4);
    cmd = REGNUM(5);
    buf = (struct shmid_ds *) REGNUM(6);
    printf("aint_shmctl( shmid = %d, cmd = 0x%x, buf = 0x%x )\n",
           shmid, cmd, buf);
#endif

    SyscallSetSuccess( 0 );
    return picode->next;
}
