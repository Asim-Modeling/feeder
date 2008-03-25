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

/***********************************************************************/
#pragma ident "$Id: atf_test.cpp 799 2006-10-31 12:27:52Z cjbeckma $"
/***********************************************************************
 *
 *	ABSTRACT:	Arana Trace Format	
 *			EXAMPLE READER/WRITER
 *
 *	AUTHOR:		snyder@segsrv.shr.cpqcorp.net
 *
 ***********************************************************************/

// generic
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#if defined (HOST_LINUX) || defined (HOST_LINUX_X86) || defined(HOST_FREEBSD) || defined(HOST_FREEBSD_X86)
#include "atom.inst.h"
#else
#include <cmplrs/atom.inst.h>
#endif

// ASIM local module
#include "atf.h"
#include "disasm_test.h"

int debug_level = 0;	/* ATF debugging level */
Boolean print_every = FALSE;	/* Print all tokens, not just instructions */
Boolean printInstructions = FALSE;

extern int64 IsLoadStore(uint64 inst);
extern int64 IsLoad(uint64 inst);

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/* Example Writer */

void writer (char *filename)
    /* Example write routine */
{
    Boolean success;

    /* Init */
    ATF_init();

    /* Open */
    success = ATF_file_open (0, filename, TRUE/*write*/);
    if (!success) {
      perror("Couldn't open output file");
    }

    /* Header */
    ATF_write_global  (0, avg_OS, aos_OSF);
    ATF_write_global  (0, avg_EPOCH, 0);
    ATF_write_local   (0, avl_SUPPAG, (asp_SP_EV5 | asp_KSEG_EV5));	/* If you don't want write_pte use asp_DIRECT */
    ATF_write_comment (0, "Test program");

    /* Various process information */
    ATF_write_global  (0, avg_CPU, 0);
    ATF_write_local   (0, avl_TID, 10);

    ATF_write_local   (0, avl_PID, 9);
    ATF_write_local   (0, avl_ASN, 0x123L);
    ATF_write_local   (0, avl_ASM, FALSE);	/* TRUE makes this _process_ direct mapped */

    /* Cache dump */
    ATF_write_dump_start (0);
    ATF_write_dump (0, 0x2222222222220000L, 0x2345234523460000L, FALSE/*I*/, ads_SHARED/*shared*/);
    ATF_write_dump (0, 0x0022222222111111L, 0x0000000000000000L, TRUE /*D*/, ads_DIRTYSHARED/*dshare*/);
    ATF_write_dump (0, 0x0000000011111111L, 0x0000000000000000L, TRUE /*D*/, ads_DIRTY/*dirty*/);
    ATF_write_dump (0, 0x0000000000000111L, 0x0000000000000000L, TRUE /*D*/, ads_EXCLUSIVE/*excl*/);
    ATF_write_dump_end (0);

    /* Couple of PTE entries (do before referencing them!!)  These apply to the current avl_ASN only! */
    /* Instruction addresses */
    ATF_write_pte (0, 0x1234123412340000L, 0x23452345ff000001L);
    ATF_write_pte (0, 0x1234123412000000L, 0x23452300ff000001L);
    ATF_write_pte (0, 0x2222222222220000L, 0x22222220ff000001L);
    /* Data EA addresses */
    ATF_write_pte (0, 0x235L, 	    0xdadada00ff000001L);
    ATF_write_pte (0, 0x235L+20000, 0xdadadb00ff000001L);
    ATF_write_pte (0, 0x234L+123123123123123L, 0xddddddd0ff000001L);

    /* Instructions */
    ATF_write_instruction (0, 0x1234123412340020L, 0x0aaaaaaa, 0);
    ATF_write_instruction (0, 0x1234123412340024L, 0x8dadadad, 0x235);
    ATF_write_instruction (0, 0x1234123412340028L, 0x8cbcbcbc, 0x236);
    ATF_write_instruction (0, 0x1234123412340020L, 0x0aaaaaaa, 0);
    ATF_write_instruction (0, 0x1234123412340024L, 0x8dadadad, 0x235);
    ATF_write_instruction (0, 0x1234123412340028L, 0x8cbcbcbc, 0x236);
    ATF_write_instruction (0, 0x1234123412340020L, 0x0aaaaaaa, 0);
    ATF_write_instruction (0, 0x1234123412340024L, 0x8dadadad, 0x235);
    ATF_write_instruction (0, 0x1234123412340028L, 0x8cbcbcbc, 0x236);
    ATF_write_instruction (0, 0x1234123412340020L, 0x0aaaaaaa, 0);
    ATF_write_instruction (0, 0x1234123412340024L, 0x8dadadad, 0x235);
    ATF_write_instruction (0, 0x1234123412340028L, 0x8cbcbcbc, 0x236);
    ATF_write_instruction (0, 0x1234123412340020L, 0xaaaaaaaa, 0x234);
    ATF_write_instruction (0, 0x1234123412340024L, 0x8dadadad, 0x235+10);
    ATF_write_instruction (0, 0x1234123412340028L, 0x8cbcbcbc, 0x236+20000);
    ATF_write_instruction (0, 0x1234123412000020L, 0xaaaaaaaa, 0x234+123123123123123L);
    ATF_write_instruction (0, 0x2222222222220020L, 0x00001234, 0);
    ATF_write_instruction (0, 0x2222222222220040L, 0x1234,     0);

    ATF_file_close (0);
}

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/* Example Reader */

