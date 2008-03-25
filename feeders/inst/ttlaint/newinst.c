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

#ifdef NEW_MVI
#include <math.h>
#define SPFP_HI(RegArg)     (pthread->FP[ifi->args[RegArg]].Sp32[1])
#define SPFP_LO(RegArg)     (pthread->FP[ifi->args[RegArg]].Sp32[0])
#define SPHEX_HI(RegArg)    (pthread->FP[ifi->args[RegArg]].Hex32[1])
#define SPHEX_LO(RegArg)    (pthread->FP[ifi->args[RegArg]].Hex32[0])
#define SPSIGN_HI(RegArg)   ((pthread->FP[ifi->args[RegArg]].Hex32[1] & 0x80000000) >> 31)
#define SPSIGN_LO(RegArg)   ((pthread->FP[ifi->args[RegArg]].Hex32[0] & 0x80000000) >> 31)
#define SP_NaN(sp) (((sp & 0x7F800000) == 0x7F800000) && ((sp &0x7FFFFF) != 0))   

#ifdef  FPOPER_PARANOID
#define CHECK_FPCR_MVI(ifi, pthread) if (pthread->fpcr & 0x8000000000000000l) \
    { if (0) warning("pc=%lx opc=%s fpcr=%lx\n", ifi->picode->addr, desc_table[ifi->picode->opnum].opname, pthread->fpcr); \
      pthread->fpcr &= 0x7C08000000000000l; \
    }
#else
#define CHECK_FPCR_MVI(ifi, pthread) 
#endif

#endif

OP(unimplemented_op);

typedef unsigned char upixel8;
typedef unsigned short upixel16;
typedef signed char spixel8;
typedef signed short spixel16;

#ifdef NEW_MVI
typedef unsigned int upixel32;
typedef unsigned long upixel64;
typedef signed int spixel32;
typedef signed long spixel64;
#endif

OP(sextb_f)
{
  spixel8 rbv = *(spixel8 *)&REG(RB);
  REG(RC) = (long)rbv;
  
  return picode->next;
}

OP(sextw_f)
{
  spixel16 rbv = *(spixel16 *)&REG(RB);
  REG(RC) = (long)rbv;

  return picode->next;
}

OP(perr_f)
{
  upixel8  *rav = (upixel8 *)&REG(RA);
  upixel8  *rbv = (upixel8 *)&REG(RB);
  upixel64 *rcv = (upixel64 *)&REG(RC);
  upixel64 temp = 0;

  int i;
  for (i = 0; i < 8; i++) {
    if (rav[i] >= rbv[i]) {
      temp += rav[i] - rbv[i];
    } else {
      temp += rbv[i] - rav[i];
    }
  }
  *rcv = temp;
  return picode->next;
}

OP(minub8_f)
{
  upixel8 *rav = (upixel8 *)&REG(RA);
  upixel8 *rbv = (upixel8 *)&REG(RB);
  upixel8 *rcv = (upixel8 *)&REG(RC);
  int i;
  for (i = 0; i < 8; i++) {
    if (rav[i] >= rbv[i]) {
     rcv[i] = rbv[i];
    } else {
     rcv[i] = rav[i];
    }
  }
  return picode->next;
}

OP(minsb8_f)
{
  spixel8 *rav = (spixel8 *)&REG(RA);
  spixel8 *rbv = (spixel8 *)&REG(RB);
  spixel8 *rcv = (spixel8 *)&REG(RC);
  int i;
  for (i = 0; i < 8; i++) {
    if (rav[i] >= rbv[i]) {
      rcv[i] = rbv[i];
    } else {
      rcv[i] = rav[i];
    }
  }
  return picode->next;
}

OP(minuw4_f)
{
  upixel16 *rav = (upixel16 *)&REG(RA);
  upixel16 *rbv = (upixel16 *)&REG(RB);
  upixel16 *rcv = (upixel16 *)&REG(RC);
  int i;
  for (i = 0; i < 4; i++) {
    if (rav[i] >= rbv[i]) {
      rcv[i] = rbv[i];
    } else {
      rcv[i] = rav[i];
    }
  }
  return picode->next;
}

OP(minsw4_f)
{
  spixel16 *rav = (spixel16 *)&REG(RA);
  spixel16 *rbv = (spixel16 *)&REG(RB);
  spixel16 *rcv = (spixel16 *)&REG(RC);
  int i;
  for (i = 0; i < 4; i++) {
    if (rav[i] >= rbv[i]) {
      rcv[i] = rbv[i];
    } else {
      rcv[i] = rav[i];
    }
  }
  return picode->next;
}

OP(maxub8_f)
{
  upixel8 *rav = (upixel8 *)&REG(RA);
  upixel8 *rbv = (upixel8 *)&REG(RB);
  upixel8 *rcv = (upixel8 *)&REG(RC);
  int i;
  for (i = 0; i < 8; i++) {
    if (rav[i] <= rbv[i]) {
      rcv[i] = rbv[i];
    } else {
      rcv[i] = rav[i];
    }
  }
  return picode->next;
}

OP(maxsb8_f)
{
  spixel8 *rav = (spixel8 *)&REG(RA);
  spixel8 *rbv = (spixel8 *)&REG(RB);
  spixel8 *rcv = (spixel8 *)&REG(RC);
  int i;
  for (i = 0; i < 8; i++) {
    if (rav[i] <= rbv[i]) {
      rcv[i] = rbv[i];
    } else {
      rcv[i] = rav[i];
    }
  }
  return picode->next;
}

OP(maxuw4_f)
{
  upixel16 *rav = (upixel16 *)&REG(RA);
  upixel16 *rbv = (upixel16 *)&REG(RB);
  upixel16 *rcv = (upixel16 *)&REG(RC);
  int i;
  for (i = 0; i < 4; i++) {
    if (rav[i] <= rbv[i]) {
      rcv[i] = rbv[i];
    } else {
      rcv[i] = rav[i];
    }
  }
  return picode->next;
}

OP(maxsw4_f)
{
  spixel16 *rav = (spixel16 *)&REG(RA);
  spixel16 *rbv = (spixel16 *)&REG(RB);
  spixel16 *rcv = (spixel16 *)&REG(RC);
  int i;
  for (i = 0; i < 4; i++) {
    if (rav[i] <= rbv[i]) {
      rcv[i] = rbv[i];
    } else {
      rcv[i] = rav[i];
    }
  }
  return picode->next;
}
#ifdef NEW_MVI
/*
  Arana MVI routines are added here
  */
OP(pklb_f)
{
  upixel8 *rav = (upixel8*)&REG(RA);  
  upixel8 *rbv = (upixel8*)&REG(RB);
  upixel8 *rcv = (upixel8*)&REG(RC);

  REG(RC) = 0;
  rcv[0] = rbv[0];
  rcv[1] = rbv[4];
  rcv[2] = rav[0];
  rcv[3] = rav[4];

  return picode->next;
}

OP(pkwb_f)
{
  upixel8 *rav = (upixel8*)&REG(RA);  
  upixel8 *rbv = (upixel8*)&REG(RB);
  upixel8 *rcv = (upixel8*)&REG(RC);

  REG(RC) = 0;
  rcv[0] = rbv[0];
  rcv[1] = rbv[2];
  rcv[2] = rbv[4];
  rcv[3] = rbv[6];
  rcv[4] = rav[0];
  rcv[5] = rav[2];
  rcv[6] = rav[4];
  rcv[7] = rav[6];

  return picode->next;
}

OP(unpkbl_f)
{
  upixel8 *rav = (upixel8 *)&REG(RA);
  upixel8 *rbv = (upixel8 *)&REG(RB);
  upixel8 *rcv = (upixel8 *)&REG(RC);

  REG(RC) = 0;
  rcv[0] = rbv[0];
  rcv[1] = rav[0];
  rcv[4] = rbv[1];
  rcv[5] = rav[1];
  return picode->next;
}

OP(unpkbw_f)
{
  upixel8 *rav = (upixel8 *)&REG(RA);
  upixel8 *rbv = (upixel8 *)&REG(RB);
  upixel8 *rcv = (upixel8 *)&REG(RC);

  REG(RC) = 0;
  rcv[0] = rbv[0];
  rcv[1] = rav[0];
  rcv[2] = rbv[1];
  rcv[3] = rav[1];
  rcv[4] = rbv[2];
  rcv[5] = rav[2];
  rcv[6] = rbv[3];
  rcv[7] = rav[3];

  return picode->next;
}

OP(paddsb8_f){
  spixel8 *rav = (spixel8 *)&REG(RA);
  spixel8 *rbv;
  spixel8 *rcv = (spixel8 *)&REG(RC);
  spixel8 rb[8],temp;
  int i;
  spixel16 cnt;

  if (!(picode->iflags & E_LITERAL))
      rbv = (spixel8 *)&REG(RB);  
  else {
      temp = (spixel8) (picode->literal & 0xFF);
      for( i = 0; i < 8; i++)
	  rb[i] = temp; 
      rbv = rb;
  }  
  
  for (i = 0; i < 8; i++){

      cnt  = (spixel16)rav[i];
      cnt += (spixel16)rbv[i];

      if (cnt > (spixel16)0x7F)
	  rcv[i] = 0x7F;
      else {
	  if (cnt < (spixel16)0xFF80)
	      rcv[i] = 0x80;
	  else
	      rcv[i] = rav[i] + rbv[i];
      }
  }

  return picode->next;
}

OP(paddsw4_f){
  spixel16 *rav = (spixel16 *)&REG(RA);
  spixel16 *rbv;
  spixel16 *rcv = (spixel16 *)&REG(RC);
  spixel16 rb[4],temp;
  int i;
  spixel32 cnt;

  REG(RC) = 0;
  if (!(picode->iflags & E_LITERAL)) 
      rbv = (spixel16 *)&REG(RB);  
  else {
      temp = (spixel16)0 + ((signed char )picode->literal);
      for( i = 0; i < 4; i++)
	  rb[i] = temp;
      rbv = rb;
  }  

  for (i = 0; i < 4; i++){
      cnt = (spixel32)rav[i] + (spixel32)rbv[i];

      if (cnt > (spixel32)0x7FFF)
	  rcv[i] = 0x7FFF;
      else {
	  if (cnt < (spixel32)0xFFFF8000)
	      rcv[i] = 0x8000;
	  else
	      rcv[i] = rav[i] + rbv[i];
      }
  }

  return picode->next;
}

OP(paddsl2_f){
  spixel32 *rav = (spixel32 *)&REG(RA);
  spixel32 *rbv;
  spixel32 *rcv = (spixel32 *)&REG(RC);
  spixel32 rb[2], temp;
  spixel64 cnt;
  int i;

  REG(RC) = 0;
  if (!(picode->iflags & E_LITERAL)) 
      rbv = (spixel32 *)&REG(RB);  
  else {
      temp = (spixel32)0 + ((signed char )picode->literal);
      for( i = 0; i < 2; i++)
	  rb[i] = temp;
      rbv = rb;
  }  

  for (i = 0; i < 2; i++){
      cnt = (spixel64)rav[i] + (spixel64)rbv[i];

      if (cnt > (spixel64)0x7FFFFFFF)
	  rcv[i] = 0x7FFFFFFF;
      else {
	  if (cnt < (spixel64)0xFFFFFFFF80000000)
	      rcv[i] = 0x80000000;
	  else
	      rcv[i] = rav[i] + rbv[i];
      }
  }

  return picode->next;
}

