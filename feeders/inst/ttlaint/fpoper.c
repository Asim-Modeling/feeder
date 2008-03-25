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

/* Assembly language routines for implementing Alpha floating point operations
 */
#include <stdio.h>

#include "icode.h"
#include "globals.h"
#include "opcodes.h"
#include <math.h>

/* In the following, the result value (fcv) is
 * written to FP(RC) only if it is equal to fbv
 * This is necessary because the conditional
 * leaves fcv untouched (copies an undetermined value)
 * if the condition is false.
 */

#ifdef FPOPER_PARANOID
  if (pthread->fpcr & 0x8000000000000000l) \
    { if (0) warning("pc=%lx opc=%s fpcr=%lx\n", ifi->picode->addr, desc_table[ifi->picode->opnum].opname, pthread->fpcr); \
      pthread->fpcr &= 0x7C08000000000000l; \
    }
#else
#define CHECK_FPCR(ifi, pthread)
#endif


#define FP_CMOV(NAME) 											\
OP(NAME ## _f) 												\
{ 													\
  register double fav = FP(RA);										\
  register double fbv = FP(RB); 									\
  register double fcv = FP(RC); 									\
  register double fpcr = *(double*)&pthread->fpcr; 							\
													\
  if (0) informative(#NAME ": %lx %x -- fpcr=%lx\n", picode->addr, picode_instr(pthread, picode), pthread->fpcr);\
													\
													\
  asm("mt_fpcr %4; trapb;" #NAME " %2, %3, %0; trapb; mf_fpcr %1 " 					\
  : "=f" (fcv) , "=f" (fpcr) 										\
  : "f" (fav) , "f" (fbv), "f" (fpcr), "0" (fcv)); 							\
													\
													\
  *(double*)&pthread -> fpcr = fpcr; 									\
  CHECK_FPCR(ifi, pthread); 										\
													\
  FP(RC) = fcv; 											\
  MapFP(ZERO_REGISTER) = 0.0; 										\
  return picode->next; 											\
}

/* "&" constraint in "=&f" says: do NOT allocate output to same register
 * as an input; thist is necessary here since FP instructions with /s flag
 * (software completion) are not allowed to clobber their input registers
 * Alpha Architecture Reference Manual, 3rd Edition, 4.7.7.3 */
#define FP_OPER(NAME) 											\
OP(NAME ## _f) 												\
{ 													\
  register double fav = FP(RA);										\
  register double fbv = FP(RB); 									\
  register double fcv; 											\
  register double fpcr = *(double*)&pthread->fpcr; 							\
													\
  if (0) informative(#NAME ": %lx %x -- fpcr=%lx\n", picode->addr, picode_instr(pthread, picode), pthread->fpcr);\
													\
													\
  asm("mt_fpcr %4; trapb;" #NAME " %2, %3, %0; trapb; mf_fpcr %1 " 					\
  : "=&f" (fcv) , "=f" (fpcr) 										\
  : "f" (fav) , "f" (fbv), "f" (fpcr));									\
													\
													\
  FP(RC) = fcv; 											\
  *(double*)&pthread -> fpcr = fpcr; 									\
													\
  CHECK_FPCR(ifi, pthread); 										\
  MapFP(ZERO_REGISTER) = 0.0; 										\
  return picode->next; 											\
}

#define FP_SOPER(NAME) 											\
OP(NAME ## _f) 												\
{ 													\
  register float fav = FP(RA);										\
  register float fbv = FP(RB);										\
  register float fcv;											\
  register double fpcr = *(double*)&pthread->fpcr; 							\
													\
  if (0) informative(#NAME ": %lx %x -- fpcr=%lx\n", picode->addr, picode_instr(pthread, picode), pthread->fpcr);\
													\
													\
  asm("mt_fpcr %4; trapb;" #NAME " %2, %3, %0; trapb; mf_fpcr %1 " 					\
  : "=&f" (fcv) , "=f" (fpcr) 										\
  : "f" (fav) , "f" (fbv), "f" (fpcr)); 								\
													\
													\
  FP(RC) = fcv; 											\
													\
  *(double*)&pthread -> fpcr = fpcr; 									\
  CHECK_FPCR(ifi, pthread); 										\
  MapFP(ZERO_REGISTER) = 0.0; 										\
  return picode->next; 											\
}

extern int in_FEED_Skip;

#define FP_CVT(NAME) 											\
OP(NAME ## _f) 												\
{ 													\
  register double fbv = FP(RB); 									\
  register double fpcr = *(double*)&pthread->fpcr; 							\
  if (0) informative(#NAME ": %lx %x -- fpcr=%lx\n", picode->addr, picode_instr(pthread, picode), pthread->fpcr);\
													\
													\
  asm("mt_fpcr %3; trapb;" #NAME " %2, %0; trapb; mf_fpcr %1 %1 %1" 					\
      : "=&f" (FP(RC)) , "=f" (pthread->fpcr) : "f" (fbv), "f" (fpcr)); 					\
													\
													\
  MapFP(ZERO_REGISTER) = 0.0; 										\
  *(double*)&pthread->fpcr = fpcr; 									\
  CHECK_FPCR(ifi, pthread); 										\
  return picode->next; 											\
}


FP_OPER(addfc)
FP_OPER(subfc)
FP_OPER(mulfc)
FP_OPER(divfc)
FP_OPER(addgc)
FP_OPER(subgc)
FP_OPER(mulgc)
FP_OPER(divgc)
FP_OPER(addf)
FP_OPER(subf)
FP_OPER(mulf)
FP_OPER(divf)
FP_OPER(addg)
FP_OPER(subg)
FP_OPER(mulg)
FP_OPER(divg)
FP_OPER(cmpgeq)
FP_OPER(cmpglt)
FP_OPER(cmpgle)
FP_OPER(addfuc)
FP_OPER(subfuc)
FP_OPER(mulfuc)
FP_OPER(divfuc)
FP_OPER(addguc)
FP_OPER(subguc)
FP_OPER(mulguc)
FP_OPER(divguc)
FP_OPER(addfu)
FP_OPER(subfu)
FP_OPER(mulfu)
FP_OPER(divfu)
FP_OPER(addgu)
FP_OPER(subgu)
FP_OPER(mulgu)
FP_OPER(divgu)
FP_OPER(addfsc)
FP_OPER(subfsc)
FP_OPER(mulfsc)
FP_OPER(divfsc)
FP_OPER(addgsc)
FP_OPER(subgsc)
FP_OPER(mulgsc)
FP_OPER(divgsc)
FP_OPER(addfs)
FP_OPER(subfs)
FP_OPER(mulfs)
FP_OPER(divfs)
FP_OPER(addgs)
FP_OPER(subgs)
FP_OPER(mulgs)
FP_OPER(divgs)
FP_OPER(cmpgeqs)
FP_OPER(cmpglts)
FP_OPER(cmpgles)
FP_OPER(addfsuc)
FP_OPER(subfsuc)
FP_OPER(mulfsuc)
FP_OPER(divfsuc)
FP_OPER(addgsuc)
FP_OPER(subgsuc)
FP_OPER(mulgsuc)
FP_OPER(divgsuc)
FP_OPER(addfsu)
FP_OPER(subfsu)
FP_OPER(mulfsu)
FP_OPER(divfsu)
FP_OPER(addgsu)
FP_OPER(subgsu)
FP_OPER(mulgsu)
FP_OPER(divgsu)
FP_OPER(addsc)
FP_OPER(subsc)
FP_OPER(mulsc)
FP_OPER(divsc)
FP_OPER(addtc)
FP_OPER(subtc)
FP_OPER(multc)
FP_OPER(divtc)
FP_OPER(addsm)
FP_OPER(subsm)
FP_OPER(mulsm)
FP_OPER(divsm)
FP_OPER(addtm)
FP_OPER(subtm)
FP_OPER(multm)
FP_OPER(divtm)
FP_SOPER(adds)
FP_SOPER(subs)
FP_SOPER(muls)
FP_SOPER(divs)
FP_OPER(addt)
FP_OPER(subt)
FP_OPER(mult)
FP_OPER(divt)
FP_OPER(cmptun)
FP_OPER(cmpteq)
FP_OPER(cmptlt)
FP_OPER(cmptle)
FP_OPER(addsd)
FP_OPER(subsd)
FP_OPER(mulsd)
FP_OPER(divsd)
FP_OPER(addtd)
FP_OPER(subtd)
FP_OPER(multd)
FP_OPER(divtd)
FP_OPER(addsuc)
FP_OPER(subsuc)
FP_OPER(mulsuc)
FP_OPER(divsuc)
FP_OPER(addtuc)
FP_OPER(subtuc)
FP_OPER(multuc)
FP_OPER(divtuc)
FP_OPER(addsum)
FP_OPER(subsum)
FP_OPER(mulsum)
FP_OPER(divsum)
FP_OPER(addtum)
FP_OPER(subtum)
FP_OPER(multum)
FP_OPER(divtum)
FP_OPER(addsu)
FP_OPER(subsu)
FP_OPER(mulsu)
FP_OPER(divsu)
FP_OPER(addtu)
FP_OPER(subtu)
FP_OPER(multu)
FP_OPER(divtu)
FP_OPER(addsud)
FP_OPER(subsud)
FP_OPER(mulsud)
FP_OPER(divsud)
FP_OPER(addtud)
FP_OPER(subtud)
FP_OPER(multud)
FP_OPER(divtud)
FP_OPER(addssuc)
FP_OPER(subssuc)
FP_OPER(mulssuc)
FP_OPER(divssuc)
FP_OPER(addtsuc)
FP_OPER(subtsuc)
FP_OPER(multsuc)
FP_OPER(divtsuc)
FP_OPER(addssum)
FP_OPER(subssum)
FP_OPER(mulssum)
FP_OPER(divssum)
FP_OPER(addtsum)
FP_OPER(subtsum)
FP_OPER(multsum)
FP_OPER(divtsum)
FP_OPER(addssu)
FP_OPER(subssu)
FP_OPER(mulssu)
FP_OPER(divssu)
FP_OPER(addtsu)
FP_OPER(subtsu)
FP_OPER(multsu)
FP_OPER(divtsu)
FP_OPER(cmptunsu)
FP_OPER(cmpteqsu)
FP_OPER(cmptltsu)
FP_OPER(cmptlesu)
FP_OPER(addssud)
FP_OPER(subssud)
FP_OPER(mulssud)
FP_OPER(divssud)
FP_OPER(addtsud)
FP_OPER(subtsud)
FP_OPER(multsud)
FP_OPER(divtsud)
FP_OPER(addssuic)
FP_OPER(subssuic)
FP_OPER(mulssuic)
FP_OPER(divssuic)
FP_OPER(addtsuic)
FP_OPER(subtsuic)
FP_OPER(multsuic)
FP_OPER(divtsuic)
FP_OPER(addssuim)
FP_OPER(subssuim)
FP_OPER(mulssuim)
FP_OPER(divssuim)
FP_OPER(addtsuim)
FP_OPER(subtsuim)
FP_OPER(multsuim)
FP_OPER(divtsuim)
FP_OPER(addssui)
FP_OPER(subssui)
FP_OPER(mulssui)
FP_OPER(divssui)
FP_OPER(addtsui)
FP_OPER(subtsui)
FP_OPER(multsui)
FP_OPER(divtsui)
FP_OPER(addssuid)
FP_OPER(subssuid)
FP_OPER(mulssuid)
FP_OPER(divssuid)
FP_OPER(addtsuid)
FP_OPER(subtsuid)
FP_OPER(multsuid)
FP_OPER(divtsuid)
FP_OPER(cpys)
FP_OPER(cpysn)
FP_OPER(cpyse)

OP(mt_fpcr_f)
{
  if (0) informative("mt_fpcr: %lx %x -- fpcr=%lx\n", picode->addr, picode_instr(pthread, picode), pthread->fpcr);
  *(double*)&pthread->fpcr = FP(RA);
  CHECK_FPCR(ifi, pthread); \
  return(picode->next);
}

OP(mf_fpcr_f)
{
  if (0) informative("mf_fpcr: %lx %x -- fpcr=%lx\n", picode->addr, picode_instr(pthread, picode), pthread->fpcr);
  FP(RA) = *(double*)&pthread->fpcr;
  MapFP(ZERO_REGISTER) = 0.0;
  return(picode->next);
}

FP_CMOV(fcmoveq)
FP_CMOV(fcmovne)
FP_CMOV(fcmovlt)
FP_CMOV(fcmovge)
FP_CMOV(fcmovle)
FP_CMOV(fcmovgt)

FP_CVT(cvtdgc)
FP_CVT(cvtgfc)
FP_CVT(cvtgdc)
FP_CVT(cvtgqc)
FP_CVT(cvtqfc)
FP_CVT(cvtqgc)
FP_CVT(cvtdg)
FP_CVT(cvtgf)
FP_CVT(cvtgd)
FP_CVT(cvtgq)
FP_CVT(cvtqf)
FP_CVT(cvtqg)
FP_CVT(cvtdguc)
FP_CVT(cvtgfuc)
FP_CVT(cvtgduc)
FP_CVT(cvtgqvc)
FP_CVT(cvtdgu)
FP_CVT(cvtgfu)
FP_CVT(cvtgdu)
FP_CVT(cvtgqv)
FP_CVT(cvtdgsc)
FP_CVT(cvtgfsc)
FP_CVT(cvtgdsc)
FP_CVT(cvtgqsc)
FP_CVT(cvtdgs)
FP_CVT(cvtgfs)
FP_CVT(cvtgds)
FP_CVT(cvtgqs)
FP_CVT(cvtdgsuc)
FP_CVT(cvtgfsuc)
FP_CVT(cvtgdsuc)
FP_CVT(cvtgqsvc)
FP_CVT(cvtdgsu)
FP_CVT(cvtgfsu)
FP_CVT(cvtgdsu)
FP_CVT(cvtgqsv)
FP_CVT(cvttsc)
FP_CVT(cvttqc)
FP_CVT(cvtqsc)
FP_CVT(cvtqtc)
FP_CVT(cvttsm)
FP_CVT(cvttqm)
FP_CVT(cvtqsm)
FP_CVT(cvtqtm)
FP_CVT(cvtts)
FP_CVT(cvttq)
FP_CVT(cvtqs)
FP_CVT(cvtqt)
FP_CVT(cvttsd)
FP_CVT(cvttqd)
FP_CVT(cvtqsd)
FP_CVT(cvtqtd)
FP_CVT(cvttsuc)
FP_CVT(cvttqvc)
FP_CVT(cvttsum)
FP_CVT(cvttqvm)
FP_CVT(cvttsu)
FP_CVT(cvttqv)
FP_CVT(cvttsud)
FP_CVT(cvttqvd)
FP_CVT(cvtst)
FP_CVT(cvtsts)
FP_CVT(cvttssuc)
FP_CVT(cvttqsvc)
FP_CVT(cvttssum)
FP_CVT(cvttqsvm)
FP_CVT(cvttssu)
FP_CVT(cvttqsv)
FP_CVT(cvttssud)
FP_CVT(cvttqsvd)
/* FP_CVT(cvttss) - gcc claims no such operation */
FP_CVT(cvttssuic)
FP_CVT(cvttqsvic)
FP_CVT(cvtqssuic)
FP_CVT(cvtqtsuic)
FP_CVT(cvttssuim)
FP_CVT(cvttqsvim)
FP_CVT(cvtqssuim)
FP_CVT(cvtqtsuim)
FP_CVT(cvttssui)
FP_CVT(cvttqsvi)
FP_CVT(cvtqssui)
FP_CVT(cvtqtsui)
FP_CVT(cvttssuid)
FP_CVT(cvttqsvid)
FP_CVT(cvtqssuid)
FP_CVT(cvtqtsuid)
FP_CVT(cvtlq)
FP_CVT(cvtql)
FP_CVT(cvtqlv)
FP_CVT(cvtqlsv)

OP(sqrtt_f)
{ 
  register double fav = FP(RA);
  register double fbv = FP(RB); 
  register double fcv = FP(RC); 
  fcv = sqrt(fbv); 
  FP(RC) = fcv; 
  if (fbv < 0)
    pthread -> fpcr |= ((1l << 52) & (1l << 63));
  CHECK_FPCR(ifi, pthread);
  MapFP(ZERO_REGISTER) = 0.0; 
  return picode->next; 
}

OP(sqrts_f) 
{ 
  register double fav = FP(RA);
  register double fbv = FP(RB); 
  register double fcv = FP(RC); 
  fcv = (double)sqrtf((float)fbv); 
  FP(RC) = fcv; 
  if (fbv < 0)
    pthread -> fpcr |= ((1l << 52) & (1l << 63));
  CHECK_FPCR(ifi, pthread);
  MapFP(ZERO_REGISTER) = 0.0; 
  return picode->next; 
}
