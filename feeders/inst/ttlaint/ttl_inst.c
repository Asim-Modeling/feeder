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
 * Assembly language routines for implementing TTL instructions 
 * We define some macros to ease common 3op/2op instructions.
 */

#include <stdio.h>
#include <math.h>
#include "globals.h"
#include "opcodes.h"
#include "protos.h"


static char *special2name[] = { "HWVL", "vl", "vs", "vm", "INVAL" };

static int	vl_reg;
static int	vs_reg;
static long	vm_reg;

int	ttl_debug = 20;


#define PHREG2PHVREG(pr) (((pr) - FIRST_VECTOR_PHYSICAL)/MAX_VECTOR_LENGTH)


#define DEBUG_VMEM(name_)	                                                                            \
  informative_ttl(10,"%lx. %-15s v%d <%d eq v%d>, %d($%d)\t using VL=<%d,val=%ld> VS=<%d,val=%ld>\n",	\
	  picode->addr,											                                            \
          name_,											                                            \
	  picode->args[RA] - FIRST_VEC_LOGICAL, ifi->args[RA], PHREG2PHVREG(ifi->args[RA]),		            \
	  picode->immed, picode->args[RB],								                                    \
	  ifi->vl, VL,											                                            \
	  ifi->vs, VS)

#define DEBUG_VVV(NAME)	                                                                                            \
  informative_ttl(10,#NAME " %lx. v%d <%d eq v%d>, v%d <%d eq v%d> --> v%d <%d eq v%d>\t using VL=<%d,val=%ld>\n",	\
	  picode->addr,												                                                    \
	  picode->args[RA] - FIRST_VEC_LOGICAL, ifi->args[RA], PHREG2PHVREG(ifi->args[RA]),			                    \
	  picode->args[RB] - FIRST_VEC_LOGICAL, ifi->args[RB], PHREG2PHVREG(ifi->args[RB]),			                    \
	  picode->args[RC] - FIRST_VEC_LOGICAL, ifi->args[RC], PHREG2PHVREG(ifi->args[RC]),			                    \
	  ifi->vl, VL)

#define DEBUG_VFV(NAME)	                                                                                            \
  informative_ttl(10,#NAME " %lx. v%d <%d eq v%d>, $f%d <l=%d p=%d> --> v%d <%d eq v%d>\t using VL=<%d,val=%ld>\n",	\
	  picode->addr,												                                                    \
	  picode->args[RA] - FIRST_VEC_LOGICAL, ifi->args[RA], PHREG2PHVREG(ifi->args[RA]),			                    \
	  picode->args[RB] - FIRST_FP_LOGICAL,  picode->args[RB], ifi->args[RB], 				                        \
	  picode->args[RC] - FIRST_VEC_LOGICAL, ifi->args[RC], PHREG2PHVREG(ifi->args[RC]),			                    \
	  ifi->vl, VL)

#define DEBUG_VIV(NAME)	                                                                                            \
  informative_ttl(10,#NAME " %lx. v%d <%d eq v%d>, $%d <l=%d p=%d> --> v%d <%d eq v%d>\t using VL=<%d,val=%ld>\n",	\
	  picode->addr,												                                                    \
	  picode->args[RA] - FIRST_VEC_LOGICAL, ifi->args[RA], PHREG2PHVREG(ifi->args[RA]),			                    \
	  picode->args[RB],  picode->args[RB], ifi->args[RB],															\
	  picode->args[RC] - FIRST_VEC_LOGICAL, ifi->args[RC], PHREG2PHVREG(ifi->args[RC]),			                    \
	  ifi->vl, VL)

#define DEBUG_VV(NAME)	                                                                                            \
  informative_ttl(10,#NAME " %lx. v%d <%d eq v%d> --> v%d <%d eq v%d>\t using VL=<%d,val=%ld>\n",	\
	  picode->addr,												                                                    \
	  picode->args[RB] - FIRST_VEC_LOGICAL, ifi->args[RB], PHREG2PHVREG(ifi->args[RB]),			                    \
	  picode->args[RC] - FIRST_VEC_LOGICAL, ifi->args[RC], PHREG2PHVREG(ifi->args[RC]),			                    \
	  ifi->vl, VL)


#define CHECK_FPCR(ifi, pthread)

