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
 * Definitions of opnums for Alpha opcodes so that all instructions can
 * be treated uniformly by using the desc_table array
 */

#ifndef OPCODES_H
#define OPCODES_H

#ifndef NEW_MVI
#define NEW_MVI 1
#endif

#include "feeder.h"

#define OP(NAME) icode_ptr NAME(inflight_inst_ptr ifi, icode_ptr picode, thread_ptr pthread)
  
#include <alpha/inst.h>
#include <alpha/pal.h>
#include "newinst.h"
#include "ttlinst.h"


#include "opprot.h"

#define UNIMPLEMENTED(NAME) \
icode_ptr NAME(inflight_inst_ptr ifi, icode_ptr picode, thread_ptr pthread) \
{return unimplemented_op(ifi, picode, pthread); }

#define RESERVED(NAME) \
icode_ptr NAME(inflight_inst_ptr ifi, icode_ptr picode, thread_ptr pthread) \
{return reserved_op(ifi, picode, pthread); }

#define EMPTY_OP(NAME) \
icode_ptr NAME(inflight_inst_ptr ifi, icode_ptr picode, thread_ptr pthread) \
{return picode->next; }

typedef enum {Reserved, PAL, Memory, Operate, Branch, Float } Iformat;

extern Iformat OpFormat[];

