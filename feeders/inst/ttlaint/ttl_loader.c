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

#include "icode.h"

#ifdef NEW_TTL
/* 
 * Routines for loading executables and shared objects
 * Tarantula patching !!
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <sys/utsname.h>
#include <string.h>
#include </usr/include/assert.h>

#include <elf_abi.h>
#include <elf_mips.h>
#include <alpha/inst.h>
#include "opcodes.h"
#include "globals.h"
#include "protos.h"
#include "bfd.h"
#include "obstack.h"

#include "ttl_loader.h"


char 	*ttl_min_entrypc;
char 	*ttl_max_entrypc;


UINT64
build_iword(int ttl_idx, int ra, int rb, int rc, int disp, int masked)
{
 TTL_Extended_Instruction w;
 int ttlClass;
 
 w.Long = 0;
 ttlClass = ttl_routines[ttl_idx].class;
 
 if ( ra != -1 && (ra < 0 || ra > 31))  panic("build_iword: ra out of range: %d\n",ra);
 if ( rb != -1 && (rb < 0 || rb > 31))  panic("build_iword: rb out of range: %d\n",rb);
 if ( rc != -1 && (rc < 0 || rc > 31))  panic("build_iword: rc out of range: %d\n",rc);

 if ((ttlClass<0)||(ttlClass>=TTL_MAX_CLASS))
 	panic("build_iword: class %d for idx %d (%s) unknown\n",ttlClass,ttl_idx,ttl_routines[ttl_idx].name);
 
 switch (ttlClass)
 {
 	case TTL_MEM_CLASS:
		w.VecMem.MemDisp= disp;
		w.VecMem.Type	= ttl_routines[ttl_idx].encoding.type;
		w.VecMem.Mask	= masked;
		w.VecMem.Rb		= rb;
		w.VecMem.Ra		= ra;
		w.VecMem.Opcode	= ttl_routines[ttl_idx].encoding.opcode;
		break;

 	case TTL_VFV_CLASS:
		w.VecOp.Rc		= rc;
		w.VecOp.Function= ttl_routines[ttl_idx].encoding.func;
		w.VecOp.Type	= ttl_routines[ttl_idx].encoding.type;
		w.VecOp.Mask	= masked;
		w.VecOp.VSV		= V_VFV;
		w.VecOp.Rb		= rb;
		w.VecOp.Ra		= ra;
		w.VecOp.Opcode	= ttl_routines[ttl_idx].encoding.opcode;
		break;
		
 	case TTL_VIV_CLASS:
	case TTL_GATH_CLASS:
	case TTL_SCAT_CLASS:
		w.VecOp.Rc		= rc;
		w.VecOp.Function= ttl_routines[ttl_idx].encoding.func;
		w.VecOp.Type	= ttl_routines[ttl_idx].encoding.type;
		w.VecOp.Mask	= masked;
		w.VecOp.VSV		= V_VIV;
		w.VecOp.Rb		= rb;
		w.VecOp.Ra		= ra;
		w.VecOp.Opcode	= ttl_routines[ttl_idx].encoding.opcode;
		break;
	
	default:						/* VVV, NVV, ...  */
		w.VecOp.Rc		= rc;
		w.VecOp.Function= ttl_routines[ttl_idx].encoding.func;
		w.VecOp.Type	= ttl_routines[ttl_idx].encoding.type;
		w.VecOp.Mask	= masked;
		w.VecOp.VSV		= V_VVV;
		w.VecOp.Rb		= rb;
		w.VecOp.Ra		= ra;
		w.VecOp.Opcode	= ttl_routines[ttl_idx].encoding.opcode;
 }
 

 /* now we put an informative message*/
 if ( ttlClass == TTL_MEM_CLASS ) {
  fprintf(Aint_output,"\t<MEM: Op=0x%x,Va=0x%x,Rb=0x%x,Mask=0x%x,Type=0x%x,Disp=0x%x> = 0x%lx\n",
		  (int)w.VecMem.Opcode, (int)w.VecMem.Ra, (int)w.VecMem.Rb, (int)w.VecMem.Mask, (int)w.VecMem.Type, (int)w.VecMem.MemDisp, (long)w.Long
	 );
 }
 else {
  fprintf(Aint_output,"\t<OP : Op=0x%x,Va=0x%x,Vb=0x%x,VSV=0x%x,Mask=0x%x,Type=0x%x,Func=0x%x,Vc=0x%x> = %lx\n",
		  w.VecOp.Opcode, w.VecOp.Ra, w.VecOp.Rb, w.VecOp.VSV, w.VecOp.Mask, w.VecOp.Type,
		  w.VecOp.Function, w.VecOp.Rc, w.Long
	 );
 }



 return w.Long;
}


/* Decide whenever any instruction is or not from ttl subset
 * We are interested in 
 *  VAXFP opcodes: 0x15
 *  VAX Memory instructions: LDF, LDG, STF, STG and so on...
*/
int is_vaxfp_insn(union alpha_instruction uinst, ulong pc)
{
#define TTL_NUM_OPCODES 4
 static int ttl_opcodes[TTL_NUM_OPCODES] = {
    ttl_load_opcode,
    ttl_store_opcode,
    ttl_arit_opcode,
    ttl_qarit_opcode
 };
 int opcode = uinst.common.opcode;
 register int i;

 for (i=0;i<TTL_NUM_OPCODES;i++)
    if (opcode==ttl_opcodes[i]) return (1);
    
 return (0);
}

/*************************************************/
/* first pass in decodification: OPCODE analysis */
/*************************************************/
int vax2ttl(union alpha_instruction uinst)
{
 int opcode = uinst.common.opcode;
 
 switch ( opcode ) {
  case ttl_load_opcode:     return vax2ttl_load (uinst);
  case ttl_store_opcode:    return vax2ttl_store(uinst);
  case ttl_arit_opcode:     return vax2ttl_arit (uinst);
  case ttl_qarit_opcode:    return vax2ttl_qarit(uinst);
  default:                  return (-1);
 }
}


