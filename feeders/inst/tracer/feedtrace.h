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
 
//
// Author:  Steven Wallace
//

#include <unistd.h>

#include "syntax.h"
#include "mpipe.h"

#ifdef NO_ASIM_THREAD
#define ASIM_THREAD int
#endif

#if 1
#define LOOKAHEAD       1024    /* # instr to retrieve from trace/aint */
#define INSTR_SAFETY    1024    /* # instr required to keep around in case
                                   need to backup */
#define BUFFER_SIZE  (LOOKAHEAD + INSTR_SAFETY)  /* total buffer size */
#else
#define BUFFER_SIZE  256
#endif

#define TRACE_WRONGPATH_ID  UINT64_MAX

/* The size of the inst_info structure should be a power of 2 for speed,
   otherwise, change the GetIdentifier() to use the byte offset of the stream
   instead of the dynamic instruction stream count. */
struct inst_info {
    UINT64 vpc;
    UINT64 opcode;
    UINT64 vea;
    UINT64 info;
};

typedef class trace_buffer_class {

private:
    PIPE_BUFFER_CLASS    pbuf;

    bool ended;
    ASIM_THREAD thread;

    static const UINT64  isize = sizeof(struct inst_info);
    struct inst_info *CurInst()  const
        { return (struct inst_info *)pbuf.GetReadPtr(); }

public:
    void   InitTracer (UINT32, UINT32, char **);
    void   StartTracer();
    void   NextInstr();
    void   PrevInstr()         { pbuf.DecReadPtr(isize); }
    void   Backup(UINT64 identifier) {  pbuf.SetReadPos(identifier * isize); }
    void   CommitInstr(UINT64 ident) {  pbuf.SetDonePos((ident+1) * isize); }
    UINT64 GetIdentifier()     const { return pbuf.GetReadPos() / isize; }

    UINT64 VirtualPc()         const { return CurInst()->vpc; }
    UINT32 Opcode()            const { return CurInst()->opcode; }
    UINT64 VirtualEffAddress() const { return CurInst()->vea; }
    bool   Eof()               const { return pbuf.IsEOF(); }
    UINT32 Trap() const { if(CurInst()->info < 0x4000) return CurInst()->info; return 0; }
    bool&  Ended()             { return(ended); }
    ASIM_THREAD&  Thread()     { return(thread); }
} TRACE_BUFFER_CLASS, *TRACE_BUFFER;


void
TRACE_BUFFER_CLASS::InitTracer (UINT32 tid, UINT32 fd, char **buffer)
{
    ended = false;
    thread = NULL;

    *buffer += pbuf.InitPipe(*buffer, BUFFER_SIZE * isize);
}

void
TRACE_BUFFER_CLASS::StartTracer()
{
    while(!pbuf.GetMaxReadSize(sizeof(struct inst_info)) && !pbuf.IsEOF()) ;
}

void
TRACE_BUFFER_CLASS::NextInstr()
{
    pbuf.IncReadPtr(sizeof(struct inst_info)) ;
    while(!pbuf.GetMaxReadSize(sizeof(struct inst_info)) && !pbuf.IsEOF()) ;
}




