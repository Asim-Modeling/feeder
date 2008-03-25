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

/* Assembly language routines for implementing instructions */

#include <stdio.h>

#include "icode.h"
#include "globals.h"
#include "opcodes.h"
#include "protos.h"

/* icode_ptr
unimplemented_op(inflight_inst_ptr ifi, icode_ptr picode, thread_ptr pthread) */

#ifdef SPECULATIVE_EXCEPTIONS
#undef SPECULATIVE_EXCEPTIONS
#endif

OP(unimplemented_op)
{
  /* This may be a speculatively issued inst - cannot cause exception */
#ifdef SPECULATIVE_EXCEPTIONS
  fatal("0x%lx: unimplemented operation %s (0x%x).\n",
	(long) picode->addr,
	desc_table[picode->opnum].opname,
	picode_instr(pthread, picode));
#endif
  return picode->next;
}

/*
icode_ptr
reserved_op(icode_ptr picode, thread_ptr pthread) */
OP(reserved_op)
{
#ifdef SPECULATIVE_EXCEPTIONS
  fatal("0x%lx: reserved instruction (0x%x).\n",
	(long) picode->addr, picode_instr(pthread, picode));
#endif
  return picode->next;
}

OP(bogus_f)
{
#ifdef SPECULATIVE_EXCEPTIONS
  warn("0x%lx: bogus (0x%x).\n",
       (long) picode->addr, picode_instr(pthread, picode));
#endif
  return picode->next;
}

OP(nop_f)
{
  return picode->next;
}

OP(gentrap_f)
{
  ifi->runstate = R_SIGNAL;
  pthread->signal = SIGTRAP;

  fprintf(Aint_output, "GENTRAP: tid=%d, pc=%lx\n", pthread->tid, picode->addr);
  /* this is a hack, because we pass control to the debugger after we commit
   * this instruction, but we want the PC to remain the same until the
   * debugger either replaces the trap instruction or moves us past the
   * trap instruction.  
   */
  return picode->next;
}

OP(rduniq_f)
{
  MapReg(RET_VAL_REG) = pthread->unique;
  return picode->next;
}
OP(wruniq_f)
{
  pthread->unique = MapReg(A0);
  return picode->next;
}
OP(bpt_f)
{
  ifi->runstate = R_SIGNAL;
  pthread->signal = SIGTRAP;

  fprintf(Aint_output, "BPT: tid=%d, pc=%lx\n", pthread->tid, picode->addr);
  /* this is a hack, because we pass control to the debugger after we commit
   * this instruction, but we want the PC to remain the same until the
   * debugger either replaces the trap instruction or moves us past the
   * trap instruction.  
   */
  return picode->next;
}

UNIMPLEMENTED(bugchk_f)

/* chmk is the same as callsys */

OP(chmk_f)
{
  return callsys_f(ifi, picode, pthread);
}

UNIMPLEMENTED(imb_f)
UNIMPLEMENTED(halt_f)
UNIMPLEMENTED(draina_f)
UNIMPLEMENTED(nphalt_f)
UNIMPLEMENTED(cobratt_f)
UNIMPLEMENTED(cserve_f)
UNIMPLEMENTED(ipir_f)
UNIMPLEMENTED(cflush_f)
UNIMPLEMENTED(rti_f)
UNIMPLEMENTED(rtsys_f)
UNIMPLEMENTED(whami_f)
UNIMPLEMENTED(rdusp_f)
UNIMPLEMENTED(wrperfmon_f)
UNIMPLEMENTED(wrusp_f)
UNIMPLEMENTED(wrkgp_f)
UNIMPLEMENTED(rdps_f)
UNIMPLEMENTED(swpipl_f)
UNIMPLEMENTED(wrent_f)
UNIMPLEMENTED(tbi_f)
UNIMPLEMENTED(rdval_f)
UNIMPLEMENTED(wrval_f)
UNIMPLEMENTED(swpctx_f)
UNIMPLEMENTED(jtopal_f)
UNIMPLEMENTED(wrvptptr_f)
UNIMPLEMENTED(wrfen_f)
UNIMPLEMENTED(mtpr_mces_f)

RESERVED(opc01_f)
RESERVED(opc02_f)
RESERVED(opc03_f)
RESERVED(opc04_f)
RESERVED(opc05_f)
RESERVED(opc06_f)
RESERVED(opc07_f)

