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
 * $Header$
 * $Log$
 * Revision 1.1  2001/11/15 16:09:43  klauser
 * - moving all (instruction) feeders one level down in directory hierarchy
 * - changed atf feeder to more generic trace feeder + atf reader
 *
 * Revision 1.5  2001/03/27 14:11:51  fardanaz
 * several tarantula imporvments
 *
 * Revision 1.4  2001/03/07 18:27:07  fardanaz
 * prefetches under verifier bug fixed
 *
 * Revision 1.3  2000/10/15 15:34:39  espasa
 * CSN-feeder-139
 *
 * Revision 1.2  2000/10/10 19:59:56  rgramunt
 * CSN-feeder-134
 *
 * Revision 1.1  1999/07/02 00:37:01  espasa
 * *** empty log message ***
 *
 * Revision 1.1.1.1  1999/03/05  21:52:43  swallace
 * AINT code that came with the nov-24-98 version of the EV8 simulator
 *
 * Revision 1.7  1997/08/18  14:16:04  jamey
 * store-conditional now updates store-buffer
 *
 * Revision 1.6  1997/04/21  14:00:16  jamey
 * added snapshot/restore capability
 *
 * Revision 1.5  1997/01/13  21:30:47  jamey
 * support for parsing spinloops
 *
 * Revision 1.4  1996/10/23  12:59:34  jamey
 * putting aint on a diet
 *
 * Revision 1.3  1996/09/11  12:40:13  jamey
 * added E_PREFETCH
 *
 * Revision 1.2  1996/08/12  18:48:51  jamey
 * folded in Vegas storebuf fixes
 *
 */
/*
 * Definition of the event structure 
 */

#ifndef EVENT_H
#define EVENT_H

typedef double aint_time_t;

/* a constant greater than any thread's time value */
#define MAXTIME ( ((unsigned ) (~0)) >> 1 )

typedef struct event {
  aint_time_t time;    /* at which the event occurred */
  aint_time_t *cpu_time;    /* accumulated cpu_time of this process */
  aint_time_t duration;    /* time that an unbloxked process spent waiting */
  struct event *next;    /* used in E_UNBLOCK_LIST */
  int tid;
    /* int pid; */
  short type;    /* of instruction */
  long utype;    /* user-defined type */
  long iaddr;    /* instruction address */
  long vaddr;    /* the effective address the traced program sees */
  long paddr;    /* the real address in memory */
  struct event *pevent;    /* to another event structure */
  struct icode *picode;    /* Passed only to sim_inst */
  int size;      /* size of data for read/write */
  long data;     /* data value for read/write */
  void *sptr;    /* used by talk scheduler when a task blocks */
  char *fname;   /* function name if type E_FUNCTION */
  long arg1;     /* args filled in by aint on E_FUNCTION event */
  long arg2;
  long arg3;
  long arg4;
  long rval;    /* return value from malloc() type calls */
  long raddr;    /* return address */
  long raddr2;    /* caller's return address (more useful) */
} event_t, *event_ptr;

/*
 * roger 13/Oct/2000
 *
 * The following types are not used at all in the  ttlaint
 * directory as of Oct/2000:
 *   E_SHARED      0x200
 *   E_USER        0x400
 *   E_UFUNC       0x800 
 * 
 */
#define E_INSTR            0x0
#define E_SPECIAL_MASK    0x3F
#define E_SPECIAL         0x40
#define E_READ            0x80
#define E_WRITE          0x100
/* free                  0x200*/
#define E_GATHER         0x400
#define E_SCATTER        0x800
#define E_FLOAT         0x1000
#define E_BARRIER       0x2000
#define E_LD_L          0x4000
#define E_ST_C          0x8000
#define E_BSR          0x10000
#define E_CONDBR       0x20000
#define E_BR           0x40000
#define E_JMP          0x80000
#define E_JSR         0x100000
#define E_RET         0x200000
#define E_CACHEOP     0x400000
#define E_SYSCALL     0x800000
#define E_PAL_OTHER  0x1000000

/* Some useful ORings of the above */
#define E_MEM_REF  (E_READ|E_WRITE|E_LD_L|E_ST_C)
#define E_LOCK     (E_LD_L|E_ST_C)
#define E_BRANCH   (E_BSR | E_CONDBR | E_BR | E_JMP | E_JSR | E_RET)
#define E_PAL (E_SYSCALL | E_PAL_OTHER)
#define E_GATSCAT  (E_GATHER|E_SCATTER)

/* basic-block info */
/* FIXME: Do these have to be exclusive E_{events} values ? -Vega 07/16/96 */
#define E_BB_FIRST 0x2000000
#define E_BB_LAST  0x4000000

/* conditional mov insts - these read as well as possibly write dest reg */
#define E_CMOV    0x8000000

#define E_PREFETCH 0x10000000

#define E_LITERAL  0x20000000

#define E_SPINLOOP 0x40000000
#define E_WAS_LDQ_U 0x80000000

/* the special types */
#define E_FORK  (E_SPECIAL | 1)
#define E_M_FORK  (E_SPECIAL | 2)
#define E_YIELD  (E_SPECIAL | 3)
#define E_DONE   (E_SPECIAL | 4)
#define E_BLOCK  (E_SPECIAL | 5)
#define E_UNBLOCK (E_SPECIAL | 6)
#define E_LOCK_ATTEMPT (E_SPECIAL | 7)
#define E_LOCK_ACQUIRE (E_SPECIAL | 8)
#define E_LOCK_RELEASE (E_SPECIAL | 9)
#define E_BARRIER_ATTEMPT (E_SPECIAL | 10)
#define E_BARRIER_ACQUIRE (E_SPECIAL | 11)
#define E_PSEMA_ATTEMPT (E_SPECIAL | 12)
#define E_PSEMA_ACQUIRE (E_SPECIAL | 13)
#define E_VSEMA  (E_SPECIAL | 14)
#define E_UNBLOCK_LIST (E_SPECIAL | 15)
#define E_WAIT  (E_SPECIAL | 16)
#define E_EXIT  (E_SPECIAL | 17)
#define E_FUNCTION (E_SPECIAL | 18)
#define E_SPROC (E_SPECIAL | 19)
#define E_KILL  (E_SPECIAL | 20)
#define E_PREFORK (E_SPECIAL | 21)
#define E_MP_CREATE (E_SPECIAL | 22)




#endif
