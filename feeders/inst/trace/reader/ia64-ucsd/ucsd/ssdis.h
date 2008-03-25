/*************************************************************
 *                                                           *
 *                                                           *
 *                  SIMPLE SCALAR DECODER                    *
 *                                                           *
 *                                                           *
 *************************************************************/
/* 
 * This file contains the IA64 decoder for UCSD trace and simulation
 * tools, and is build into libssdis.a.  This library takes a 
 * PC into the text section, uses bfd to access that opcode, decodes
 * and returns a record of salient architectural instruction information.
 * This library depends on libbfd.a and libiberty.a.  
 * 
 * ssdis.c is derived from the GNU binutils/objdump.c and is covered by
 * the GNU GPL.  
 *
 * Author: Weihaw Chuang
 * Date:   Dec 2000.
 *
 * Any rights not precluded by the GNU GPL, are reserved 
 * by University of California, San Diego, and the UC Regents. 
 * Copyright 2001.
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
 * 
 */
/* 
 * Revision 1.19  2001/10/23 18:39:53  wei
 * Coincides with validated trace.testcase version.  Includes support for
 * phi attribute.
 *
 * Revision 1.18  2001/09/29 03:39:21  wei
 * partial bug fix for annotations
 *
 * Revision 1.13  2001/07/25 20:17:50  wei
 * checkin for VSSAD
 *
 * Revision 1.12  2001/06/04 10:24:19  wei
 * version for rse rename and op injection
 *
 * Revision 1.10  2001/05/27 20:21:01  wei
 * minor bug fixes
 *
 * Revision 1.9  2001/05/24 06:35:18  wei
 * This version contains Simple annotations
 *
 * Revision 1.8  2001/05/19 10:47:59  wei
 * first sort of working uArch version
 *
 * Revision 1.7  2001/05/13 06:46:07  wei
 * Updated version to determine effective address.  also new iterator macro
 *
 */


#ifndef SSDIS_H
#define SSDIS_H

// ASIM core
#include "asim/syntax.h"

// ASIM local module
#include "opcode_ia64.h"
#include "ia64-enum.h"


bool ssi_err_msg(char*,INT32,char*);
#define SSI_ERROR(msg)  ssi_err_msg(__FILE__, __LINE__, (msg))

#define OPER_ITER_LIMIT              5
#define SSDIS_DEBUG                  1

/* 
 * operand bv access macros 
 */
#define SSI_BV_OGET(op, index, code) ((op)->opbv[index] & (code))
#define SSI_BV_OSET(op, index, code) ((op)->opbv[index] |= (code))
#define SSI_BV_VALID                 0x1
#define SSI_BV_REG                   0x2
#define SSI_BV_INPUT                 0x4
#define SSI_BV_OUTPUT                0x8
#define SSI_BV_IREG                  0x10
#define SSI_BV_BREG                  0x20
#define SSI_BV_FREG                  0x40
#define SSI_BV_PREG                  0x80
#define SSI_BV_CONST                0x100
#define SSI_BV_AREG                 0x200
#define SSI_BV_ARPFS                0x400
#define SSI_BV_PR                   0x800
#define SSI_BV_BASE                0x1000
#define SSI_BV_EXTCONST            0x2000

/*
 * begin inst attributes 
 */
#define SSI_BV_GET(op, code)         ((op)->bv & (code))
#define SSI_BV_SET(op, code)         ((op)->bv |= (code))
#define SSI_BV_BR                    0x1
#define SSI_BV_BRCOND                0x2
#define SSI_BV_BRUNCOND              0x4
#define SSI_BV_CALL                  0x8
#define SSI_BV_RET                  0x10
#define SSI_BV_MEM                  0x20
#define SSI_BV_NOP                  0x40
#define SSI_BV_EXTENDED             0x80
#define SSI_BV_CTRL                0x100
#define SSI_BV_TRAP                0x200
#define SSI_BV_DIRJMP              0x400
#define SSI_BV_INDIRJMP            0x800
#define SSI_BV_CHK                0x1000
#define SSI_BV_PRED_DEF           0x2000
#define SSI_BV_PRED_DEF2          0x4000
#define SSI_BV_ST                 0x8000
#define SSI_BV_LD                0x10000
#define SSI_BV_POSTINC           0x40000
#define SSI_BV_HAS_EA            0x80000