void reader (char *filename)
    /* Example read routine */
{
    Boolean success;
    Boolean eof=FALSE;
    AtfAction_enum action;
    AtfGenParam value1, value2, value3, value4;
    u64 inum[ATF_CPU_MAX];

    /* Init */
    ATF_init();
    bzero ((char *)inum, sizeof(inum));

    /* Open */
    success = ATF_file_open (0, filename, FALSE/*read*/);
    if (!success) {
      perror("Couldn't open input file");
    }

    while (!eof) {
	action = ATF_read_action (0, &value1, &value2, &value3, &value4);
	switch (action) {

	case act_INSTR_OLD:
	case act_INSTR_new:
	{
	    Address vpc = value1.uint64;	
	    Atf_Instr_Op_Etc *ope_ptr = (Atf_Instr_Op_Etc*)value2.ptr;
	    Address vea = value3.uint64;
	    if (printInstructions) {
		int cpu = ATF_get_global (0, avg_CPU);
		u64 pid = ATF_get_local  (0, avl_PID);
		u64 tid = ATF_get_local  (0, avl_TID);
		u64 asn = ATF_get_local  (0, avl_ASN);
		Address ppc = ATF_tlb_translate (0, vpc, TRUE/*istream*/);
		Address pea = (IsLoadStore(ope_ptr->opcode)) ? ATF_tlb_translate (0, vea, FALSE/*dstream*/) : 0;
		if (debug_level) fflush (stderr);
		printf ("act_INSTR C%d %"FMT64"d P%"FMT64"d T%"FMT64"d A%03"FMT64"x: VPC %016"FMT64"x PPC %016"FMT64"x  %-40s",
			cpu, inum[cpu]++,	pid, tid, asn,
			vpc, ppc,    		disassemble (ope_ptr->opcode, vpc));
		if (IsLoadStore(ope_ptr->opcode)) {
		    printf ("   ! VEA %016"FMT64"x  PEA %016"FMT64"x\n", vea, pea);
		} else {
		    printf ("\n");
		}
		if (debug_level) fflush (stdout);
	    }
            else if (0) {
                if (IsLoadStore(ope_ptr->opcode)) {
                    Address pea = ATF_tlb_translate (0, vea, FALSE/*dstream*/);
                    /* 0x2E91CC0, 0x2E87654, 0x12345, 0x28AC5DD, 0x1178CF8, 0x2ABCDEF, 0x485FCBA, 0x8CA342D, 0x2E8AB4E0, 0x40FDABC */
                    if (((pea >> 6) & ((1UL << 12) - 1)) ==  ((0x40FDABC >> 6) & ((1UL << 12) - 1)))
                        printf("%"FMT64"d %d 0x%2x 0x%016"FMT64"x\n",
                               ATF_get_global(0, avg_CPU),
                               (int)IsLoad(ope_ptr->opcode),
                               (int)OPCODE(ope_ptr->opcode),
                               pea);
                }
            }
	    break;
	}

	case act_SET: {
	    char *varname = "Unknown";
	    AtfVariable_enum varnum = (AtfVariable_enum) value1.uint64;
	    u64 value = value2.uint64;
	    /* Based on what variable changed, do something... */
	    /* (Yes this could be better done as a array, it's just an example) */
	    switch (varnum) {
	    case avg_CPU:	varname = "CPU#"; break;
	    case avg_EPOCH:	varname = "EPOCH"; break;
	    case avg_VERSION: 	varname = "VERSION"; break;
	    case avg_OS:	varname = "OS"; break;
	    case avl_TID:	varname = "TID"; break;
	    case avl_PID:	varname = "PID"; break;
	    case avl_ASN:	varname = "ASN"; break;
	    case avl_ASM:	varname = "ASM"; break;
            case avl_SUPPAG:    varname = "SUPPAG"; break;
	    }
	    if (print_every) printf ("act_SET changed var %d %s  = %#"FMT64"x\n", varnum, varname, value);
	    break;
	}

	case act_PTE: {
	    Address vpc = value1.uint64;	
	    uint64 pte = value2.uint64;	
	    if (print_every) printf ("act_PTE va=%016"FMT64"x pte=%016"FMT64"x\n", vpc, pte);
	    break;
	}

	case act_DMA: {
	    Address pa = value1.uint64;	
	    uint    len = value2.uint64;	
	    Boolean isWrite = value3.uint64;	
	    if (print_every) printf ("act_DMA pa=%#"FMT64"x len=%#x isWrite=%#x\n", pa, len, isWrite);
	    break;
	}

	case act_DUMP: {
	    Address pa = value1.uint64;
	    Address va = value2.uint64;
	    Boolean id = value3.uint64;
	    AtfDumpState_enum state = (AtfDumpState_enum) value4.uint64;
	    if (print_every) printf ("act_DUMP PA %016"FMT64"x  VA %016"FMT64"x  I/D%d  State%d\n", pa, va, id, state);
	    break;
	}

	case act_COMMENT: {
	    if (print_every) printf ("act_COMMENT: '%s'\n", (char *)value1.ptr);
	    break;
	}

	case act_BR_ACTION: {
            AtfBranchAction_enum action = (AtfBranchAction_enum) value1.uint64;
	    if (print_every) printf ("act_BR_ACTION %s\n", (action==atf_br_act_TAKEN ? "TAKEN" : "NOT_TAKEN"));
	    break;
	}

	case act_INTERRUPT: {
	    if (print_every) printf ("act_INTERRUPT\n");
	    break;
	}

	case act_PROC_TEXT_INFO: {
	    u64 pid = value1.uint64;
	    Address start = value2.uint64;
	    Address end = value3.uint64;
	    if (print_every) printf ("act_PROC_TEXT_INFO: pid=%"FMT64"u text=%#"FMT64"x-%#"FMT64"x comm='%s'\n", pid, start, end, (char *)value4.ptr);
	    break;
	}

	case act_PROC_BSS_INFO: {
	    u64 pid = value1.uint64;
	    Address start = value2.uint64;
	    Address end = value3.uint64;
	    if (print_every) printf ("act_PROC_BSS_INFO: pid=%"FMT64"u bss=%#"FMT64"x-%#"FMT64"x\n", pid, start, end);
	    break;
	}

	case act_THREAD_USTACK_ADDR_INFO: {
	    u64 pid = value1.uint64;
	    u64 tid = value2.uint64;
	    Address start = value3.uint64;
	    Address end = value4.uint64;
	    if (print_every) printf ("act_THREAD_USTACK_ADDR_INFO: pid=%"FMT64"u tid=%#"FMT64"x ustack=%#"FMT64"x-%#"FMT64"x\n", pid, tid, start, end);
	    break;
	}

	case act_THREAD_USTACK_GROW_INFO: {
	    u64 pid = value1.uint64;
	    u64 tid = value2.uint64;
	    u64 size = value3.uint64;
	    Boolean grow_up = value4.uint64;
	    if (print_every) printf ("act_THREAD_USTACK_GROW_INFO: pid=%"FMT64"u tid=%#"FMT64"x ustack_size=%"FMT64"u grow=%s\n", pid, tid, size, grow_up ? "Up" : "Down");
	    break;
	}

	case act_THREAD_KSTACK_INFO: {
	    u64 pid = value1.uint64;
	    u64 tid = value2.uint64;
	    Address start = value3.uint64;
	    Address end = value4.uint64;
	    if (print_every) printf ("act_THREAD_KSTACK_INFO: pid=%"FMT64"u tid=%#"FMT64"x kstack=%#"FMT64"x-%#"FMT64"x\n", pid, tid, start, end);
	    break;
	}

	case act_LIBLOADER_INFO: {
	    Address text_start = value1.uint64;
	    Address text_end = value2.uint64;
	    Address bss_start = value3.uint64;
	    Address bss_end = value4.uint64;
	    if (print_every) printf ("act_LIBLOADER_INFO: text=%#"FMT64"x-%#"FMT64"x bss=%#"FMT64"x-%#"FMT64"x\n", text_start, text_end, bss_start, bss_end);
	    break;
	}

	case act_LIB_INFO: {
	    Address text_start = value1.uint64;
	    Address text_end = value2.uint64;
	    Address bss_start = value3.uint64;
	    Address bss_end = value4.uint64;
	    if (print_every) printf ("act_LIB_INFO: text=%#"FMT64"x-%#"FMT64"x bss=%#"FMT64"x-%#"FMT64"x\n", text_start, text_end, bss_start, bss_end);
	    break;
	}

	case act_KERNEL_SWITCH: {
	    Boolean in = value1.uint64;
	    Address addr = value2.uint64;
	    if (print_every) printf ("act_KERNEL_SWITCH: %s addr=%#"FMT64"x comment='%s'\n", (in ? "IN" : "OUT"), addr, (char *)value3.ptr);
	    break;
	}

	case act_IDLE_THREAD: {
            Boolean idle_start = value1.uint64;
	    if (print_every) printf ("act_IDLE_THREAD %s\n", (idle_start ? "START" : "END"));
	    break;
	}

	case act_EOF: {
	    if (print_every) printf ("act_EOF\n");
	    eof = TRUE;
	    break;
	}
	} /* switch */
    } /* while */

    ATF_file_close (0);
}

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/* Usage/Main */

