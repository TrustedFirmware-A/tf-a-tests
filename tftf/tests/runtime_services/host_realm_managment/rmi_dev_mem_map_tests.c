/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>

#include <arch_features.h>
#include <cassert.h>
#include <host_da_helper.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_shared_data.h>
#include <plat_topology.h>
#include <platform.h>
#include <test_helpers.h>

CASSERT(RTT_MIN_DEV_BLOCK_LEVEL == 2L, min_dev_block_level_mismatch);

/* Maximum number of dev memory regions to test */
#define MAX_DEV_REGIONS		32U

#define NUM_L3_REGIONS		MAX_DEV_REGIONS
#define NUM_L2_REGIONS		8U

#define NUM_INFO_TESTS		3U

struct dev_mem_info {
	uintptr_t base_pa;		/* Dev memory region PA */
	size_t min_size;		/* Dev memory region size required */
	long map_level;			/* RTT level */
	size_t map_size;		/* RTT level mapping size */
	unsigned int num_regions;	/* Number of granule/level 2 block regions */
};

struct dev_mem_region {
	uintptr_t base;			/* Dev memory region base */
	size_t size;			/* Dev memory region size */
};

/*
 * @Test_Aim@ Test device memory map and unmap commands
 *
 * Delegate device granules for 2 memory regions.
 * Dev Mem Map/Unmap 2 device memory regions with page and block mapping levels.
 * Check returned 'pa' and 'top' output values after Dev Mem Unmap.
 * Check RTT entries states and RIPAS values after Dev Mem Map/Unmap.
 * Undelegate device granules.
 */
