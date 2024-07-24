/*
 * Copyright (c) 2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <test_helpers.h>
#include <tftf_lib.h>
#include <tftf.h>
#include <string.h>
#include <arch_helpers.h>

#ifdef __aarch64__

static bool is_init_val_set(u_register_t reg, u_register_t init_val,
	u_register_t feat_mask)
{
	return (reg & feat_mask) == (init_val & feat_mask);
}

#define CHECK_FEAT_TRAP_INITIALIZED(_reg, _REG, _feat_check, _FEAT)	\
do {									\
	if (_feat_check() != 0) {					\
		if (is_init_val_set(_reg, _REG ## _INIT_VAL,		\
			_REG ## _ ## FEAT_ ## _FEAT ## _MASK) == 0) {	\
			return TEST_RESULT_FAIL;			\
		}							\
	}								\
} while (false);

#define CHECK_FEAT_TRAP_INITIALIZED2(_reg, _REG, _feat_check, _FEAT,	\
	_feat2_check, _FEAT2, _op)					\
do {									\
	if ((_feat_check() != 0) _op (_feat2_check() != 0)) {		\
		if (is_init_val_set(_reg, _REG ## _INIT_VAL, _REG ## _	\
			## FEAT_ ## _FEAT ## _ ## _FEAT2 ## _MASK)	\
			== 0) {						\
			return TEST_RESULT_FAIL;			\
		}							\
	}								\
} while (false);

#endif

/*
 * TF-A is expected to allow access to ARMv8.6-FGT system registers from EL2.
 * Reading these registers causes a trap to EL3 and crash when TF-A has not
 * allowed access.
 */
