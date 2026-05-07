/*
 * Copyright (c) 2024-2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <arch.h>
#include <tftf_lib.h>

#include <platform_def.h>
#include <util.h>

/*
 * Topology configurations for different Versal2 variants
 */
#if (VERSAL2_VARIANT == 14)
static const struct {
	unsigned int cluster_id;
	unsigned int cpu_id;
} versal2_cores[PLATFORM_CORE_COUNT] = {
	CLUSTER_DEF(0)
};

/* Power Domain Tree Descriptor: 1 cluster, 4 cores (VERSAL2_VARIANT=14) */
const unsigned char versal2_pwr_tree_desc[] = {
	/* Number of root nodes */
	1,
	/* Number of clusters */
	PLATFORM_CLUSTER_COUNT,
	/* Number of children for the cluster */
	PLATFORM_CORE_COUNT_PER_CLUSTER
};

#else
/* Default (VERSAL2_VARIANT=42): 4 clusters with 2 cores per cluster */
static const struct {
	unsigned int cluster_id;
	unsigned int cpu_id;
} versal2_cores[PLATFORM_CORE_COUNT] = {
	CLUSTER_DEF(0),
	CLUSTER_DEF(1),
	CLUSTER_DEF(2),
	CLUSTER_DEF(3)
};

/* Power Domain Tree Descriptor for Default Variant */
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

#endif

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
