/*
 * Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <debug.h>
#include <smccc.h>
#include <tftf_lib.h>

#include <platform_def.h>

/* Number of 32bits values in payload */
#define PAYLOAD_ARG_CNT			4U

#define upper_32_bits(n)		((uint32_t)(((n) >> 32)))
#define lower_32_bits(n)		((uint32_t)((n) & 0xffffffff))


#define PM_GET_API_VERSION		0xC2000001
#define PM_GET_CHIPID			0xC2000018


/*
 * @Test_Aim@ Test to read the PM-API version from AMD-Xilinx platform
 * This test run on lead CPU and issues PM_GET_API_VERSION SMC call to read the
 * supported PM-API version on the platform.
 * Return vslues are packed as
 * ret.ret0(31:0) : actual return value
 * ret.ret0(63:32) : Return arg1
 * ret.ret1(31:0) : Return arg2
 * ret.ret1(63:32) : Return arg3 and so on.
 */
test_result_t test_pmapi_version(void)
{
	smc_args args = { PM_GET_API_VERSION };
	smc_ret_values ret;
	uint32_t major, minor, status;

	ret = tftf_smc(&args);
	status = lower_32_bits(ret.ret0);
	if (status) {
		tftf_testcase_printf("%s ERROR Reading PM-API Version\n",
				     __func__);
		return TEST_RESULT_FAIL;
	}

	major = upper_32_bits(ret.ret0) >> 16;
	minor = upper_32_bits(ret.ret0) & 0xFFFF;

	tftf_testcase_printf("%s PM-API Version : %d.%d\n", __func__,
			     major, minor);
	return TEST_RESULT_SUCCESS;
}

/*
 * @Test_Aim@ Test to read the Chip ID of AMD-Xilinx platforms.
 * This test runs on Lead CPU and issues PM_GET_CHIPID SMC call to read  ChipID
 * The IDcode and version is printed
 * Return vslues are packed as
 * ret.ret0(31:0) : actual return value
 * ret.ret0(63:32) : Return arg1
 * ret.ret1(31:0) : Return arg2
 * ret.ret1(63:32) : Return arg3 and so on.
 */
test_result_t test_get_chipid(void)
{
	smc_args args = { PM_GET_CHIPID };
	smc_ret_values ret;
	uint32_t idcode, version, status;

	ret = tftf_smc(&args);
	status = lower_32_bits(ret.ret0);
	if (status) {
		tftf_testcase_printf("%s ERROR Reading Chip ID\n", __func__);
		return TEST_RESULT_FAIL;
	}

	idcode = upper_32_bits(ret.ret0);
	version = lower_32_bits(ret.ret1);

	tftf_testcase_printf("%s Idcode = 0x%x Version = 0x%x\n", __func__,
			     idcode, version);

	return TEST_RESULT_SUCCESS;
}
