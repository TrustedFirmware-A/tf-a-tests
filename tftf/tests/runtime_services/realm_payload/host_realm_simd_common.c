/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_features.h>
#include <tftf.h>
#include <host_realm_rmi.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <lib/extensions/sve.h>

#include "host_realm_simd_common.h"

/*
 * Testcases in host_realm_spm.c, host_realm_payload_simd_tests.c uses these
 * common variables.
 */
sve_z_regs_t ns_sve_z_regs_write;
sve_z_regs_t ns_sve_z_regs_read;

test_result_t host_create_sve_realm_payload(struct realm *realm, bool sve_en,
					    uint8_t sve_vq)
{
	u_register_t feature_flag = 0UL;
	long sl = RTT_MIN_LEVEL;
	u_register_t rec_flag[1] = {RMI_RUNNABLE};

	if (is_feat_52b_on_4k_2_supported() == true) {
		feature_flag = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	if (sve_en) {
		feature_flag |= RMI_FEATURE_REGISTER_0_SVE_EN |
				INPLACE(FEATURE_SVE_VL, sve_vq);
	}

	/* Initialise Realm payload */
	if (!host_create_activate_realm_payload(realm,
				       (u_register_t)REALM_IMAGE_BASE,
				       feature_flag, sl, rec_flag, 1U, 0U)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