/******************************************************/
/* second pass in decodification: ex analysis (loads) */
/******************************************************/
int vax2ttl_load(union alpha_instruction uinst)
{
  #define TTL_MAX_LD 5
  /* I follow the ex field assignment (see final_opcodes.txt) */
  static int loads[TTL_MAX_LD] = {TTL_VLDL, TTL_VLDS, TTL_VLDQ, TTL_VLDT, TTL_VLDPF};
  union  ttl_instruction ttl_ins;
  int    ex;
  
  ttl_ins.word = uinst.word;    
  ex = ttl_ins.ttl_memory_instruction.ex;

  /* now I look into ex field to know what kind of load it is */
  if ( (ex<0) || (ex>=TTL_MAX_LD) )
    return (-1);
  else    
    return (loads[ex]);
}

/*******************************************************/
/* second pass in decodification: ex analysis (stores) */
/*******************************************************/
int vax2ttl_store(union alpha_instruction uinst)
{
  #define TTL_MAX_ST 5
  /* I follow the ex field assignment (see final_opcodes.txt) */
  static int stores[TTL_MAX_ST] = {TTL_VSTL, TTL_VSTS, TTL_VSTQ, TTL_VSTT, TTL_VSTPF};
  union  ttl_instruction ttl_ins;
  int    ex;
  
  ttl_ins.word = uinst.word;    
  ex = ttl_ins.ttl_memory_instruction.ex;
  
  /* now I look into ex field to know what kind of store it is */
  if ( (ex<0) || (ex>=TTL_MAX_ST) )
    return (-1);
  else    
    return (stores[ex]);
}

/****************************************************************/
/* second pass in decodification: function analysis (aritmetic) */
/****************************************************************/
int vax2ttl_arit(union alpha_instruction uinst)
{
#define TTL_MAX_ARIT 0x75
  /* I follow the function field assignment (see final_opcodes.txt) */
  static int aritmetics[TTL_MAX_ARIT][2] = {
    {-1,-1},
    {TTL_VVADDT, TTL_VSADDT},    {TTL_VVADDS, TTL_VSADDS},
    {TTL_VVADDQ, TTL_VSADDQ},    {TTL_VVADDL, TTL_VSADDL},
    {TTL_VVSUBT, TTL_VSSUBT},    {TTL_VVSUBS, TTL_VSSUBS},
    {TTL_VVSUBQ, TTL_VSSUBQ},    {TTL_VVSUBL, TTL_VSSUBL},
    {TTL_VVMULT, TTL_VSMULT},    {TTL_VVMULS, TTL_VSMULS},
    {TTL_VVMULQ, TTL_VSMULQ},    {TTL_VVMULL, TTL_VSMULL},
    {TTL_VVUMULH,TTL_VSUMULH},   
    {TTL_VVDIVT, TTL_VSDIVT},    {TTL_VVDIVS, TTL_VSDIVS},
    {TTL_VSQRTT, -1        },    {TTL_VSQRTS, -1        },
    {TTL_VS4ADDQ, -1       },    {TTL_VS8ADDQ,-1        },
    {TTL_VS4ADDL, -1       },    {TTL_VS8ADDL,-1        },
    {TTL_VS4SUBQ, -1       },    {TTL_VS8SUBQ,-1        },
    {TTL_VS4SUBL, -1       },    {TTL_VS8SUBL,-1        },
    {TTL_VVCMPEQ,TTL_VSCMPEQ},   {TTL_VVCMPLE,TTL_VSCMPLE},
    {TTL_VVCMPLT,TTL_VSCMPLT},   {TTL_VVCMPTEQ,TTL_VSCMPTEQ},
    {TTL_VVCMPTLE,TTL_VSCMPTLE}, {TTL_VVCMPTLT,TTL_VSCMPTLT},
    {TTL_VVCMPTUN,TTL_VSCMPTUN},
    {TTL_VVCMPULE,TTL_VSCMPULE}, {TTL_VVCMPULT,TTL_VSCMPULT},
    {TTL_VVCMPBGE,TTL_VSCMPBGE},    
    {TTL_SETVM,  -1},            {TTL_SETNVM, -1},
    {TTL_VVAND, TTL_VSAND  },    {TTL_VVBIS, TTL_VSBIS  },
    {TTL_VVXOR, TTL_VSXOR  },    {TTL_VVBIC, TTL_VSBIC  },
    {TTL_VVORNOT,TTL_VSORNOT},   {TTL_VVEQV, TTL_VSEQV  },
    {TTL_VVSRA, TTL_VSSRA  },    {TTL_VVSLL, TTL_VSSLL  },
    {TTL_VVSRL, TTL_VSSRL  },    {TTL_VCTPOP, -1  },
    {TTL_VCTLZ, -1  },           {TTL_VCTTZ, -1  },
    {TTL_VVINSBL, TTL_VSINSBL},  {TTL_VVINSWL, TTL_VSINSWL},
    {TTL_VVINSLL, TTL_VSINSLL},  {TTL_VVINSQL, TTL_VSINSQL},
    {TTL_VVINSWH, TTL_VSINSWH},  {TTL_VVINSLH, TTL_VSINSLH},
    {TTL_VVINSQH, TTL_VSINSQH},  
    {TTL_VVEXTBL, TTL_VSEXTBL},  {TTL_VVEXTWL, TTL_VSEXTWL},
    {TTL_VVEXTLL, TTL_VSEXTLL},  {TTL_VVEXTQL, TTL_VSEXTQL},
    {TTL_VVEXTWH, TTL_VSEXTWH},  {TTL_VVEXTLH, TTL_VSEXTLH},
    {TTL_VVEXTQH, TTL_VSEXTQH},  
    {TTL_VVMSKBL, TTL_VSMSKBL},  {TTL_VVMSKWL, TTL_VSMSKWL},
    {TTL_VVMSKLL, TTL_VSMSKLL},  {TTL_VVMSKQL, TTL_VSMSKQL},
    {TTL_VVMSKWH, TTL_VSMSKWH},  {TTL_VVMSKLH, TTL_VSMSKLH},
    {TTL_VVMSKQH, TTL_VSMSKQH},  
    {TTL_VVZAP,   TTL_VSZAP},    {TTL_VVZAPNOT,TTL_VSZAPNOT},  
    {TTL_VCVTTQ, -1},   {TTL_VCVTQS, -1},    {TTL_VCVTQT, -1},  
    {TTL_VCVTST, -1},   {TTL_VCVTTS, -1},    {TTL_VEXTSL, -1},  
    {TTL_VEXTSH, -1},   {TTL_VCMPR,  -1},    {TTL_VIOTA,  -1},  
    {TTL_VVMERG, -1},  
    {TTL_MVFVP, -1},    {TTL_MVTVP, -1},     {TTL_VSKEWH,-1},
    {TTL_VSKEWL,-1},    {-1,TTL_VGATHQ},     {-1,TTL_VGATHL},
    {-1,TTL_VGATHT},    {-1,TTL_VGATHS},     {-1,TTL_VSCATQ},
    {-1,TTL_VSCATL},    {-1,TTL_VSCATT},     {-1,TTL_VSCATS},
	{-1,TTL_VSMERGQ},	{-1,TTL_VSMERGT},
	{-1,TTL_VNCGATHQ},    {-1,TTL_VNCGATHL},
    {-1,TTL_VNCGATHT},    {-1,TTL_VNCGATHS},     
	{-1,TTL_VNCSCATQ},    {-1,TTL_VNCSCATL},    
	{-1,TTL_VNCSCATT},    {-1,TTL_VNCSCATS},
	{TTL_VSYNCH,-1},
	{-1,TTL_VGATHPF},	{-1,TTL_VSCATPF},	
	{-1,TTL_VNCGATHPF},	{-1,TTL_VNCSCATPF},
	{TTL_VDRAINM0, -1},	{TTL_VDRAINM1, -1},{TTL_VDRAINM2, -1},
	{TTL_VDRAINV0, -1},	{TTL_VDRAINV1, -1},{TTL_VDRAINV2, -1},
	{TTL_MVFVR,-1}
  };
  
  union  ttl_instruction ttl_ins;
  int    func, t;
  
  ttl_ins.word = uinst.word;    
  func = ttl_ins.ttl_aritmetic_instruction.function;
  t    = ttl_ins.ttl_aritmetic_instruction.t;
  
  /* now I look into these fields to know what kind of aritm inst. is ...*/
  if ( (t<0) || (t>=V_VSV_MAX) )            return (-1);
  if ( (func<0) || (func>=TTL_MAX_ARIT) )   return (-1);
    
  /* I don´t distinct between vfv and viv in my static matrix so:*/
  if (t>1) t = 1;
    
  return (aritmetics[func][t]);
}