typedef enum opnum_e {
  default_opnum = 0,
  /* PAL group: see <alpha/pal.h> */
  pal_gentrap, pal_rduniq, pal_wruniq, pal_bpt, 
  pal_bugchk, pal_chmk, pal_callsys, pal_imb,
  pal_halt, pal_draina, pal_nphalt, pal_cobratt,
  pal_cserve, pal_ipir, pal_cflush, pal_rti,
  pal_rtsys, pal_whami, pal_rdusp, pal_wrperfmon,
  pal_wrusp, pal_wrkgp, pal_rdps, pal_swpipl, 
  pal_wrent, pal_tbi, pal_rdval, pal_wrval,
  pal_swpctx, pal_jtopal, pal_wrvptptr, pal_wrfen,
  pal_mtpr_mces, 

  /* Opcodes corresponding to single instructions */
    res_opc01, res_opc02, res_opc03,
  res_opc04, res_opc05, res_opc06, res_opc07,
  lda_opn, ldah_opn, ldb_opn, ldq_u_opn,
  ldw_opn, stw_opn, stb_opn, stq_u_opn,

  /* Integer arithmetic group */
  addl_opn, s4addl_opn, subl_opn, s4subl_opn,
#ifdef NEW_MVI
  cmpwge_opn,
#endif
  cmpbge_opn, s8addl_opn, s8subl_opn, cmpult_opn,
  addq_opn, s4addq_opn, subq_opn, s4subq_opn,
  cmpeq_opn, s8addq_opn, s8subq_opn, cmpule_opn,
  addlv_opn, sublv_opn, cmplt_opn, addqv_opn,
  subqv_opn, cmple_opn,

  /* Integer logical group */
    and_opn, bic_opn, 
  cmovlbs_opn, cmovlbc_opn, bis_opn, cmoveq_opn,
  cmovne_opn, ornot_opn, xor_opn, cmovlt_opn,
  cmovge_opn, eqv_opn, amask_opn, cmovle_opn, cmovgt_opn, implver_opn,

  /* Integer shift group */
  mskbl_opn, extbl_opn, insbl_opn, mskwl_opn,
  extwl_opn, inswl_opn, mskll_opn, extll_opn,
  insll_opn, zap_opn, zapnot_opn, mskql_opn,
  srl_opn, extql_opn, sll_opn, insql_opn,
  sra_opn, mskwh_opn, inswh_opn, extwh_opn,
  msklh_opn, inslh_opn, extlh_opn, mskqh_opn,
  insqh_opn, extqh_opn,

  /* integer multiply group */
    mull_opn, mulq_opn, 
  umulh_opn, mullv_opn, mulqv_opn,

  res_opc14,

  /* vax floating point group */
  addfc_opn, subfc_opn, mulfc_opn, divfc_opn,
  cvtdgc_opn, addgc_opn, subgc_opn, mulgc_opn,
  divgc_opn, cvtgfc_opn, cvtgdc_opn, cvtgqc_opn,
  cvtqfc_opn, cvtqgc_opn, addf_opn, subf_opn,
  mulf_opn, divf_opn, cvtdg_opn, addg_opn,
  subg_opn, mulg_opn, divg_opn, cmpgeq_opn,
  cmpglt_opn, cmpgle_opn, cvtgf_opn, cvtgd_opn,
  cvtgq_opn, cvtqf_opn, cvtqg_opn,  addfuc_opn, 
  subfuc_opn, mulfuc_opn,
  divfuc_opn, cvtdguc_opn, addguc_opn, subguc_opn,
  mulguc_opn, divguc_opn, cvtgfuc_opn, cvtgduc_opn,
  cvtgqvc_opn, addfu_opn, subfu_opn, mulfu_opn, 
  divfu_opn, cvtdgu_opn, addgu_opn, subgu_opn,
  mulgu_opn, divgu_opn, cvtgfu_opn, cvtgdu_opn,
  cvtgqv_opn, addfsc_opn, subfsc_opn, mulfsc_opn,
  divfsc_opn, cvtdgsc_opn, addgsc_opn, subgsc_opn,
  mulgsc_opn, divgsc_opn, cvtgfsc_opn, cvtgdsc_opn,
  cvtgqsc_opn, addfs_opn, subfs_opn, mulfs_opn,
  divfs_opn, cvtdgs_opn, addgs_opn, subgs_opn,
  mulgs_opn, divgs_opn, cmpgeqs_opn, cmpglts_opn,
  cmpgles_opn, cvtgfs_opn, cvtgds_opn, cvtgqs_opn,
  addfsuc_opn, subfsuc_opn, mulfsuc_opn, divfsuc_opn,
  cvtdgsuc_opn, addgsuc_opn, subgsuc_opn, mulgsuc_opn,
  divgsuc_opn, cvtgfsuc_opn, cvtgdsuc_opn, cvtgqsvc_opn,
  addfsu_opn, subfsu_opn, mulfsu_opn, divfsu_opn,
  cvtdgsu_opn, addgsu_opn, subgsu_opn, mulgsu_opn,
  divgsu_opn, cvtgfsu_opn, cvtgdsu_opn, cvtgqsv_opn,

  /* ieee floating point group */
  
  addsc_opn,subsc_opn,mulsc_opn,divsc_opn,sqrtsc_opn,
  addtc_opn,subtc_opn,multc_opn,divtc_opn,sqrttc_opn,cvttsc_opn,
  cvttqc_opn,cvtqsc_opn,cvtqtc_opn,addsm_opn,
  subsm_opn,mulsm_opn,divsm_opn,sqrtsm_opn,addtm_opn,
  subtm_opn,multm_opn,divtm_opn,sqrttm_opn,cvttsm_opn,cvttqm_opn,
  cvtqsm_opn,cvtqtm_opn,adds_opn,subs_opn,
  muls_opn,divs_opn,sqrts_opn,addt_opn,subt_opn,
  mult_opn,divt_opn,sqrtt_opn,cmptun_opn,cmpteq_opn,
  cmptlt_opn,cmptle_opn,cvtts_opn,cvttq_opn,
  cvtqs_opn,cvtqt_opn,addsd_opn,subsd_opn,
  mulsd_opn,divsd_opn,sqrtsd_opn,addtd_opn,subtd_opn,
  multd_opn,divtd_opn,sqrttd_opn,cvttsd_opn,cvttqd_opn,
  cvtqsd_opn,cvtqtd_opn,addsuc_opn,subsuc_opn,
  mulsuc_opn,divsuc_opn,sqrtsuc_opn,addtuc_opn,subtuc_opn,
  multuc_opn,divtuc_opn,sqrttuc_opn,cvttsuc_opn,cvttqvc_opn,
  addsum_opn,subsum_opn,mulsum_opn,divsum_opn,sqrtsum_opn,
  addtum_opn,subtum_opn,multum_opn,divtum_opn,sqrttum_opn,
  cvttsum_opn,cvttqvm_opn,addsu_opn,subsu_opn,
  mulsu_opn,divsu_opn,sqrtsu_opn,addtu_opn,subtu_opn,
  multu_opn,divtu_opn,sqrttu_opn,cvttsu_opn,cvttqv_opn,
  addsud_opn,subsud_opn,mulsud_opn,divsud_opn,sqrtsud_opn,
  addtud_opn,subtud_opn,multud_opn,divtud_opn,sqrttud_opn,
  cvttsud_opn,cvttqvd_opn,cvtst_opn,cvtsts_opn,
  addssuc_opn,subssuc_opn,mulssuc_opn,divssuc_opn,sqrtssuc_opn,
  addtsuc_opn,subtsuc_opn,multsuc_opn,divtsuc_opn,sqrttsuc_opn,
  cvttssuc_opn,cvttqsvc_opn,addssum_opn,subssum_opn,
  mulssum_opn,divssum_opn,sqrtssum_opn,addtsum_opn,subtsum_opn,
  multsum_opn,divtsum_opn,sqrttsum_opn,cvttssum_opn,cvttqsvm_opn,
  addssu_opn,subssu_opn,mulssu_opn,divssu_opn,sqrtssu_opn,
  addtsu_opn,subtsu_opn,multsu_opn,divtsu_opn,sqrttsu_opn,
  cmptunsu_opn,cmpteqsu_opn,cmptltsu_opn,cmptlesu_opn,
  cvttssu_opn,cvttqsv_opn,addssud_opn,subssud_opn,
  mulssud_opn,divssud_opn,sqrtssud_opn,addtsud_opn,subtsud_opn,
  multsud_opn,divtsud_opn,sqrttsud_opn,cvttssud_opn,cvttqsvd_opn,
  /* cvttss_opn, */ addssuic_opn,subssuic_opn,mulssuic_opn,
  divssuic_opn,sqrtssuic_opn,addtsuic_opn,subtsuic_opn,multsuic_opn,
  divtsuic_opn,sqrttsuic_opn,cvttssuic_opn,cvttqsvic_opn,cvtqssuic_opn,
  cvtqtsuic_opn,addssuim_opn,subssuim_opn,mulssuim_opn,
  divssuim_opn,sqrtssuim_opn,addtsuim_opn,subtsuim_opn,multsuim_opn,
  divtsuim_opn,sqrttsuim_opn,cvttssuim_opn,cvttqsvim_opn,cvtqssuim_opn,
  cvtqtsuim_opn,addssui_opn,subssui_opn,mulssui_opn,
  divssui_opn,sqrtssui_opn,addtsui_opn,subtsui_opn,multsui_opn,
  divtsui_opn,sqrttsui_opn,cvttssui_opn,cvttqsvi_opn,cvtqssui_opn,
  cvtqtsui_opn,addssuid_opn,subssuid_opn,mulssuid_opn,
  divssuid_opn,sqrtssuid_opn,addtsuid_opn,subtsuid_opn,multsuid_opn,
  divtsuid_opn,sqrttsuid_opn,cvttssuid_opn,cvttqsvid_opn,cvtqssuid_opn,
  cvtqtsuid_opn,
  
  /* Datatype independent floating point group */
    cvtlq_opn, cpys_opn, cpysn_opn,
  cpyse_opn, mt_fpcr_opn, mf_fpcr_opn, fcmoveq_opn,
  fcmovne_opn, fcmovlt_opn, fcmovge_opn,
  fcmovle_opn, fcmovgt_opn, cvtql_opn, cvtqlv_opn,
  cvtqlsv_opn,

  /* Miscellaneous ops group */
    trapb_opn, excb_opn, mb_opn,
  wmb_opn, fetch_opn, fetch_m_opn, rpcc_opn,
  rc_opn, rs_opn, ecb_opn, wh64_opn, wh64en_opn,

    pal19_opn,

  /* jsr group */
    jmp_opn,
  jsr_opn, ret_opn, jsr_coroutine_opn,

  pal1b_opn,
  pal1d_opn, pal1e_opn, pal1f_opn,
  ldf_opn, ldg_opn, lds_opn, ldt_opn,
  stf_opn, stg_opn, sts_opn, stt_opn,
  ldl_opn, ldq_opn, ldl_l_opn, ldq_l_opn,
  stl_opn, stq_opn, stl_c_opn, stq_c_opn,
  br_opn, fbeq_opn, fblt_opn, fble_opn, 
  bsr_opn, fbne_opn, fbge_opn, fbgt_opn,
  blbc_opn, beq_opn, blt_opn, ble_opn,
  blbs_opn, bne_opn, bge_opn, bgt_opn,

  /* opc14 group */
  itofs_opn, itoff_opn, itoft_opn,

  /* intmisc group */
  sextb_opn, sextw_opn,
  perr_opn, minub8_opn, minsb8_opn, minuw4_opn, minsw4_opn, maxub8_opn,
  maxsb8_opn, maxuw4_opn, maxsw4_opn, pklb_opn, pkwb_opn, unpkbl_opn,
  unpkbw_opn, ctpop_opn, ctlz_opn, cttz_opn, ftois_opn, ftoit_opn,

#ifdef NEW_MVI
  /* Arana MVI group */
  paddsb8_opn, paddsw4_opn, paddub8_opn, padduw4_opn,
  psubsb8_opn, psubsw4_opn, psubub8_opn, psubuw4_opn,
  pmulluw4_opn, pmulhuw4_opn, pminmaxsl2_opn, pminmaxul2_opn,
  pminmaxsb8_opn, pminmaxsw4_opn, pminmaxub8_opn, pminmaxuw4_opn,
  taddsb8_opn, taddsw4_opn, taddub8_opn, tadduw4_opn,
  tsubsb8_opn, tsubsw4_opn, tsubub8_opn, tsubuw4_opn,
  tmulsb8_opn, tmulsw4_opn, tmulub8_opn, tmuluw4_opn, tmulusb8_opn, tmulusw4_opn,
  psll2_opn,
  psrb8_opn, psrw4_opn, pslb8_opn, pslw4_opn,
  psrl2_opn, psral2_opn, psrab8_opn, psraw4_opn,
  pkswb8_opn, pkslw4_opn, pkuwb8_opn, pkulw4_opn,
  upksbw4_opn, upkswl2_opn, upkubw4_opn, upkuwl2_opn,
  tabserrsb8_opn, tabserrub8_opn, tabserrsw4_opn, tabserruw4_opn,
  tsqerrsb8_opn, tsqerrub8_opn, tsqerrsw4_opn, tsqerruw4_opn,
  permb8_opn, gpkblb4_opn,
  paddsl2_opn, paddul2_opn, psubsl2_opn, psubul2_opn,

  /* Arana MVI GI group */
  padd_opn, psub_opn, paddc_opn, psubc_opn, phadd_opn, phsub_opn, phsubr_opn,
  pmul_opn, pmull_opn, pmulls_opn, pmulh_opn, pmulhs_opn,
  parcpl_opn, parcplh_opn, parcpll_opn,
  parsqrt_opn, parsqrth_opn, parsqrtl_opn,
  pmovll_opn, pmovlh_opn, pmovhl_opn, pmovhh_opn,
  pcpys_opn, pcpysn_opn, pcpyse_opn,
  pcmpeq_opn, pcmpne_opn, pcmplt_opn, pcmple_opn, pcmpun_opn,
  pfmax_opn, pfmin_opn,
  pcvtsp_opn,
  pextl_opn, pexth_opn,
  pcvtfi_opn,
#endif

#ifdef NEW_TTL
  /* Tarantula vector instructions */

    ttl_mvtvp_opn,
    ttl_mvfvp_opn,
    ttl_setnvm_opn,
    ttl_setvm_opn,
    ttl_vctlz_opn,
    ttl_vcttz_opn,
    ttl_vctpop_opn,
    ttl_vextsl_opn,
    ttl_vextsh_opn,
    ttl_viota_opn,
    ttl_vskewh_opn,
    ttl_vskewl_opn,
    ttl_vldt_opn,
    ttl_vlds_opn,
    ttl_vldq_opn,
    ttl_vldl_opn,
    ttl_vstt_opn,
    ttl_vsts_opn,
    ttl_vstq_opn,
    ttl_vstl_opn,
    ttl_vgathq_opn,
    ttl_vgathl_opn,
    ttl_vgatht_opn,
    ttl_vgaths_opn,
    ttl_vscatq_opn,
    ttl_vscatl_opn,
    ttl_vscatt_opn,
    ttl_vscats_opn,
    ttl_vncgathq_opn,
    ttl_vncgathl_opn,
    ttl_vncgatht_opn,
    ttl_vncgaths_opn,
    ttl_vncscatq_opn,
    ttl_vncscatl_opn,
    ttl_vncscatt_opn,
    ttl_vncscats_opn,
    ttl_vvaddt_opn,
    ttl_vvadds_opn,
    ttl_vvaddq_opn,
    ttl_vvaddl_opn,
    ttl_vvsubt_opn,
    ttl_vvsubs_opn,
    ttl_vvsubq_opn,
    ttl_vvsubl_opn,
    ttl_vvmult_opn,
    ttl_vvmuls_opn,
    ttl_vvmulq_opn,
    ttl_vvmull_opn,
    ttl_vvdivt_opn,
    ttl_vvdivs_opn,
    ttl_vsqrtt_opn,
    ttl_vsqrts_opn,
    ttl_vsaddt_opn,
    ttl_vsadds_opn,
    ttl_vssubt_opn,
    ttl_vssubs_opn,
    ttl_vsmult_opn,
    ttl_vsmuls_opn,
    ttl_vsdivt_opn,
    ttl_vsdivs_opn,
    ttl_vsaddq_opn,
    ttl_vsaddl_opn,
    ttl_vssubq_opn,
    ttl_vssubl_opn,
    ttl_vsmulq_opn,
    ttl_vsmull_opn,
    ttl_vsand_opn,
    ttl_vsbis_opn,
    ttl_vsxor_opn,
    ttl_vsbic_opn,
    ttl_vsornot_opn,
    ttl_vseqv_opn,
    ttl_vvand_opn,
    ttl_vvbis_opn,
    ttl_vvxor_opn,
    ttl_vvbic_opn,
    ttl_vvornot_opn,
    ttl_vveqv_opn,
    ttl_vvcmpeq_opn,
    ttl_vvcmple_opn,
    ttl_vvcmplt_opn,
    ttl_vscmpeq_opn,
    ttl_vscmple_opn,
    ttl_vscmplt_opn,
    ttl_vvcmpteq_opn,
    ttl_vvcmptle_opn,
    ttl_vvcmptlt_opn,
    ttl_vvcmptun_opn,
    ttl_vscmpteq_opn,
    ttl_vscmptle_opn,
    ttl_vscmptlt_opn,
    ttl_vscmptun_opn,
    ttl_vvcmpule_opn,
    ttl_vvcmpult_opn,
    ttl_vscmpule_opn,
    ttl_vscmpult_opn,
    ttl_vvcmpbge_opn,
    ttl_vscmpbge_opn,
    ttl_vcvtqs_opn,
    ttl_vcvtqt_opn,
    ttl_vcvtst_opn,
    ttl_vcvttq_opn,
    ttl_vcvtts_opn,
    ttl_vsextbl_opn,
    ttl_vsextwl_opn,
    ttl_vsextll_opn,
    ttl_vsextql_opn,
    ttl_vsextwh_opn,
    ttl_vsextlh_opn,
    ttl_vsextqh_opn,
    ttl_vvextbl_opn,
    ttl_vvextwl_opn,
    ttl_vvextll_opn,
    ttl_vvextql_opn,
    ttl_vvextwh_opn,
    ttl_vvextlh_opn,
    ttl_vvextqh_opn,
    ttl_vsinsbl_opn,
    ttl_vsinswl_opn,
    ttl_vsinsll_opn,
    ttl_vsinsql_opn,
    ttl_vsinswh_opn,
    ttl_vsinslh_opn,
    ttl_vsinsqh_opn,
    ttl_vvinsbl_opn,
    ttl_vvinswl_opn,
    ttl_vvinsll_opn,
    ttl_vvinsql_opn,
    ttl_vvinswh_opn,
    ttl_vvinslh_opn,
    ttl_vvinsqh_opn,
    ttl_vvmerg_opn,
    ttl_vsmergq_opn,
    ttl_vsmergt_opn,
    ttl_vvmskbl_opn,
    ttl_vvmskwl_opn,
    ttl_vvmskll_opn,
    ttl_vvmskql_opn,
    ttl_vvmskwh_opn,
    ttl_vvmsklh_opn,
    ttl_vvmskqh_opn,
    ttl_vsmskbl_opn,
    ttl_vsmskwl_opn,
    ttl_vsmskll_opn,
    ttl_vsmskql_opn,
    ttl_vsmskwh_opn,
    ttl_vsmsklh_opn,
    ttl_vsmskqh_opn,
    ttl_vssra_opn,
    ttl_vvsra_opn,
    ttl_vssll_opn,
    ttl_vvsll_opn,
    ttl_vssrl_opn,
    ttl_vvsrl_opn,
    ttl_vs4addq_opn,
    ttl_vs8addq_opn,
    ttl_vs4addl_opn,
    ttl_vs8addl_opn,
    ttl_vs4subq_opn,
    ttl_vs8subq_opn,
    ttl_vs4subl_opn,
    ttl_vs8subl_opn,
    ttl_vvumulh_opn,
    ttl_vsumulh_opn,
    ttl_vvzap_opn,
    ttl_vszap_opn,
    ttl_vvzapnot_opn,
    ttl_vszapnot_opn,
    ttl_vcmpr_opn, 
	ttl_vsynch_opn,   
	ttl_vldpf_opn,
	ttl_vstpf_opn,
	ttl_vgathpf_opn,
	ttl_vscatpf_opn,
	ttl_vncgathpf_opn,
	ttl_vncscatpf_opn,
	ttl_vdrainm0_opn,
	ttl_vdrainm1_opn,
	ttl_vdrainm2_opn,
	ttl_vdrainv0_opn,
	ttl_vdrainv1_opn,
	ttl_vdrainv2_opn,
	ttl_mvfvr_opn,
	
#endif

#ifdef NEW_TLDS
  /* TLDS instructions */
  tlds_bs_opn, tlds_bns_opn, tlds_q_opn, tlds_arm_opn, tlds_en_opn,
#endif

  /* no-ops */
  nop_opn,

  opn_count /* must be last */
} opnum_t;

/* Structure for decoding instructions in different groups. */  

typedef struct group_struct {
  int opkey;
  opnum_t opnum;
} group_t;


extern group_t pal_group[], inta_group[], intl_group[], intm_group[],
  ints_group[], fltv_group[], flti_group[], fltl_group[], misc_group[],
  jsr_group[], opc14_group[], intmisc_group[],
#ifdef NEW_MVI
  opc04_group[],
#endif
#ifdef NEW_TTL
  ttl_group[],
#endif
#ifdef NEW_TLDS
  tlds_group[],
#endif
  defgroup[];


struct op_desc {
  char *opname;
  PFPI func;
  int  iflags;
};

extern struct op_desc desc_table[];

/* Function prototypes */
OP(terminator1);
OP(terminator2);
OP(terminate_thr);
OP(done_thr);
OP(done_pthr);

OP(pterminator1);
OP(pterminator2);
OP(terminate_pthr);

OP(event_read);
OP(event_write);
OP(event_yield);
OP(event_fork);
OP(event_load_locked);
OP(event_store_conditional);
OP(event_memory_barrier);
OP(event_wh64);
OP(event_sim_user);
OP(event_inst);

OP(event_pthread_create);

#endif
