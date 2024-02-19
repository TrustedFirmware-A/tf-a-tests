/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sp_helpers.h>

#include <arch_features.h>

static void cpu_check_id_regs(void)
{
	/* ID_AA64PFR0_EL1 */
	EXPECT(is_feat_advsimd_present(), true);
	EXPECT(is_feat_fp_present(), true);
	EXPECT(is_armv8_2_sve_present(), false);

	/* ID_AA64PFR1_EL1 */
	EXPECT(is_feat_sme_supported(), false);
}

void cpu_feature_tests(void)
{
	const char *test_cpu_str = "CPU tests";

	announce_test_section_start(test_cpu_str);
	cpu_check_id_regs();
	announce_test_section_end(test_cpu_str);
}
