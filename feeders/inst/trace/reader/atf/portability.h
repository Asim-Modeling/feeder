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

#ifndef	_PORTABILITY_H
#define	_PORTABILITY_H

// ASIM core
#include "asim/syntax.h"

//#ifndef UINT64_MAX
//#define UINT64_MAX 0xffffffffffffffffUL
//#endif

#if defined(WIN32) || defined(_NTDDK_) || defined(_WINDOWS_) || defined(WINDOWS)
#define _OS_WINNT
#endif

#ifdef	_OS_WINNT

/*
 * WinNT / Alpha definitions
 *
 */

/*
 * includes
 *
 */

#ifndef _NTDDK_
#ifndef _WINDOWS_
#include <windows.h>
#endif
#endif

/* 
 * integer types
 *
 * WinNT/Alpha integer types have the same size as UNIX/Alpha types,
 * except for "long", which is 32 bits on NT and 64 bits on Unix.
 * In addition, NT does not define uchar, ushort, uint, or ulong.
 * 
 */
#define UINT_MAX 0xffffffffU
typedef unsigned __int64 uint64;
typedef __int64 int64;
typedef unsigned int uint;

#define EXPORT __declspec( dllexport )

/*
 * constants 
 *
 */

/* processor types (from Digital Unix <machine/cpuconf.h>) */
#define	EV4_CPU		(2)
#define	EV5_CPU		(5)
#define	EV45_CPU	(6)
#define	EV56_CPU	(7)

/* printf/scanf formatting for 64 bit integers */
#define	FMT64		"I64"

/* open flags */
#define	FOPEN_BINARY	"b"

/*
 * macros
 *
 */

#define	bzero(dest,size)	memset((void *) (dest), 0, (size))
#define	strcasecmp(s1,s2)	stricmp((s1), (s2))
#define	strncasecmp(s1,s2,n)	strnicmp((s1), (s2), (n))
#define	sleep(sec)		Sleep((sec) * 1000)
#define	S_ISREG(m)		(((m) & _S_IFMT) == _S_IFREG)
#define	S_ISDIR(m)		(((m) & _S_IFMT) == _S_IFDIR)

/* compatibility functions */
int srandom(unsigned seed);
int random(void);

#else /* must be in UNIX */

#if defined(HOST_LINUX) || defined (HOST_LINUX_X86) || defined (HOST_LINUX_IA64)
typedef unsigned int uint;
#endif
#if defined(HOST_FREEBSD) || defined (HOST_FREEBSD_X86)
typedef unsigned int uint;
#endif


/*
 * Digital Unix / Alpha definitions
 *
 */

/* compilation flag */
#define	DUNIX		(1)

/*
 * integer types 
 *
 * Some Digital Unix header files already define uintXX and intXX, 
 * so we define integer types named uXX and iXX instead.
 *
 */

#if defined(HOST_LINUX_X86) || defined(HOST_FREEBSD_X86)

#ifndef UINT64_CONST
#define UINT64_CONST(c) c ## ULL
#endif

typedef unsigned char u8;
typedef   signed char i8;
typedef unsigned int  u32;
typedef   signed int  i32;
typedef unsigned long long u64;
typedef   signed long long i64;
typedef unsigned long long uint64;
typedef   signed long long int64;

#else

#ifndef UINT64_CONST
#define UINT64_CONST(c) c ## UL
#endif

typedef unsigned char u8;
typedef   signed char i8;
typedef unsigned int  u32;
typedef   signed int  i32;
typedef unsigned long u64;
typedef   signed long i64;
typedef unsigned long long uint64;
typedef   signed long long int64;

#endif

/*
 * constants
 *
 */

/* dummy WIN32-specific open flags */
#define	O_BINARY	(0)
#define	FOPEN_BINARY	""

/* printf/scanf formatting for 64 bit integers */
#if defined (HOST_LINUX_X86)|| defined (HOST_FREEBSD_X86)
#define	FMT64		"ll"
#else
#define	FMT64		"l"
#endif

#define EXPORT extern

#endif
#endif	/* _PORTABILITY_H */
