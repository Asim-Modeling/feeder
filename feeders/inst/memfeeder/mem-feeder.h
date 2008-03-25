/*
 * **********************************************************************
 * Copyright (C) 2004-2006 Intel Corporation
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
// Author:  Ramon Matas
//

#ifndef _MEMORY_FEEDER_H
#define _MEMORY_FEEDER_H

// ASIM core
#include "asim/syntax.h"

// ASIM public modules
#include "asim/provides/instfeeder_interface.h"

#include <iostream>

class MEMORY_FEEDER_CLASS : public IFEEDER_BASE_CLASS
{
  public:
    
    //
    // STREAM_HANDLE is the internal representation of an IFEEDER_STREAM_HANDLE.
    // STREAM_HANDLE is one less than IFEEDER_STREAM_HANDLE so that the former
    // is 0 based and the latter has NULL meaning all streams.
    //
    class STREAM_HANDLE
    {
      public:
        STREAM_HANDLE(UINT32 h) :
            handle(h)
        {};

        STREAM_HANDLE(IFEEDER_STREAM_HANDLE h)
        {
            ASSERTX(h != NULL);
            handle = (UINT32) h - 1;
        };

        UINT32 Handle(void)
        {
            return handle;
        };

        operator UINT32()
        {
            return handle;
        };

        operator IFEEDER_STREAM_HANDLE()
        {
            return (IFEEDER_STREAM_HANDLE) (handle + 1);
        };

      private:
        UINT32 handle;
    };


  public:
    
    MEMORY_FEEDER_CLASS();
    ~MEMORY_FEEDER_CLASS() {};

    virtual bool Init(UINT32 argc, char **argv, char **envp);
    
    virtual void Done(void);

    virtual bool Fetch(IFEEDER_STREAM_HANDLE stream,
                       IADDR_CLASS predicted_pc,
                       ASIM_INST inst);
    
    virtual void Commit(ASIM_INST inst);
    
    virtual void Kill(ASIM_INST inst,
                      bool fetchNext,
                      bool killMe);

    virtual bool ITranslate(UINT32 hwcNum, UINT64 va, UINT64& pa);
    virtual bool DTranslate(ASIM_INST inst, UINT64 va, UINT64& pa);
    
    bool WarmUp(IFEEDER_STREAM_HANDLE stream, WARMUP_INFO warmup);

  private:
    
    UINT32 get_stream_id(ASIM_INST inst);
        
};

#endif /*_MEMORY_FEEDER_H*/