/* for beth simon */
#define SSI_BV_TK               0x100000
#define SSI_BV_NT               0x200000
#define SSI_BV_DYNAMIC          0x400000
#define SSI_BV_STATIC           0x800000

/* this is set for RSE operations excluding
 * br.wtop br.ctop br.wexit and br.cexit
 * currently this is br.call br.ret, cover,
 * alloc, flushrs, loadrs, rfi, break
 */
#define SSI_BV_RSE             0x1000000
/* RSE rotation register operation
 * br.wtop br.ctop br.wexit and br.cexit
 */
#define SSI_BV_SWP             0x2000000
#define SSI_BV_CLOOP           0x4000000

#define SSI_BV_MTPR            0x8000000
#define SSI_BV_MFPR           0x10000000
#define SSI_BV_PHI            0x20000000
/*
 * end of inst attributes 
 */

/* operand unique id */
#define SSI_OP_ID(op, index) ((op)->op_id[index])

/* unique register ranges id's */
#define IA64_REGID_R0           1
#define IA64_REGID_IREG_BASE    1
#define IA64_REGID_IREG_STATIC  32
#define IA64_REGID_IREG_STACK_BASE (IA64_REGID_IREG_BASE+IA64_REGID_IREG_STATIC)
#define IA64_REGID_IREG_MAX     128
#define IA64_REGID_IREG_HI      (IA64_REGID_IREG_MAX + IA64_REGID_IREG_BASE - 1)

#define IA64_REGID_FREG_BASE    (IA64_REGID_IREG_MAX + IA64_REGID_IREG_BASE)
#define IA64_REGID_FREG_F0      (IA64_REGID_IREG_BASE + 0)
#define IA64_REGID_FREG_F1      (IA64_REGID_IREG_BASE + 1)
#define IA64_REGID_FREG_STATIC  32
#define IA64_REGID_FREG_ROT_BASE (IA64_REGID_FREG_STATIC+IA64_REGID_FREG_BASE)
#define IA64_REGID_FREG_MAX     128
#define IA64_REGID_FREG_ROT_MAX 96
#define IA64_REGID_FREG_HI      (IA64_REGID_FREG_MAX + IA64_REGID_FREG_BASE - 1)

#define IA64_REGID_BREG_BASE    (IA64_REGID_FREG_MAX + IA64_REGID_FREG_BASE)
#define IA64_REGID_BREG_MAX     8
#define IA64_REGID_BREG_HI      (IA64_REGID_BREG_MAX + IA64_REGID_BREG_BASE - 1)

#define IA64_REGID_PREG_BASE    (IA64_REGID_BREG_MAX + IA64_REGID_BREG_BASE)
#define IA64_REGID_PREG_MAX     64
#define IA64_REGID_PREG_ROT_MAX 48
#define IA64_REGID_PREG_P0      (IA64_REGID_PREG_BASE + 0)
#define IA64_REGID_PREG_STATIC  16
#define IA64_REGID_PREG_ROT_BASE (IA64_REGID_PREG_STATIC+IA64_REGID_PREG_BASE)
#define IA64_REGID_PREG_HI      (IA64_REGID_PREG_MAX + IA64_REGID_PREG_BASE - 1)

#define IA64_REGID_CREG_BASE    (IA64_REGID_PREG_MAX + IA64_REGID_PREG_BASE)
#define IA64_REGID_CREG_MAX     128
#define IA64_REGID_CREG_HI      (IA64_REGID_CREG_MAX + IA64_REGID_CREG_BASE - 1)

#define IA64_REGID_AREG_BASE    (IA64_REGID_CREG_MAX + IA64_REGID_CREG_BASE)
#define IA64_REGID_AREG_MAX     128
#define IA64_REGID_AREG_HI      (IA64_REGID_AREG_MAX + IA64_REGID_AREG_BASE - 1)

