/*
 * Copyright (c) 2022-2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_features.h>
#include <arch_helpers.h>
#include <test_helpers.h>
#include <tftf.h>
#include <tftf_lib.h>

#define MAX_ITERATIONS		100

/*
 * This runs RNDR/RNDRRS instructions when it detects FEAT_RNG_TRAP and checks
 * that it generates a random number each time.
 * Argument "use_rndrrs" decides whether to execute "rndrrs" or "rndr" instruction.
 */
static test_result_t test_rng_trap(bool use_rndrrs)
{
	SKIP_TEST_IF_AARCH32();

#if defined __aarch64__
	u_register_t rng, rng1 = 0;
	bool rng_changed = false;

	SKIP_TEST_IF_RNG_TRAP_NOT_SUPPORTED();

	/* Loop to check things seem random. */
	for (unsigned i = 0; i < MAX_ITERATIONS; i++) {
		if (use_rndrrs) {
			/* Attempt to read RNDRRS. */
			rng = read_rndrrs();
		} else {
			/* Attempt to read RNDR. */
			rng = read_rndr();
		}
		if (rng != rng1) {
			rng_changed = true;
		}
		rng1 = rng;
	}

	/*
	 * It is possible (however unlikely) that subsequent random numbers are
	 * identical and/or that there's little entropy to make a truly new
	 * random number. The only certain check is that the number isn't the
	 * same at least once.
	 */
	if (!rng_changed) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
#endif
}

/* Test RNDR read access causes a trap to EL3 and generates a random number each time */
test_result_t test_rndr_rng_trap(void)
{
	return test_rng_trap(false);
}

/* Test RNDRRS read access causes a trap to EL3 and generates a random number each time */
test_result_t test_rndrrs_rng_trap(void)
{
	return test_rng_trap(true);
}
