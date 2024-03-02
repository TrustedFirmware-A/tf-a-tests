/*
 * Copyright (c) 2018-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform.h>

#define NRD_DRAM1_NS_START	(TFTF_BASE + 0x4000000)
#define NRD_DRAM1_NS_SIZE	0x10000000

static const mem_region_t nrd_ram_ranges[] = {
	{ NRD_DRAM1_NS_START, NRD_DRAM1_NS_SIZE },
};

const mem_region_t *plat_get_prot_regions(int *nelem)
{
	*nelem = ARRAY_SIZE(nrd_ram_ranges);
	return nrd_ram_ranges;
}
