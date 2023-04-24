/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <arch_helpers.h>
#include <arm_arch_svc.h>
#include <debug.h>
#include <mmio.h>
#include <tftf_lib.h>
#include <smccc.h>
#include <xlat_tables_v2.h>

#define TEST_ADDRESS	UL(0x7FFFF000)

/*
 * Purpose of these tests is to ensure EA from lower EL trap/handled in EL3.
 *
 * Tests HANDLE_EA_EL3_FIRST_NS feature(SCR_EL3.EA = 1) of TF-A
 *
 * Works in conjunction with PLATFORM_TEST_EA_FFH macro in TF-A.
 */

/*
 * This test maps a non-existent memory as Device memory and reads it.
 * Memory is mapped as device and cause an error on bus and trap as an Sync EA.
 */
test_result_t test_inject_syncEA(void)
{
	int rc;

	rc = mmap_add_dynamic_region(TEST_ADDRESS, TEST_ADDRESS, PAGE_SIZE,
						MT_DEVICE | MT_RO | MT_NS);
	if (rc != 0) {
		tftf_testcase_printf("%d: mapping address %lu(%d) failed\n",
				      __LINE__, TEST_ADDRESS, rc);
		return TEST_RESULT_FAIL;
	}

	/*
	 * Try reading invalid address, which will cause an exception to be handled in EL3.
	 * EL3 after handling the exception returns to the next instruction to avoid
	 * continous exceptions.
	 */
	rc = mmio_read_32(TEST_ADDRESS);

	rc = mmap_remove_dynamic_region(TEST_ADDRESS, PAGE_SIZE);
	if (rc != 0) {
		tftf_testcase_printf("%d: mmap_remove_dynamic_region() = %d\n", __LINE__, rc);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * This test maps a non-existent memory as Device memory and write to it.
 * Memory is mapped as device and cause an error on bus and trap as an SError.
 */
test_result_t test_inject_serror(void)
{
	int rc;

	rc = mmap_add_dynamic_region(TEST_ADDRESS, TEST_ADDRESS, PAGE_SIZE,
						MT_DEVICE | MT_RW | MT_NS);
	if (rc != 0) {
		tftf_testcase_printf("%d: mapping address %lu(%d) failed\n",
				      __LINE__, TEST_ADDRESS, rc);
		return TEST_RESULT_FAIL;
	}

	/* Try writing to invalid address */
	mmio_write_32(TEST_ADDRESS, 1);

	rc = mmap_remove_dynamic_region(TEST_ADDRESS, PAGE_SIZE);
	if (rc != 0) {
		tftf_testcase_printf("%d: mmap_remove_dynamic_region() = %d\n", __LINE__, rc);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
