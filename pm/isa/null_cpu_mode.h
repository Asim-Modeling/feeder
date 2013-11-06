/*****************************************************************************
 *
 *
 * @author Sailashri Parthasarathy (based on Prachi P. Bhagwat's implementation)
 *
 * Copyright (C) 2002-2006 Intel Corporation
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
 *
 *****************************************************************************/

#ifndef _CPU_MODE_H_
#define _CPU_MODE_H_

//
// The CPU_MODE enumeration is defined here in a simple file so it can be
// included by the SoftSDV stub code.  The SoftSDV stub does not get the
// full Asim environment.
//

enum CPU_MODE_TYPE
{
  CPU_MODE_UNKNOWN,
  CPU_MODE_REAL,
  CPU_MODE_V8086,
  CPU_MODE_PROTECTED_16,
  CPU_MODE_PROTECTED_32,
  CPU_MODE_LONG_COMPATIBLE_16,
  CPU_MODE_LONG_COMPATIBLE_32,
  CPU_MODE_LONG_64,
  CPU_MODE_SMM, 
};

#endif /* _CPU_MODE_H_ */