OP(paddub8_f)
{
  upixel8 *rav = (upixel8 *)&REG(RA);
  upixel8 *rbv;
  upixel8 *rcv = (upixel8 *)&REG(RC);
  upixel8 rb[8],temp;
  upixel16 sum;
  int i;

  if (!(picode->iflags & E_LITERAL)) 
      rbv = (upixel8 *)&REG(RB);  
  else {
      temp = (upixel8) (picode->literal & 0xFF);

      for( i = 0; i < 8; i++)
	  rb[i] = temp; 

      rbv = rb;
  }  

  for (i = 0; i < 8; i++){
      sum     = (upixel16)rav[i] + (upixel16)rbv[i];

      if (sum > 0xFF)
	  sum = 0xFF;

      rcv[i] = (upixel8)sum;
  }

  return picode->next;
}

OP(padduw4_f)
{
  upixel16 *rav = (upixel16 *)&REG(RA);
  upixel16 *rbv;
  upixel16 *rcv = (upixel16 *)&REG(RC);
  upixel16 rb[4],temp;
  upixel32 sum;
  int i;


  REG(RC) = 0;
  if (!(picode->iflags & E_LITERAL)) 
      rbv = (upixel16 *)&REG(RB);  
  else {
      temp = (upixel16) (picode->literal & 0xFF);
      for( i = 0; i < 4; i++)
	  rb[i] = temp; 
      rbv = rb;
  }  
  for (i = 0; i < 4; i++){
      sum  = (upixel32)rav[i] + (upixel32)rbv[i];

      if(sum > 0xFFFF)
	  sum = 0xFFFF;

      rcv[i] = (upixel16)sum;
  }

  return picode->next;
}


OP(paddul2_f)
{
  upixel32 *rav = (upixel32 *)&REG(RA);
  upixel32 *rbv;
  upixel32 *rcv = (upixel32 *)&REG(RC);
  upixel32 rb[2],temp;
  upixel64 sum;
  int i;


  REG(RC) = 0;
  if (!(picode->iflags & E_LITERAL)) 
      rbv = (upixel32 *)&REG(RB);  
  else {
      temp = (upixel32) (picode->literal & 0xFF);
      for( i = 0; i < 2; i++)
	  rb[i] = temp; 
      rbv = rb;
  }  
  for (i = 0; i < 2; i++){
      sum  = (upixel64)rav[i] + (upixel64)rbv[i];

      if(sum >  0xFFFFFFFF)
	  sum = 0xFFFFFFFF;

      rcv[i] = (upixel32)sum;
  }

  return picode->next;
}

OP(psubsb8_f)
{
  spixel8 *rav = (upixel8 *)&REG(RA);
  spixel8 *rbv;
  spixel8 *rcv = (upixel8 *)&REG(RC);
  spixel8 rb[8],temp;
  spixel16 sum;
  int i;

  if (!(picode->iflags & E_LITERAL)) 
      rbv = (spixel8 *)&REG(RB);  
  else {
      temp = (spixel8) (picode->literal & 0xFF);

      for( i = 0; i < 8; i++)
	  rb[i] = temp; 

      rbv = rb;
  }  

  for (i = 0; i < 8; i++){
      sum    = (spixel16)rav[i] - (spixel16)rbv[i];
      if (sum < (spixel16)0xFF80)
	  rcv[i] = 0x80;
      else {
	  if (sum > (spixel16)0x7F)
	      rcv[i] = 0x7F;
	  else
	      rcv[i] = rav[i] - rbv[i];
      }
  }

  return picode->next;
}

OP(psubsw4_f)
{
  spixel16 *rav = (upixel16 *)&REG(RA);
  spixel16 *rbv;
  spixel16 *rcv = (upixel16 *)&REG(RC);
  spixel16 rb[4],temp;
  spixel32 sum;
  int i;


  REG(RC) = 0;
  if (!(picode->iflags & E_LITERAL)) 
      rbv = (spixel16 *)&REG(RB);  
  else {
      temp = (spixel16)(picode->literal & 0xFF);
      for( i = 0; i < 4; i++)
	  rb[i] = temp; 
      rbv = rb;
  }  
  for (i = 0; i < 4; i++){
      sum = (spixel32)rav[i] - (spixel32)rbv[i];
     
      if (sum < (spixel32)0xFFFF8000)
	  rcv[i] = 0x8000;
      else {
	  if (sum > (spixel32)0x7FFF)
	      rcv[i] = 0x7FFF;
	  else
	      rcv[i] = rav[i] - rbv[i];
      }
  }

  return picode->next;
}

OP(psubsl2_f)
{
  spixel32 *rav = (upixel32 *)&REG(RA);
  spixel32 *rbv;
  spixel32 *rcv = (upixel32 *)&REG(RC);
  spixel32 rb[2],temp;
  spixel64 sum;
  int i;


  REG(RC) = 0;
  if (!(picode->iflags & E_LITERAL)) 
      rbv = (spixel32 *)&REG(RB);  
  else {
      temp = (spixel32)(picode->literal & 0xFF);
      for( i = 0; i < 2; i++)
	  rb[i] = temp; 
      rbv = rb;
  }  
  for (i = 0; i < 2; i++){
      sum = (spixel64)rav[i] - (spixel64)rbv[i];
     
      if (sum < (spixel32)0xFFFFFFFF80000000)
	  rcv[i] = 0x80000000;
      else {
	  if (sum > (spixel64)0x7FFFFFFF)
	      rcv[i] = 0x7FFFFFFF;
	  else
	      rcv[i] = rav[i] - rbv[i];
      }
  }

  return picode->next;
}

OP(psubub8_f)
{
  upixel8 *rav = (upixel8 *)&REG(RA);
  upixel8 *rbv;
  upixel8 *rcv = (upixel8 *)&REG(RC);
  upixel8 rb[8],temp;
  int i;

  if (!(picode->iflags & E_LITERAL)) 
      rbv = (upixel8 *)&REG(RB);  
  else {
      temp = (upixel8) (picode->literal & 0xFF);

      for( i = 0; i < 8; i++)
	  rb[i] = temp; 

      rbv = rb;
  }  

  for (i = 0; i < 8; i++){
      if (rav[i] > rbv[i])
	  rcv[i] = rav[i] - rbv[i];
      else
	  rcv[i] = 0;
  }

  return picode->next;
}

OP(psubuw4_f)
{
  upixel16 *rav = (upixel16 *)&REG(RA);
  upixel16 *rbv;
  upixel16 *rcv = (upixel16 *)&REG(RC);
  upixel16 rb[4],temp;
  int i;


  REG(RC) = 0;
  if (!(picode->iflags & E_LITERAL)) 
      rbv = (upixel16 *)&REG(RB);  
  else {
      temp = (upixel16) (picode->literal & 0xFF);
      for( i = 0; i < 4; i++)
	  rb[i] = temp; 
      rbv = rb;
  }  
  for (i = 0; i < 4; i++){
      if (rav[i] > rbv[i])
	  rcv[i] = rav[i] - rbv[i];
      else 
	  rcv[i] = 0;
  }

  return picode->next;
}

OP(psubul2_f)
{
  upixel32 *rav = (upixel32 *)&REG(RA);
  upixel32 *rbv;
  upixel32 *rcv = (upixel32 *)&REG(RC);
  upixel32 rb[2],temp;
  int i;


  REG(RC) = 0;
  if (!(picode->iflags & E_LITERAL)) 
      rbv = (upixel32 *)&REG(RB);  
  else {
      temp = (upixel32) (picode->literal & 0xFF);
      for( i = 0; i < 2; i++)
	  rb[i] = temp; 
      rbv = rb;
  }  
  for (i = 0; i < 2; i++){
      if (rav[i] > rbv[i])
	  rcv[i] = rav[i] - rbv[i];
      else 
	  rcv[i] = 0;
  }

  return picode->next;
}

OP(pmulluw4_f){
  upixel16 *rav = (upixel16 *)&REG(RA);
  upixel16 *rbv;
  upixel16 *rcv = (upixel16 *)&REG(RC);
  upixel16 rb[4],temp;
  int i;

  REG(RC) = 0;

  if (!(picode->iflags & E_LITERAL)) 
      rbv = (upixel16 *)&REG(RB);  
  else {
      temp = (upixel16)0 + ((unsigned char )picode->literal);

      for( i = 0; i < 4; i++)
	  rb[i] = temp;
      rbv = rb;
  }  

  for(i = 0; i < 4; i++)
      rcv[i] = (upixel16)(((upixel32)rav[i] * (upixel32)rbv[i]) & 0xFFFF);

  return picode->next;
}

OP(pmulhuw4_f){
  upixel16 *rav = (upixel16 *)&REG(RA);
  upixel16 *rbv;
  upixel16 *rcv = (upixel16 *)&REG(RC);
  upixel16 rb[4],temp;
  int i;

  REG(RC) = 0;

  if (!(picode->iflags & E_LITERAL)) 
      rbv = (upixel16 *)&REG(RB);  
  else {
      temp = (upixel16)0 + ((unsigned char )picode->literal);

      for( i = 0; i < 4; i++)
	  rb[i] = temp;
      rbv = rb;
  }  

  for(i = 0; i < 4; i++)
      rcv[i] = (upixel16)((((upixel32)rav[i] * (upixel32)rbv[i]) >> 16) & 0xFFFF);

  return picode->next;
}

OP(pminmaxsl2_f)
{
  spixel32 *rav = (spixel32*)&REG(RA);  
  spixel32 *rbv = (spixel32*)&REG(RB);
  spixel32 *rcv = (spixel32*)&REG(RC);
  spixel32 min, max;
  int i;

  min = rbv[0];
  max = rbv[1];

  for(i = 0; i < 2; i++){
      if (rav[i] < min)
	  min = rav[i];
      if (rav[i] > max)
	  max = rav[i];
  }
  rcv[0] = min;
  rcv[1] = max;
  return picode->next;
}

OP(pminmaxul2_f)
{
  upixel32 *rav = (upixel32*)&REG(RA);  
  upixel32 *rbv = (upixel32*)&REG(RB);
  upixel32 *rcv = (upixel32*)&REG(RC);
  upixel32 min, max;
  int i;

  min = rbv[0];
  max = rbv[1];

  for(i = 0; i < 2; i++){
      if (rav[i] < min)
	  min = rav[i];
      if (rav[i] > max)
	  max = rav[i];
  }
  rcv[0] = min;
  rcv[1] = max;
  return picode->next;
}

OP(pminmaxsb8_f)
{
  spixel8 *rav = (spixel8*)&REG(RA);  
  spixel8 *rbv = (spixel8*)&REG(RB);
  spixel8 *rcv = (spixel8*)&REG(RC);
  spixel8 min, max;
  int i;

  min = rbv[0];
  max = rbv[4];

  for(i = 0; i < 8; i++){
      if (rav[i] < min)
	  min = rav[i];
      if (rav[i] > max)
	  max = rav[i];
      rcv[i] = 0;
  }
  rcv[0] = min;
  rcv[4] = max;
  return picode->next;
}

