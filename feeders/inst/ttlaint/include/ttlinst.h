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

#ifdef NEW_TTL

/* 
 * definitions required for vector opcodes/funs new to EV8, EV9, EV10
 */

OP(ttl_mvtvp_f);
OP(ttl_mvfvp_f);
OP(ttl_vsetnvm_f);
OP(ttl_vsetvm_f);

OP(ttl_vctlz_f);
OP(ttl_vcttz_f);
OP(ttl_vctpop_f);
OP(ttl_vextsl_f);
OP(ttl_vextsh_f);
OP(ttl_viota_f);
OP(ttl_vskewh_f);
OP(ttl_vskewl_f);

OP(ttl_vldt_f);
OP(ttl_vlds_f);
OP(ttl_vldq_f);
OP(ttl_vldl_f);

OP(ttl_vstt_f);
OP(ttl_vsts_f);
OP(ttl_vstq_f);
OP(ttl_vstl_f);

OP(ttl_vgathq_f);
OP(ttl_vgathl_f);
OP(ttl_vgatht_f);
OP(ttl_vgaths_f);

OP(ttl_vscatq_f);
OP(ttl_vscatl_f);
OP(ttl_vscatt_f);
OP(ttl_vscats_f);

OP(ttl_vncgathq_f);
OP(ttl_vncgathl_f);
OP(ttl_vncgatht_f);
OP(ttl_vncgaths_f);

OP(ttl_vncscatq_f);
OP(ttl_vncscatl_f);
OP(ttl_vncscatt_f);
OP(ttl_vncscats_f);

OP(ttl_vvaddt_f);
OP(ttl_vvadds_f);
OP(ttl_vvaddq_f);
OP(ttl_vvaddl_f);

OP(ttl_vvsubt_f);
OP(ttl_vvsubs_f);
OP(ttl_vvsubq_f);
OP(ttl_vvsubl_f);

OP(ttl_vvmult_f);
OP(ttl_vvmuls_f);
OP(ttl_vvmulq_f);
OP(ttl_vvmull_f);

OP(ttl_vvdivt_f);
OP(ttl_vvdivs_f);

OP(ttl_vsqrtt_f);
OP(ttl_vsqrts_f);

OP(ttl_vsaddt_f);
OP(ttl_vsadds_f);
OP(ttl_vssubt_f);
OP(ttl_vssubs_f);
OP(ttl_vsmult_f);
OP(ttl_vsmuls_f);
OP(ttl_vsdivt_f);
OP(ttl_vsdivs_f);

OP(ttl_vsaddq_f);
OP(ttl_vsaddl_f);
OP(ttl_vssubq_f);
OP(ttl_vssubl_f);
OP(ttl_vsmulq_f);
OP(ttl_vsmull_f);

OP(ttl_vsand_f);
OP(ttl_vsbis_f);
OP(ttl_vsxor_f);
OP(ttl_vsbic_f);
OP(ttl_vsornot_f);
OP(ttl_vseqv_f);

OP(ttl_vvand_f);
OP(ttl_vvbis_f);
OP(ttl_vvxor_f);
OP(ttl_vvbic_f);
OP(ttl_vvornot_f);
OP(ttl_vveqv_f);

OP(ttl_vvcmpeq_f);
OP(ttl_vvcmple_f);
OP(ttl_vvcmplt_f);
OP(ttl_vscmpeq_f);
OP(ttl_vscmple_f);
OP(ttl_vscmplt_f);

OP(ttl_vvcmpule_f);
OP(ttl_vvcmpult_f);
OP(ttl_vscmpule_f);
OP(ttl_vscmpult_f);

OP(ttl_vvcmpbge_f);
OP(ttl_vscmpbge_f);

OP(ttl_vvcmpteq_f);
OP(ttl_vvcmptle_f);
OP(ttl_vvcmptlt_f);
OP(ttl_vvcmptun_f);
OP(ttl_vscmpteq_f);
OP(ttl_vscmptle_f);
OP(ttl_vscmptlt_f);
OP(ttl_vscmptun_f);

