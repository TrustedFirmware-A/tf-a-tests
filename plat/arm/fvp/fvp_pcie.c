/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pcie.h>

#include <platform.h>
#include <platform_pcie.h>

CASSERT(PLATFORM_NUM_ECAM != 0, PLATFORM_NUM_ECAM_is_zero);

const struct pcie_info_table fvp_pcie_cfg = {
	.num_entries = PLATFORM_NUM_ECAM,
	.block[0] = {
		PLATFORM_PCIE_ECAM_BASE_ADDR_0,
		PLATFORM_PCIE_SEGMENT_GRP_NUM_0,
		PLATFORM_PCIE_START_BUS_NUM_0,
		PLATFORM_PCIE_END_BUS_NUM_0
	}
};

const struct pcie_info_table *plat_pcie_get_info_table(void)
{
	return &fvp_pcie_cfg;
}
