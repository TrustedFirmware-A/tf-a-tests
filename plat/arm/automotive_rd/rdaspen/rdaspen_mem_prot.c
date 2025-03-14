/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform.h>
#include <platform_def.h>

#define NS_IMAGE_OFFSET		TFTF_BASE
#define NS_IMAGE_LIMIT		(NS_IMAGE_OFFSET + (32 << TWO_MB_SHIFT))

static const mem_region_t rdaspen_ram_ranges[] = {
	{NS_IMAGE_LIMIT, 128 << TWO_MB_SHIFT},
};

const mem_region_t *plat_get_prot_regions(int *nelem)
{
	*nelem = ARRAY_SIZE(rdaspen_ram_ranges);
	return rdaspen_ram_ranges;
}
