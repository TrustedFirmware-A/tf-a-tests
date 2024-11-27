/*
 * Copyright (c) 2023-2024, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>

#include <assert.h>
#include <arch_features.h>
#include <debug.h>
#include <test_helpers.h>
#include <lib/extensions/fpu.h>
#include <lib/extensions/sme.h>
#include <lib/extensions/sve.h>

#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_realm_simd.h>
#include <host_shared_data.h>

#include "host_realm_simd_common.h"

#define NS_SVE_OP_ARRAYSIZE		1024U
#define SVE_TEST_ITERATIONS		50U

static int ns_sve_op_1[NS_SVE_OP_ARRAYSIZE];
static int ns_sve_op_2[NS_SVE_OP_ARRAYSIZE];

/* Defined in host_realm_simd_common.c */
extern sve_z_regs_t ns_sve_z_regs_write;
extern sve_z_regs_t ns_sve_z_regs_read;

static struct realm realm;

/* Skip test if SVE is not supported in H/W or in RMI features */
#define CHECK_SVE_SUPPORT_IN_HW_AND_IN_RMI(_reg0)				\
	do {									\
		SKIP_TEST_IF_SVE_NOT_SUPPORTED();				\
										\
		/* Get RMM support for SVE and its max SVE VL */		\
		if (host_rmi_features(0UL, &_reg0) != REALM_SUCCESS) {		\
			ERROR("Failed to get RMI feat_reg0\n");			\
			return TEST_RESULT_FAIL;				\
		}								\
										\
		/* SVE not supported in RMI features? */			\
		if ((_reg0 & RMI_FEATURE_REGISTER_0_SVE_EN) == 0UL) {		\
			ERROR("SVE not in RMI features, skipping\n");		\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (false)

/*
 * RMI should report SVE VL in RMI features and it must be the same value as the
 * max SVE VL seen by the NS world.
 */
test_result_t host_check_rmi_reports_proper_sve_vl(void)
{
	u_register_t rmi_feat_reg0;
	uint8_t rmi_sve_vq;
	uint8_t ns_sve_vq;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	CHECK_SVE_SUPPORT_IN_HW_AND_IN_RMI(rmi_feat_reg0);

	rmi_sve_vq = EXTRACT(RMI_FEATURE_REGISTER_0_SVE_VL, rmi_feat_reg0);

	/*
	 * Configure NS to arch supported max VL and get the value reported
	 * by rdvl
	 */
	sve_config_vq(SVE_VQ_ARCH_MAX);
	ns_sve_vq = SVE_VL_TO_VQ(sve_rdvl_1());

	if (rmi_sve_vq != ns_sve_vq) {
		ERROR("RMI max SVE VL %u bits don't match NS max "
		      "SVE VL %u bits\n", SVE_VQ_TO_BITS(rmi_sve_vq),
		      SVE_VQ_TO_BITS(ns_sve_vq));
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/* Test Realm creation with SVE enabled and run command rdvl */
test_result_t host_sve_realm_cmd_rdvl(void)
{
	host_shared_data_t *sd;
	struct sve_cmd_rdvl *rl_output;
	uint8_t sve_vq, rl_max_sve_vq;
	u_register_t rmi_feat_reg0;
	test_result_t rc;
	bool realm_rc;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	CHECK_SVE_SUPPORT_IN_HW_AND_IN_RMI(rmi_feat_reg0);

	sve_vq = EXTRACT(RMI_FEATURE_REGISTER_0_SVE_VL, rmi_feat_reg0);

	rc = host_create_sve_realm_payload(&realm, true, sve_vq);
	if (rc != TEST_RESULT_SUCCESS) {
		ERROR("Failed to create Realm with SVE\n");
		return TEST_RESULT_FAIL;
	}

	realm_rc = host_enter_realm_execute(&realm, REALM_SVE_RDVL,
					    RMI_EXIT_HOST_CALL, 0U);
	if (realm_rc != true) {
		rc = TEST_RESULT_FAIL;
		goto rm_realm;
	}

	/* Check if rdvl matches the SVE VL created */
	sd = host_get_shared_structure(&realm, PRIMARY_PLANE_ID, 0U);
	rl_output = (struct sve_cmd_rdvl *)sd->realm_cmd_output_buffer;
	rl_max_sve_vq = SVE_VL_TO_VQ(rl_output->rdvl);
	if (sve_vq == rl_max_sve_vq) {
		rc = TEST_RESULT_SUCCESS;
	} else {
		ERROR("Realm created with max VL: %u bits, but Realm reported "
		      "max VL as: %u bits\n", SVE_VQ_TO_BITS(sve_vq),
		      SVE_VQ_TO_BITS(rl_max_sve_vq));
		rc = TEST_RESULT_FAIL;
	}

rm_realm:
	if (!host_destroy_realm(&realm)) {
		return TEST_RESULT_FAIL;
	}

	return rc;
}

/* Test Realm creation with SVE enabled but with invalid SVE VL */
test_result_t host_sve_realm_test_invalid_vl(void)
{
	u_register_t rmi_feat_reg0;
	test_result_t rc;
	uint8_t sve_vq;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	CHECK_SVE_SUPPORT_IN_HW_AND_IN_RMI(rmi_feat_reg0);

	sve_vq = EXTRACT(RMI_FEATURE_REGISTER_0_SVE_VL, rmi_feat_reg0);

	/*
	 * Pass a sve_vq that is greater than the value supported by RMM
	 * and check whether creating Realm fails
	 */
	rc = host_create_sve_realm_payload(&realm, true, (sve_vq + 1));
	if (rc == TEST_RESULT_SUCCESS) {
		ERROR("Error: Realm created with invalid SVE VL %u\n", (sve_vq + 1));
		host_destroy_realm(&realm);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

static test_result_t _host_sve_realm_check_id_registers(bool sve_en)
{
	host_shared_data_t *sd;
	struct sve_cmd_id_regs *r_regs;
	u_register_t rmi_feat_reg0;
	test_result_t rc;
	bool realm_rc;
	uint8_t sve_vq = 0U;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (sve_en) {
		CHECK_SVE_SUPPORT_IN_HW_AND_IN_RMI(rmi_feat_reg0);
		sve_vq = EXTRACT(RMI_FEATURE_REGISTER_0_SVE_VL, rmi_feat_reg0);
	}

	rc = host_create_sve_realm_payload(&realm, sve_en, sve_vq);
	if (rc != TEST_RESULT_SUCCESS) {
		return rc;
	}

	realm_rc = host_enter_realm_execute(&realm, REALM_SVE_ID_REGISTERS,
					    RMI_EXIT_HOST_CALL, 0U);
	if (!realm_rc) {
		rc = TEST_RESULT_FAIL;
		goto rm_realm;
	}

	sd = host_get_shared_structure(&realm, PRIMARY_PLANE_ID, 0U);
	r_regs = (struct sve_cmd_id_regs *)sd->realm_cmd_output_buffer;

	/* Check ID register SVE flags */
	if (sve_en) {
		rc = TEST_RESULT_SUCCESS;
		if (EXTRACT(ID_AA64PFR0_SVE, r_regs->id_aa64pfr0_el1) == 0UL) {
			ERROR("ID_AA64PFR0_EL1: SVE not enabled\n");
			rc = TEST_RESULT_FAIL;
		}
		if (r_regs->id_aa64zfr0_el1 == 0UL) {
			ERROR("ID_AA64ZFR0_EL1: No SVE features present\n");
			rc = TEST_RESULT_FAIL;
		}
	} else {
		rc = TEST_RESULT_SUCCESS;
		if (EXTRACT(ID_AA64PFR0_SVE, r_regs->id_aa64pfr0_el1) != 0UL) {
			ERROR("ID_AA64PFR0_EL1: SVE enabled\n");
			rc = TEST_RESULT_FAIL;
		}
		if (r_regs->id_aa64zfr0_el1 != 0UL) {
			ERROR("ID_AA64ZFR0_EL1: Realm reported non-zero value\n");
			rc = TEST_RESULT_FAIL;
		}
	}

rm_realm:
	host_destroy_realm(&realm);
	return rc;
}

/* Test ID_AA64PFR0_EL1, ID_AA64ZFR0_EL1_SVE values in SVE Realm */
test_result_t host_sve_realm_cmd_id_registers(void)
{
	return _host_sve_realm_check_id_registers(true);
}

/* Test ID_AA64PFR0_EL1, ID_AA64ZFR0_EL1_SVE values in non SVE Realm */
test_result_t host_non_sve_realm_cmd_id_registers(void)
{
	return _host_sve_realm_check_id_registers(false);
}

static void print_sve_vl_bitmap(uint32_t vl_bitmap)
{
	for (uint8_t vq = 0U; vq <= SVE_VQ_ARCH_MAX; vq++) {
		if ((vl_bitmap & BIT_32(vq)) != 0U) {
			INFO("\t%u\n", SVE_VQ_TO_BITS(vq));
		}
	}
}

/* Create SVE Realm and probe all the supported VLs */
test_result_t host_sve_realm_cmd_probe_vl(void)
{
	host_shared_data_t *sd;
	struct sve_cmd_probe_vl *rl_output;
	uint32_t vl_bitmap_expected;
	u_register_t rmi_feat_reg0;
	test_result_t rc;
	bool realm_rc;
	uint8_t sve_vq;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	CHECK_SVE_SUPPORT_IN_HW_AND_IN_RMI(rmi_feat_reg0);

	sve_vq = EXTRACT(RMI_FEATURE_REGISTER_0_SVE_VL, rmi_feat_reg0);

	rc = host_create_sve_realm_payload(&realm, true, sve_vq);
	if (rc != TEST_RESULT_SUCCESS) {
		return rc;
	}

	/*
	 * Configure TFTF with sve_vq and probe all VLs and compare it with
	 * the bitmap returned from Realm
	 */
	vl_bitmap_expected = sve_probe_vl(sve_vq);

	realm_rc = host_enter_realm_execute(&realm, REALM_SVE_PROBE_VL,
					    RMI_EXIT_HOST_CALL, 0U);
	if (!realm_rc) {
		rc = TEST_RESULT_FAIL;
		goto rm_realm;
	}

	sd = host_get_shared_structure(&realm, PRIMARY_PLANE_ID, 0U);
	rl_output = (struct sve_cmd_probe_vl *)sd->realm_cmd_output_buffer;

	INFO("Supported SVE vector length in bits (expected):\n");
	print_sve_vl_bitmap(vl_bitmap_expected);

	INFO("Supported SVE vector length in bits (probed):\n");
	print_sve_vl_bitmap(rl_output->vl_bitmap);

	if (vl_bitmap_expected == rl_output->vl_bitmap) {
		rc = TEST_RESULT_SUCCESS;
	} else {
		rc = TEST_RESULT_FAIL;
	}

rm_realm:
	if (!host_destroy_realm(&realm)) {
		return TEST_RESULT_FAIL;
	}

	return rc;
}

/* Check whether RMM preserves NS ZCR_EL2 register. */
test_result_t host_sve_realm_check_config_register(void)
{
	u_register_t ns_zcr_el2, ns_zcr_el2_cur;
	u_register_t rmi_feat_reg0;
	test_result_t rc;
	uint8_t vq;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	CHECK_SVE_SUPPORT_IN_HW_AND_IN_RMI(rmi_feat_reg0);

	vq = EXTRACT(RMI_FEATURE_REGISTER_0_SVE_VL, rmi_feat_reg0);

	rc = host_create_sve_realm_payload(&realm, true, vq);
	if (rc != TEST_RESULT_SUCCESS) {
		return rc;
	}

	/*
	 * Configure TFTF from 0 to SVE_VQ_ARCH_MAX, and in each iteration check
	 * if NS ZCR_EL2 is same before and after call to run Realm.
	 */
	rc = TEST_RESULT_SUCCESS;
	for (vq = 0U; vq <= SVE_VQ_ARCH_MAX; vq++) {
		bool realm_rc;

		sve_config_vq(vq);
		ns_zcr_el2 = read_zcr_el2();

		/* Call Realm to run SVE command */
		realm_rc = host_enter_realm_execute(&realm, REALM_SVE_RDVL,
						    RMI_EXIT_HOST_CALL, 0U);
		if (!realm_rc) {
			ERROR("Realm command REALM_SVE_RDVL failed\n");
			rc = TEST_RESULT_FAIL;
			break;
		}
		ns_zcr_el2_cur = read_zcr_el2();

		if (ns_zcr_el2 != ns_zcr_el2_cur) {
			ERROR("NS ZCR_EL2 expected: 0x%lx, got: 0x%lx\n",
			      ns_zcr_el2, ns_zcr_el2_cur);
			rc = TEST_RESULT_FAIL;
		}
	}

	if (!host_destroy_realm(&realm)) {
		return TEST_RESULT_FAIL;
	}

	return rc;
}

/*
 * Sends command to Realm to do SVE operations, while NS is also doing SVE
 * operations.
 * Returns:
 *	false - On success
 *	true  - On failure
 */
static bool callback_realm_do_sve(void)
{

	return !host_enter_realm_execute(&realm, REALM_SVE_OPS,
					 RMI_EXIT_HOST_CALL, 0U);
}

/*
 * Sends command to Realm to do SVE operations, while NS is also doing SVE
 * operations.
 * Returns:
 *	false - On success
 *	true  - On failure
 */
static bool callback_realm_do_fpu(void)
{
	return !host_enter_realm_execute(&realm, REALM_REQ_FPU_FILL_CMD,
					 RMI_EXIT_HOST_CALL, 0U);
}

static test_result_t run_sve_vectors_operations(bool realm_sve_en,
						uint8_t realm_sve_vq,
						int ns_sve_mode)
{
	bool (*realm_callback)(void);
	test_result_t rc;
	bool cb_err;
	unsigned int i;
	int val;

	rc = host_create_sve_realm_payload(&realm, realm_sve_en, realm_sve_vq);
	if (rc != TEST_RESULT_SUCCESS) {
		return rc;
	}

	/* Get at random value to do sve_subtract */
	val = rand();
	for (i = 0U; i < NS_SVE_OP_ARRAYSIZE; i++) {
		ns_sve_op_1[i] = val - i;
		ns_sve_op_2[i] = 1;
	}

	if (realm_sve_en) {
		realm_callback = callback_realm_do_sve;
	} else {
		realm_callback = callback_realm_do_fpu;
	}

	for (i = 0U; i < SVE_TEST_ITERATIONS; i++) {
		/* Config NS world with random SVE VL or SVE SVL */
		if (ns_sve_mode == NS_NORMAL_SVE) {
			sve_config_vq(SVE_GET_RANDOM_VQ);
		} else {
			sme_config_svq(SME_GET_RANDOM_SVQ);
		}

		/* Perform SVE operations with intermittent calls to Realm */
		cb_err = sve_subtract_arrays_interleaved(ns_sve_op_1,
							 ns_sve_op_1,
							 ns_sve_op_2,
							 NS_SVE_OP_ARRAYSIZE,
							 realm_callback);
		if (cb_err) {
			ERROR("Callback to realm failed\n");
			rc = TEST_RESULT_FAIL;
			goto rm_realm;
		}
	}

	/* Check result of SVE operations. */
	rc = TEST_RESULT_SUCCESS;

	for (i = 0U; i < NS_SVE_OP_ARRAYSIZE; i++) {
		if (ns_sve_op_1[i] != (val - i - SVE_TEST_ITERATIONS)) {
			ERROR("%s op failed at idx: %u, expected: 0x%x received:"
			      " 0x%x\n", (ns_sve_mode == NS_NORMAL_SVE) ?
			      "SVE" : "SVE", i,
			      (val - i - SVE_TEST_ITERATIONS), ns_sve_op_1[i]);
			rc = TEST_RESULT_FAIL;
		}
	}

rm_realm:
	if (!host_destroy_realm(&realm)) {
		return TEST_RESULT_FAIL;
	}

	return rc;
}

/*
 * Intermittently switch to Realm while doing NS is doing SVE ops in Normal
 * SVE mode.
 *
 * This testcase runs for SVE only config or SVE + SME config
 */
test_result_t host_sve_realm_check_vectors_operations(void)
{
	u_register_t rmi_feat_reg0;
	uint8_t realm_sve_vq;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();
	CHECK_SVE_SUPPORT_IN_HW_AND_IN_RMI(rmi_feat_reg0);

	realm_sve_vq = EXTRACT(RMI_FEATURE_REGISTER_0_SVE_VL, rmi_feat_reg0);

	/* Run SVE operations in Normal SVE mode */
	return run_sve_vectors_operations(true, realm_sve_vq, NS_NORMAL_SVE);
}

/*
 * Intermittently switch to Realm while doing NS is doing SVE ops in Streaming
 * SVE mode
 *
 * This testcase runs for SME only config or SVE + SME config
 */
test_result_t host_sve_realm_check_streaming_vectors_operations(void)
{
	u_register_t rmi_feat_reg0;
	test_result_t rc;
	uint8_t realm_sve_vq;
	bool realm_sve_en;

	SKIP_TEST_IF_SME_NOT_SUPPORTED();
	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_armv8_2_sve_present()) {
		CHECK_SVE_SUPPORT_IN_HW_AND_IN_RMI(rmi_feat_reg0);
		realm_sve_en = true;
		realm_sve_vq = EXTRACT(RMI_FEATURE_REGISTER_0_SVE_VL,
				       rmi_feat_reg0);
	} else {
		realm_sve_en = 0;
		realm_sve_vq = 0;
	}

	/* Enter Streaming SVE mode */
	sme_smstart(SMSTART_SM);

	/* Run SVE operations in Streaming SVE mode */
	rc = run_sve_vectors_operations(realm_sve_en, realm_sve_vq,
					NS_STREAMING_SVE);

	/* Exit Streaming SVE mode */
	sme_smstop(SMSTOP_SM);

	return rc;
}

/*
 * Check if RMM leaks Realm SVE registers.
 * This test is skipped if the supported max VQ is 128 bits, as we won't be able
 * to run NS and Realm context with lower and higher VQ respectively.
 * This test does the below steps:
 *
 * 1. Set NS SVE VQ to max and write known pattern
 * 2. NS programs ZCR_EL2 with VQ as 0 (128 bits).
 * 3. Create Realm with max VQ (higher than NS SVE VQ).
 * 4. Call Realm to fill in Z registers
 * 5. Once Realm returns, NS sets ZCR_EL2 with max VQ and reads the Z registers
 * 6. The upper bits of Z registers must be either 0 or the old values filled by
 *    NS world at step 1.
 */
test_result_t host_sve_realm_check_vectors_leaked(void)
{
	u_register_t rmi_feat_reg0;
	test_result_t rc;
	uint64_t bitmap;
	bool realm_rc;
	uint8_t sve_vq;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	CHECK_SVE_SUPPORT_IN_HW_AND_IN_RMI(rmi_feat_reg0);

	sve_vq = EXTRACT(RMI_FEATURE_REGISTER_0_SVE_VL, rmi_feat_reg0);

	/* Skip test if the supported max VQ is 128 bits */
	if (sve_vq == SVE_VQ_ARCH_MIN) {
		return TEST_RESULT_SKIPPED;
	}

	/* 1. Set NS SVE VQ to max and write known pattern */
	sve_config_vq(sve_vq);
	(void)memset((void *)&ns_sve_z_regs_write, 0xAA,
		     SVE_VQ_TO_BYTES(sve_vq) * SVE_NUM_VECTORS);
	sve_z_regs_write(&ns_sve_z_regs_write);

	/* 2. NS programs ZCR_EL2 with VQ as 0 */
	sve_config_vq(SVE_VQ_ARCH_MIN);

	/* 3. Create Realm with max VQ (higher than NS SVE VQ) */
	rc = host_create_sve_realm_payload(&realm, true, sve_vq);
	if (rc != TEST_RESULT_SUCCESS) {
		return rc;
	}

	/* 4. Call Realm to fill in Z registers */
	realm_rc = host_enter_realm_execute(&realm, REALM_SVE_FILL_REGS,
					    RMI_EXIT_HOST_CALL, 0U);
	if (!realm_rc) {
		rc = TEST_RESULT_FAIL;
		goto rm_realm;
	}

	/* 5. NS sets ZCR_EL2 with max VQ and reads the Z registers */
	sve_config_vq(sve_vq);
	sve_z_regs_read(&ns_sve_z_regs_read);

	/*
	 * 6. The upper bits in Z vectors (sve_vq - SVE_VQ_ARCH_MIN) must
	 *    be either 0 or the old values filled by NS world.
	 *    TODO: check if upper bits are zero
	 */
	bitmap = sve_z_regs_compare(&ns_sve_z_regs_write, &ns_sve_z_regs_read);
	if (bitmap != 0UL) {
		ERROR("SVE Z regs compare failed (bitmap: 0x%016llx)\n",
		      bitmap);
		rc = TEST_RESULT_FAIL;
	} else {
		rc = TEST_RESULT_SUCCESS;
	}

rm_realm:
	if (!host_destroy_realm(&realm)) {
		return TEST_RESULT_FAIL;
	}

	return rc;
}

/*
 * Create a non SVE Realm and try to access SVE, the Realm must receive
 * undefined abort.
 */
test_result_t host_non_sve_realm_check_undef_abort(void)
{
	test_result_t rc;
	bool realm_rc;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();
	SKIP_TEST_IF_SVE_NOT_SUPPORTED();

	rc = host_create_sve_realm_payload(&realm, false, 0);
	if (rc != TEST_RESULT_SUCCESS) {
		return rc;
	}

	realm_rc = host_enter_realm_execute(&realm, REALM_SVE_UNDEF_ABORT,
					    RMI_EXIT_HOST_CALL, 0U);
	if (!realm_rc) {
		ERROR("Realm didn't receive undefined abort\n");
		rc = TEST_RESULT_FAIL;
	} else {
		rc = TEST_RESULT_SUCCESS;
	}

	if (!host_destroy_realm(&realm)) {
		return TEST_RESULT_FAIL;
	}

	return rc;
}

/*
 * Create a Realm and check SME specific ID registers. Realm must report SME
 * not present in ID_AA64PFR1_EL1 and no SME features present in
 * ID_AA64SMFR0_EL1
 */
test_result_t host_realm_check_sme_id_registers(void)
{
	host_shared_data_t *sd;
	struct sme_cmd_id_regs *r_regs;
	test_result_t rc;
	bool realm_rc;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	rc = host_create_sve_realm_payload(&realm, false, 0);
	if (rc != TEST_RESULT_SUCCESS) {
		return rc;
	}

	realm_rc = host_enter_realm_execute(&realm, REALM_SME_ID_REGISTERS,
					    RMI_EXIT_HOST_CALL, 0U);
	if (!realm_rc) {
		rc = TEST_RESULT_FAIL;
		goto rm_realm;
	}

	sd = host_get_shared_structure(&realm, PRIMARY_PLANE_ID, 0U);
	r_regs = (struct sme_cmd_id_regs *)sd->realm_cmd_output_buffer;

	/* Check ID register SME flags */
	rc = TEST_RESULT_SUCCESS;
	if (EXTRACT(ID_AA64PFR1_EL1_SME, r_regs->id_aa64pfr1_el1) >=
	    ID_AA64PFR1_EL1_SME_SUPPORTED) {
		ERROR("ID_AA64PFR1_EL1: SME enabled\n");
		rc = TEST_RESULT_FAIL;
	}
	if (r_regs->id_aa64smfr0_el1 != 0UL) {
		ERROR("ID_AA64SMFR0_EL1: Realm reported non-zero value\n");
		rc = TEST_RESULT_FAIL;
	}

rm_realm:
	host_destroy_realm(&realm);
	return rc;
}

/*
 * Create a Realm and try to access SME, the Realm must receive undefined abort.
 */
test_result_t host_realm_check_sme_undef_abort(void)
{
	test_result_t rc;
	bool realm_rc;

	SKIP_TEST_IF_SME_NOT_SUPPORTED();
	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	rc = host_create_sve_realm_payload(&realm, false, 0);
	if (rc != TEST_RESULT_SUCCESS) {
		return rc;
	}

	realm_rc = host_enter_realm_execute(&realm, REALM_SME_UNDEF_ABORT,
					    RMI_EXIT_HOST_CALL, 0U);
	if (!realm_rc) {
		ERROR("Realm didn't receive undefined abort\n");
		rc = TEST_RESULT_FAIL;
	} else {
		rc = TEST_RESULT_SUCCESS;
	}

	host_destroy_realm(&realm);
	return rc;
}

/*
 * Check whether RMM preserves NS SME config values and flags
 * 1. SMCR_EL2.LEN field
 * 2. SMCR_EL2.FA64 flag
 * 3. Streaming SVE mode status
 *
 * This test case runs for SVE + SME config and SME only config and skipped for
 * non SME config.
 */
test_result_t host_realm_check_sme_configs(void)
{
	u_register_t ns_smcr_el2, ns_smcr_el2_cur;
	u_register_t rmi_feat_reg0;
	bool ssve_mode;
	test_result_t rc;
	uint8_t sve_vq;
	uint8_t sme_svq;
	bool sve_en;

	SKIP_TEST_IF_SME_NOT_SUPPORTED();
	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_armv8_2_sve_present()) {
		CHECK_SVE_SUPPORT_IN_HW_AND_IN_RMI(rmi_feat_reg0);
		sve_en = true;
		sve_vq = EXTRACT(RMI_FEATURE_REGISTER_0_SVE_VL, rmi_feat_reg0);
	} else {
		sve_en = false;
		sve_vq = 0;
	}

	rc = host_create_sve_realm_payload(&realm, sve_en, sve_vq);
	if (rc != TEST_RESULT_SUCCESS) {
		return rc;
	}

	/*
	 * Configure TFTF from 0 to SME_SVQ_ARCH_MAX, and in each iteration
	 * randomly enable or disable FA64 and Streaming SVE mode. Ater calling
	 * Realm, check the NS SME configuration status.
	 */
	rc = TEST_RESULT_SUCCESS;
	for (sme_svq = 0U; sme_svq <= SME_SVQ_ARCH_MAX; sme_svq++) {
		bool realm_rc;

		sme_config_svq(sme_svq);

		/* randomly enable or disable FEAT_SME_FA64 */
		if (sme_svq % 2) {
			sme_enable_fa64();
			sme_smstart(SMSTART_SM);
			ssve_mode = true;
		} else {
			sme_disable_fa64();
			sme_smstop(SMSTOP_SM);
			ssve_mode = false;
		}

		ns_smcr_el2 = read_smcr_el2();

		/*
		 * If SVE is supported then we would have created a Realm with
		 * SVE support, so run SVE command else run FPU command
		 */
		if (sve_en) {
			realm_rc = host_enter_realm_execute(&realm, REALM_SVE_RDVL,
							    RMI_EXIT_HOST_CALL,
							    0U);
		} else {
			realm_rc = host_enter_realm_execute(&realm,
							REALM_REQ_FPU_FILL_CMD,
							RMI_EXIT_HOST_CALL, 0U);
		}

		if (!realm_rc) {
			ERROR("Realm command REALM_SVE_RDVL failed\n");
			rc = TEST_RESULT_FAIL;
			break;
		}
		ns_smcr_el2_cur = read_smcr_el2();

		if (ns_smcr_el2 != ns_smcr_el2_cur) {
			ERROR("NS SMCR_EL2 expected: 0x%lx, got: 0x%lx\n",
			      ns_smcr_el2, ns_smcr_el2_cur);
			rc = TEST_RESULT_FAIL;
		}

		if (sme_smstat_sm() != ssve_mode) {
			if (ssve_mode) {
				ERROR("NS Streaming SVE mode is disabled\n");
			} else {
				ERROR("NS Streaming SVE mode is enabled\n");
			}

			rc = TEST_RESULT_FAIL;
		}
	}

	/* Exit Streaming SVE mode if test case enabled it */
	if (ssve_mode) {
		sme_smstop(SMSTOP_SM);
	}

	if (!host_destroy_realm(&realm)) {
		return TEST_RESULT_FAIL;
	}

	return rc;
}
