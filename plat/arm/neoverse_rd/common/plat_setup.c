/*
 * Copyright (c) 2018-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <drivers/arm/arm_gic.h>
#include <xlat_tables_v2.h>

static const mmap_region_t mmap[] = {
	MAP_REGION_FLAT(NRD_CSS_PERIPH0_BASE, NRD_CSS_PERIPH0_SIZE,
			MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(NRD_CSS_PERIPH1_BASE, NRD_CSS_PERIPH1_SIZE,
			MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(DRAM_BASE, TFTF_BASE - DRAM_BASE,
			MT_MEMORY | MT_RW | MT_NS),
	{0}
};

const mmap_region_t *tftf_platform_get_mmap(void)
{
	return mmap;
}

void plat_arm_gic_init(void)
{
	arm_gic_init(PLAT_ARM_GICC_BASE, PLAT_ARM_GICD_BASE, PLAT_ARM_GICR_BASE);
}