OP(pminmaxsw4_f)
{
  spixel16 *rav = (spixel16*)&REG(RA);  
  spixel16 *rbv = (spixel16*)&REG(RB);
  spixel16 *rcv = (spixel16*)&REG(RC);
  spixel16 min, max;
  int i;

  min = rbv[0];
  max = rbv[2];

  for(i = 0; i < 4; i++){
      if (rav[i] < min)
	  min = rav[i];
      if (rav[i] > max)
	  max = rav[i];
  }
  rcv[1] = 0;
  rcv[3] = 0;
  rcv[0] = min;
  rcv[2] = max;
  return picode->next;
}

OP(pminmaxub8_f)
{
  upixel8 *rav = (upixel8*)&REG(RA);  
  upixel8 *rbv = (upixel8*)&REG(RB);
  upixel8 *rcv = (upixel8*)&REG(RC);
  upixel8 min, max;
  int i;

  min = rbv[0];
  max = rbv[4];

  for(i = 0; i < 8; i++){
      if (rav[i] < min)
	  min = rav[i];
      if (rav[i] > max)
	  max = rav[i];
      rcv[i] = 0;
  }
  rcv[0] = min;
  rcv[4] = max;
  return picode->next;
}

OP(pminmaxuw4_f)
{
  upixel16 *rav = (upixel16*)&REG(RA);  
  upixel16 *rbv = (upixel16*)&REG(RB);
  upixel16 *rcv = (upixel16*)&REG(RC);
  upixel16 min, max;
  int i;

  min = rbv[0];
  max = rbv[2];

  for(i = 0; i < 4; i++){
      if (rav[i] < min)
	  min = rav[i];
      if (rav[i] > max)
	  max = rav[i];
      rcv[i] = 0;
  }
  rcv[0] = min;
  rcv[2] = max;
  return picode->next;
}

OP(taddsb8_f){
  spixel8 *rav = (spixel8 *)&REG(RA);
  spixel8 *rbv;
  spixel64 *rcv = (spixel64 *)&REG(RC);
  spixel8 rb[8], temp_bt;
  spixel64 temp;
  int i;

  if (!(picode->iflags & E_LITERAL))
      rbv = (spixel8 *)&REG(RB);  
  else {
      temp_bt = (spixel8) (picode->literal & 0xFF);
      for( i = 0; i < 8; i++)
	  rb[i] = temp_bt; 
      rbv = rb;
  }
  
  temp = (spixel64)0;
  for (i = 0; i < 8; i++){

      temp += rav[i];
      temp += rbv[i];
  }
  *rcv = temp;

  return picode->next;
}

OP(taddsw4_f){
  spixel16 *rav = (spixel16 *)&REG(RA);
  spixel16 *rbv;
  spixel64 *rcv = (spixel64 *)&REG(RC);
  spixel16 rb[4], temp_wd;
  spixel64 temp;
  int i;

  REG(RC) = 0;
  if (!(picode->iflags & E_LITERAL)) 
      rbv = (spixel16 *)&REG(RB);  
  else {
      temp_wd = (spixel16)0 + ((signed char )picode->literal);
      for( i = 0; i < 4; i++)
	  rb[i] = temp_wd;
      rbv = rb;
  }  

  temp = 0;
  for (i = 0; i < 4; i++){
      temp += rav[i];
      temp += rbv[i];
  }
  *rcv = temp;
  return picode->next;
}

OP(taddub8_f)
{
  upixel8 *rav = (upixel8 *)&REG(RA);
  upixel8 *rbv;
  upixel64 *rcv = (upixel64 *)&REG(RC);
  upixel8 rb[8], temp_bt;
  upixel64 temp;
  int i;

  if (!(picode->iflags & E_LITERAL)) 
      rbv = (upixel8 *)&REG(RB);  
  else {
      temp_bt = (upixel8) (picode->literal & 0xFF);

      for( i = 0; i < 8; i++)
	  rb[i] = temp_bt; 

      rbv = rb;
  }  
  temp = 0;
  for (i = 0; i < 8; i++){
      temp += rav[i];
      temp += rbv[i];
  }
  *rcv = temp;
  return picode->next;
}

OP(tadduw4_f)
{
  upixel16 *rav = (upixel16 *)&REG(RA);
  upixel16 *rbv;
  upixel64 *rcv = (upixel64 *)&REG(RC);
  upixel16 rb[4], temp_wd;
  upixel64 temp;
  int i;


  REG(RC) = 0;
  if (!(picode->iflags & E_LITERAL)) 
      rbv = (upixel16 *)&REG(RB);  
  else {
      temp_wd = (upixel16) (picode->literal & 0xFF);
      for( i = 0; i < 4; i++)
	  rb[i] = temp_wd; 
      rbv = rb;
  }  
  temp = 0;
  for (i = 0; i < 4; i++){
      temp += rav[i];
      temp += rbv[i];
  }
  *rcv = temp;
  return picode->next;
}

OP(tsubsb8_f)
{
  spixel8 *rav = (spixel8 *)&REG(RA);
  spixel8 *rbv;
  spixel64 *rcv = (spixel64 *)&REG(RC);
  spixel8 rb[8], temp_bt;
  spixel64 temp;
  int i;

  if (!(picode->iflags & E_LITERAL)) 
      rbv = (spixel8 *)&REG(RB);  
  else {
      temp_bt = (spixel8) (picode->literal & 0xFF);

      for( i = 0; i < 8; i++)
	  rb[i] = temp_bt; 

      rbv = rb;
  }  
  temp = 0;
  for (i = 0; i < 8; i++){
      temp += rav[i];
      temp -= rbv[i];
  }
  *rcv = temp;
  return picode->next;
}

OP(tsubsw4_f)
{
  spixel16 *rav = (spixel16 *)&REG(RA);
  spixel16 *rbv;
  spixel64 *rcv = (spixel64 *)&REG(RC);
  spixel16 rb[4], temp_wd;
  spixel64 temp;
  int i;


  REG(RC) = 0;
  if (!(picode->iflags & E_LITERAL)) 
      rbv = (spixel16 *)&REG(RB);  
  else {
      temp_wd = (spixel16)(picode->literal & 0xFF);
      for( i = 0; i < 4; i++)
	  rb[i] = temp_wd; 
      rbv = rb;
  }

  temp = 0;
  for (i = 0; i < 4; i++){
      temp += rav[i];
      temp -= rbv[i];
  }
  *rcv = temp;
  return picode->next;
}

OP(tsubub8_f)
{
  upixel8 *rav = (upixel8 *)&REG(RA);
  upixel8 *rbv;
  upixel64 *rcv = (upixel64 *)&REG(RC);
  upixel8 rb[8],temp_bt;
  upixel64 temp;
  int i;

  if (!(picode->iflags & E_LITERAL)) 
      rbv = (upixel8 *)&REG(RB);  
  else {
      temp_bt = (upixel8) (picode->literal & 0xFF);

      for( i = 0; i < 8; i++)
	  rb[i] = temp_bt; 

      rbv = rb;
  }  
  temp = 0;
  for (i = 0; i < 8; i++){
      temp += rav[i];
      temp -= rbv[i];
  }
  *rcv = temp;
  return picode->next;
}

OP(tsubuw4_f)
{
  upixel16 *rav = (upixel16 *)&REG(RA);
  upixel16 *rbv;
  upixel64 *rcv = (upixel64 *)&REG(RC);
  upixel16 rb[4], temp_wd;
  upixel64 temp;
  upixel32 sum;
  int i;


  REG(RC) = 0;
  if (!(picode->iflags & E_LITERAL)) 
      rbv = (upixel16 *)&REG(RB);  
  else {
      temp_wd = (upixel16) (picode->literal & 0xFF);
      for( i = 0; i < 4; i++)
	  rb[i] = temp_wd; 
      rbv = rb;
  }

  temp = 0;
  for (i = 0; i < 4; i++){
      temp += rav[i];
      temp -= rbv[i];
  }
  *rcv = temp;
  return picode->next;
}

OP(tmulsb8_f){
  spixel8 *rav = (spixel8 *)&REG(RA);
  spixel8 *rbv;
  spixel64 *rcv = (spixel64 *)&REG(RC);
  spixel8 rb[4], temp_bt;
  spixel64 temp;
  int i;

  REG(RC) = 0;

  if (!(picode->iflags & E_LITERAL)) 
      rbv = (spixel8 *)&REG(RB);  
  else {
      temp_bt = (spixel8)0 + ((signed char )picode->literal);

      for( i = 0; i < 8; i++)
	  rb[i] = temp_bt;
      rbv = rb;
  } 

  temp = 0;
  for(i = 0; i < 8; i++)
      temp += (spixel16)rav[i] * (spixel16)rbv[i];

  *rcv = temp;

  return picode->next;
}

OP(tmulsw4_f){
  spixel16 *rav = (spixel16 *)&REG(RA);
  spixel16 *rbv;
  spixel64 *rcv = (spixel64 *)&REG(RC);
  spixel16 rb[4], temp_wd;
  spixel64 temp;
  int i;

  REG(RC) = 0;

  if (!(picode->iflags & E_LITERAL)) 
      rbv = (spixel16 *)&REG(RB);  
  else {
      temp_wd = (spixel16)0 + ((signed char )picode->literal);

      for( i = 0; i < 4; i++)
	  rb[i] = temp_wd;
      rbv = rb;
  }  

  temp = 0;
  for(i = 0; i < 4; i++)
      temp += (spixel32)rav[i] * (spixel32)rbv[i];

  *rcv = temp;

  return picode->next;
}

OP(tmulub8_f){
  upixel8 *rav = (upixel8 *)&REG(RA);
  upixel8 *rbv;
  upixel64 *rcv = (upixel64 *)&REG(RC);
  upixel8 rb[4], temp_bt;
  upixel64 temp;
  int i;

  REG(RC) = 0;

  if (!(picode->iflags & E_LITERAL)) 
      rbv = (upixel8 *)&REG(RB);  
  else {
      temp_bt = (upixel8)0 + ((unsigned char )picode->literal);

      for( i = 0; i < 8; i++)
	  rb[i] = temp_bt;
      rbv = rb;
  } 

  temp = 0;
  for(i = 0; i < 8; i++)
      temp += (upixel16)rav[i] * (upixel16)rbv[i];

  *rcv = temp;

  return picode->next;
}

OP(tmuluw4_f){
  upixel16 *rav = (upixel16 *)&REG(RA);
  upixel16 *rbv;
  upixel64 *rcv = (upixel64 *)&REG(RC);
  upixel16 rb[4], temp_wd;
  upixel64 temp, sop;
  int i;

  REG(RC) = 0;

  if (!(picode->iflags & E_LITERAL)) 
      rbv = (upixel16 *)&REG(RB);  
  else {
      temp_wd = (upixel16)0 + ((unsigned char )picode->literal);

      for( i = 0; i < 4; i++)
	  rb[i] = temp_wd;
      rbv = rb;
  }  
  
  sop = 0;
  for(i = 0; i < 4; i++){
      temp = (upixel32)0 + rav[i];
      sop += (temp * rbv[i]);
  }
  *rcv = sop;

  return picode->next;
}

OP(tmulusb8_f){
  upixel8 *rav = (upixel8 *)&REG(RA);
  spixel8 *rbv;
  spixel64 *rcv = (spixel64 *)&REG(RC);
  spixel8 rb[4], temp_bt;
  spixel64 temp;
  int i;

  REG(RC) = 0;

  if (!(picode->iflags & E_LITERAL)) 
      rbv = (spixel8 *)&REG(RB);  
  else {
      temp_bt = (spixel8)0 + ((signed char )picode->literal);

      for( i = 0; i < 8; i++)
	  rb[i] = temp_bt;
      rbv = rb;
  } 

  temp = 0;
  for(i = 0; i < 8; i++)
      temp += (upixel16)rav[i] * (spixel16)rbv[i];

  *rcv = temp;

  return picode->next;
}

