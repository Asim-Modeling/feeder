/*
 * Copyright (C) 2003-2006 Intel Corporation
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


#ifndef _HDB_CALLS_H
#define _HDB_CALLS_H

extern "C" {


/* ones that have some hacking over the baseline */
//this used to be main
int asim_init(int argc,char *argv[],char *arge[]);
void asim_hdb_set_view_cpu(U32 cpu_num);
int asim_step(U64 y);


HDB_ADDR_t msl_label_addr(char *name);
static U64 hdb_get_i_count();
char * msl_label_name(HDB_ADDR_t addr, int exact, U64 *offset);


/*****************************************************************
 *!!!!!!!!!!!!The FUNCTIONS BELOW ARE INCLUDED FOR COMPLETENESS
 *HOWEVER USE MUCH SAFER TO USE THE CPUAPI !!!!!!!!!!!!!!!!!!!
 *****************************************************************/



int hdb_include(char *fname, int warn);

/* called through asim_init do not use */
void hdb_init();

/* Functions that work */

/* conversion */
char*
u64_toString(U64 u,        /* 64-bit number */
             U32 numSys,   /* 2: binary, 8: octal, 10: decimal, 16: hex */
             char* str     /* must be big enough to hold string (include zero msb's) */
             );

void hdb_v2p(HDB_ADDR_t addr, int mode);
U64 any_str_to_i(char *str);
U64 hs_to_i(char *str);
U64 ds_to_i(char *str);
HDB_ADDR_t get_linear_addr(HDB_ADDR_t addr, int addr_mode);

/* print outs */
void hdb_print_addr(parse_type *parse_ptr);
void hdb_dissasm(HDB_ADDR_t addr, long val, int mode);
void hdb_print_addr_buf(HDB_ADDR_t addr, int size, int base, int mode,int buf_size);

/* flow control */
void hdb_remove_all_brk(GE_brkpt_type_t type);
void hdb_del_break(U64 pid, HDB_ADDR_t addr, GE_brkpt_type_t type, int noisy);
void hdb_del_tmp_break();
void hdb_break_list(GE_brkpt_type_t type);
void hdb_set_break (U64 pid, HDB_ADDR_t addr, GE_brkpt_type_t type, int addr_mode, int noisy);
void hdb_set_tmp_break (U64 pid, HDB_ADDR_t addr, int addr_mode);

/*sets/get */
void hdb_set_oml_obj(char* input);
void hdb_get_oml_obj(char *obj_name);

/* eflag stuff */
void hdb_set_eflag(char* name, unsigned val);
void hdb_get_eflag(char* name);


/* memory */
void hdb_set_addr(HDB_ADDR_t addr, U64 val, int size, int mode);
#define FUNC
FUNC GE_stat_t hdb_access_memory(HDB_ADDR_t addr,
                                 HDB_memory_access_t   access,
                                 unsigned   mode,
                                 unsigned   size,
                                 unsigned   noisy,
                                 char*      buffer);

/* frequency register for doing breaks by time do not use */
void hdb_set_freg_int_ia(int num,U64 val);
void hdb_set_freg_ia(int num,U128 val);

/* print x86 regs stuff */
void hdb_print_all_iregs_ia(int base);
void hdb_print_all_fregs_ia(int base);

/* ip */
HDB_ADDR_t hdb_get_ip(void);
void hdb_print_ip(parse_type* parse_ptr);
void hdb_set_ip(parse_type*, U64 val);

/* display stuff for interactive mode do not use */
OML_handle_t hdb_get_ia32_reg_handle(HDB_reg_info_t* reg);
void hdb_do_exit(int exit_code);
void hdb_reset_line_count(void);
void hdb_reset_increment(void);
int hdb_process_input(void);
void open_decoder_conection(void);
char *fix_nat_printing( char *source , char *dest );
void hdb_add_display(parse_type *parse_ptr);
void hdb_callf(HDB_ADDR_t func, HDB_ADDR_t *params);
void hdb_help(char *str);
void hdb_show(char *str);
void hdb_platform(char *str);
void yyerror (char *str);
void hdb_quit (void);

void hdb_save(char *fname);
void hdb_load(char *name);

/* Stepping functions do not use!!! Use asim_step instead */
int hdb_internal_step(unsigned is_single_cpu, U32 cpu_num, U64 num, int noisy);
int hdb_step(U64 num,int noisy);
int hdb_advance_single_cpu(U32 cpu_num, U64 num, int noisy);
void hdb_run(char *str, int load);
void hdb_next(U64 num);
void hdb_next_single_cpu(U32 cpu_num, U64 num);


/* no idea */
HDB_ADDR_t hdb_name_to_addr(char *str);
char **hdb_get_command_list(char *sym);
void hdb_event_set_brk(char *ev);
void hdb_event_rel_brk(char *ev);
void hdb_mode(void);
U32 hdb_get_current_cpu();
char *get_hdb_name();
void hdb_print_EFLAGS(U32 val);




