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

// ASIM core
#include "asim/syntax.h"

// ASIM local module
#include "disasm_test.h"
#if defined (HOST_LINUX) || defined (HOST_LINUX_X86) || defined(HOST_FREEBSD) || defined(HOST_FREEBSD_X86) || defined (HOST_LINUX_IA64)
#include "pal.h"
#else
#include <alpha/pal.h>
#endif

#define PAL_OS_OSF 1
#define PAL_OS_NT 2
#ifndef PAL_OS
#define PAL_OS 1	/* CALL_PAL disassembly:  0=none, 1=OSF pal calls, 2=NT pal calls */
#endif

static char *alpha_int_regs[32] =
{
  "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",  "r8",  "r9",
  "r10", "r11", "r12", "r13", "r14", "r15", "r16", "r17", "r18", "r19",
  "r20", "r21", "r22", "r23", "r24", "r25", "r26", "r27", "r28", "r29",
  "r30", "r31"
};

static char *alpha_fp_regs[32] =
{
  "f0",  "f1",  "f2",  "f3",  "f4",  "f5",  "f6",  "f7",  "f8",  "f9",
  "f10", "f11", "f12", "f13", "f14", "f15", "f16", "f17", "f18", "f19",
  "f20", "f21", "f22", "f23", "f24", "f25", "f26", "f27", "f28", "f29",
  "f30", "f31"
};