OP(tmulusw4_f){
  upixel16 *rav = (upixel16 *)&REG(RA);
  spixel16 *rbv;
  spixel64 *rcv = (spixel64 *)&REG(RC);
  spixel16 rb[4], temp_wd;
  spixel64 temp;
  int i;

  REG(RC) = 0;

  if (!(picode->iflags & E_LITERAL)) 
      rbv = (upixel16 *)&REG(RB);  
  else {
      temp_wd = (spixel16)0 + ((signed char )picode->literal);

      for( i = 0; i < 4; i++)
	  rb[i] = temp_wd;
      rbv = rb;
  }  
  
  temp = 0;
  for(i = 0; i < 4; i++)
      temp += (upixel32)rav[i] * (spixel32)rbv[i];
  *rcv = temp;

  return picode->next;
}

OP(psrb8_f){
  upixel8 *rav = (upixel8 *)&REG(RA);
  upixel8 *rbv ;
  upixel8 *rcv = (upixel8 *)&REG(RC);
  upixel8 rb[8], cnt, temp_bt;
  int i;

  REG(RC) = 0;

  if (!(picode->iflags & E_LITERAL)) { 
    rbv = (upixel8 *)&REG(RB);  
  } else {
      temp_bt  = (upixel8)0 + ((unsigned char )picode->literal);
      for(i = 0; i < 8; i++)
	  rb[i] = temp_bt;
      rbv = rb;
  }  

  cnt = rbv[0] & 0x7;
  for(i = 0; i < 8; i++)
      rcv[i] = rav[i] >> cnt;

  return picode->next;
}

OP(psrw4_f){
  upixel16 *rav = (upixel16 *)&REG(RA);
  upixel16 *rbv ;
  upixel16 *rcv = (upixel16 *)&REG(RC);
  upixel16 rb[4], cnt, temp_wd;
  int i;

  REG(RC) = 0;

  if (!(picode->iflags & E_LITERAL)) { 
    rbv = (upixel16 *)&REG(RB);  
  } else {
      temp_wd  = (upixel16)0 + ((unsigned char )picode->literal);
      for(i = 0; i < 4; i++)
	  rb[i] = temp_wd;
      rbv = rb;
  }  

  cnt = rbv[0] & 0xF;
  for(i = 0; i < 4; i++)
      rcv[i] = rav[i] >> cnt;

  return picode->next;
}

OP(pslb8_f){
  upixel8 *rav = (upixel8 *)&REG(RA);
  upixel8 *rbv ;
  upixel8 *rcv = (upixel8 *)&REG(RC);
  upixel8 rb[8], cnt, temp_bt;
  int i;

  REG(RC) = 0;

  if (!(picode->iflags & E_LITERAL)) { 
    rbv = (upixel8 *)&REG(RB);  
  } else {
      temp_bt  = (upixel8)0 + ((unsigned char )picode->literal);
      for(i = 0; i < 8; i++)
	  rb[i] = temp_bt;
      rbv = rb;
  }  

  cnt = rbv[0] & 0x7;
  for(i = 0; i < 8; i++)
      rcv[i] = rav[i] << cnt;

  return picode->next;
}

OP(pslw4_f){
  upixel16 *rav = (upixel16 *)&REG(RA);
  upixel16 *rbv ;
  upixel16 *rcv = (upixel16 *)&REG(RC);
  upixel16 rb[4], cnt, temp_wd;
  int i;

  REG(RC) = 0;

  if (!(picode->iflags & E_LITERAL)) { 
    rbv = (upixel16 *)&REG(RB);  
  } else {
      temp_wd  = (upixel16)0 + ((unsigned char )picode->literal);
      for(i = 0; i < 4; i++)
	  rb[i] = temp_wd;
      rbv = rb;
  }  

  cnt = rbv[0] & 0xF;
  for(i = 0; i < 4; i++)
      rcv[i] = rav[i] << cnt;

  return picode->next;
}

OP(psll2_f){
  upixel32 *rav = (upixel32 *)&REG(RA);
  upixel32 *rbv ;
  upixel32 *rcv = (upixel32 *)&REG(RC);
  upixel32 rb[2], cnt, temp_wd;
  int i;

  REG(RC) = 0;

  if (!(picode->iflags & E_LITERAL)) { 
    rbv = (upixel32 *)&REG(RB);  
  } else {
      temp_wd  = (upixel32)0 + ((unsigned char )picode->literal);
      for(i = 0; i < 2; i++)
	  rb[i] = temp_wd;
      rbv = rb;
  }  

  cnt = rbv[0] & 0xF;
  for(i = 0; i < 2; i++)
      rcv[i] = rav[i] << cnt;

  return picode->next;
}

OP(psrl2_f){
  upixel32 *rav = (upixel32 *)&REG(RA);
  upixel32 *rbv ;
  upixel32 *rcv = (upixel32 *)&REG(RC);
  upixel32 rb[2], signs, cnt, rounds, temp_bt;
  int i;

  REG(RC) = 0;

  if (!(picode->iflags & E_LITERAL)) { 
    rbv = (upixel32 *)&REG(RB);
  } else {
      temp_bt  = (upixel32)0 + ((unsigned char )picode->literal);
      for (i = 0; i < 2; i++)
	  rb[i] = temp_bt;
      rbv = rb;
  }  

  cnt = rbv[0] & 0x1F;
  
  for(i = 0; i < 2; i++)
      rcv[i] = rav[i] >> cnt;
      
  return picode->next;
}

OP(psral2_f){
  spixel32 *rav = (spixel32 *)&REG(RA);
  spixel32 *rbv ;
  spixel32 *rcv = (spixel32 *)&REG(RC);
  spixel32 rb[2], signs, cnt, temp_wd;
  int i;

  REG(RC) = 0;

  if (!(picode->iflags & E_LITERAL)) { 
    rbv = (upixel32 *)&REG(RB);
  } else {
      temp_wd  = (spixel32)0 + ((unsigned char )picode->literal);
      for (i = 0; i < 2; i++)
	  rb[i] = temp_wd;
      rbv = rb;
  }  

  cnt = rbv[0] & 0x1F;
  for(i = 0; i < 2; i++){
      rcv[i] = rav[i] >> cnt;
      if (rav[i] & 0x80000000)
	  signs = 0xFFFFFFFF;
      else
	  signs = 0;
      rcv[i] |= (signs << (32 - cnt));
  }

  return picode->next;
}

OP(psrab8_f){
  spixel8 *rav = (spixel8 *)&REG(RA);
  spixel8 *rbv ;
  spixel8 *rcv = (spixel8 *)&REG(RC);
  spixel8 rb[8], signs, cnt, temp_bt;
  int i;

  REG(RC) = 0;

  if (!(picode->iflags & E_LITERAL)) { 
    rbv = (upixel8 *)&REG(RB);
  } else {
      temp_bt  = (spixel8)0 + ((unsigned char )picode->literal);
      for (i = 0; i < 4; i++)
	  rb[i] = temp_bt;
      rbv = rb;
  }  

  cnt = rbv[0] & 0x7;
  
  for(i = 0; i < 8; i++){
      if (rav[i] & 0x80)
	  signs = 0xFF;
      else
	  signs = 0x00;
      rcv[i]  = rav[i] >> cnt;
      signs <<= (8 - cnt);
      rcv[i] |= signs;
  }

  return picode->next;
}

OP(psraw4_f){
  spixel16 *rav = (spixel16 *)&REG(RA);
  spixel16 *rbv ;
  spixel16 *rcv = (spixel16 *)&REG(RC);
  spixel16 rb[4], signs, cnt, temp_wd;
  int i;

  REG(RC) = 0;

  if (!(picode->iflags & E_LITERAL)) { 
    rbv = (upixel16 *)&REG(RB);
  } else {
      temp_wd  = (spixel16)0 + ((unsigned char )picode->literal);
      for (i = 0; i < 4; i++)
	  rb[i] = temp_wd;
      rbv = rb;
  }  

  cnt = rbv[0] & 0xF;
  for(i = 0; i < 4; i++){
      rcv[i] = rav[i] >> cnt;
      if (rav[i] & 0x8000)
	  signs = 0xFFFF;
      else
	  signs = 0;
      rcv[i] |= (signs << (16 - cnt));
  }

  return picode->next;
}

OP(pkswb8_f){
  spixel16 *rav = (spixel16 *)&REG(RA);
  spixel16 *rbv = (spixel16 *)&REG(RB);
  spixel8 *rcv  = (spixel8 *)&REG(RC);
  spixel16 max, min;
  int i;

  REG(RC) = 0;
 
  max = (spixel16)0x00FF;
  min = (spixel16)0xFF80;
  for (i = 0; i < 4; i++){
      if (rav[i] > max)
	  rcv[i] = 0x7F;
      else {
	  if (rav[i] < min)
	      rcv[i] = 0x80;
	  else
	      rcv[i] = (spixel8)(rav[i] & 0xFF);
      }
      if (rbv[i] > max)
	  rcv[i + 4] = 0x7F;
      else {
	  if (rbv[i] < min)
	      rcv[i + 4] = 0x80;
	  else
	      rcv[i + 4] = (spixel8)(rbv[i] & 0xFF);
      }
  }
  return picode->next;

}

OP(pkslw4_f){
  spixel32 *rav = (spixel32 *)&REG(RA);
  spixel32 *rbv = (spixel32 *)&REG(RB);
  spixel16 *rcv = (spixel16 *)&REG(RC);
  spixel32 max, min;
  int i;

  REG(RC) = 0;

  max = (spixel32)32767;
  min = (spixel32)-32768;
  for (i = 0; i < 2; i++){
      if (rav[i] > max)
	  rcv[i] = 0x7FFF;
      else {
	  if (rav[i] < min)
	      rcv[i] = 0x8000;
	  else
	      rcv[i] = (spixel16)(rav[i] & 0xFFFF);
      }
      if (rbv[i] > max)
	  rcv[i + 2] = 0x7FFF;
      else {
	  if (rbv[i] < min)
	      rcv[i + 2] = 0x8000;
	  else
	      rcv[i + 2] = (spixel16)(rbv[i] & 0xFFFF);
      }
  }
  return picode->next;


}

OP(pkuwb8_f){
  upixel16 *rav = (upixel16 *)&REG(RA);
  upixel16 *rbv = (upixel16 *)&REG(RB);
  upixel8 *rcv  = (upixel8 *)&REG(RC);
  upixel16 max;
  int i;

  REG(RC) = 0;

  max = (upixel16)0x00FF;
  for (i = 0; i < 4; i++){
      if (rbv[i] > max)
	  rcv[i] = 0xFF;
      else 
	  rcv[i] = (upixel8)(rbv[i] & 0xFF);
 
      if (rav[i] > max)
	  rcv[i + 4] = 0xFF;
      else
	  rcv[i + 4] = (upixel8)(rav[i] & 0xFF);
  }
  return picode->next;

}

