/*
 * Copyright (c) 2024, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <arch.h>
#include <drivers/arm/arm_gic.h>
#include <drivers/console.h>
#include <platform.h>
#include <tftf_lib.h>

#include <platform_def.h>
#include <util.h>

static const struct {
	unsigned int cluster_id;
	unsigned int cpu_id;
} versal2_cores[PLATFORM_CORE_COUNT] = {
	CLUSTER_DEF(0),
	CLUSTER_DEF(1),
	CLUSTER_DEF(2),
	CLUSTER_DEF(3)
};


static const mmap_region_t mmap[] = {
	MAP_REGION_FLAT(DRAM_BASE + TFTF_NVM_OFFSET, TFTF_NVM_SIZE, MT_MEMORY | MT_RW | MT_NS),
	MAP_REGION_FLAT(GIC_BASE, GIC_SIZE, MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(CRASH_CONSOLE_BASE, CRASH_CONSOLE_SIZE, MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(TTC_BASE, TTC_SIZE, MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(LPD_IOU_SLCR, LPD_IOU_SLCR_SIZE, MT_DEVICE | MT_RW | MT_NS),
	{0}
};

/* Power Domain Tree Descriptor array */
const unsigned char versal2_pwr_tree_desc[] = {
	/* Number of root nodes */
	1,
	/* Number of clusters */
	PLATFORM_CLUSTER_COUNT,
	/* Number of children for the first cluster node */
	PLATFORM_CORE_COUNT_PER_CLUSTER,
	/* Number of children for the second cluster node */
	PLATFORM_CORE_COUNT_PER_CLUSTER,
	/* Number of children for the third cluster node */
	PLATFORM_CORE_COUNT_PER_CLUSTER,
	/* Number of children for the fourth cluster node */
	PLATFORM_CORE_COUNT_PER_CLUSTER
};


const unsigned char *tftf_plat_get_pwr_domain_tree_desc(void)
{
	return versal2_pwr_tree_desc;
}

/*
 * Generate the MPID from the core position.
 */
uint64_t tftf_plat_get_mpidr(unsigned int core_pos)
{
	assert(core_pos < PLATFORM_CORE_COUNT);

	return (uint64_t)make_mpid(versal2_cores[core_pos].cluster_id,
				versal2_cores[core_pos].cpu_id);
}

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
