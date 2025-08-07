/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <arm_arch_svc.h>
#include <debug.h>
#include <smccc.h>
#include <string.h>
#include <tftf_lib.h>

/*
 * Return SOC ID parameters(SOC revision/SOC version) according
 * to argument passed
 */
static smc_ret_values get_soc_id_param(u_register_t arg)
{
	smc_args args;
	smc_ret_values ret;

	memset(&args, 0, sizeof(args));
	args.fid = SMCCC_ARCH_SOC_ID;
	args.arg1 = arg;
	ret = tftf_smc(&args);

	return ret;
}

#if __aarch64__
/*
 * Return SOC ID SMC64 parameters(SOC Name) according
 * to argument passed
 */
static smc_ret_values_ext get_soc_id_smc64_param(u_register_t arg)
{
	smc_args_ext args;
	smc_ret_values_ext ret = {};

	memset(&args, 0, sizeof(args));
	args.fid = SMCCC_ARCH_SOC_ID | (SMC_64 << FUNCID_CC_SHIFT);
	args.arg1 = arg;
	tftf_smc_no_retval_x8(&args, &ret);

	return ret;
}

/* Checks if the SoC Name is a null terminated ASCII string */
static int32_t is_soc_name_valid(char *soc_name)
{
	int i;

	for (i = 0; i < SMCCC_SOC_NAME_LEN; i++) {
		char ch = soc_name[i];

		if (ch == '\0') {
			return 0;
		} else if (ch <= 0x20 && ch > 0x7F) {
			tftf_testcase_printf("Non-ASCII character detected\n");
			return -1;
		}
	}

	return -1;
}
#endif /* __aarch64__ */

/* Entry function to execute SMCCC_ARCH_SOC_ID test */
test_result_t test_smccc_arch_soc_id(void)
{
	smc_args args;
	smc_ret_values ret;
	int32_t expected_ver;

	/* Check if SMCCC version is at least v1.2 */
	expected_ver = MAKE_SMCCC_VERSION(1, 2);
	memset(&args, 0, sizeof(args));
	args.fid = SMCCC_VERSION;
	ret = tftf_smc(&args);
	if ((int32_t)ret.ret0 < expected_ver) {
		tftf_testcase_printf("Unexpected SMCCC version: 0x%x\n",
		       (int)ret.ret0);
		return TEST_RESULT_SKIPPED;
	}

	/* Check if SMCCC_ARCH_SOC_ID is implemented or not */
	memset(&args, 0, sizeof(args));
	args.fid = SMCCC_ARCH_FEATURES;
	args.arg1 = SMCCC_ARCH_SOC_ID;
	ret = tftf_smc(&args);
	if ((int)ret.ret0 == SMC_ARCH_CALL_NOT_SUPPORTED) {
		tftf_testcase_printf("SMCCC_ARCH_SOC_ID is not implemented\n");
		return TEST_RESULT_SKIPPED;
	}

	/* If the call returns SMC_OK then SMCCC_ARCH_SOC_ID is feature available */
	if ((int)ret.ret0 == SMC_OK) {
		ret = get_soc_id_param(SMC_GET_SOC_REVISION);

		if ((int)ret.ret0 == SMC_ARCH_CALL_INVAL_PARAM) {
			tftf_testcase_printf("Invalid param passed to	\
					SMCCC_ARCH_SOC_ID\n");
			return TEST_RESULT_FAIL;
		} else if ((int)ret.ret0 == SMC_ARCH_CALL_NOT_SUPPORTED) {
			tftf_testcase_printf("SOC Rev is not implemented\n");
			return TEST_RESULT_FAIL;
		}

		tftf_testcase_printf("SOC Rev = 0x%x\n", (int)ret.ret0);

		ret = get_soc_id_param(SMC_GET_SOC_VERSION);

		if ((int)ret.ret0 == SMC_ARCH_CALL_INVAL_PARAM) {
			tftf_testcase_printf("Invalid param passed to	\
					SMCCC_ARCH_SOC_ID\n");
			return TEST_RESULT_FAIL;
		} else if ((int)ret.ret0 == SMC_ARCH_CALL_NOT_SUPPORTED) {
			tftf_testcase_printf("SOC Ver is not implemented\n");
			return TEST_RESULT_FAIL;
		}

		tftf_testcase_printf("SOC Ver = 0x%x\n", (int)ret.ret0);

#if __aarch64__
		char soc_name[SMCCC_SOC_NAME_LEN];
		smc_ret_values_ext ext_ret = get_soc_id_smc64_param(SMC_GET_SOC_NAME);

		if ((int)ext_ret.ret0 == SMC_ARCH_CALL_INVAL_PARAM) {
			tftf_testcase_printf("Invalid param passed to	\
					SMCCC_ARCH_SOC_ID\n");
			return TEST_RESULT_FAIL;
		} else if ((int)ext_ret.ret0 == SMC_ARCH_CALL_NOT_SUPPORTED) {
			tftf_testcase_printf("SOC Name is not implemented\n");
			return TEST_RESULT_SKIPPED;
		}

		memcpy(&soc_name, &ext_ret.ret1, SMCCC_SOC_NAME_LEN);

		if (is_soc_name_valid(soc_name) != 0)
			return TEST_RESULT_FAIL;

		tftf_testcase_printf("SOC Name = %s\n", soc_name);
#endif /* __aarch64__*/

		/*
		 * Negative test to check if SMC_GET_SOC_NAME is present
		 * in SMC32. This should not return SMC_ARCH_CALL_SUCCESS.
		 */
		ret = get_soc_id_param(SMC_GET_SOC_NAME);

		if ((int)ret.ret0 == SMC_ARCH_CALL_SUCCESS) {
			tftf_testcase_printf("SoC Name should only be present \
					for SMC64 invocations\n");
			return TEST_RESULT_FAIL;
		}

	} else {
		ERROR("Invalid error during SMCCC_ARCH_FEATURES call = 0x%x\n",
			(int)ret.ret0);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
