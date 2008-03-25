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
 ***********************************************************************/

// generic
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/* Date */

char *UTIL_date_string (
    time_t	time_num	/* Time to parse, or 0 for current time */
    )
{
    static char	date_str[50];
    struct tm *timestr;
    
    if (!time_num) {
	time_num = time (NULL);
	}
    timestr = localtime (&time_num);
    strcpy (date_str, asctime (timestr));
    if (date_str[strlen (date_str)-1]=='\n')
	date_str[strlen (date_str)-1]='\0';

    return (date_str);
    }

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/* Strings */

char *msg (const char *format, ...)
{
    va_list args;
    static char buf[1000];
  
    va_start (args, format);
    vsprintf (buf, format, args);
    va_end (args);

    return (buf);
}

void strncat_dotdotdot (char *outstr, const char *instr, size_t size)
    /* Do a strcat, but limit the length of the output string to a specified length */
    /* Add a ... if over the specified size to prevent overflow */
{
    if (strlen (outstr) > size) {
	strcpy (outstr, "...");		/* Prevent string overflows */
    }
    if (outstr[0] != '\0') {
	strcat (outstr, ", ");
    }
    strcat (outstr, instr);
}


/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/* Memory management */

void *UTIL_calloc_internal (size_t num_of_elts, size_t elt_size, char *file, int line)
    /* Allocate memory with calloc, error if fails */
{
    void *ptr;
    ptr = calloc (num_of_elts, elt_size);
    if (NULL == ptr) {
	fprintf (stderr, "Out of memory at %s:%d\n", file, line);
        exit (1L);
    }
    return (ptr);
}

void *UTIL_recalloc_internal (void *old_ptr, size_t old_size, size_t size, char *file, int line)
    /* Reallocate memory to new size and clear new elements, error if fails */
{
    void *ptr;
    ptr = realloc (old_ptr, size);
    if (NULL == ptr) {
	fprintf (stderr, "Out of memory at %s:%d\n", file, line);
        exit (1L);
    }
    /* Now clear the new space */
    bzero (((char *)ptr + old_size), (size_t)(size - old_size));

    return (ptr);
}

void *UTIL_malloc_internal (size_t size, char *file, int line)
    /* Allocate memory with malloc, error if fails */
{
    void *ptr;
    ptr = malloc (size);
    if (NULL == ptr) {
	fprintf (stderr, "Out of memory at %s:%d\n", file, line);
        exit (1L);
    }
    return (ptr);
}

void *UTIL_realloc_internal (void *old_ptr, size_t size, char *file, int line)
    /* Reallocate memory to new size, error if fails */
{
    void *ptr;
    ptr = realloc (old_ptr, size);
    if (NULL == ptr) {
	fprintf (stderr, "Out of memory at %s:%d\n", file, line);
        exit (1L);
    }
    return (ptr);
}