OP(pkulw4_f){
  upixel32 *rav = (upixel32 *)&REG(RA);
  upixel32 *rbv = (upixel32 *)&REG(RB);
  upixel16 *rcv = (upixel16 *)&REG(RC);
  upixel32 max;
  int i;

  REG(RC) = 0;

  max = (upixel32)0x00FFFF;
  for (i = 0; i < 2; i++){
      if (rav[i] > max)
	  rcv[i] = 0xFFFF;
      else 
	  rcv[i] = (upixel16)(rav[i] & 0xFFFF);
 
      if (rbv[i] > max)
	  rcv[i + 2] = 0xFFFF;
      else
	  rcv[i + 2] = (upixel16)(rbv[i] & 0xFFFF);
  }
  return picode->next;

}

OP(upksbw4_f){
  spixel8 *rbv = (spixel8 *)&REG(RB);
  spixel8 *rcv = (spixel8 *)&REG(RC);
  int i, k;

  for(i = 0; i < 4; i++){
      k = i << 1;
      rcv[k] = rbv[i];
      if (rbv[i] & 0x80)
	  rcv[k+1] = 0xFF;
      else
	  rcv[k+1] = 0;
  }
  return picode->next;      
}

OP(upkswl2_f){
  spixel16 *rbv = (spixel16 *)&REG(RB);
  spixel16 *rcv = (spixel16 *)&REG(RC);
  int i, k;

  for(i = 0; i < 2; i++){
      k = i << 1;
      rcv[k] = rbv[i];
      if (rbv[i] & 0x8000)
	  rcv[k+1] = 0xFFFF;
      else
	  rcv[k+1] = 0;
  }
  return picode->next;      
}

OP(upkubw4_f){
  upixel8 *rav = (upixel8 *)&REG(RA);
  upixel8 *rbv = (upixel8 *)&REG(RB);
  upixel8 *rcv = (upixel8 *)&REG(RC);
  int i, k;

  for(i = 0; i < 4; i++){
      k = i << 1;
      rcv[k] = rbv[i];
      rcv[k+1] = rav[i];
  }
  return picode->next;      
}

OP(upkuwl2_f){
  upixel16 *rav = (upixel16 *)&REG(RA);
  upixel16 *rbv = (upixel16 *)&REG(RB);
  upixel16 *rcv = (upixel16 *)&REG(RC);
  int i, k;

  for(i = 0; i < 2; i++){
      k = i << 1;
      rcv[k] = rbv[i];
      rcv[k+1] = rav[i];
  }
  return picode->next;      
}

OP(tabserrsb8_f)
{
  spixel8 *rav = (spixel8 *)&REG(RA);
  spixel8 *rbv;
  spixel64 *rcv = (spixel64 *)&REG(RC);
  spixel8 rb[8], temp_bt;
  spixel64 temp;
  int i;

  if (!(picode->iflags & E_LITERAL)) 
      rbv = (spixel8 *)&REG(RB);  
  else {
      temp_bt = (spixel8) (picode->literal & 0xFF);

      for( i = 0; i < 8; i++)
	  rb[i] = temp_bt; 

      rbv = rb;
  }  
  temp = 0;
  for (i = 0; i < 8; i++){
      if (rav[i] > rbv[i])
	  temp += (rav[i] - rbv[i]);
      else
	  temp += (rbv[i] - rav[i]);
  }
  *rcv = temp;
  return picode->next;
}

OP(tabserrub8_f)
{
  upixel8 *rav = (upixel8 *)&REG(RA);
  upixel8 *rbv;
  upixel64 *rcv = (upixel64 *)&REG(RC);
  upixel8 rb[8], temp_bt;
  upixel64 temp;
  int i;

  if (!(picode->iflags & E_LITERAL)) 
      rbv = (upixel8 *)&REG(RB);  
  else {
      temp_bt = (upixel8) (picode->literal & 0xFF);

      for( i = 0; i < 8; i++)
	  rb[i] = temp_bt; 

      rbv = rb;
  }  
  temp = 0;
  for (i = 0; i < 8; i++){
      if (rav[i] > rbv[i])
	  temp += (rav[i] - rbv[i]);
      else
	  temp += (rbv[i] - rav[i]);
  }
  *rcv = temp;
  return picode->next;
}

OP(tabserrsw4_f)
{
  spixel16 *rav = (spixel16 *)&REG(RA);
  spixel16 *rbv;
  spixel64 *rcv = (spixel64 *)&REG(RC);
  spixel16 rb[4], temp_bt;
  spixel64 temp;
  int i;

  if (!(picode->iflags & E_LITERAL)) 
      rbv = (spixel16 *)&REG(RB);  
  else {
      temp_bt = (spixel16) (picode->literal & 0xFF);

      for( i = 0; i < 4; i++)
	  rb[i] = temp_bt; 

      rbv = rb;
  }  
  temp = 0;
  for (i = 0; i < 4; i++){
      if (rav[i] > rbv[i])
	  temp += (rav[i] - rbv[i]);
      else
	  temp += (rbv[i] - rav[i]);
  }
  *rcv = temp;
  return picode->next;
}

OP(tabserruw4_f)
{
  upixel16 *rav = (upixel16 *)&REG(RA);
  upixel16 *rbv;
  upixel64 *rcv = (upixel64 *)&REG(RC);
  upixel16 rb[4], temp_bt;
  upixel64 temp;
  int i;

  if (!(picode->iflags & E_LITERAL)) 
      rbv = (upixel16 *)&REG(RB);  
  else {
      temp_bt = (upixel16) (picode->literal & 0xFF);

      for( i = 0; i < 4; i++)
	  rb[i] = temp_bt; 

      rbv = rb;
  }  
  temp = 0;
  for (i = 0; i < 4; i++){
      if (rav[i] > rbv[i])
	  temp += (rav[i] - rbv[i]);
      else
	  temp += (rbv[i] - rav[i]);
  }
  *rcv = temp;
  return picode->next;
}

OP(tsqerrsb8_f)
{
  spixel8 *rav = (spixel8 *)&REG(RA);
  spixel8 *rbv;
  spixel64 *rcv = (spixel64 *)&REG(RC);
  spixel8 rb[8], temp_bt;
  spixel64 temp;
  int i;

  if (!(picode->iflags & E_LITERAL)) 
      rbv = (spixel8 *)&REG(RB);  
  else {
      temp_bt = (spixel8) (picode->literal & 0xFF);

      for( i = 0; i < 8; i++)
	  rb[i] = temp_bt; 

      rbv = rb;
  }  
  temp = 0;
  for (i = 0; i < 8; i++)
	  temp += ((rav[i] - rbv[i]) * (rav[i] - rbv[i]));

  *rcv = temp;
  return picode->next;
}

OP(tsqerrub8_f)
{
  upixel8 *rav = (upixel8 *)&REG(RA);
  upixel8 *rbv;
  upixel64 *rcv = (upixel64 *)&REG(RC);
  upixel8 rb[8], temp_bt;
  upixel64 temp;
  int i;

  if (!(picode->iflags & E_LITERAL)) 
      rbv = (upixel8 *)&REG(RB);  
  else {
      temp_bt = (upixel8) (picode->literal & 0xFF);

      for( i = 0; i < 8; i++)
	  rb[i] = temp_bt; 

      rbv = rb;
  }  
  temp = 0;
  for (i = 0; i < 8; i++)
      temp += ((rav[i] - rbv[i]) * (rav[i] - rbv[i]));
  *rcv = temp;
  return picode->next;
}

OP(tsqerrsw4_f)
{
  spixel16 *rav = (spixel16 *)&REG(RA);
  spixel16 *rbv;
  spixel64 *rcv = (spixel64 *)&REG(RC);
  spixel16 rb[4], temp_bt;
  spixel64 temp;
  int i;

  if (!(picode->iflags & E_LITERAL)) 
      rbv = (spixel16 *)&REG(RB);  
  else {
      temp_bt = (spixel16) (picode->literal & 0xFF);

      for( i = 0; i < 4; i++)
	  rb[i] = temp_bt; 

      rbv = rb;
  }  
  temp = 0;
  for (i = 0; i < 4; i++)
      temp += ((rav[i] - rbv[i]) * (rav[i] - rbv[i]));

  *rcv = temp;
  return picode->next;
}

OP(tsqerruw4_f)
{
  upixel16 *rav = (upixel16 *)&REG(RA);
  upixel16 *rbv;
  upixel64 *rcv = (upixel64 *)&REG(RC);
  upixel16 rb[4], temp_bt;
  upixel64 temp;
  int i;

  if (!(picode->iflags & E_LITERAL)) 
      rbv = (upixel16 *)&REG(RB);  
  else {
      temp_bt = (upixel16) (picode->literal & 0xFF);

      for( i = 0; i < 4; i++)
	  rb[i] = temp_bt; 

      rbv = rb;
  }  
  temp = 0;
  for (i = 0; i < 4; i++)
      temp += ((rav[i] - rbv[i]) * (rav[i] - rbv[i]));

  *rcv = temp;
  return picode->next;
}

OP(permb8_f)
{
  upixel8 *rav = (upixel8 *)&REG(RA);
  upixel32 *rbv = (upixel32 *)&REG(RB);
  upixel8 *rblit = (upixel8 *)&REG(RB) + 4;
  upixel8 *rcv = (upixel8 *)&REG(RC);
  int i,ind;
  upixel32 cnt;

  cnt  = rbv[0];

  for(i = 0; i < 8; i++){
      ind = cnt & 0x7;
      if (cnt & 8){

	switch(cnt & 0xF) {
	case 8:
	  rcv[i] = 0;
	  break;

	case 9:
	  rcv[i] = 0xff;
	  break;

	case 10:
	  if (i==0) rcv[i] = 0;
	  else {
	    if(rcv[i-1] & 0x80) rcv[i] = 0xff;
	    else rcv[i] = 0;
	  }
	  break;

	case 11:
	  rcv[i] = 0;
	  break;
	  
	case 12:
	  rcv[i] = rblit[0];
	  break;

	case 13:
	  rcv[i] = rblit[1];
	  break;

	case 14:
	  rcv[i] = rblit[2];
	  break;

	case 15:
	  rcv[i] = rblit[3];
	  break;
	}
      }
      else
	  rcv[i] = rav[ind];

      cnt >>= 4;
  }
  return picode->next;      
}

OP(extqa_f)
{
  upixel64 *rav = (upixel64 *)&REG(RA);
  upixel64 *rbv = (upixel64 *)&REG(RB);
  upixel64 *rcv = (upixel64 *)&REG(RC);
  
  *rcv = ((*rav) & 0x0FFFFFFFFFFFFFF) |
      (((*rbv) & 0x7) << 56);

  return picode->next;      
}

OP(extqd_f)
{
  upixel64 *rav = (upixel64 *)&REG(RA);
  upixel64 *rbv = (upixel64 *)&REG(RB);
  upixel64 *rcv = (upixel64 *)&REG(RC);
  upixel64 temp;
  int cnt;
  
  cnt = (((*rbv) >> 56) & 0x7) << 3;
  if (cnt == 0)
      *rcv = *rav;
  else 
      *rcv = ((*rav) >> cnt) | ((*rbv) << (64 - cnt));
 
  return picode->next;      
}

