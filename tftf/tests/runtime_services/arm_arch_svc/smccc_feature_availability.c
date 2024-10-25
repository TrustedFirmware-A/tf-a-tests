/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <tftf.h>

#include <arm_arch_svc.h>
#include <smccc.h>
#include <test_helpers.h>
#include <tftf_lib.h>

static inline u_register_t get_feature_for_reg(u_register_t reg)
{
	smc_args args = {0};
	smc_ret_values ret;

	args.fid = SMCCC_ARCH_FEATURE_AVAILABILITY | (SMC_64 << FUNCID_CC_SHIFT);
	args.arg1 = reg;
	ret = tftf_smc(&args);

	/* APi is pretty simple, support was already checked. This is simpler */
	assert((int)ret.ret0 == SMC_ARCH_CALL_SUCCESS);
	return ret.ret1;
}

static inline bool always_present(void)
{
	return true;
}

/*
 * Checks that the bit's matches the feature's presence
 * feat_func should be is_feat_xyz_present(), but get_feat_xyz_support() will
 * also work.
 */
#define CHECK_BIT_SET(feat_func, bit)						\
	do {									\
		if (!!feat_func() != !!(reg & bit)) {				\
			tftf_testcase_printf(					\
				#feat_func " says feature is %ssupported but "	\
				#bit " was %sset!\n", feat_func() ? "" : "not ",\
				(reg & bit) ? "" : "not ");			\
			bad = true;						\
		}								\
		reg &= ~bit;							\
	} while (0)

/* when support for a new feature is added, we want this test to be
 * updated. Fail the test so it's noticed. */
#define CHECK_NO_BITS_SET(reg_name)						\
	do {									\
		if (reg != 0) {							\
			tftf_testcase_printf(					\
			#reg_name " still has values set: 0x%lx. "		\
				"Test needs to be updated\n", reg);		\
			bad = true;						\
		}								\
	} while (0)

test_result_t test_smccc_arch_feature_availability(void)
{
	SKIP_TEST_IF_AARCH32();
#ifdef __aarch64__

	SKIP_TEST_IF_SMCCC_VERSION_LT(1, 1);
	SKIP_TEST_IF_SMCCC_FUNC_NOT_SUPPORTED(SMCCC_ARCH_FEATURE_AVAILABILITY);

	u_register_t reg;
	bool bad = false;

	reg = get_feature_for_reg(SCR_EL3_OPCODE);
	CHECK_BIT_SET(is_armv8_9_fgt2_present,			SCR_FGTEN2_BIT);
	CHECK_BIT_SET(is_feat_fpmr_present,			SCR_EnFPM_BIT);
	CHECK_BIT_SET(is_feat_d128_supported,			SCR_D128En_BIT);
	CHECK_BIT_SET(is_feat_s1pie_present,			SCR_PIEN_BIT);
	CHECK_BIT_SET(is_feat_sctlr2_supported,			SCR_SCTLR2En_BIT);
	CHECK_BIT_SET(is_feat_tcr2_supported,			SCR_TCR2EN_BIT);
	CHECK_BIT_SET(is_feat_the_supported,			SCR_RCWMASKEn_BIT);
	CHECK_BIT_SET(is_feat_sme_supported,			SCR_ENTP2_BIT);
	CHECK_BIT_SET(is_feat_rng_trap_present,			SCR_TRNDR_BIT);
	CHECK_BIT_SET(is_feat_gcs_present,			SCR_GCSEn_BIT);
	CHECK_BIT_SET(get_feat_hcx_support,			SCR_HXEn_BIT);
	CHECK_BIT_SET(is_feat_ls64_accdata_present,		SCR_ADEn_BIT);
	CHECK_BIT_SET(is_feat_ls64_accdata_present,		SCR_EnAS0_BIT);
	CHECK_BIT_SET(is_feat_amuv1p1_present,			SCR_AMVOFFEN_BIT);
	CHECK_BIT_SET(get_armv8_6_ecv_support,			SCR_ECVEN_BIT);
	CHECK_BIT_SET(is_armv8_6_fgt_present,			SCR_FGTEN_BIT);
	CHECK_BIT_SET(is_feat_mte2_present,			SCR_ATA_BIT);
	CHECK_BIT_SET(is_feat_csv2_2_present,			SCR_EnSCXT_BIT);
	CHECK_BIT_SET(is_armv8_3_pauth_present,			SCR_APK_BIT);
	CHECK_BIT_SET(is_feat_ras_present,			SCR_TERR_BIT);
	CHECK_NO_BITS_SET(SCR_EL3);

	reg = get_feature_for_reg(CPTR_EL3_OPCODE);
	CHECK_BIT_SET(always_present,				CPTR_EL3_TCPAC_BIT);
	CHECK_BIT_SET(is_feat_amuv1_present,			CPTR_EL3_TAM_BIT);
	CHECK_BIT_SET(get_armv8_0_sys_reg_trace_support,	CPTR_EL3_TTA_BIT);
	CHECK_BIT_SET(is_feat_sme_supported,			CPTR_EL3_ESM_BIT);
	CHECK_BIT_SET(always_present,				CPTR_EL3_TFP_BIT);
	CHECK_BIT_SET(is_armv8_2_sve_present,			CPTR_EL3_EZ_BIT);
	CHECK_NO_BITS_SET(CPTR_EL3);

	reg = get_feature_for_reg(MDCR_EL3_OPCODE);
	CHECK_BIT_SET(get_feat_brbe_support,			MDCR_SBRBE(1));
	CHECK_BIT_SET(is_armv8_6_fgt_present,			MDCR_TDCC_BIT);
	CHECK_BIT_SET(is_feat_trbe_present,			MDCR_NSTB(1));
	CHECK_BIT_SET(get_armv8_4_trf_support,			MDCR_TTRF_BIT);
	CHECK_BIT_SET(is_feat_spe_supported,			MDCR_NSPB(1));
	CHECK_BIT_SET(is_feat_doublelock_present,		MDCR_TDOSA_BIT);
	CHECK_BIT_SET(always_present,				MDCR_TDA_BIT);
	CHECK_BIT_SET(get_feat_pmuv3_supported,			MDCR_TPM_BIT);
	CHECK_NO_BITS_SET(MDCR_EL3);

	reg = get_feature_for_reg(MPAM3_EL3_OPCODE);
	CHECK_BIT_SET(is_feat_mpam_supported,			MPAM3_EL3_TRAPLOWER_BIT);
	CHECK_NO_BITS_SET(MPAM3_EL3);

	if (bad)
		return TEST_RESULT_FAIL;

	return TEST_RESULT_SUCCESS;
#endif /* __aarch64__ */
}
