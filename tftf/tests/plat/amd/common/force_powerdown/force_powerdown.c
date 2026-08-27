/*
 * Copyright (c) 2025-2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_defs.h"
#include "xpm_nodeid.h"

#define TARGET_SUBSYSTEM	PM_SUBSYS_RPU
#define WAKEUP_ADDR		RPU_WAKEUP_ADDR

/* set_address argument: 1 = pass the resume address, 0 = ignore it */
#define SET_ADDRESS		1U

/* Delay between force power down and wake-up of the target (milliseconds) */
#define WAKEUP_DELAY_MS		10000U

/*
 * This function is used to force power down the subsystem if the
 * subsystem is unresponsive and by calling this API all the resources of
 * that subsystem will be automatically released.
 *
 * This function is used to force power down the subsystem if the
 * subsystem is unresponsive. By calling this API, all the resources of
 * that subsystem will be automatically released.
 *
 * Force power down support for individual processors and power domains has been
 * deprecated. As a result, the only available option now is to force power down
 * the entire subsystem.
 *
 * To support this, another subsystem (RPU) needs to be present. For example,
 * the RPU subsystem can be added with only a NOP CDO (which contains only a
 * single "nop" instruction). This allows the force power down feature to be
 * tested without requiring an actual executable partition for the RPU.
 *
 * The ack_type argument selects the acknowledgment mode (IPI_BLOCKING or
 * IPI_NON_BLOCKING) passed to the force power down EEMI API.
 */
static test_result_t force_powerdown_and_wake_up(uint32_t ack_type)
{
	int32_t status;

	status = xpm_force_powerdown(TARGET_SUBSYSTEM, ack_type);
	if (status != PM_RET_SUCCESS) {
		tftf_testcase_printf("%s ERROR force powering down system: 0x%x, "
				     "Status: 0x%x\n", __func__, TARGET_SUBSYSTEM, status);
		return TEST_RESULT_FAIL;
	}

	tftf_testcase_printf("Waiting before waking up the target\n\r");
	waitms(WAKEUP_DELAY_MS);

	status = xpm_request_wakeup(TARGET_SUBSYSTEM, SET_ADDRESS, WAKEUP_ADDR,
				    IPI_NON_BLOCKING);
	if (status != PM_RET_SUCCESS) {
		tftf_testcase_printf("%s ERROR requesting wake-up for %x, "
				     "Status: 0x%x\n", __func__, TARGET_SUBSYSTEM, status);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

test_result_t test_force_powerdown_ack(void)
{
	return force_powerdown_and_wake_up(IPI_BLOCKING);
}

test_result_t test_force_powerdown_no_ack(void)
{
	return force_powerdown_and_wake_up(IPI_NON_BLOCKING);
}