/****************************************************************/
/* second pass in decodification: function analysis (aritmetic) */
/* Aritmetic instructions with qualifiers                       */
/****************************************************************/
int vax2ttl_qarit(union alpha_instruction uinst)
{
    printf ("\n### WARNING! vax2ttl_qarit called but unimplemented!!!\n");
}

/****************************************************************/
/****************************************************************/
/****************************************************************/
/*    Main pathcing routine                                     */
/****************************************************************/
/****************************************************************/
/****************************************************************/
void patch_vaxfp_insn(process_ptr process, icode_ptr picode, ulong target, union alpha_instruction uinst)
{
  int	    i,ridx,ttl_idx,nargs,opnum,merger;
  union     ttl_instruction ttl_ins;
  ulong     textaddr;


  ttl_ins.word = uinst.word;

   /* 
    * Find the corresponding ttl instruction. If this is not a valid ttl instruction, simply return;
    */
   ttl_idx = vax2ttl(uinst);
   if ( ttl_idx == -1 ) 
   {
      printf ("### AINT: FATAL ERROR, vax2ttl call failed!!\n");
      printf ("### AINT: uinst=%lx , opcode=%lx\n",uinst,uinst.common.opcode);
      printf ("### AINT: decoded as operational: va=%d, vb=%d, vc=%d, mask=%d, type=%d, function=%x \n",
       ttl_ins.ttl_aritmetic_instruction.va, ttl_ins.ttl_aritmetic_instruction.vb,
       ttl_ins.ttl_aritmetic_instruction.vc, ttl_ins.ttl_aritmetic_instruction.m,
       ttl_ins.ttl_aritmetic_instruction.t,  ttl_ins.ttl_aritmetic_instruction.function);
	   
      exit(-1);
   }
   
	/* process the mask bit, remember that mask bit is always on position 15 no matter this is
	 * a memory, control or arithmetic instruction. There are a few instructions that have not 
	 * sense with the mask bit asserted: mvfvp, mvtvp, setvm, setnvm, iota, merg, mergq, mergt,
	 * cmpr ,skewh, skewl and synch.
	 */
	 
	/* by default we are not masked */
	picode->is_masked = 0;

	 merger =  (ttl_routines[ttl_idx].opnum == ttl_vvmerg_opn);
	 merger |= (ttl_routines[ttl_idx].opnum == ttl_vsmergq_opn);
	 merger |= (ttl_routines[ttl_idx].opnum == ttl_vsmergt_opn);

     if (ttl_ins.ttl_memory_instruction.m || merger )
	 {
	 	/* 1) check all the invalid masked instructions */
		int ok = 1;
		ok = ok && (ttl_routines[ttl_idx].opnum != ttl_mvtvp_opn );
		ok = ok && (ttl_routines[ttl_idx].opnum != ttl_mvfvp_opn );
		ok = ok && (ttl_routines[ttl_idx].opnum != ttl_setnvm_opn);
		ok = ok && (ttl_routines[ttl_idx].opnum != ttl_setvm_opn);
		ok = ok && (ttl_routines[ttl_idx].opnum != ttl_viota_opn);
		ok = ok && (ttl_routines[ttl_idx].opnum != ttl_vcmpr_opn);
		ok = ok && (ttl_routines[ttl_idx].opnum != ttl_vsynch_opn);
		ok = ok && (ttl_routines[ttl_idx].opnum != ttl_vskewh_opn);
		ok = ok && (ttl_routines[ttl_idx].opnum != ttl_vskewl_opn);
		
		if (!ok)
		{
			printf("### AINT: FATAL ERROR, vax2ttl call failed!\n");
			printf("### AINT: I found the mask bit set on an intruction of the kind: %s\n",ttl_routines[ttl_idx].name);
		 	exit(-1);
		}
		
		/* all right them => just put the flag into the picode...*/
		picode->is_masked = 1;
 		
	 }
	 
	 /* a special flag to setvm / setnvm */
	 picode->is_setvm = (ttl_routines[ttl_idx].opnum == ttl_setnvm_opn) ||(ttl_routines[ttl_idx].opnum == ttl_setvm_opn);
	 
	 /* another special flag to mvfvp */
	 picode->is_mvfvp = (ttl_routines[ttl_idx].opnum == ttl_mvfvp_opn);
	 
		
   /*
    * Start by updating the opnum field. This allows using it later. 
    */
   opnum = ttl_routines[ttl_idx].opnum;
   picode->opnum = opnum;
   
   /*
    * Set pointer to TTL function
    */
   picode->func = desc_table[opnum].func;

   /*
    * Mark this instruction as a vector one
    */
   picode->is_vector = 1;

   /*
    * Set new flags corresponding to instruction
    */
   picode->iflags = 0;
   picode->iflags |= desc_table[picode->opnum].iflags; 

   /* We mark the instruction as being an 'ISA_EXTENSION' and then we
    * fill the 'extended_instr' field with the actual bits that define
    * the semantics of this new instruction.  Note that the value
    * '04000000' corresponds to a 0x1 in the 6-bit opcode field, a 0x0
    * in the 6-bit extension field, and everything else set to '0'
    *       6      5           20      
    *    oooooo eeeee zzzzzzzzzzzzzzzzzzzzz
    *    000001 00000 000000000000000000000
    *       |    |    |   |   |   |   |   |
    *     0   4    0    0   0   0   0   0
    */
   picode->instr = 0x04000000;

   /*
    * No vector instruction makes use of 'literal' or target. Therefore, clear them.
    */
   picode->literal = 0;
   picode->target = 0;


   switch ( ttl_routines[ttl_idx].class ) {
    case  TTL_MVTVP_CLASS:
	    if ( opnum == ttl_mvtvp_opn ) {
	    /*
	     * Picode contains a VAXFP instruction. Thus, RA, is F31, and RB and RC are expressed 
         * in terms of FP registers; their value
	     * must be normalized down to the 0..31 range. 
         * He expect RB holds the scalar register and RC the vector one
	     */

	     picode->args[RB] -= FIRST_FP_LOGICAL;
	     picode->args[RC] -= FIRST_FP_LOGICAL;

	    /*
	     * Construct a fake extended instruction to be given to the ASIM performance model. This fake instruction
	     * does not affect the behaviour of AINT at all
	     */
	     picode->extended_instr = build_iword(ttl_idx,picode->args[RA],picode->args[RB],picode->args[RC],0,picode->is_masked);

	     /*
	      * Our internal routine expects that RA be an INTEGER source register and that RC contains a number
	      * indicating which special register must be updated.
	      * We need to turn the RB of the VaxFP instruction into the appropriate RA
	      * We need to turn the RC of the VaxFp instruction into the appropriate RC
	      * We need to clean RB
	      */

	      picode->args[RA] = picode->args[RB];	                    /* already in 0..31 range */
	      picode->args[RC] = FIRST_VCTRL_LOGICAL + picode->args[RC];
	      picode->args[RB] = ZERO_REGISTER;	                        /* cleanup RB */
	      picode->dest     = RC;
	    }
	    else 
           panic("patch_vaxfp_insn","unknown opnum for ttl routine %d\n",ttl_routines[ttl_idx].name);
	    break;

    case  TTL_MVFVP_CLASS:
		/* We recive Rb with 0/1 => VMH/VML and Vc with scalar destination register */

        if ( picode->args[RC] == ZERO_REGISTER ) picode->args[RC] += FIRST_FP_LOGICAL;
		/* RB cannot be 31 ...*/
  	    picode->args[RB] -= FIRST_FP_LOGICAL;
  	    picode->args[RC] -= FIRST_FP_LOGICAL;
     	picode->extended_instr = build_iword(ttl_idx,picode->args[RA],picode->args[RB],picode->args[RC],0,picode->is_masked);
		/* in this case we dosn't need to change nothing at all in picode->args... */
	    picode->dest     = RC;
		break;
		
    case  TTL_MVFVR_CLASS:
		/* We have in Ra a vector reg, Rb and Rc scalar ones*/

		if ( picode->args[RA] == ZERO_REGISTER ) picode->args[RA] += FIRST_FP_LOGICAL;
		if ( picode->args[RB] == ZERO_REGISTER ) picode->args[RB] += FIRST_FP_LOGICAL;
		if ( picode->args[RC] == ZERO_REGISTER ) picode->args[RC] += FIRST_FP_LOGICAL;
		picode->args[RA] -= FIRST_FP_LOGICAL;
		picode->args[RB] -= FIRST_FP_LOGICAL;
		picode->args[RC] -= FIRST_FP_LOGICAL;

		picode->extended_instr = build_iword(ttl_idx,picode->args[RA],picode->args[RB],picode->args[RC],0,picode->is_masked);

		/* ok, Ra is a vector one! */
		picode->args[RA] += FIRST_VEC_LOGICAL;
	    picode->dest     = RC;
		
		break;
		
    case  TTL_IOTA_CLASS:
		/*
		 * We recive just 'Vc', Va and Vb must point to 31 (dummy). 
         * This VAXFP is considered a vector memory instruction. Restore the correct value of V31 in case RA happens
         * to be V31. AINT does this nasty thing of aliasing f31 to r31. Thus, if we had an f31 in some register field,
         * at this point in time AINT will have a '31' in that field (were, normally, you would expect a 31+32 = 63.
         * Thus we have to restore f31 to its proper value if present
		 */
         if ( picode->args[RC] == ZERO_REGISTER ) picode->args[RC] += FIRST_FP_LOGICAL;
  	     picode->args[RC] -= FIRST_FP_LOGICAL;

	    /*
	     * Construct a fake extended instruction to be given to the ASIM performance model. This fake instruction
	     * does not affect the behaviour of AINT at all
	     */
	     picode->extended_instr = build_iword(ttl_idx,picode->args[RA],picode->args[RB],picode->args[RC],0,picode->is_masked);

	    /*
	     * Now, update  register specifiers to be Vector registers
	     */
	     picode->args[RC] += FIRST_VEC_LOGICAL;
	     break;

    case  TTL_SETM_CLASS:
		/* We expect RB to hold to unique parameter. It will be always a vector register. */

        /*
         * This VAXFP is considered a vector memory instruction. Restore the correct value of V31 in case RA happens
         * to be V31. AINT does this nasty thing of aliasing f31 to r31. Thus, if we had an f31 in some register field,
         * at this point in time AINT will have a '31' in that field (were, normally, you would expect a 31+32 = 63.
         * Thus we have to restore f31 to its proper value if present
        */
		if ( picode->args[RB] == ZERO_REGISTER ) picode->args[RB] += FIRST_FP_LOGICAL;		
	    
	    /*
	     * Picode contains a 3op VAXFP instruction. Thus, RA, RB and RC are expressed in terms of FP registers; their values
	     * have to be normalized down to the 0..31 range.
	    */
		picode->args[RB] -= FIRST_FP_LOGICAL;

	    /*
	     * Construct a fake extended instruction to be given to the ASIM performance model. This fake instruction
	     * does not affect the behaviour of AINT at all
	     */

	     picode->extended_instr = build_iword(ttl_idx,picode->args[RA],picode->args[RB],picode->args[RC],0,picode->is_masked);
		
	    /*
	     * Now, update  register specifiers to be Vector registers
	    */
	     picode->args[RB] += FIRST_VEC_LOGICAL;
	     picode->args[RC] = VM_REG;
		 picode->dest = RC ;		
		 
		break;

    case  TTL_MEM_CLASS:
        /*
         * This VAXFP is considered a vector memory instruction. Restore the correct value of V31 in case RA happens
         * to be V31. AINT does this nasty thing of aliasing f31 to r31. Thus, if we had an f31 in some register field,
         * at this point in time AINT will have a '31' in that field (were, normally, you would expect a 31+32 = 63.
         * Thus we have to restore f31 to its proper value if present
         */
         if ( picode->args[RA] == ZERO_REGISTER ) picode->args[RA] += FIRST_FP_LOGICAL;

	    /*
	     * Picode contains a MEM VAXFP instruction. Thus, RA, is expressed in terms of FP registers; its value
	     * must be normalized down to the 0..31 range. RB need not be modified, since it's an integer register
	     */
  	     picode->args[RA] -= FIRST_FP_LOGICAL;

	    /*
	     * First, construct a fake extended instruction to be given to the ASIM performance model. This fake instruction
	     * does not affect the behaviour of AINT at all
	     */

        /* Federico Ardanaz notes:
         * Warning, picode->immed  have the general memory format displacement but in ttl we use less bits
         * to codify this field so I need to patch this:
         */
         picode->immed = ttl_ins.ttl_memory_instruction.disp;
	     picode->extended_instr = build_iword(ttl_idx,picode->args[RA],picode->args[RB],-1,picode->immed,picode->is_masked);

	    /*
	     * Now, update  RA to be a Vector register
	     */
		 
		/* Well, if we are dealing with a vector prefetch is better to leave ZERO_REGISTER anyway */
		 if (ttl_routines[ttl_idx].encoding.type != V_DTYPE_PF)
	     	picode->args[RA] += FIRST_VEC_LOGICAL;
			
	     break;

    case  TTL_VVV_CLASS:
        /*
         * This VAXFP is considered a vector instruction. Restore the correct value of V31 in case some register happens
         * to be V31. AINT does this nasty thing of aliasing f31 to r31. Thus, if we had an f31 in some register field,
         * at this point in time AINT will have a '31' in that field (were, normally, you would expect a 31+32 = 63.
         * Thus we have to restore f31 to its proper value if present
         */
         if ( picode->args[RA] == ZERO_REGISTER ) picode->args[RA] += FIRST_FP_LOGICAL;
         if ( picode->args[RB] == ZERO_REGISTER ) picode->args[RB] += FIRST_FP_LOGICAL;
         if ( picode->args[RC] == ZERO_REGISTER ) picode->args[RC] += FIRST_FP_LOGICAL;

	    /*
	     * Picode contains a 3op VAXFP instruction. Thus, RA, RB and RC are expressed in terms of FP registers; their values
	     * have to be normalized down to the 0..31 range.
	     */
	     picode->args[RA] -= FIRST_FP_LOGICAL;
	     picode->args[RB] -= FIRST_FP_LOGICAL;
	     picode->args[RC] -= FIRST_FP_LOGICAL;

	    /*
	     * Construct a fake extended instruction to be given to the ASIM performance model. This fake instruction
	     * does not affect the behaviour of AINT at all
	     */
	     picode->extended_instr = build_iword(ttl_idx,picode->args[RA],picode->args[RB],picode->args[RC],0,picode->is_masked);

	    /*
	     * Now, update  all three register specifiers to be Vector registers
	     */
	     picode->args[RA] += FIRST_VEC_LOGICAL;
	     picode->args[RB] += FIRST_VEC_LOGICAL;
	     picode->args[RC] += FIRST_VEC_LOGICAL;
	     break;

    case  TTL_VFV_CLASS:
        /*
         * This VAXFP is considered a vector instruction. Restore the correct value of V31 in case some register happens
         * to be V31. AINT does this nasty thing of aliasing f31 to r31. Thus, if we had an f31 in some register field,
         * at this point in time AINT will have a '31' in that field (were, normally, you would expect a 31+32 = 63.
         * Thus we have to restore f31 to its proper value if present
         */
         if ( picode->args[RA] == ZERO_REGISTER ) picode->args[RA] += FIRST_FP_LOGICAL;
         if ( picode->args[RC] == ZERO_REGISTER ) picode->args[RC] += FIRST_FP_LOGICAL;

	    /*
	     * Picode contains a 3op VAXFP instruction. Thus, RA, RB and RC are expressed in terms of FP registers; their values
	     * have to be normalized down to the 0..31 range.
	     */
	     picode->args[RA] -= FIRST_FP_LOGICAL;
	     picode->args[RB] -= FIRST_FP_LOGICAL;
	     picode->args[RC] -= FIRST_FP_LOGICAL;

	    /*
	     * First, construct a fake extended instruction to be given to the ASIM performance model. This fake instruction
	     * does not affect the behaviour of AINT at all
	     */
	     picode->extended_instr = build_iword(ttl_idx,picode->args[RA],picode->args[RB],picode->args[RC],0,picode->is_masked);

	    /*
	     * Now, update  RA and RC (but not RB) register specifiers to be Vector registers. Make RB go back to its normal FP value.
	     */
	     picode->args[RA] += FIRST_VEC_LOGICAL;
	     picode->args[RB] += FIRST_FP_LOGICAL;
	     picode->args[RC] += FIRST_VEC_LOGICAL;
	     break;

    case  TTL_VIV_CLASS:
        /*
         * This VAXFP is considered a vector instruction. Restore the correct value of V31 in case some register happens
         * to be V31. AINT does this nasty thing of aliasing f31 to r31. Thus, if we had an f31 in some register field,
         * at this point in time AINT will have a '31' in that field (were, normally, you would expect a 31+32 = 63.
         * Thus we have to restore f31 to its proper value if present
         */
         if ( picode->args[RA] == ZERO_REGISTER ) picode->args[RA] += FIRST_FP_LOGICAL;
         if ( picode->args[RB] == ZERO_REGISTER ) picode->args[RB] += FIRST_FP_LOGICAL;
         if ( picode->args[RC] == ZERO_REGISTER ) picode->args[RC] += FIRST_FP_LOGICAL;

	    /*
	     * Picode contains a 3op VAXFP instruction. Thus, RA, RB and RC are expressed in terms of FP registers; their values
	     * have to be normalized down to the 0..31 range.
	     */
	     picode->args[RA] -= FIRST_FP_LOGICAL;
	     picode->args[RB] -= FIRST_FP_LOGICAL;
	     picode->args[RC] -= FIRST_FP_LOGICAL;

	    /*
	     * First, construct a fake extended instruction to be given to the ASIM performance model. This fake instruction
	     * does not affect the behaviour of AINT at all
	     */
	     picode->extended_instr = build_iword(ttl_idx,picode->args[RA],picode->args[RB],picode->args[RC],0,picode->is_masked);

	    /*
	     * Now, update  RA and RC (but not RB) register specifiers to be Vector registers. 
	     */
	     picode->args[RA] += FIRST_VEC_LOGICAL;
	     picode->args[RC] += FIRST_VEC_LOGICAL;
	    break;
		
	case  TTL_GATH_CLASS:
	     /* a gather has not immediate filed */
         picode->immed = 0;
		 
		/* Same as VIV but we initialize dest field */
         if ( picode->args[RA] == ZERO_REGISTER ) picode->args[RA] += FIRST_FP_LOGICAL;
         if ( picode->args[RB] == ZERO_REGISTER ) picode->args[RB] += FIRST_FP_LOGICAL;
         if ( picode->args[RC] == ZERO_REGISTER ) picode->args[RC] += FIRST_FP_LOGICAL;
	     picode->args[RA] -= FIRST_FP_LOGICAL;
	     picode->args[RB] -= FIRST_FP_LOGICAL;
	     picode->args[RC] -= FIRST_FP_LOGICAL;
	     picode->extended_instr = build_iword(ttl_idx,picode->args[RA],picode->args[RB],picode->args[RC],0,picode->is_masked);

	     picode->args[RA] += FIRST_VEC_LOGICAL;

		/* Well, if we are dealing with a vector prefetch is better to leave ZERO_REGISTER anyway */
		 if (ttl_routines[ttl_idx].encoding.type != V_DTYPE_PF)
	     	picode->args[RC] += FIRST_VEC_LOGICAL;
		 
		 picode->dest = RC;
	     break;
		 
	case  TTL_SCAT_CLASS:
	     /* a gather has not immediate filed */
         picode->immed = 0;

		/* Same as VIV but we de-initialize dest field */
         if ( picode->args[RA] == ZERO_REGISTER ) picode->args[RA] += FIRST_FP_LOGICAL;
         if ( picode->args[RB] == ZERO_REGISTER ) picode->args[RB] += FIRST_FP_LOGICAL;
         if ( picode->args[RC] == ZERO_REGISTER ) picode->args[RC] += FIRST_FP_LOGICAL;
	     picode->args[RA] -= FIRST_FP_LOGICAL;
	     picode->args[RB] -= FIRST_FP_LOGICAL;
	     picode->args[RC] -= FIRST_FP_LOGICAL;
	     picode->extended_instr = build_iword(ttl_idx,picode->args[RA],picode->args[RB],picode->args[RC],0,picode->is_masked);
	     picode->args[RA] += FIRST_VEC_LOGICAL;

		/* Well, if we are dealing with a vector prefetch is better to leave ZERO_REGISTER anyway */
		 if (ttl_routines[ttl_idx].encoding.type != V_DTYPE_PF)
	     	picode->args[RC] += FIRST_VEC_LOGICAL;
		 
		 picode->dest = MaxArgs;
	     break;
		
	
    case  TTL_NVV_CLASS:
        /*
         * This VAXFP is considered a vector instruction. Restore the correct value of V31 in case some register happens
         * to be V31. AINT does this nasty thing of aliasing f31 to r31. Thus, if we had an f31 in some register field,
         * at this point in time AINT will have a '31' in that field (were, normally, you would expect a 31+32 = 63.
         * Thus we have to restore f31 to its proper value if present
         */
         if ( picode->args[RB] == ZERO_REGISTER ) picode->args[RB] += FIRST_FP_LOGICAL;
         if ( picode->args[RC] == ZERO_REGISTER ) picode->args[RC] += FIRST_FP_LOGICAL;

	    /*
	     * Picode contains a 3op VAXFP instruction. Thus, RA, RB and RC are expressed in terms of FP registers; their values
	     * have to be normalized down to the 0..31 range.
	     */
	     picode->args[RB] -= FIRST_FP_LOGICAL;
	     picode->args[RC] -= FIRST_FP_LOGICAL;

	    /*
	     * Construct a fake extended instruction to be given to the ASIM performance model. This fake instruction
	     * does not affect the behaviour of AINT at all
	     */
	     picode->extended_instr = build_iword(ttl_idx,picode->args[RA],picode->args[RB],picode->args[RC],0,picode->is_masked);

	    /*
	     * Now, update  register specifiers to be Vector registers
	     */
	     picode->args[RB] += FIRST_VEC_LOGICAL;
	     picode->args[RC] += FIRST_VEC_LOGICAL;
	     break;
		 
	case TTL_SYNCH_CLASS:
		/* make sure that everybody is ZERO_REGISTER */
		picode->args[RA] = ZERO_REGISTER;
		picode->args[RB] = ZERO_REGISTER;
		picode->args[RC] = ZERO_REGISTER;
	    picode->extended_instr = build_iword(ttl_idx,picode->args[RA],picode->args[RB],picode->args[RC],0,picode->is_masked);
		break;

    default:
	    panic("patch_vaxfp_insn","unknown class for ttl routine %s\n",ttl_routines[ttl_idx].name);
	    break;
   }

   fprintf(Aint_output,"%s (0x%lx)\t ra=%d,rb=%d --> rc=%d iflags=%d instr=%lx einstr=%lx\n",
		ttl_routines[ttl_idx].name,
		picode->func,
		picode->args[RA],
		picode->args[RB],
		picode->args[RC],
		picode->iflags,
		picode->instr,
		picode->extended_instr
	  );

}

