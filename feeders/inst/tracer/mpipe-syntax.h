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


#ifndef _MPIPE_SYNTAX_
#define _MPIPE_SYNTAX_

#ifndef FMT64U
typedef char                INT8;
typedef unsigned char       UINT8;
typedef short               INT16;
typedef unsigned short      UINT16;
typedef int                 INT32;
typedef unsigned int        UINT32;
typedef int                 BOOL;
#ifdef __FreeBSD__
typedef long long           INT64;
typedef unsigned long long  UINT64;
#define FMT64D              "%qd"
#define FMT32U              "%u"
#else
typedef long                INT64;
typedef unsigned long       UINT64;
#define FMT64D              "%ld"
#define FMT64U              "%lu"
#endif
#define UINT64_MAX (UINT64)-1
#define VOID   void
#define CONST  const
#define NULL   0L
#define CHAR   INT8
#endif  /* syntax not defined */

#ifdef DEBUG
#define DEBUGF fprintf
#define FLUSHF fflush
#else
#define DEBUGF
#define FLUSHF
#endif
#ifndef DOUT
#define DOUT   stderr
#endif /* DEBUG */

#if !(defined(__cplusplus) || defined(__GNUC__))
#define inline
#endif

#endif /* _MPIPE_SYNTAX_ */
