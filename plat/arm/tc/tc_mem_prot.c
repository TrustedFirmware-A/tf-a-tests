/*
 * Copyright (c) 2020-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform.h>

#define TC_DRAM1_NS_START	(TFTF_BASE + 0x4000000)
#define TC_DRAM1_NS_SIZE	0x10000000

static const mem_region_t tc_ram_ranges[] = {
	{ TC_DRAM1_NS_START, TC_DRAM1_NS_SIZE }
};

const mem_region_t *plat_get_prot_regions(int *nelem)
{
	*nelem = ARRAY_SIZE(tc_ram_ranges);
	return tc_ram_ranges;
}