/* special registers */
#define IA64_REGID_ARPFS        (IA64_REGID_AREG_HI+1)
#define IA64_REGID_PR           (IA64_REGID_AREG_HI+2)
/* also represents writes to CFM */
#define IA64_REGID_GR           (IA64_REGID_AREG_HI+3)
/* rse address base */
#define IA64_REGID_RSE_BASE0    (IA64_REGID_AREG_HI+4)
#define IA64_REGID_RSE_BASE1    (IA64_REGID_AREG_HI+5)
#define IA64_REGID_RSE_BASE2    (IA64_REGID_AREG_HI+6)
#define IA64_REGID_RSE_BASE3    (IA64_REGID_AREG_HI+7)
#define IA64_REGID_RSE_BASE4    (IA64_REGID_AREG_HI+8)
#define IA64_REGID_RSE_BASE5    (IA64_REGID_AREG_HI+9)
#define IA64_REGID_RSE_BASE6    (IA64_REGID_AREG_HI+10)
#define IA64_REGID_RSE_BASE7    (IA64_REGID_AREG_HI+11)

#define SSI_NUM_OPERAND         5

/* some useful macro given a ss_ia64_inst struct */
/* look in ia64.h */
#define SSI_UNIT_CLASS(rec)    ((rec)->opc->type)
#define SSI_IS_B_UNIT(rec)     ((rec)->opc->type==IA64_TYPE_B)
/* see table, IA-64 manual vol 3, page 4-3 */
#define SSI_MAJOR_OPC(rec)     (IA64_OP((rec)->opc->opcode))
#define SSI_OPC_ID(rec)        ((rec)->opc->id)
#define SSI_MEM_SIZE(rec)      ((rec)->mem_size)

/* this only used for conditional branches 
 * (watch out for indirect call)
*/
#define SSI_IA64_WH_34_33(rec)  (((rec)>>33) & 0x3)
#define SPTK_34_33              0x0
#define SPNT_34_33              0x1
#define DPTK_34_33              0x2
#define DPNT_34_33              0x3
#define SSI_WH_34_33(rec)       (SSI_IA64_WH_34_33((rec)->opc->opcode))
/* for indirect call */
#define SSI_IA64_WH_34_32(rec)  (((rec)>>32) & 0x7)
#define SPTK_34_32              0x1
#define SPNT_34_32              0x3
#define DPTK_34_32              0x5
#define DPNT_34_32              0x7
#define SSI_WH_34_32(rec)       (SSI_IA64_WH_34_32((rec)->opc->opcode))

extern char* ss_ia64_funame[];

#define SSI_GET_FU(rec)        ((rec)->opc->ia64_fu)
#define SSI_GET_UNIT(rec)      ((rec)->unit)
#define SSI_GET_FUNAME(rec)    (ss_ia64_funame[SSI_GET_FU(rec)])

#define SSI_FU_A(rec)          (((rec)->opc->ia64_fu >=FU_A_BEGIN) && \
				((rec)->opc->ia64_fu <=FU_A_END))
#define SSI_FU_B(rec)          (((rec)->opc->ia64_fu >=FU_B_BEGIN) && \
				((rec)->opc->ia64_fu <=FU_B_END))
#define SSI_FU_F(rec)          (((rec)->opc->ia64_fu >=FU_F_BEGIN) && \
				((rec)->opc->ia64_fu <=FU_F_END))
#define SSI_FU_I(rec)          (((rec)->opc->ia64_fu >=FU_I_BEGIN) && \
				((rec)->opc->ia64_fu <=FU_I_END))
#define SSI_FU_M(rec)          (((rec)->opc->ia64_fu >=FU_M_BEGIN) && \
				((rec)->opc->ia64_fu <=FU_M_END))
#define SSI_FU_X(rec)          (((rec)->opc->ia64_fu >=FU_X_BEGIN) && \
				((rec)->opc->ia64_fu <=FU_X_END))

typedef struct ss_ia64_inst ss_ia64_inst;


/* some operand accessor routines, the first gets the reg index, 
 * the second, gets the operand class, third gets the enum
 */
#define SSI_OPR_OUT(rec, i)    (*((((rec)->opbv[(i)] & SSI_BV_VALID) \
    || SSI_ERROR("incorrectly set opbv")), &(rec)->operands[(i)]))
#define SSI_OPRC_OUT(rec, i)   (*((((rec)->opbv[(i)] & SSI_BV_VALID) \
    || SSI_ERROR("incorrectly set opbv")), \
   &(rec)->odesc[(i)]))
#define SSI_OPRE_OUT(rec, i)   (*((((rec)->opbv[(i)] & SSI_BV_VALID) \
    || SSI_ERROR("incorrectly set opbv")), \
   &(rec)->opc->operands[(i)]))
