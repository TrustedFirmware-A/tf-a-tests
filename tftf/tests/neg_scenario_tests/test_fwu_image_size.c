/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <firmware_image_package.h>
#include <fwu_nvm.h>
#include <platform.h>
#include <platform_def.h>
#include <psci.h>
#include <smccc.h>
#include <stddef.h>
#include <status.h>
#include <tftf_lib.h>
#include <uuid.h>
#include <uuid_utils.h>

/*
 * @TEST_AIM@ Validate FWU failure caused by an invalid BL2U image size.
 *
 * The test corrupts the primary FIP ToC header to force BL1 into the FWU flow
 * on the next boot. It then corrupts the BL2U ToC entry size in FWU_FIP so
 * the FWU image load fails due to an invalid size.
 *
 * TEST SUCCESS if TF-A does not complete the FWU flow and does not reboot back
 * into TFTF.
 * TEST FAIL if the system returns after reboot, which means FWU unexpectedly
 * completed.
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
	int fwu_fip_base = PLAT_ARM_FWU_FIP_BASE;
	const uuid_t bl2u	= UUID_FIRMWARE_UPDATE_BL2U;

	fip_toc_entry_t *bl2u_entry;
	uintptr_t bl2u_entry_address;
	uint32_t corrupt_toc = 0xdeadbeef;
	uint64_t corrupt_size = PLAT_ARM_FWU_FIP_SIZE + 1ULL;
	unsigned long long offset;

	smc_args args = { SMC_PSCI_SYSTEM_RESET };
	smc_ret_values ret = {0};

	if (tftf_is_rebooted()) {
		tftf_testcase_printf("FWU unexpectedly completed reboot\n");
		return TEST_RESULT_FAIL;
	}

	/* Locate BL2U in FWU_FIP. */
	bl2u_entry = find_fiptoc_entry_t(fwu_fip_base, &bl2u);
	if (bl2u_entry == NULL) {
		tftf_testcase_printf("Failed to find BL2U entry in FWU_FIP\n");
		return TEST_RESULT_FAIL;
	}

	bl2u_entry_address = (intptr_t)bl2u_entry;

	/*
	 * Corrupt the primary FIP ToC header to force entry into FWU on the
	 * next boot, then corrupt the BL2U ToC size field in FWU_FIP so the
	 * FWU image load fails deterministically due to an invalid size.
	 */
	offset = offsetof(fip_toc_header_t, name);
	status = fwu_nvm_write(offset, &corrupt_toc, sizeof(corrupt_toc));

	if (status != STATUS_SUCCESS) {
		tftf_testcase_printf("Failed to corrupt primary FIP ToC (%d)\n",
				     status);
		return TEST_RESULT_SKIPPED;
	}

	offset = bl2u_entry_address + offsetof(fip_toc_entry_t, size) -
		 FLASH_BASE;
	status = fwu_nvm_write(offset, &corrupt_size, sizeof(corrupt_size));

	if (status != STATUS_SUCCESS) {
		tftf_testcase_printf("Failed to corrupt BL2U image size (%d)\n",
				     status);
		return TEST_RESULT_SKIPPED;
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
