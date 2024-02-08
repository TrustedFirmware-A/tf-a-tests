/*
 * Copyright (c) 2022-2024, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <assert.h>
#include <mmio.h>
#include <plat_topology.h>
#include <platform_def.h>
#include <stddef.h>
#include <tftf_lib.h>

static const struct {
	unsigned int cluster_id;
	unsigned int cpu_id;
} zynqmp_cores[PLATFORM_CORE_COUNT] = {
	{ 0, 0 },
	{ 0, 1 },
	{ 0, 2 },
	{ 0, 3 }
};

static const unsigned char zynqmp_power_domain_tree_desc[] = {
	/* Number of root nodes */
	PLATFORM_SYSTEM_COUNT,
	/* Number of children of root node */
	PLATFORM_CLUSTER_COUNT,
	/* Number of children for the cluster */
	PLATFORM_CORES_PER_CLUSTER
};

const unsigned char *tftf_plat_get_pwr_domain_tree_desc(void)
{
	return zynqmp_power_domain_tree_desc;
}

uint64_t tftf_plat_get_mpidr(unsigned int core_pos)
{
	assert(core_pos < PLATFORM_CORE_COUNT);

	return make_mpid(zynqmp_cores[core_pos].cluster_id,
			 zynqmp_cores[core_pos].cpu_id);
}
