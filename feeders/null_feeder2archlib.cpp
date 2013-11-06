/*
 * Copyright (C) 2002-2006 Intel Corporation
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

// Author: Sailashri Parthasarathy (based on Chris Weaver's original implementation)
//

// Null feeder2archlib module

#include "asim/provides/feeder_2_archlib.h"

void indigo_fire_perfplot(FILE* perfp_fp) {};
void embed_indigo_yes_file() {};
bool is_indigo_count_stat(string stat_name) { return false; };
bool is_indigo_histo_stat(string stat_name) { return false; };
