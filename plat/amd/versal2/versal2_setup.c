/*
 * Copyright (c) 2024-2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <drivers/arm/arm_gic.h>
#include <drivers/console.h>
#include <platform.h>
#include <tftf_lib.h>

#include <platform_def.h>

static const mmap_region_t mmap[] = {
	MAP_REGION_FLAT(DRAM_BASE + TFTF_NVM_OFFSET, TFTF_NVM_SIZE, MT_MEMORY | MT_RW | MT_NS),
	MAP_REGION_FLAT(GIC_BASE, GIC_SIZE, MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(CRASH_CONSOLE_BASE, CRASH_CONSOLE_SIZE, MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(TTC_BASE, TTC_SIZE, MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(LPD_IOU_SLCR, LPD_IOU_SLCR_SIZE, MT_DEVICE | MT_RW | MT_NS),
	{0}
};

void tftf_plat_arch_setup(void)
{
	tftf_plat_configure_mmu();
}

void tftf_early_platform_setup(void)
{
	console_init(CRASH_CONSOLE_BASE, PL011_UART_CLK_IN_HZ, PL011_BAUDRATE);
}

void tftf_platform_setup(void)
{
	arm_gic_init(GICC_REG_BASE, GICD_REG_BASE, GICR_REG_BASE);
	arm_gic_setup_global();
	arm_gic_setup_local();
}

const mmap_region_t *tftf_platform_get_mmap(void)
{
	return mmap;
}
