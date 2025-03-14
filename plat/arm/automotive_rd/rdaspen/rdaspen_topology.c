/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

 #include <arch.h>
 #include <assert.h>
 #include <platform_def.h>
 #include <plat_topology.h>
 #include <stddef.h>
 #include <tftf_lib.h>

static const struct {
	unsigned int cluster_id;
	unsigned int cpu_id;
} rdaspen_cores[] = {
	/* Cluster0: 4 cores*/
	{ 0, 0 },
	{ 0, 1 },
	{ 0, 2 },
	{ 0, 3 },
	/* Cluster1: 4 cores */
	{ 1, 0 },
	{ 1, 1 },
	{ 1, 2 },
	{ 1, 3 },
	/* Cluster2: 4 cores*/
	{ 2, 0 },
	{ 2, 1 },
	{ 2, 2 },
	{ 2, 3 },
	/* Cluster3: 4 cores */
	{ 3, 0 },
	{ 3, 1 },
	{ 3, 2 },
	{ 3, 3 },
};

const unsigned char aspen_power_domain_tree_desc[] = {
	RDASPEN_CLUSTER_COUNT,
	RDASPEN_CLUSTER_CORE_COUNT,
	RDASPEN_CLUSTER_CORE_COUNT,
	RDASPEN_CLUSTER_CORE_COUNT,
	RDASPEN_CLUSTER_CORE_COUNT,
};

const unsigned char *tftf_plat_get_pwr_domain_tree_desc(void)
{
	return aspen_power_domain_tree_desc;
}

uint64_t tftf_plat_get_mpidr(unsigned int core_pos)
{
	assert(core_pos < PLATFORM_CORE_COUNT);
	return make_mpid(rdaspen_cores[core_pos].cluster_id,
			rdaspen_cores[core_pos].cpu_id);
}
