/*
 * Copyright (c) 2025-2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <drivers/arm/arm_gic.h>
#include <platform_def.h>
#include <plat_arm.h>

/*
 * Table of regions to map using the MMU.
 */
static const mmap_region_t mmap[] = {
	RDASPEN_MAP_DEVICE0,
	RDASPEN_MAP_DEVICE1,
	RDASPEN_MAP_DEVICE2,
	RDASPEN_MAP_NS_DRAM1,
	{0}
};

static const uintptr_t rdaspen_gicr_base_addrs[] = {
	GICR_BASE_VIEW0_0_0,
	GICR_BASE_VIEW0_0_1,
	GICR_BASE_VIEW0_0_2,
	GICR_BASE_VIEW0_0_3,
	GICR_BASE_VIEW0_1_0,
	GICR_BASE_VIEW0_1_1,
	GICR_BASE_VIEW0_1_2,
	GICR_BASE_VIEW0_1_3,
	GICR_BASE_VIEW0_2_0,
	GICR_BASE_VIEW0_2_1,
	GICR_BASE_VIEW0_2_2,
	GICR_BASE_VIEW0_2_3,
	GICR_BASE_VIEW0_3_0,
	GICR_BASE_VIEW0_3_1,
	GICR_BASE_VIEW0_3_2,
	GICR_BASE_VIEW0_3_3,
	0U				/* Zero Termination */
};

const mmap_region_t *tftf_platform_get_mmap(void)
{
	return mmap;
}

void tftf_platform_setup(void)
{
	plat_arm_gic_init();
	arm_gic_setup_global();
	arm_gic_setup_local();
}

void plat_arm_gic_init(void)
{
	/* GICC is not supported as GICv3 does not require it*/
	arm_gic_init_frames(0x0, GICD_BASE, rdaspen_gicr_base_addrs);
}
