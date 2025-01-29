/*
 * Copyright (c) 2018-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>
#include <smccc.h>

#include <arch_helpers.h>
#include <cactus_test_cmds.h>
#include <ffa_endpoints.h>
#include <ffa_svc.h>
#include <lib/events.h>
#include <lib/power_management.h>
#include <platform.h>
#include <spm_test_helpers.h>
#include <test_helpers.h>

#define ECHO_VAL1 U(0xa0a0a0a0)
#define ECHO_VAL2 U(0xb0b0b0b0)
#define ECHO_VAL3 U(0xc0c0c0c0)

static const struct ffa_uuid expected_sp_uuids[] = {
		{PRIMARY_UUID}, {SECONDARY_UUID}, {TERTIARY_UUID}
	};

static event_t cpu_booted[PLATFORM_CORE_COUNT];

static test_result_t send_cactus_echo_cmd(ffa_id_t sender,
					  ffa_id_t dest,
					  uint64_t value)
{
	struct ffa_value ret;
	ret = cactus_echo_send_cmd(sender, dest, value);

	/*
	 * Return responses may be FFA_MSG_SEND_DIRECT_RESP or FFA_INTERRUPT,
	 * but only expect the former. Expect SMC32 convention from SP.
	 */
	if (!is_ffa_direct_response(ret)) {
		return TEST_RESULT_FAIL;
	}

	if (cactus_get_response(ret) != CACTUS_SUCCESS ||
	    cactus_echo_get_val(ret) != value) {
		ERROR("Echo Failed!\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

test_result_t test_ffa_direct_messaging(void)
{
	test_result_t result;

	/**********************************************************************
	 * Check SPMC has ffa_version and expected FFA endpoints are deployed.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 0, expected_sp_uuids);

	/**********************************************************************
	 * Send a message to SP1 through direct messaging
	 **********************************************************************/
	result = send_cactus_echo_cmd(HYP_ID, SP_ID(1), ECHO_VAL1);
	if (result != TEST_RESULT_SUCCESS) {
		return result;
	}

	/**********************************************************************
	 * Send a message to SP2 through direct messaging
	 **********************************************************************/
	result = send_cactus_echo_cmd(HYP_ID, SP_ID(2), ECHO_VAL2);
	if (result != TEST_RESULT_SUCCESS) {
		return result;
	}

	/**********************************************************************
	 * Send a message to SP1 through direct messaging
	 **********************************************************************/
	result = send_cactus_echo_cmd(HYP_ID, SP_ID(1), ECHO_VAL3);

	return result;
}

/**
 * The 'send_cactus_req_echo_cmd' sends a CACTUS_REQ_ECHO_CMD to a cactus SP.
 * Handling this command, cactus should then send CACTUS_ECHO_CMD to
 * the specified SP according to 'echo_dest'. If the CACTUS_ECHO_CMD is resolved
 * successfully, cactus will reply to tftf with CACTUS_SUCCESS, or CACTUS_ERROR
 * otherwise.
 * For the CACTUS_SUCCESS response, the test returns TEST_RESULT_SUCCESS.
 */
static test_result_t send_cactus_req_echo_cmd(ffa_id_t sender,
					      ffa_id_t dest,
					      ffa_id_t echo_dest,
					      uint64_t value)
{
	struct ffa_value ret;

	ret = cactus_req_echo_send_cmd(sender, dest, echo_dest, value);

	if (!is_ffa_direct_response(ret)) {
		return TEST_RESULT_FAIL;
	}

	if (cactus_get_response(ret) == CACTUS_ERROR) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

test_result_t test_ffa_sp_to_sp_direct_messaging(void)
{
	test_result_t result;

	CHECK_SPMC_TESTING_SETUP(1, 0, expected_sp_uuids);

	result = send_cactus_req_echo_cmd(HYP_ID, SP_ID(1), SP_ID(2),
					  ECHO_VAL1);
	if (result != TEST_RESULT_SUCCESS) {
		return result;
	}

	/*
	 * The following the tests are intended to test the handling of a
	 * direct message request with a VM's ID as a the sender.
	 */
	result = send_cactus_req_echo_cmd(VM_ID(1), SP_ID(2), SP_ID(3),
					  ECHO_VAL2);
	if (result != TEST_RESULT_SUCCESS) {
		return result;
	}

	result = send_cactus_req_echo_cmd(VM_ID(2), SP_ID(3), SP_ID(1),
					  ECHO_VAL3);

	return result;
}

test_result_t test_ffa_sp_to_sp_deadlock(void)
{
	struct ffa_value ret;

	/**********************************************************************
	 * Check SPMC has ffa_version and expected FFA endpoints are deployed.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 0, expected_sp_uuids);

	ret = cactus_req_deadlock_send_cmd(HYP_ID, SP_ID(1), SP_ID(2), SP_ID(3));

	if (is_ffa_direct_response(ret) == false) {
		return TEST_RESULT_FAIL;
	}

	if (cactus_get_response(ret) == CACTUS_ERROR) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Handler that is passed during tftf_cpu_on to individual CPU cores.
 * Runs a specific core and send a direct message request.
 */
static test_result_t cpu_on_handler(void)
{
	unsigned int core_pos = get_current_core_id();
	test_result_t ret = TEST_RESULT_SUCCESS;
	struct ffa_value ffa_ret;

	/*
	 * Send a direct message request to SP1 (MP SP) from current physical
	 * CPU. Notice SP1 ECs are already woken as a result of the PSCI_CPU_ON
	 * invocation so they already reached the message loop.
	 * The SPMC uses the MP pinned context corresponding to the physical
	 * CPU emitting the request.
	 */
	ret = send_cactus_echo_cmd(HYP_ID, SP_ID(1), ECHO_VAL1);
	if (ret != TEST_RESULT_SUCCESS) {
		goto out;
	}

	/*
	 * Send a direct message request to SP2 (MP SP) from current physical
	 * CPU. The SPMC uses the MP pinned context corresponding to the
	 * physical CPU emitting the request.
	 */
	ret = send_cactus_echo_cmd(HYP_ID, SP_ID(2), ECHO_VAL2);
	if (ret != TEST_RESULT_SUCCESS) {
		goto out;
	}

	/*
	 * Send a direct message request to SP3 (UP SP) from current physical CPU.
	 * The SPMC uses the single vCPU migrated to the new physical core.
	 * The single SP vCPU may receive requests from multiple physical CPUs.
	 * Thus it is possible one message is being processed on one core while
	 * another (or multiple) cores attempt sending a new direct message
	 * request. In such case the cores attempting the new request receive
	 * a busy response from the SPMC. To handle this case a retry loop is
	 * implemented permitting some fairness.
	 */
	uint32_t trial_loop = 5U;
	while (trial_loop--) {
		ffa_ret = cactus_echo_send_cmd(HYP_ID, SP_ID(3), ECHO_VAL3);
		if ((ffa_func_id(ffa_ret) == FFA_ERROR) &&
		    (ffa_error_code(ffa_ret) == FFA_ERROR_BUSY)) {
			VERBOSE("%s(%u) trial %u\n", __func__, core_pos, trial_loop);
			waitms(1);
			continue;
		}

		if (is_ffa_direct_response(ffa_ret) == true) {
			if (cactus_get_response(ffa_ret) != CACTUS_SUCCESS ||
				cactus_echo_get_val(ffa_ret) != ECHO_VAL3) {
				ERROR("Echo Failed!\n");
				ret = TEST_RESULT_FAIL;
			}

			goto out;
		}
	}

	ret = TEST_RESULT_FAIL;

out:
	/* Tell the lead CPU that the calling CPU has completed the test */
	tftf_send_event(&cpu_booted[core_pos]);

	return ret;
}

/**
 * Test direct messaging in multicore setup. Runs SPs on all the cores and sends
 * direct messages to SPs.
 */
test_result_t test_ffa_secondary_core_direct_msg(void)
{
	/**********************************************************************
	 * Check SPMC has ffa_version and expected FFA endpoints are deployed.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 0, expected_sp_uuids);
	return spm_run_multi_core_test((uintptr_t)cpu_on_handler, cpu_booted);
}
