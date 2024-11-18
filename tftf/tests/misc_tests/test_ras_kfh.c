/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <arch_helpers.h>
#include <serror.h>
#include <tftf_lib.h>

#ifdef __aarch64__
static volatile uint64_t serror_triggered;
extern void inject_unrecoverable_ras_error(void);

static bool serror_handler(bool *incr_elr_elx)
{
	serror_triggered = 1;
	*incr_elr_elx = false;
	return true;
}

/*
 * Test Kernel First handling paradigm of RAS errors.
 *
 * Register a custom serror handler in tftf, inject a RAS error and wait
 * for finite time to ensure that SError triggered and handled.
 */
test_result_t test_ras_kfh(void)
{
	register_custom_serror_handler(serror_handler);
	inject_unrecoverable_ras_error();

	/* Give reasonable time for SError to be triggered/handled */
	waitms(500);

	unregister_custom_serror_handler();

	if (serror_triggered == false) {
		tftf_testcase_printf("SError is not triggered\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
#else
test_result_t test_ras_kfh(void)
{
	tftf_testcase_printf("Not supported on AArch32.\n");
	return TEST_RESULT_SKIPPED;
}

#endif