static alpha_insn alpha_insn_set[] =  
{

/* Reserved opcodes */
RESERVED_FORMAT(0x01,"resdec01"),
RESERVED_FORMAT(0x02,"resdec02"),
RESERVED_FORMAT(0x03,"resdec03"),
RESERVED_FORMAT(0x04,"resdec04"),
RESERVED_FORMAT(0x05,"resdec05"),
RESERVED_FORMAT(0x06,"resdec06"),
RESERVED_FORMAT(0x07,"resdec07"),
RESERVED_FORMAT(0x19,"hw_mfpr"),
RESERVED_FORMAT(0x1b,"hw_ld"),
RESERVED_FORMAT(0x1d,"hw_mtpr"),
RESERVED_FORMAT(0x1e,"hw_rei"),
RESERVED_FORMAT(0x1f,"hw_st"),

/* Memory format instruction opcodes */
MEMORY_FORMAT(0x08,"lda"),
MEMORY_FLOAT_FORMAT(0x21,"ldg"),
MEMORY_FORMAT(0x29,"ldq"),
MEMORY_FLOAT_FORMAT(0x22,"lds"),
MEMORY_FLOAT_FORMAT(0x25,"stg"),
MEMORY_FORMAT(0x2d,"stq"),
MEMORY_FLOAT_FORMAT(0x26,"sts"),
MEMORY_FORMAT(0x09,"ldah"),
MEMORY_FORMAT(0x28,"ldl"),
MEMORY_FORMAT(0x2b,"ldq_l"),
MEMORY_FLOAT_FORMAT(0x23,"ldt"),
MEMORY_FORMAT(0x2c,"stl"),
MEMORY_FORMAT(0x2f,"stq_c"),
MEMORY_FLOAT_FORMAT(0x27,"stt"),
MEMORY_FLOAT_FORMAT(0x20,"ldf"),
MEMORY_FORMAT(0x2a,"ldl_l"),
MEMORY_FORMAT(0x0b,"ldq_u"),
MEMORY_FLOAT_FORMAT(0x24,"stf"),
MEMORY_FORMAT(0x2e,"stl_c"),
MEMORY_FORMAT(0x0a,"ldb"),
MEMORY_FORMAT(0x0c,"ldw"),
MEMORY_FORMAT(0x0d,"stw"),
MEMORY_FORMAT(0x0e,"stb"),
MEMORY_FORMAT(0x0f,"stq_u"),
/* Memory format instructions with a function code */
MEMORY_FORMAT_FUNCTION(0x18, 0x0000,"trapb"),
MEMORY_FORMAT_FUNCTION(0x18, 0x0400,"excb"),
MEMORY_FORMAT_FUNCTION(0x18, 0x4000,"mb"),
MEMORY_FORMAT_FUNCTION(0x18, 0x4400,"wmb"),
MEMORY_FORMAT_FUNCTION(0x18, 0x8000,"fetch"),
MEMORY_FORMAT_FUNCTION(0x18, 0xa000,"fetch_m"),
MEMORY_FORMAT_FUNCTION(0x18, 0xc000,"rpcc"),
MEMORY_FORMAT_FUNCTION(0x18, 0xe000,"rc"),
MEMORY_FORMAT_FUNCTION(0x18, 0xe800,"ecb"),
MEMORY_FORMAT_FUNCTION(0x18, 0xf000,"rs"),
MEMORY_FORMAT_FUNCTION(0x18, 0xf800,"wh64"),
MEMORY_FORMAT_FUNCTION(0x18, 0xfc00,"wh64en"),

MEMORY_BRANCH_FORMAT(0x1a, 0x0, "jmp"),
MEMORY_BRANCH_FORMAT(0x1a, 0x2, "ret"),
MEMORY_BRANCH_FORMAT(0x1a, 0x1, "jsr"),
MEMORY_BRANCH_FORMAT(0x1a, 0x3, "jsr_coroutine"),
/***	Branch Instr		***/
BRANCH_FORMAT(0x30,"br"),
BRANCH_FORMAT(0x39,"beq"),
BRANCH_FORMAT(0x3c,"blbs"),
BRANCH_FORMAT(0x3f,"bgt"),
BRANCH_FORMAT(0x34,"bsr"),
BRANCH_FORMAT(0x3a,"blt"),
BRANCH_FORMAT(0x3d,"bne"),
BRANCH_FORMAT(0x38,"blbc"),
BRANCH_FORMAT(0x3b,"ble"),
BRANCH_FORMAT(0x3e,"bge"),
/***	FP Branch Instr		***/
FP_BRANCH_FORMAT(0x33,"fble"),
FP_BRANCH_FORMAT(0x36,"fbge"),
FP_BRANCH_FORMAT(0x31,"fbeq"),
FP_BRANCH_FORMAT(0x37,"fbgt"),
FP_BRANCH_FORMAT(0x32,"fblt"),
FP_BRANCH_FORMAT(0x35,"fbne"),
/***	Integer Int/Quad Instr	***/
OPERATE_FORMAT(0x10,0x00,"addl"),
OPERATE_FORMAT(0x10,0x02,"s4addl"),
OPERATE_FORMAT(0x10,0x09,"subl"),
OPERATE_FORMAT(0x10,0x0b,"s4subl"),
OPERATE_FORMAT(0x10,0x0f,"cmpbge"),
OPERATE_FORMAT(0x10,0x12,"s8addl"),
OPERATE_FORMAT(0x10,0x1b,"s8subl"),
OPERATE_FORMAT(0x10,0x1d,"cmpult"),
OPERATE_FORMAT(0x10,0x20,"addq"),
OPERATE_FORMAT(0x10,0x22,"s4addq"),
OPERATE_FORMAT(0x10,0x29,"subq"),
OPERATE_FORMAT(0x10,0x2b,"s4subq"),
OPERATE_FORMAT(0x10,0x2d,"cmpeq"),
OPERATE_FORMAT(0x10,0x32,"s8addq"),
OPERATE_FORMAT(0x10,0x3b,"s8subq"),
OPERATE_FORMAT(0x10,0x3d,"cmpule"),
OPERATE_FORMAT(0x10,0x40,"addl/v"),
OPERATE_FORMAT(0x10,0x49,"subl/v"),
OPERATE_FORMAT(0x10,0x4d,"cmplt"),
OPERATE_FORMAT(0x10,0x60,"addq/v"),
OPERATE_FORMAT(0x10,0x69,"subq/v"),
OPERATE_FORMAT(0x10,0x6d,"cmple"),
OPERATE_FORMAT(0x11,0x00,"and"),
OPERATE_FORMAT(0x11,0x08,"bic"),
OPERATE_FORMAT(0x11,0x14,"cmovlbs"),
OPERATE_FORMAT(0x11,0x16,"cmovlbc"),
OPERATE_FORMAT(0x11,0x20,"bis"),
OPERATE_FORMAT(0x11,0x24,"cmoveq"),
OPERATE_FORMAT(0x11,0x26,"cmovne"),
OPERATE_FORMAT(0x11,0x28,"ornot"),
OPERATE_FORMAT(0x11,0x40,"xor"),
OPERATE_FORMAT(0x11,0x44,"cmovlt"),
OPERATE_FORMAT(0x11,0x46,"cmovge"),
OPERATE_FORMAT(0x11,0x48,"eqv"),
OPERATE_FORMAT(0x11,0x61,"amask"),
OPERATE_FORMAT(0x11,0x64,"cmovle"),
OPERATE_FORMAT(0x11,0x66,"cmovgt"),
OPERATE_FORMAT(0x11,0x6c,"implver"),
OPERATE_FORMAT(0x12,0x02,"mskbl"),
OPERATE_FORMAT(0x12,0x06,"extbl"),
OPERATE_FORMAT(0x12,0x0b,"insbl"),
OPERATE_FORMAT(0x12,0x12,"mskwl"),
OPERATE_FORMAT(0x12,0x16,"extwl"),
OPERATE_FORMAT(0x12,0x1b,"inswl"),
OPERATE_FORMAT(0x12,0x22,"mskll"),
OPERATE_FORMAT(0x12,0x26,"extll"),
OPERATE_FORMAT(0x12,0x2b,"insll"),
OPERATE_FORMAT(0x12,0x30,"zap"),
OPERATE_FORMAT(0x12,0x31,"zapnot"),
OPERATE_FORMAT(0x12,0x32,"mskql"),
OPERATE_FORMAT(0x12,0x34,"srl"),
OPERATE_FORMAT(0x12,0x36,"extql"),
OPERATE_FORMAT(0x12,0x39,"sll"),
OPERATE_FORMAT(0x12,0x3b,"insql"),
OPERATE_FORMAT(0x12,0x3c,"sra"),
OPERATE_FORMAT(0x12,0x52,"mskwh"),
OPERATE_FORMAT(0x12,0x57,"inswh"),
OPERATE_FORMAT(0x12,0x5a,"extwh"),
OPERATE_FORMAT(0x12,0x62,"msklh"),
OPERATE_FORMAT(0x12,0x67,"inslh"),
OPERATE_FORMAT(0x12,0x6a,"extlh"),
OPERATE_FORMAT(0x12,0x72,"mskqh"),
OPERATE_FORMAT(0x12,0x77,"insqh"),
OPERATE_FORMAT(0x12,0x7a,"extqh"),
OPERATE_FORMAT(0x13,0x00,"mull"),
OPERATE_FORMAT(0x13,0x20,"mulq"),
OPERATE_FORMAT(0x13,0x30,"umulh"),
OPERATE_FORMAT(0x13,0x40,"mull/v"),
OPERATE_FORMAT(0x13,0x60,"mulq/v"),

OPERATE_FORMAT(0x1c,0x00,"sextb"),
OPERATE_FORMAT(0x1c,0x01,"sextw"),
OPERATE_FORMAT(0x1c,0x30,"ctpop"),
OPERATE_FORMAT(0x1c,0x31,"perr"),
OPERATE_FORMAT(0x1c,0x32,"ctlz"),
OPERATE_FORMAT(0x1c,0x33,"cttz"),
OPERATE_FORMAT(0x1c,0x34,"unpkbw"),
OPERATE_FORMAT(0x1c,0x35,"unpkbl"),
OPERATE_FORMAT(0x1c,0x36,"pkwb"),
OPERATE_FORMAT(0x1c,0x37,"pklb"),
OPERATE_FORMAT(0x1c,0x38,"minsb8"),
OPERATE_FORMAT(0x1c,0x39,"minsw4"),
OPERATE_FORMAT(0x1c,0x3a,"minub8"),
OPERATE_FORMAT(0x1c,0x3b,"minuw4"),
OPERATE_FORMAT(0x1c,0x3c,"maxub8"),
OPERATE_FORMAT(0x1c,0x3d,"maxuw4"),
OPERATE_FORMAT(0x1c,0x3e,"maxsb8"),
OPERATE_FORMAT(0x1c,0x3f,"maxsw4"),
OPERATE_FORMAT(0x1c,0x70,"ftoit"),
OPERATE_FORMAT(0x1c,0x78,"ftois"),


FLOAT_FORMAT(0x15,0x000,"addf/c"),
FLOAT_FORMAT(0x15,0x001,"subf/c"),
FLOAT_FORMAT(0x15,0x002,"mulf/c"),
FLOAT_FORMAT(0x15,0x003,"divf/c"),
FLOAT_FORMAT(0x15,0x020,"addg/c"),
FLOAT_FORMAT(0x15,0x021,"subg/c"),
FLOAT_FORMAT(0x15,0x022,"mulg/c"),
FLOAT_FORMAT(0x15,0x023,"divg/c"),
FLOAT_FORMAT(0x15,0x080,"addf"),
FLOAT_FORMAT(0x15,0x081,"subf"),
FLOAT_FORMAT(0x15,0x082,"mulf"),
FLOAT_FORMAT(0x15,0x083,"divf"),
FLOAT_FORMAT(0x15,0x0a0,"addg"),
FLOAT_FORMAT(0x15,0x0a1,"subg"),
FLOAT_FORMAT(0x15,0x0a2,"mulg"),
FLOAT_FORMAT(0x15,0x0a3,"divg"),
FLOAT_FORMAT(0x15,0x0a5,"cmpgeq"),
FLOAT_FORMAT(0x15,0x04a5,"cmpgeq/s"),
FLOAT_FORMAT(0x15,0x0a6,"cmpglt"),
FLOAT_FORMAT(0x15,0x04a6,"cmpglt/s"),
FLOAT_FORMAT(0x15,0x0a7,"cmpgle"),
FLOAT_FORMAT(0x15,0x04a7,"cmpgle/s"),
FLOAT_FORMAT(0x15,0x100,"addf/uc"),
FLOAT_FORMAT(0x15,0x101,"subf/uc"),
FLOAT_FORMAT(0x15,0x102,"mulf/uc"),
FLOAT_FORMAT(0x15,0x103,"divf/uc"),
FLOAT_FORMAT(0x15,0x120,"addg/uc"),
FLOAT_FORMAT(0x15,0x121,"subg/uc"),
FLOAT_FORMAT(0x15,0x122,"mulg/uc"),
FLOAT_FORMAT(0x15,0x123,"divg/uc"),
FLOAT_FORMAT(0x15,0x180,"addf/u"),
FLOAT_FORMAT(0x15,0x181,"subf/u"),
FLOAT_FORMAT(0x15,0x182,"mulf/u"),
FLOAT_FORMAT(0x15,0x183,"divf/u"),
FLOAT_FORMAT(0x15,0x1a0,"addg/u"),
FLOAT_FORMAT(0x15,0x1a1,"subg/u"),
FLOAT_FORMAT(0x15,0x1a2,"mulg/u"),
FLOAT_FORMAT(0x15,0x1a3,"divg/u"),
FLOAT_FORMAT(0x15,0x400,"addf/sc"),
FLOAT_FORMAT(0x15,0x401,"subf/sc"),
FLOAT_FORMAT(0x15,0x402,"mulf/sc"),
FLOAT_FORMAT(0x15,0x403,"divf/sc"),
FLOAT_FORMAT(0x15,0x420,"addg/sc"),
FLOAT_FORMAT(0x15,0x421,"subg/sc"),
FLOAT_FORMAT(0x15,0x422,"mulg/sc"),
FLOAT_FORMAT(0x15,0x423,"divg/sc"),
FLOAT_FORMAT(0x15,0x480,"addf/s"),
FLOAT_FORMAT(0x15,0x481,"subf/s"),
FLOAT_FORMAT(0x15,0x482,"mulf/s"),
FLOAT_FORMAT(0x15,0x483,"divf/s"),
FLOAT_FORMAT(0x15,0x4a0,"addg/s"),
FLOAT_FORMAT(0x15,0x4a1,"subg/s"),
FLOAT_FORMAT(0x15,0x4a2,"mulg/s"),
FLOAT_FORMAT(0x15,0x4a3,"divg/s"),
FLOAT_FORMAT(0x15,0x4a5,"cmpgeq/s"),
FLOAT_FORMAT(0x15,0x4a6,"cmpglt/s"),
FLOAT_FORMAT(0x15,0x4a7,"cmpgle/s"),
FLOAT_FORMAT(0x15,0x500,"addf/suc"),
FLOAT_FORMAT(0x15,0x501,"subf/suc"),
FLOAT_FORMAT(0x15,0x502,"mulf/suc"),
FLOAT_FORMAT(0x15,0x503,"divf/suc"),
FLOAT_FORMAT(0x15,0x520,"addg/suc"),
FLOAT_FORMAT(0x15,0x521,"subg/suc"),
FLOAT_FORMAT(0x15,0x522,"mulg/suc"),
FLOAT_FORMAT(0x15,0x523,"divg/suc"),
FLOAT_FORMAT(0x15,0x580,"addf/su"),
FLOAT_FORMAT(0x15,0x581,"subf/su"),
FLOAT_FORMAT(0x15,0x582,"mulf/su"),
FLOAT_FORMAT(0x15,0x583,"divf/su"),
FLOAT_FORMAT(0x15,0x5a0,"addg/su"),
FLOAT_FORMAT(0x15,0x5a1,"subg/su"),
FLOAT_FORMAT(0x15,0x5a2,"mulg/su"),
FLOAT_FORMAT(0x15,0x5a3,"divg/su"),

FLOAT_FORMAT(0x17,0x20,"cpys"),
FLOAT_FORMAT(0x17,0x21,"cpysn"),
FLOAT_FORMAT(0x17,0x22,"cpyse"),
FLOAT_FORMAT(0x17,0x24,"mt_fpcr"),
FLOAT_FORMAT(0x17,0x25,"mf_fpcr"),
FLOAT_FORMAT(0x17,0x2a,"fcmoveq"),
FLOAT_FORMAT(0x17,0x2b,"fcmovne"),
FLOAT_FORMAT(0x17,0x2c,"fcmovlt"),
FLOAT_FORMAT(0x17,0x2d,"fcmovge"),
FLOAT_FORMAT(0x17,0x2e,"fcmovle"),
FLOAT_FORMAT(0x17,0x2f,"fcmovgt"),

FLOAT_FORMAT(0x16,0x080,"adds"),
FLOAT_FORMAT(0x16,0x000,"adds/c"),
FLOAT_FORMAT(0x16,0x040,"adds/m"),
FLOAT_FORMAT(0x16,0x0c0,"adds/d"),
FLOAT_FORMAT(0x16,0x180,"adds/u"),
FLOAT_FORMAT(0x16,0x100,"adds/uc"),
FLOAT_FORMAT(0x16,0x140,"adds/um"),
FLOAT_FORMAT(0x16,0x1c0,"adds/ud"),
FLOAT_FORMAT(0x16,0x580,"adds/su"),
FLOAT_FORMAT(0x16,0x500,"adds/suc"),
FLOAT_FORMAT(0x16,0x540,"adds/sum"),
FLOAT_FORMAT(0x16,0x5c0,"adds/sud"),
FLOAT_FORMAT(0x16,0x780,"adds/sui"),
FLOAT_FORMAT(0x16,0x700,"adds/suic"),
FLOAT_FORMAT(0x16,0x740,"adds/suim"),
FLOAT_FORMAT(0x16,0x7c0,"adds/suid"),
FLOAT_FORMAT(0x16,0x0a0,"addt"),
FLOAT_FORMAT(0x16,0x020,"addt/c"),
FLOAT_FORMAT(0x16,0x060,"addt/m"),
FLOAT_FORMAT(0x16,0x0e0,"addt/d"),
FLOAT_FORMAT(0x16,0x1a0,"addt/u"),
FLOAT_FORMAT(0x16,0x120,"addt/uc"),
FLOAT_FORMAT(0x16,0x160,"addt/um"),
FLOAT_FORMAT(0x16,0x1e0,"addt/ud"),
FLOAT_FORMAT(0x16,0x5a0,"addt/su"),
FLOAT_FORMAT(0x16,0x520,"addt/suc"),
FLOAT_FORMAT(0x16,0x560,"addt/sum"),
FLOAT_FORMAT(0x16,0x5e0,"addt/sud"),
FLOAT_FORMAT(0x16,0x7a0,"addt/sui"),
FLOAT_FORMAT(0x16,0x720,"addt/suic"),
FLOAT_FORMAT(0x16,0x760,"addt/suim"),
FLOAT_FORMAT(0x16,0x7e0,"addt/suid"),

FLOAT_FORMAT(0x16,0x0a5,"cmpteq"),
FLOAT_FORMAT(0x16,0x5a5,"cmpteq/su"),
FLOAT_FORMAT(0x16,0x0a6,"cmptlt"),
FLOAT_FORMAT(0x16,0x5a6,"cmptlt/su"),
FLOAT_FORMAT(0x16,0x0a7,"cmptle"),
FLOAT_FORMAT(0x16,0x5a7,"cmptle/su"),
FLOAT_FORMAT(0x16,0x0a4,"cmptun"),
FLOAT_FORMAT(0x16,0x5a4,"cmptun/su"),
FLOAT_FORMAT(0x16,0x083,"divs"),
FLOAT_FORMAT(0x16,0x003,"divs/c"),
FLOAT_FORMAT(0x16,0x043,"divs/m"),
FLOAT_FORMAT(0x16,0x0c3,"divs/d"),
FLOAT_FORMAT(0x16,0x183,"divs/u"),
FLOAT_FORMAT(0x16,0x103,"divs/uc"),
FLOAT_FORMAT(0x16,0x143,"divs/um"),
FLOAT_FORMAT(0x16,0x1c3,"divs/ud"),
FLOAT_FORMAT(0x16,0x583,"divs/su"),
FLOAT_FORMAT(0x16,0x503,"divs/suc"),
FLOAT_FORMAT(0x16,0x543,"divs/sum"),
FLOAT_FORMAT(0x16,0x5c3,"divs/sud"),
FLOAT_FORMAT(0x16,0x783,"divs/sui"),
FLOAT_FORMAT(0x16,0x703,"divs/suic"),
FLOAT_FORMAT(0x16,0x743,"divs/suim"),
FLOAT_FORMAT(0x16,0x7c3,"divs/suid"),
FLOAT_FORMAT(0x16,0x0a3,"divt"),
FLOAT_FORMAT(0x16,0x023,"divt/c"),
FLOAT_FORMAT(0x16,0x063,"divt/m"),
FLOAT_FORMAT(0x16,0x0e3,"divt/d"),
FLOAT_FORMAT(0x16,0x1a3,"divt/u"),
FLOAT_FORMAT(0x16,0x123,"divt/uc"),
FLOAT_FORMAT(0x16,0x163,"divt/um"),
FLOAT_FORMAT(0x16,0x1e3,"divt/ud"),
FLOAT_FORMAT(0x16,0x5a3,"divt/su"),
FLOAT_FORMAT(0x16,0x523,"divt/suc"),
FLOAT_FORMAT(0x16,0x563,"divt/sum"),
FLOAT_FORMAT(0x16,0x5e3,"divt/sud"),
FLOAT_FORMAT(0x16,0x7a3,"divt/sui"),
FLOAT_FORMAT(0x16,0x723,"divt/suic"),
FLOAT_FORMAT(0x16,0x763,"divt/suim"),
FLOAT_FORMAT(0x16,0x7e3,"divt/suid"),
FLOAT_FORMAT(0x16,0x082,"muls"),
FLOAT_FORMAT(0x16,0x002,"muls/c"),
FLOAT_FORMAT(0x16,0x042,"muls/m"),
FLOAT_FORMAT(0x16,0x0c2,"muls/d"),
FLOAT_FORMAT(0x16,0x182,"muls/u"),
FLOAT_FORMAT(0x16,0x102,"muls/uc"),
FLOAT_FORMAT(0x16,0x142,"muls/um"),
FLOAT_FORMAT(0x16,0x1c2,"muls/ud"),
FLOAT_FORMAT(0x16,0x582,"muls/su"),
FLOAT_FORMAT(0x16,0x502,"muls/suc"),
FLOAT_FORMAT(0x16,0x542,"muls/sum"),
FLOAT_FORMAT(0x16,0x5c2,"muls/sud"),
FLOAT_FORMAT(0x16,0x782,"muls/sui"),
FLOAT_FORMAT(0x16,0x702,"muls/suic"),
FLOAT_FORMAT(0x16,0x742,"muls/suim"),
FLOAT_FORMAT(0x16,0x7c2,"muls/suid"),
FLOAT_FORMAT(0x16,0x0a2,"mult"),
FLOAT_FORMAT(0x16,0x022,"mult/c"),
FLOAT_FORMAT(0x16,0x062,"mult/m"),
FLOAT_FORMAT(0x16,0x0e2,"mult/d"),
FLOAT_FORMAT(0x16,0x1a2,"mult/u"),
FLOAT_FORMAT(0x16,0x122,"mult/uc"),
FLOAT_FORMAT(0x16,0x162,"mult/um"),
FLOAT_FORMAT(0x16,0x1e2,"mult/ud"),
FLOAT_FORMAT(0x16,0x5a2,"mult/su"),
FLOAT_FORMAT(0x16,0x522,"mult/suc"),
FLOAT_FORMAT(0x16,0x562,"mult/sum"),
FLOAT_FORMAT(0x16,0x5e2,"mult/sud"),
FLOAT_FORMAT(0x16,0x7a2,"mult/sui"),
FLOAT_FORMAT(0x16,0x722,"mult/suic"),
FLOAT_FORMAT(0x16,0x762,"mult/suim"),
FLOAT_FORMAT(0x16,0x7e2,"mult/suid"),
FLOAT_FORMAT(0x16,0x081,"subs"),
FLOAT_FORMAT(0x16,0x001,"subs/c"),
FLOAT_FORMAT(0x16,0x041,"subs/m"),
FLOAT_FORMAT(0x16,0x0c1,"subs/d"),
FLOAT_FORMAT(0x16,0x181,"subs/u"),
FLOAT_FORMAT(0x16,0x101,"subs/uc"),
FLOAT_FORMAT(0x16,0x141,"subs/um"),
FLOAT_FORMAT(0x16,0x1c1,"subs/ud"),
FLOAT_FORMAT(0x16,0x581,"subs/su"),
FLOAT_FORMAT(0x16,0x501,"subs/suc"),
FLOAT_FORMAT(0x16,0x541,"subs/sum"),
FLOAT_FORMAT(0x16,0x5c1,"subs/sud"),
FLOAT_FORMAT(0x16,0x781,"subs/sui"),
FLOAT_FORMAT(0x16,0x701,"subs/suic"),
FLOAT_FORMAT(0x16,0x741,"subs/suim"),
FLOAT_FORMAT(0x16,0x7c1,"subs/suid"),
FLOAT_FORMAT(0x16,0x0a1,"subt"),
FLOAT_FORMAT(0x16,0x021,"subt/c"),
FLOAT_FORMAT(0x16,0x061,"subt/m"),
FLOAT_FORMAT(0x16,0x0e1,"subt/d"),
FLOAT_FORMAT(0x16,0x1a1,"subt/u"),
FLOAT_FORMAT(0x16,0x121,"subt/uc"),
FLOAT_FORMAT(0x16,0x161,"subt/um"),
FLOAT_FORMAT(0x16,0x1e1,"subt/ud"),
FLOAT_FORMAT(0x16,0x5a1,"subt/su"),
FLOAT_FORMAT(0x16,0x521,"subt/suc"),
FLOAT_FORMAT(0x16,0x561,"subt/sum"),
FLOAT_FORMAT(0x16,0x5e1,"subt/sud"),
FLOAT_FORMAT(0x16,0x7a1,"subt/sui"),
FLOAT_FORMAT(0x16,0x721,"subt/suic"),
FLOAT_FORMAT(0x16,0x761,"subt/suim"),
FLOAT_FORMAT(0x16,0x7e1,"subt/suid"),

FLOAT_FORMAT(0x15,0x1e,"cvtdg/c"),
FLOAT_FORMAT(0x15,0x9e,"cvtdg"),
FLOAT_FORMAT(0x15,0x11e,"cvtdg/uc"),
FLOAT_FORMAT(0x15,0x19e,"cvtdg/u"),
FLOAT_FORMAT(0x15,0x41e,"cvtdg/sc"),
FLOAT_FORMAT(0x15,0x49e,"cvtdg/s"),
FLOAT_FORMAT(0x15,0x51e,"cvtdg/suc"),
FLOAT_FORMAT(0x15,0x59e,"cvtdg/su"),

FLOAT_FORMAT(0x15,0x2c,"cvtgf/c"),
FLOAT_FORMAT(0x15,0x2d,"cvtgd/c"),
FLOAT_FORMAT(0x15,0x2f,"cvtgq/c"),
FLOAT_FORMAT(0x15,0xac,"cvtgf"),
FLOAT_FORMAT(0x15,0xad,"cvtgd"),
FLOAT_FORMAT(0x15,0xaf,"cvtgq"),
FLOAT_FORMAT(0x15,0x12c,"cvtgf/uc"),
FLOAT_FORMAT(0x15,0x12d,"cvtgd/uc"),
FLOAT_FORMAT(0x15,0x12f,"cvtgq/vc"),
FLOAT_FORMAT(0x15,0x1ac,"cvtgf/u"),
FLOAT_FORMAT(0x15,0x1ad,"cvtgd/u"),
FLOAT_FORMAT(0x15,0x1af,"cvtgq/v"),
FLOAT_FORMAT(0x15,0x42c,"cvtgf/sc"),
FLOAT_FORMAT(0x15,0x42d,"cvtgd/sc"),
FLOAT_FORMAT(0x15,0x42f,"cvtgq/sc"),
FLOAT_FORMAT(0x15,0x4ac,"cvtgf/s"),
FLOAT_FORMAT(0x15,0x4ad,"cvtgd/s"),
FLOAT_FORMAT(0x15,0x04af,"cvtgq/s"),
FLOAT_FORMAT(0x15,0x52c,"cvtgf/suc"),
FLOAT_FORMAT(0x15,0x52d,"cvtgd/suc"),
FLOAT_FORMAT(0x15,0x52f,"cvtgq/svc"),
FLOAT_FORMAT(0x15,0x5ac,"cvtgf/su"),
FLOAT_FORMAT(0x15,0x5ad,"cvtgd/su"),
FLOAT_FORMAT(0x15,0x5af,"cvtgq/sv"),

FLOAT_FORMAT(0x17,0x10,"cvtlq"),

FLOAT_FORMAT(0x15,0x3c,"cvtqf/c"),
FLOAT_FORMAT(0x15,0x3e,"cvtqg/c"),
FLOAT_FORMAT(0x15,0xbc,"cvtqf"),
FLOAT_FORMAT(0x15,0xbe,"cvtqg"),
FLOAT_FORMAT(0x16,0x3c,"cvtqs/c"),
FLOAT_FORMAT(0x16,0x3e,"cvtqt/c"),
FLOAT_FORMAT(0x16,0x7c,"cvtqs/m"),
FLOAT_FORMAT(0x16,0x7e,"cvtqt/m"),
FLOAT_FORMAT(0x16,0xbc,"cvtqs"),
FLOAT_FORMAT(0x16,0xbe,"cvtqt"),
FLOAT_FORMAT(0x16,0xfc,"cvtqs/d"),
FLOAT_FORMAT(0x16,0xfe,"cvtqt/d"),
FLOAT_FORMAT(0x16,0x73c,"cvtqs/suic"),
FLOAT_FORMAT(0x16,0x73e,"cvtqt/suic"),
FLOAT_FORMAT(0x16,0x77c,"cvtqs/suim"),
FLOAT_FORMAT(0x16,0x77e,"cvtqt/suim"),
FLOAT_FORMAT(0x16,0x7bc,"cvtqs/sui"),
FLOAT_FORMAT(0x16,0x7be,"cvtqt/sui"),
FLOAT_FORMAT(0x16,0x7fc,"cvtqs/sui"),
FLOAT_FORMAT(0x16,0x7fe,"cvtqt/sui"),
FLOAT_FORMAT(0x17,0x30,"cvtql"),
FLOAT_FORMAT(0x17,0x130,"cvtql/v"),
FLOAT_FORMAT(0x17,0x530,"cvtql/sv"),

FLOAT_FORMAT(0x16,0x2ac,"cvtst"),
FLOAT_FORMAT(0x16,0x6ac,"cvtst/s"),

FLOAT_FORMAT(0x16,0x2c,"cvtts/c"),
FLOAT_FORMAT(0x16,0x2f,"cvttq/c"),
FLOAT_FORMAT(0x16,0x6c,"cvtts/m"),
FLOAT_FORMAT(0x16,0x6f,"cvttq/m"),
FLOAT_FORMAT(0x16,0xac,"cvtts"),
FLOAT_FORMAT(0x16,0xaf,"cvttq"),
FLOAT_FORMAT(0x16,0xec,"cvtts/d"),
FLOAT_FORMAT(0x16,0xef,"cvttq/d"),
FLOAT_FORMAT(0x16,0x12c,"cvtts/uc"),
FLOAT_FORMAT(0x16,0x12f,"cvttq/vc"),
FLOAT_FORMAT(0x16,0x16c,"cvtts/um"),
FLOAT_FORMAT(0x16,0x16f,"cvttq/vm"),
FLOAT_FORMAT(0x16,0x1ac,"cvtts/u"),
FLOAT_FORMAT(0x16,0x1af,"cvttq/v"),
FLOAT_FORMAT(0x16,0x1ec,"cvtts/ud"),
FLOAT_FORMAT(0x16,0x1ef,"cvttq/vd"),
FLOAT_FORMAT(0x16,0x52c,"cvtts/suc"),
FLOAT_FORMAT(0x16,0x52f,"cvttq/svc"),
FLOAT_FORMAT(0x16,0x56c,"cvtts/sum"),
FLOAT_FORMAT(0x16,0x56f,"cvttq/svm"),
FLOAT_FORMAT(0x16,0x5ac,"cvtts/su"),
FLOAT_FORMAT(0x16,0x5af,"cvttq/sv"),
FLOAT_FORMAT(0x16,0x5ec,"cvtts/sud"),
FLOAT_FORMAT(0x16,0x5ef,"cvttq/svd"),
FLOAT_FORMAT(0x16,0x72c,"cvtts/suic"),
FLOAT_FORMAT(0x16,0x72f,"cvttq/svic"),
FLOAT_FORMAT(0x16,0x76c,"cvtts/suim"),
FLOAT_FORMAT(0x16,0x76f,"cvttq/svim"),
FLOAT_FORMAT(0x16,0x7ac,"cvtts/sui"),
FLOAT_FORMAT(0x16,0x7af,"cvttq/svi"),
FLOAT_FORMAT(0x16,0x7ec,"cvtts/suid"),
FLOAT_FORMAT(0x16,0x7ef,"cvttq/svid"),
/***	SQRT 			***/
FLOAT_FORMAT(0x14,0xa,  "sqrtf/c"),
FLOAT_FORMAT(0x14,0x40a,"sqrtf/sc"),
FLOAT_FORMAT(0x14,0x50a,"sqrtf/suc"),
FLOAT_FORMAT(0x14,0x58a,"sqrtf/su"),
FLOAT_FORMAT(0x14,0x48a,"sqrtf/s"),
FLOAT_FORMAT(0x14,0x10a,"sqrtf/uc"),
FLOAT_FORMAT(0x14,0x18a,"sqrtf/u"),
FLOAT_FORMAT(0x14,0x8a, "sqrtf"),

FLOAT_FORMAT(0x14,0x2a, "sqrtg/c"),
FLOAT_FORMAT(0x14,0x42a,"sqrtg/sc"),
FLOAT_FORMAT(0x14,0x52a,"sqrtg/suc"),
FLOAT_FORMAT(0x14,0x5aa,"sqrtg/su"),
FLOAT_FORMAT(0x14,0x4aa,"sqrtg/s"),
FLOAT_FORMAT(0x14,0x12a,"sqrtg/uc"),
FLOAT_FORMAT(0x14,0x1aa,"sqrtg/u"),
FLOAT_FORMAT(0x14,0xaa, "sqrtg"),

FLOAT_FORMAT(0x14,0xb,  "sqrts/c"),
FLOAT_FORMAT(0x14,0xcb, "sqrts/d"),
FLOAT_FORMAT(0x14,0x4b, "sqrts/m"),
FLOAT_FORMAT(0x14,0x50b,"sqrts/suc"),
FLOAT_FORMAT(0x14,0x5cb,"sqrts/suc"),
FLOAT_FORMAT(0x14,0x70b,"sqrts/suic"),
FLOAT_FORMAT(0x14,0x7cb,"sqrts/suid"),
FLOAT_FORMAT(0x14,0x74b,"sqrts/suim"),
FLOAT_FORMAT(0x14,0x78b,"sqrts/sui"),
FLOAT_FORMAT(0x14,0x54b,"sqrts/sum"),
FLOAT_FORMAT(0x14,0x58b,"sqrts/su"),
FLOAT_FORMAT(0x14,0x10b,"sqrts/uc"),
FLOAT_FORMAT(0x14,0x1cb,"sqrts/ud"),
FLOAT_FORMAT(0x14,0x14b,"sqrts/um"),
FLOAT_FORMAT(0x14,0x18b,"sqrts/u"),
FLOAT_FORMAT(0x14,0x8b,"sqrts"),

FLOAT_FORMAT(0x14,0x2b, "sqrtt/c"),
FLOAT_FORMAT(0x14,0xeb, "sqrtt/d"),
FLOAT_FORMAT(0x14,0x6b, "sqrtt/m"),
FLOAT_FORMAT(0x14,0x52b,"sqrtt/suc"),
FLOAT_FORMAT(0x14,0x5eb,"sqrtt/suc"),
FLOAT_FORMAT(0x14,0x72b,"sqrtt/suic"),
FLOAT_FORMAT(0x14,0x7eb,"sqrtt/suid"),
FLOAT_FORMAT(0x14,0x76b,"sqrtt/suim"),
FLOAT_FORMAT(0x14,0x7ab,"sqrtt/sui"),
FLOAT_FORMAT(0x14,0x56b,"sqrtt/sum"),
FLOAT_FORMAT(0x14,0x5ab,"sqrtt/su"),
FLOAT_FORMAT(0x14,0x12b,"sqrtt/uc"),
FLOAT_FORMAT(0x14,0x1eb,"sqrtt/ud"),
FLOAT_FORMAT(0x14,0x16b,"sqrtt/um"),
FLOAT_FORMAT(0x14,0x1ab,"sqrtt/u"),
FLOAT_FORMAT(0x14,0xab, "sqrtt"),

FLOAT_FORMAT(0x14,0x14, "itoff"),
FLOAT_FORMAT(0x14,0x04, "itofs"),
FLOAT_FORMAT(0x14,0x24, "itoft"),

PAL_FORMAT(0x00, 0x0000,"HALT"),  	/* the Halt pal instruction. */
#if PAL_OS==PAL_OS_NT
/* NT calls, opcodes from NT's genalpha.c */
PAL_FORMAT(0x00, 0x0001, "restart"),
PAL_FORMAT(0x00, 0x0002, "draina"),
PAL_FORMAT(0x00, 0x0003, "reboot"),
PAL_FORMAT(0x00, 0x0004, "initpal"),
PAL_FORMAT(0x00, 0x0005, "wrentry"),
PAL_FORMAT(0x00, 0x0006, "swpirql"),
PAL_FORMAT(0x00, 0x0007, "rdirql"),
PAL_FORMAT(0x00, 0x0008, "di"),
PAL_FORMAT(0x00, 0x0009, "ei"),
PAL_FORMAT(0x00, 0x000a, "swppal"),
PAL_FORMAT(0x00, 0x000c, "ssir"),
PAL_FORMAT(0x00, 0x000d, "csir"),
PAL_FORMAT(0x00, 0x000e, "rfe"),
PAL_FORMAT(0x00, 0x000f, "retsys"),
PAL_FORMAT(0x00, 0x0010, "swpctx"),
PAL_FORMAT(0x00, 0x0011, "swpprocess"),
PAL_FORMAT(0x00, 0x0012, "rdmces"),
PAL_FORMAT(0x00, 0x0013, "wrmces"),
PAL_FORMAT(0x00, 0x0014, "tbia"),
PAL_FORMAT(0x00, 0x0015, "tbis"),
PAL_FORMAT(0x00, 0x0016, "dtbis"),
PAL_FORMAT(0x00, 0x0017, "tbisasn"),
PAL_FORMAT(0x00, 0x0018, "rdksp"),
PAL_FORMAT(0x00, 0x0019, "swpksp"),
PAL_FORMAT(0x00, 0x001a, "rdpsr"),
PAL_FORMAT(0x00, 0x001c, "rdpcr"),
PAL_FORMAT(0x00, 0x001e, "rdthread"),
PAL_FORMAT(0x00, 0x0020, "tbim"),
PAL_FORMAT(0x00, 0x0021, "tbimasn"),
PAL_FORMAT(0x00, 0x0030, "rdcounters"),
PAL_FORMAT(0x00, 0x0031, "rdstate"),
PAL_FORMAT(0x00, 0x0032, "wrperfmon"),
PAL_FORMAT(0x00, 0x0080, "bpt"),
PAL_FORMAT(0x00, 0x0083, "callsys"),
PAL_FORMAT(0x00, 0x0086, "imb"),
PAL_FORMAT(0x00, 0x00aa, "gentrap"),
PAL_FORMAT(0x00, 0x00ab, "rdteb"),
PAL_FORMAT(0x00, 0x00ac, "kbpt"),
PAL_FORMAT(0x00, 0x00ad, "callkd"),
#endif
#if PAL_OS==PAL_OS_OSF
/* OSF PAL codes, system include file provides the opcode numbers */
PAL_FORMAT(0x00, PAL_gentrap,	"gentrap"),
PAL_FORMAT(0x00, PAL_rduniq,	"rduniq"),
PAL_FORMAT(0x00, PAL_wruniq,	"wruniq"),
PAL_FORMAT(0x00, PAL_bpt,	"bpt"),
PAL_FORMAT(0x00, PAL_bugchk,	"bugchk"),
PAL_FORMAT(0x00, PAL_chmk,	"chmk"),
PAL_FORMAT(0x00, PAL_callsys,	"callsys"),
PAL_FORMAT(0x00, PAL_imb,	"imb"),
PAL_FORMAT(0x00, PAL_halt,	"halt"),
PAL_FORMAT(0x00, PAL_draina,	"draina"),
PAL_FORMAT(0x00, PAL_nphalt,	"nphalt"),
PAL_FORMAT(0x00, PAL_cobratt,	"cobratt"),
PAL_FORMAT(0x00, PAL_cserve,	"cserve"),
PAL_FORMAT(0x00, PAL_ipir,	"ipir"),
PAL_FORMAT(0x00, PAL_cflush,	"cflush"),
PAL_FORMAT(0x00, PAL_rti,	"rti"),
PAL_FORMAT(0x00, PAL_rtsys,	"rtsys"),
PAL_FORMAT(0x00, PAL_whami,	"whami"),
PAL_FORMAT(0x00, PAL_rdusp,	"rdusp"),
PAL_FORMAT(0x00, PAL_wrperfmon,	"wrperfmon"),
PAL_FORMAT(0x00, PAL_wrusp,	"wrusp"),
PAL_FORMAT(0x00, PAL_wrkgp,	"wrkgp"),
PAL_FORMAT(0x00, PAL_rdps,	"rdps"),
PAL_FORMAT(0x00, PAL_swpipl,	"swpipl"),
PAL_FORMAT(0x00, PAL_wrent,	"wrent"),
PAL_FORMAT(0x00, PAL_tbi,	"tbi"),
PAL_FORMAT(0x00, PAL_rdval,	"rdval"),
PAL_FORMAT(0x00, PAL_wrval,	"wrval"),
PAL_FORMAT(0x00, PAL_swpctx,	"swpctx"),
PAL_FORMAT(0x00, PAL_jtopal,	"jtopal"),
PAL_FORMAT(0x00, PAL_wrvptptr,	"wrvptptr"),
PAL_FORMAT(0x00, PAL_wrfen,	"wrfen"),
PAL_FORMAT(0x00, PAL_mtpr_mces,	"mtpr_mces"),
#endif
PAL_GENERIC_FORMAT(0x00,"CALLPAL"),
	{0,NULL,0}    	    	    	/* testing */
};