OP(bsextb_f)
{
  upixel8 *rav = (upixel8 *)&REG(RA);
  upixel8 *rbv = (upixel8 *)&REG(RB);
  upixel64 *rcv = (upixel64 *)&REG(RC);
  upixel8 src, mask, temp;
  int i, k;

  *rcv = (upixel64)0xFFFFFFFFFFFFFFFF;

  src = rav[0];
  for(i = 0; i < 4; i++){
      k = i << 1;
      mask = 1 << (rbv[k+1] & 7);
      mask = (upixel8)(mask - 1);
      temp = rbv[k];
      if ((src & mask) == (temp & mask)){
	  *rcv = (upixel64)i;
	  break;
      }
  }
  return picode->next;      
}

OP(bsextw_f)
{
  upixel16 *rav = (upixel16 *)&REG(RA);
  upixel16 *rbv = (upixel16 *)&REG(RB);
  upixel64 *rcv = (upixel64 *)&REG(RC);
  upixel16 src, mask, temp;
  int i, k;

  *rcv = (upixel64)0xFFFFFFFFFFFFFFFF;

  src = rav[0];
  for(i = 0; i < 2; i++){
      k = i << 1;
      mask = rbv[k+1] & 0xF;
      mask = (1 << mask) - 1;
      temp = rbv[k];
      if ((src & mask) == (temp & mask)){
	  *rcv = (upixel64)i;
	  break;
      }
  }

  return picode->next;      
}

OP(bmatch_f)
{
  upixel8 *rav = (upixel8 *)&REG(RA);
  upixel8 *rbv = (upixel8 *)&REG(RB);
  upixel64 *rcv = (upixel64 *)&REG(RC);
  upixel8 src;
  int i;

  *rcv = (upixel64)0xFFFFFFFFFFFFFFFF;
  src = rav[0];
  for (i = 0; i < 8; i++){
      if (src == rbv[i]){
	  *rcv = (upixel64)i;
	  break;
      }
  }
  return picode->next;      
}


OP(cmpwge_f){
  upixel16 *rav = (upixel16 *)&REG(RA);
  upixel16 *rbv = (upixel16 *)&REG(RB);
  ulong    *rcv = (ulong *)&REG(RC);
  int i,cnt;

  cnt = 0;
  for (i = 3; i >= 0; i--) {
      cnt <<= 2;

      if (rav[i] >= rbv[i])
	  cnt |= 0x3;
  }
  *rcv = (ulong) cnt;
  return picode->next;
}

OP(cmpdge_f){
  upixel32 *rav = (upixel32 *)&REG(RA);
  upixel32 *rbv = (upixel32 *)&REG(RB);

  REG(RC) = 0;
  if (rav[0] >= rbv[0])
      REG(RC) |= (long)0xF;

  if (rav[1] >= rbv[1])
      REG(RC) |= (long)0xF0;

  return picode->next;
}

OP(gpkblb4_f)
{
  upixel8 *rav = (upixel8 *)&REG(RA);
  upixel8 *rbv = (upixel8 *)&REG(RB);
  upixel8 *rcv = (upixel8 *)&REG(RC);
  int i;

  rcv[0] = rav[3];
  rcv[1] = rav[7];
  rcv[2] = rbv[3];
  rcv[3] = rbv[7];

  for(i = 4; i< 8; i ++)
      rcv[i] = 0;

  return picode->next;

}

#else

OP(pklb_f)
{
  upixel8 *rbv = (upixel8*)&REG(RB);
  upixel8 *rcv = (upixel8*)&REG(RC);

  REG(RC) = 0;
  rcv[0] = rbv[0];
  rcv[1] = rbv[4];

  return picode->next;
}

OP(pkwb_f)
{
  upixel8 *rbv = (upixel8*)&REG(RB);
  upixel8 *rcv = (upixel8*)&REG(RC);

  REG(RC) = 0;
  rcv[0] = rbv[0];
  rcv[1] = rbv[2];
  rcv[2] = rbv[4];
  rcv[3] = rbv[6];

  return picode->next;
}

OP(unpkbl_f)
{
  upixel8 *rbv = (upixel8*)&REG(RB);
  upixel8 *rcv = (upixel8*)&REG(RC);

  REG(RC) = 0;
  rcv[0] = rbv[0];
  rcv[4] = rbv[1];
  return picode->next;
}

OP(unpkbw_f)
{
  upixel8 *rbv = (upixel8*)&REG(RB);
  upixel8 *rcv = (upixel8*)&REG(RC);

  REG(RC) = 0;
  rcv[0] = rbv[0];
  rcv[2] = rbv[1];
  rcv[4] = rbv[2];
  rcv[6] = rbv[3];

  return picode->next;
}
#endif

OP(ctpop_f)
{
  int cnt = 0;
  int i;
  ulong rbv = REG(RB);
  for (i = 0; i < 64; i++) {
    if ((rbv >> i) & 1)
      cnt++;
  }
  REG(RC) = cnt;
  return picode->next;
}

OP(ctlz_f)
{
  int cnt = 0;
  int i;
  ulong rbv = REG(RB);
  for (i = 63; i >= 0; i--) {
    if ((rbv >> i) & 1)
      break;
    cnt++;
  }
  REG(RC) = cnt;
  return picode->next;
}

OP(cttz_f)
{
  int cnt = 0;
  int i;
  ulong rbv = REG(RB);
  for (i = 0; i < 64; i++) {
    if ((rbv >> i) & 1)
      break;
    cnt++;
  }
  REG(RC) = cnt;
  return picode->next;
}


OP(itofs_f)
{
  double rcv;
  long tmp;
  asm("   stl %1, 0(%2)\n\t"
      "   lds %0, 0(%2)\n"
      : "=f" (rcv) : "r" (REG(RA)), "r" (&tmp));
  FP(RC) = rcv;
  return picode->next;
}

OP(itoff_f)
{
  double rcv;
  long tmp;
  asm("  stl %1, 0(%2)\n\t"
      "  ldf %0, 0(%2)\n"
      : "=f" (rcv) : "r" (REG(RA)), "r" (&tmp));
  FP(RC) = rcv;
  return picode->next;
}

OP(itoft_f)
{
  REG(RC) = REG(RA);
  return picode->next;
}

OP(ftois_f)
{
  long rcv;
  long tmp;
  asm("  sts %1, 0(%2)\n\t"
      "  ldl %0, 0(%2)\n"
      : "=r" (rcv) : "f" (FP(RA)), "r" (&tmp));
  REG(RC) = rcv;
  return picode->next;
}

OP(ftoit_f)
{
  REG(RC) = REG(RA);
  return picode->next;
}

OP(ecb_f)
{
  /* for now, does nothing in AINT */
  return picode->next;
}

OP(wh64_f)
{
  /* for now, does nothing in AINT */
  return picode->next;
}

OP(wh64en_f)
{
  /* for now, does nothing in AINT */
  return picode->next;
}

