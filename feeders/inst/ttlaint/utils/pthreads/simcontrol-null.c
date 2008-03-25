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
 * simcontrol-null.c:
 *   Null implementation of AINT-syscalls that control simulation and skipping
 *   for use outside of AINT
 *
 * $Header$
 * $Log$
 * Revision 1.1  2001/11/15 16:09:43  klauser
 * - moving all (instruction) feeders one level down in directory hierarchy
 * - changed atf feeder to more generic trace feeder + atf reader
 *
 * Revision 1.1  1999/08/20 18:24:13  steffan
 * CSN-feeder-29
 *
 * Revision 1.1  1997/03/04  21:57:53  jamey
 * *** empty log message ***
 *
 *
 */

int request_begin_skipping(long how_many_to_skip)
{
  /* nothing to do */
}


int request_end_skipping()
{
  /* nothing to do */
}

int request_end_simulation()
{
  /* nothing to do */
}

int record_event(long event, long value)
{
  /* nothing to do */
}