char buffer[256];

char *decode_memop(unsigned int encoded, u64 pc, int *src1, int *src2, int *offset)
{
  unsigned int given;
  alpha_insn *insn;
  int found = 0;

  given = encoded;

  /*
  // Make buffer point to nothing
  */
  buffer[0] = 0x00;

  for(insn = alpha_insn_set; insn->name && !found; insn++) {
    switch(insn->type) {
    case MEMORY_FORMAT_CODE:
      if((insn->i & MEMORY_FORMAT_MASK) ==(given & MEMORY_FORMAT_MASK)) {
	
	*src1 = RA(given);
	*src2 = RB(given);
	*offset = (OPCODE(given) == 9) ? DISP(given) * 65536 : DISP(given);

	sprintf(buffer,"%-9s%s, %d(%s)",
		insn->name,
		alpha_int_regs[RA(given)],
		OPCODE(given) == 9 ? DISP(given) * 65536 : DISP(given),
		alpha_int_regs[RB(given)]);
	found = 1;
      }
      break;
    case MEMORY_FLOAT_FORMAT_CODE:
      if((insn->i & MEMORY_FORMAT_MASK) ==(given & MEMORY_FORMAT_MASK)) {
	*src1 = RA(given);
	*src2 = RB(given);
	*offset = DISP(given);
	sprintf(buffer,"%-9s%s, %d(%s)",
		insn->name,
		alpha_fp_regs[RA(given)],
		DISP(given),
		alpha_int_regs[RB(given)]);
	found = 1;
      }
      break;
    }
  }
  return(buffer);
}

