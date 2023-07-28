/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cactus_test_cmds.h>
#include <ffa_endpoints.h>
#include <platform.h>
#include <spm_test_helpers.h>
#include <test_helpers.h>

#define SP_SLEEP_TIME	200U
#define NS_TIME_SLEEP	200U

#define SENDER		HYP_ID
#define RECEIVER	SP_ID(1)

static const struct ffa_uuid expected_sp_uuids[] = { {PRIMARY_UUID} };

test_result_t test_ffa_group0_interrupt_sp_running(void)
{
	struct ffa_value ret_values;

	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);

	/* Send request to first Cactus SP to sleep for 200ms.*/
	ret_values = cactus_sleep_cmd(SENDER, RECEIVER, SP_SLEEP_TIME);

	/*
	 * SBSA secure watchdog timer fires every 100ms. Hence a Group0 secure
	 * interrupt should trigger during this time.
	 */
	if (!is_ffa_direct_response(ret_values)) {
		ERROR("Expected a direct response for sleep command\n");
		return TEST_RESULT_FAIL;
	}

	/* Make sure elapsed time not less than sleep time. */
	if (cactus_get_response(ret_values) < SP_SLEEP_TIME) {
		ERROR("Lapsed time less than requested sleep time\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

test_result_t test_ffa_group0_interrupt_in_nwd(void)
{
	uint64_t time1;
	volatile uint64_t time2, time_lapsed;
	uint64_t timer_freq = read_cntfrq_el0();

	time1 = syscounter_read();

	/*
	 * Sleep for NS_TIME_SLEEP ms. This ensures SBSA secure wdog timer
	 * triggers during this time.
	 */
	waitms(NS_TIME_SLEEP);
	time2 = syscounter_read();

	/* Lapsed time should be at least equal to sleep time. */
	time_lapsed = ((time2 - time1) * 1000) / timer_freq;

	if (time_lapsed < NS_TIME_SLEEP) {
		ERROR("Time elapsed less than expected value: %llu vs %u\n",
				time_lapsed, NS_TIME_SLEEP);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
