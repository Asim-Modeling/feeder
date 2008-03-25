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

typedef unsigned int bitvector_t;

/* define bit manipulation macros */
#define SETVALID(BVECTOR, PID)	  ((BVECTOR) |= (1 << (PID)))
#define SETEXCL(BVECTOR, PID)	  ((BVECTOR) = (1 << (PID)))
#define CLRVALID(BVECTOR, PID)	  ((BVECTOR) &= ~(1 << (PID)))
#define CLRALLBITS(BVECTOR)	  (BVECTOR = 0)
#define CLROTHERS(BVECTOR, PID)	  (BVECTOR &= (1 << (PID)))
#define ISVALID(BVECTOR, PID)	  ((BVECTOR) & (1 << (PID)))
#define ISEXCL(BVECTOR, PID)	  ((BVECTOR) == (1 << (PID)))
#define ISOTHER(BVECTOR, PID)	  ((BVECTOR) & ~(1 << (PID)))
#define ISANY(BVECTOR)		  (BVECTOR)
#define SETOR(DEST, SRC)	  (DEST |= SRC)
#define SETAND(DEST, SRC)	  (DEST &= SRC)
#define SETOR2(DEST, SRC1, SRC2)  (DEST = (SRC1) | (SRC2))
#define SETAND2(DEST, SRC1, SRC2) (DEST = (SRC1) & (SRC2))
#define SETEQUAL(DEST, SRC)	  (DEST = SRC)
#define SETNOT(DEST, SRC)	  (DEST = ~(SRC))
#define NUM_1BITS(BVECTOR)	  (count_1bits(BVECTOR))
#define BITPOS(BVECTOR)		  (bitpos(BVECTOR))
#define LEAST1(BVECTOR)		  (least1(BVECTOR))
#define MOST1(BVECTOR)		  (most1(BVECTOR))

/* Federico Ardanaz notes:
 * I just add a LONG version of some of the above macros. If you want
 * to manage long/double variables bits you must use this version.    
*/
#define SETVALID_L(BVECTOR, PID)    ((BVECTOR) |=   (1L << (PID)))
#define CLRVALID_L(BVECTOR, PID)    ((BVECTOR) &=  ~(1L << (PID)))
#define ISVALID_L(BVECTOR, PID)	    ((BVECTOR) &    (1L << (PID)))