OP(lda_f)
{
#ifdef DEBUG_LDA  
  fprintf(Aint_output, "$%d(%lx) + %lx => ",
	  picode->args[RB], REG(RB), picode->immed);
#endif
  REG(RA) = REG(RB) + picode->immed;
#ifdef DEBUG_LDA  
  fprintf(Aint_output, "$%d(%lx)\n", picode->args[RA], REG(RA));
#endif
  ZERO_ZERO_REG;
  return picode->next;
}

OP(ldah_f)
{
  REG(RA) = REG(RB) + picode->immed;
  ZERO_ZERO_REG;
  return picode->next;
}

RESERVED(opc0a_f)

OP(ldq_u_f)
{
  REG(RA) = (long) ifi->data;
  ZERO_ZERO_REG;
  return picode->next;
}

RESERVED(opc0c_f)
RESERVED(opc0d_f)
RESERVED(opc0e_f)

OP(stq_u_f)
{
  event_ptr pevent = pthread->pevent;
  if (ifi->paddr) *(long *) ifi->paddr = ifi->data;
  return picode->next;
}

#define OPFMT(NAME) \
OP(NAME ## _f) \
{ \
  long lv1, rv1, rv2; \
\
  lv1 = REG(RC); \
  rv1 = REG(RA); \
  if (!(picode->iflags & E_LITERAL)) { \
    rv2 = REG(RB);  \
  } else { \
    rv2 = (long) picode->literal; \
  }    \
  /* printf("NAME  =r%d r%di (0x%lx) 0x%lx ", picode->args[RC], */  \
  /* picode->args[RA], rv1, rv2); */  \
  asm(#NAME "	%1, %2, %0" : "=&r" (lv1) : "r" (rv1) , "r" (rv2)); \
  /* if (NAME ## _opn == addl_opn) printf("addl(%lx, %lx) => 0x%lx\n", rv1, rv2, lv1); */\
  REG(RC) = lv1; \
  ZERO_ZERO_REG; \
  return(picode->next); \
}

OPFMT(addl)
OPFMT(s4addl)
OPFMT(subl)
OPFMT(s4subl)
OPFMT(cmpbge)
OPFMT(s8addl)
OPFMT(s8subl)
OPFMT(cmpult)
OPFMT(addq)
OPFMT(s4addq)
OPFMT(subq)
OPFMT(s4subq)
OPFMT(cmpeq)
OPFMT(s8addq)
OPFMT(s8subq)
OPFMT(cmpule)
OPFMT(addlv)
OPFMT(sublv)
OPFMT(cmplt)
OPFMT(addqv)
OPFMT(subqv)
OPFMT(cmple)
OPFMT(and)
OPFMT(bic)

/* OPFMT(cmovlbs) */

OP(cmovlbs_f) {
    long rbv = ((picode->iflags & E_LITERAL)? (picode->literal): REG(RB));
    if (REG(RA)&1) REG(RC) = rbv;
    ZERO_ZERO_REG;
    return picode->next;
}

/* OPFMT(cmovlbc) */
OP(cmovlbc_f) {
    long rbv = ((picode->iflags & E_LITERAL)? (picode->literal): REG(RB));
    if ( (REG(RA)&1) == 0) REG(RC) = rbv;
    ZERO_ZERO_REG;
    return picode->next;
}

OPFMT(bis)

/* OPFMT(cmoveq) */
OP(cmoveq_f) {
    long rbv = ((picode->iflags & E_LITERAL)? (picode->literal): REG(RB));
    if (REG(RA) == 0) REG(RC) = rbv;
    ZERO_ZERO_REG;
    return picode->next;
}

/* OPFMT(cmovne) */
OP(cmovne_f) {
    long rbv = ((picode->iflags & E_LITERAL)? (picode->literal): REG(RB));
    if (REG(RA)) REG(RC) = rbv;
    ZERO_ZERO_REG;
    return picode->next;
}

OPFMT(ornot)
OPFMT(xor)

/* OPFMT(cmovlt) */
OP(cmovlt_f) {
    long rbv = ((picode->iflags & E_LITERAL)? (picode->literal): REG(RB));
    if (REG(RA)<0) REG(RC) = rbv;
    ZERO_ZERO_REG;
    return picode->next;
}

/* OPFMT(cmovge) */
OP(cmovge_f) {
    long rbv = ((picode->iflags & E_LITERAL)? (picode->literal): REG(RB));
    if (REG(RA)>=0) REG(RC) = rbv;
    ZERO_ZERO_REG;
    return picode->next;
}

OPFMT(eqv)

OP(amask_f)
{
  long rbv = ((picode->instr & 0x1000)? (picode->literal): REG(RB));
  REG(RC) = rbv & ~(AINT_AMASK);
  ZERO_ZERO_REG;
  return picode->next;
}

/*OPFMT(cmovle) */
OP(cmovle_f) {
    long rbv = ((picode->iflags & E_LITERAL)? (picode->literal): REG(RB));
    if (REG(RA)<=0) REG(RC) = rbv;
    ZERO_ZERO_REG;
    return picode->next;
}

/* OPFMT(cmovgt) */
OP(cmovgt_f) {
    long rbv = ((picode->iflags & E_LITERAL)? (picode->literal): REG(RB));
    if (REG(RA)>0) REG(RC) = rbv;
    ZERO_ZERO_REG;
    return picode->next;
}

OP(implver_f)
{
  REG(RC) = AINT_IMPLVER;
  ZERO_ZERO_REG;
  return picode->next;
}

OPFMT(mskbl)
OPFMT(extbl)
OPFMT(insbl)
OPFMT(mskwl)
OPFMT(extwl)
OPFMT(inswl)
OPFMT(mskll)
OPFMT(extll)
OPFMT(insll)
OPFMT(zap)
OPFMT(zapnot)
OPFMT(mskql)
OPFMT(srl)
OPFMT(extql)
OPFMT(sll)
OPFMT(insql)
OPFMT(sra)
OPFMT(mskwh)
OPFMT(inswh)
OPFMT(extwh)
OPFMT(msklh)
OPFMT(inslh)
OPFMT(extlh)
OPFMT(mskqh)
OPFMT(insqh)
OPFMT(extqh)
OPFMT(mull)
OPFMT(mulq)
OPFMT(umulh)
OPFMT(mullv)
OPFMT(mulqv)

RESERVED(opc14_f)

EMPTY_OP(trapb_f)
EMPTY_OP(excb_f)
EMPTY_OP(mb_f)
EMPTY_OP(wmb_f)
EMPTY_OP(fetch_f)
EMPTY_OP(fetch_m_f)

OP(rpcc_f)
{
  REG(RA) = pmint_get_sim_cycle(); /* (long) ifi->time; */
  ZERO_ZERO_REG;
  return picode->next;
}

UNIMPLEMENTED(rc_f)
UNIMPLEMENTED(rs_f)

RESERVED(pal19_f)

#define JUMP_OP(NAME) \
OP(NAME ## _f) \
{ \
   REG(RA) = IR2V(picode->next); \
   ZERO_ZERO_REG; \
   if (0) fprintf(Aint_output, "\t %lx: " #NAME " 0x%lx\n", picode->addr, REG(RB)); \
   return IV2R(pthread->process, REG(RB)); \
}
 
JUMP_OP(jmp)
JUMP_OP(jsr)
JUMP_OP(ret)
JUMP_OP(jsr_coroutine)
 
OP(pal1b_f) {	/* HW_LD */
  REG(RA) = ifi->data;
  ZERO_ZERO_REG;
  return picode->next;
}

RESERVED(pal1d_f)
RESERVED(pal1e_f)
RESERVED(pal1f_f)

#define FP_RD 1
#define FP_WR 0

/* The following hack fails if the preprocessor recognizes strings and
 * passes them unchanged. Use the -traditional-cpp option with gcc to
 * enable NAME to be substituted in the string
 */

#define MEM_FP(NAME, rw, size) \
OP(NAME ## _f) \
{ \
  event_ptr pevent = pthread->pevent; \
  if (0) informative(#NAME ": %lx %x %d\n", picode->addr, picode_instr(pthread, picode), picode->opnum);\
  if (rw) /* This is a read into reg */ {\
   asm(#NAME " %0,%1" : "=f" (FP(RA)) : "o" (* (size *) &ifi->data)); \
   MapFP(ZERO_REGISTER) = 0.0; \
  } else {\
	    /* asm(#NAME " %1,%0" : "=o" (* (size *) pthread->paddr) : "f" (FP(RA))); */ \
   if (ifi->paddr) *(size *) ifi->paddr = ifi->data; \
  } \
  return (picode->next); \
}

MEM_FP(ldf, FP_RD, long)
MEM_FP(ldg, FP_RD, long)
MEM_FP(lds, FP_RD, long)
MEM_FP(ldt, FP_RD, long)


MEM_FP(stf, FP_WR, int)
MEM_FP(stg, FP_WR, long)
MEM_FP(sts, FP_WR, int)
MEM_FP(stt, FP_WR, long)

void aint_cvt_fp_mem(unsigned int opnum, double fpreg, long *memloc)
{
  switch(opnum) {
  case stf_opn:
    asm("stf %1, %0" : "=o" (* memloc) : "f" (fpreg));
    break;
  case stg_opn:
    asm("stg %1, %0" : "=o" (* memloc) : "f" (fpreg));
    break;
#ifdef NEW_TTL
  case ttl_vsts_opn:
  case ttl_vscats_opn:
#endif
  case sts_opn:
    asm("sts %1, %0" : "=o" (* memloc) : "f" (fpreg));
    break;
#ifdef NEW_TTL
  case ttl_vstt_opn:
  case ttl_vscatt_opn:
#endif
  case stt_opn:
    asm("stt %1, %0" : "=o" (* memloc) : "f" (fpreg));
    break;
  default:
    break;
  }
}


OP(ldb_f) {
  REG(RA) = (unsigned char)ifi->data;
  ZERO_ZERO_REG;
  return picode->next;
}

OP(ldw_f) {
  REG(RA) = (unsigned short)ifi->data;
  ZERO_ZERO_REG;
  return picode->next;
}

OP(ldl_f) {
  REG(RA) = ((int)ifi->data);
  ZERO_ZERO_REG;
  return picode->next;
}

OP(ldq_f) {
  REG(RA) = ifi->data;
  ZERO_ZERO_REG;
  return picode->next;
}

OP(ldl_l_f)
{
  /* Load locked.
   * Load data into register
   */

  REG(RA) = (int) ifi->data;
#if 0
  fprintf(Aint_output, "LDL_L: thrd:%d, cyc:%ld  %lx (%lx)  val == %lx\n",
		pthread->tid, pmint_get_sim_cycle(),
		ifi->paddr, ifi->vaddr,(long) ifi->data);
  fflush(Aint_output);
#endif
  ZERO_ZERO_REG;
  return picode->next;
}

OP(ldq_l_f)
{ 
  REG(RA) = (long) ifi->data;

  ZERO_ZERO_REG;
#if 0
  fprintf(Aint_output, "LDQ_L: thrd:%d, cyc:%ld %lx (%lx)  val == %lx\n",
		pthread->tid, pmint_get_sim_cycle(),
		ifi->paddr, ifi->vaddr, (long) ifi->data);
  fflush(Aint_output);
#endif
  return(picode->next);
}

OP(stb_f)
{
  if (ifi->paddr) *(char *) ifi->paddr = (char) ifi->data;
  if(0) fprintf(Aint_output, "STB: thrd: %d cyc:%ld  %lx (%lx)   val=%lx\n",
		pthread->tid, 
		pmint_get_sim_cycle(),
		ifi->paddr, ifi->vaddr,
		* (char *) ifi->paddr
		);

  return picode->next;
}

OP(stw_f)
{
  if (ifi->paddr) *(short *) ifi->paddr = (short) ifi->data;
  if(0) fprintf(Aint_output, "STF: thrd: %d cyc:%ld  %lx (%lx)   val=%x\n",
		pthread->tid, 
		pmint_get_sim_cycle(),
		ifi->paddr, ifi->vaddr,
		* (int *) ifi->paddr
		);

  return picode->next;
}

OP(stl_f)
{

  if (ifi->paddr) *(int *) ifi->paddr = (int) ifi->data;
  if(0) fprintf(Aint_output, "STL: thrd: %d cyc:%ld  %lx (%lx)   val=%x\n",
		pthread->tid, 
		pmint_get_sim_cycle(),
		ifi->paddr, ifi->vaddr,
		* (int *) ifi->paddr
		);

  return picode->next;
}

OP(stq_f)
{
  if (ifi->paddr) * (long *) ifi->paddr = ifi->data;

  if(0) fprintf(Aint_output, "STQ: thrd: %d cyc:%ld  %lx (%lx)   val=%lx\n",
		pthread->tid, 
		pmint_get_sim_cycle(),
		ifi->paddr, ifi->vaddr,
		* (long *) ifi->paddr
		);


  return picode->next;
}

OP(stl_c_f)
{
  /* Store conditional
   * If store succeeds, store RA at address
   */
  int new_val = (int) ifi->data;
#if 0
  fprintf(Aint_output, "STL_C: thrd: %d cyc:%ld  %lx (%lx) %s   old_val=%lx new_val=%lx\n",
		pthread->tid, 
		pmint_get_sim_cycle(),
		ifi->paddr, ifi->vaddr,
		(ifi->stc_success) ? "locked" : "revoked",
		* (int *) ifi->paddr,
		(ifi->stc_success) ? new_val : * (int *) ifi->paddr
		);
  fflush(Aint_output);
#endif
  if (ifi->stc_success) {
    * (int *) ifi->paddr = new_val;
  }

  /*  Update result register */
  REG(RA) = ifi->stc_success; ZERO_ZERO_REG;

  return picode->next;
}

OP(stq_c_f)
{
  long new_val = (long) ifi->data;
#if 0
  fprintf(Aint_output, "STQ_C: thrd: %d cyc:%ld  %lx (%lx) %s   old_val=%lx new_val=%lx\n",
		pthread->tid, 
		pmint_get_sim_cycle(),
		ifi->paddr, ifi->vaddr,
		(ifi->stc_success) ? "locked" : "revoked",
		* (int *) ifi->paddr,
		(ifi->stc_success) ? new_val : * (int *) ifi->paddr
		);
  fflush(Aint_output);
#endif
  if (ifi->stc_success) {
    * (long *) ifi->paddr = new_val;
  }

  /*  Update result register */
  REG(RA) = ifi->stc_success; ZERO_ZERO_REG;

  return picode->next;
}


OP(br_f) 
{ 
  REG(RA) = IR2V(picode->next);
  ZERO_ZERO_REG;
  return picode->target; 
}

OP(bsr_f)
{ 
  if (0) printf("%lx:  BSR 0x%lx\n", picode->addr, IR2V(picode->target));
  REG(RA) = IR2V(picode->next);
  ZERO_ZERO_REG;
  return picode->target; 
}

#define FP_BR(NAME, LABEL) \
OP(NAME ## _f) \
{ \
  int i=1, cond = 1; \
  asm (#NAME " %1, " # LABEL "; subq %2,1,%0; " #LABEL ":" : "=r" (cond) : \
       "f" (FP(RA)) , "r" (i)); \
  if (cond) { \
    return picode->target; \
  } else { \
    return picode->next; \
  } \
}

FP_BR(fbeq, lfbeq)
FP_BR(fblt, lfblt)
FP_BR(fble, lfble)

FP_BR(fbne, lfbne)
FP_BR(fbge, lfbge)
FP_BR(fbgt, lfbgt)

OP(blbc_f) { 
  if (REG(RA)&1) {
    return picode->next; 
  } else {
    return picode->target;
  }
}

OP(beq_f) { 
  if (REG(RA)) {
    return picode->next; 
  }
  return picode->target;
}

OP(blt_f) {
  if (REG(RA) >= 0) {
    return picode->next; 
  } 
  return picode->target;
}

OP(ble_f) {
  if (REG(RA) > 0) {
    return picode->next;
  }
  return picode->target;
}

OP(blbs_f) {
  if (REG(RA)&1) {
    return picode->target;
  }
  return picode->next;
}

OP(bne_f) {
  if (REG(RA)) {
    return picode->target;
  }
  return picode->next;
}

OP(bge_f) { 
  if (REG(RA) >= 0) {
    return picode->target; 
  }
  return picode->next; 
}

OP(bgt_f) {
  if (REG(RA) > 0) {
    return picode->target;
  }
  return picode->next;
}



double get_fpcr()
{
  double fpcr; asm ("trapb; mf_fpcr %0; trapb"
	 : "=f" (fpcr));
  return fpcr;
}

void put_fpcr(double fpcr)
{
  asm ("trapb; mt_fpcr %0; trapb"
       : : "f" (fpcr));
}
