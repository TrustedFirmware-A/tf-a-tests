/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * IOCTL cases only ZynqMP firmware implements. tests-versal.mk compiles this
 * file for that platform alone.
 */

#include "eemi_api.h"
#include "xpm_defs.h"
#include "xpm_nodeid.h"
#include "xpm_nodeid_plat.h"

/* Register counts, the value written to check a round-trip, and a bad id. */
#define PM_GGS_NUM_REGS			4U
#define PM_PGGS_NUM_REGS		4U
#define IOCTL_TEST_PATTERN		0xA5A5A5A5U
#define IOCTL_INVALID_ID		0xFFFFU

/* Write, read back and restore each register, leaving none modified. */
static test_result_t ioctl_general_storage(const char *name, uint32_t write_id,
					   uint32_t read_id, uint32_t num_regs)
{
	int32_t status;
	uint32_t i;

	for (i = 0U; i < num_regs; i++) {
		uint32_t orig = 0U, readback = 0U;
		uint32_t pattern = IOCTL_TEST_PATTERN ^ i;

		status = xpm_ioctl(PM_DEV_ACPU_CORE, read_id, i, 0U, 0U, &orig);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR read %s[%u], Status: 0x%x\n",
					     __func__, name, i, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_ioctl(PM_DEV_ACPU_CORE, write_id, i, pattern, 0U, NULL);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR write %s[%u], Status: 0x%x\n",
					     __func__, name, i, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_ioctl(PM_DEV_ACPU_CORE, read_id, i, 0U, 0U, &readback);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR read back %s[%u], Status: 0x%x\n",
					     __func__, name, i, status);
			return TEST_RESULT_FAIL;
		}

		if (readback != pattern) {
			tftf_testcase_printf("%s ERROR %s[%u] mismatch: wrote 0x%x "
					     "read 0x%x\n", __func__, name, i,
					     pattern, readback);
			(void)xpm_ioctl(PM_DEV_ACPU_CORE, write_id, i, orig, 0U, NULL);
			return TEST_RESULT_FAIL;
		}

		status = xpm_ioctl(PM_DEV_ACPU_CORE, write_id, i, orig, 0U, NULL);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR restore %s[%u], Status: 0x%x\n",
					     __func__, name, i, status);
			return TEST_RESULT_FAIL;
		}
	}

	return TEST_RESULT_SUCCESS;
}

/* Check both kinds of general-storage register survive a round-trip. */
test_result_t test_ioctl_general_storage(void)
{
	test_result_t result;

	result = ioctl_general_storage("GGS", IOCTL_WRITE_GGS, IOCTL_READ_GGS,
				       PM_GGS_NUM_REGS);
	if (result != TEST_RESULT_SUCCESS) {
		return result;
	}

	return ioctl_general_storage("PGGS", IOCTL_WRITE_PGGS, IOCTL_READ_PGGS,
				     PM_PGGS_NUM_REGS);
}

/* An unimplemented identifier must be rejected, not silently accepted. */
test_result_t test_ioctl_invalid_id(void)
{
	int32_t status;

	status = xpm_ioctl(PM_DEV_RPU_CORE, IOCTL_INVALID_ID, 0U, 0U, 0U, NULL);
	if (status == PM_RET_SUCCESS) {
		tftf_testcase_printf("%s ERROR invalid IOCTL id 0x%x accepted\n",
				     __func__, IOCTL_INVALID_ID);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
