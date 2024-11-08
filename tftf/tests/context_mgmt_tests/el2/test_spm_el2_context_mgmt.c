/*
 * Copyright (c) 2024 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cactus_test_cmds.h>
#include <ffa_endpoints.h>
#include <lib/context_mgmt/context_el2.h>
#include <spm_test_helpers.h>
#include <test_helpers.h>
#include <tftf.h>
#include <tftf_lib.h>

static const struct ffa_uuid expected_sp_uuids[] = {
	{PRIMARY_UUID}, {SECONDARY_UUID}, {TERTIARY_UUID}
};

/*
 * This test aims to validate EL2 system registers are restored to previously saved
 * values upon returning from context switch triggered by an FF-A direct
 * message request to an SP.
 */
test_result_t test_spm_el2_regs_ctx_mgmt(void)
{
	struct ffa_value ret;
	el2_sysregs_t ctx_original = {0}, ctx_expected = {0}, ctx_actual = {0};

	/* Check SPMC has ffa_version and expected FFA endpoints are deployed. */
	CHECK_SPMC_TESTING_SETUP(1, 0, expected_sp_uuids);

	/* Save the original values of EL2 system registers. */
	el2_save_registers(&ctx_original);

	/* Corrupt the EL2 system registers and save the expected values. */
	el2_modify_registers(&ctx_original, CORRUPT_REGS);
	el2_save_registers(&ctx_expected);

	/* Send a message to SP1 through direct messaging. */
	ret = cactus_echo_send_cmd(HYP_ID, SP_ID(1), ECHO_VAL1);

	if (cactus_get_response(ret) != CACTUS_SUCCESS ||
	    cactus_echo_get_val(ret) != ECHO_VAL1 ||
		!is_ffa_direct_response(ret)) {
		return TEST_RESULT_FAIL;
	}

	/* Save the modified values of EL2 system registers after the FF-A command. */
	el2_save_registers(&ctx_actual);

	/* Restore the EL2 system registers to their original state. */
	el2_modify_registers(&ctx_original, RESTORE_REGS);

	if (memcmp(&ctx_expected, &ctx_actual, sizeof(el2_sysregs_t))) {
		ERROR("Expected vs actual EL2 register contexts differ.\n\n");
		el2_dump_register_context("Expected", &ctx_expected);
		el2_dump_register_context("Actual", &ctx_actual);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