void usage() {
    fprintf (stderr, "atf_test\t[flags] {atf_filename}\n");
    fprintf (stderr, "\t{atf_filename} is a xxx_aa.atf Arana trace file.\n");
    fprintf (stderr, "\t  A filename may contain:\n");
    fprintf (stderr, "\t  $ENVVARS which are expanded to environment contents.\n");
    fprintf (stderr, "\t  .gz to pipe input/output through gzip.\n");
    fprintf (stderr, "\t  trailing :# to indicate filtering to a specific CPU.\n");
    fprintf (stderr, "\t[flags] are one of:\n");
    fprintf (stderr, "\t\t-e\tPrint every token, not just instructions\n");
    fprintf (stderr, "\t\t-i\tPrint read instructions as they execute\n");
    fprintf (stderr, "\t\t-D\tEnable verbose debugging messages\n");
    fprintf (stderr, "\tOne of the following modes must be used:\n");
    fprintf (stderr, "\t\t-r\tRead the file\n");
    fprintf (stderr, "\t\t-w\tWrite the file as an example\n");
    exit (0L);
}

int main (int argc, char **argv)
{
    char *fname=NULL;
    Boolean do_read = FALSE;
    Boolean do_write = FALSE;

    while (--argc) {
	argv++;
	if (**argv == '-') {
            switch (*++*argv) {
	    case 'r':
		do_read = TRUE;
		break;
	    case 'w':
		do_write = TRUE;
		break;
	    case 'D':
		debug_level = 99;
		break;
	    case 'i':
		printInstructions = TRUE;
		break;
	    case 'e':
		print_every = TRUE;
		printInstructions = TRUE;
		break;
	    default:
                usage ();
            }
	}
	else {
	    /* Non-switch */
	    if (fname==NULL) fname = *argv;
	    else usage();
	}
    }
    if (fname==NULL) usage();

    /* Run */
    if (do_write) writer(fname);
    if (do_read) reader(fname);
}

