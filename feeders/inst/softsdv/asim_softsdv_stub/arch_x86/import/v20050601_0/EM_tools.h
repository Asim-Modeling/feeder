/* Non MED section begin */
/*
/* Copyright (C) 2005-2006 Intel Corporation
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
 */

#ifndef _EM_TOOLS_H
#define _EM_TOOLS_H

typedef struct EM_version_s
{
	int      major;
	int      minor;
} EM_version_t;

typedef struct EM_library_version_s
{
	EM_version_t   xversion;
	EM_version_t   api;
	EM_version_t   emdb;
    char           date[12];
    char           time[9];
} EM_library_version_t;

#endif /* _EM_TOOLS_H */

