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
 *
 * warn.c
 *
 * $Header$
 * $Log$
 * Revision 1.1  2001/11/15 16:09:43  klauser
 * - moving all (instruction) feeders one level down in directory hierarchy
 * - changed atf feeder to more generic trace feeder + atf reader
 *
 * Revision 1.3  2000/07/25 23:17:03  harish
 * CSN-feeder-97
 *
 * Revision 1.2  1999/09/15 18:52:36  espasa
 * CSN-feeder-34
 *
 * Revision 1.1  1999/08/20 18:24:13  steffan
 * CSN-feeder-29
 *
 * Revision 1.2  1997/03/04  21:45:44  jamey
 * add system calls for simulation control
 *
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/*VARARGS1*/


/* This code is broken because of a bug in stdarg and the current
 * compiler. */
extern void
__warning(const char *s, ...)
{
    va_list args;

    fprintf(stderr, "\nWarning: ");
    va_start(args, s);
    vfprintf(stderr, s, args);
    va_end(args);
    fprintf(stderr,"\n");
    fflush(stderr);
}
