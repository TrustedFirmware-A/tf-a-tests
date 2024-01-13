/*
 * Copyright (c) 2019-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pauth.h>
#include <psci.h>
#include <smccc.h>
#include <test_helpers.h>
#include <tftf_lib.h>
#include <tftf.h>
#include <tsp.h>
#include <string.h>

#ifdef __aarch64__
static uint128_t pauth_keys_before[NUM_KEYS];
static uint128_t pauth_keys_after[NUM_KEYS];
#endif

/*
 * TF-A is expected to allow access to key registers from lower EL's,
 * reading the keys excercises this, on failure this will trap to
 * EL3 and crash.
 */
test_result_t test_pauth_reg_access(void)
{
	SKIP_TEST_IF_AARCH32();
#ifdef __aarch64__
	SKIP_TEST_IF_PAUTH_NOT_SUPPORTED();
	pauth_test_lib_read_keys(pauth_keys_before);
	return TEST_RESULT_SUCCESS;
#endif	/* __aarch64__ */
}

/*
 * Makes a call to PSCI version, and checks that the EL3 pauth keys are not
 * leaked when it returns
 */
test_result_t test_pauth_leakage(void)
{
	SKIP_TEST_IF_AARCH32();
#ifdef __aarch64__
	SKIP_TEST_IF_PAUTH_NOT_SUPPORTED();
	pauth_test_lib_read_keys(pauth_keys_before);

	tftf_get_psci_version();

	return pauth_test_lib_compare_template(pauth_keys_before, pauth_keys_after);
#endif	/* __aarch64__ */
}

/* Test execution of ARMv8.3-PAuth instructions */
test_result_t test_pauth_instructions(void)
{
	SKIP_TEST_IF_AARCH32();
#ifdef __aarch64__
	SKIP_TEST_IF_PAUTH_NOT_SUPPORTED();

#if ARM_ARCH_AT_LEAST(8, 3)
	/* Pointer authentication instructions */
	__asm__ volatile (
		"paciasp\n"
		"autiasp\n"
		"paciasp\n"
		"xpaclri"
	);
	return TEST_RESULT_SUCCESS;
#else
	tftf_testcase_printf("Pointer Authentication instructions "
				"are not supported on ARMv%u.%u\n",
				ARM_ARCH_MAJOR, ARM_ARCH_MINOR);
	return TEST_RESULT_SKIPPED;
#endif	/* ARM_ARCH_AT_LEAST(8, 3) */
#endif	/* __aarch64__ */
}

/*
 * Makes a call to TSP ADD, and checks that the checks that the Secure World
 * pauth keys are not leaked
 */
test_result_t test_pauth_leakage_tsp(void)
{
	SKIP_TEST_IF_AARCH32();
#ifdef __aarch64__
	smc_args tsp_svc_params;
	smc_ret_values tsp_result = {0};

	SKIP_TEST_IF_PAUTH_NOT_SUPPORTED();
	SKIP_TEST_IF_TSP_NOT_PRESENT();

	pauth_test_lib_fill_regs_and_template(pauth_keys_before);

	/* Standard SMC to ADD two numbers */
	tsp_svc_params.fid = TSP_STD_FID(TSP_ADD);
	tsp_svc_params.arg1 = 4;
	tsp_svc_params.arg2 = 6;
	tsp_result = tftf_smc(&tsp_svc_params);

	/*
	 * Check the result of the addition-TSP_ADD will add
	 * the arguments to themselves and return
	 */
	if (tsp_result.ret0 != 0 || tsp_result.ret1 != 8 ||
	    tsp_result.ret2 != 12) {
		tftf_testcase_printf("TSP add returned wrong result: "
				     "got %d %d %d expected: 0 8 12\n",
				     (unsigned int)tsp_result.ret0,
				     (unsigned int)tsp_result.ret1,
				     (unsigned int)tsp_result.ret2);
		return TEST_RESULT_FAIL;
	}

	return pauth_test_lib_compare_template(pauth_keys_before, pauth_keys_after);
#endif	/* __aarch64__ */
}
