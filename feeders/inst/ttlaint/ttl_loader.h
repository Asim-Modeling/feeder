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

#ifndef _TTL_LOADER_H_
#define _TTL_LOADER_H_


#ifdef NEW_TTL

/*
 * The "ttl_isa.h" file is a file with a peculiar format. It contains, separated by 'ifdef'
 * three parts of the TTL ISA definition. At this point, we want access to the opcode 
 * definitions. To get that (and only that) we must define the OPCODE_FORMAT macro
 */
#define HOST_DUNIX /* to be deleted */
#include "syntax.h"
#define OPCODE_FORMAT	1
#define TTL_EXTENSION   1
#include "ttl_isa.h"


/* Federico Ardanaz notes:
 * Here I #define the opcodes used for tarantula. Remember
 * that these opcodes overlaps with vax fp instructions and so on...
 * we define it in terms of ttl_opcode just for code clarity in some
 * switch{} cases...
*/
#define ttl_load_opcode     0x20
#define ttl_store_opcode    0x21
#define ttl_arit_opcode     0x15
#define ttl_qarit_opcode    0x24
/* where _qarit_ means aritmetic instructions with qualifiers....*/

/* I need also some bit fields that match with 32 bit TTL
 * instruction format!
*/
union ttl_instruction{
  
  unsigned word;
  
  struct {
    unsigned    disp    :  11;
    unsigned    ex      :   4;
    unsigned    m       :   1;
    unsigned    rb      :   5;
    unsigned    va      :   5;
    unsigned    opcode  :   6;
  } ttl_memory_instruction;

  struct {
    unsigned    vc      :   5;
    unsigned    function:   8;
    unsigned    t       :   2;
    unsigned    m       :   1;
    unsigned    vb      :   5;
    unsigned    va      :   5;
    unsigned    opcode  :   6;
  } ttl_aritmetic_instruction;
};

/* end Fede additions */

typedef struct {
	 vector_opcode_t	opcode;
	 vector_data_type_t	type;
	 vector_func_t		func;
	} ttl_encoding_t;

typedef enum {
	 TTL_MEM_CLASS,		/* Tarantula mem:  $(rx) -> vector                      */
	 TTL_VVV_CLASS,		/* Tarantula insn: Vector op Vector -> Vector           */
	 TTL_VFV_CLASS,		/* Tarantula insn: Vector op FPreg  -> Vector           */
	 TTL_VIV_CLASS,		/* Tarantula insn: Vector op INTreg -> Vector           */
	 TTL_NVV_CLASS,		/* Tarantula insn: Vector -> Vector, like sqrt, ctpop...*/
     TTL_MVTVP_CLASS,   /* Tarantula insn: ttl_mvtvp $reg, [vl|vs]              */
     TTL_MVFVP_CLASS,   /* Tarantula insn: ttl_mvfvp [vmh|vml], $reg   			*/
	 TTL_IOTA_CLASS,	/* Tarantula insn: ttl_viota Vxx						*/
	 TTL_SETM_CLASS,	/* Tarantula insn: ttl_setvm /  ttlsetnvm Vxx			*/
	 TTL_GATH_CLASS,	/* Tarantula insn: ttl_gath[q,l,t,s]					*/
	 TTL_SCAT_CLASS,	/* Tarantula insn: ttl_scat[q,l,t,s]					*/
	 TTL_MVFVR_CLASS,
	 TTL_SYNCH_CLASS,	
	 TTL_MAX_CLASS
	} ttl_class_t;


typedef struct {
	 char		*name;		    /* TTL routine name (as it appears in a.out file) */
	 int		nargs;		    /* number of arguments expected for this TTL instruction */
	 ttl_class_t	class;	    /* Generic class of instruction: see ttl_class_t */
	 int		opnum;		    /* Index into the op_desc desc_table */
	 ttl_encoding_t	encoding;
	} ttl_routine_t;



/*
 * NOTE: ttl_routines and ttl_index MUST BE KEPT IN THE SAME ORDER. Otherwise, vax2ttl() will break;
 */
