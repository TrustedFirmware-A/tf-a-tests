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
#include <test_helpers.h>

#include "host_realm_simd_common.h"

/*
 * Testcases in host_realm_spm.c, host_realm_payload_simd_tests.c uses these
 * common variables.
 */
sve_z_regs_t ns_sve_z_regs_write;
sve_z_regs_t ns_sve_z_regs_read;

test_result_t host_create_sve_realm_payload(struct realm *realm, bool sve_en,
					    uint8_t sve_vq,
					    unsigned int max_recs,
					    unsigned int max_aux_planes)
{
	u_register_t feature_flag0 = 0UL;
	u_register_t feature_flag1 = RMI_REALM_FLAGS1_RTT_TREE_PP;
	long sl = RTT_MIN_LEVEL;
	u_register_t rec_flag[MAX_REC_COUNT] = {RMI_NOT_RUNNABLE};

	if (is_feat_52b_on_4k_2_supported() == true) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	for (unsigned int i = 0U; i < max_recs; i++) {
		rec_flag[i] = RMI_RUNNABLE;
	}

	if (sve_en) {
		feature_flag0 |= RMI_FEATURE_REGISTER_0_SVE_EN |
				INPLACE(RMI_FEATURE_REGISTER_0_SVE_VL, sve_vq);
	}

	/* Initialise Realm payload */
	if (!host_create_activate_realm_payload(realm,
				       (u_register_t)REALM_IMAGE_BASE,
				       feature_flag0, feature_flag1,
				       sl, rec_flag, max_recs, max_aux_planes,
				       get_test_mecid())) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
