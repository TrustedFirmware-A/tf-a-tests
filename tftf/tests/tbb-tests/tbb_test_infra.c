/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "tbb_test_infra.h"

#include <fwu_nvm.h>
#include <io_storage.h>
#include <platform.h>
#include <status.h>
#include <test_helpers.h>
#include <tftf_lib.h>

test_result_t test_corrupt_boot_fip(unsigned int offset)
{
	unsigned int flag = 0xDEADBEEF;
	size_t written = 0;
	uintptr_t dev_handle;
	int result;

	if (tftf_is_rebooted()) {
		/* FIP successfully repaired */
		return TEST_RESULT_SUCCESS;
	}

	/* Corrupt the FIP at the provided offset */
	plat_get_nvm_handle(&dev_handle);
	result = io_seek(dev_handle, IO_SEEK_SET, offset);
	TEST_ASSERT(result == IO_SUCCESS);
	result = io_write(dev_handle, (uintptr_t) &flag, sizeof(flag), &written);
	TEST_ASSERT(result == IO_SUCCESS);
	TEST_ASSERT(written == sizeof(flag));

	/*
	 * Now reboot the system.
	 * On the next boot, EL3 firmware should notice and repair the corruption
	 * before re-entering TFTF
	 */

	tftf_notify_reboot();
	psci_system_reset();
	return TEST_RESULT_FAIL;
}
