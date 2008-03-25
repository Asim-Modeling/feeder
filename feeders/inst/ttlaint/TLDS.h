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


#define ENABLE_TLDS_EXTENSIONS

#include <machine/inst.h>

#define AMASK_VECTOR    0x8000  /* vector extensions */
#define AMASK_TLDS      0x4000  /* thread-level data spec extensions */

/* for now we set the machine to look like an ev6 */
#define AINT_IMPLVER (IMPLVER_EV6_FAMILY)
#define AINT_AMASK   (AMASK_BWX | AMASK_FIX | AMASK_MVI | AMASK_PRECISE | AMASK_TLDS)
