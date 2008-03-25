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

/*
 * HISTORY
 */
/*
 *
 * ALPHA PAL code definitions.  These are used with the call_pal
 * instruction to avoid changing the assembler whenever the PALcodes
 * change.  For example, a halt would be coded as
 * 
 * call_pal	PAL_halt
 * 
 * The PAL_ prefix allows these to be used in assembler functions with
 * entry points that match the real PALcode name (much like the SYS_
 * prefix is used on system call numbers).
 * 
 */

#ifndef _PAL_H_
#define _PAL_H_

/*
 * These are common to OSF & VMS flavors of palcode
 */
#define PAL_clrfen	174	/* 0xae */
#define PAL_gentrap	170	/* 0xaa */
#define PAL_rduniq	158 	/* 0x9e	*/
#define PAL_wruniq	159	/* 0x9f	*/
#define PAL_urti	146	/* 0x92 == VMS PAL_rei */
#define	PAL_bpt		128	/* 0x80	*/
#define	PAL_bugchk	129	/* 0x81 */
#define	PAL_chmk	131	/* 0x83	*/
#define PAL_callsys	131	/* 0x83	same as chmk */
#define	PAL_imb		134	/* 0x86	*/
#define	PAL_halt	0	/* 0x00	*/
#define	PAL_draina	2	/* 0x02	*/

#define PAL_nphalt	190	/* 0xbe	*/
#define	PAL_cobratt	9	/* 0x9	*/
#define	PAL_cserve 	9	/* 0x9	same as cobratt */
#define PAL_ipir	13	/* 0xd  */
/*
 * cflush added for memory mapped presto RAM
*/
#define PAL_cflush	1	/* 0x01 */

/*
 * This is the OSF flavor
 */

#define PAL_rti		63	/* 0x3f	*/
#define PAL_wtint	62	/* 0x3e */
#define PAL_rtsys	61	/* 0x3d	*/
#define PAL_whami	60	/* 0x3c	*/
#define PAL_rdusp	58	/* 0x3a */
#define PAL_wrperfmon	57	/* 0x39	*/
#define PAL_wrusp	56	/* 0x38 */
#define PAL_wrkgp	55	/* 0x37	*/
#define PAL_rdps	54	/* 0x36	*/
#define PAL_swpipl	53	/* 0x35	*/
#define PAL_wrent	52	/* 0x34	*/
#define PAL_tbi		51	/* 0x33	*/
#define PAL_rdval	50	/* 0x32	*/
#define PAL_wrval	49	/* 0x31	*/
#define PAL_swpctx	48	/* 0x30 */
#define PAL_jtopal	46	/* 0x2e */
#define PAL_wrvptptr	45	/* 0x2d */
#define PAL_wrfen	43	/* 0x2b */
#define	PAL_mtpr_mces	17	/* 0x11	*/
#define PAL_rdmces      16  /* 0x10 -read machine check err summary reg */
#define PAL_swppal      10  /* 0xa  -swap PALcode image*/

/*
 * Directives for PAL_urti emulation.
 */
#define URTI_GENTRAP 	1
#define URTI_EMULATE	2
#define URTI_RESCHED	3

#endif /* _PAL_H_ */