#endif /* NEW_TTL */



#ifdef NEW_TLDS
/* 
 * Routines for loading executables and shared objects
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <sys/utsname.h>
#include <string.h>
#include </usr/include/assert.h>

#include <elf_abi.h>
#include <elf_mips.h>
#include "opcodes.h"
#include "globals.h"
#include "protos.h"
#include "bfd.h"
#include "obstack.h"

#include "ttl_loader.h"


char 	*tlds_min_entrypc;
char 	*tlds_max_entrypc;


/*
 * This routine searches for all known TLDS routines in the symbol table and tries to 
 * dicover their entry point
 */
void
tlds_find_entry_points(char *Objname)
{
  int i;
  const char 	*names[TLDS_MAX_INSNS+1];
  char 		*targets[TLDS_MAX_INSNS+1];


  /* 
   * Build array of names in a suitable format for 'find_proc_addrs'
   */
  for ( i = 0; i < TLDS_MAX_INSNS; i++ ) names[i] = tlds_routines[i].name;
  names[i] = NULL;

  /*
   * Get entry point for each TLDS routine
   */
  find_proc_addrs(Objname, names, targets);

  /*
   * Copy back each target to the corresponding routine
   * At the same time, find min and max entry points, to speed up 
   * checking of pointers to TLDS routines
   */
  tlds_min_entrypc = 0xffffffffffffffff;
  tlds_max_entrypc = 0;

  for ( i = 0; i < TLDS_MAX_INSNS; i++ ) { 
#if 1 /* this code assumes a gp prologue before every tlds function */
    targets[i] += 8;
#endif
    tlds_routines[i].entrypc = targets[i]; 
    if ( targets[i] < tlds_min_entrypc ) tlds_min_entrypc = targets[i];
    if ( targets[i] > tlds_max_entrypc ) tlds_max_entrypc = targets[i];
    fprintf(Aint_output,"%s = %lx\n",tlds_routines[i].name,tlds_routines[i].entrypc);
  }
  fprintf(Aint_output,"tlds_min_entrypc = %lx\n",tlds_min_entrypc);
  fprintf(Aint_output,"tlds_max_entrypc = %lx\n",tlds_max_entrypc);
}

