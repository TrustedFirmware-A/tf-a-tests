/*
 * Copyright (c) 2022-2023, Advanced Micro Devices, Inc. All rights reserved.
 * Copyright (c) 2024, Linaro Limited.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <arch.h>
#include <drivers/arm/arm_gic.h>
#include <drivers/console.h>
#include <platform.h>
#include <tftf_lib.h>
#include <libfdt.h>

#include <platform_def.h>

static const mmap_region_t mmap[] = {
	MAP_REGION_FLAT(DRAM_BASE, DRAM_SIZE, MT_MEMORY | MT_RW | MT_NS),
	MAP_REGION_FLAT(DEVICE0_BASE, DEVICE0_SIZE, MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(CRASH_CONSOLE_BASE, CRASH_CONSOLE_SIZE, MT_DEVICE | MT_RW | MT_NS),
	{0}
};

/* Power Domain Tree Descriptor array */
const unsigned char qemu_pwr_tree_desc[] = {
	/* Number of root nodes */
	1,
	/* Number of clusters */
	PLATFORM_CLUSTER_COUNT,
	/* Number of children for the first cluster node */
	PLATFORM_CORE_COUNT_PER_CLUSTER,
	/* Number of children for the second cluster node */
	PLATFORM_CORE_COUNT_PER_CLUSTER,
};

static unsigned int core_count;

const unsigned char *tftf_plat_get_pwr_domain_tree_desc(void)
{
	return qemu_pwr_tree_desc;
}

/*
 * Generate the MPID from the core position.
 */
uint64_t tftf_plat_get_mpidr(unsigned int core_pos)
{
	if (core_pos >= core_count)
		return INVALID_MPID;

	return (uint64_t)make_mpid(core_pos / PLATFORM_CORE_COUNT_PER_CLUSTER,
				   core_pos % PLATFORM_CORE_COUNT_PER_CLUSTER);
}

void tftf_plat_arch_setup(void)
{
	tftf_plat_configure_mmu();
}

void tftf_early_platform_setup(void)
{
	console_init(CRASH_CONSOLE_BASE, PL011_UART_CLK_IN_HZ, PL011_BAUDRATE);
}

static bool is_cpu_node(void *fdt, int offs)
{
	int prop_len;
	const void *prop;

	/* Looking for the property, device_type = "cpu" */
	prop = fdt_getprop(fdt, offs, "device_type", &prop_len);
	if (!prop)
		return false;
	return !memcmp(prop, "cpu", strlen("cpu") + 1);
}

static void init_core_count(void)
{
	void *fdt = (void *)PLAT_QEMU_DT_BASE;
	int offs;
	int ret;

	ret = fdt_check_header(fdt);
	if (ret < 0) {
		ERROR("Invalid Device Tree at %p: error %d\n", fdt, ret);
		panic();
	}

	offs = fdt_path_offset(fdt, "/cpus");
	if (offs < 0) {
		ERROR("Can't find \"/cpus\" path\n");
		panic();
	}

	offs = fdt_first_subnode(fdt, offs);
	if (offs < 0) {
		ERROR("Can't find the first cpu subnode\n");
		panic();
	}

	core_count = 0;
	if (is_cpu_node(fdt, offs)) {
		INFO("Found cpu subnode %s\n", fdt_get_name(fdt, offs, NULL));
		core_count++;
	}

	for (offs = fdt_next_subnode(fdt, offs); offs >= 0;
	     offs = fdt_next_subnode(fdt, offs)) {
		if (is_cpu_node(fdt, offs)) {
			INFO("Found cpu subnode %s\n",
			     fdt_get_name(fdt, offs, NULL));
			core_count++;
		}
	}
}

void tftf_platform_setup(void)
{
	init_core_count();
	arm_gic_init(GICC_BASE, GICD_BASE, GICR_BASE);
	arm_gic_setup_global();
	arm_gic_setup_local();
}

const mmap_region_t *tftf_platform_get_mmap(void)
{
	return mmap;
}