OP(ttl_vcvtqs_f);
OP(ttl_vcvtqt_f);
OP(ttl_vcvtst_f);
OP(ttl_vcvttq_f);
OP(ttl_vcvtts_f);

OP(ttl_vsextbl_f);
OP(ttl_vsextwl_f);
OP(ttl_vsextll_f);
OP(ttl_vsextql_f);
OP(ttl_vsextwh_f);
OP(ttl_vsextlh_f);
OP(ttl_vsextqh_f);

OP(ttl_vvextbl_f);
OP(ttl_vvextwl_f);
OP(ttl_vvextll_f);
OP(ttl_vvextql_f);
OP(ttl_vvextwh_f);
OP(ttl_vvextlh_f);
OP(ttl_vvextqh_f);

OP(ttl_vsinsbl_f);
OP(ttl_vsinswl_f);
OP(ttl_vsinsll_f);
OP(ttl_vsinsql_f);
OP(ttl_vsinswh_f);
OP(ttl_vsinslh_f);
OP(ttl_vsinsqh_f);

OP(ttl_vvinsbl_f);
OP(ttl_vvinswl_f);
OP(ttl_vvinsll_f);
OP(ttl_vvinsql_f);
OP(ttl_vvinswh_f);
OP(ttl_vvinslh_f);
OP(ttl_vvinsqh_f);

OP(ttl_vvmerg_f);
OP(ttl_vsmergq_f);
OP(ttl_vsmergt_f);

OP(ttl_vvmskbl_f);
OP(ttl_vvmskwl_f);
OP(ttl_vvmskll_f);
OP(ttl_vvmskql_f);
OP(ttl_vvmskwh_f);
OP(ttl_vvmsklh_f);
OP(ttl_vvmskqh_f);

OP(ttl_vsmskbl_f);
OP(ttl_vsmskwl_f);
OP(ttl_vsmskll_f);
OP(ttl_vsmskql_f);
OP(ttl_vsmskwh_f);
OP(ttl_vsmsklh_f);
OP(ttl_vsmskqh_f);

OP(ttl_vssra_f);
OP(ttl_vvsra_f);

OP(ttl_vssll_f);
OP(ttl_vvsll_f);
OP(ttl_vssrl_f);
OP(ttl_vvsrl_f);

OP(ttl_vs4addq_f);
OP(ttl_vs8addq_f);
OP(ttl_vs4addl_f);
OP(ttl_vs8addl_f);

OP(ttl_vs4subq_f);
OP(ttl_vs8subq_f);
OP(ttl_vs4subl_f);
OP(ttl_vs8subl_f);

OP(ttl_vvumulh_f);
OP(ttl_vsumulh_f);

OP(ttl_vvzap_f);
OP(ttl_vszap_f);

OP(ttl_vvzapnot_f);
OP(ttl_vszapnot_f);

OP(ttl_vcmpr_f);

OP(ttl_vsynch_f);

/* new vector prefetching instructions */
OP(ttl_vldpf_f);
OP(ttl_vstpf_f);
OP(ttl_vgathpf_f);
OP(ttl_vscatpf_f);
OP(ttl_vncgathpf_f);
OP(ttl_vncscatpf_f);

/* draining instructions */
OP(ttl_vdrainm0_f);
OP(ttl_vdrainm1_f);
OP(ttl_vdrainm2_f);
OP(ttl_vdrainv0_f);
OP(ttl_vdrainv1_f);
OP(ttl_vdrainv2_f);

/* mvfvr */
OP(ttl_mvfvr_f);

#endif /* NEW_TTL */

#ifdef NEW_TLDS

/* 
 * definitions required for TLDS opcodes new to EV8, EV9, EV10
 */

#define op_tlds        	0x05
#define tlds_bs	        0x00
#define tlds_bns	    0x01
#define tlds_q          0x02
#define tlds_arm        0x03
#define tlds_en         0x04

OP(tlds_bs_f);
OP(tlds_bns_f);
OP(tlds_q_f);
OP(tlds_arm_f);
OP(tlds_en_f);

#endif




