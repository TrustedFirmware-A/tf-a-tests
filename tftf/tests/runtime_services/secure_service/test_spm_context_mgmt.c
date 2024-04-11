/*
 * Copyright (c) 2024 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <ffa_endpoints.h>
#include <tftf.h>
#include <tftf_lib.h>
#include <test_helpers.h>
#include <cactus_test_cmds.h>

#include <spm_test_helpers.h>

#define ECHO_VAL ULL(0xabcddcba)
#define GCR_VAL ULL(0x1ff)
#define RGSR_VAL ULL((1 << 8) | 0xf)
#define TFSR_VAL ULL(0x3)
#define TFSRE0_VAL ULL(0x3)

static const struct ffa_uuid expected_sp_uuids[] = {
		{PRIMARY_UUID}, {SECONDARY_UUID}, {TERTIARY_UUID}
	};

/*
 * This test aims to validate MTE system registers are restored to correct
 * values upon returning from context switch triggered by an FF-A direct
 * message request to an SP.
 */
test_result_t test_spm_mte_regs_ctxt_mgmt(void)
{
	struct ffa_value ret;
	uint64_t gcr_el1, rgsr_el1, tfsr_el1, tfsre0_el1;

	SKIP_TEST_IF_MTE_SUPPORT_LESS_THAN(MTE_IMPLEMENTED_ELX);

	/* Check SPMC has ffa_version and expected FFA endpoints are deployed. */
	CHECK_SPMC_TESTING_SETUP(1, 0, expected_sp_uuids);

	write_gcr_el1(GCR_VAL);
	write_rgsr_el1(RGSR_VAL);
	write_tfsr_el1(TFSR_VAL);
	write_tfsre0_el1(TFSRE0_VAL);

	/* Send a message to SP1 through direct messaging. */
	ret = cactus_echo_send_cmd(HYP_ID, SP_ID(1), ECHO_VAL);

	if (cactus_get_response(ret) != CACTUS_SUCCESS ||
	    cactus_echo_get_val(ret) != ECHO_VAL ||
		!is_ffa_direct_response(ret)) {
		return TEST_RESULT_FAIL;
	}

	gcr_el1 = read_gcr_el1();
	rgsr_el1 = read_rgsr_el1();
	tfsr_el1 = read_tfsr_el1();
	tfsre0_el1 = read_tfsre0_el1();

	if (gcr_el1 != GCR_VAL) {
		ERROR("Expected vs actual value of gcr_el1: %llx, %llx\n", GCR_VAL, gcr_el1);
		return TEST_RESULT_FAIL;
	}

	if (rgsr_el1 != RGSR_VAL) {
		ERROR("Expected vs actual value of rgsr_el1: %llx, %llx\n", RGSR_VAL, rgsr_el1);
		return TEST_RESULT_FAIL;
	}

	if (tfsr_el1 != TFSR_VAL) {
		ERROR("Expected vs actual value of tfsr_el1: %llx, %llx\n", TFSR_VAL, tfsr_el1);
		return TEST_RESULT_FAIL;
	}

	if (tfsre0_el1 != TFSRE0_VAL) {
		ERROR("Expected vs actual value of tfsre0_el1: %llx, %llx\n", TFSRE0_VAL, tfsre0_el1);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