test_result_t host_realm_dev_mem_map_unmap(void)
{
	u_register_t rec_flag[] = {RMI_RUNNABLE};
	u_register_t map_addr[NUM_INFO_TESTS][MAX_DEV_REGIONS];
	u_register_t feature_flag = 0UL;
	u_register_t res, rmi_features;
	__unused size_t dev_size;
	test_result_t ret = TEST_RESULT_FAIL;
	long sl = RTT_MIN_LEVEL;
	struct realm realm;
	struct rtt_entry rtt;
	struct dev_mem_region mem_region[2];
	struct dev_mem_info mem_info[NUM_INFO_TESTS] = {
		/* Test region 1 */
		{0UL, 2 * RTT_L2_BLOCK_SIZE, RTT_MAX_LEVEL,
		 RTT_MAP_SIZE(RTT_MAX_LEVEL), NUM_L3_REGIONS},

		/* Test region 2 */
		{0UL, 2 * RTT_L2_BLOCK_SIZE, RTT_MAX_LEVEL,
		 RTT_MAP_SIZE(RTT_MAX_LEVEL), NUM_L3_REGIONS},

		/* Test region 2 */
		{0UL, 2 * RTT_L1_BLOCK_SIZE, RTT_MIN_DEV_BLOCK_LEVEL,
		 RTT_MAP_SIZE(RTT_MIN_DEV_BLOCK_LEVEL), NUM_L2_REGIONS}
	};
	unsigned int num[NUM_INFO_TESTS];
	unsigned int num_reg, offset, i, j;

	SKIP_DA_TEST_IF_PREREQS_NOT_MET(rmi_features);

	/* Initialise memory test structures */

	/* Retrieve platform PCIe memory regions */
	for (num_reg = 0U; num_reg < 2U; num_reg++) {
		if (plat_get_dev_region((uint64_t *)&mem_region[num_reg].base,
					&mem_region[num_reg].size,
					DEV_MEM_NON_COHERENT, num_reg) != 0) {
			break;
		}

		INFO("PCIe memory region %u 0x%lx-0x%lx\n",
			num_reg, mem_region[num_reg].base,
			mem_region[num_reg].base +
			mem_region[num_reg].size - 1UL);
	}

	if (num_reg == 0U) {
		INFO("Cannot retrieve PCIe memory regions\n");
		return TEST_RESULT_SKIPPED;
	}

	if (num_reg == 1U) {
		/* Found 1 PCIe memory region */
		if (mem_region[0].size < mem_info[0].min_size) {
			INFO("PCIe memory region 0 too small\n");
			return TEST_RESULT_SKIPPED;
		}

		if (mem_region[0].size <
		    mem_info[1].min_size + mem_info[2].min_size) {
			/* Setup test region 1 */
			mem_info[0].base_pa = mem_region[0].base;
		} else {
			/* Setup test region 2 */
			mem_info[1].base_pa = mem_region[0].base;
			mem_info[2].base_pa = mem_region[0].base;
		}
	} else {
		/* Found 2 PCIe memory regions */
		unsigned int reg_s, reg_b;

		/* Find smaller and bigger region */
		reg_s = (mem_region[0].size < mem_region[1].size) ? 0U : 1U;
		reg_b = reg_s ^ 1U;

		if (mem_region[reg_b].size < mem_info[0].min_size) {
			INFO("PCIe memory regions too small\n");
			return TEST_RESULT_SKIPPED;
		}

		if (mem_region[reg_s].size >= mem_info[0].min_size) {
			/* Setup test region 1 to the smaller region */
			mem_info[0].base_pa = mem_region[reg_s].base;

			if (mem_region[reg_b].size >=
			    mem_info[1].min_size + mem_info[2].min_size) {
				/* Setup test region 2 to the bigger region */
				mem_info[1].base_pa = mem_region[reg_b].base;
				mem_info[2].base_pa = mem_region[reg_b].base;
			}
		} else {
			if (mem_region[reg_b].size <
			    mem_info[1].min_size + mem_info[2].min_size) {
				/* Setup test region 1 to the bigger region */
				mem_info[0].base_pa = mem_region[reg_b].base;
			} else {
				/* Setup test region 2 to the bigger region */
				mem_info[1].base_pa = mem_region[reg_b].base;
				mem_info[2].base_pa = mem_region[reg_b].base;
			}
		}
	}

	host_rmi_init_cmp_result();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	if (!host_create_activate_realm_payload(&realm,
						(u_register_t)REALM_IMAGE_BASE,
						feature_flag, 0U, sl, rec_flag,
						1U, 0U, get_test_mecid())) {
		ERROR("Realm creation failed\n");
		return TEST_RESULT_FAIL;
	}

	/* Seed the random number generator */
	assert(is_feat_rng_present());
	srand((unsigned int)read_rndr());

	/*
	 * To use two 2MB blocks with level 3 mapping calculate
	 * random offset from the start of the 1st 2MB block.
	 */
	if (mem_info[0].base_pa != 0UL) {
		offset = (RTT_L2_BLOCK_SIZE / GRANULE_SIZE) -
			 ((unsigned int)rand() % (NUM_L3_REGIONS - 1U));
		mem_info[0].base_pa += offset * GRANULE_SIZE;
	}

	if (mem_info[1].base_pa != 0UL) {
		offset = (RTT_L2_BLOCK_SIZE / GRANULE_SIZE) -
			 ((unsigned int)rand() % (NUM_L3_REGIONS - 1U));
		mem_info[1].base_pa += offset * GRANULE_SIZE;
	}

	/*
	 * To use two 1GB blocks with level 2 mapping calculate
	 * random offset from the start of the 1st 1GB block.
	 */
	if (mem_info[2].base_pa != 0UL) {
		offset = (RTT_L1_BLOCK_SIZE / RTT_L2_BLOCK_SIZE) -
			 ((unsigned int)rand() % (NUM_L2_REGIONS - 1U));
		mem_info[2].base_pa += offset * RTT_L2_BLOCK_SIZE;
	}

	/* Delegate device granules */
	for (i = 0U; i < NUM_INFO_TESTS; i++) {
		unsigned int num_granules;	/* number of granules */

		/* Skip non-initialised test region */
		if (mem_info[i].base_pa == 0UL) {
			continue;
		}

		num_granules = (mem_info[i].map_size / GRANULE_SIZE) *
							mem_info[i].num_regions;

		for (num[i] = 0U; num[i] < num_granules; num[i]++) {
			res = host_rmi_granule_delegate(mem_info[i].base_pa +
							num[i] * GRANULE_SIZE);
			if (res != RMI_SUCCESS) {
				ERROR("%s() for 0x%lx failed, 0x%lx\n",
					"host_rmi_granule_delegate",
					(mem_info[i].base_pa + num[i] * GRANULE_SIZE),
					res);
				goto undelegate_granules;
			}
		}
	}

	/* Map device memory */
	for (i = 0U; i < NUM_INFO_TESTS; i++) {
		/* Skip non-initialised test region */
		if (mem_info[i].base_pa == 0UL) {
			continue;
		}

		for (j = 0U; j < mem_info[i].num_regions; j++) {
			res = host_dev_mem_map(&realm,
					mem_info[i].base_pa + j * mem_info[i].map_size,
					mem_info[i].map_level, &map_addr[i][j]);
			if (res != REALM_SUCCESS) {
				ERROR("%s() for 0x%lx failed, 0x%lx\n",
					"host_realm_dev_mem_map",
					mem_info[i].base_pa + j * mem_info[i].map_size,
					res);
				goto undelegate_granules;
			}
		}
	}

	/* Check RTT entries */
	for (i = 0U; i < NUM_INFO_TESTS; i++) {
		/* Skip non-initialised test region */
		if (mem_info[i].base_pa == 0UL) {
			continue;
		}

		for (j = 0U; j < mem_info[i].num_regions; j++) {
			res = host_rmi_rtt_readentry(realm.rd, map_addr[i][j],
						mem_info[i].map_level, &rtt);
			if (res != RMI_SUCCESS) {
				ERROR("%s() for 0x%lx failed, 0x%lx\n",
					"host_rmi_rtt_readentry",
					map_addr[i][j], res);
				goto undelegate_granules;
			}

			if ((rtt.state != RMI_ASSIGNED_DEV) ||
				(rtt.ripas != RMI_EMPTY) ||
				(rtt.walk_level != mem_info[i].map_level) ||
				(rtt.out_addr != map_addr[i][j])) {
				ERROR("RTT entry for 0x%lx:\n", map_addr[i][j]);
				ERROR("%s level:%ld addr:0x%lx state:%lu ripas:%lu\n",
					"Expected", mem_info[i].map_level, map_addr[i][j],
					RMI_ASSIGNED_DEV, RMI_EMPTY);
				ERROR("%s level:%ld addr:0x%lx state:%lu ripas:%lu\n",
					"Read    ", rtt.walk_level,
					(unsigned long)rtt.out_addr, rtt.state, rtt.ripas);
				goto undelegate_granules;
			}
		}
	}

	/* Unmap device memory */
	for (i = 0U; i < NUM_INFO_TESTS; i++) {
		/* Skip non-initialised test region */
		if (mem_info[i].base_pa == 0UL) {
			continue;
		}

		for (j = 0U; j < mem_info[i].num_regions; j++) {
			u_register_t pa, pa_exp, top, top_exp;

			res = host_rmi_dev_mem_unmap(realm.rd, map_addr[i][j],
							mem_info[i].map_level,
							&pa, &top);
			if (res != RMI_SUCCESS) {
				ERROR("%s() for 0x%lx failed, 0x%lx\n",
					"host_rmi_dev_mem_unmap",
					map_addr[i][j], res);
				goto undelegate_granules;
			}

			INFO("DEV_MEM_UNMAP 0x%lx: pa 0x%lx top 0x%lx\n",
				map_addr[i][j], pa, top);

			pa_exp = mem_info[i].base_pa + j * mem_info[i].map_size;

			/* Check PA of the device memory region which was unmapped. */
			if (pa != pa_exp) {
				ERROR("%s() for 0x%lx failed, "
					"expected pa 0x%lx, returned 0x%lx\n",
					"host_rmi_dev_mem_unmap",
					map_addr[i][j], pa_exp, pa);
				goto undelegate_granules;
			}

			/*
			 * Check top IPA of non-live RTT entries, from entry
			 * at which the RTT walk terminated.
			 */
			if (j == (mem_info[i].num_regions - 1U)) {
				/* IPA aligned to the previous mapping level */
				top_exp = ALIGN_DOWN(map_addr[i][j],
						RTT_MAP_SIZE(mem_info[i].map_level - 1L)) +
						RTT_MAP_SIZE(mem_info[i].map_level - 1L);
			} else {
				/* Next IPA of the current mapping level */
				top_exp = map_addr[i][j] + mem_info[i].map_size;
			}

			if (top != top_exp) {
				ERROR("%s() for 0x%lx failed, "
					"expected top 0x%lx, returned 0x%lx\n",
					"host_rmi_dev_mem_unmap",
					map_addr[i][j], top_exp, top);
				goto undelegate_granules;
			}
		}
	}

	/* Check RTT entries */
	for (i = 0U; i < NUM_INFO_TESTS; i++) {
		/* Skip non-initialised test region */
		if (mem_info[i].base_pa == 0UL) {
			continue;
		}

		for (j = 0U; j < mem_info[i].num_regions; j++) {
			res = host_rmi_rtt_readentry(realm.rd, map_addr[i][j],
						mem_info[i].map_level, &rtt);
			if (res != RMI_SUCCESS) {
				ERROR("%s() for 0x%lx failed, 0x%lx\n",
					"host_rmi_rtt_readentry",
					map_addr[i][j], res);
				goto undelegate_granules;
			}

			if ((rtt.state != RMI_UNASSIGNED) ||
				(rtt.ripas != RMI_EMPTY)) {
				ERROR("%s() for 0x%lx failed, "
					"expected state %lu ripas %lu, read %lu %lu",
					"host_rmi_rtt_readentry",
					map_addr[i][j], RMI_UNASSIGNED, RMI_EMPTY,
					rtt.state, rtt.ripas);
				goto undelegate_granules;
			}
		}
	}

	ret = TEST_RESULT_SUCCESS;

undelegate_granules:
	for (i = 0U; i < NUM_INFO_TESTS; i++) {
		/* Skip non-initialised test region */
		if (mem_info[i].base_pa == 0UL) {
			continue;
		}

		for (j = 0U; j < num[i]; j++) {
			res = host_rmi_granule_undelegate(mem_info[i].base_pa + j * GRANULE_SIZE);
			if (res != RMI_SUCCESS) {
				ERROR("%s for 0x%lx failed, 0x%lx\n",
					"host_rmi_granule_undelegate",
					(mem_info[i].base_pa + j * GRANULE_SIZE),
					res);
				ret = TEST_RESULT_FAIL;
				break;
			}
		}
	}

	if (!host_destroy_realm(&realm)) {
		ERROR("host_destroy_realm() failed\n");
		return TEST_RESULT_FAIL;
	}

	if (ret == TEST_RESULT_FAIL) {
		return ret;
	}

	return host_cmp_result();
}

