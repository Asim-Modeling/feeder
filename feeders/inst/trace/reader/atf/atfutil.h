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

/***********************************************************************/
#pragma ident "$Id: atf.h,v 1.6 1996/08/13 19:12:43 snyder Exp snyder"
/***********************************************************************
 *
 *	ABSTRACT:	Arana Model, General Utilities
 *
 **********************************************************************/
#ifndef _UTIL_H
#define _UTIL_H

/**********************************************************************/
/* Types */

typedef	u64 Counter;
typedef u64 Cycle;
typedef u64 Address;
typedef unsigned int  Boolean;
typedef	unsigned int  Thread;
typedef	u32 Opcode;

/**********************************************************************/
/* Useful values */

#ifndef TRUE
# define TRUE 1
#endif
#ifndef FALSE
# define FALSE 0
#endif

#define POW2(bit)  (UINT64_CONST(1) << (bit))		/* A shift.  Important to use this, else remember the UL! */
#define MASK(bit) ((UINT64_CONST(1) << (bit)) - 1)	/* A Mask with all bits less then the specified one set */

/**********************************************************************/
/* Date */

extern char *UTIL_date_string (time_t time_num);

/**********************************************************************/
/* Externs */

/* Standard externs -- These must be provided by the main module(s) */
#ifndef EV7
extern int debug_level;
#endif

/**********************************************************************/
/* Strings */

extern char *msg (const char *format, ...);
extern void strncat_dotdotdot  (char *outstr, const char *instr, size_t size);

/**********************************************************************/
/* Debugging */

#ifndef EV7 /* EV7 has it's own definitions */
#ifndef DBG_ERROR
# define DBG_ERROR(message) {fprintf(stderr,message); exit (10L);}
#endif
#ifndef DBG
# ifndef lint
#  define DBG(dbl, condition, message) {if ((debug_level >= dbl) && (dbl) && (!(condition))) fprintf(stderr, message); }
# else
#  define DBG(dbl, condition, message)	printf (message, condition, dbl)	/* lint fakeouts */
# endif
#endif
#if !defined(ASSERT) && !defined(SIMOS)
# ifndef lint
#  define ASSERT(condition,message) {if (!(condition)) {fprintf(stderr, "Assert fail %s at %s:%d\n", message, __FILE__, __LINE__); exit(1L);}}
# else
#  define ASSERT(condition, message) 	printf (message, condition)	/* lint fakeouts */
# endif
#endif
#endif /*EV7*/

#ifndef CKPT
# define CKPT() fprintf(stderr, "CKPT %d %s\n", __LINE__, __FILE__)
#endif

/**********************************************************************/
/* Math */

#define DIVSAFE(a, b) (((b)==0)?0:(a)/(b))
#define DIVSAFEFLOAT(a, b) (DIVSAFE((float)(a), (float)(b)))
#define DIVPERCENT(a,b) (100.0*DIVSAFEFLOAT((a), (b)))

/*
#ifndef MIN
#define MIN(A,B) (((A)<=(B))?(A):(B))
#endif
#ifndef MAX
#define MAX(A,B) (((A)<=(B))?(B):(A))
#endif
*/

/**********************************************************************/
/* Memory Management */

extern void *UTIL_calloc_internal (size_t num_of_elts, size_t elt_size, char *file, int line);
extern void *UTIL_malloc_internal (size_t size, char *file, int line);
extern void *UTIL_realloc_internal (void *ptr, size_t size, char *file, int line);
extern void *UTIL_recalloc_internal (void *ptr, size_t old_size, size_t size, char *file, int line);
#define UTIL_recalloc(ptr,oe,ne,el) (UTIL_recalloc_internal ((ptr), (oe), ((ne)*(el)), __FILE__, __LINE__))
#define UTIL_calloc(ne,el) (UTIL_calloc_internal ((ne), (el), __FILE__, __LINE__))
#define UTIL_malloc(sz) (UTIL_malloc_internal ((sz), __FILE__, __LINE__))
#define UTIL_realloc(ptr,sz) (UTIL_realloc_internal ((ptr), (sz), __FILE__, __LINE__))
#define UTIL_new(type) ((type)(UTIL_malloc_internal ((sizeof (type)), __FILE__, __LINE__)))	/* Aka XtNew */

#endif /*_UTIL_H*/
