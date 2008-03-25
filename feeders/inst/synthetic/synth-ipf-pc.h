/*
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

/* synth-ipf-pc.h */
/* Mark Charney   <mark.charney@intel.com> */
/*$Id: synth-ipf-pc.h 799 2006-10-31 12:27:52Z cjbeckma $ */


#ifndef _SYNTH_IPF_PC_H_
# define _SYNTH_IPF_PC_H_

class IPF_PC_CLASS
{
  public:
    UINT64 pc;
    UINT32 syllable;

    IPF_PC_CLASS(UINT64 arg_pc=0, UINT32 arg_syllable=0) //CONS
        : pc(arg_pc), syllable(arg_syllable)
    {
    }

    inline void
    inc(void)
    {
        syllable++;
        if (syllable==SYLLABLES_PER_BUNDLE)
        {
            syllable=0;
            pc+= 16;
        }
    }
    
    friend ostream& operator<<(ostream& o, const IPF_PC_CLASS& x);
};

ostream& operator<<(ostream& o, const IPF_PC_CLASS& x)
{
    o << hex << x.pc << dec << "." << x.syllable;
    return o;
}
    
#endif
