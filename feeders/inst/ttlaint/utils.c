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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include "protos.h"

/* The default maximum number of error and warning messages. If these limits
 * are reached, then the program will exit. This prevents long-running
 * programs from generating so many messages that an output file fills
 * up the disk. These limits are stored in the global variables
 * "Max_errors" and "Max_warnings" and can be changed by the back-end,
 * if necessary. If these limits are set to 0 (or any negative number)
 * then the limits are ignored.
 */
#define MAX_ERRORS 100
#define MAX_WARNINGS 0

extern int Verbose;
int Max_errors = MAX_ERRORS;
int Max_warnings = MAX_WARNINGS;
/* extern int Quiet; */

/*
 * This routine prints an error message and exits.
 */

/*VARARGS1*/
void
fatal(const char *s, ...)
{
    va_list ap;

    fflush(NULL);
    fprintf(Aint_output, "\nFATAL: ");
    fflush(Aint_output);
    va_start(ap, s);
    vfprintf(Aint_output, s, ap);
    va_end(ap);
    if (Verbose) {
      void invoke_debugger_on_self ();
    }
    exit(1);
}


/*
 * This routine emulates the behaviour of the assert macro
 */

/*VARARGS1*/
void
myassert(unsigned int bool, const char *s, ...)
{
    va_list ap;

    /* IF condition holds, simply return. If not, die */
    if ( bool ) return;

    fflush(NULL);
    fprintf(Aint_output, "\nAINT ASSERTION FAILURE: ");
    fflush(Aint_output);
    va_start(ap, s);
    vfprintf(Aint_output, s, ap);
    va_end(ap);
    if (Verbose) {
      void invoke_debugger_on_self ();
    }
    exit(1);
}

/*
 * This routine prints an error message.
 *
 * To prevent a long-running program from generating so many error
 * messages that an output file fills up the disk, a limit is placed
 * on how many total error messages can be produced. This limit
 * is a global variable and can be changed by the back-end, if necessary.
 */

/*VARARGS1*/
void
error(const char *s, ...)
{
    va_list ap;
    static total_errors = 0;

    fflush(NULL);
    fprintf(Aint_output, "\nERROR: ");
    va_start(ap, s);
    vfprintf(Aint_output, s, ap);
    va_end(ap);

    if (Max_errors > 0 && total_errors++ >= Max_errors) {
        fprintf(Aint_output, "Too many errors (%d)\n", total_errors);
        exit(1);
    }
}

/*
 * This routine prints a warning message.
 *
 * To prevent a long-running program from generating so many warning
 * messages that an output file fills up the disk, a limit is placed
 * on how many total warning messages can be produced. This limit
 * is a global variable and can be changed by the back-end, if necessary.
 */

/*VARARGS1*/
void
warning(const char *s, ...)
{
    va_list ap;
    static total_warnings = 0;

    fflush(NULL);
    fprintf(Aint_output, "\nWarning: ");
    va_start(ap, s);
    vfprintf(Aint_output, s, ap);
    va_end(ap);

    if (Max_warnings > 0 && total_warnings++ >= Max_warnings) {
        fprintf(Aint_output, "Too many warnings (%d)\n", total_warnings);
        exit(1);
    }
}

void
notice(const char *s, ...)
{
    va_list ap;
    static total_warnings;

    if (Verbose) {
      fflush(NULL);
      fprintf(Aint_output, "\nNotice: ");
      va_start(ap, s);
      vfprintf(Aint_output, s, ap);
      va_end(ap);

      if (Max_warnings > 0 && total_warnings++ >= Max_warnings) {
        fprintf(Aint_output, "Too many notices (%d)\n", total_warnings);
        exit(1);
      }
    }
}


/*
 * This routine prints an informative message.
 *
 * To prevent a long-running program from generating so many warning
 * messages that an output file fills up the disk, a limit is placed
 * on how many total warning messages can be produced. This limit
 * is a global variable and can be changed by the back-end, if necessary.
 */

/*VARARGS1*/
#if 0
void
informative(const char *s, ...)
{
  va_list ap;
  static total_warnings;

    /*    if (Quiet)
        return; */

    if (Verbose) {
      fflush(NULL);
      va_start(ap, s);
      vfprintf(Aint_output, s, ap);
      va_end(ap);

      if (Max_warnings > 0 && total_warnings++ >= Max_warnings) {
        fprintf(Aint_output, "Too many warnings (%d)\n", total_warnings);
        exit(1);
      }
    }
}
#endif

/* strlen_expand() returns the length of the string, counting tabs
 * as the equivalent number of spaces that would be generated.
 */
int
strlen_expand(const char *str)
{
    int len;

    if (str == NULL)
        return 0;
    for (len = 0; *str; len++, str++)
        if (*str == '\t')
            len += 7 - (len % 8);
    return len;
}

/*
 * base2roundup() returns the log (base 2) of its argument, rounded up.
 * It also rounds up its argument to the next higher power of 2.
 *
 * Example:
 *   int logsize, size = 5;
 * 
 *   logsize = base2roundup(&size);
 *
 * will make logsize = 3, and size = 8.
 */
int
base2roundup(int *pnum)
{
    int logsize, exp;

    for (logsize = 0, exp = 1; exp < *pnum; logsize++)
        exp *= 2;
    
    /* round pnum up to nearest power of 2 */
    *pnum = exp;

    return logsize;
}

/* returns the number of newlines in "buf"
 */
int
newlines(const char *buf)
{
    int count;

    count = 0;
    while (*buf) {
        if (*buf == '\n')
            count++;
        buf++;
    }
    return count;
}