#ifdef NEW_MVI
OP(padd_f)
{
    float ra_hi, ra_lo, rb_hi, rb_lo;
    float rc_hi, rc_lo;
    float *rcv_hi = (float *)&SPFP_HI(RC);
    float *rcv_lo = (float *)&SPFP_LO(RC);

    ra_hi = SPFP_HI(RA); ra_lo = SPFP_LO(RA);
    rb_hi = SPFP_HI(RB); rb_lo = SPFP_LO(RB);

    rc_hi = ra_hi + rb_hi;
    rc_lo = ra_lo + rb_lo;

    *rcv_hi = rc_hi; *rcv_lo = rc_lo;
    CHECK_FPCR_MVI(ifi, pthread);
    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
    
OP(psub_f)
{
    float ra_hi, ra_lo, rb_hi, rb_lo;
    float rc_hi, rc_lo;
    float *rcv_hi = (float *)&SPFP_HI(RC);
    float *rcv_lo = (float *)&SPFP_LO(RC);

    ra_hi = SPFP_HI(RA); ra_lo = SPFP_LO(RA);
    rb_hi = SPFP_HI(RB); rb_lo = SPFP_LO(RB);

    rc_hi = ra_hi - rb_hi;
    rc_lo = ra_lo - rb_lo;

    *rcv_hi = rc_hi; *rcv_lo = rc_lo;
    CHECK_FPCR_MVI(ifi, pthread);
    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
    
OP(paddc_f)
{
    float ra_hi, ra_lo, rb_hi, rb_lo;
    float rc_hi, rc_lo;
    float *rcv_hi = (float *)&SPFP_HI(RC);
    float *rcv_lo = (float *)&SPFP_LO(RC);

    ra_hi = SPFP_HI(RA); ra_lo = SPFP_LO(RA);
    rb_hi = SPFP_HI(RB); rb_lo = SPFP_LO(RB);

    rc_hi = ra_hi + rb_lo;
    rc_lo = ra_lo + rb_hi;

    *rcv_hi = rc_hi; *rcv_lo = rc_lo;
    CHECK_FPCR_MVI(ifi, pthread);
    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
    
OP(psubc_f)
{
    float ra_hi, ra_lo, rb_hi, rb_lo;
    float rc_hi, rc_lo;
    float *rcv_hi = (float *)&SPFP_HI(RC);
    float *rcv_lo = (float *)&SPFP_LO(RC);

    ra_hi = SPFP_HI(RA); ra_lo = SPFP_LO(RA);
    rb_hi = SPFP_HI(RB); rb_lo = SPFP_LO(RB);

    rc_hi = ra_hi - rb_lo;
    rc_lo = ra_lo - rb_hi;

    *rcv_hi = rc_hi; *rcv_lo = rc_lo;
    CHECK_FPCR_MVI(ifi, pthread);
    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
    
OP(phadd_f)
{
    float ra_hi, ra_lo, rb_hi, rb_lo;
    float rc_hi, rc_lo;
    float *rcv_hi = (float *)&SPFP_HI(RC);
    float *rcv_lo = (float *)&SPFP_LO(RC);

    ra_hi = SPFP_HI(RA); ra_lo = SPFP_LO(RA);
    rb_hi = SPFP_HI(RB); rb_lo = SPFP_LO(RB);

    rc_hi = ra_hi + ra_lo;
    rc_lo = rb_hi + rb_lo;

    *rcv_hi = rc_hi; *rcv_lo = rc_lo;
    CHECK_FPCR_MVI(ifi, pthread);
    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
    
OP(phsub_f)
{
    float ra_hi, ra_lo, rb_hi, rb_lo;
    float rc_hi, rc_lo;
    float *rcv_hi = (float *)&SPFP_HI(RC);
    float *rcv_lo = (float *)&SPFP_LO(RC);

    ra_hi = SPFP_HI(RA); ra_lo = SPFP_LO(RA);
    rb_hi = SPFP_HI(RB); rb_lo = SPFP_LO(RB);

    rc_hi = ra_hi - ra_lo;
    rc_lo = rb_hi - rb_lo;

    *rcv_hi = rc_hi; *rcv_lo = rc_lo;

    CHECK_FPCR_MVI(ifi, pthread);
    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
    
OP(phsubr_f)
{
    float ra_hi, ra_lo, rb_hi, rb_lo;
    float rc_hi, rc_lo;
    float *rcv_hi = (float *)&SPFP_HI(RC);
    float *rcv_lo = (float *)&SPFP_LO(RC);

    ra_hi = SPFP_HI(RA); ra_lo = SPFP_LO(RA);
    rb_hi = SPFP_HI(RB); rb_lo = SPFP_LO(RB);

    rc_hi = ra_lo - ra_hi;
    rc_lo = rb_lo - rb_hi;

    *rcv_hi = rc_hi; *rcv_lo = rc_lo;
    CHECK_FPCR_MVI(ifi, pthread);
    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
    
OP(pmul_f)
{
    float ra_hi, ra_lo, rb_hi, rb_lo;
    float rc_hi, rc_lo;
    float *rcv_hi = (float *)&SPFP_HI(RC);
    float *rcv_lo = (float *)&SPFP_LO(RC);

    ra_hi = SPFP_HI(RA); ra_lo = SPFP_LO(RA);
    rb_hi = SPFP_HI(RB); rb_lo = SPFP_LO(RB);

    rc_hi = (float)(ra_hi * rb_hi);
    rc_lo = (float)(ra_lo * rb_lo);

    *rcv_hi = rc_hi; *rcv_lo = rc_lo;
    CHECK_FPCR_MVI(ifi, pthread);
    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
    
OP(pmull_f)
{
    float ra_hi, ra_lo, rb_hi, rb_lo;
    float rc_hi, rc_lo;
    float *rcv_hi = (float *)&SPFP_HI(RC);
    float *rcv_lo = (float *)&SPFP_LO(RC);

    ra_hi = SPFP_HI(RA); ra_lo = SPFP_LO(RA);
    rb_hi = SPFP_HI(RB); rb_lo = SPFP_LO(RB);
    rc_hi = (float)(ra_hi * rb_lo);
    rc_lo = (float)(ra_lo * rb_lo);

    *rcv_hi = rc_hi; *rcv_lo = rc_lo;
    CHECK_FPCR_MVI(ifi, pthread);
    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
    
OP(pmulls_f)
{
    float ra_hi, ra_lo, rb_hi, rb_lo;
    float rc_hi, rc_lo;
    float *rcv_hi = (float *)&SPFP_HI(RC);
    float *rcv_lo = (float *)&SPFP_LO(RC);

    ra_hi = SPFP_HI(RA); ra_lo = SPFP_LO(RA);
    rb_hi = SPFP_HI(RB); rb_lo = SPFP_LO(RB);

    rc_hi = (float)(ra_hi * rb_lo);
    rc_lo = (float)(-ra_lo * rb_lo);

    *rcv_hi = rc_hi; *rcv_lo = rc_lo;
    CHECK_FPCR_MVI(ifi, pthread);
    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
    
OP(pmulh_f)
{
    float ra_hi, ra_lo, rb_hi, rb_lo;
    float rc_hi, rc_lo;
    float *rcv_hi = (float *)&SPFP_HI(RC);
    float *rcv_lo = (float *)&SPFP_LO(RC);

    ra_hi = SPFP_HI(RA); ra_lo = SPFP_LO(RA);
    rb_hi = SPFP_HI(RB); rb_lo = SPFP_LO(RB);

    rc_hi = (float)(ra_hi * rb_hi);
    rc_lo = (float)(ra_lo * rb_hi);

    *rcv_hi = rc_hi; *rcv_lo = rc_lo;
    CHECK_FPCR_MVI(ifi, pthread);
    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
    
OP(pmulhs_f)
{
    float ra_hi, ra_lo, rb_hi, rb_lo;
    float rc_hi, rc_lo;
    float *rcv_hi = (float *)&SPFP_HI(RC);
    float *rcv_lo = (float *)&SPFP_LO(RC);

    ra_hi = SPFP_HI(RA); ra_lo = SPFP_LO(RA);
    rb_hi = SPFP_HI(RB); rb_lo = SPFP_LO(RB);

    rc_hi = (float)(ra_hi * rb_hi);
    rc_lo = (float)(-ra_lo * rb_hi);

    *rcv_hi = rc_hi; *rcv_lo = rc_lo;
    CHECK_FPCR_MVI(ifi, pthread);
    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
    
OP(parcpl_f)
{
    float rb_hi, rb_lo;
    float rc_hi, rc_lo;
    float *rcv_hi = (float *)&SPFP_HI(RC);
    float *rcv_lo = (float *)&SPFP_LO(RC);

    rb_hi = SPFP_HI(RB); rb_lo = SPFP_LO(RB);
    
    if (rb_hi == 0)
	pthread->fpcr |= ((1L << 53) | (1L << 63));
    else
	rc_hi = (float)(1.0 / rb_hi);

    if (rb_lo == 0)
	pthread->fpcr |= ((1L << 53) | (1L << 63));
    else    
	rc_lo = (float)(1.0 / rb_lo);

    *rcv_hi = rc_hi; *rcv_lo = rc_lo;
    CHECK_FPCR_MVI(ifi, pthread);
    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
    
OP(parcplh_f)
{
    float rb_hi, rb_lo;
    float rc_hi, rc_lo;
    float *rcv_hi = (float *)&SPFP_HI(RC);
    float *rcv_lo = (float *)&SPFP_LO(RC);

    rb_hi = SPFP_HI(RB); rb_lo = SPFP_LO(RB);
    
    if (rb_hi == 0)
	pthread->fpcr |= ((1L << 53) | (1L << 63));
    else
	rc_hi = (float)(1.0 / rb_hi);

    rc_lo = 0;

    *rcv_hi = rc_hi; *rcv_lo = rc_lo;
    CHECK_FPCR_MVI(ifi, pthread);
    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
    
OP(parcpll_f)
{
    float rb_hi, rb_lo;
    float rc_hi, rc_lo;
    float *rcv_hi = (float *)&SPFP_HI(RC);
    float *rcv_lo = (float *)&SPFP_LO(RC);

    rb_hi = SPFP_HI(RB); rb_lo = SPFP_LO(RB);
    
    rc_hi = 0;

    if (rb_lo == 0)
	pthread->fpcr |= ((1L << 53) | (1L << 63));
    else    
	rc_lo = (float)(1.0 / rb_lo);

    *rcv_hi = rc_hi; *rcv_lo = rc_lo;
    CHECK_FPCR_MVI(ifi, pthread);
    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
    
OP(parsqrt_f)
{
    float rb_hi, rb_lo;
    float rc_hi, rc_lo;
    float *rcv_hi = (float *)&SPFP_HI(RC);
    float *rcv_lo = (float *)&SPFP_LO(RC);

    rb_hi = SPFP_HI(RB); rb_lo = SPFP_LO(RB);
    
    if (rb_hi == 0)
	pthread->fpcr |= ((1L << 53) | (1L << 63));
    else {
	if (rb_hi < 0)
	    pthread->fpcr |= (1L << 52);
	else	
	    rc_hi = (float)(1.0 / (float)sqrtf(rb_hi));
    }

    if (rb_lo == 0)
	pthread->fpcr |= ((1L << 53) | (1L << 63));
    else {
	if (rb_lo < 0)
	    pthread->fpcr |= (1L << 52);
	else   
	    rc_lo = (float)(1.0 / (float)sqrtf(rb_lo));
    }
    *rcv_hi = rc_hi; *rcv_lo = rc_lo;
    CHECK_FPCR_MVI(ifi, pthread);
    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
 
OP(parsqrth_f)
{
    float rb_hi, rb_lo;
    float rc_hi, rc_lo;
    float *rcv_hi = (float *)&SPFP_HI(RC);
    float *rcv_lo = (float *)&SPFP_LO(RC);

    rb_hi = SPFP_HI(RB); rb_lo = SPFP_LO(RB);
    
    if (rb_hi == 0)
	pthread->fpcr |= ((1L << 53) | (1L << 63));
    else {
	if (rb_hi < 0)
	    pthread->fpcr |= (1L << 52);
	else	
	    rc_hi = (float)(1.0 / (float)sqrtf(rb_hi));
    }

    rc_lo = 0;

    *rcv_hi = rc_hi; *rcv_lo = rc_lo;
    CHECK_FPCR_MVI(ifi, pthread);
    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
 
OP(parsqrtl_f)
{
    float rb_hi, rb_lo;
    float rc_hi, rc_lo;
    float *rcv_hi = (float *)&SPFP_HI(RC);
    float *rcv_lo = (float *)&SPFP_LO(RC);

    rb_hi = SPFP_HI(RB); rb_lo = SPFP_LO(RB);
    
    rc_hi = 0;

    if (rb_lo == 0)
	pthread->fpcr |= ((1L << 53) | (1L << 63));
    else {
	if (rb_lo < 0)
	    pthread->fpcr |= (1L << 52);
	else   
	    rc_lo = (float)(1.0 / (float)sqrtf(rb_lo));
    }
    *rcv_hi = rc_hi; *rcv_lo = rc_lo;
    CHECK_FPCR_MVI(ifi, pthread);
    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
 
OP(pmovll_f)
{
    int ra_hi, ra_lo, rb_hi, rb_lo;
    int rc_hi, rc_lo;
    int *rcv_hi = (int *)&SPHEX_HI(RC);
    int *rcv_lo = (int *)&SPHEX_LO(RC);


    *rcv_hi = SPHEX_LO(RA); 
    *rcv_lo = SPHEX_LO(RB);

    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
 
OP(pmovlh_f)
{
    int ra_hi, ra_lo, rb_hi, rb_lo;
    int rc_hi, rc_lo;
    int *rcv_hi = (int *)&SPHEX_HI(RC);
    int *rcv_lo = (int *)&SPHEX_LO(RC);


    *rcv_hi = SPHEX_LO(RA); 
    *rcv_lo = SPHEX_HI(RB);

    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
 
OP(pmovhl_f)
{
    int ra_hi, ra_lo, rb_hi, rb_lo;
    int rc_hi, rc_lo;
    int *rcv_hi = (int *)&SPHEX_HI(RC);
    int *rcv_lo = (int *)&SPHEX_LO(RC);


    *rcv_hi = SPHEX_HI(RA); 
    *rcv_lo = SPHEX_LO(RB);

    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
 
OP(pmovhh_f)
{
    int ra_hi, ra_lo, rb_hi, rb_lo;
    int rc_hi, rc_lo;
    int *rcv_hi = (int *)&SPHEX_HI(RC);
    int *rcv_lo = (int *)&SPHEX_LO(RC);


    *rcv_hi = SPHEX_HI(RA); 
    *rcv_lo = SPHEX_HI(RB);

    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
 
OP(pcpys_f)
{
    int ra_hi, ra_lo, rb_hi, rb_lo;
    int rc_hi, rc_lo;
    int *rcv_hi = (int *)&SPHEX_HI(RC);
    int *rcv_lo = (int *)&SPHEX_LO(RC);

    ra_hi = SPHEX_HI(RA); ra_lo = SPHEX_LO(RA);
    rb_hi = SPHEX_HI(RB); rb_lo = SPHEX_LO(RB);
    
    rc_hi = (ra_hi & 0x80000000) | (rb_hi & 0x7FFFFFFF);
    rc_lo = (ra_lo & 0x80000000) | (rb_lo & 0x7FFFFFFF);
	

    *rcv_hi = rc_hi; *rcv_lo = rc_lo;
    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
 
OP(pcpysn_f)
{
    int ra_hi, ra_lo, rb_hi, rb_lo;
    int rc_hi, rc_lo;
    int *rcv_hi = (int *)&SPHEX_HI(RC);
    int *rcv_lo = (int *)&SPHEX_LO(RC);

    ra_hi = SPHEX_HI(RA); ra_lo = SPHEX_LO(RA);
    rb_hi = SPHEX_HI(RB); rb_lo = SPHEX_LO(RB);
    
    rc_hi = ((ra_hi ^ 0x80000000) & 0x80000000) | (rb_hi & 0x7FFFFFFF);
    rc_lo = ((ra_lo ^ 0x80000000) & 0x80000000) | (rb_lo & 0x7FFFFFFF);
	

    *rcv_hi = rc_hi; *rcv_lo = rc_lo;

    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
 
OP(pcpyse_f)
{
    int ra_hi, ra_lo, rb_hi, rb_lo;
    int rc_hi, rc_lo;
    int *rcv_hi = (int *)&SPHEX_HI(RC);
    int *rcv_lo = (int *)&SPHEX_LO(RC);

    ra_hi = SPHEX_HI(RA); ra_lo = SPHEX_LO(RA);
    rb_hi = SPHEX_HI(RB); rb_lo = SPHEX_LO(RB);
    
    rc_hi = (ra_hi & 0xFF800000) | (rb_hi & 0x007FFFFF);
    rc_lo = (ra_lo & 0xFF800000) | (rb_lo & 0x007FFFFF);
	

    *rcv_hi = rc_hi; *rcv_lo = rc_lo;

    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}

OP(pcmpeq_f)
{
    float ra_hi, ra_lo, rb_hi, rb_lo;
    int rc_hi, rc_lo;
    int *rcv_hi = (int *)&SPHEX_HI(RC);
    int *rcv_lo = (int *)&SPHEX_LO(RC);

    ra_hi = SPFP_HI(RA); ra_lo = SPFP_LO(RA);
    rb_hi = SPFP_HI(RB); rb_lo = SPFP_LO(RB);

    if (ra_hi == rb_hi)
	rc_hi = 0x40000000;
    else
	rc_hi = 0;

    if (ra_lo == rb_lo)
	rc_lo = 0x40000000;
    else
	rc_lo = 0;


    *rcv_hi = rc_hi; *rcv_lo = rc_lo;

    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
    
OP(pcmpne_f)
{
    float ra_hi, ra_lo, rb_hi, rb_lo;
    int rc_hi, rc_lo;
    int *rcv_hi = (int *)&SPHEX_HI(RC);
    int *rcv_lo = (int *)&SPHEX_LO(RC);

    ra_hi = SPFP_HI(RA); ra_lo = SPFP_LO(RA);
    rb_hi = SPFP_HI(RB); rb_lo = SPFP_LO(RB);

    if (ra_hi != rb_hi)
	rc_hi = 0x40000000;
    else
	rc_hi = 0;

    if (ra_lo != rb_lo)
	rc_lo = 0x40000000;
    else
	rc_lo = 0;


    *rcv_hi = rc_hi; *rcv_lo = rc_lo;

    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
    
OP(pcmplt_f)
{
    register double test_a = FP(RA);
    register double test_b = FP(RB);
    float ra_hi, ra_lo, rb_hi, rb_lo;
    int rc_hi, rc_lo;
    int *rcv_hi = (int *)&SPHEX_HI(RC);
    int *rcv_lo = (int *)&SPHEX_LO(RC);

    ra_hi = SPFP_HI(RA); ra_lo = SPFP_LO(RA);
    rb_hi = SPFP_HI(RB); rb_lo = SPFP_LO(RB);

    if (ra_hi < rb_hi)
	rc_hi = 0x40000000;
    else
	rc_hi = 0;

    if (ra_lo < rb_lo)
	rc_lo = 0x40000000;
    else
	rc_lo = 0;

 
    *rcv_hi = rc_hi; *rcv_lo = rc_lo;
    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
} 
    
OP(pcmple_f)
{
    float ra_hi, ra_lo, rb_hi, rb_lo;
    int rc_hi, rc_lo;
    int *rcv_hi = (int *)&SPHEX_HI(RC);
    int *rcv_lo = (int *)&SPHEX_LO(RC);

    ra_hi = SPFP_HI(RA); ra_lo = SPFP_LO(RA);
    rb_hi = SPFP_HI(RB); rb_lo = SPFP_LO(RB);

    if (ra_hi <= rb_hi)
	rc_hi = 0x40000000;
    else
	rc_hi = 0;

    if (ra_lo <= rb_lo)
	rc_lo = 0x40000000;
    else
	rc_lo = 0;


    *rcv_hi = rc_hi; *rcv_lo = rc_lo;

    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
OP(pcmpun_f)
{
    int ra_hi, ra_lo, rb_hi, rb_lo;
    int rc_hi, rc_lo;
    int *rcv_hi = (int *)&SPHEX_HI(RC);
    int *rcv_lo = (int *)&SPHEX_LO(RC);

    ra_hi = SPHEX_HI(RA); ra_lo = SPHEX_LO(RA);
    rb_hi = SPHEX_HI(RB); rb_lo = SPHEX_LO(RB);

    if (SP_NaN(ra_hi) || SP_NaN(rb_hi))
	rc_hi = 0x40000000;
    else
	rc_hi = 0;

    if (SP_NaN(ra_lo) || SP_NaN(rb_lo))
	rc_lo = 0x40000000;
    else
	rc_lo = 0;


    *rcv_hi = rc_hi; *rcv_lo = rc_lo;

    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
    
OP(pfmax_f)
{
    float ra_hi, ra_lo, rb_hi, rb_lo;
    float rc_hi, rc_lo;
    float *rcv_hi = (float *)&SPFP_HI(RC);
    float *rcv_lo = (float *)&SPFP_LO(RC);
    
    ra_hi = SPFP_HI(RA); ra_lo = SPFP_LO(RA);
    rb_hi = SPFP_HI(RB); rb_lo = SPFP_LO(RB);
    rc_hi = ra_hi;
    rc_lo = ra_lo;
 
    if (ra_hi < rb_hi)
	rc_hi = rb_hi;
    
    if (ra_lo < rb_lo)
	rc_lo = rb_lo; 

    *rcv_hi = rc_hi; *rcv_lo = rc_lo;
    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
    
OP(pfmin_f)
{
    float ra_hi, ra_lo, rb_hi, rb_lo;
    float rc_hi, rc_lo;
    float *rcv_hi = (float *)&SPFP_HI(RC);
    float *rcv_lo = (float *)&SPFP_LO(RC);

    ra_hi = SPFP_HI(RA); ra_lo = SPFP_LO(RA);
    rb_hi = SPFP_HI(RB); rb_lo = SPFP_LO(RB);

    rc_hi = ra_hi;
    if (ra_hi > rb_hi)
	rc_hi = rb_hi;
    
    rc_lo = ra_lo;
    if (ra_lo > rb_lo)
	rc_lo = rb_lo;
    

    *rcv_hi = rc_hi; *rcv_lo = rc_lo;
    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
    
OP(pcvtsp_f)
{
    int ra_hi, ra_lo, rb_hi, rb_lo, exp, sign, frac;
    int rc_hi, rc_lo;
    int *rcv_hi = (int *)&SPHEX_HI(RC);
    int *rcv_lo = (int *)&SPHEX_LO(RC);

    ra_hi = SPHEX_HI(RA); ra_lo = SPHEX_LO(RA);
    rb_hi = SPHEX_HI(RB); rb_lo = SPHEX_LO(RB);

    /* convert  64b SP of ra to 32bSP of rc_hi */
    sign = ra_hi & 0x80000000;
    frac = ((ra_hi & 0xFFFFF) << 3) | ((ra_lo & 0xE0000000) >> 29);
    exp  = (ra_hi >> 20) & 0x7FF;
    if (exp & 0x400)
	exp = (exp & 0x7F) | 0x80;
    else
	exp = exp & 0x7F;
    rc_hi = sign | (exp << 23) | frac;

    /* convert  64b SP of rb to 32bSP of rc_lo */
    sign = rb_hi & 0x80000000;
    frac = ((rb_hi & 0xFFFFF) << 3) | ((rb_lo & 0xE0000000) >> 29);
    exp  = (rb_hi >> 20) & 0x7FF;
    if (exp & 0x400)
	exp = (exp & 0x7F) | 0x80;
    else
	exp = exp & 0x7F;
    rc_lo = sign | (exp << 23) | frac;


    *rcv_hi = rc_hi; *rcv_lo = rc_lo;

    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
     
OP(pextl_f)
{
    int ra_lo;
    int rc_hi, rc_lo, frac, exp, sign;
    int *rcv_hi = (int *)&SPHEX_HI(RC);
    int *rcv_lo = (int *)&SPHEX_LO(RC);

    ra_lo = (int)SPHEX_LO(RA);

    rc_lo = 0;
    rc_hi = 0;

    frac  = (ra_lo & 0x7FFFFF);
    rc_lo = (frac & 7) << 29;
    ra_lo >>= 23;
    exp   = ra_lo & 0xFF;
    sign  = (ra_lo >> 8) & 1;

    if ((exp & 0xFF) == 0xFF)
	exp = 0x7FF;
    else {
	if (exp & 0x80)
	    exp = (exp & 0x7F) | 0x400;
	else {
	    if (exp == 0)
		exp = 0;
	    else
		exp = (exp & 0x7F) | 0x380;
	}
    }
    rc_hi = (sign << 31) | (exp << 20) | (frac >> 3);

 
    *rcv_hi = rc_hi; *rcv_lo = rc_lo;

    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
     
OP(pexth_f)
{
    int rb_hi;
    int rc_hi, rc_lo, frac, exp, sign;
    int *rcv_hi = (int *)&SPHEX_HI(RC);
    int *rcv_lo = (int *)&SPHEX_LO(RC);

    rb_hi = (int)SPHEX_HI(RB);

    rc_lo = 0;
    rc_hi = 0;

    frac  = (rb_hi & 0x7FFFFF);
    rc_lo = (frac & 7) << 29;
    rb_hi >>= 23;
    exp   = rb_hi & 0xFF;
    sign  = (rb_hi >> 8) & 1;

    if ((exp & 0xFF) == 0xFF)
	exp = 0x7FF;
    else {
	if (exp & 0x80)
	    exp = (exp & 0x7F) | 0x400;
	else {
	    if (exp == 0)
		exp = 0;
	    else
		exp = (exp & 0x7F) | 0x380;
	}
    }
    rc_hi = (sign << 31) | (exp << 20) | (frac >> 3);

 
    *rcv_hi = rc_hi; *rcv_lo = rc_lo;

    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}

OP(pcvtfi_f)
{
    float rb_hi, rb_lo;
    int rc_hi, rc_lo;
    int *rcv_hi = (int *)&SPHEX_HI(RC);
    int *rcv_lo = (int *)&SPHEX_LO(RC);

    rb_hi = SPFP_HI(RB); 
    rb_lo = SPFP_LO(RB);

    rb_hi = truncf(rb_hi);
    rb_lo = truncf(rb_lo);

    if ((rb_hi > (0xFFFFFFFF * 1.0)) || (rb_hi < (0xFFFFFFFF * -1.0)))
	*rcv_hi = 0xFFFFFFFF;
    else 
	*rcv_hi = (int)rb_hi;

    if ((rb_lo > (0xFFFFFFFF * 1.0)) || (rb_hi < (0xFFFFFFFF * -1.0)))
	*rcv_lo = 0xFFFFFFFF;
    else 
	*rcv_lo = (int)rb_lo;

    MapFP(ZERO_REGISTER) = 0.0;
    return picode->next;
}
           
#endif