test_result_t test_fgt_enabled(void)
{
	SKIP_TEST_IF_AARCH32();

#ifdef __aarch64__
	SKIP_TEST_IF_FGT_NOT_SUPPORTED();

	u_register_t hfgitr_el2 = read_hfgitr_el2();
	u_register_t hfgrtr_el2 = read_hfgrtr_el2();
	u_register_t hfgwtr_el2 = read_hfgwtr_el2();

	/*
	 * The following registers are not supposed to be consumed, but
	 * are read to test their presence when FEAT_FGT is supported.
	 */
	read_hdfgrtr_el2();
	read_hdfgwtr_el2();

	CHECK_FEAT_TRAP_INITIALIZED(hfgitr_el2, HFGITR_EL2,		\
		get_feat_brbe_support, BRBE)
	CHECK_FEAT_TRAP_INITIALIZED(hfgitr_el2, HFGITR_EL2,		\
		is_feat_specres_present, SPECRES)
	CHECK_FEAT_TRAP_INITIALIZED(hfgitr_el2, HFGITR_EL2,		\
		is_feat_tlbirange_present, TLBIRANGE)
	CHECK_FEAT_TRAP_INITIALIZED2(hfgitr_el2, HFGITR_EL2,		\
		is_feat_tlbirange_present, TLBIRANGE,			\
		is_feat_tlbios_present, TLBIOS, &&)
	CHECK_FEAT_TRAP_INITIALIZED(hfgitr_el2, HFGITR_EL2,		\
		is_feat_tlbios_present, TLBIOS)
	CHECK_FEAT_TRAP_INITIALIZED(hfgitr_el2, HFGITR_EL2,		\
		is_armv8_2_pan2_present, PAN2)
	CHECK_FEAT_TRAP_INITIALIZED(hfgitr_el2, HFGITR_EL2,		\
		is_feat_dpb2_present, DPB2)
	if (is_init_val_set(hfgitr_el2, HFGITR_EL2_INIT_VAL,
		HFGITR_EL2_NON_FEAT_DEPENDENT_MASK) == 0) {
		return TEST_RESULT_FAIL;
	}

	CHECK_FEAT_TRAP_INITIALIZED(hfgrtr_el2, HFGRTR_EL2,		\
		is_feat_sme_supported, SME)
	CHECK_FEAT_TRAP_INITIALIZED(hfgrtr_el2, HFGRTR_EL2,		\
		is_feat_ls64_accdata_present, LS64_ACCDATA)
	CHECK_FEAT_TRAP_INITIALIZED(hfgrtr_el2, HFGRTR_EL2,		\
		is_feat_ras_present, RAS)
	CHECK_FEAT_TRAP_INITIALIZED(hfgrtr_el2, HFGRTR_EL2,		\
		is_feat_rasv1p1_present, RASV1P1)
	CHECK_FEAT_TRAP_INITIALIZED(hfgrtr_el2, HFGRTR_EL2,		\
		is_feat_gicv3_gicv4_present, GICV3)
	CHECK_FEAT_TRAP_INITIALIZED2(hfgrtr_el2, HFGRTR_EL2,		\
		is_feat_csv2_2_present, CSV2_2,				\
		is_feat_csv2_1p2_present, CSV2_1P2, ||)
	CHECK_FEAT_TRAP_INITIALIZED(hfgrtr_el2, HFGRTR_EL2,		\
		is_feat_lor_present, LOR)
	CHECK_FEAT_TRAP_INITIALIZED(hfgrtr_el2, HFGRTR_EL2,		\
		is_armv8_3_pauth_apa_api_apa3_present, PAUTH)
	if (is_init_val_set(hfgrtr_el2, HFGRTR_EL2_INIT_VAL,
		HFGRTR_EL2_NON_FEAT_DEPENDENT_MASK) == 0) {
		return TEST_RESULT_FAIL;
	}

	CHECK_FEAT_TRAP_INITIALIZED(hfgwtr_el2, HFGWTR_EL2,		\
		is_feat_sme_supported, SME)
	CHECK_FEAT_TRAP_INITIALIZED(hfgwtr_el2, HFGWTR_EL2,		\
		is_feat_ls64_accdata_present, LS64_ACCDATA);
	CHECK_FEAT_TRAP_INITIALIZED(hfgwtr_el2, HFGWTR_EL2,		\
		is_feat_ras_present, RAS);
	CHECK_FEAT_TRAP_INITIALIZED(hfgwtr_el2, HFGWTR_EL2,		\
		is_feat_rasv1p1_present, RASV1P1);
	CHECK_FEAT_TRAP_INITIALIZED(hfgwtr_el2, HFGWTR_EL2,		\
		is_feat_gicv3_gicv4_present, GICV3);
	CHECK_FEAT_TRAP_INITIALIZED2(hfgwtr_el2, HFGWTR_EL2,		\
		is_feat_csv2_2_present, CSV2_2,				\
		is_feat_csv2_1p2_present, CSV2_1P2, ||)
	CHECK_FEAT_TRAP_INITIALIZED(hfgwtr_el2, HFGWTR_EL2,		\
		is_feat_lor_present, LOR)
	CHECK_FEAT_TRAP_INITIALIZED(hfgwtr_el2, HFGWTR_EL2,		\
		is_armv8_3_pauth_apa_api_apa3_present, PAUTH)
	if (is_init_val_set(hfgwtr_el2, HFGWTR_EL2_INIT_VAL,		\
		HFGWTR_EL2_NON_FEAT_DEPENDENT_MASK) == 0) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
#endif	/* __aarch64__ */
}

test_result_t test_fgt2_enabled(void)
{
	SKIP_TEST_IF_AARCH32();


#ifdef __aarch64__
	SKIP_TEST_IF_FGT2_NOT_SUPPORTED();

	/* The following registers are read to test their presence when
	 * FEAT_FGT2 is supported
	 */

	read_hfgitr2_el2();
	read_hfgrtr2_el2();
	read_hfgwtr2_el2();
	read_hdfgrtr2_el2();
	read_hdfgwtr2_el2();

	return TEST_RESULT_SUCCESS;
#endif /* __aarch64__ */
}
