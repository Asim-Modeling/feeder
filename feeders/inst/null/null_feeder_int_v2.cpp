/*
 * Copyright (C) 2005-2006 Intel Corporation
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

//
// Authors:  Ramon Matas Navarro
//

// generic
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/trace.h"
#include "asim/thread.h"
#include "asim/cmd.h"

// ASIM local module
#include "null_feeder_v2.h"

// ASIM public modules
#include "asim/provides/instfeeder_interface.h"

void
IFEEDER_Usage (FILE *file)
/*
 * Print usage...
 */
{
    fprintf(file, "\nFeeder usage: ... [-eventfile <filename>] ...\n");
}

IFEEDER_BASE
IFEEDER_New(void)
{
    return new NULL_FEEDER_CLASS();
}

