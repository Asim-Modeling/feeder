/*
 * Copyright (C) 2005-2006 Intel Corporation
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
// Author:  Ramon Matas Navarro
//



#ifndef _NULL_FEEDER_H
#define _NULL_FEEDER_H

class NULL_FEEDER_CLASS : public IFEEDER_BASE_CLASS
{
  public:

    NULL_FEEDER_CLASS(): IFEEDER_BASE_CLASS("Null Feeder") {};
    ~NULL_FEEDER_CLASS() {};

    bool Init(UINT32 argc, char **argv, char **envp) { return true; };
    void Done(void) {};
    bool Fetch(IFEEDER_STREAM_HANDLE stream, IADDR_CLASS predicted_pc, ASIM_INST inst) { return false; };
    void Commit(ASIM_INST inst) {};
    void Kill(ASIM_INST inst, bool fetchNext, bool killMe) {};
    bool ITranslate(UINT32 hwcNum, UINT64 va, UINT64& pa) { return false; };
    bool DTranslate(ASIM_INST inst, UINT64 va, UINT64& pa) { return false; };

};

#endif /*_NULL_FEEDER_H*/

