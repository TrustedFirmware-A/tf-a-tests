/*
 * Copyright (c) 2021-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cactus_test_cmds.h>
#include <ffa_endpoints.h>
#include <ffa_helpers.h>
#include <fpu.h>
#include <spm_test_helpers.h>
#include <test_helpers.h>
#include <lib/extensions/sve.h>

#define SENDER HYP_ID
#define RECEIVER SP_ID(1)
#define SVE_TEST_ITERATIONS	100
#define NS_SVE_OP_ARRAYSIZE		1024

static const struct ffa_uuid expected_sp_uuids[] = { {PRIMARY_UUID} };

static sve_z_regs_t sve_vectors_input;
static sve_z_regs_t sve_vectors_output;
static sve_p_regs_t sve_predicates_input;
static sve_p_regs_t sve_predicates_output;
static sve_ffr_regs_t sve_ffr_input;
static sve_ffr_regs_t sve_ffr_output;
static int sve_op_1[NS_SVE_OP_ARRAYSIZE];
static int sve_op_2[NS_SVE_OP_ARRAYSIZE];
static fpu_state_t g_fpu_state_write;
static fpu_state_t g_fpu_state_read;

/*
 * Tests that SIMD vectors and FPU state are preserved during the context switches between
 * normal world and the secure world.
 * Fills the SIMD vectors, FPCR and FPSR with random values, requests SP to fill the vectors
 * with a different values, request SP to check if secure SIMD context is restored.
 * Checks that the NS context is restored on return.
 */
