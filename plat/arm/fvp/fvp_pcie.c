/*
 * Copyright (c) 2024-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
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

/* Retrieve platform PCIe bar config values */
int plat_pcie_get_bar_config(uint64_t *bar64_val, uint64_t *rp_bar64_val,
			     uint32_t *bar32np_val, uint32_t *bar32p_val,
			     uint32_t *rp_bar32_val)
{
#ifdef __aarch64__
	assert((bar64_val != NULL) && (rp_bar64_val != NULL) &&
	       (bar32np_val != NULL) && (bar32p_val != NULL) &&
	       (rp_bar32_val != NULL));

	*bar64_val = PLATFORM_OVERRIDE_PCIE_BAR64_VALUE;
	*rp_bar64_val = PLATFORM_OVERRIDE_RP_BAR64_VALUE;

	*bar32np_val = PLATFORM_OVERRIDE_PCIE_BAR32NP_VALUE;
	*bar32p_val = PLATFORM_OVERRIDE_PCIE_BAR32P_VALUE;
	*rp_bar32_val = PLATOFRM_OVERRIDE_RP_BAR32_VALUE;

	return 0;
#endif
	return -1;
}

/*
 * Retrieve platform PCIe memory region (Base Platform RevC only)
 */
int plat_get_dev_region(uint64_t *base, size_t *size,
			uint32_t type, uint32_t idx)
{
#ifdef __aarch64__
	assert((base != NULL) && (size != NULL));

	if (type == DEV_MEM_NON_COHERENT) {
		switch (idx) {
		case 0U:
			/* PCIe memory region 1 */
			*base = PCIE_MEM_1_BASE;
			*size = PCIE_MEM_1_SIZE;
			return 0;
		case 1U:
			/* PCIe memory region 2 */
			*base = PCIE_MEM_2_BASE;
			*size = PCIE_MEM_2_SIZE;
			return 0;
		default:
			break;
		}
	}
#endif
	return -1;
}