enum ttl_index {
    /* 00 */    TTL_INIT,
	/* 01 */    TTL_MVTVP,
	/* 02 */    TTL_MVFVP,
                TTL_SETNVM,
                TTL_SETVM,
                TTL_VCTLZ,
                TTL_VCTTZ,
                TTL_VCTPOP,
                TTL_VEXTSL,
                TTL_VEXTSH,
                TTL_VIOTA,
                TTL_VSKEWH,
                TTL_VSKEWL,
                TTL_VLDT,
                TTL_VLDS,
                TTL_VLDQ,
                TTL_VLDL,
                TTL_VSTT,
                TTL_VSTS,
                TTL_VSTQ,
                TTL_VSTL,
                TTL_VGATHQ,
                TTL_VGATHL,
                TTL_VGATHT,
                TTL_VGATHS,
                TTL_VSCATQ,
                TTL_VSCATL,
                TTL_VSCATT,
                TTL_VSCATS,
                TTL_VNCGATHQ,
                TTL_VNCGATHL,
                TTL_VNCGATHT,
                TTL_VNCGATHS,
                TTL_VNCSCATQ,
                TTL_VNCSCATL,
                TTL_VNCSCATT,
                TTL_VNCSCATS,
                TTL_VVADDT,
                TTL_VVADDS,
                TTL_VVADDQ,
                TTL_VVADDL,
                TTL_VVSUBT,
                TTL_VVSUBS,
                TTL_VVSUBQ,
                TTL_VVSUBL,
                TTL_VVMULT,
                TTL_VVMULS,
                TTL_VVMULQ,
                TTL_VVMULL,
                TTL_VVDIVT,
                TTL_VVDIVS,
                TTL_VSQRTT,
                TTL_VSQRTS,
                TTL_VSADDT,
                TTL_VSADDS,
                TTL_VSSUBT,
                TTL_VSSUBS,
                TTL_VSMULT,
                TTL_VSMULS,
                TTL_VSDIVT,
                TTL_VSDIVS,
                TTL_VSADDQ,
                TTL_VSADDL,
                TTL_VSSUBQ,
                TTL_VSSUBL,
                TTL_VSMULQ,
                TTL_VSMULL,
                TTL_VSAND,
                TTL_VSBIS,
                TTL_VSXOR,
                TTL_VSBIC,
                TTL_VSORNOT,
                TTL_VSEQV,
                TTL_VVAND,
                TTL_VVBIS,
                TTL_VVXOR,
                TTL_VVBIC,
                TTL_VVORNOT,
                TTL_VVEQV,
                TTL_VVCMPEQ,
                TTL_VVCMPLE,
                TTL_VVCMPLT,
                TTL_VSCMPEQ,
                TTL_VSCMPLE,
                TTL_VSCMPLT,
                TTL_VVCMPULE,
                TTL_VVCMPULT,
                TTL_VSCMPULE,
                TTL_VSCMPULT,
                TTL_VVCMPBGE,
                TTL_VSCMPBGE,
                TTL_VCVTQS,
                TTL_VCVTQT,
                TTL_VCVTST,
                TTL_VCVTTQ,
                TTL_VCVTTS,
                TTL_VSEXTBL,
                TTL_VSEXTWL,
                TTL_VSEXTLL,
                TTL_VSEXTQL,
                TTL_VSEXTWH,
                TTL_VSEXTLH,
                TTL_VSEXTQH,
                TTL_VVEXTBL,
                TTL_VVEXTWL,
                TTL_VVEXTLL,
                TTL_VVEXTQL,
                TTL_VVEXTWH,
                TTL_VVEXTLH,
                TTL_VVEXTQH,
                TTL_VSINSBL,
                TTL_VSINSWL,
                TTL_VSINSLL,
                TTL_VSINSQL,
                TTL_VSINSWH,
                TTL_VSINSLH,
                TTL_VSINSQH,
                TTL_VVINSBL,
                TTL_VVINSWL,
                TTL_VVINSLL,
                TTL_VVINSQL,
                TTL_VVINSWH,
                TTL_VVINSLH,
                TTL_VVINSQH,
                TTL_VVMERG,
                TTL_VSMERGQ,
                TTL_VSMERGT,
                TTL_VVMSKBL,
                TTL_VVMSKWL,
                TTL_VVMSKLL,
                TTL_VVMSKQL,
                TTL_VVMSKWH,
                TTL_VVMSKLH,
                TTL_VVMSKQH,
                TTL_VSMSKBL,
                TTL_VSMSKWL,
                TTL_VSMSKLL,
                TTL_VSMSKQL,
                TTL_VSMSKWH,
                TTL_VSMSKLH,
                TTL_VSMSKQH,
                TTL_VSSRA,
                TTL_VVSRA,
                TTL_VSSLL,
                TTL_VVSLL,
                TTL_VSSRL,
                TTL_VVSRL,
                TTL_VS4ADDQ,
                TTL_VS8ADDQ,
                TTL_VS4ADDL,
                TTL_VS8ADDL,
                TTL_VS4SUBQ,
                TTL_VS8SUBQ,
                TTL_VS4SUBL,
                TTL_VS8SUBL,
                TTL_VVUMULH,
                TTL_VSUMULH,
                TTL_VVZAP,
                TTL_VSZAP,
                TTL_VVZAPNOT,
                TTL_VSZAPNOT,
                TTL_VVCMPTEQ,
                TTL_VVCMPTLE,
                TTL_VVCMPTLT,
                TTL_VVCMPTUN,
                TTL_VSCMPTEQ,
                TTL_VSCMPTLE,
                TTL_VSCMPTLT,
                TTL_VSCMPTUN,
                TTL_VCMPR,
                TTL_VSYNCH,
                TTL_VLDPF,
                TTL_VSTPF,
                TTL_VGATHPF,
                TTL_VSCATPF,
                TTL_VNCGATHPF,
                TTL_VNCSCATPF,
				TTL_VDRAINM0,
				TTL_VDRAINM1,
				TTL_VDRAINM2,
				TTL_VDRAINV0,
				TTL_VDRAINV1,
				TTL_VDRAINV2,
				TTL_MVFVR,
	            TTL_MAX_INSNS
};

/* --------------------------------------------------------------------------------------------------
 * NOTE: ttl_routines and ttl_index MUST BE KEPT IN THE SAME ORDER. Otherwise, vax2ttl() will break;
 * --------------------------------------------------------------------------------------------------
*/
 