test_result_t test_simd_vectors_preserved(void)
{
	/**********************************************************************
	 * Verify that FF-A is there and that it has the correct version.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

	fpu_state_write_rand(&g_fpu_state_write);
	struct ffa_value ret = cactus_req_simd_fill_send_cmd(SENDER, RECEIVER);

	if (!is_ffa_direct_response(ret)) {
		return TEST_RESULT_FAIL;
	}

	if (cactus_get_response(ret) == CACTUS_ERROR) {
		return TEST_RESULT_FAIL;
	}

	ret = cactus_req_simd_compare_send_cmd(SENDER, RECEIVER);

	if (!is_ffa_direct_response(ret)) {
		return TEST_RESULT_FAIL;
	}

	if (cactus_get_response(ret) == CACTUS_ERROR) {
		return TEST_RESULT_FAIL;
	}

	/* Normal world verify its FPU/SIMD state registers data */
	fpu_state_read(&g_fpu_state_read);
	if (fpu_state_compare(&g_fpu_state_write, &g_fpu_state_read) != 0) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

static test_result_t test_sve_vectors_preserved_impl(uint8_t vq)
{
	uint8_t *sve_vector;
	uint64_t vl;

	/* Configure requested VL. */
	sve_config_vq(vq);

	/*
	 * Clear SVE vectors buffers used to compare the SVE state before calling
	 * into the Swd compared to SVE state restored after returning to NWd.
	 */
	memset(sve_vectors_input, 0, sizeof(sve_vectors_input));
	memset(sve_vectors_output, 0, sizeof(sve_vectors_output));

	/* Vector length in bytes from vq. */
	vl = SVE_VQ_TO_BYTES(vq);

	/* Fill each vector for the VL size with a fixed pattern. */
	sve_vector = (uint8_t *) sve_vectors_input;

	for (uint32_t vector_num = 0U; vector_num < SVE_NUM_VECTORS; vector_num++) {
		memset(sve_vector, 0x11 * (vector_num + 1), vl);
		sve_vector += vl;
	}

	/* Fill SVE vector registers with the buffer contents prepared above. */
	sve_z_regs_write(&sve_vectors_input);

	/*
	 * Call cactus secure partition which uses SIMD (and expect it doesn't
	 * affect the normal world state on return).
	 */
	struct ffa_value ret = cactus_req_simd_fill_send_cmd(SENDER, RECEIVER);

	if (!is_ffa_direct_response(ret)) {
		return TEST_RESULT_FAIL;
	}

	if (cactus_get_response(ret) == CACTUS_ERROR) {
		return TEST_RESULT_FAIL;
	}

	/* Check ZCR_EL2 was preserved. */
	if (sve_read_zcr_elx() != vq) {
		return TEST_RESULT_FAIL;
	}

	/* Get the SVE vectors state after returning to normal world. */
	sve_z_regs_read(&sve_vectors_output);

	/* Compare to state before calling into secure world. */
	if (sve_z_regs_compare(&sve_vectors_input, &sve_vectors_output) != 0UL) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

static test_result_t helper_test_sve(test_result_t (*func)(uint8_t vq))
{
	uint32_t bitmap, vl_bitmap;
	uint32_t vq = 0;
	test_result_t ret;

	SKIP_TEST_IF_SVE_NOT_SUPPORTED();

	/**********************************************************************
	 * Verify that FF-A is there and that it has the correct version.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

	/*
	 * Check SVE state is preserved normal/secure accross world switches for
	 * the discovered vector lengths.
	 */
	vl_bitmap = sve_probe_vl(SVE_VQ_ARCH_MAX);
	for (bitmap = vl_bitmap; bitmap != 0U; bitmap >>= 1) {
		if ((bitmap & 1) != 0) {
			VERBOSE("Test VL %u bits.\n", SVE_VQ_TO_BITS(vq));

			ret = func(vq);
			if (ret != TEST_RESULT_SUCCESS) {
				return ret;
			}
		}
		vq++;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * Tests that SVE vectors are preserved during the context switches between
 * normal world and the secure world.
 * Probe all possible vector length that the platform implements.
 * For each vector length value:
 * -Fill SVE vectors with known values.
 * -Request SP to fill NEON vectors with different values.
 * -Check the SVE context is restored on return.
 */
test_result_t test_sve_vectors_preserved(void)
{
	return helper_test_sve(test_sve_vectors_preserved_impl);
}

/*
 * Sends SIMD fill command to Cactus SP
 * Returns:
 *	false - On success
 *	true  - On failure
 */
static bool callback_enter_cactus_sp(void)
{
	struct ffa_value ret = cactus_req_simd_fill_send_cmd(SENDER, RECEIVER);

	if (!is_ffa_direct_response(ret)) {
		return true;
	}

	if (cactus_get_response(ret) == CACTUS_ERROR) {
		return true;
	}

	return false;
}

/*
 * Tests that SVE vector operations in normal world are not affected by context
 * switches between normal world and the secure world.
 */
test_result_t test_sve_vectors_operations(void)
{
	unsigned int val;
	bool cb_err;

	SKIP_TEST_IF_SVE_NOT_SUPPORTED();

	/**********************************************************************
	 * Verify that FF-A is there and that it has the correct version.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

	val = 2 * SVE_TEST_ITERATIONS;

	for (unsigned int i = 0; i < NS_SVE_OP_ARRAYSIZE; i++) {
		sve_op_1[i] = val;
		sve_op_2[i] = 1;
	}

	/* Set ZCR_EL2.LEN to implemented VL (constrained by EL3). */
	write_zcr_el2(SVE_VQ_ARCH_MAX);
	isb();

	for (unsigned int i = 0; i < SVE_TEST_ITERATIONS; i++) {
		/* Perform SVE operations with intermittent calls to Swd. */
		cb_err = sve_subtract_arrays_interleaved(sve_op_1, sve_op_1,
							 sve_op_2,
							 NS_SVE_OP_ARRAYSIZE,
							 &callback_enter_cactus_sp);
		if (cb_err == true) {
			ERROR("Callback to Cactus SP failed\n");
			return TEST_RESULT_FAIL;
		}

	}

	/* Check result of SVE operations. */
	for (unsigned int i = 0; i < NS_SVE_OP_ARRAYSIZE; i++) {
		if (sve_op_1[i] != (val - SVE_TEST_ITERATIONS)) {
			return TEST_RESULT_FAIL;
		}
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * This base function probes all possible Streaming SVE vector length
 * values and calls the function passed as parameter with each discovered
 * value.
 */
static test_result_t helper_test_sme(test_result_t (*func)(uint8_t svq))
{
	uint32_t svl_bitmap;
	uint8_t svq;
	test_result_t ret;

	SKIP_TEST_IF_AARCH32();

	/* Skip the test if SME is not supported. */
	SKIP_TEST_IF_SME_NOT_SUPPORTED();

	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

	svl_bitmap = sme_probe_svl(SME_SVQ_ARCH_MAX);
	svq = 0;
	for (uint32_t bitmap = svl_bitmap; bitmap != 0U; bitmap >>= 1) {
		if ((bitmap & 1) != 0) {
			VERBOSE("Test SVL %u bits.\n", SVE_VQ_TO_BITS(svq));
			ret = func(svq);
			if (ret != TEST_RESULT_SUCCESS) {
				return ret;
			}
		}

		svq++;
	}

	return TEST_RESULT_SUCCESS;
}

static test_result_t do_test_sme_streaming_sve(uint8_t svq)
{
	struct ffa_value ret;

	/* Enable SME FA64 if implemented. */
	if (is_feat_sme_fa64_supported()) {
		sme_enable_fa64();
	}

	/* Enter Streaming SVE mode. */
	sme_smstart(SMSTART_SM);

	/* Configure SVQ. */
	sme_config_svq((uint32_t)svq);

	sve_z_regs_write_rand(&sve_vectors_input);
	sve_p_regs_write_rand(&sve_predicates_input);
	fpu_cs_regs_write_rand(&g_fpu_state_write.cs_regs);

	if (is_feat_sme_fa64_supported()) {
		sve_ffr_regs_write_rand(&sve_ffr_input);
	}

	ret = cactus_req_simd_fill_send_cmd(SENDER, RECEIVER);

	if (!is_expected_cactus_response(ret, CACTUS_SUCCESS, 0)) {
		return TEST_RESULT_FAIL;
	}

	if (is_feat_sme_fa64_supported()) {
		if (!sme_feat_fa64_enabled()) {
			ERROR("FA64 trap bit disabled, expected enabled.\n");
			return TEST_RESULT_FAIL;
		}
	}

	/* Expect Streaming SVE to be active. */
	if (!sme_smstat_sm()) {
		ERROR("Streaming SVE disabled, expected enabled.\n");
		return TEST_RESULT_FAIL;
	}

	sve_z_regs_read(&sve_vectors_output);
	if (sve_z_regs_compare(&sve_vectors_input, &sve_vectors_output) != 0) {
		ERROR("SME Z vectors compare failed.");
		return TEST_RESULT_FAIL;
	}

	sve_p_regs_read(&sve_predicates_output);
	if (sve_p_regs_compare(&sve_predicates_input, &sve_predicates_output) != 0) {
		ERROR("SME predicates compare failed.");
		return TEST_RESULT_FAIL;
	}

	fpu_cs_regs_read(&g_fpu_state_read.cs_regs);
	if (fpu_cs_regs_compare(&g_fpu_state_write.cs_regs, &g_fpu_state_read.cs_regs) != 0) {
		ERROR("FPU control/status compare failed.");
		return TEST_RESULT_FAIL;
	}

	if (is_feat_sme_fa64_supported()) {
		sve_ffr_regs_read(&sve_ffr_output);
		if (sve_ffr_regs_compare(&sve_ffr_input, &sve_ffr_output) != 0) {
			ERROR("SVE FFR register compare failed.");
			return TEST_RESULT_FAIL;
		}
	}

	/* Exit Streaming SVE mode. */
	sme_smstop(SMSTOP_SM);

	return TEST_RESULT_SUCCESS;
}

/*
 * SME enter SPMC with SSVE enabled.
 *
 * Check Streaming SVE is preserved on a normal/secure world switch.
 *
 */
test_result_t test_sme_streaming_sve(void)
{
	return helper_test_sme(do_test_sme_streaming_sve);
}

static test_result_t do_test_sme_za(uint8_t vq)
{
	struct ffa_value ret;

	/* Enable SME FA64 if implemented. */
	if (is_feat_sme_fa64_supported()) {
		sme_enable_fa64();
	}

	/* Enable SME ZA Array Storage */
	sme_smstart(SMSTART_ZA);

	/* Configure VQ. */
	sve_config_vq(vq);

	sve_z_regs_write_rand(&sve_vectors_input);
	sve_p_regs_write_rand(&sve_predicates_input);
	fpu_cs_regs_write_rand(&g_fpu_state_write.cs_regs);
	sve_ffr_regs_write_rand(&sve_ffr_input);

	ret = cactus_req_simd_fill_send_cmd(SENDER, RECEIVER);
	if (!is_ffa_direct_response(ret)) {
		return TEST_RESULT_FAIL;
	}

	if (cactus_get_response(ret) == CACTUS_ERROR) {
		return TEST_RESULT_FAIL;
	}

	if (is_feat_sme_fa64_supported()) {
		if (!sme_feat_fa64_enabled()) {
			ERROR("FA64 trap bit disabled, expected enabled.\n");
			return TEST_RESULT_FAIL;
		}
	}

	/* Expect Streaming SVE to be inactive. */
	if (sme_smstat_sm()) {
		ERROR("Streaming SVE enabled, expected disabled.\n");
		return TEST_RESULT_FAIL;
	}


	sve_z_regs_read(&sve_vectors_output);
	if (sve_z_regs_compare(&sve_vectors_input, &sve_vectors_output) != 0) {
		ERROR("SME Z vectors compare failed.");
		return TEST_RESULT_FAIL;
	}

	sve_p_regs_read(&sve_predicates_output);
	if (sve_p_regs_compare(&sve_predicates_input, &sve_predicates_output) != 0) {
		ERROR("SME predicates compare failed.");
		return TEST_RESULT_FAIL;
	}

	fpu_cs_regs_read(&g_fpu_state_read.cs_regs);
	if (fpu_cs_regs_compare(&g_fpu_state_write.cs_regs, &g_fpu_state_read.cs_regs) != 0) {
		ERROR("FPU control/status compare failed.");
		return TEST_RESULT_FAIL;
	}

	sve_ffr_regs_read(&sve_ffr_output);
	if (sve_ffr_regs_compare(&sve_ffr_input, &sve_ffr_output) != 0) {
		ERROR("SVE FFR register compare failed.");
		return TEST_RESULT_FAIL;
	}

	/* Disable SME ZA array storage. */
	sme_smstop(SMSTOP_ZA);

	return TEST_RESULT_SUCCESS;
}

/*
 * SME enter SPMC with ZA enabled.
 *
 * Check ZA array enabled is preserved on a normal/secure world switch.
 */
test_result_t test_sme_za(void)
{
	SKIP_TEST_IF_AARCH32();

	/* Skip the test if SME is not supported. */
	SKIP_TEST_IF_SME_NOT_SUPPORTED();

	return helper_test_sve(do_test_sme_za);
}

static test_result_t do_test_sme_streaming_sve_za(uint8_t svq)
{
	struct ffa_value ret;

	/* Enable SME FA64 if implemented. */
	if (is_feat_sme_fa64_supported()) {
		sme_enable_fa64();
	}

	/* Enable SME SSVE + ZA. */
	sme_smstart(SMSTART);

	/* Configure SVQ. */
	sme_config_svq((uint32_t)svq);

	sve_z_regs_write_rand(&sve_vectors_input);
	sve_p_regs_write_rand(&sve_predicates_input);
	fpu_cs_regs_write_rand(&g_fpu_state_write.cs_regs);

	if (is_feat_sme_fa64_supported()) {
		sve_ffr_regs_write_rand(&sve_ffr_input);
	}

	ret = cactus_req_simd_fill_send_cmd(SENDER, RECEIVER);
	if (!is_ffa_direct_response(ret)) {
		return TEST_RESULT_FAIL;
	}

	if (cactus_get_response(ret) == CACTUS_ERROR) {
		return TEST_RESULT_FAIL;
	}

	if (is_feat_sme_fa64_supported()) {
		if (!sme_feat_fa64_enabled()) {
			return TEST_RESULT_FAIL;
		}
	}

	/* Expect Streaming SVE to be active. */
	if (!sme_smstat_sm()) {
		ERROR("Streaming SVE disabled, expected enabled.\n");
		return TEST_RESULT_FAIL;
	}

	sve_z_regs_read(&sve_vectors_output);
	if (sve_z_regs_compare(&sve_vectors_input, &sve_vectors_output) != 0) {
		ERROR("SME Z vectors compare failed.");
		return TEST_RESULT_FAIL;
	}

	sve_p_regs_read(&sve_predicates_output);
	if (sve_p_regs_compare(&sve_predicates_input, &sve_predicates_output) != 0) {
		ERROR("SME predicates compare failed.");
		return TEST_RESULT_FAIL;
	}

	fpu_cs_regs_read(&g_fpu_state_read.cs_regs);
	if (fpu_cs_regs_compare(&g_fpu_state_write.cs_regs, &g_fpu_state_read.cs_regs) != 0) {
		ERROR("FPU control/status compare failed.");
		return TEST_RESULT_FAIL;
	}

	if (is_feat_sme_fa64_supported()) {
		sve_ffr_regs_read(&sve_ffr_output);
		if (sve_ffr_regs_compare(&sve_ffr_input, &sve_ffr_output) != 0) {
			ERROR("SVE FFR register compare failed.");
			return TEST_RESULT_FAIL;
		}
	}

	/* Disable SSVE + ZA. */
	sme_smstop(SMSTOP);

	return TEST_RESULT_SUCCESS;
}

/*
 * SME enter SPMC with SSVE+ZA enabled.
 *
 * Check Streaming SVE and ZA array enabled are preserved on a
 * normal/secure world switch.
 */
test_result_t test_sme_streaming_sve_za(void)
{
	return helper_test_sme(do_test_sme_streaming_sve_za);
}
