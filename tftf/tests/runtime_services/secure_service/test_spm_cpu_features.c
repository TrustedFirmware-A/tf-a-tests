/*
 * Copyright (c) 2021-2023, Arm Limited. All rights reserved.
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
	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);

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

/*
 * Tests that SVE vectors are preserved during the context switches between
 * normal world and the secure world.
 * Fills the SVE vectors with known values, requests SP to fill the vectors
 * with a different values, checks that the context is restored on return.
 */
test_result_t test_sve_vectors_preserved(void)
{
	uint64_t vl;
	uint8_t *sve_vector;

	SKIP_TEST_IF_SVE_NOT_SUPPORTED();

	/**********************************************************************
	 * Verify that FF-A is there and that it has the correct version.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);

	/*
	 * Clear SVE vectors buffers used to compare the SVE state before calling
	 * into the Swd compared to SVE state restored after returning to NWd.
	 */
	memset(sve_vectors_input, 0, sizeof(sve_vectors_input));
	memset(sve_vectors_output, 0, sizeof(sve_vectors_output));

	/* Set ZCR_EL2.LEN to implemented VL (constrained by EL3). */
	write_zcr_el2(0xf);
	isb();

	/* Get the implemented VL. */
	vl = sve_rdvl_1();

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

	/* Get the SVE vectors state after returning to normal world. */
	sve_z_regs_read(&sve_vectors_output);

	/* Compare to state before calling into secure world. */
	if (sve_z_regs_compare(&sve_vectors_input, &sve_vectors_output) != 0UL) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * Sends SIMD fill command to Cactus SP
 * Returns:
 *	false - On success
 *	true  - On failure
 */
#ifdef __aarch64__
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
#endif /* __aarch64__ */

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
	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);

	val = 2 * SVE_TEST_ITERATIONS;

	for (unsigned int i = 0; i < NS_SVE_OP_ARRAYSIZE; i++) {
		sve_op_1[i] = val;
		sve_op_2[i] = 1;
	}

	/* Set ZCR_EL2.LEN to implemented VL (constrained by EL3). */
	write_zcr_el2(0xf);
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
