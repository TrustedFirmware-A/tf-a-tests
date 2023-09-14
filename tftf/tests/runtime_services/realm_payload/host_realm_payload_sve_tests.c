/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>

#include <assert.h>
#include <arch_features.h>
#include <debug.h>
#include <test_helpers.h>
#include <lib/extensions/sve.h>

#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_realm_sve.h>
#include <host_shared_data.h>

#define NS_SVE_OP_ARRAYSIZE		1024U
#define SVE_TEST_ITERATIONS		50U

static int ns_sve_op_1[NS_SVE_OP_ARRAYSIZE];
static int ns_sve_op_2[NS_SVE_OP_ARRAYSIZE];

static sve_vector_t ns_sve_vectors_write[SVE_NUM_VECTORS] __aligned(16);
static sve_vector_t ns_sve_vectors_read[SVE_NUM_VECTORS] __aligned(16);

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

static test_result_t host_create_sve_realm_payload(bool sve_en, uint8_t sve_vq)
{
	u_register_t feature_flag;

	if (sve_en) {
		feature_flag = RMI_FEATURE_REGISTER_0_SVE_EN |
				INPLACE(FEATURE_SVE_VL, sve_vq);
	} else {
		feature_flag = 0UL;
	}

	/* Initialise Realm payload */
	if (!host_create_realm_payload((u_register_t)REALM_IMAGE_BASE,
				       (u_register_t)PAGE_POOL_BASE,
				       (u_register_t)(PAGE_POOL_MAX_SIZE +
						      NS_REALM_SHARED_MEM_SIZE),
				       (u_register_t)PAGE_POOL_MAX_SIZE,
				       feature_flag)) {
		return TEST_RESULT_FAIL;
	}

	/* Create shared memory between Host and Realm */
	if (!host_create_shared_mem(NS_REALM_SHARED_MEM_BASE,
				    NS_REALM_SHARED_MEM_SIZE)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

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
	ns_sve_vq = SVE_VL_TO_VQ(sve_vector_length_get());

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

	rc = host_create_sve_realm_payload(true, sve_vq);
	if (rc != TEST_RESULT_SUCCESS) {
		ERROR("Failed to create Realm with SVE\n");
		return TEST_RESULT_FAIL;
	}

	realm_rc = host_enter_realm_execute(REALM_SVE_RDVL, NULL,
					    RMI_EXIT_HOST_CALL);
	if (realm_rc != true) {
		rc = TEST_RESULT_FAIL;
		goto rm_realm;
	}

	/* Check if rdvl matches the SVE VL created */
	sd = host_get_shared_structure();
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
	if (!host_destroy_realm()) {
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
	rc = host_create_sve_realm_payload(true, (sve_vq + 1));
	if (rc == TEST_RESULT_SUCCESS) {
		ERROR("Error: Realm created with invalid SVE VL %u\n", (sve_vq + 1));
		host_destroy_realm();
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

	rc = host_create_sve_realm_payload(sve_en, sve_vq);
	if (rc != TEST_RESULT_SUCCESS) {
		return rc;
	}

	realm_rc = host_enter_realm_execute(REALM_SVE_ID_REGISTERS, NULL,
					    RMI_EXIT_HOST_CALL);
	if (!realm_rc) {
		rc = TEST_RESULT_FAIL;
		goto rm_realm;
	}

	sd = host_get_shared_structure();
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
	host_destroy_realm();
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

	rc = host_create_sve_realm_payload(true, sve_vq);
	if (rc != TEST_RESULT_SUCCESS) {
		return rc;
	}

	/*
	 * Configure TFTF with sve_vq and probe all VLs and compare it with
	 * the bitmap returned from Realm
	 */
	vl_bitmap_expected = sve_probe_vl(sve_vq);

	realm_rc = host_enter_realm_execute(REALM_SVE_PROBE_VL, NULL,
					    RMI_EXIT_HOST_CALL);
	if (!realm_rc) {
		rc = TEST_RESULT_FAIL;
		goto rm_realm;
	}

	sd = host_get_shared_structure();
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
	if (!host_destroy_realm()) {
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

	rc = host_create_sve_realm_payload(true, vq);
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
		realm_rc = host_enter_realm_execute(REALM_SVE_RDVL, NULL,
						    RMI_EXIT_HOST_CALL);
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

	if (!host_destroy_realm()) {
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
static bool callback_enter_realm(void)
{
	return !host_enter_realm_execute(REALM_SVE_OPS, NULL,
					    RMI_EXIT_HOST_CALL);
	}

/* Intermittently switch to Realm while doing NS SVE ops */
test_result_t host_sve_realm_check_vectors_operations(void)
{
	u_register_t rmi_feat_reg0;
	test_result_t rc;
	uint8_t sve_vq;
	bool cb_err;
	unsigned int i;
	int val;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	CHECK_SVE_SUPPORT_IN_HW_AND_IN_RMI(rmi_feat_reg0);

	sve_vq = EXTRACT(RMI_FEATURE_REGISTER_0_SVE_VL, rmi_feat_reg0);

	rc = host_create_sve_realm_payload(true, sve_vq);
	if (rc != TEST_RESULT_SUCCESS) {
		return rc;
	}

	/* Get at random value to do sve_subtract */
	val = rand();
	for (i = 0U; i < NS_SVE_OP_ARRAYSIZE; i++) {
		ns_sve_op_1[i] = val - i;
		ns_sve_op_2[i] = 1;
	}

	for (i = 0U; i < SVE_TEST_ITERATIONS; i++) {
		/* Config NS world with random SVE length */
		sve_config_vq(SVE_GET_RANDOM_VQ);

		/* Perform SVE operations with intermittent calls to Realm */
		cb_err = sve_subtract_arrays_interleaved(ns_sve_op_1,
							 ns_sve_op_1,
							 ns_sve_op_2,
							 NS_SVE_OP_ARRAYSIZE,
							 &callback_enter_realm);
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
			ERROR("SVE op failed at idx: %u, expected: 0x%x "
			      "received: 0x%x\n", i,
			      (val - i - SVE_TEST_ITERATIONS), ns_sve_op_1[i]);
			rc = TEST_RESULT_FAIL;
		}
	}

rm_realm:
	if (!host_destroy_realm()) {
		return TEST_RESULT_FAIL;
	}

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
	uint8_t *regs_base_wr, *regs_base_rd;
	test_result_t rc;
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
	(void)memset((void *)&ns_sve_vectors_write, 0xAA,
		     SVE_VQ_TO_BYTES(sve_vq) * SVE_NUM_VECTORS);
	sve_fill_vector_regs(ns_sve_vectors_write);

	/* 2. NS programs ZCR_EL2 with VQ as 0 */
	sve_config_vq(SVE_VQ_ARCH_MIN);

	/* 3. Create Realm with max VQ (higher than NS SVE VQ) */
	rc = host_create_sve_realm_payload(true, sve_vq);
	if (rc != TEST_RESULT_SUCCESS) {
		return rc;
	}

	/* 4. Call Realm to fill in Z registers */
	realm_rc = host_enter_realm_execute(REALM_SVE_FILL_REGS, NULL,
					    RMI_EXIT_HOST_CALL);
	if (!realm_rc) {
		rc = TEST_RESULT_FAIL;
		goto rm_realm;
	}

	/* 5. NS sets ZCR_EL2 with max VQ and reads the Z registers */
	sve_config_vq(sve_vq);
	sve_read_vector_regs(ns_sve_vectors_read);

	/*
	 * 6. The upper bits in Z vectors (sve_vq - SVE_VQ_ARCH_MIN) must
	 *    be either 0 or the old values filled by NS world.
	 *    TODO: check if upper bits are zero
	 */
	regs_base_wr = (uint8_t *)&ns_sve_vectors_write;
	regs_base_rd = (uint8_t *)&ns_sve_vectors_read;

	rc = TEST_RESULT_SUCCESS;
	for (int i = 0U; i < SVE_NUM_VECTORS; i++) {
		if (memcmp(regs_base_wr + (i * SVE_VQ_TO_BYTES(sve_vq)),
			   regs_base_rd + (i * SVE_VQ_TO_BYTES(sve_vq)),
			   SVE_VQ_TO_BYTES(sve_vq)) != 0) {
			ERROR("SVE Z%d mismatch\n", i);
			rc = TEST_RESULT_FAIL;
		}
	}

rm_realm:
	if (!host_destroy_realm()) {
		return TEST_RESULT_FAIL;
	}

	return rc;
}
