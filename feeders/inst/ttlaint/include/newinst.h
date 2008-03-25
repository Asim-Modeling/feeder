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
 * definitions required for opcodes/funs new to EV5, EV6, EV7:
 */

#ifdef NEW_MVI
#define op_opc04        0x04
#define opc04_padd	0x00
#define opc04_psub	0x01
#define opc04_paddc	0x02
#define opc04_psubc	0x03
#define opc04_phadd	0x04
#define opc04_phsub	0x05
#define opc04_phsubr	0x06

#define opc04_pmul	0x08
#define opc04_pmull	0x09
#define opc04_pmulls	0x0A
#define opc04_pmulh	0x0B
#define opc04_pmulhs	0x0C

#define opc04_parcpl	0x10
#define opc04_parcplh	0x11
#define opc04_parcpll	0x12

#define opc04_parsqrt	0x14
#define opc04_parsqrth	0x15
#define opc04_parsqrtl	0x16

#define opc04_pmovll	0x18
#define opc04_pmovlh	0x19
#define opc04_pmovhl	0x1A
#define opc04_pmovhh	0x1B

#define opc04_pcpys	0x1C
#define opc04_pcpysn	0x1D
#define opc04_pcpyse	0x1E

#define opc04_pcmpeq	0x20
#define opc04_pcmpne	0x21
#define opc04_pcmplt	0x22
#define opc04_pcmple	0x23
#define opc04_pcmpun	0x24

#define opc04_pfmax	0x28
#define opc04_pfmin	0x29

#define opc04_pcvtsp	0x2C

#define opc04_pextl	0x2D
#define opc04_pexth	0x2E

#define opc04_pcvtfi	0x2F
#endif

/* New flti_group fun codes: */
#define opc14_sqrttc	0x02b
#define opc14_sqrttm	0x06b
#define opc14_sqrtt	0x0ab
#define opc14_sqrttd	0x0eb
#define opc14_sqrttuc	0x12b
#define opc14_sqrttum	0x16b
#define opc14_sqrttu	0x1ab
#define opc14_sqrttud	0x1eb
#define opc14_sqrttsuc	0x52b
#define opc14_sqrttsum	0x56b
#define opc14_sqrttsu	0x5ab
#define opc14_sqrttsud	0x5eb
#define opc14_sqrttsuic	0x72b
#define opc14_sqrttsuim	0x76b
#define opc14_sqrttsui	0x7ab
#define opc14_sqrttsuid	0x7eb

#define opc14_sqrtsc	0x00b
#define opc14_sqrtsm	0x04b
#define opc14_sqrts	0x08b
#define opc14_sqrtsd	0x0cb
#define opc14_sqrtsuc	0x10b
#define opc14_sqrtsum	0x14b
#define opc14_sqrtsu	0x18b
#define opc14_sqrtsud	0x1cb
#define opc14_sqrtssuc	0x50b
#define opc14_sqrtssum	0x54b
#define opc14_sqrtssu	0x58b
#define opc14_sqrtssud	0x5cb
#define opc14_sqrtssuic	0x70b
#define opc14_sqrtssuim	0x74b
#define opc14_sqrtssui	0x78b
#define opc14_sqrtssuid	0x7cb

#if OSVER < 40
#define op_ldbu		0x0a
#define op_ldwu		0x0c
#define op_stw		0x0d
#define op_stb 		0x0e

#define op_intmisc	0x1c            /* miscellaneous integer group */

#endif /* OSF3 */

#define intmisc_sextb 0x00
#define intmisc_sextw 0x01

#ifdef NEW_MVI
#define intmisc_paddsb8 0x10
#define intmisc_paddsw4 0x11
#define intmisc_paddub8 0x12
#define intmisc_padduw4 0x13

#define intmisc_psubsb8 0x14
#define intmisc_psubsw4 0x15
#define intmisc_psubub8 0x16
#define intmisc_psubuw4 0x17

#define intmisc_pmulluw4 0x18
#define intmisc_pmulhuw4 0x19
#define intmisc_pminmaxsl2 0x1a
#define intmisc_pminmaxul2 0x1b

#define intmisc_pminmaxsb8 0x1c
#define intmisc_pminmaxsw4 0x1d
#define intmisc_pminmaxub8 0x1e
#define intmisc_pminmaxuw4 0x1f

#define intmisc_taddsb8 0x20
#define intmisc_taddsw4 0x21
#define intmisc_taddub8 0x22
#define intmisc_tadduw4 0x23

#define intmisc_tsubsb8 0x24
#define intmisc_tsubsw4 0x25
#define intmisc_tsubub8 0x26
#define intmisc_tsubuw4 0x27

#define intmisc_tmulsb8  0x28
#define intmisc_tmulsw4  0x29
#define intmisc_tmulub8  0x2a
#define intmisc_tmuluw4  0x2b
#define intmisc_tmulusb8 0x2c
#define intmisc_tmulusw4 0x2d
#define intmisc_psll2    0x2e
#endif

#define intmisc_perr 0x31

#ifndef intmisc_minub8 /* some versions of /usr/include/alpha/inst.h define these */

#define intmisc_minub8 0x3A
#define intmisc_minsb8 0x38

#define intmisc_minuw4 0x3B
#define intmisc_minsw4 0x39

#define intmisc_maxub8 0x3C
#define intmisc_maxsb8 0x3E

#define intmisc_maxuw4 0x3D
#define intmisc_maxsw4 0x3F
#endif

#define intmisc_pklb 0x37
#define intmisc_pkwb 0x36

#define intmisc_unpkbl 0x35
#define intmisc_unpkbw 0x34

#define intmisc_ctpop 0x30
#define intmisc_ctlz 0x32
#define intmisc_cttz 0x33

