/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <uuid.h>

#include "tbb_test_infra.h"

#include <firmware_image_package.h>
#include <platform.h>
#include <tftf_lib.h>
#include <uuid_utils.h>

/*
 * Return the offset relative to the base of the FIP of
 * the image described by the uuid. 0 is returned on failure.
 * The first image will not have an offset of 0, as the header
 * exists at offset 0.
 */
static unsigned int
find_offset_in_fip(const uuid_t *uuid)
{
	fip_toc_entry_t *current_file =
		(fip_toc_entry_t *) (PLAT_ARM_FIP_BASE + sizeof(fip_toc_header_t));

	while (!is_uuid_null(&(current_file->uuid))) {
		if (uuid_equal(&(current_file->uuid), uuid)) {
			return current_file->offset_address;
		}
		current_file += 1;
	};
	return 0;
}

test_result_t test_tbb_tkey_cert_header(void)
{
	static const uuid_t tkey_cert_uuid = UUID_TRUSTED_KEY_CERT;
	unsigned int image_offset = find_offset_in_fip(&tkey_cert_uuid);

	TEST_ASSERT_SKIP(image_offset != 0);
	return test_corrupt_boot_fip(image_offset);
}