int
is_call_to_tlds(char *target)
{
  int i;

  /*
   * Fast check
   */
  if ( target < tlds_min_entrypc ) return -1;
  if ( target > tlds_max_entrypc ) return -1;

  fprintf(Aint_output,"is_call_to_tlds: %lx\n",target);

  /*
   * Now search for exact match to each entry point
   */
  for ( i = 0; i < TLDS_MAX_INSNS; i++ ) {
    if ( tlds_routines[i].entrypc == target ) return i;
  }

  return -1;
}

UINT64
build_iword(int tlds_idx, int ra, int rb, int rc, int disp)
{
  EXTENDED_ALPHA_INSTRUCTION w;

  w.Long = 0;

  if ( ra != -1 && (ra < 0 || ra > 31))  panic("build_iword: ra out of range: %d\n",ra);
  if ( rb != -1 && (rb < 0 || rb > 31))  panic("build_iword: rb out of range: %d\n",rb);
  if ( rc != -1 && (rc < 0 || rc > 31))  panic("build_iword: rc out of range: %d\n",rc);

  if ( tlds_routines[tlds_idx].class == TLDS_CMD ) {
    w.TLDSCmd.Opcode = tlds_routines[tlds_idx].encoding.opcode;
  } else if ( tlds_routines[tlds_idx].class == TLDS_1ARG ) {
    w.TLDS1Arg.Opcode = tlds_routines[tlds_idx].encoding.opcode;
    w.TLDS1Arg.Ra = ra;
  }
  else {
    panic("build_iword: class %d for idx %d (%s) unknown\n",
          tlds_routines[tlds_idx].class,tlds_idx,tlds_routines[tlds_idx].name);
  }

  return w.Long;
}