static ttl_routine_t ttl_routines[TTL_MAX_INSNS] = {

    /* remember format is:
     *  {name,argnum,class,opnum, {opcode,dataType,functionCode} }
    */ 


    {"ttl_init",  3, TTL_VVV_CLASS  }, 
    {"ttl_mvtvp", 2, TTL_MVTVP_CLASS, ttl_mvtvp_opn,       { V_NORMAL_OP,   0, V_MVTVP_FUNC } },
    {"ttl_mvfvp", 1, TTL_MVFVP_CLASS, ttl_mvfvp_opn,       { V_NORMAL_OP,   0, V_MVFVP_FUNC } },
    {"ttl_setnvm",  1, TTL_SETM_CLASS, ttl_setnvm_opn,     { V_NORMAL_OP,   0, V_SETNVM_FUNC } },
    {"ttl_setvm",   1, TTL_SETM_CLASS, ttl_setvm_opn,      { V_NORMAL_OP,   0, V_SETVM_FUNC } },
    {"ttl_vctlz",   2, TTL_NVV_CLASS, ttl_vctlz_opn,       { V_NORMAL_OP,   0, V_CTLZ_FUNC } },
    {"ttl_vcttz",   2, TTL_NVV_CLASS, ttl_vcttz_opn,       { V_NORMAL_OP,   0, V_CTTZ_FUNC } },
    {"ttl_vctpop",  2, TTL_NVV_CLASS, ttl_vctpop_opn,      { V_NORMAL_OP,   0, V_CTPOP_FUNC } },
    {"ttl_vextsl",  2, TTL_NVV_CLASS, ttl_vextsl_opn,      { V_NORMAL_OP,   0, V_EXTSL_FUNC } },
    {"ttl_vextsh",  2, TTL_NVV_CLASS, ttl_vextsh_opn,      { V_NORMAL_OP,   0, V_EXTSH_FUNC } },
    {"ttl_viota",   1, TTL_IOTA_CLASS, ttl_viota_opn,      { V_NORMAL_OP,   0, V_IOTA_FUNC } },
    {"ttl_vskewh",  2, TTL_NVV_CLASS, ttl_vskewh_opn,      { V_NORMAL_OP,   0, V_SKEWH_FUNC } },
    {"ttl_vskewl",  2, TTL_NVV_CLASS, ttl_vskewl_opn,      { V_NORMAL_OP,   0, V_SKEWL_FUNC } },
    {"ttl_vldt",   2, TTL_MEM_CLASS, ttl_vldt_opn,         { V_LOAD_OP,   V_DTYPE_T, 0 } },
    {"ttl_vlds",   2, TTL_MEM_CLASS, ttl_vlds_opn,         { V_LOAD_OP,   V_DTYPE_S, 0 } },
    {"ttl_vldq",   2, TTL_MEM_CLASS, ttl_vldq_opn,         { V_LOAD_OP,   V_DTYPE_Q, 0 } },
    {"ttl_vldl",   2, TTL_MEM_CLASS, ttl_vldl_opn,         { V_LOAD_OP,   V_DTYPE_L, 0 } },
    {"ttl_vstt",   2, TTL_MEM_CLASS, ttl_vstt_opn,         { V_STORE_OP,   V_DTYPE_T, 0 } },
    {"ttl_vsts",   2, TTL_MEM_CLASS, ttl_vsts_opn,         { V_STORE_OP,   V_DTYPE_S, 0 } },
    {"ttl_vstq",   2, TTL_MEM_CLASS, ttl_vstq_opn,         { V_STORE_OP,   V_DTYPE_Q, 0 } },
    {"ttl_vstl",   2, TTL_MEM_CLASS, ttl_vstl_opn,         { V_STORE_OP,   V_DTYPE_L, 0 } },
    {"ttl_vgathq", 3, TTL_GATH_CLASS, ttl_vgathq_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_GATHQ_FUNC } },
    {"ttl_vgathl", 3, TTL_GATH_CLASS, ttl_vgathl_opn,      { V_NORMAL_OP,   V_DTYPE_L, V_GATHL_FUNC } },
    {"ttl_vgatht", 3, TTL_GATH_CLASS, ttl_vgatht_opn,      { V_NORMAL_OP,   V_DTYPE_T, V_GATHT_FUNC } },
    {"ttl_vgaths", 3, TTL_GATH_CLASS, ttl_vgaths_opn,      { V_NORMAL_OP,   V_DTYPE_S, V_GATHS_FUNC } },
    {"ttl_vscatq", 3, TTL_SCAT_CLASS, ttl_vscatq_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_SCATQ_FUNC } },
    {"ttl_vscatl", 3, TTL_SCAT_CLASS, ttl_vscatl_opn,      { V_NORMAL_OP,   V_DTYPE_L, V_SCATL_FUNC } },
    {"ttl_vscatt", 3, TTL_SCAT_CLASS, ttl_vscatt_opn,      { V_NORMAL_OP,   V_DTYPE_T, V_SCATT_FUNC } },
    {"ttl_vscats", 3, TTL_SCAT_CLASS, ttl_vscats_opn,      { V_NORMAL_OP,   V_DTYPE_S, V_SCATS_FUNC } },
    {"ttl_vncgathq", 3, TTL_GATH_CLASS, ttl_vncgathq_opn,  { V_NORMAL_OP,   V_DTYPE_Q, V_NCGATHQ_FUNC } },
    {"ttl_vncgathl", 3, TTL_GATH_CLASS, ttl_vncgathl_opn,  { V_NORMAL_OP,   V_DTYPE_L, V_NCGATHL_FUNC } },
    {"ttl_vncgatht", 3, TTL_GATH_CLASS, ttl_vncgatht_opn,  { V_NORMAL_OP,   V_DTYPE_T, V_NCGATHT_FUNC } },
    {"ttl_vncgaths", 3, TTL_GATH_CLASS, ttl_vncgaths_opn,  { V_NORMAL_OP,   V_DTYPE_S, V_NCGATHS_FUNC } },
    {"ttl_vncscatq", 3, TTL_SCAT_CLASS, ttl_vncscatq_opn,  { V_NORMAL_OP,   V_DTYPE_Q, V_NCSCATQ_FUNC } },
    {"ttl_vncscatl", 3, TTL_SCAT_CLASS, ttl_vncscatl_opn,  { V_NORMAL_OP,   V_DTYPE_L, V_NCSCATL_FUNC } },
    {"ttl_vncscatt", 3, TTL_SCAT_CLASS, ttl_vncscatt_opn,  { V_NORMAL_OP,   V_DTYPE_T, V_NCSCATT_FUNC } },
    {"ttl_vncscats", 3, TTL_SCAT_CLASS, ttl_vncscats_opn,  { V_NORMAL_OP,   V_DTYPE_S, V_NCSCATS_FUNC } },
    {"ttl_vvaddt", 3, TTL_VVV_CLASS, ttl_vvaddt_opn,       { V_NORMAL_OP,   V_DTYPE_T, V_ADDT_FUNC } },
    {"ttl_vvadds", 3, TTL_VVV_CLASS, ttl_vvadds_opn,       { V_NORMAL_OP,   V_DTYPE_S, V_ADDS_FUNC } },
    {"ttl_vvaddq", 3, TTL_VVV_CLASS, ttl_vvaddq_opn,       { V_NORMAL_OP,   V_DTYPE_Q, V_ADDQ_FUNC } },
    {"ttl_vvaddl", 3, TTL_VVV_CLASS, ttl_vvaddl_opn,       { V_NORMAL_OP,   V_DTYPE_L, V_ADDL_FUNC } },
    {"ttl_vvsubt", 3, TTL_VVV_CLASS, ttl_vvsubt_opn,       { V_NORMAL_OP,   V_DTYPE_T, V_SUBT_FUNC } },
    {"ttl_vvsubs", 3, TTL_VVV_CLASS, ttl_vvsubs_opn,       { V_NORMAL_OP,   V_DTYPE_S, V_SUBS_FUNC } },
    {"ttl_vvsubq", 3, TTL_VVV_CLASS, ttl_vvsubq_opn,       { V_NORMAL_OP,   V_DTYPE_Q, V_SUBQ_FUNC } },
    {"ttl_vvsubl", 3, TTL_VVV_CLASS, ttl_vvsubl_opn,       { V_NORMAL_OP,   V_DTYPE_L, V_SUBL_FUNC } },
    {"ttl_vvmult", 3, TTL_VVV_CLASS, ttl_vvmult_opn,       { V_NORMAL_OP,   V_DTYPE_T, V_MULT_FUNC } },
    {"ttl_vvmuls", 3, TTL_VVV_CLASS, ttl_vvmuls_opn,       { V_NORMAL_OP,   V_DTYPE_S, V_MULS_FUNC } },
    {"ttl_vvmulq", 3, TTL_VVV_CLASS, ttl_vvmulq_opn,       { V_NORMAL_OP,   V_DTYPE_Q, V_MULQ_FUNC } },
    {"ttl_vvmull", 3, TTL_VVV_CLASS, ttl_vvmull_opn,       { V_NORMAL_OP,   V_DTYPE_L, V_MULL_FUNC } },
    {"ttl_vvdivt", 3, TTL_VVV_CLASS, ttl_vvdivt_opn,       { V_NORMAL_OP,   V_DTYPE_T, V_DIVT_FUNC } },
    {"ttl_vvdivs", 3, TTL_VVV_CLASS, ttl_vvdivs_opn,       { V_NORMAL_OP,   V_DTYPE_S, V_DIVS_FUNC } },
    {"ttl_vsqrtt", 2, TTL_NVV_CLASS, ttl_vsqrtt_opn,       { V_NORMAL_OP,   V_DTYPE_T, V_SQRTT_FUNC} },
    {"ttl_vsqrts", 2, TTL_NVV_CLASS, ttl_vsqrts_opn,       { V_NORMAL_OP,   V_DTYPE_S, V_SQRTS_FUNC} },
    {"ttl_vsaddt", 3, TTL_VFV_CLASS, ttl_vsaddt_opn,       { V_NORMAL_OP,   V_DTYPE_T, V_ADDT_FUNC } },
    {"ttl_vsadds", 3, TTL_VFV_CLASS, ttl_vsadds_opn,       { V_NORMAL_OP,   V_DTYPE_S, V_ADDS_FUNC } },
    {"ttl_vssubt", 3, TTL_VFV_CLASS, ttl_vssubt_opn,       { V_NORMAL_OP,   V_DTYPE_T, V_SUBT_FUNC } },
    {"ttl_vssubs", 3, TTL_VFV_CLASS, ttl_vssubs_opn,       { V_NORMAL_OP,   V_DTYPE_S, V_SUBS_FUNC } },
    {"ttl_vsmult", 3, TTL_VFV_CLASS, ttl_vsmult_opn,       { V_NORMAL_OP,   V_DTYPE_T, V_MULT_FUNC } },
    {"ttl_vsmuls", 3, TTL_VFV_CLASS, ttl_vsmuls_opn,       { V_NORMAL_OP,   V_DTYPE_S, V_MULS_FUNC } },
    {"ttl_vsdivt", 3, TTL_VFV_CLASS, ttl_vsdivt_opn,       { V_NORMAL_OP,   V_DTYPE_T, V_DIVT_FUNC } },
    {"ttl_vsdivs", 3, TTL_VFV_CLASS, ttl_vsdivs_opn,       { V_NORMAL_OP,   V_DTYPE_S, V_DIVS_FUNC } },
    {"ttl_vsaddq", 3, TTL_VIV_CLASS, ttl_vsaddq_opn,       { V_NORMAL_OP,   V_DTYPE_Q, V_ADDQ_FUNC } },
    {"ttl_vsaddl", 3, TTL_VIV_CLASS, ttl_vsaddl_opn,       { V_NORMAL_OP,   V_DTYPE_L, V_ADDL_FUNC } },
    {"ttl_vssubq", 3, TTL_VIV_CLASS, ttl_vssubq_opn,       { V_NORMAL_OP,   V_DTYPE_Q, V_SUBQ_FUNC } },
    {"ttl_vssubl", 3, TTL_VIV_CLASS, ttl_vssubl_opn,       { V_NORMAL_OP,   V_DTYPE_L, V_SUBL_FUNC } },
    {"ttl_vsmulq", 3, TTL_VIV_CLASS, ttl_vsmulq_opn,       { V_NORMAL_OP,   V_DTYPE_Q, V_MULQ_FUNC } },
    {"ttl_vsmull", 3, TTL_VIV_CLASS, ttl_vsmull_opn,       { V_NORMAL_OP,   V_DTYPE_L, V_MULL_FUNC } },
    {"ttl_vsand", 3, TTL_VIV_CLASS, ttl_vsand_opn,         { V_NORMAL_OP,   V_DTYPE_Q, V_AND_FUNC } },
    {"ttl_vsbis", 3, TTL_VIV_CLASS, ttl_vsbis_opn,         { V_NORMAL_OP,   V_DTYPE_Q, V_BIS_FUNC } },
    {"ttl_vsxor", 3, TTL_VIV_CLASS, ttl_vsxor_opn,         { V_NORMAL_OP,   V_DTYPE_Q, V_XOR_FUNC } },
    {"ttl_vsbic", 3, TTL_VIV_CLASS, ttl_vsbic_opn,         { V_NORMAL_OP,   V_DTYPE_Q, V_BIC_FUNC } },
    {"ttl_vsornot", 3, TTL_VIV_CLASS, ttl_vsornot_opn,     { V_NORMAL_OP,   V_DTYPE_Q, V_ORNOT_FUNC } },
    {"ttl_vseqv", 3, TTL_VIV_CLASS, ttl_vseqv_opn,         { V_NORMAL_OP,   V_DTYPE_Q, V_EQV_FUNC } },
    {"ttl_vvand", 3, TTL_VVV_CLASS, ttl_vvand_opn,         { V_NORMAL_OP,   V_DTYPE_Q, V_AND_FUNC } },
    {"ttl_vvbis", 3, TTL_VVV_CLASS, ttl_vvbis_opn,         { V_NORMAL_OP,   V_DTYPE_Q, V_BIS_FUNC } },
    {"ttl_vvxor", 3, TTL_VVV_CLASS, ttl_vvxor_opn,         { V_NORMAL_OP,   V_DTYPE_Q, V_XOR_FUNC } },
    {"ttl_vvbic", 3, TTL_VVV_CLASS, ttl_vvbic_opn,         { V_NORMAL_OP,   V_DTYPE_Q, V_BIC_FUNC } },
    {"ttl_vvornot", 3, TTL_VVV_CLASS, ttl_vvornot_opn,     { V_NORMAL_OP,   V_DTYPE_Q, V_ORNOT_FUNC } },
    {"ttl_vveqv", 3, TTL_VVV_CLASS, ttl_vveqv_opn,         { V_NORMAL_OP,   V_DTYPE_Q, V_EQV_FUNC } },
    {"ttl_vvcmpeq",3, TTL_VVV_CLASS, ttl_vvcmpeq_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_CMPEQ_FUNC } },
    {"ttl_vvcmple",3, TTL_VVV_CLASS, ttl_vvcmple_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_CMPLE_FUNC } },
    {"ttl_vvcmplt",3, TTL_VVV_CLASS, ttl_vvcmplt_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_CMPLT_FUNC } },
    {"ttl_vscmpeq",3, TTL_VIV_CLASS, ttl_vscmpeq_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_CMPEQ_FUNC } },
    {"ttl_vscmple",3, TTL_VIV_CLASS, ttl_vscmple_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_CMPLE_FUNC } },
    {"ttl_vscmplt",3, TTL_VIV_CLASS, ttl_vscmplt_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_CMPLT_FUNC } },
    {"ttl_vvcmpule",3, TTL_VVV_CLASS, ttl_vvcmpule_opn,    { V_NORMAL_OP,   V_DTYPE_Q, V_CMPULE_FUNC } },
    {"ttl_vvcmpult",3, TTL_VVV_CLASS, ttl_vvcmpult_opn,    { V_NORMAL_OP,   V_DTYPE_Q, V_CMPULT_FUNC } },
    {"ttl_vscmpule",3, TTL_VIV_CLASS, ttl_vscmpule_opn,    { V_NORMAL_OP,   V_DTYPE_Q, V_CMPULE_FUNC } },
    {"ttl_vscmpult",3, TTL_VIV_CLASS, ttl_vscmpult_opn,    { V_NORMAL_OP,   V_DTYPE_Q, V_CMPULT_FUNC } },
    {"ttl_vvcmpbge",3, TTL_VVV_CLASS, ttl_vvcmpbge_opn,    { V_NORMAL_OP,   V_DTYPE_Q, V_CMPBGE_FUNC } },
    {"ttl_vscmpbge",3, TTL_VIV_CLASS, ttl_vscmpbge_opn,    { V_NORMAL_OP,   V_DTYPE_Q, V_CMPBGE_FUNC } },
    {"ttl_vcvtqs",2, TTL_NVV_CLASS, ttl_vcvtqs_opn,        { V_NORMAL_OP,   V_DTYPE_Q, V_CVTQS_FUNC } },
    {"ttl_vcvtqt",2, TTL_NVV_CLASS, ttl_vcvtqt_opn,        { V_NORMAL_OP,   V_DTYPE_Q, V_CVTQT_FUNC } },
    {"ttl_vcvtst",2, TTL_NVV_CLASS, ttl_vcvtst_opn,        { V_NORMAL_OP,   V_DTYPE_Q, V_CVTST_FUNC } },
    {"ttl_vcvttq",2, TTL_NVV_CLASS, ttl_vcvttq_opn,        { V_NORMAL_OP,   V_DTYPE_Q, V_CVTTQ_FUNC } },
    {"ttl_vcvtts",2, TTL_NVV_CLASS, ttl_vcvtts_opn,        { V_NORMAL_OP,   V_DTYPE_Q, V_CVTTS_FUNC } },
    {"ttl_vsextbl",3, TTL_VIV_CLASS, ttl_vsextbl_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_EXTBL_FUNC } },
    {"ttl_vsextwl",3, TTL_VIV_CLASS, ttl_vsextwl_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_EXTWL_FUNC } },
    {"ttl_vsextll",3, TTL_VIV_CLASS, ttl_vsextll_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_EXTLL_FUNC } },
    {"ttl_vsextql",3, TTL_VIV_CLASS, ttl_vsextql_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_EXTQL_FUNC } },
    {"ttl_vsextwh",3, TTL_VIV_CLASS, ttl_vsextwh_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_EXTWH_FUNC } },
    {"ttl_vsextlh",3, TTL_VIV_CLASS, ttl_vsextlh_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_EXTLH_FUNC } },
    {"ttl_vsextqh",3, TTL_VIV_CLASS, ttl_vsextqh_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_EXTQH_FUNC } },
    {"ttl_vvextbl",3, TTL_VVV_CLASS, ttl_vvextbl_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_EXTBL_FUNC } },
    {"ttl_vvextwl",3, TTL_VVV_CLASS, ttl_vvextwl_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_EXTWL_FUNC } },
    {"ttl_vvextll",3, TTL_VVV_CLASS, ttl_vvextll_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_EXTLL_FUNC } },
    {"ttl_vvextql",3, TTL_VVV_CLASS, ttl_vvextql_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_EXTQL_FUNC } },
    {"ttl_vvextwh",3, TTL_VVV_CLASS, ttl_vvextwh_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_EXTWH_FUNC } },
    {"ttl_vvextlh",3, TTL_VVV_CLASS, ttl_vvextlh_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_EXTLH_FUNC } },
    {"ttl_vvextqh",3, TTL_VVV_CLASS, ttl_vvextqh_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_EXTQH_FUNC } },
    {"ttl_vsinsbl",3, TTL_VIV_CLASS, ttl_vsinsbl_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_INSBL_FUNC } },
    {"ttl_vsinswl",3, TTL_VIV_CLASS, ttl_vsinswl_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_INSWL_FUNC } },
    {"ttl_vsinsll",3, TTL_VIV_CLASS, ttl_vsinsll_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_INSLL_FUNC } },
    {"ttl_vsinsql",3, TTL_VIV_CLASS, ttl_vsinsql_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_INSQL_FUNC } },
    {"ttl_vsinswh",3, TTL_VIV_CLASS, ttl_vsinswh_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_INSWH_FUNC } },
    {"ttl_vsinslh",3, TTL_VIV_CLASS, ttl_vsinslh_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_INSLH_FUNC } },
    {"ttl_vsinsqh",3, TTL_VIV_CLASS, ttl_vsinsqh_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_INSQH_FUNC } },
    {"ttl_vvinsbl",3, TTL_VVV_CLASS, ttl_vvinsbl_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_INSBL_FUNC } },
    {"ttl_vvinswl",3, TTL_VVV_CLASS, ttl_vvinswl_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_INSWL_FUNC } },
    {"ttl_vvinsll",3, TTL_VVV_CLASS, ttl_vvinsll_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_INSLL_FUNC } },
    {"ttl_vvinsql",3, TTL_VVV_CLASS, ttl_vvinsql_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_INSQL_FUNC } },
    {"ttl_vvinswh",3, TTL_VVV_CLASS, ttl_vvinswh_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_INSWH_FUNC } },
    {"ttl_vvinslh",3, TTL_VVV_CLASS, ttl_vvinslh_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_INSLH_FUNC } },
    {"ttl_vvinsqh",3, TTL_VVV_CLASS, ttl_vvinsqh_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_INSQH_FUNC } },
    {"ttl_vvmerg", 3, TTL_VVV_CLASS, ttl_vvmerg_opn,       { V_NORMAL_OP,   V_DTYPE_Q, V_MERG_FUNC } },
    {"ttl_vsmergq",3, TTL_VIV_CLASS, ttl_vsmergq_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_MERGQ_FUNC } },
    {"ttl_vsmergt",3, TTL_VFV_CLASS, ttl_vsmergt_opn,      { V_NORMAL_OP,   V_DTYPE_T, V_MERGT_FUNC } },
    {"ttl_vvmskbl",3, TTL_VVV_CLASS, ttl_vvmskbl_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_MSKBL_FUNC } },
    {"ttl_vvmskwl",3, TTL_VVV_CLASS, ttl_vvmskwl_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_MSKWL_FUNC } },
    {"ttl_vvmskll",3, TTL_VVV_CLASS, ttl_vvmskll_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_MSKLL_FUNC } },
    {"ttl_vvmskql",3, TTL_VVV_CLASS, ttl_vvmskql_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_MSKQL_FUNC } },
    {"ttl_vvmskwh",3, TTL_VVV_CLASS, ttl_vvmskwh_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_MSKWH_FUNC } },
    {"ttl_vvmsklh",3, TTL_VVV_CLASS, ttl_vvmsklh_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_MSKLH_FUNC } },
    {"ttl_vvmskqh",3, TTL_VVV_CLASS, ttl_vvmskqh_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_MSKQH_FUNC } },
    {"ttl_vsmskbl",3, TTL_VIV_CLASS, ttl_vvmskbl_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_MSKBL_FUNC } },
    {"ttl_vsmskwl",3, TTL_VIV_CLASS, ttl_vvmskwl_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_MSKWL_FUNC } },
    {"ttl_vsmskll",3, TTL_VIV_CLASS, ttl_vvmskll_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_MSKLL_FUNC } },
    {"ttl_vsmskql",3, TTL_VIV_CLASS, ttl_vvmskql_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_MSKQL_FUNC } },
    {"ttl_vsmskwh",3, TTL_VIV_CLASS, ttl_vvmskwh_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_MSKWH_FUNC } },
    {"ttl_vsmsklh",3, TTL_VIV_CLASS, ttl_vvmsklh_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_MSKLH_FUNC } },
    {"ttl_vsmskqh",3, TTL_VIV_CLASS, ttl_vvmskqh_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_MSKQH_FUNC } },
    {"ttl_vssra",  3, TTL_VIV_CLASS, ttl_vssra_opn,        { V_NORMAL_OP,   V_DTYPE_Q, V_SRA_FUNC } },
    {"ttl_vvsra",  3, TTL_VVV_CLASS, ttl_vssra_opn,        { V_NORMAL_OP,   V_DTYPE_Q, V_SRA_FUNC } },
    {"ttl_vssll",  3, TTL_VIV_CLASS, ttl_vssll_opn,        { V_NORMAL_OP,   V_DTYPE_Q, V_SLL_FUNC } },
    {"ttl_vvsll",  3, TTL_VVV_CLASS, ttl_vssll_opn,        { V_NORMAL_OP,   V_DTYPE_Q, V_SLL_FUNC } },
    {"ttl_vssrl",  3, TTL_VIV_CLASS, ttl_vssrl_opn,        { V_NORMAL_OP,   V_DTYPE_Q, V_SRL_FUNC } },
    {"ttl_vvsrl",  3, TTL_VVV_CLASS, ttl_vssrl_opn,        { V_NORMAL_OP,   V_DTYPE_Q, V_SRL_FUNC } },
    {"ttl_vs4addq",3, TTL_VVV_CLASS, ttl_vs4addq_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_S4ADDQ_FUNC } },
    {"ttl_vs8addq",3, TTL_VVV_CLASS, ttl_vs8addq_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_S8ADDQ_FUNC } },
    {"ttl_vs4addl",3, TTL_VVV_CLASS, ttl_vs4addl_opn,      { V_NORMAL_OP,   V_DTYPE_L, V_S4ADDL_FUNC } },
    {"ttl_vs8addl",3, TTL_VVV_CLASS, ttl_vs8addl_opn,      { V_NORMAL_OP,   V_DTYPE_L, V_S8ADDL_FUNC } },
    {"ttl_vs4subq",3, TTL_VVV_CLASS, ttl_vs4subq_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_S4SUBQ_FUNC } },
    {"ttl_vs8subq",3, TTL_VVV_CLASS, ttl_vs8subq_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_S8SUBQ_FUNC } },
    {"ttl_vs4subl",3, TTL_VVV_CLASS, ttl_vs4subl_opn,      { V_NORMAL_OP,   V_DTYPE_L, V_S4SUBL_FUNC } },
    {"ttl_vs8subl",3, TTL_VVV_CLASS, ttl_vs8subl_opn,      { V_NORMAL_OP,   V_DTYPE_L, V_S8SUBL_FUNC } },
    {"ttl_vvumulh",3, TTL_VVV_CLASS, ttl_vvumulh_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_UMULH_FUNC } },
    {"ttl_vsumulh",3, TTL_VIV_CLASS, ttl_vvumulh_opn,      { V_NORMAL_OP,   V_DTYPE_Q, V_UMULH_FUNC } },
    {"ttl_vvzap",  3, TTL_VVV_CLASS, ttl_vvzap_opn,        { V_NORMAL_OP,   V_DTYPE_Q, V_ZAP_FUNC } },
    {"ttl_vszap",  3, TTL_VIV_CLASS, ttl_vszap_opn,        { V_NORMAL_OP,   V_DTYPE_Q, V_ZAP_FUNC } },
    {"ttl_vvzapnot",3,TTL_VVV_CLASS, ttl_vvzapnot_opn,     { V_NORMAL_OP,   V_DTYPE_Q, V_ZAPNOT_FUNC } },
    {"ttl_vszapnot",3,TTL_VIV_CLASS, ttl_vszapnot_opn,     { V_NORMAL_OP,   V_DTYPE_Q, V_ZAPNOT_FUNC } },
    {"ttl_vvcmpteq",3,TTL_VVV_CLASS, ttl_vvcmpteq_opn,     { V_NORMAL_OP,   V_DTYPE_T, V_CMPTEQ_FUNC } },
    {"ttl_vvcmptle",3,TTL_VVV_CLASS, ttl_vvcmptle_opn,     { V_NORMAL_OP,   V_DTYPE_T, V_CMPTLE_FUNC } },
    {"ttl_vvcmptlt",3,TTL_VVV_CLASS, ttl_vvcmptlt_opn,     { V_NORMAL_OP,   V_DTYPE_T, V_CMPTLT_FUNC } },
    {"ttl_vvcmptun",3,TTL_VVV_CLASS, ttl_vvcmptun_opn,     { V_NORMAL_OP,   V_DTYPE_T, V_CMPTUN_FUNC } },
    {"ttl_vscmpteq",3,TTL_VFV_CLASS, ttl_vscmpteq_opn,     { V_NORMAL_OP,   V_DTYPE_T, V_CMPTEQ_FUNC } },
    {"ttl_vscmptle",3,TTL_VFV_CLASS, ttl_vscmptle_opn,     { V_NORMAL_OP,   V_DTYPE_T, V_CMPTLE_FUNC } },
    {"ttl_vscmptlt",3,TTL_VFV_CLASS, ttl_vscmptlt_opn,     { V_NORMAL_OP,   V_DTYPE_T, V_CMPTLT_FUNC } },
    {"ttl_vscmptun",3,TTL_VFV_CLASS, ttl_vscmptun_opn,     { V_NORMAL_OP,   V_DTYPE_T, V_CMPTUN_FUNC } },
    {"ttl_vcmpr",   3,TTL_NVV_CLASS, ttl_vcmpr_opn,        { V_NORMAL_OP,   V_DTYPE_T, V_CMPR_FUNC } },
    {"ttl_vsynch",  0,TTL_SYNCH_CLASS,ttl_vsynch_opn,      { V_NORMAL_OP, 0, V_SYNCH_FUNC} },
	/* all the prefetching instruction set */
    {"ttl_vldpf",   1, TTL_MEM_CLASS, ttl_vldpf_opn,       { V_LOAD_OP,   V_DTYPE_PF, 0 } },
    {"ttl_vstpf",   1, TTL_MEM_CLASS, ttl_vstpf_opn,       { V_STORE_OP,   V_DTYPE_PF, 0 } },
    {"ttl_vgathpf", 2, TTL_GATH_CLASS,ttl_vgathpf_opn,     { V_NORMAL_OP,   V_DTYPE_PF, V_GATHPF_FUNC } },
    {"ttl_vscatpf", 2, TTL_SCAT_CLASS,ttl_vscatpf_opn,     { V_NORMAL_OP,   V_DTYPE_PF, V_SCATPF_FUNC } },
    {"ttl_vncgathpf", 2, TTL_GATH_CLASS,ttl_vncgathpf_opn,     { V_NORMAL_OP,   V_DTYPE_PF, V_NCGATHPF_FUNC } },
    {"ttl_vncscatpf", 2, TTL_SCAT_CLASS,ttl_vncscatpf_opn,     { V_NORMAL_OP,   V_DTYPE_PF, V_NCSCATPF_FUNC } },
    {"ttl_vdrainm0", 0, TTL_VVV_CLASS,ttl_vdrainm0_opn,	{ V_NORMAL_OP,0,  V_DRAINM0_FUNC } },
    {"ttl_vdrainm1", 0, TTL_VVV_CLASS,ttl_vdrainm1_opn,	{ V_NORMAL_OP,0,  V_DRAINM1_FUNC } },
    {"ttl_vdrainm2", 0, TTL_VVV_CLASS,ttl_vdrainm2_opn,	{ V_NORMAL_OP,0,  V_DRAINM2_FUNC } },
    {"ttl_vdrainv0", 0, TTL_VVV_CLASS,ttl_vdrainv0_opn,	{ V_NORMAL_OP,0,  V_DRAINV0_FUNC } },
    {"ttl_vdrainv1", 0, TTL_VVV_CLASS,ttl_vdrainv1_opn,	{ V_NORMAL_OP,0,  V_DRAINV1_FUNC } },
    {"ttl_vdrainv2", 0, TTL_VVV_CLASS,ttl_vdrainv2_opn,	{ V_NORMAL_OP,0,  V_DRAINV2_FUNC } },
    {"ttl_mvfvr", 3, TTL_MVFVR_CLASS,ttl_mvfvr_opn,	{ V_NORMAL_OP, V_DTYPE_Q,  V_MVFVR_FUNC } }
};

