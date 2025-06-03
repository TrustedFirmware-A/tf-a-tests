/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <firmware_image_package.h>
#include <fwu_nvm.h>
#include <image_loader.h>
#include <io_storage.h>
#include <platform.h>
#include <platform_def.h>
#include <psci.h>
#include <smccc.h>
#include <status.h>
#include <tftf_lib.h>
#include <uuid.h>
#include <uuid_utils.h>

/*
 * @TEST_AIM@ validate FWU IMAGE SIZE invalid scenario
 * TEST SUCCESS if FWU FIP IMAGE SIZE is found invalid and Fails to Proceed
 * TEST FAIL in case FWU process is able to reboot
 */

static fip_toc_entry_t *
find_fiptoc_entry_t(const int fip_base, const uuid_t *uuid)
{
	fip_toc_entry_t *current_file =
		(fip_toc_entry_t *) (fip_base + sizeof(fip_toc_header_t));

	while (!is_uuid_null(&(current_file->uuid))) {
		if (uuid_equal(&(current_file->uuid), uuid))
			return current_file;

		current_file += 1;
	};
	return NULL;
}

test_result_t test_fwu_image_size(void)
{
	STATUS status;
	int fip_base = PLAT_ARM_FIP_BASE;
	int fwu_fip_base = PLAT_ARM_FWU_FIP_BASE;

	const uuid_t bl2	= UUID_TRUSTED_BOOT_FIRMWARE_BL2;
	const uuid_t bl2u	= UUID_FIRMWARE_UPDATE_BL2U;

	fip_toc_entry_t *bl2_entry;
	fip_toc_entry_t *bl2u_entry;

	uintptr_t bl2_entry_address, bl2u_entry_address;

	int corrupt_size = 0xFFFF;
	int offset;

	smc_args args = { SMC_PSCI_SYSTEM_RESET };
	smc_ret_values ret = {0};

	/* retrieve bl2 and bl2u */
	bl2_entry = find_fiptoc_entry_t(fip_base, &bl2);
	bl2_entry_address = (uintptr_t)bl2_entry;

	bl2u_entry = find_fiptoc_entry_t(fwu_fip_base, &bl2u);
	bl2u_entry_address = (intptr_t)bl2u_entry;

	/* Reboot has not occurred yet */
	tftf_testcase_printf("not rebooted yet\n");

	/* Corrupt FWU_FIP_SIZE */
	tftf_testcase_printf("bl2_entry_address value is (%lx)\n", bl2_entry_address);

	/*
	 * corrupt bl2 in fip
	 * corrupt toc entry size field of bl2u
	 */

	/* corrupt bl2 image */
	offset = bl2_entry_address + sizeof(uuid_t) + sizeof(uint64_t) - FLASH_BASE;
	status = fwu_nvm_write(offset, &corrupt_size, sizeof(uint64_t));

	if (status != STATUS_SUCCESS) {
		tftf_testcase_printf("staus not success error %d\n", status);
		return TEST_RESULT_FAIL;
	}

	/* corrupt toc bl2u entry */
	offset = bl2u_entry_address + sizeof(uuid_t) + sizeof(uint64_t) - FLASH_BASE;
	status = fwu_nvm_write(offset, &corrupt_size, sizeof(uint64_t));

	if (status != STATUS_SUCCESS) {
		tftf_testcase_printf("status not success error %d\n", status);
		return TEST_RESULT_FAIL;
	}

	/* Notify that we are rebooting now. */
	tftf_notify_reboot();

	/* Request PSCI system reset. */
	ret = tftf_smc(&args);

	/* The PSCI SYSTEM_RESET call is not supposed to return */
	tftf_testcase_printf("System didn't reboot properly (%d)\n",
						(unsigned int)ret.ret0);

	return TEST_RESULT_FAIL;
}