char *disassemble(unsigned int encoded, u64 pc)
{
  unsigned int given;
  alpha_insn *insn;
  int found = 0;

  given = encoded;

  /*
  // Make buffer point to nothing
  */
  buffer[0] = 0x00;

  for(insn = alpha_insn_set; insn->name && !found; insn++) {
    switch(insn->type) {
    case MEMORY_FORMAT_CODE:
      if((insn->i & MEMORY_FORMAT_MASK) ==(given & MEMORY_FORMAT_MASK)) {
	sprintf(buffer,"%-9s%s, %d(%s)",
		insn->name,
		alpha_int_regs[RA(given)],
		OPCODE(given) == 9 ? DISP(given) * 65536 : DISP(given),
		alpha_int_regs[RB(given)]);
	found = 1;
      }
      break;
    case MEMORY_FLOAT_FORMAT_CODE:
      if((insn->i & MEMORY_FORMAT_MASK) ==(given & MEMORY_FORMAT_MASK)) {
	sprintf(buffer,"%-9s%s, %d(%s)",
		insn->name,
		alpha_fp_regs[RA(given)],
		DISP(given),
		alpha_int_regs[RB(given)]);
	found = 1;
      }
      break;
    case MEMORY_FUNCTION_FORMAT_CODE:
      if ((insn->i & MEMORY_FUNCTION_FORMAT_MASK) ==
	  (given & MEMORY_FUNCTION_FORMAT_MASK))
	{
	  sprintf(buffer,"%-9s%s, %s",
		  insn->name,
		  alpha_int_regs[RA(given)],
		  alpha_int_regs[RB(given)]);
	  found = 1;
	}
      break;
    case BRANCH_FORMAT_CODE:
      if((insn->i & BRANCH_FORMAT_MASK) == (given & BRANCH_FORMAT_MASK)) {
	u64 curr_pc;
	u64 branch_target;
	int displacement;
	displacement = BDISP(given) * 4;  
	curr_pc = pc;

	branch_target = curr_pc + 4 + (long)displacement;

	sprintf(buffer,"%-9s%s, "FMT64X FMT64X,
		insn->name,
		alpha_int_regs[RA(given)],
		(branch_target>>32) & 0xffffffffL,
		branch_target & 0xffffffffL);
	
                found = 1;
      }
      break;
    case FP_BRANCH_FORMAT_CODE:
      if((insn->i & BRANCH_FORMAT_MASK) == (given & BRANCH_FORMAT_MASK)) {
	u64 curr_pc;
	u64 branch_target;
	int displacement;
	displacement = BDISP(given) * 4;  
	curr_pc = pc;

	branch_target = curr_pc + 4 + (long)displacement;

	sprintf(buffer,"%-9s%s, "FMT64X FMT64X,
		insn->name,
		alpha_fp_regs[RA(given)],
		(branch_target>>32) & 0xffffffffL,
		branch_target & 0xffffffffL);
	
                found = 1;
      }
      break;
    case MEMORY_BRANCH_FORMAT_CODE:
      if((insn->i & MEMORY_BRANCH_FORMAT_MASK) ==
	 (given & MEMORY_BRANCH_FORMAT_MASK))
	{
	  if(given & (1<<15))
	    {
	      sprintf(buffer,"%-9s%s, (%s)",
		      insn->name,
		      alpha_int_regs[RA(given)],
		      alpha_int_regs[RB(given)]);
	    }
	  else
	    {
	        /* Displacement is a hint only. do not put out a symbolic
		   address. */
	      u64 curr_pc;
	      u64 branch_target;
	      int displacement;
	      displacement = JDISP(given) * 4;  
	      curr_pc = pc;

	      branch_target = (long)displacement + curr_pc + 4;         
	      sprintf(buffer, "%-9s%s, (%s), 0x"FMT64X FMT64X,
		      insn->name,
		      alpha_int_regs[RA(given)],
		      alpha_int_regs[RB(given)],
		      (branch_target>>32) & 0xffffffffL,
		      branch_target & 0xffffffffL);
	    }
	  found = 1;
	}
      break;
    case OPERATE_FORMAT_CODE:
      if((insn->i & OPERATE_FORMAT_MASK)==(given & OPERATE_FORMAT_MASK))
	{
	  if(OP_OPTYPE(insn->i) == OP_OPTYPE(given))
	    {
	      if(OP_IS_CONSTANT(given))
		{
		  sprintf(buffer,"%-9s%s, 0x%x, %s",
			  insn->name,
			  alpha_int_regs[RA(given)],
			  LITERAL(given),
			  alpha_int_regs[RC(given)]);
		}
	      else
		{
		  sprintf(buffer,"%-9s%s, %s, %s",
			  insn->name,
			  alpha_int_regs[RA(given)],
			  alpha_int_regs[RB(given)],
			  alpha_int_regs[RC(given)]);
		}
	      found = 1;
	    }
	}
      break;
    case FLOAT_FORMAT_CODE:
      if((insn->i & FLOAT_FORMAT_MASK)==(given & FLOAT_FORMAT_MASK))
	{
	  sprintf(buffer,"%-9s%s, %s, %s",
		  insn->name,
		  alpha_fp_regs[RA(given)],
		  alpha_fp_regs[RB(given)],
		  alpha_fp_regs[RC(given)]);
	  found = 1;
	}
      break;
    case PAL_FORMAT_CODE:
      if(insn->i == given)
	{
	  sprintf(buffer,"call_pal %s",insn->name);
	  found = 1;
	}
      break;
    case PAL_GENERIC_FORMAT_CODE:
      if((insn->i & PAL_GENERIC_FORMAT_MASK) == (given & PAL_GENERIC_FORMAT_MASK))
	{
	  sprintf(buffer,"call_pal 0x%04x",PAL_FUN(given));
	  found = 1;
	}
      break;
    case RESERVED_FORMAT_CODE:
      if((insn->i & RESERVED_FORMAT_MASK) ==
	 (given & RESERVED_FORMAT_MASK))
	{
          /* PALtemp registers:
           *     pt18  usp
           *     pt19  ksp
           *       30  sp
           *
           * mfpr r30, r18  ==>  switch stack to user stack    ==> go to user mode
           * mfpr r30, r19  ==>  switch stack to kernel stack  ==> go to kernel mode
           * 
           * if (OPCODE(given) == 0x19 || OPCODE(given) == 0x1d)
           *     sprintf(buffer,"%-9s%s, %s, %s%s",
           *             insn->name,
           *             alpha_int_regs[RA(given)],
           *             alpha_int_regs[RB(given)],
           *             alpha_int_regs[RC(given)],
           *             ((OPCODE(given)==0x19 && RA(given) == 30) ?
           *                    ((RC(given) == 18) ?
           *                        "\nUSER Swtch" :
           *                        ((RC(given) == 19) ? "\nKERNEL Swtch" : "")) : ""));
           * else
           *     sprintf(buffer,"%s",insn->name);
           */
          sprintf(buffer,"%s",insn->name);
	  found = 1;
	}
      break;
    default: 
      fprintf(stderr, "What the hell is this? The insn doesn't have a type. insn->i = %08x insn->name = %s  insn->type = %08x\n",
	      insn->i, insn->name, insn->type);
      break;
    }
  }

  
  if (buffer[0] == '\0') {
      sprintf(buffer,"opcode=0x%02x ???, inst=0x%08x",(given>>26) & 0x3f, given);
  }

  return(buffer);
}

