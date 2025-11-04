/*
 * Copyright (c) 2024-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>
#include <tftf_lib.h>
#include <sdei.h>
#include <drivers/arm/arm_gic.h>
#include <drivers/arm/gic_v2v3_common.h>

/*
 * Only this many events can be bound in the PPI range. If you attempt to bind
 * more, an error code should be returned.
 */
#define MAX_EVENTS_IN_PPI_RANGE		U(3)

/*
 * Test that it doesn't crash when you attempt to bind more events in the PPI
 * range than are available.
 */
test_result_t test_sdei_bind_failure(void)
{
	int64_t ret, ev;
	struct sdei_intr_ctx intr_ctx;
	int intr, num_bound;
	bool bind_failed, bind_should_fail;

	ret = sdei_version();
	if (ret != MAKE_SDEI_VERSION(1, 0, 0)) {
		printf("%d: Unexpected SDEI version: 0x%llx\n",
			__LINE__, (unsigned long long) ret);
		return TEST_RESULT_SKIPPED;
	}

	num_bound = 0;
	for (intr = MIN_PPI_ID; intr < MAX_PPI_ID; intr++) {
		printf("Binding to interrupt %d.\n", intr);

		bind_should_fail = num_bound >= MAX_EVENTS_IN_PPI_RANGE;

		ev = sdei_interrupt_bind(intr, &intr_ctx);

		if (ev == -SMC_EDENY) {
			/*
			 * This isn't a non-secure interrupt number, so we can't
			 * use it.
			 */
			continue;
		}

		num_bound++;

		/*
		 * Every bind should succeed except the last one, which should
		 * fail.
		 */
		bind_failed = ev < 0;
		if (bind_failed != bind_should_fail) {
			printf("%d: SDEI interrupt bind; ret=%lld; Bind should "
			       "have %s\n",
			       __LINE__,
			       ev,
			       bind_should_fail ? "failed" : "succeeded");
			return TEST_RESULT_FAIL;
		}
	}

	return TEST_RESULT_SUCCESS;
}