#ifdef NEW_MVI
#define intmisc_psrb8 0x40
#define intmisc_psrw4 0x41
#define intmisc_pslb8 0x42
#define intmisc_pslw4 0x43

#define intmisc_psrl2  0x44
#define intmisc_psral2 0x45
#define intmisc_psrab8 0x46
#define intmisc_psraw4 0x47

#define intmisc_pkswb8 0x48
#define intmisc_pkslw4 0x49
#define intmisc_pkuwb8 0x4a
#define intmisc_pkulw4 0x4b

#define intmisc_upksbw4 0x4c
#define intmisc_upkswl2 0x4d
#define intmisc_upkubw4 0x4e
#define intmisc_upkuwl2 0x4f

#define intmisc_tabserrsb8   0x50
#define intmisc_tabserrub8   0x51
#define intmisc_tabserrsw4   0x52
#define intmisc_tabserruw4   0x53

#define intmisc_tsqerrsb8   0x54
#define intmisc_tsqerrub8   0x54
#define intmisc_tsqerrsw4   0x54
#define intmisc_tsqerruw4   0x54

#define intmisc_permb8  0x58
#define intmisc_gpkblb4 0x59

#define intmisc_paddsl2  0x5C
#define intmisc_paddul2  0x5D
#define intmisc_psubsl2  0x5E
#define intmisc_psubul2  0x5F
#endif

/* Integer/Floating Register Moves */

#define opc14_itofs 0x04 
#define opc14_itoff 0x14 
#define opc14_itoft 0x24 

#define intmisc_ftois 0x78
#define intmisc_ftoit 0x70

/* Write Hint */
#ifndef misc_ecb /* some versions of /usr/include/alpha/inst.h define these */
#define misc_ccb 0xEC00
#define misc_ecb 0xE800
#define misc_wh64 0xF800
#endif
/* Write Hint evict next (SRM ECO #127 - apparently approved) */
/* encoding according to Steve Root's email */
#ifndef misc_wh64en
#define misc_wh64en 0xFC00
#endif

#ifdef NEW_MVI
#define inta_cmpwge 0x0E
#endif

OP(sextb_f);
OP(sextw_f);
OP(perr_f);
OP(minub8_f);
OP(minsb8_f);
OP(minuw4_f);
OP(minsw4_f);
OP(maxub8_f);
OP(maxsb8_f);
OP(maxuw4_f);
OP(maxsw4_f);
OP(pklb_f);
OP(pkwb_f);
OP(unpkbl_f);
OP(unpkbw_f);
OP(itofs_f);
OP(itoff_f);
OP(itoft_f);
OP(ctpop_f);

#ifdef NEW_MVI
OP(padd_f);
OP(psub_f);
OP(paddc_f);
OP(psubc_f);
OP(phadd_f);
OP(phsub_f);
OP(phsubr_f);

OP(pmul_f);
OP(pmull_f);
OP(pmulls_f);
OP(pmulh_f);
OP(pmulhs_f);

OP(parcpl_f);
OP(parcplh_f);
OP(parcpll_f);

OP(parsqrt_f);
OP(parsqrth_f);
OP(parsqrtl_f);

OP(pmovll_f);
OP(pmovlh_f);
OP(pmovhl_f);
OP(pmovhh_f);

OP(pcpys_f);
OP(pcpysn_f);
OP(pcpyse_f);

OP(pcmpeq_f);
OP(pcmpne_f);
OP(pcmplt_f);
OP(pcmple_f);
OP(pcmpun_f);

OP(pfmax_f);
OP(pfmin_f);

OP(pcvtsp_f);

OP(pextl_f);
OP(pexth_f);

OP(pcvtfi_f);

OP(paddsb8_f);
OP(paddsw4_f);
OP(paddub8_f);
OP(padduw4_f);

OP(psubsb8_f);
OP(psubsw4_f);
OP(psubub8_f);
OP(psubuw4_f);

OP(pmulluw4_f);
OP(pmulhuw4_f);
OP(pminmaxsl2_f);
OP(pminmaxul2_f);

OP(pminmaxsb8_f);
OP(pminmaxsw4_f);
OP(pminmaxub8_f);
OP(pminmaxuw4_f);

OP(taddsb8_f);
OP(taddsw4_f);
OP(taddub8_f);
OP(tadduw4_f);

OP(tsubsb8_f);
OP(tsubsw4_f);
OP(tsubub8_f);
OP(tsubuw4_f);

OP(tmulsb8_f);
OP(tmulsw4_f);
OP(tmulub8_f);
OP(tmuluw4_f);
OP(tmulusb8_f);
OP(tmulusw4_f);
OP(psll2_f);

OP(psrb8_f);
OP(psrw4_f);
OP(pslb8_f);
OP(pslw4_f);

OP(psrl2_f);
OP(psral2_f);
OP(psrab8_f);
OP(psraw4_f);

OP(pkswb8_f);
OP(pkslw4_f);
OP(pkuwb8_f);
OP(pkulw4_f);

OP(upksbw4_f);
OP(upkswl2_f);
OP(upkubw4_f);
OP(upkuwl2_f);

OP(tabserrsb8_f);
OP(tabserrub8_f);
OP(tabserrsw4_f);
OP(tabserruw4_f);

OP(tsqerrsb8_f);
OP(tsqerrub8_f);
OP(tsqerrsw4_f);
OP(tsqerruw4_f);

OP(permb8_f);
OP(gpkblb4_f);

OP(paddsl2_f);
OP(paddul2_f);
OP(psubsl2_f);
OP(psubul2_f);

OP(cmpwge_f);
#endif

OP(ctlz_f);
OP(cttz_f);
OP(ftois_f);
OP(ftoit_f);
OP(ecb_f);
OP(wh64_f);
OP(wh64en_f);
