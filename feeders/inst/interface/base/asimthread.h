/*
 * Copyright (C) 2003-2006 Intel Corporation
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

/**
 * @file
 * @author David Goodwin
 * @brief ASIM's thread abstraction for multi-threaded performance
 * models and feeders.
 *
 * @note There is currently too much confusion between thread and tpu
 * in this code, which represents both concepts. This is not flexible
 * enough and will need to be split into two separte concepts for
 * supporting switching of (software) threads between tpus (hardware
 * threads).
 */

#ifndef _ASIMTHREAD_
#define _ASIMTHREAD_

// ASIM core
#include "asim/syntax.h"
#if ENABLE_ALPHA
#  include "asim/alphaops.h"
#endif
#include "asim/disasm.h"
#include "asim/atomic.h"

//
// We need this forward reference BEFORE including instfeeder_interface.h,
// since it includes isa.h and it is needed there...
//
typedef class ASIM_THREAD_CLASS *ASIM_THREAD;

// ASIM public modules -- BAD! in asim-core
#include "asim/provides/instfeeder_interface.h"

// ASIM public modules -- BAD! in asim-core
#include "asim/provides/iaddr.h"

/*
 * ASIM_THREAD
 *
 * A software thread.
 */
#define ASIM_TPU_NONE       UINT32_MAX
#define ASIM_CPU_NONE	    UINT32_MAX

typedef class IFEEDER_BASE_CLASS *IFEEDER_BASE;
class ASIM_THREAD_CLASS
{
  private:
    /*
     * Feeder associated with the thread.
     */
    const IFEEDER_BASE iFeeder;
    const IFEEDER_STREAM_HANDLE iStreamHandle;

    /*
     * uid is required by tcl code.  Nowhere else.  DON'T USE IT!!!
     */
    const UINT32 uid;
    static UID_GEN32 last_uid;

    /*
     * Process id and start pc for the thread.
     */
    IADDR_CLASS spc;

    /*
     * Processing unit that thread is executing on. This field
     * is maintained by the system.
     */
    UINT32 tpu;
    UINT32 cpu; 	

    UINT64 last_retire_time;
        
  public:
    ASIM_THREAD_CLASS(IFEEDER_BASE f,
                      IFEEDER_STREAM_HANDLE h,
                      IADDR_CLASS p)
        : iFeeder(f), iStreamHandle(h),
          uid(last_uid++),
          spc(p),
          tpu(ASIM_TPU_NONE), cpu(ASIM_CPU_NONE),
          last_retire_time(0)
    {};

    /*
     * Accessors...
     */
    IFEEDER_BASE IFeeder(void) const { return iFeeder; }
    IFEEDER_STREAM_HANDLE IStreamHandle(void) const { return iStreamHandle; }
    IADDR_CLASS StartVirtualPc(void) const { return(spc); }
    UINT32 Tpu(void) const { return(tpu); }
    UINT32 Cpu(void) const { return(cpu); }
    UINT64 LastRetireTime(void) const { return(last_retire_time); }
    UINT32 TUid(void) const { return uid; }
    
    /*
     * Modifiers...
     */
    void SetVirtualPc(IADDR_CLASS pc) { spc = pc; }
    void SetTpu(UINT32 t) { tpu = t; }
    void SetCpu(UINT32 c) { cpu = c; }
    void SetLastRetireTime(UINT64 t) { last_retire_time = t; }
};




#endif /* _ASIMTHREAD_ */
