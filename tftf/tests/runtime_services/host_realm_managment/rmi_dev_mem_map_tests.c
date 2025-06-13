/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>

#include <arch_features.h>
#include <host_da_helper.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_shared_data.h>
#include <pcie_doe.h>
#include <plat_topology.h>
#include <platform.h>
#include <test_helpers.h>

/* Maximum number of dev memory regions to test */
#define MAX_DEV_REGIONS		32U

#define NUM_L3_REGIONS		MAX_DEV_REGIONS
#define NUM_L2_REGIONS		8U

#define NUM_INFO_TESTS		3U

struct dev_mem_info {
	uintptr_t base_pa;		/* Dev memory region PA */
	long map_level;			/* RTT level */
	size_t map_size;		/* RTT level mapping size */
	unsigned int num_regions;	/* Number of granule/level 2 block regions */
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
	struct dev_mem_info mem_info[NUM_INFO_TESTS] = {
		/* PCIe memory region 1 */
		{0UL, RTT_MAX_LEVEL,
			RTT_MAP_SIZE(RTT_MAX_LEVEL), NUM_L3_REGIONS},
		/* PCIe memory region 2 */
		{0UL, RTT_MAX_LEVEL,
			RTT_MAP_SIZE(RTT_MAX_LEVEL), NUM_L3_REGIONS},
		/* PCIe memory region 2 */
		{0UL, RTT_MIN_DEV_BLOCK_LEVEL,
			RTT_MAP_SIZE(RTT_MIN_DEV_BLOCK_LEVEL), NUM_L2_REGIONS}
	};
	unsigned int num[NUM_INFO_TESTS];
	unsigned int offset, i, j;

	CHECK_DA_SUPPORT_IN_RMI(rmi_features);

	/* Initialise memory test structures */

	/*
	 * TODO: the test depends on 2 PCIe regions of suitable size and alignment.
	 * The test can be made more flexible and not depend on 2 PCIe regions.
	 */
	/* Retrieve platform PCIe memory region 1 */
	if (plat_get_dev_region((uint64_t *)&mem_info[0].base_pa, &dev_size,
				DEV_MEM_NON_COHERENT, 0U) != 0) {
		tftf_testcase_printf("Cannot retrieve PCIe memory region 0\n");
		return TEST_RESULT_SKIPPED;
	}

	if (dev_size < mem_info[0].num_regions * mem_info[0].map_size) {
		tftf_testcase_printf("PCIe memory region 0 too small\n");
		return TEST_RESULT_SKIPPED;
	}

	/* Retrieve platform PCIe memory region 2 */
	if (plat_get_dev_region((uint64_t *)&mem_info[1].base_pa, &dev_size,
				DEV_MEM_NON_COHERENT, 1U) != 0) {
		tftf_testcase_printf("Cannot retrieve PCIe memory region 1\n");
		return TEST_RESULT_SKIPPED;
	}

	mem_info[2].base_pa = mem_info[1].base_pa;

	if ((dev_size < mem_info[1].num_regions * mem_info[1].map_size) ||
	      (dev_size < mem_info[2].num_regions * mem_info[2].map_size)) {
		tftf_testcase_printf("PCIe memory region 1 too small\n");
		return TEST_RESULT_SKIPPED;
	}

	host_rmi_init_cmp_result();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	if (!host_create_activate_realm_payload(&realm,
						(u_register_t)REALM_IMAGE_BASE,
						feature_flag, 0U, sl, rec_flag, 1U, 0U)) {
		tftf_testcase_printf("Realm creation failed\n");
		return TEST_RESULT_FAIL;
	}

	/* Seed the random number generator */
	assert(is_feat_rng_present());
	srand((unsigned int)read_rndr());

	/*
	 * To use two 2MB blocks with level 3 mapping calculate
	 * random offset from the start of the 1st 2MB block.
	 */
	offset = (RTT_L2_BLOCK_SIZE / GRANULE_SIZE) -
			((unsigned int)rand() % (NUM_L3_REGIONS - 1U));

	mem_info[0].base_pa += offset * GRANULE_SIZE;

	offset = (RTT_L2_BLOCK_SIZE / GRANULE_SIZE) -
			((unsigned int)rand() % (NUM_L3_REGIONS - 1U));

	mem_info[1].base_pa += offset * GRANULE_SIZE;

	/*
	 * To use two 1GB blocks with level 2 mapping calculate
	 * random offset from the start of the 1st 1GB block.
	 */
	offset = (RTT_MAP_SIZE(1) / RTT_L2_BLOCK_SIZE) -
			((unsigned int)rand() % (NUM_L2_REGIONS - 1U));

	mem_info[2].base_pa += offset * RTT_L2_BLOCK_SIZE;

	/* Delegate device granules */
	for (i = 0U; i < NUM_INFO_TESTS; i++) {
		/* Number of granules */
		unsigned int num_granules = (mem_info[i].map_size / GRANULE_SIZE) *
							mem_info[i].num_regions;

		for (num[i] = 0U; num[i] < num_granules; num[i]++) {
			res = host_rmi_granule_delegate(mem_info[i].base_pa +
							num[i] * GRANULE_SIZE);
			if (res != RMI_SUCCESS) {
				tftf_testcase_printf(
					"%s() for 0x%lx failed, 0x%lx\n",
					"host_rmi_granule_delegate",
					(mem_info[i].base_pa + num[i] * GRANULE_SIZE),
					res);
				goto undelegate_granules;
			}
		}
	}