#endif /* NEW_TTL */

#ifdef NEW_TLDS

#include "tlds_isa.h"

typedef struct {
  tlds_opcode_t	opcode;
} tlds_encoding_t;

typedef enum {
  TLDS_CMD,                      /* TLDS command (no src regs) */
  TLDS_1ARG,
  TLDS_MAX_CLASS
} tlds_class_t;

typedef struct {
  char		*name;              /* TLDS routine name (as it appears in a.out file) */
  char 		*entrypc;           /* Entry point pc (target of JSR tlds_subroutine) */
  int		nargs;              /* number of arguments expected for this TLDS instruction */
  tlds_class_t	class;          /* Generic class of instruction: see tlds_class_t */
  int		opnum;              /* Index into the op_desc desc_table */
  tlds_encoding_t	encoding;
} tlds_routine_t;


#define TLDS_MAX_INSNS 5
static tlds_routine_t tlds_routines[TLDS_MAX_INSNS] = {

 { "tlds_bs",	 0, 0, TLDS_CMD,  tlds_bs_opn, 		{ TLDS_BS_OP } },
 { "tlds_bns",	 0, 0, TLDS_CMD,  tlds_bns_opn, 	{ TLDS_BNS_OP } },
 { "tlds_q",	 0, 0, TLDS_CMD,  tlds_q_opn, 		{ TLDS_Q_OP } },
 { "tlds_arm",	 0, 1, TLDS_1ARG, tlds_arm_opn, 	{ TLDS_ARM_OP } },
 { "tlds_en",	 0, 1, TLDS_1ARG, tlds_en_opn, 		{ TLDS_EN_OP } }
};

#endif /* NEW_TLDS */

#endif /* _TTL_LOADER_H_ */

