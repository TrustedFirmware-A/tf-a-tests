/*
 * Copyright (c) 2022-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_features.h>
#include <arch_helpers.h>
#include <test_helpers.h>
#include <tftf.h>
#include <tftf_lib.h>

#define MAX_ITERATIONS_EXCLUSIVE 	100

/*
 * This test ensures that a RNDR/RNDRRS instructions causes a trap to EL3 and
 * generates a random number each time.
 * Argument "use_rndrrs" decides whether to execute "rndrrs" or "rndr" instruction.
 *
 * This test usage load/store exclusive pairs to detect whether the execution
 * trapped to EL3 or not?
 * It relies on the fact that when exception level changes the monitor is cleared.
 * In a load/store exclusive pair with stxr instruction, the status gets updated
 * to '1' when monitor is cleared.
 * In this test start with ldxr and execute trap instruction and if the trap to EL3
 * happened then stxr status will be '1', to avoid chances of monitor being cleared
 * (highly unlikely in this scenario) by any other reason do this test iteratively.
 * If stxr succeds even single time we are sure that trap did not happen.
 */
static test_result_t test_rng_trap(bool use_rndrrs)
{
#if defined __aarch64__
	u_register_t rng, rng1 = 0;
	u_register_t exclusive;
	u_register_t status;
	unsigned int i;

	/* Make sure FEAT_RNG_TRAP is supported. */
	SKIP_TEST_IF_RNG_TRAP_NOT_SUPPORTED();

	/*
	 * The test was inserted in a loop that runs a safe number of times
	 * in order to discard any possible trap returns other than RNG_TRAP
	 */
	for (i = 0; i < MAX_ITERATIONS_EXCLUSIVE; i++) {
		/* Attempt to acquire address for exclusive access */
		__asm__ volatile ("ldxr %0, %1\n" : "=r"(rng)
					: "Q"(exclusive));
		if (use_rndrrs) {
			/* Attempt to read RNDRRS. */
			rng = read_rndrrs();
		} else {
			/* Attempt to read RNDR. */
			rng = read_rndr();
		}
		/*
		 * After returning from the trap, the monitor variable should
		 * be cleared, so the status value should be 1.
		 */
		__asm__ volatile ("stxr %w0, %1, %2\n" : "=&r"(status)
					: "r"(rng), "Q"(exclusive));
		/* If monitor is not cleared or not a new random number */
		if ((status == 0) || (rng == rng1)) {
			return TEST_RESULT_FAIL;
		}
		rng1 = rng;
	}

	return TEST_RESULT_SUCCESS;
#else
	/* Skip test if AArch32 */
	SKIP_TEST_IF_AARCH32();
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