/* more break points */
void hdb_del_data_break(HDB_ADDR_t addr, int write);
void hdb_set_data_break(HDB_ADDR_t addr, int write);
void hdb_data_break_list(int write);
void hdb_remove_all_data_brk(int write);

/* cfm stuff */
U64 hdb_get_cfm();
void hdb_set_cfm(parse_type* parse_ptr, U64 val);
void hdb_print_cfm(parse_type* parse_ptr);

/* ireg stuff */
U64 hdb_get_ireg(int num);
void hdb_set_ireg(parse_type* parse_ptr, U64 val);
void hdb_print_ireg(parse_type* parse_ptr);
void hdb_print_all_iregs(int base);
U8 hdb_get_iregNaT(int num);
void hdb_set_iregNaT(parse_type* parse_ptr, U64 val);
void hdb_print_iregNaT(parse_type* parse_ptr);

/* areg stuff */
U64 hdb_get_areg(int num);
void hdb_set_areg(parse_type* parse_ptr,U64 val);
void hdb_print_areg(parse_type* parse_ptr);

/* predicate stuff */
U64 hdb_get_predicates();
U8 hdb_get_prreg(int num);
void hdb_print_prreg(parse_type* parse_ptr);
void hdb_print_all_prreg(int base);

/* banked reg */
void hdb_set_banked_regNaT(parse_type* parse_ptr, U64 val);
U8 hdb_get_banked_regNaT(int num);
void hdb_print_banked_regNaT(parse_type* parse_ptr);
void hdb_set_banked_reg(parse_type* parse_ptr, U64 val);
U64 hdb_get_banked_reg(int num);
void hdb_print_banked_reg(parse_type* parse_ptr);

/* breg */
void hdb_set_breg(int num, U64 val);
U64 hdb_get_breg(int num);
void hdb_print_breg(parse_type* parse_ptr);

/* ibreg */
void hdb_set_ibreg(parse_type* parse_ptr, U64 val);
U64 hdb_get_ibreg(int num);
void hdb_print_ibreg(parse_type* parse_ptr);
void hdb_print_all_ibregs(int base);

/* dbreg */
void hdb_set_dbreg(parse_type* parse_ptr, U64 val);
U64 hdb_get_dbreg(int num);
void hdb_print_dbreg(parse_type* parse_ptr);
void hdb_print_all_dbregs(int base);

/* ms reg */
U64 hdb_get_msreg(int num);
void hdb_print_msreg(parse_type* parse_ptr);
void hdb_set_msreg(parse_type* parse_ptr, U64 val);

/* cpu id stuff */
U64 hdb_get_cpuid(int num);
void hdb_print_cpuid(parse_type* parse_ptr);

/* ctrl reg */
U64 hdb_get_ctrl_reg(int num);
void hdb_set_ctrl_reg(parse_type* parse_ptr,U64 val);
void hdb_print_ctrl_reg(parse_type* parse_ptr);

/* pk reg */
U64 hdb_get_pkreg(int num);
void hdb_set_pkreg(parse_type* parse_ptr, U64 val);
void hdb_print_pkreg(parse_type* parse_ptr);

/* rreg */
U64 hdb_get_rreg(int num);
void hdb_set_rreg(parse_type* parse_ptr,U64 val);
void hdb_print_rreg(parse_type* parse_ptr);

/* rse stuff */
U32 hdb_get_rse_bof();
void hdb_set_rse_bof(parse_type* parse_ptr, U64 val);
void hdb_print_rse_bof(parse_type* parse_ptr);
void hdb_print_sapic_isr(parse_type* parse_ptr);
void hdb_set_rse_load_reg(parse_type* parse_ptr,U64 val);
void hdb_print_rse_load_reg(parse_type* parse_ptr);
void hdb_set_rse_store_reg(parse_type* parse_ptr,U64 val);
void hdb_print_rse_store_reg(parse_type* parse_ptr);
void hdb_set_rse_bspload(parse_type* parse_ptr,U64 val);
void hdb_print_rse_bspload(parse_type* parse_ptr);
/* psr stuff */
void hdb_set_psr(parse_type* parse_ptr, U64 val);
void hdb_print_psr(parse_type* parse_ptr);

/* debug stuff */
void hdb_nat_debug (int set);
void hdb_alat_debug (int set);

/* freg stuff */
U128 hdb_get_freg(int num);
void hdb_set_freg_NaTVaL(int num);
void hdb_set_freg(int num,U128 val);
void hdb_set_freg_int(int num,U64 val);
void hdb_print_freg(parse_type *parse_ptr);
void hdb_print_all_fregs();

void hdb_inject(char *string);

}

#endif