#define VECTOR_3OPf(NAME,OPCODE,bsource,op2type)						\
OP(NAME ## _f) 													        \
{ 														                \
 int i,vl = VL; 												        \
 if ( op2type == 1 ) { DEBUG_VVV(NAME); } else { DEBUG_VFV(NAME); }		\
 for ( i = 0; i < vl; i++ ) {											\
  register double fav; 													\
  register double fbv; 													\
  register double fcv; 													\
  register double fpcr;													\
  if ((picode->is_masked)&&(!GETVM(i)))	continue;						\
  fav = VELEMFP(RA,i);									  	  	  	    \
  fbv = bsource;     									  	  	  	    \
  fcv = VELEMFP(RC,i); 									  	  	  	    \
  fpcr = *(double*)&pthread->fpcr; 						  	  	  	    \
  informative_ttl(10,#NAME " [%d]: (%lg) op (%lg) = ",i,fav,fbv);		\
  asm("mt_fpcr %4; trapb;" #OPCODE " %2, %3, %0; trapb; mf_fpcr %1 " 	\
  : "=f" (fcv) , "=f" (fpcr) 											\
  : "f" (fav) , "f" (fbv), "f" (fpcr)); 								\
  VELEMFP(RC,i) = fcv; 												    \
  informative_ttl(10,"%lg = inreg=%lg\n",fcv,VELEMFP(RC,i));			\
  *(double*)&pthread -> fpcr = fpcr; 									\
  CHECK_FPCR(ifi, pthread); 											\
  MapFP(ZERO_REGISTER) = 0.0; 											\
 } 														                \
 return picode->next; 												    \
}

#define VECTOR_3OPi(NAME,OPCODE,bsource,op2type)						\
OP(NAME ## _f) 													        \
{ 														                \
 int i,vl = VL; 												        \
 if ( op2type == 1 ) { DEBUG_VVV(NAME); } else { DEBUG_VIV(NAME); }		\
 for ( i = 0; i < vl; i++ ) {											\
  register long fav; 													\
  register long fbv; 													\
  register long fcv; 													\
  if ((picode->is_masked)&&(!GETVM(i)))   continue;						\
  fav = VELEMINT(RA,i); 												\
  fbv = bsource;														\
  fcv = VELEMINT(RC,i); 												\
  informative_ttl(10,#NAME " [%d]: (%ld) op (%ld) = ",i,fav,fbv);		\
  asm("trapb;" #OPCODE " %1, %2, %0; trapb;" 						    \
  : "=&r" (fcv) : "r" (fav) , "r" (fbv) ); 								\
  VELEMINT(RC,i) = fcv; 												\
  informative_ttl(10,"%ld = inreg=%ld\n",fcv,VELEMINT(RC,i));			\
 } 														                \
 return picode->next; 												    \
}																		\


#define VECTOR_2OPf(NAME,OPCODE)							            \
OP(NAME ## _f) 													        \
{ 														                \
 int i,vl = VL; 												        \
 DEBUG_VVV(NAME);														\
 for ( i = 0; i < vl; i++ ) {											\
  register double fbv;     			  									\
  register double fcv;													\
  register double fpcr; 												\
  if ((picode->is_masked)&&(!GETVM(i))) continue;						\
  fbv = VELEMFP(RB,i); 													\
  fpcr = *(double*)&pthread->fpcr; 										\
  informative_ttl(10,#NAME " [%d]: (%lg) op ->",i,fbv);	    			\
  asm("mt_fpcr %3; trapb;" #OPCODE " %2, %0; trapb; mf_fpcr %1 " 		\
  : "=f" (fcv) , "=f" (fpcr) 											\
  : "f" (fbv) , "f" (fpcr)); 									        \
  VELEMFP(RC,i) = fcv; 												    \
  informative_ttl(10,"%lg\n",fcv);	    								\
  *(double*)&pthread -> fpcr = fpcr; 									\
  CHECK_FPCR(ifi, pthread); 											\
  MapFP(ZERO_REGISTER) = 0.0; 											\
 } 														                \
 return picode->next; 												    \
}


/*	Unused right now
    ================
	
	#define VECTOR_2OPi(NAME,OPCODE)							            \
	OP(NAME ## _f) 													        \
	{ 														                \
	 int i,vl = VL; 											    	    \
	 // <TO DO> DEBUG_VV(NAME); 					                        \
	 for ( i = 0; i < vl; i++ ) {											\
	  register long fbv = VELEMINT(RB,i); 									\
	  register long fcv; 		                							\
	  asm("trapb;" #OPCODE " %1, %0; trapb" 						        \
	  : "=&r" (fcv) : "r" (fbv)); 									        \
	  VELEMINT(RC,i) = fcv; 												\
	  informative_ttl(10,#NAME " [%d]: (%ld) op ->%ld\n",i,fbv,fcv);	    \
	 } 														                \
	 return picode->next; 												    \
	}
*/


/*
 * Tarantula vector routines added here
 */

OP(ttl_mvtvp_f)
{
 informative_ttl(10,"%lx. ttl_mvtvp_f: mov reg $%d (phreg<%d,val=%ld>) to special %s(%d) (phreg<%d,xxx>)\n",
           picode->addr,
           picode->args[RA], CurMap(picode->args[RA]), REG(RA),
           special2name[picode->args[RC]-FIRST_VCTRL_LOGICAL], picode->args[RC], ifi->args[RC]
          );
    
  if ( picode->args[RC] != VL_REG && picode->args[RC] != VS_REG )
   fatal("ttl_mvtvp_f: special dest %d invalid\nGood values are VL_REG=%d, VS_REG=%d\n",picode->args[RC], VL_REG, VS_REG);


  /*
   * Store the value to the special register, (either VS or VL)
   */
  REG(RC) = REG(RA); 
  return picode->next;
}

OP(ttl_mvfvp_f)
{
	static char* vmsg[] = {"VMH", "VML"};
	char*	tmp;
	int pr = ifi->args[RC];
	
	// we expect RB have 0->VMH, 1->VML and RC the scalar destination register:
	REG(RC) = pthread->Reg[ifi->vm].Mask[picode->args[RB]];
	tmp = vmsg[RB];
	informative_ttl(10,"ttl_mvfvp_f: mov special %s to reg $%d with value %lx\n",tmp,picode->args[RC], REG(RC));

  return picode->next;
}

OP(ttl_mvfvr_f)
{
	REG(RC) = VELEMINT(RA,REG(RB));
	informative_ttl(10,"ttl_mvfvr_f: moving pos v%d[%d] (vec phreg %d), value %lx to scalar $%d (phreg %d)\n",
	  picode->args[RA] - FIRST_VEC_LOGICAL,REG(RB),ifi->args[RA],VELEMINT(RA,REG(RB)),picode->args[RC],ifi->args[RC]);
	  
	return picode->next;
}

/* Federico Ardanaz notes:

   By ISA definition setvm/setnvm instruction set all the 128 mask bits no matter what
   the VL says right now. This is a problem to the verifier'cause when we are working with
   VL<128 we set the mask with garbagge and the garbagge in the verifier thread use to be
   different from the backend... therefore as an easy workarround to this we put 0's in
   the bits beyond the VL
*/

OP(ttl_vsetnvm_f)
{
 register long fbv;
 register int vl = VL; 												        
 int i;  
 for (i=0; i< vl; i++)
 {
  register long fbv = VELEMINT(RB,i);									
  
  if (!fbv)
  	SETVM(i);
  else
  	CLEARVM(i);
	
  informative_ttl(10,"vsetnvm[%d]: (%ld) => inmask=%ld\n",	
           i,fbv,GETVM(i)); 								
 } 		
 
 for (i = vl; i < MAX_VECTOR_LENGTH; i++ ) {
  	CLEARVM(i);
  informative_ttl(10,"vsetnvm[%d]: (%ld) => inmask=%ld (beyond VL)\n",	
           i,0,GETVM(i)); 								
 } 		

 return picode->next; 												    
}

OP(ttl_vsetvm_f)
{
 register long fbv;
 register int vl = VL; 												        
 int i;

 for ( i = 0; i <vl ; i++ ) {											
  register long fbv = VELEMINT(RB,i);									
  if (fbv)
  	SETVM(i);
  else
  	CLEARVM(i);
  
  informative_ttl(10,"vsetvm[%d]: (%ld) => inmask=%ld\n",	
           i,fbv,GETVM(i)); 								
 } 		

 for ( i = vl; i < MAX_VECTOR_LENGTH; i++ ) {											
  	CLEARVM(i);
  informative_ttl(10,"vsetvm[%d]: (%ld) => inmask=%ld\n",	
           i,0,GETVM(i)); 								
 } 		

 return picode->next; 												    
}

/* Not all ALPHA implementations have ctlz,cttz and ctpop instructions
   so we use by hand implementation
*/
OP(ttl_vctlz_f)
{
	/* Federico Ardanaz notes:
	 * I know this is a naivy way to implement this instruction but it's easy and the
	 * frecuency of this kind of instrucctions in real world code is very low...
	 */
 register int cnt,i,j,vl = VL; 												        
 register long fbv;
 
 for ( i = 0; i < vl; i++ ) {											
  if ((picode->is_masked)&&(!GETVM(i))) continue;							
  fbv = VELEMINT(RB,i);									
  cnt = 0;
  j = 63;
  while ((j>=0) && !ISVALID_L(fbv,j) )
  {
  	++cnt;
	--j;
  }
	
  VELEMINT(RC,i) = (long)cnt; 												
  informative_ttl(10,"ttl_vctlz [%d]: (%lx) -> %ld\n",i,fbv,VELEMINT(RC,i)); 								
 }
 return picode->next; 												    
}

OP(ttl_vcttz_f)
{
 register int cnt,i,j,vl = VL; 												        
 register long fbv;
 
 for ( i = 0; i < vl; i++ ) {											
  if ((picode->is_masked)&&(!GETVM(i))) continue;							
  fbv = VELEMINT(RB,i);									
  cnt = 0;
  j = 0;
  while ((j<64) && !ISVALID_L(fbv,j))
  {
  	++cnt;
	++j;
  }
	
  VELEMINT(RC,i) = (long)cnt; 												
  informative_ttl(10,"ttl_vcttz [%d]: (%lx) -> %ld\n",i,fbv,VELEMINT(RC,i)); 								
 }
 return picode->next; 												    
}


OP(ttl_vctpop_f)
{
 register int i,j,cnt,vl = VL; 												        
 register long fbv;
 
 for ( i = 0; i < vl; i++ ) {											
  if ((picode->is_masked)&&(!GETVM(i))) continue;							
  fbv = VELEMINT(RB,i);
  cnt = 0;
  for (j=0;j<64;j++)
  	if (ISVALID_L(fbv,j)) ++cnt;
										
  VELEMINT(RC,i) = cnt;
  informative_ttl(10,"ttl_vctpop [%d]: (%lx) -> %ld\n",i,fbv,VELEMINT(RC,i)); 								
 }
 return picode->next; 												    
 
}

/* Not all ALPHA implementations have sqrt by hard so we use library 
   implementation instead
*/   

OP(ttl_vsqrtt_f)
{

 int i,vl = VL; 												

 for ( i = 0; i < vl; i++ ) {											
  register double fbv; 										
  register double fcv; 		                								
  register double fpcr; 								
  
  if ((picode->is_masked)&&(!GETVM(i))) continue;							

  fbv = VELEMFP(RB,i); 										
  fpcr = *(double*)&pthread->fpcr; 								

  asm("mt_fpcr %0; trapb " : : "f" (fpcr));
  fcv = sqrt(fbv);
  asm("trapb; mf_fpcr %0 " : "=f" (fpcr));
  VELEMFP(RC,i) = fcv; 												
    
  informative_ttl(10,"ttl_vsqrtt_f [%d]: (%lg) op ->%lg\n",i,fbv,fcv);	                
  *(double*)&pthread -> fpcr = fpcr; 										

  CHECK_FPCR(ifi, pthread); 											
  MapFP(ZERO_REGISTER) = 0.0; 											
 } 														
 return picode->next; 												
}

OP(ttl_vsqrts_f)
{
 int i,vl = VL; 												

 for ( i = 0; i < vl; i++ ) {											
  register double fbv; 										
  register float fcv; 		                								
  register double fpcr; 								
  
  if ((picode->is_masked)&&(!GETVM(i))) continue;							

  fbv = VELEMFP(RB,i); 										
  fpcr = *(double*)&pthread->fpcr; 								

  asm("mt_fpcr %0; trapb " : : "f" (fpcr));
  fcv = sqrt(fbv);
  asm("trapb; mf_fpcr %0 " : "=f" (fpcr));
  VELEMFP(RC,i) = fcv; 												
    
  informative_ttl(10,"ttl_vsqrts_f [%d]: (%lg) op ->%lg\n",i,fbv,fcv);	                
  *(double*)&pthread -> fpcr = fpcr; 										

  CHECK_FPCR(ifi, pthread); 											
  MapFP(ZERO_REGISTER) = 0.0; 											
 } 														
 return picode->next; 												
}

/* Some special kind instructions we need simulate by hand */
OP(ttl_vextsl_f)
{
	// --> the expected behaviour is
    //    FOR (i=0; i<VL; i++)
	//		Vc[i] <- Vb[i]<31> || MAP_S(Vb[i]<30:23>) || Vb[i]<22:0> || 0<28:0>
	//	  ENFOR
    //    FOR (i=VL; i<HWVL; i++)
    //       Vc[i] <- UNDEFINED
    //    ENDFOR
	// ------------- Well extsl is the same as scalar itofs but between vectors...
	// ok, not all alpha implementations has itofs so we make this by hand...
	// remember that a itofs are semantically equivalent to a stl + lds sequence:
	
 int i,vl = VL; 												       

 for ( i = 0; i < vl; i++ ) {										
  long   fbv; 								
  double fcv; 		                						
  long dummy;
  
  if ((picode->is_masked)&&(!GETVM(i))) continue;							

  fbv = VELEMINT(RB,i); 								

  asm("stl %1, 0(%2)\n\t"
      "lds %0, 0(%2)\n"
	  : "=f" (fcv) : "r" (fbv), "r" (&dummy));

  VELEMFP(RC,i) = fcv; 												   
  informative_ttl(10,"ttl_vextsl [%d]: (%lx) op ->%lg\n",i,fbv,fcv);	   
 } 														               
 return picode->next; 												   
	
}


OP(ttl_vextsh_f)
{
 int i,vl = VL; 												       

 for ( i = 0; i < vl; i++ ) {										
  register long fbv; 								
  double fcv; 		                						
  long dummy;
  
  if ((picode->is_masked)&&(!GETVM(i))) continue;							

  fbv = VELEMINT(RB,i) >> 32 ; 								

  asm("stl %1, 0(%2)\n\t"
      "lds %0, 0(%2)\n"
	  : "=f" (fcv) : "r" (fbv), "r" (&dummy));

  VELEMFP(RC,i) = fcv; 												   
  informative_ttl(10,"ttl_vextsh [%d]: (%lx) op ->%lg\n",i,fbv,fcv);	   
 }
 return picode->next; 												   
	
}

OP(ttl_viota_f)
{
	/*
	// --> the expected behaviour is
    //    FOR (i=0; i<VL; i++)
	//		Vc[i] <- VS*i
	//	  ENFOR
    //    FOR (i=VL; i<HWVL; i++)
    //       Vc[i] <- UNDEFINED
    //    ENDFOR
	*/
	
 register long vs = VS;
 register int i,vl = VL; 												

 for ( i = 0; i < vl; i++ ) {											
  
  VELEMINT(RC,i) = (long)i*vs; 												
  informative_ttl(10,"viota[%d]: vs=(%ld) -> %ld\n",	
           i,vs,VELEMINT(RC,i)); 								
 } 		
 return picode->next; 												    
}

OP(ttl_vskewh_f)
{
	// --> the expected behaviour is
	//    VC[0] <- 0
    //    FOR (i=0; i < min(VL-1,HWVL-2); i++)
	//		Vc[i+1] <- Vb[i]
	//	  ENFOR
    //    FOR (i=VL+1; i<HWVL; i++)
    //       Vc[i] <- UNDEFINED
    //    ENDFOR
	
 register int i; 												
 register int top ;

 top = VL-1 < MAX_VECTOR_LENGTH-2 ? VL - 1 : MAX_VECTOR_LENGTH-2 ; 
 
 VELEMINT(RC,0) = 0;
 informative_ttl(10,"vskewh[0]: -> 0 (by definition)\n"); 								
 for ( i = 0; i < top; i++ ) {											
  
  VELEMINT(RC,i+1) = VELEMINT(RB,i);
  informative_ttl(10,"vskewh[%d]: vb[%d]=%ld -> vc[%d]=%ld\n",i ,i ,
  					VELEMINT(RB,i), i+1, VELEMINT(RC,i+1)); 								
 } 		
 return picode->next; 												    
}


OP(ttl_vskewl_f)
{
	// --> the expected behaviour is
    //    FOR (i=1; i < VL; i++)
	//		Vc[i-1] <- Vb[i]
	//	  ENFOR
    //    FOR (i=VL-1; i<HWVL; i++)
    //       Vc[i] <- UNDEFINED
    //    ENDFOR
	
 register int i, vl = VL; 												
 for ( i = 1; i < vl; i++ ) {											
  
  VELEMINT(RC,i-1) = VELEMINT(RB,i);
  informative_ttl(10,"vskewl[%d]: vb[%d]=%ld -> vc[%d]=%ld\n",i-1 ,i ,
  					VELEMINT(RB,i), i-1, VELEMINT(RC,i-1)); 								
 } 		
 return picode->next; 												    
}

OP(ttl_vcmpr_f)
{
    // --> the expected behaviour is
	//	  j <- 0
    //    FOR (i=0; i<VL; i++)
    //       IF (VM<i> EQ 1) THEN
    //          Vc[i] <- Vb[i]
	//			j <- j + 1
    //       ENDIF
    //    ENDFOR
    //    FOR (i=j; i<HWVL; i++)
    //       Vc[i] <- UNDEFINED
    //    ENDFOR
    
  register int i;
  register int j = 0;
  register int vl = VL; 												        
 	
 for ( i = 0; i < vl; i++ ) {											
  
  // I need to get the mask bit for our iteration <I>
  if (GETVM(i))
  {
	VELEMINT(RC,j) = VELEMINT(RB,i); 												
	j++;
  }

  informative_ttl(10,"vcmpr[%d]: op-mask -> %ld\n",i,VELEMINT(RC,i)); 								

 } 		
 return picode->next; 												    
}


OP(ttl_vvmerg_f)
{
    // --> the expected behaviour is
    //    FOR (i=0; i<VL; i++)
    //       IF (VM<i> EQ 1) THEN
    //          Vc[i] <- Va[i]
    //       ELSE
    //          VVMERG: Vc[i] <- Vb[i]
    //       ENDIF
    //    ENDFOR
    //    FOR (i=VL; i<HWVL; i++)
    //       Vc[i] <- UNDEFINED
    //    ENDFOR
    
 int i,vl = VL; 												        

 for ( i = 0; i < vl; i++ ) {											
  register long fav = VELEMINT(RA,i);									
  register long fbv = VELEMINT(RB,i);									
  register long fcv;
  
  // I need to get the mask bit for our iteration <I>
  fcv = GETVM(i) ? fav : fbv;
  VELEMINT(RC,i) = fcv; 												

  informative_ttl(10,"vvmerg[%d]: (%ld) op-mask (%ld) = %ld = inreg=%ld\n",	
           i,fav,fbv,fcv,VELEMINT(RC,i)); 								
 } 		
 return picode->next; 												    
    
}

OP(ttl_vsmergq_f)
{
 int i,vl = VL; 												        

 for ( i = 0; i < vl; i++ ) {											
  register long fav = VELEMINT(RA,i);									
  register long fbv = REG(RB);									
  register long fcv;
  
  // I need to get the mask bit for our iteration <I>
  fcv = GETVM(i) ? fav : fbv;
  VELEMINT(RC,i) = fcv; 												

  informative_ttl(10,"vsmergq[%d]: (%ld) op-mask (%ld) = %ld = inreg=%ld\n",	
           i,fav,fbv,fcv,VELEMINT(RC,i)); 								
 } 		
 return picode->next; 												    
    
}

OP(ttl_vsmergt_f)
{
 int i,vl = VL; 												        

 for ( i = 0; i < vl; i++ ) {											
  register double fav = VELEMFP(RA,i);									
  register double fbv = FP(RB);									
  register double fcv;
  
  // I need to get the mask bit for our iteration <I>
  fcv = GETVM(i) ? fav : fbv;
  VELEMFP(RC,i) = fcv; 												

  informative_ttl(10,"vsmergt[%d]: (%lg) op-mask (%lg) = %lg = inreg=%lg\n",	
           i,fav,fbv,fcv,VELEMFP(RC,i)); 								
 } 		
 return picode->next; 												    
    
}


/******** Loads *********/

OP(ttl_vldt_f)
{
  int pr;

  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						

  pr = ifi->args[RA] + ifi->pos_in_vreg;

  asm("ldt" " %0,%1" : "=f" (VELEMFP(RA,ifi->pos_in_vreg)) : "o" (* (long *) &ifi->data));

  if ( ifi->pos_in_vreg == 0 ) DEBUG_VMEM("ttl_vldt_f");
  informative_ttl(10,"LD  v%d[%d] (phys v%d[%d] = v%d[%d] = Reg[%d]) : vaddr=%lx, paddr=%lx : data=%lg inreg=%lg\n",
	  picode->args[RA] - FIRST_VEC_LOGICAL, ifi->pos_in_vreg,
	  PHREG2PHVREG(ifi->args[RA]), ifi->pos_in_vreg,
	  ifi->args[RA], ifi->pos_in_vreg,
	  ifi->args[RA] + ifi->pos_in_vreg,
	  ifi->vaddr,
	  ifi->paddr,
	  ifi->data,
	  pthread->Reg[pr].Double
	 );


  return picode->next;
}

OP(ttl_vlds_f)
{
  int pr;

  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						
  
  pr = ifi->args[RA] + ifi->pos_in_vreg;

  asm("lds" " %0,%1" : "=f" (VELEMFP(RA,ifi->pos_in_vreg)) : "o" (* (long *) &ifi->data));

  if ( ifi->pos_in_vreg == 0 ) DEBUG_VMEM("ttl_vlds_f");
  informative_ttl(10,"LD  v%d[%d] (phys v%d[%d] = v%d[%d] = Reg[%d]) : vaddr=%lx, paddr=%lx : data=%lg inreg=%lg\n",
	  picode->args[RA] - FIRST_VEC_LOGICAL, ifi->pos_in_vreg,
	  PHREG2PHVREG(ifi->args[RA]), ifi->pos_in_vreg,
	  ifi->args[RA], ifi->pos_in_vreg,
	  ifi->args[RA] + ifi->pos_in_vreg,
	  ifi->vaddr,
	  ifi->paddr,
	  *(float*)&ifi->data,
	  pthread->Reg[pr].Double
	 );


  return picode->next;
}

OP(ttl_vldq_f)
{
  int pr;

  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						
  
  pr = ifi->args[RA] + ifi->pos_in_vreg;

  asm("ldq" " %0,%1" : "=&r" (VELEMINT(RA,ifi->pos_in_vreg)) : "o" (* (long *) &ifi->data));

  if ( ifi->pos_in_vreg == 0 ) DEBUG_VMEM("ttl_vldq_f");
  informative_ttl(10,"LD  v%d[%d] (phys v%d[%d] = v%d[%d] = Reg[%d]) : vaddr=%lx, paddr=%lx : data=%ld inreg=%ld\n",
	  picode->args[RA] - FIRST_VEC_LOGICAL, ifi->pos_in_vreg,
	  PHREG2PHVREG(ifi->args[RA]), ifi->pos_in_vreg,
	  ifi->args[RA], ifi->pos_in_vreg,
	  ifi->args[RA] + ifi->pos_in_vreg,
	  ifi->vaddr,
	  ifi->paddr,
	  ifi->data,
	  pthread->Reg[pr].Int64
	 );


  return picode->next;
}

OP(ttl_vldl_f)
{
  int pr;
  
  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						

  pr = ifi->args[RA] + ifi->pos_in_vreg;

  asm("ldl" " %0,%1" : "=&r" (VELEMINT(RA,ifi->pos_in_vreg)) : "o" (* (int *) &ifi->data));

  if ( ifi->pos_in_vreg == 0 ) DEBUG_VMEM("ttl_vldt_f");
  informative_ttl(10,"LD  v%d[%d] (phys v%d[%d] = v%d[%d] = Reg[%d]) : vaddr=%lx, paddr=%lx : data=%ld inreg=%ld\n",
	  picode->args[RA] - FIRST_VEC_LOGICAL, ifi->pos_in_vreg,
	  PHREG2PHVREG(ifi->args[RA]), ifi->pos_in_vreg,
	  ifi->args[RA], ifi->pos_in_vreg,
	  ifi->args[RA] + ifi->pos_in_vreg,
	  ifi->vaddr,
	  ifi->paddr,
	  ifi->data,
	  pthread->Reg[pr].Int64
	 );


  return picode->next;
}



/******* Stores *********/

OP(ttl_vstt_f)
{

  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						

  if (ifi->paddr) *(long *) ifi->paddr = ifi->data; 			

   if ( ifi->pos_in_vreg == 0 ) DEBUG_VMEM("ttl_vstt_f");
   informative_ttl(10,"ST  v%d[%d] (phys v%d[%d] = v%d[%d] = Reg[%d]) : vaddr=%lx, paddr=%lx : data=%lg inmem=%lg\n",
	  picode->args[RA] - FIRST_VEC_LOGICAL, ifi->pos_in_vreg,
	  PHREG2PHVREG(ifi->args[RA]), ifi->pos_in_vreg,
	  ifi->args[RA], ifi->pos_in_vreg,
	  ifi->args[RA] + ifi->pos_in_vreg,
	  ifi->vaddr,
	  ifi->paddr,
	  ifi->data,
	  *(double *)ifi->paddr
	 );

  return picode->next;
}


OP(ttl_vsts_f)
{

  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						

  if (ifi->paddr) *(int *) ifi->paddr = ifi->data; 			

   if ( ifi->pos_in_vreg == 0 ) DEBUG_VMEM("ttl_vsts_f");
   informative_ttl(10,"ST  v%d[%d] (phys v%d[%d] = v%d[%d] = Reg[%d]) : vaddr=%lx, paddr=%lx : data=%lg inmem=%lg\n",
	  picode->args[RA] - FIRST_VEC_LOGICAL, ifi->pos_in_vreg,
	  PHREG2PHVREG(ifi->args[RA]), ifi->pos_in_vreg,
	  ifi->args[RA], ifi->pos_in_vreg,
	  ifi->args[RA] + ifi->pos_in_vreg,
	  ifi->vaddr,
	  ifi->paddr,
	  *(float*) &ifi->data,
	  *(float *)ifi->paddr
	 );

  return picode->next;
}

OP(ttl_vstq_f)
{
  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						

  if (ifi->paddr) *(long *) ifi->paddr = ifi->data; 			

   if ( ifi->pos_in_vreg == 0 ) DEBUG_VMEM("ttl_vstq_f");
   informative_ttl(10,"ST  v%d[%d] (phys v%d[%d] = v%d[%d] = Reg[%d]) : vaddr=%lx, paddr=%lx : data=%ld inmem=%ld\n",
	  picode->args[RA] - FIRST_VEC_LOGICAL, ifi->pos_in_vreg,
	  PHREG2PHVREG(ifi->args[RA]), ifi->pos_in_vreg,
	  ifi->args[RA], ifi->pos_in_vreg,
	  ifi->args[RA] + ifi->pos_in_vreg,
	  ifi->vaddr,
	  ifi->paddr,
	  ifi->data,
	  *(long *)ifi->paddr
	 );

  return picode->next;
}

OP(ttl_vstl_f)
{

  int value;
  ulong address;

  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						

  value = (int) ifi->data;
  address = ifi->paddr;
	
  if (address) *(int*) address = value; 			

   if ( ifi->pos_in_vreg == 0 ) DEBUG_VMEM("ttl_vstl_f");
   informative_ttl(10,"ST  v%d[%d] (phys v%d[%d] = v%d[%d] = Reg[%d]) : vaddr=%lx, paddr=%lx : data=%ld inmem=%ld\n",
	  picode->args[RA] - FIRST_VEC_LOGICAL, ifi->pos_in_vreg,
	  PHREG2PHVREG(ifi->args[RA]), ifi->pos_in_vreg,
	  ifi->args[RA], ifi->pos_in_vreg,
	  ifi->args[RA] + ifi->pos_in_vreg,
	  ifi->vaddr,
	  ifi->paddr,
	  (int)ifi->data,
	  *(int *)address
	 );

  return picode->next;
}


/***** Gather and scatter ! *****/
OP(ttl_vgathq_f)
{
  int pr;

  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						

  pr = ifi->args[RC] + ifi->pos_in_vreg;

  asm("ldq" " %0,%1" : "=&r" (VELEMINT(RC,ifi->pos_in_vreg)) : "o" (* (long *) &ifi->data));

  if ( ifi->pos_in_vreg == 0 ) DEBUG_VIV("ttl_gathq_f");
  informative_ttl(10,"vgathq[%d]  v%d, $%d,v%d  (phys v%d[%d] = v%d[%d] = Reg[%d]) : vaddr=%lx, paddr=%lx : data=%ld inreg=%ld\n",
  	  ifi->pos_in_vreg,
	  picode->args[RA] - FIRST_VEC_LOGICAL,
	  picode->args[RB],
	  picode->args[RC] - FIRST_VEC_LOGICAL,
	  PHREG2PHVREG(ifi->args[RA]), ifi->pos_in_vreg,
	  ifi->args[RA], ifi->pos_in_vreg,
	  ifi->args[RA] + ifi->pos_in_vreg,
	  ifi->vaddr,
	  ifi->paddr,
	  ifi->data,
	  pthread->Reg[pr].Int64
	 );

  return picode->next;
}

OP(ttl_vgathl_f)
{
  int pr;

  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						

  
  pr = ifi->args[RC] + ifi->pos_in_vreg;

  asm("ldl" " %0,%1" : "=&r" (VELEMINT(RC,ifi->pos_in_vreg)) : "o" (* (int *) &ifi->data));

  if ( ifi->pos_in_vreg == 0 ) DEBUG_VIV("ttl_vgathl_f");

  informative_ttl(10,"vgathl[%d]  v%d, $%d,v%d  (phys v%d[%d] = v%d[%d] = Reg[%d]) : vaddr=%lx, paddr=%lx : data=%ld inreg=%ld\n",
  	  ifi->pos_in_vreg,
	  picode->args[RA] - FIRST_VEC_LOGICAL,
	  picode->args[RB],
	  picode->args[RC] - FIRST_VEC_LOGICAL,
	  PHREG2PHVREG(ifi->args[RA]), ifi->pos_in_vreg,
	  ifi->args[RA], ifi->pos_in_vreg,
	  ifi->args[RA] + ifi->pos_in_vreg,
	  ifi->vaddr,
	  ifi->paddr,
	  ifi->data,
	  pthread->Reg[pr].Int64
	 );

  return picode->next;
}

OP(ttl_vgatht_f)
{
  int pr;

  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						

  pr = ifi->args[RC] + ifi->pos_in_vreg;

  asm("ldt" " %0,%1" : "=f" (VELEMFP(RC,ifi->pos_in_vreg)) : "o" (* (long *) &ifi->data));

  if ( ifi->pos_in_vreg == 0 ) DEBUG_VIV("ttl_vgatht_f");

  informative_ttl(10,"vgatht[%d]  v%d, $%d,v%d  (phys v%d[%d] = v%d[%d] = Reg[%d]) : vaddr=%lx, paddr=%lx : data=%lg inreg=%lg\n",
  	  ifi->pos_in_vreg,
	  picode->args[RA] - FIRST_VEC_LOGICAL,
	  picode->args[RB],
	  picode->args[RC] - FIRST_VEC_LOGICAL,
	  PHREG2PHVREG(ifi->args[RA]), ifi->pos_in_vreg,
	  ifi->args[RA], ifi->pos_in_vreg,
	  ifi->args[RA] + ifi->pos_in_vreg,
	  ifi->vaddr,
	  ifi->paddr,
	  ifi->data,
	  pthread->Reg[pr].Double
	 );

  return picode->next;
}

OP(ttl_vgaths_f)
{
  int pr;
  
  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						

  pr = ifi->args[RC] + ifi->pos_in_vreg;

  asm("lds" " %0,%1" : "=f" (VELEMFP(RC,ifi->pos_in_vreg)) : "o" (* (long *) &ifi->data));

  if ( ifi->pos_in_vreg == 0 ) DEBUG_VIV("ttl_vgaths_f");

  informative_ttl(10,"vgaths[%d]  v%d, $%d,v%d  (phys v%d[%d] = v%d[%d] = Reg[%d]) : vaddr=%lx, paddr=%lx : data=%lg inreg=%lg\n",
  	  ifi->pos_in_vreg,
	  picode->args[RA] - FIRST_VEC_LOGICAL,
	  picode->args[RB],
	  picode->args[RC] - FIRST_VEC_LOGICAL,
	  PHREG2PHVREG(ifi->args[RA]), ifi->pos_in_vreg,
	  ifi->args[RA], ifi->pos_in_vreg,
	  ifi->args[RA] + ifi->pos_in_vreg,
	  ifi->vaddr,
	  ifi->paddr,
	  *(float*)&ifi->data,	  
	  pthread->Reg[pr].Double
	);
  return picode->next;
}


OP(ttl_vscatq_f)
{
  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						

   if (ifi->paddr) *(long *) ifi->paddr = ifi->data; 			
   if ( ifi->pos_in_vreg == 0 ) DEBUG_VIV("ttl_vscatq_f");
   
   informative_ttl(10,"vscatq[%d] v%d,$%d, v%d (phys v%d[%d] = v%d[%d] = Reg[%d]) : vaddr=%lx, paddr=%lx : data=%ld inmem=%ld\n",
      ifi->pos_in_vreg,
	  picode->args[RA] - FIRST_VEC_LOGICAL, 
	  picode->args[RB],
	  picode->args[RC] - FIRST_VEC_LOGICAL, 
	  PHREG2PHVREG(ifi->args[RA]), ifi->pos_in_vreg,
	  ifi->args[RA], ifi->pos_in_vreg,
	  ifi->args[RA] + ifi->pos_in_vreg,
	  ifi->vaddr,
	  ifi->paddr,
	  ifi->data,
	  *(long *)ifi->paddr
	 );

  return picode->next;
}

OP(ttl_vscatl_f)
{
  int value;
  ulong address;
	
  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						

  value = (int) ifi->data;
  address = ifi->paddr;

  if (address) *(int*) address = value; 			

   if ( ifi->pos_in_vreg == 0 ) DEBUG_VIV("ttl_vscatl_f");

   informative_ttl(10,"vscatl[%d] v%d,$%d, v%d (phys v%d[%d] = v%d[%d] = Reg[%d]) : vaddr=%lx, paddr=%lx : data=%ld inmem=%ld\n",
      ifi->pos_in_vreg,
	  picode->args[RA] - FIRST_VEC_LOGICAL, 
	  picode->args[RB],
	  picode->args[RC] - FIRST_VEC_LOGICAL, 
	  PHREG2PHVREG(ifi->args[RA]), ifi->pos_in_vreg,
	  ifi->args[RA], ifi->pos_in_vreg,
	  ifi->args[RA] + ifi->pos_in_vreg,
	  ifi->vaddr,
	  ifi->paddr,
	  (int)ifi->data,
	  *(int *)ifi->paddr
	 );

  return picode->next;
}

OP(ttl_vscatt_f)
{
  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						

  if (ifi->paddr) *(long *) ifi->paddr = ifi->data; 			

   if ( ifi->pos_in_vreg == 0 ) DEBUG_VIV("ttl_vscatt_f");

   informative_ttl(10,"vscatt[%d] v%d,$%d, v%d (phys v%d[%d] = v%d[%d] = Reg[%d]) : vaddr=%lx, paddr=%lx : data=%lg inmem=%lg\n",
      ifi->pos_in_vreg,
	  picode->args[RA] - FIRST_VEC_LOGICAL, 
	  picode->args[RB],
	  picode->args[RC] - FIRST_VEC_LOGICAL, 
	  PHREG2PHVREG(ifi->args[RA]), ifi->pos_in_vreg,
	  ifi->args[RA], ifi->pos_in_vreg,
	  ifi->args[RA] + ifi->pos_in_vreg,
	  ifi->vaddr,
	  ifi->paddr,
	  ifi->data,
	  *(double *)ifi->paddr
	 );

  return picode->next;
}

OP(ttl_vscats_f)
{
  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						

  if (ifi->paddr) *(int *) ifi->paddr = ifi->data; 			

   if ( ifi->pos_in_vreg == 0 ) DEBUG_VIV("ttl_vscats_f");

   informative_ttl(10,"vscats[%d] v%d,$%d, v%d (phys v%d[%d] = v%d[%d] = Reg[%d]) : vaddr=%lx, paddr=%lx : data=%lg inmem=%lg\n",
      ifi->pos_in_vreg,
	  picode->args[RA] - FIRST_VEC_LOGICAL, 
	  picode->args[RB],
	  picode->args[RC] - FIRST_VEC_LOGICAL, 
	  PHREG2PHVREG(ifi->args[RA]), ifi->pos_in_vreg,
	  ifi->args[RA], ifi->pos_in_vreg,
	  ifi->args[RA] + ifi->pos_in_vreg,
	  ifi->vaddr,
	  ifi->paddr,
	  *(float*) &ifi->data,
	  *(float *)ifi->paddr
	 );
   
  return picode->next;
}



/***************************************************/
/*********** Non conflict gaher/scatters ***********/
/***************************************************/


OP(ttl_vncgathq_f)
{
  int pr;
  unsigned expectedBank, realBank;

  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						

  pr = ifi->args[RC] + ifi->pos_in_vreg;

  /* check bank and pos_in_vreg coherence */
  realBank = (unsigned) ((ifi->vaddr >> 6) & 0xfL); 
  expectedBank = (unsigned) (ifi->pos_in_vreg % 16); 
  if (realBank!=expectedBank)
	fatal ("ttl_vncgathq_f: iteration %d, we expect bank=%d and vaddrs=%lx => bank %d\n",
		ifi->pos_in_vreg, expectedBank, ifi->vaddr, realBank);  	
		    
  asm("ldq" " %0,%1" : "=&r" (VELEMINT(RC,ifi->pos_in_vreg)) : "o" (* (long *) &ifi->data));

  if ( ifi->pos_in_vreg == 0 ) DEBUG_VIV("ttl_gathq_f");
  informative_ttl(10,"vncgathq[%d]  v%d, $%d,v%d  (phys v%d[%d] = v%d[%d] = Reg[%d]) : vaddr=%lx, paddr=%lx : data=%ld inreg=%ld\n",
  	  ifi->pos_in_vreg,
	  picode->args[RA] - FIRST_VEC_LOGICAL,
	  picode->args[RB],
	  picode->args[RC] - FIRST_VEC_LOGICAL,
	  PHREG2PHVREG(ifi->args[RA]), ifi->pos_in_vreg,
	  ifi->args[RA], ifi->pos_in_vreg,
	  ifi->args[RA] + ifi->pos_in_vreg,
	  ifi->vaddr,
	  ifi->paddr,
	  ifi->data,
	  pthread->Reg[pr].Int64
	 );

  return picode->next;
}

OP(ttl_vncgathl_f)
{
  int pr;
  unsigned expectedBank, realBank;
  
  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						

  /* check bank and pos_in_vreg coherence */
  realBank = (unsigned) ((ifi->vaddr >> 6) & 0xfL); 
  expectedBank = (unsigned) (ifi->pos_in_vreg % 16); 
  if (realBank!=expectedBank)
	fatal ("ttl_vncgathq_f: iteration %d, we expect bank=%d and vaddrs=%lx => bank %d\n",
		ifi->pos_in_vreg, expectedBank, ifi->vaddr, realBank);  	

  pr = ifi->args[RC] + ifi->pos_in_vreg;

  asm("ldl" " %0,%1" : "=&r" (VELEMINT(RC,ifi->pos_in_vreg)) : "o" (* (int *) &ifi->data));

  if ( ifi->pos_in_vreg == 0 ) DEBUG_VIV("ttl_vgathl_f");

  informative_ttl(10,"vncgathl[%d]  v%d, $%d,v%d  (phys v%d[%d] = v%d[%d] = Reg[%d]) : vaddr=%lx, paddr=%lx : data=%ld inreg=%ld\n",
  	  ifi->pos_in_vreg,
	  picode->args[RA] - FIRST_VEC_LOGICAL,
	  picode->args[RB],
	  picode->args[RC] - FIRST_VEC_LOGICAL,
	  PHREG2PHVREG(ifi->args[RA]), ifi->pos_in_vreg,
	  ifi->args[RA], ifi->pos_in_vreg,
	  ifi->args[RA] + ifi->pos_in_vreg,
	  ifi->vaddr,
	  ifi->paddr,
	  ifi->data,
	  pthread->Reg[pr].Int64
	 );

  return picode->next;
}

OP(ttl_vncgatht_f)
{
  int pr;
  unsigned expectedBank, realBank;

  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						

  /* check bank and pos_in_vreg coherence */
  realBank = (unsigned) ((ifi->vaddr >> 6) & 0xfL); 
  expectedBank = (unsigned) (ifi->pos_in_vreg % 16); 
  if (realBank!=expectedBank)
	fatal ("ttl_vncgathq_f: iteration %d, we expect bank=%d and vaddrs=%lx => bank %d\n",
		ifi->pos_in_vreg, expectedBank, ifi->vaddr, realBank);  	

  pr = ifi->args[RC] + ifi->pos_in_vreg;

  asm("ldt" " %0,%1" : "=f" (VELEMFP(RC,ifi->pos_in_vreg)) : "o" (* (long *) &ifi->data));

  if ( ifi->pos_in_vreg == 0 ) DEBUG_VIV("ttl_vgatht_f");

  informative_ttl(10,"vncgatht[%d]  v%d, $%d,v%d  (phys v%d[%d] = v%d[%d] = Reg[%d]) : vaddr=%lx, paddr=%lx : data=%lg inreg=%lg\n",
  	  ifi->pos_in_vreg,
	  picode->args[RA] - FIRST_VEC_LOGICAL,
	  picode->args[RB],
	  picode->args[RC] - FIRST_VEC_LOGICAL,
	  PHREG2PHVREG(ifi->args[RA]), ifi->pos_in_vreg,
	  ifi->args[RA], ifi->pos_in_vreg,
	  ifi->args[RA] + ifi->pos_in_vreg,
	  ifi->vaddr,
	  ifi->paddr,
	  ifi->data,
	  pthread->Reg[pr].Double
	 );

  return picode->next;
}

OP(ttl_vncgaths_f)
{
  int pr;
  unsigned expectedBank, realBank;
  
  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						

  /* check bank and pos_in_vreg coherence */
  realBank = (unsigned) ((ifi->vaddr >> 6) & 0xfL); 
  expectedBank = (unsigned) (ifi->pos_in_vreg % 16); 
  if (realBank!=expectedBank)
	fatal ("ttl_vncgathq_f: iteration %d, we expect bank=%d and vaddrs=%lx => bank %d\n",
		ifi->pos_in_vreg, expectedBank, ifi->vaddr, realBank);  	

  pr = ifi->args[RC] + ifi->pos_in_vreg;

  asm("lds" " %0,%1" : "=f" (VELEMFP(RC,ifi->pos_in_vreg)) : "o" (* (long *) &ifi->data));

  if ( ifi->pos_in_vreg == 0 ) DEBUG_VIV("ttl_vgaths_f");

  informative_ttl(10,"vncgaths[%d]  v%d, $%d,v%d  (phys v%d[%d] = v%d[%d] = Reg[%d]) : vaddr=%lx, paddr=%lx : data=%lg inreg=%lg\n",
  	  ifi->pos_in_vreg,
	  picode->args[RA] - FIRST_VEC_LOGICAL,
	  picode->args[RB],
	  picode->args[RC] - FIRST_VEC_LOGICAL,
	  PHREG2PHVREG(ifi->args[RA]), ifi->pos_in_vreg,
	  ifi->args[RA], ifi->pos_in_vreg,
	  ifi->args[RA] + ifi->pos_in_vreg,
	  ifi->vaddr,
	  ifi->paddr,
	  *(float*)&ifi->data,	  
	  pthread->Reg[pr].Double
	);
  return picode->next;
}


OP(ttl_vncscatq_f)
{
  unsigned expectedBank, realBank;

  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						

  /* check bank and pos_in_vreg coherence */
  realBank = (unsigned) ((ifi->vaddr >> 6) & 0xfL); 
  expectedBank = (unsigned) (ifi->pos_in_vreg % 16); 
  if (realBank!=expectedBank)
	fatal ("ttl_vncgathq_f: iteration %d, we expect bank=%d and vaddrs=%lx => bank %d\n",
		ifi->pos_in_vreg, expectedBank, ifi->vaddr, realBank);  	

   if (ifi->paddr) *(long *) ifi->paddr = ifi->data; 			
   if ( ifi->pos_in_vreg == 0 ) DEBUG_VIV("ttl_vscatq_f");
   
   informative_ttl(10,"vncscatq[%d] v%d,$%d, v%d (phys v%d[%d] = v%d[%d] = Reg[%d]) : vaddr=%lx, paddr=%lx : data=%ld inmem=%ld\n",
      ifi->pos_in_vreg,
	  picode->args[RA] - FIRST_VEC_LOGICAL, 
	  picode->args[RB],
	  picode->args[RC] - FIRST_VEC_LOGICAL, 
	  PHREG2PHVREG(ifi->args[RA]), ifi->pos_in_vreg,
	  ifi->args[RA], ifi->pos_in_vreg,
	  ifi->args[RA] + ifi->pos_in_vreg,
	  ifi->vaddr,
	  ifi->paddr,
	  ifi->data,
	  *(long *)ifi->paddr
	 );

  return picode->next;
}

OP(ttl_vncscatl_f)
{
  unsigned expectedBank, realBank;
  int value;
  ulong address;
	
  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						

  value = (int) ifi->data;
  address = ifi->paddr;

  /* check bank and pos_in_vreg coherence */
  realBank = (unsigned) ((ifi->vaddr >> 6) & 0xfL); 
  expectedBank = (unsigned) (ifi->pos_in_vreg % 16); 
  if (realBank!=expectedBank)
	fatal ("ttl_vncgathq_f: iteration %d, we expect bank=%d and vaddrs=%lx => bank %d\n",
		ifi->pos_in_vreg, expectedBank, ifi->vaddr, realBank);  	

  if (address) *(int*) address = value; 			

   if ( ifi->pos_in_vreg == 0 ) DEBUG_VIV("ttl_vscatl_f");

   informative_ttl(10,"vncscatl[%d] v%d,$%d, v%d (phys v%d[%d] = v%d[%d] = Reg[%d]) : vaddr=%lx, paddr=%lx : data=%ld inmem=%ld\n",
      ifi->pos_in_vreg,
	  picode->args[RA] - FIRST_VEC_LOGICAL, 
	  picode->args[RB],
	  picode->args[RC] - FIRST_VEC_LOGICAL, 
	  PHREG2PHVREG(ifi->args[RA]), ifi->pos_in_vreg,
	  ifi->args[RA], ifi->pos_in_vreg,
	  ifi->args[RA] + ifi->pos_in_vreg,
	  ifi->vaddr,
	  ifi->paddr,
	  (int)ifi->data,
	  *(int *)ifi->paddr
	 );

  return picode->next;
}

OP(ttl_vncscatt_f)
{
  unsigned expectedBank, realBank;

  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						

  /* check bank and pos_in_vreg coherence */
  realBank = (unsigned) ((ifi->vaddr >> 6) & 0xfL); 
  expectedBank = (unsigned) (ifi->pos_in_vreg % 16); 
  if (realBank!=expectedBank)
	fatal ("ttl_vncgathq_f: iteration %d, we expect bank=%d and vaddrs=%lx => bank %d\n",
		ifi->pos_in_vreg, expectedBank, ifi->vaddr, realBank);  	

  if (ifi->paddr) *(long *) ifi->paddr = ifi->data; 			

   if ( ifi->pos_in_vreg == 0 ) DEBUG_VIV("ttl_vscatt_f");

   informative_ttl(10,"vncscatt[%d] v%d,$%d, v%d (phys v%d[%d] = v%d[%d] = Reg[%d]) : vaddr=%lx, paddr=%lx : data=%lg inmem=%lg\n",
      ifi->pos_in_vreg,
	  picode->args[RA] - FIRST_VEC_LOGICAL, 
	  picode->args[RB],
	  picode->args[RC] - FIRST_VEC_LOGICAL, 
	  PHREG2PHVREG(ifi->args[RA]), ifi->pos_in_vreg,
	  ifi->args[RA], ifi->pos_in_vreg,
	  ifi->args[RA] + ifi->pos_in_vreg,
	  ifi->vaddr,
	  ifi->paddr,
	  ifi->data,
	  *(double *)ifi->paddr
	 );

  return picode->next;
}

OP(ttl_vncscats_f)
{
  unsigned expectedBank, realBank;

  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						

  /* check bank and pos_in_vreg coherence */
  realBank = (unsigned) ((ifi->vaddr >> 6) & 0xfL); 
  expectedBank = (unsigned) (ifi->pos_in_vreg % 16); 
  if (realBank!=expectedBank)
	fatal ("ttl_vncgathq_f: iteration %d, we expect bank=%d and vaddrs=%lx => bank %d\n",
		ifi->pos_in_vreg, expectedBank, ifi->vaddr, realBank);  	

  if (ifi->paddr) *(int *) ifi->paddr = ifi->data; 			

   if ( ifi->pos_in_vreg == 0 ) DEBUG_VIV("ttl_vscats_f");

   informative_ttl(10,"vncscats[%d] v%d,$%d, v%d (phys v%d[%d] = v%d[%d] = Reg[%d]) : vaddr=%lx, paddr=%lx : data=%lg inmem=%lg\n",
      ifi->pos_in_vreg,
	  picode->args[RA] - FIRST_VEC_LOGICAL, 
	  picode->args[RB],
	  picode->args[RC] - FIRST_VEC_LOGICAL, 
	  PHREG2PHVREG(ifi->args[RA]), ifi->pos_in_vreg,
	  ifi->args[RA], ifi->pos_in_vreg,
	  ifi->args[RA] + ifi->pos_in_vreg,
	  ifi->vaddr,
	  ifi->paddr,
	  *(float*) &ifi->data,
	  *(float *)ifi->paddr
	 );
   
  return picode->next;
}


/*** vsynch/drains does not modify the state of the machine, it's just a memory
     barrier => pm must model it but I need do nothing  
***/
OP(ttl_vsynch_f)
{
	informative_ttl(10,"vsynch \n");
	return picode->next;
}
OP(ttl_vdrainm0_f)
{
	informative_ttl(10,"vdrainm0 \n");
	return picode->next;
}
OP(ttl_vdrainm1_f)
{
	informative_ttl(10,"vdrainm1 \n");
	return picode->next;
}
OP(ttl_vdrainm2_f)
{
	informative_ttl(10,"vdrainm2 \n");
	return picode->next;
}
OP(ttl_vdrainv0_f)
{
	informative_ttl(10,"vdrainv0 \n");
	return picode->next;
}
OP(ttl_vdrainv1_f)
{
	informative_ttl(10,"vdrainv1 \n");
	return picode->next;
}
OP(ttl_vdrainv2_f)
{
	informative_ttl(10,"vdrainv2 \n");
	return picode->next;
}


/* prefetching instuctions (must do nothing at this point) */
OP(ttl_vldpf_f)
{
	informative_ttl(10,"vld.pf \n");
	return picode->next;
}
OP(ttl_vstpf_f)
{
	informative_ttl(10,"vst.pf \n");
	return picode->next;
}

OP(ttl_vgathpf_f)
{
	informative_ttl(10,"vgath.pf \n");
	return picode->next;
}

OP(ttl_vscatpf_f)
{
	informative_ttl(10,"vscat.pf \n");
	return picode->next;
}

/* just checking that we have the right addresses */
OP(ttl_vncgathpf_f)
{
  unsigned expectedBank, realBank;

  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						

 /* check bank and pos_in_vreg coherence */
  realBank = (unsigned) ((ifi->vaddr >> 6) & 0xfL); 
  expectedBank = (unsigned) (ifi->pos_in_vreg % 16); 
  if (realBank!=expectedBank)
	fatal ("ttl_vncgath.pf_f: iteration %d, we expect bank=%d and vaddrs=%lx => bank %d\n",
		ifi->pos_in_vreg, expectedBank, ifi->vaddr, realBank);  	
		    
  informative_ttl(10,"vncgath.pf \n");
  return picode->next;
}

OP(ttl_vncscatpf_f)
{
  unsigned expectedBank, realBank;

  if ((picode->is_masked)&&(!GETVM(ifi->pos_in_vreg))) return picode->next;						

  /* check bank and pos_in_vreg coherence */
  realBank = (unsigned) ((ifi->vaddr >> 6) & 0xfL); 
  expectedBank = (unsigned) (ifi->pos_in_vreg % 16); 
  if (realBank!=expectedBank)
	fatal ("ttl_vncgathq_f: iteration %d, we expect bank=%d and vaddrs=%lx => bank %d\n",
		ifi->pos_in_vreg, expectedBank, ifi->vaddr, realBank);  	

  informative_ttl(10,"vncscat.pf \n");
  return picode->next;
}


/***** Normal aritmetic instructions ******/

VECTOR_3OPf(ttl_vvaddt,addt,VELEMFP(RB,i),1)
VECTOR_3OPf(ttl_vvadds,adds,VELEMFP(RB,i),1)
VECTOR_3OPi(ttl_vvaddq,addq, VELEMINT(RB,i),1)         
VECTOR_3OPi(ttl_vvaddl,addl, VELEMINT(RB,i),1)         

VECTOR_3OPf(ttl_vvsubt,subt, VELEMFP(RB,i),1)
VECTOR_3OPf(ttl_vvsubs,subs, VELEMFP(RB,i),1)
VECTOR_3OPi(ttl_vvsubq,subq, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvsubl,subl, VELEMINT(RB,i),1)

VECTOR_3OPf(ttl_vvmult,mult, VELEMFP(RB,i),1)
VECTOR_3OPf(ttl_vvmuls,muls, VELEMFP(RB,i),1)
VECTOR_3OPi(ttl_vvmulq,mulq, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvmull,mull, VELEMINT(RB,i),1)

VECTOR_3OPf(ttl_vvdivt,divt, VELEMFP(RB,i),1)
VECTOR_3OPf(ttl_vvdivs,divs, VELEMFP(RB,i),1)

VECTOR_3OPf(ttl_vsaddt,addt, FP(RB),2)
VECTOR_3OPf(ttl_vsadds,adds, FP(RB),2)
VECTOR_3OPf(ttl_vssubt,subt, FP(RB),2)
VECTOR_3OPf(ttl_vssubs,subs, FP(RB),2)
VECTOR_3OPf(ttl_vsmult,mult, FP(RB),2)
VECTOR_3OPf(ttl_vsmuls,muls, FP(RB),2)
VECTOR_3OPf(ttl_vsdivt,divt, FP(RB),2)
VECTOR_3OPf(ttl_vsdivs,divs, FP(RB),2)

VECTOR_3OPi(ttl_vsaddq,addq, REG(RB),2)
VECTOR_3OPi(ttl_vsaddl,addl, REG(RB),2)
VECTOR_3OPi(ttl_vssubq,subq, REG(RB),2)
VECTOR_3OPi(ttl_vssubl,subl, REG(RB),2)
VECTOR_3OPi(ttl_vsmulq,mulq, REG(RB),2)
VECTOR_3OPi(ttl_vsmull,mull, REG(RB),2)

VECTOR_3OPi(ttl_vsand,and, REG(RB),2)
VECTOR_3OPi(ttl_vsbis,bis, REG(RB),2)
VECTOR_3OPi(ttl_vsxor,xor, REG(RB),2)
VECTOR_3OPi(ttl_vsbic,bic, REG(RB),2)
VECTOR_3OPi(ttl_vsornot,ornot, REG(RB),2)
VECTOR_3OPi(ttl_vseqv,eqv, REG(RB),2)

VECTOR_3OPi(ttl_vvand,and, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvbis,bis, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvxor,xor, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvbic,bic, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvornot,ornot, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vveqv,eqv, VELEMINT(RB,i),1)

VECTOR_3OPi(ttl_vvcmpeq,cmpeq, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvcmple,cmple, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvcmplt,cmplt, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vscmpeq,cmpeq, REG(RB),2)
VECTOR_3OPi(ttl_vscmple,cmple, REG(RB),2)
VECTOR_3OPi(ttl_vscmplt,cmplt, REG(RB),2)

VECTOR_3OPi(ttl_vvcmpule,cmpule, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvcmpult,cmpult, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vscmpule,cmpule, REG(RB),2)
VECTOR_3OPi(ttl_vscmpult,cmpult, REG(RB),2)
VECTOR_3OPi(ttl_vvcmpbge,cmpbge, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vscmpbge,cmpbge, REG(RB),2)

VECTOR_3OPf(ttl_vvcmpteq,cmpteq, VELEMFP(RB,i),1)
VECTOR_3OPf(ttl_vvcmptle,cmptle, VELEMFP(RB,i),1)
VECTOR_3OPf(ttl_vvcmptlt,cmptlt, VELEMFP(RB,i),1)
VECTOR_3OPf(ttl_vvcmptun,cmptun, VELEMFP(RB,i),1)
VECTOR_3OPf(ttl_vscmpteq,cmpteq, FP(RB),2)
VECTOR_3OPf(ttl_vscmptle,cmptle, FP(RB),2)
VECTOR_3OPf(ttl_vscmptlt,cmptlt, FP(RB),2)
VECTOR_3OPf(ttl_vscmptun,cmptun, FP(RB),2)

VECTOR_2OPf(ttl_vcvtqs,cvtqs)
VECTOR_2OPf(ttl_vcvtqt,cvtqt)
VECTOR_2OPf(ttl_vcvtst,cvtst)
VECTOR_2OPf(ttl_vcvttq,cvttq)
VECTOR_2OPf(ttl_vcvtts,cvtts)

VECTOR_3OPi(ttl_vsextbl,extbl, REG(RB),2)
VECTOR_3OPi(ttl_vsextwl,extwl, REG(RB),2)
VECTOR_3OPi(ttl_vsextll,extll, REG(RB),2)
VECTOR_3OPi(ttl_vsextql,extql, REG(RB),2)
VECTOR_3OPi(ttl_vsextwh,extwh, REG(RB),2)
VECTOR_3OPi(ttl_vsextlh,extlh, REG(RB),2)
VECTOR_3OPi(ttl_vsextqh,extqh, REG(RB),2)

VECTOR_3OPi(ttl_vvextbl,extbl, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvextwl,extwl, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvextll,extll, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvextql,extql, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvextwh,extwh, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvextlh,extlh, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvextqh,extqh, VELEMINT(RB,i),1)

VECTOR_3OPi(ttl_vsinsbl,insbl, REG(RB),2)
VECTOR_3OPi(ttl_vsinswl,inswl, REG(RB),2)
VECTOR_3OPi(ttl_vsinsll,insll, REG(RB),2)
VECTOR_3OPi(ttl_vsinsql,insql, REG(RB),2)
VECTOR_3OPi(ttl_vsinswh,inswh, REG(RB),2)
VECTOR_3OPi(ttl_vsinslh,inslh, REG(RB),2)
VECTOR_3OPi(ttl_vsinsqh,insqh, REG(RB),2)

VECTOR_3OPi(ttl_vvinsbl,insbl, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvinswl,inswl, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvinsll,insll, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvinsql,insql, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvinswh,inswh, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvinslh,inslh, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvinsqh,insqh, VELEMINT(RB,i),1)

VECTOR_3OPi(ttl_vvmskbl,mskbl, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvmskwl,mskwl, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvmskll,mskll, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvmskql,mskql, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvmskwh,mskwh, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvmsklh,msklh, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vvmskqh,mskqh, VELEMINT(RB,i),1)

VECTOR_3OPi(ttl_vsmskbl,mskbl, REG(RB),2)
VECTOR_3OPi(ttl_vsmskwl,mskwl, REG(RB),2)
VECTOR_3OPi(ttl_vsmskll,mskll, REG(RB),2)
VECTOR_3OPi(ttl_vsmskql,mskql, REG(RB),2)
VECTOR_3OPi(ttl_vsmskwh,mskwh, REG(RB),2)
VECTOR_3OPi(ttl_vsmsklh,msklh, REG(RB),2)
VECTOR_3OPi(ttl_vsmskqh,mskqh, REG(RB),2)

VECTOR_3OPi(ttl_vssra,sra, REG(RB),2)
VECTOR_3OPi(ttl_vvsra,sra, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vssll,sll, REG(RB),2)
VECTOR_3OPi(ttl_vvsll,sll, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vssrl,srl, REG(RB),2)
VECTOR_3OPi(ttl_vvsrl,srl, VELEMINT(RB,i),1)

VECTOR_3OPi(ttl_vs4addq,s4addq, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vs8addq,s8addq, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vs4addl,s4addl, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vs8addl,s8addl, VELEMINT(RB,i),1)

VECTOR_3OPi(ttl_vs4subq,s4subq, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vs8subq,s8subq, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vs4subl,s4subl, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vs8subl,s8subl, VELEMINT(RB,i),1)

VECTOR_3OPi(ttl_vvumulh,umulh, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vsumulh,umulh, REG(RB),2)

VECTOR_3OPi(ttl_vvzap,zap, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vszap,zap, REG(RB),2)

VECTOR_3OPi(ttl_vvzapnot,zapnot, VELEMINT(RB,i),1)
VECTOR_3OPi(ttl_vszapnot,zapnot, REG(RB),2)


#endif /* NEW_TTL */




/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/





#ifdef NEW_TLDS

/* 
 * Assembly language routines for implementing instructions 
 */

#include <stdio.h>

#include "globals.h"
#include "opcodes.h"
#include "protos.h"

/*
 * implementations of TLDS instructions
 */

OP(tlds_bs_f)
{
  /* do nothing: this is a noop */
  REG(RA) = IR2V(picode->next);
  ZERO_ZERO_REG;
  return picode->next;
}

OP(tlds_bns_f)
{
  /* do nothing: this is a noop */
  REG(RA) = IR2V(picode->next);
  ZERO_ZERO_REG;
  return picode->next;
}

OP(tlds_q_f)
{
  /* do nothing: this is a noop */
  REG(RA) = IR2V(picode->next);
  ZERO_ZERO_REG;
  return picode->next;
}

OP(tlds_arm_f)
{
  /* do nothing: this is a noop */
  REG(RA) = IR2V(picode->next);
  ZERO_ZERO_REG;
  return picode->next;
}

OP(tlds_en_f)
{
  /* do nothing: this is a noop */
  REG(RA) = IR2V(picode->next);
  ZERO_ZERO_REG;
  return picode->next;
}

#endif /* NEW_TLDS */