void 
tlds_patch_binary(process_ptr process, icode_ptr picode, ulong target)
{
  int	i,ridx,tlds_idx,nargs,opnum;
  ulong textaddr;


  /*
   * In the following structure we keep the information related to each 
   * argument to this tlds subroutine call. Note that numbering in the array
   * is as follows: 0 is the first argument, thus, the argument further away
   * from the BSR instruction
   */
  struct {
   icode_ptr picode;			/* pointer to insn creating the argument */
  } args[MAX_TLDS_ARGS];
  /*
   * The first thing to do is determine how many arguments does this
   * vector instruction have
   */
  tlds_idx = is_call_to_tlds(target);
  assert(tlds_idx != -1);
  nargs = tlds_routines[tlds_idx].nargs;

  /* 
   * We are ready to begin transformation of our packet of nargs+1 instructions into a real
   * vector instruction
   */

  /*
   * Start by updating the opnum field. This allows using it later.
   */
  opnum = tlds_routines[tlds_idx].opnum;
  picode->opnum = opnum;

  /*
   * Set pointer to TLDS function
   */
  picode->func = desc_table[opnum].func;
  picode->is_vector = 0;
  picode->is_masked = 0;
  
  /*
   * We clean up the E_BSR flag that we have set (by definition, since this is a a BSR call
   * to a tlds subroutine) and then we OR the proper TLDS flags. We also have to clear the E_BB_LAST i
   * flag, since this is no longer the end of a basic block. As a consequence, the first instruction
   * after the BSR (if any) should have its E_BB_FIRST flag cleared.
   */
  picode->iflags &= ~(E_BSR);
  picode->iflags &= ~(E_BB_LAST);
  picode->iflags |= desc_table[picode->opnum].iflags; 
  assert(picode->next);
  if (picode->next != NULL) picode->next->iflags &= ~(E_BB_FIRST);

   /* We mark the instruction as being an 'ISA_EXTENSION' and then we
    * fill the 'extended_instr' field with the actual bits that define
    * the semantics of this new instruction.  Note that the value
    * '04100000' corresponds to a 0x1 in the 6-bit opcode field, a 0x1
    * in the 6-bit extension field, and everything else set to '0'
    *       6      6            20          
    *    oooooo eeeee zzzzzzzzzzzzzzzzzzzzz
    *    000001 00001 000000000000000000000
    *       |    |    |   |   |   |   |   |
    *     0   4    1    0   0   0   0   0
    */
  picode->instr = 0x04000000;

  /*
   * Set the immed and literal fields to 0 and let the logic below fill them if needed
   */
  picode->immed = 0;
  picode->literal = 0;
  picode->size = 0;
  picode->target = 0;

  switch ( tlds_routines[tlds_idx].class ) {

  case  TLDS_CMD:
    picode->args[RC] = ZERO_REGISTER;
    picode->args[RB] = ZERO_REGISTER;
    picode->args[RA] = ZERO_REGISTER;
    picode->dest     = RC;
    picode->extended_instr = build_iword(tlds_idx,0,0,0,0);
	break;

  case  TLDS_1ARG:
    picode->args[RC] = ZERO_REGISTER;
    picode->args[RB] = ZERO_REGISTER;
    picode->args[RA] = 16;
    picode->dest     = RC;
    picode->extended_instr = build_iword(tlds_idx,16,0,0,0);
	break;

  default:
	panic("tlds_patch_binary","unknown class for tlds routine %d\n",tlds_routines[tlds_idx].name);
	break;
  }

  fprintf(Aint_output,"%s (0x%lx)\t RA=%d,RB=%d --> RC=%d iflags=%d instr=%lx einstr=%lx\n",
          tlds_routines[tlds_idx].name,
          picode->func,
          picode->args[RA],
          picode->args[RB],
          picode->args[RC],
          picode->iflags,
          picode->instr,
          picode->extended_instr
          );
}


#endif



