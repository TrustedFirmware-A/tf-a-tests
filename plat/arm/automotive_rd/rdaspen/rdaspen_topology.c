/*
 * Copyright (c) 2025-2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <assert.h>
#include <platform_def.h>
#include <plat_topology.h>
#include <stddef.h>
#include <tftf_lib.h>

const unsigned char rdaspen_power_domain_tree_desc[] = {
	RDASPEN_CLUSTER_COUNT,
	PLATFORM_CLUSTER_0_CORE_COUNT,
#if (PLATFORM_CLUSTER_COUNT > 1)
	PLATFORM_CLUSTER_1_CORE_COUNT,
#endif
#if (PLATFORM_CLUSTER_COUNT > 2)
	PLATFORM_CLUSTER_2_CORE_COUNT,
#endif
#if (PLATFORM_CLUSTER_COUNT > 3)
	PLATFORM_CLUSTER_3_CORE_COUNT,
#endif
};

const unsigned char *tftf_plat_get_pwr_domain_tree_desc(void)
{
	return rdaspen_power_domain_tree_desc;
}

uint64_t tftf_plat_get_mpidr(unsigned int core_pos)
{
	assert(core_pos < PLATFORM_CORE_COUNT);

	return make_mpid(core_pos / PLAT_MAX_CPUS_PER_CLUSTER,
			 core_pos % PLAT_MAX_CPUS_PER_CLUSTER);
}