#define SSI_OPRBV_OUT(rec, i)   (*((((rec)->opbv[(i)] & SSI_BV_VALID) \
    || SSI_ERROR("incorrectly set opbv")), \
   &(rec)->opbv[(i)]))


#define SSI_OPR_IN(rec, i)     (*((((rec)->opbv[(i)+(rec)->num_outputs] & SSI_BV_VALID) \
    || SSI_ERROR("incorrectly set opbv")), \
   &(rec)->operands[(i)+(rec)->num_outputs]))
#define SSI_OPRC_IN(rec, i)    (*((((rec)->opbv[(i)+(rec)->num_outputs] & SSI_BV_VALID) \
    || SSI_ERROR("incorrectly set opbv")), \
  &(rec)->odesc[(i)+(rec)->num_outputs]))
#define SSI_OPRE_IN(rec, i)    (*((((rec)->opbv[(i)+(rec)->num_outputs] & SSI_BV_VALID) \
    || SSI_ERROR("incorrectly set opbv")), \
  &(rec)->opc->operands[(i)+(rec)->num_outputs]))
#define SSI_OPRBV_IN(rec, i)    (*((((rec)->opbv[(i)+(rec)->num_outputs] & SSI_BV_VALID) \
    || SSI_ERROR("incorrectly set opbv")), \
  &(rec)->opbv[(i)+(rec)->num_outputs]))


/* 
 * effective address accessor functions
 */
#define SSI_OPR_HAS_EA(rec)     (SSI_BV_GET(rec, SSI_BV_HAS_EA))

#define SSI_OPR_GET_EA(rec)     ((rec)->ea)


/*******************************************/

/*
 * simple operand iterator 
 */
