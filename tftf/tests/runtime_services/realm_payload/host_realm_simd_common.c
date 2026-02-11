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
	bool lpa2 = false;
	u_register_t s2sz = MAX_IPA_BITS;
	long sl = RTT_MIN_LEVEL;
	u_register_t rec_flag[MAX_REC_COUNT] = {RMI_NOT_RUNNABLE};
	struct test_realm_params params = {0};

	if (is_feat_52b_on_4k_2_supported() == true) {
		lpa2 = true;
		s2sz = MAX_IPA_BITS_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	for (unsigned int i = 0U; i < max_recs; i++) {
		rec_flag[i] = RMI_RUNNABLE;
	}

	/* Initialise Realm payload */
	params.realm_payload_adr = (u_register_t)REALM_IMAGE_BASE;
	params.lpa2 = lpa2;
	params.s2sz = s2sz;
	params.sve = sve_en;
	params.sve_vl = sve_vq;
	params.sl = sl;
	params.rec_flag = rec_flag;
	params.rec_count = max_recs;
	params.num_aux_planes = max_aux_planes;

	if (!host_create_activate_realm_payload(realm, &params)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
