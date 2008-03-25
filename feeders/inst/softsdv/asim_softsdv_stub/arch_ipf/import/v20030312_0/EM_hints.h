/*
 * Copyright (C) 2004-2006 Intel Corporation
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

#ifndef EM_HINTS_H
#define EM_HINTS_H

typedef enum EM_branch_hint_e
{
    EM_branch_hint_none,
    EM_branch_hint_sptk,
    EM_branch_hint_sptk_imp,
    EM_branch_hint_sptk_many,
    EM_branch_hint_sptk_many_imp,
    EM_branch_hint_mov,
    EM_branch_hint_imp,
    EM_branch_hint_many,
    EM_branch_hint_many_imp,
    EM_branch_hint_dptk,
    EM_branch_hint_dptk_imp,
    EM_branch_hint_dptk_many,
    EM_branch_hint_dptk_many_imp,
    EM_branch_hint_sptk_dc_nt,
    EM_branch_hint_sptk_dc_nt_imp,
    EM_branch_hint_sptk_many_dc_nt,
    EM_branch_hint_sptk_many_dc_nt_imp,
    EM_branch_hint_dc_nt,
    EM_branch_hint_dc_nt_imp,
    EM_branch_hint_many_dc_nt,
    EM_branch_hint_many_dc_nt_imp,
    EM_branch_hint_dptk_dc_nt,
    EM_branch_hint_dptk_dc_nt_imp,
    EM_branch_hint_dptk_many_dc_nt,
    EM_branch_hint_dptk_many_dc_nt_imp,
    EM_branch_hint_sptk_tk_dc,
    EM_branch_hint_sptk_tk_dc_imp,
    EM_branch_hint_sptk_many_tk_dc,
    EM_branch_hint_sptk_many_tk_dc_imp,
    EM_branch_hint_tk_dc,
    EM_branch_hint_tk_dc_imp,
    EM_branch_hint_many_tk_dc,
    EM_branch_hint_many_tk_dc_imp,
    EM_branch_hint_dptk_tk_dc,
    EM_branch_hint_dptk_tk_dc_imp,
    EM_branch_hint_dptk_many_tk_dc,
    EM_branch_hint_dptk_many_tk_dc_imp,
    EM_branch_hint_sptk_tk_tk,
    EM_branch_hint_sptk_tk_tk_imp,
    EM_branch_hint_sptk_many_tk_tk,
    EM_branch_hint_sptk_many_tk_tk_imp,
    EM_branch_hint_tk_tk,
    EM_branch_hint_tk_tk_imp,
    EM_branch_hint_many_tk_tk,
    EM_branch_hint_many_tk_tk_imp,
    EM_branch_hint_dptk_tk_tk,
    EM_branch_hint_dptk_tk_tk_imp,
    EM_branch_hint_dptk_many_tk_tk,
    EM_branch_hint_dptk_many_tk_tk_imp,
    EM_branch_hint_sptk_tk_nt,
    EM_branch_hint_sptk_tk_nt_imp,
    EM_branch_hint_sptk_many_tk_nt,
    EM_branch_hint_sptk_many_tk_nt_imp,
    EM_branch_hint_tk_nt,
    EM_branch_hint_tk_nt_imp,
    EM_branch_hint_many_tk_nt,
    EM_branch_hint_many_tk_nt_imp,
    EM_branch_hint_dptk_tk_nt,
    EM_branch_hint_dptk_tk_nt_imp,
    EM_branch_hint_dptk_many_tk_nt,
    EM_branch_hint_dptk_many_tk_nt_imp,
    EM_branch_hint_sptk_nt_dc,
    EM_branch_hint_sptk_nt_dc_imp,
    EM_branch_hint_sptk_many_nt_dc,
    EM_branch_hint_sptk_many_nt_dc_imp,
    EM_branch_hint_nt_dc,
    EM_branch_hint_nt_dc_imp,
    EM_branch_hint_many_nt_dc,
    EM_branch_hint_many_nt_dc_imp,
    EM_branch_hint_dptk_nt_dc,
    EM_branch_hint_dptk_nt_dc_imp,
    EM_branch_hint_dptk_many_nt_dc,
    EM_branch_hint_dptk_many_nt_dc_imp,
    EM_branch_hint_sptk_nt_tk,
    EM_branch_hint_sptk_nt_tk_imp,
    EM_branch_hint_sptk_many_nt_tk,
    EM_branch_hint_sptk_many_nt_tk_imp,
    EM_branch_hint_nt_tk,
    EM_branch_hint_nt_tk_imp,
    EM_branch_hint_many_nt_tk,
    EM_branch_hint_many_nt_tk_imp,
    EM_branch_hint_dptk_nt_tk,
    EM_branch_hint_dptk_nt_tk_imp,
    EM_branch_hint_dptk_many_nt_tk,
    EM_branch_hint_dptk_many_nt_tk_imp,
    EM_branch_hint_sptk_nt_nt,
    EM_branch_hint_sptk_nt_nt_imp,
    EM_branch_hint_sptk_many_nt_nt,
    EM_branch_hint_sptk_many_nt_nt_imp,
    EM_branch_hint_nt_nt,
    EM_branch_hint_nt_nt_imp,
    EM_branch_hint_many_nt_nt,
    EM_branch_hint_many_nt_nt_imp,
    EM_branch_hint_dptk_nt_nt,
    EM_branch_hint_dptk_nt_nt_imp,
    EM_branch_hint_dptk_many_nt_nt,
    EM_branch_hint_dptk_many_nt_nt_imp,
    EM_branch_hint_ret_sptk,
    EM_branch_hint_ret_sptk_imp,
    EM_branch_hint_ret_sptk_many,
    EM_branch_hint_ret_sptk_many_imp,
    EM_branch_hint_ret,
    EM_branch_hint_ret_imp,
    EM_branch_hint_ret_many,
    EM_branch_hint_ret_many_imp,
    EM_branch_hint_ret_dptk,
    EM_branch_hint_ret_dptk_imp,
    EM_branch_hint_ret_dptk_many,
    EM_branch_hint_ret_dptk_many_imp,
    EM_branch_hint_ret_sptk_dc_nt,
    EM_branch_hint_ret_sptk_dc_nt_imp,
    EM_branch_hint_ret_sptk_many_dc_nt,
    EM_branch_hint_ret_sptk_many_dc_nt_imp,
    EM_branch_hint_ret_dc_nt,
    EM_branch_hint_ret_dc_nt_imp,
    EM_branch_hint_ret_many_dc_nt,
    EM_branch_hint_ret_many_dc_nt_imp,
    EM_branch_hint_ret_dptk_dc_nt,
    EM_branch_hint_ret_dptk_dc_nt_imp,
    EM_branch_hint_ret_dptk_many_dc_nt,
    EM_branch_hint_ret_dptk_many_dc_nt_imp,
    EM_branch_hint_ret_sptk_tk_dc,
    EM_branch_hint_ret_sptk_tk_dc_imp,
    EM_branch_hint_ret_sptk_many_tk_dc,
    EM_branch_hint_ret_sptk_many_tk_dc_imp,
    EM_branch_hint_ret_tk_dc,
    EM_branch_hint_ret_tk_dc_imp,
    EM_branch_hint_ret_many_tk_dc,
    EM_branch_hint_ret_many_tk_dc_imp,
    EM_branch_hint_ret_dptk_tk_dc,
    EM_branch_hint_ret_dptk_tk_dc_imp,
    EM_branch_hint_ret_dptk_many_tk_dc,
    EM_branch_hint_ret_dptk_many_tk_dc_imp,
    EM_branch_hint_ret_sptk_tk_tk,
    EM_branch_hint_ret_sptk_tk_tk_imp,
    EM_branch_hint_ret_sptk_many_tk_tk,
    EM_branch_hint_ret_sptk_many_tk_tk_imp,
    EM_branch_hint_ret_tk_tk,
    EM_branch_hint_ret_tk_tk_imp,
    EM_branch_hint_ret_many_tk_tk,
    EM_branch_hint_ret_many_tk_tk_imp,
    EM_branch_hint_ret_dptk_tk_tk,
    EM_branch_hint_ret_dptk_tk_tk_imp,
    EM_branch_hint_ret_dptk_many_tk_tk,
    EM_branch_hint_ret_dptk_many_tk_tk_imp,
    EM_branch_hint_ret_sptk_tk_nt,
    EM_branch_hint_ret_sptk_tk_nt_imp,
    EM_branch_hint_ret_sptk_many_tk_nt,
    EM_branch_hint_ret_sptk_many_tk_nt_imp,
    EM_branch_hint_ret_tk_nt,
    EM_branch_hint_ret_tk_nt_imp,
    EM_branch_hint_ret_many_tk_nt,
    EM_branch_hint_ret_many_tk_nt_imp,
    EM_branch_hint_ret_dptk_tk_nt,
    EM_branch_hint_ret_dptk_tk_nt_imp,
    EM_branch_hint_ret_dptk_many_tk_nt,
    EM_branch_hint_ret_dptk_many_tk_nt_imp,
    EM_branch_hint_ret_sptk_nt_dc,
    EM_branch_hint_ret_sptk_nt_dc_imp,
    EM_branch_hint_ret_sptk_many_nt_dc,
    EM_branch_hint_ret_sptk_many_nt_dc_imp,
    EM_branch_hint_ret_nt_dc,
    EM_branch_hint_ret_nt_dc_imp,
    EM_branch_hint_ret_many_nt_dc,
    EM_branch_hint_ret_many_nt_dc_imp,
    EM_branch_hint_ret_dptk_nt_dc,
    EM_branch_hint_ret_dptk_nt_dc_imp,
    EM_branch_hint_ret_dptk_many_nt_dc,
    EM_branch_hint_ret_dptk_many_nt_dc_imp,
    EM_branch_hint_ret_sptk_nt_tk,
    EM_branch_hint_ret_sptk_nt_tk_imp,
    EM_branch_hint_ret_sptk_many_nt_tk,
    EM_branch_hint_ret_sptk_many_nt_tk_imp,
    EM_branch_hint_ret_nt_tk,
    EM_branch_hint_ret_nt_tk_imp,
    EM_branch_hint_ret_many_nt_tk,
    EM_branch_hint_ret_many_nt_tk_imp,
    EM_branch_hint_ret_dptk_nt_tk,
    EM_branch_hint_ret_dptk_nt_tk_imp,
    EM_branch_hint_ret_dptk_many_nt_tk,
    EM_branch_hint_ret_dptk_many_nt_tk_imp,
    EM_branch_hint_ret_sptk_nt_nt,
    EM_branch_hint_ret_sptk_nt_nt_imp,
    EM_branch_hint_ret_sptk_many_nt_nt,
    EM_branch_hint_ret_sptk_many_nt_nt_imp,
    EM_branch_hint_ret_nt_nt,
    EM_branch_hint_ret_nt_nt_imp,
    EM_branch_hint_ret_many_nt_nt,
    EM_branch_hint_ret_many_nt_nt_imp,
    EM_branch_hint_ret_dptk_nt_nt,
    EM_branch_hint_ret_dptk_nt_nt_imp,
    EM_branch_hint_ret_dptk_many_nt_nt,
    EM_branch_hint_ret_dptk_many_nt_nt_imp,
    EM_branch_hint_sptk_few,
    EM_branch_hint_spnt_few,
    EM_branch_hint_spnt_many,
    EM_branch_hint_dptk_few,
    EM_branch_hint_dpnt_few,
    EM_branch_hint_dpnt_many,
    EM_branch_hint_sptk_few_clr,
    EM_branch_hint_sptk_many_clr,
    EM_branch_hint_spnt_few_clr,
    EM_branch_hint_spnt_many_clr,
    EM_branch_hint_dptk_few_clr,
    EM_branch_hint_dptk_many_clr,
    EM_branch_hint_dpnt_few_clr,
    EM_branch_hint_dpnt_many_clr,
    EM_branch_hint_loop,
    EM_branch_hint_loop_imp,
    EM_branch_hint_loop_many,
    EM_branch_hint_loop_many_imp,
    EM_branch_hint_exit,
    EM_branch_hint_exit_imp,
    EM_branch_hint_exit_many,
    EM_branch_hint_exit_many_imp,
    EM_branch_hint_loop_dc_nt,
    EM_branch_hint_loop_dc_nt_imp,
    EM_branch_hint_loop_many_dc_nt,
    EM_branch_hint_loop_many_dc_nt_imp,
    EM_branch_hint_exit_dc_nt,
    EM_branch_hint_exit_dc_nt_imp,
    EM_branch_hint_exit_many_dc_nt,
    EM_branch_hint_exit_many_dc_nt_imp,
    EM_branch_hint_loop_tk_dc,
    EM_branch_hint_loop_tk_dc_imp,
    EM_branch_hint_loop_many_tk_dc,
    EM_branch_hint_loop_many_tk_dc_imp,
    EM_branch_hint_exit_tk_dc,
    EM_branch_hint_exit_tk_dc_imp,
    EM_branch_hint_exit_many_tk_dc,
    EM_branch_hint_exit_many_tk_dc_imp,
    EM_branch_hint_loop_tk_tk,
    EM_branch_hint_loop_tk_tk_imp,
    EM_branch_hint_loop_many_tk_tk,
    EM_branch_hint_loop_many_tk_tk_imp,
    EM_branch_hint_exit_tk_tk,
    EM_branch_hint_exit_tk_tk_imp,
    EM_branch_hint_exit_many_tk_tk,
    EM_branch_hint_exit_many_tk_tk_imp,
    EM_branch_hint_loop_tk_nt,
    EM_branch_hint_loop_tk_nt_imp,
    EM_branch_hint_loop_many_tk_nt,
    EM_branch_hint_loop_many_tk_nt_imp,
    EM_branch_hint_exit_tk_nt,
    EM_branch_hint_exit_tk_nt_imp,
    EM_branch_hint_exit_many_tk_nt,
    EM_branch_hint_exit_many_tk_nt_imp,
    EM_branch_hint_loop_nt_dc,
    EM_branch_hint_loop_nt_dc_imp,
    EM_branch_hint_loop_many_nt_dc,
    EM_branch_hint_loop_many_nt_dc_imp,
    EM_branch_hint_exit_nt_dc,
    EM_branch_hint_exit_nt_dc_imp,
    EM_branch_hint_exit_many_nt_dc,
    EM_branch_hint_exit_many_nt_dc_imp,
    EM_branch_hint_loop_nt_tk,
    EM_branch_hint_loop_nt_tk_imp,
    EM_branch_hint_loop_many_nt_tk,
    EM_branch_hint_loop_many_nt_tk_imp,
    EM_branch_hint_exit_nt_tk,
    EM_branch_hint_exit_nt_tk_imp,
    EM_branch_hint_exit_many_nt_tk,
    EM_branch_hint_exit_many_nt_tk_imp,
    EM_branch_hint_loop_nt_nt,
    EM_branch_hint_loop_nt_nt_imp,
    EM_branch_hint_loop_many_nt_nt,
    EM_branch_hint_loop_many_nt_nt_imp,
    EM_branch_hint_exit_nt_nt,
    EM_branch_hint_exit_nt_nt_imp,
    EM_branch_hint_exit_many_nt_nt,
    EM_branch_hint_exit_many_nt_nt_imp,
    EM_branch_hint_last
} EM_branch_hint_t;

typedef enum EM_memory_access_hint_e
{
    EM_memory_access_hint_none,
    EM_memory_access_hint_nt1,
    EM_memory_access_hint_nta,
    EM_memory_access_hint_nt2,
    EM_memory_access_hint_last
} EM_memory_access_hint_t;


#endif  /* EM_HINTS_H */