	/* Map device memory */
	for (i = 0U; i < NUM_INFO_TESTS; i++) {
		for (j = 0U; j < mem_info[i].num_regions; j++) {
			res = host_dev_mem_map(&realm,
					mem_info[i].base_pa + j * mem_info[i].map_size,
					mem_info[i].map_level, &map_addr[i][j]);
			if (res != REALM_SUCCESS) {
				tftf_testcase_printf("%s() for 0x%lx failed, 0x%lx\n",
					"host_realm_dev_mem_map",
					mem_info[i].base_pa + j * mem_info[i].map_size,
					res);
				goto undelegate_granules;
			}
		}
	}

	/* Check RTT entries */
	for (i = 0U; i < NUM_INFO_TESTS; i++) {
		for (j = 0U; j < mem_info[i].num_regions; j++) {
			res = host_rmi_rtt_readentry(realm.rd, map_addr[i][j],
						mem_info[i].map_level, &rtt);
			if (res != RMI_SUCCESS) {
				tftf_testcase_printf("%s() for 0x%lx failed, 0x%lx\n",
					"host_rmi_rtt_readentry",
					map_addr[i][j], res);
				goto undelegate_granules;
			}

			if ((rtt.state != RMI_ASSIGNED_DEV) ||
				(rtt.ripas != RMI_EMPTY) ||
				(rtt.walk_level != mem_info[i].map_level) ||
				(rtt.out_addr != map_addr[i][j])) {
				tftf_testcase_printf("RTT entry for 0x%lx:\n", map_addr[i][j]);
				tftf_testcase_printf(
					"%s level:%ld addr:0x%lx state:%lu ripas:%lu\n",
					"Expected", mem_info[i].map_level, map_addr[i][j],
					RMI_ASSIGNED_DEV, RMI_EMPTY);
				tftf_testcase_printf(
					"%s level:%ld addr:0x%lx state:%lu ripas:%lu\n",
					"Read    ", rtt.walk_level,
					(unsigned long)rtt.out_addr, rtt.state, rtt.ripas);
				goto undelegate_granules;
			}
		}
	}

	/* Unmap device memory */
	for (i = 0U; i < NUM_INFO_TESTS; i++) {
		for (j = 0U; j < mem_info[i].num_regions; j++) {
			u_register_t pa, pa_exp, top, top_exp;

			res = host_rmi_dev_mem_unmap(realm.rd, map_addr[i][j],
							mem_info[i].map_level,
							&pa, &top);
			if (res != RMI_SUCCESS) {
				tftf_testcase_printf("%s() for 0x%lx failed, 0x%lx\n",
							"host_rmi_dev_mem_unmap",
							map_addr[i][j], res);
				goto undelegate_granules;
			}

			INFO("DEV_MEM_UNMAP 0x%lx: pa 0x%lx top 0x%lx\n",
				map_addr[i][j], pa, top);

			pa_exp = mem_info[i].base_pa + j * mem_info[i].map_size;

			/* Check PA of the device memory region which was unmapped. */
			if (pa != pa_exp) {
				tftf_testcase_printf("%s() for 0x%lx failed, "
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
				tftf_testcase_printf("%s() for 0x%lx failed, "
						"expected top 0x%lx, returned 0x%lx\n",
						"host_rmi_dev_mem_unmap",
						map_addr[i][j], top_exp, top);
				goto undelegate_granules;
			}
		}
	}

	/* Check RTT entries */
	for (i = 0U; i < NUM_INFO_TESTS; i++) {
		for (j = 0U; j < mem_info[i].num_regions; j++) {
			res = host_rmi_rtt_readentry(realm.rd, map_addr[i][j],
						mem_info[i].map_level, &rtt);
			if (res != RMI_SUCCESS) {
				tftf_testcase_printf("%s() for 0x%lx failed, 0x%lx\n",
					"host_rmi_rtt_readentry",
					map_addr[i][j], res);
				goto undelegate_granules;
			}

			if ((rtt.state != RMI_UNASSIGNED) ||
				(rtt.ripas != RMI_EMPTY)) {
				tftf_testcase_printf("%s() for 0x%lx failed, "
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
		for (j = 0U; j < num[i]; j++) {
			res = host_rmi_granule_undelegate(mem_info[i].base_pa + j * GRANULE_SIZE);
			if (res != RMI_SUCCESS) {
				tftf_testcase_printf("%s for 0x%lx failed, 0x%lx\n",
							"host_rmi_granule_undelegate",
							(mem_info[i].base_pa + j * GRANULE_SIZE),
							res);
				ret = TEST_RESULT_FAIL;
				break;
			}
		}
	}

	if (!host_destroy_realm(&realm)) {
		tftf_testcase_printf("host_destroy_realm() failed\n");
		return TEST_RESULT_FAIL;
	}

	if (ret == TEST_RESULT_FAIL) {
		return ret;
	}

	return host_cmp_result();
}