#define FOREACH_SSI_OPR_INDEX(rec, opr, index) {          \
    INT32 (opr) ;                                          \
    INT32(index);                                          \
    for((index)=0;(index)<(rec)->num_operands;(index)++) { \
         opr = (rec)->operands[(index)];              
       
#define ENDFE_SSI_OPR_INDEX }}

#define CONTINUE_SSI_OPR_INDEX {continue;} 

#define BREAK_SSI_OPR_INDEX    {break;}

/*******************************************/
/*
 * simple operand iterator 
 */
#define FOREACH_SSI_OPR_INDEX_REVERSE(rec, opr, index) {     \
    INT32 (opr) ;                                             \
    INT32 (index);                                            \
    for((index)=(rec)->num_operands-1;(index)>=0;(index)--) { \
         opr = (rec)->operands[(index)];              
       
#define ENDFE_SSI_OPR_INDEX_REVERSE }}

#define CONTINUE_SSI_OPR_INDEX_REVERSE {continue;} 

#define BREAK_SSI_OPR_INDEX_REVERSE    {break;}


/*******************************************/


/*
 * simple operand iterator, provides unique ids
 */
#define FOREACH_SSI_OPR_ID_INDEX(rec, oprid, index) {     \
    INT32 (oprid) ;                                        \
    INT32 (index);                                         \
    for((index)=0;(index)<(rec)->num_operands;(index)++) { \
         oprid = (rec)->op_id[(index)];              
       
#define ENDFE_SSI_OPR_ID_INDEX }}

#define CONTINUE_SSI_OPR_ID_INDEX {continue;} 

#define BREAK_SSI_OPR_ID_INDEX    {break;}


/*******************************************/
/*
 * simple operand iterator, provides unique ids
 */
#define FOREACH_SSI_OPR_ID_INDEX_REVERSE(rec, oprid, index) { \
    INT32 (oprid) ;                                            \
    INT32 (index);                                             \
    for((index)=(rec)->num_operands-1;(index)>=0;(index)--) {  \
         oprid = (rec)->op_id[(index)];              
       
#define ENDFE_SSI_OPR_ID_INDEX_REVERSE }}

#define CONTINUE_SSI_OPR_ID_INDEX_REVERSE {continue;} 

#define BREAK_SSI_OPR_ID_INDEX_REVERSE    {break;}


/*******************************************/

/* this accesses the bv directly 
 */
#define SSI_GENERIC_BV_GET(bv, code)         ((bv) & (code))
#define SSI_GENERIC_BV_SET(bv, code)         ((bv) |= (code))


struct ss_ia64_inst {
    /* virtual address */
    UINT64               vaddr; 
    /* libopcodes static info */
    const struct ia64_opcode *opc;
    /* bitvector of inst properties      */
    UINT32               bv;
    /* contains predicate register index */
    INT16                pred;
    /* 1 is sbit follows                 */
    bool                 sbit;
    /* returns number of outputs registers */
    INT16                num_outputs;      
    /* returns number of inputs registers */
    INT16                num_inputs;      
    /* returns number of operands */
    INT16                num_operands;      
    /* operand bitvector, with operand properties */
    UINT32               opbv[SSI_NUM_OPERAND];
    /* effective address index */
    INT16                ea;
    /* operand register index            */
    INT8                 operands[SSI_NUM_OPERAND];
    /* unique register identifier, probably more useful than operands */
    INT32                op_id[SSI_NUM_OPERAND];       
    /* pointer to decoder record */
    const struct ia64_operand *odesc[SSI_NUM_OPERAND];
    /* which execution unit */
    enum ia64_unit      unit;

    /* mem op size */
    char                mem_size;

    /* ref count for mem manager*/
    INT32               refcnt;
    /* pointer for free list */
    struct ss_ia64_inst* next;
    UINT32              refid;
    UINT64              stamp;
};




/* SIMPLE SCALAR INTERFACE SIMPLE SCALAR INTERFACE SIMPLE SCALAR INTERFACE
 * 
 * NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE
 *
 * there are now two methods for using ssdis library.  The first
 * (original) is kept of compatibility.   This will be eventually
 * deleted.  The second has been updated, and does not suffer some
 * brain dead memory leaks.
 *
 * The original interface uses:
 * ss_init
 * ss_load
 * ss_insn_ia64
 * ss_section_limit
 * ss_reset
 * ss_done
 * ss_map_gdb_bfd
 * ss_map_bfd_gdb
 * 
 * The new interface uses:
 * ss_init
 * ss_load
 * ss_insn2_ia64
 * ss_section_limit
 * 
 */
/*
 * initialize any package stuff
 */
extern void              ss_init();

/*
 * reset for new section 
 */
extern void              ss_reset();

/*
 * call this function initialize some basic data bfd 
 * structures for SimpleScalar interface.
 */
extern bool              ss_load(char* name);


/*
 * This is the new interface to the decoder.  It now expects that
 * the section has been loaded.  This is step is designated
 * by passing a section, which previously is assumed to have been
 * loaded via ss_load_section or ss_load_section_addr.  For backward
 * compatibility, passing a null to the section forces it to lookup
 * the section, but this is inefficient.
 * The address taken in can be either gdb or bfd formatted slot offsets
 */ 
extern INT32              ss_insn2_ia64 (UINT64, ss_ia64_inst*);

extern void               ss_done();

extern UINT64             ss_get_start_address();


/* some globals for SimpleScalar interface exported for iterators
 */
extern  bfd              *ss_file;

extern  void             ss_section_limit(asection*, UINT64* start, 
					  UINT64* end, 
					  INT32* inc);
                                          

/* various incrementing routines */
extern  bool             ss_inc_vma(UINT64*   adr);
extern  bool             ss_dec_vma(UINT64*   adr);

extern  bool             ss_inc_gdb(UINT64*   adr);
extern  bool             ss_dec_gdb(UINT64*   adr);

/* maps address slot offset from gdb land to bfd land and vice versa
 * will be obseleted
 */
extern  void             ss_map_gdb_bfd(UINT64*);
extern  void             ss_map_bfd_gdb(UINT64*);


extern  INT32             ss_debug_flag;
#if SSDIS_DEBUG
#define         ss_debug(fmt64, fmt32, args...)              \
                if(ss_debug_flag) {                          \
                    fprintf(stderr, "SS line %d:", __LINE__);\
                    if(NATIVE_INT==64 || fmt32==0)           \
                        fprintf(stderr, fmt64,##args);       \
                    else                                     \
                        fprintf(stderr,fmt32,##args);        \
                    fprintf(stderr, "\n");                   \
                }
#else
#define         ss_debug(fmt, args...)
#endif

#define SS_SAME 0
/* creates a human readable string 
 * assumes that string is long enough, 
 * returns length
 */
extern  INT32            ss_sprintf(char* str, ss_ia64_inst* inst);
extern  INT32            ss_attr_sprintf(char* str, ss_ia64_inst* inst);
extern  INT32            ss_other_sprintf(char* str, ss_ia64_inst* inst);


/* maps a unique reg number id, to a human readable string */
extern  INT32            ss_opr_sprintf(char* str, INT32 opr);


/* 
 * set operands 
 */
extern  INT32      ss_set_opr(ss_ia64_inst* inst,
			      INT32 op_id0,
			      INT32 op_id1,
			      INT32 op_id2,
			      INT32 op_id3,
			      INT32 op_id4,
			      INT32 pred,
			      bool sbit);


/*
 * find an instruction based on opcodes, dont assign operands
 * here.  this is incredibly slow.  dont use this often
 * return 1 for success, 0 for fail
 */
extern  int       ss_find_insn(ia64opc_enum opc,
                               ss_ia64_inst* inst);


/*
 * exposed internal interfaces for Asim tools
 */
/* generic functions to fill in inst attributes */
extern INT32
fast_inst_attribute(const struct ia64_opcode *idesc,
		    ss_ia64_inst             *ssi,
		    ia64_insn                insn);

/* decode operand attributes */
extern INT32
fast_operand_attribute(const struct ia64_opcode  *idesc,
		       const struct ia64_operand *odesc,
		       ss_ia64_inst              *ssi,
		       INT32                      value,
		       INT32                      index);

/**************************************************************************
 * actually instruction decoder.  Wrapped in func to handle
**************************************************************************/

extern INT32
fast_get_bundle_bytes(UINT64 memaddr,
                      bfd_byte*  bundle);

extern INT32
fast_get_insn_opc(UINT64               instaddr,
                  char*                bundle,
                  INT32*               slotnum,
                  char*                templatebits,
                  char*                s_bit,
                  ia64_insn*           insn,
                  enum ia64_unit*      unit,
                  struct ia64_opcode** idesc,
                  ia64_insn*           slot);

extern INT32
fast_insn_ia64 (UINT64                  memaddr, 
		bfd_byte*               bundle,
		struct ss_ia64_inst*    ssi
    );

extern INT32 
fast_get_section(
    UINT64          memaddr, 
    asection**      psection, 
    bfd_byte**      pdata,
    bfd_size_type*  plength
    );


/*
 * Simple annotation interface
 */

/* per instruction annotations  */
struct ss_ano_inst {
    INT16       bv;
    INT16       eil;     /* eil num */
    INT16       blk;     /* blk num */
    INT16       tag;
};
typedef struct ss_ano_inst ss_ano_inst;

/* routine annotations */
struct ss_ano_routine_raw {
    UINT64       routine_name;
    UINT64       module_name; 
    UINT64       inst_ano;   
    UINT64       tag_ano;
};
typedef struct ss_ano_routine_raw ss_ano_routine_raw;

struct ss_ano_routine {
    char*       routine_name;
    char*       module_name; 
    ss_ano_inst* inst_ano;   
    void*       tag_ano;
};
typedef struct ss_ano_routine ss_ano_routine;

/*
 * initialize annotations, return 0 if no annotations
 * present, else 1 if annotations present
 */
extern  bool     ss_init_ano();

/* given address, find annotation information associated
 */
extern  bool     ss_find_ano(UINT64, 
                             ss_ano_inst**,
                             ss_ano_routine**);


#define ANUC_BV_SET(bv, loc)  ((bv) |= (loc))
#define ANUC_BV_GET(bv, loc)  ((bv) & (loc))

#define ANUC_BV_SP                0x1
#define ANUC_BV_SPECULATED        0x2
#define ANUC_BV_COMPENSATION      0x4
#define ANUC_BV_REG_MOV           0x8
#define ANUC_BV_SWP              0x10




/*
 * decoder inst memory management interface 
 */
extern bool
sm_initialize();

extern void
sm_finish();

extern ss_ia64_inst*
sm_create(UINT64 timestamp);

extern ss_ia64_inst*
sm_copy(ss_ia64_inst*   inst);

extern bool
sm_delete(ss_ia64_inst** inst);

extern bool
sm_sanitycheck(UINT64    clock,
               UINT64    margin);

#endif
