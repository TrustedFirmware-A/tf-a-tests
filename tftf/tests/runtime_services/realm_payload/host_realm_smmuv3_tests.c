/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>

#include <arch_features.h>
#include <heap/page_alloc.h>
#include <host_da_flow_helper.h>
#include <host_da_helper.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_shared_data.h>
#include <pci_tdisp.h>
#include <test_helpers.h>
#include <tftf_lib.h>

u_register_t host_rmi_data_create(bool unknown,
				  u_register_t rd,
				  u_register_t data,
				  u_register_t map_addr,
				  u_register_t src);

/*
 * @Test_Aim@ Test realm SMMUv3
 *
 * This function tests SMMUv3 functionality in Realm
 *
 * @return test result
 */
test_result_t host_test_realm_smmuv3(void)
{
	struct realm realm;
	u_register_t rmi_feat_reg0;
	u_register_t res, exit_reason, out_top = 0UL;
	u_register_t *map_addr;
	struct host_pdev *h_pdev;
	struct host_vdev *h_vdev;
	struct rmi_rec_run *run;
	pci_tdisp_device_interface_report_struct_t *ifc_report;
	pci_tdisp_mmio_range_t *mmio_range;
	struct rmi_address_range addr_range[MAX_ADDR_RANGE_NUM];
	unsigned int num_dlg[MAX_ADDR_RANGE_NUM] = {0U};
	unsigned int host_call_result = TEST_RESULT_FAIL;
	unsigned int num_gran = 0U, num_map = 0U;
	unsigned int range_count;
	test_result_t result = TEST_RESULT_SUCCESS;
	bool return_error = false;
	bool bar64_waround;
	int rc;

	host_rmi_init_cmp_result();

	INIT_AND_SKIP_DA_TEST_IF_PREREQS_NOT_MET(rmi_feat_reg0);

	/*
	 * Create a Realm with DA feature enabled
	 *
	 * TODO: creating this after host_pdev_setup causes Realm create to
	 * fail.
	 */
	rc = host_create_realm_with_feat_da(&realm);
	if (rc != 0) {
		ERROR("Realm create with feat_da failed\n");
		return TEST_RESULT_FAIL;
	}

	/* Find device with TSM */
	h_pdev = get_host_pdev_by_type(DEV_TYPE_INDEPENDENTLY_ATTESTED);
	if (h_pdev == NULL) {
		/* If no device is connected to TSM, then skip the test */
		result = TEST_RESULT_SKIPPED;
		goto destroy_realm;
	}

	/* Connect device with TSM */
	rc = tsm_connect_device(h_pdev);
	if (rc != 0) {
		ERROR("TSM connect failed for device 0x%x\n", h_pdev->dev->bdf);
		return_error = true;
		goto destroy_realm;
	}

	h_vdev = &gbl_host_vdev;

	/* Assign TSM connected device to a Realm */
	rc = realm_assign_device(&realm, h_vdev, h_pdev->dev->bdf, h_pdev->pdev);
	if (rc != 0) {
		return_error = true;
		goto destroy_realm;
	}

	/* Get interface report */
	ifc_report = (pci_tdisp_device_interface_report_struct_t *)h_vdev->ifc_report;

	/* Dump interface report */
	host_dump_interface_report(ifc_report, h_vdev->ifc_report_len);
	assert(ifc_report->mmio_range_count <= MAX_ADDR_RANGE_NUM);

	range_count = ifc_report->mmio_range_count;

	mmio_range = (pci_tdisp_mmio_range_t *)(ifc_report + 1UL);

	/* Check if work-around for 64-bit BARs is required */
	bar64_waround = (h_pdev->ncoh_addr_range[0].base >
				mmio_range[0].first_page * PAGE_SIZE);

	INFO("Using %s address ranges\n", bar64_waround ? "BARs" : "MMIO");

	for (unsigned int i = 0U; i < range_count; i++) {
		if (bar64_waround) {
			/* Use address ranges from BARs */
			addr_range[i].base = h_pdev->ncoh_addr_range[i].base;
			addr_range[i].top = h_pdev->ncoh_addr_range[i].top;
		} else {
			/* Use address ranges from interface report */
			addr_range[i].base = mmio_range[i].first_page * PAGE_SIZE;
			addr_range[i].top = addr_range[i].base +
					mmio_range[i].number_of_pages * PAGE_SIZE;
		}
		num_gran += mmio_range[i].number_of_pages;
		INFO("addr_range[%u]: 0x%lx-0x%lx\n", i,
			addr_range[i].base, addr_range[i].top - 1UL);
	}

	/* Set MMIO range for Realm */
	host_shared_data_set_mmio_range(&realm, PRIMARY_PLANE_ID, 0U,
					range_count, &addr_range[0]);

	pcie_mem_enable(h_pdev->dev->bdf);

	map_addr = (u_register_t *)page_alloc(ALIGN(num_gran * sizeof(u_register_t) +
						PAGE_SIZE - 1UL, PAGE_SIZE));
	if (map_addr == NULL) {
		ERROR("Cannot allocate %lu bytes of memory\n",
			ALIGN(num_gran * sizeof(u_register_t) + PAGE_SIZE - 1UL,
			PAGE_SIZE));
		return_error = true;
		goto destroy_realm;
	}

	/* Delegate and map all device granules across all MMIO ranges */
	for (unsigned int i = 0U; i < range_count; i++) {
		u_register_t addr = addr_range[i].base;

		while (addr < addr_range[i].top) {
			res = host_rmi_granule_delegate(addr);
			if (res != RMI_SUCCESS) {
				ERROR("%s() for 0x%lx failed, %lu\n",
					"host_rmi_granule_delegate", addr, res);
				return_error = true;
				goto undelegate_granules;
			}
			++num_dlg[i];	/* number of granules delegated */

			res = host_dev_mem_map(&realm, h_vdev, addr, RTT_MAX_LEVEL,
					&map_addr[num_map]);
			if (res != REALM_SUCCESS) {
				ERROR("%s() for 0x%lx failed, %lu\n",
					"host_dev_mem_map", addr, res);
				return_error = true;
				goto unmap_memory;
			}

			++num_map;	/* number of granules mapped */
			addr += GRANULE_SIZE;
		}
	}

	/* Call Realm to do DA related RSI calls */
	if (!host_enter_realm_execute(&realm, REALM_SMMU, RMI_EXIT_VDEV_MAP, 0U)) {
		ERROR("Realm SMMUv3 test failed\n");
		return_error = true;
		goto destroy_realm;
	}

	run = (struct rmi_rec_run *)realm.run[0];
	exit_reason = run->exit.exit_reason;

	INFO("Enter device memory validation loop\n");

	/* Complete Realm's requests to validate mappings to device memory */
	while (out_top != run->exit.dev_mem_top) {
		/*
		 * Validate device memory mappings
		 */
		res = host_rmi_vdev_validate_mapping(realm.rd, realm.rec[0],
						(u_register_t)h_pdev,
						(u_register_t)h_vdev,
						run->exit.dev_mem_base,
						run->exit.dev_mem_top,
						&out_top);
		if (res != RMI_SUCCESS) {
			ERROR("%s() failed, %lu out_top=0x%lx\n",
				"host_rmi_vdev_validate_mapping", res, out_top);
				run->entry.flags = REC_ENTRY_FLAG_DEV_MEM_RESPONSE;
				return_error = true;
		}

		res = host_realm_rec_enter(&realm, &exit_reason, &host_call_result, 0U);
		if ((res != RMI_SUCCESS) || (exit_reason != RMI_EXIT_VDEV_MAP)) {
			break;
		}
	}

	INFO("Exit device memory validation loop\n");

	/* Check exit */
	if ((res != RMI_SUCCESS) || (exit_reason != RMI_EXIT_HOST_CALL) ||
	    (host_call_result != TEST_RESULT_SUCCESS)) {
		ERROR("%s() failed, %lu exit_reason=%lu host_call_result=%u\n",
			"host_realm_rec_enter", res, exit_reason, host_call_result);
		return_error = true;
	}

	rc = host_unassign_vdev_from_realm(&realm, h_vdev);
	if (rc != 0) {
		ERROR("Destroying VDEV failed\n");
		return_error = true;
		goto undelegate_granules;
	}

	/* Destroy PDEV */
	rc = tsm_disconnect_device(h_pdev);
	if (rc != 0) {
		ERROR("Destroying PDEV failed\n");
		return_error = true;
	}

unmap_memory:
	/* Unmap all device memory */
	for (unsigned int i = 0U; i < num_map; i++) {
		__unused u_register_t pa, top;

		res = host_rmi_vdev_unmap(realm.rd, map_addr[i],
					  RTT_MAX_LEVEL, &pa, &top);
		if (res != RMI_SUCCESS) {
			ERROR("%s() for 0x%lx failed, 0x%lx\n",
				"host_rmi_vdev_unmap", map_addr[i], res);
			return_error = true;
			goto undelegate_granules;
		}
	}

undelegate_granules:
	/* Undelegate device granules across all MMIO ranges */
	for (unsigned int i = 0U; i < range_count; i++) {
		u_register_t addr = addr_range[i].base;

		for (unsigned int j = 0U; j < num_dlg[i]; j++) {
			res = host_rmi_granule_undelegate(addr);
			if (res != RMI_SUCCESS) {
				ERROR("%s() for 0x%lx failed, 0x%lx\n",
					"host_rmi_granule_undelegate", addr, res);
				return_error = true;
				goto destroy_realm;
			}
			addr += GRANULE_SIZE;
		}
	}

destroy_realm:
	pcie_mem_disable(h_pdev->dev->bdf);

	/* Destroy the Realm */
	if (!host_destroy_realm(&realm)) {
		return_error = true;
	}

	if (return_error) {
		return TEST_RESULT_FAIL;
	}

	if (result == TEST_RESULT_SUCCESS) {
		return host_cmp_result();
	}

	return result;
}
