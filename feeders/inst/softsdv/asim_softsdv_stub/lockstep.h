/**************************************************************************
 * Copyright (C) 2006 Intel Corporation
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
 **************************************************************************/

/**
 * @file lockstep.h
 * @author Michael Adler
 * @brief Lock-step debugging tool -- compare 2 SoftSDV processes
 */

#include "md5.h"

class ASIM_LOCKSTEP_CLASS
{
  private:
    struct LOCKSTEP_COMM_BUFFER_CLASS
    {
        UINT8 md5_digest[16];
        int ready;
    };

    typedef struct LOCKSTEP_COMM_BUFFER_CLASS *LOCKSTEP_COMM_BUFFER;

  public:
    ASIM_LOCKSTEP_CLASS(void);
    ~ASIM_LOCKSTEP_CLASS();

    // Add instruction data to the running MD5 checksum
    inline void NoteInstrData(const void *data, UINT32 len);

    // Do a lock-step comparison.  Returns true on valid comparison.
    // Resets the running MD5 checksum as a side-effect.
    bool CompareProcesses(void);

  private:
    LOCKSTEP_COMM_BUFFER dataOut;
    LOCKSTEP_COMM_BUFFER dataIn;

    MD5 newInstrsMD5;
};

    
inline void
ASIM_LOCKSTEP_CLASS::NoteInstrData(
    const void *data,
    UINT32 len)
{
    newInstrsMD5.update((const unsigned char *)data, len);
};
