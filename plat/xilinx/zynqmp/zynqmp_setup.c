/*
 * Copyright (c) 2022-2024, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <drivers/console.h>
#include <drivers/arm/gic_v2v3_common.h>
#include <drivers/arm/gic_v2.h>
#include <platform.h>
#include <platform_def.h>
#include <xlat_tables_v2.h>
#include <drivers/console.h>
#include <debug.h>
#include <drivers/arm/arm_gic.h>

static const mmap_region_t zynqmp_mmap[] = {
	MAP_REGION_FLAT(DRAM_BASE + TFTF_NVM_OFFSET, TFTF_NVM_SIZE, MT_MEMORY | MT_RW | MT_NS),
	MAP_REGION_FLAT(GIC_BASE, GIC_SIZE, MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(ZYNQMP_UART_BASE, CRASH_CONSOLE_SIZE, MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(TTC_BASE, TTC_SIZE, MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(LPD_IOU_SLCR, LPD_IOU_SLCR_SIZE, MT_DEVICE | MT_RW | MT_NS),
	{0}
};

const mmap_region_t *tftf_platform_get_mmap(void)
{
	return zynqmp_mmap;
}

void tftf_plat_arch_setup(void)
{
	tftf_plat_configure_mmu();
}

void tftf_early_platform_setup(void)
{
	console_init(ZYNQMP_UART_BASE, ZYNQMP_CRASH_UART_CLK_IN_HZ,
		     ZYNQMP_UART_BAUDRATE);
}

void plat_arm_gic_init(void)
{
	arm_gic_init(BASE_GICC_BASE, BASE_GICD_BASE, 0);
}

void tftf_platform_setup(void)
{
	plat_arm_gic_init();
	arm_gic_setup_global();
	arm_gic_setup_local();
}
