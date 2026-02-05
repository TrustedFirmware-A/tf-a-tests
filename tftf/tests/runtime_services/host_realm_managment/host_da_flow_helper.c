/*
 * Copyright (c) 2024-2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <string.h>

#include <host_da_helper.h>
#include <host_realm_helper.h>
#include <host_shared_data.h>
#include <pcie.h>

int tsm_disconnect_device(struct host_pdev *h_pdev)
{
	int rc;
	const unsigned char tsm_disconnect_flow[] = {
		RMI_PDEV_STATE_STOPPING,
		RMI_PDEV_STATE_STOPPED
	};

	assert(h_pdev);
	assert(h_pdev->dev);
	assert(h_pdev->is_connected_to_tsm);

	INFO("===========================================\n");
	INFO("Host: TSM disconnect device: (0x%x) %x:%x.%x\n",
	     h_pdev->dev->bdf,
	     PCIE_EXTRACT_BDF_BUS(h_pdev->dev->bdf),
	     PCIE_EXTRACT_BDF_DEV(h_pdev->dev->bdf),
	     PCIE_EXTRACT_BDF_FUNC(h_pdev->dev->bdf));
	INFO("===========================================\n");

	rc = host_pdev_state_transition(h_pdev, tsm_disconnect_flow,
					sizeof(tsm_disconnect_flow));
	if (rc != 0) {
		ERROR("PDEV TSM disconnect state transitions: failed\n");
		return rc;
	}

	h_pdev->is_connected_to_tsm = false;

	return 0;
}

/*
 * This invokes various RMI calls related to PDEV, VDEV management that does
 * PDEV create/communicate/set_key/abort/stop/destroy and assigns the device
 * to a Realm using RMI VDEV ABIs
 *
 * 1. Create a Realm with DA feature enabled
 * 2. Find a known PCIe endpoint and connect with TSM to get_cert and establish
 *    secure session
 */
int tsm_connect_device(struct host_pdev *h_pdev)
{
	int rc;
	const unsigned char tsm_connect_flow[] = {
		RMI_PDEV_STATE_NEW,
		RMI_PDEV_STATE_NEEDS_KEY,
		RMI_PDEV_STATE_HAS_KEY,
		RMI_PDEV_STATE_READY
	};

	assert(h_pdev);
	assert(h_pdev->dev);
	assert(!h_pdev->is_connected_to_tsm);

	/* Get device memory regions */
	host_get_addr_range(h_pdev);

	INFO("======================================\n");
	INFO("Host: TSM connect device: (0x%x) %x:%x.%x\n",
	     h_pdev->dev->bdf,
	     PCIE_EXTRACT_BDF_BUS(h_pdev->dev->bdf),
	     PCIE_EXTRACT_BDF_DEV(h_pdev->dev->bdf),
	     PCIE_EXTRACT_BDF_FUNC(h_pdev->dev->bdf));

	for (unsigned int i = 0U; i < h_pdev->ncoh_num_addr_range; i++) {
		INFO("BAR[%u]: 0x%lx-0x%lx\n", i,
			h_pdev->ncoh_addr_range[i].base,
			h_pdev->ncoh_addr_range[i].top - 1UL);
	}

	INFO("======================================\n");

	rc = host_pdev_state_transition(h_pdev, tsm_connect_flow,
					sizeof(tsm_connect_flow));
	if (rc != 0) {
		ERROR("PDEV TSM connect state transitions: failed\n");
		return rc;
	}

	h_pdev->is_connected_to_tsm = true;

	return 0;
}

/* Get the first pdev from host_pdevs and try to connect to TSM */
int tsm_connect_first_device(struct host_pdev **h_pdev)
{
	int rc;

	*h_pdev = &gbl_host_pdevs[0];

	if (!is_host_pdev_independently_attested(*h_pdev)) {
		ERROR("%s: 1st dev is not independently attested\n", __func__);
	}

	rc = tsm_connect_device(*h_pdev);
	if (rc != 0) {
		ERROR("tsm_connect_device: 0x%x failed\n", (*h_pdev)->dev->bdf);
	}

	return rc;
}

/* Iterate thorough all host_pdevs and try to connect to TSM */
int tsm_connect_devices(unsigned int *count)
{
	int rc = 0;
	unsigned int count_ret = 0U;

	for (unsigned int i = 0U; i < gbl_host_pdev_count; i++) {
		struct host_pdev *h_pdev = &gbl_host_pdevs[i];

		if (!is_host_pdev_independently_attested(h_pdev)) {
			continue;
		}

		rc = tsm_connect_device(h_pdev);
		if (rc != 0) {
			ERROR("tsm_connect_device: 0x%x failed\n",
			      h_pdev->dev->bdf);
			break;
		}

		count_ret++;
	}

	INFO("%u devices connected to TSM\n", count_ret);
	*count = count_ret;

	return rc;
}

/* Iterate thorough all connected host_pdevs and disconnect from TSM */
int tsm_disconnect_devices(void)
{
	bool return_error = false;

	for (unsigned int i = 0U; i < gbl_host_pdev_count; i++) {
		struct host_pdev *h_pdev = &gbl_host_pdevs[i];

		if (h_pdev->is_connected_to_tsm) {
			if (tsm_disconnect_device(h_pdev) != 0) {
				/* Set error, continue with other devices */
				return_error = true;
			}
		}
	}

	if (return_error) {
		return -1;
	}

	return 0;
}

int realm_assign_device(struct realm *realm_ptr,
			struct host_vdev *h_vdev,
			unsigned long tdi_id,
			void *pdev_ptr)
{
	int rc;

	/* Assign VDEV */
	INFO("======================================\n");
	INFO("Host: Assign device: (0x%x) %x:%x.%x to Realm\n",
	     (uint32_t)tdi_id,
	     PCIE_EXTRACT_BDF_BUS((uint32_t)tdi_id),
	     PCIE_EXTRACT_BDF_DEV((uint32_t)tdi_id),
	     PCIE_EXTRACT_BDF_FUNC((uint32_t)tdi_id));
	INFO("======================================\n");

	rc = host_assign_vdev_to_realm(realm_ptr, h_vdev, tdi_id, pdev_ptr);
	if (rc != 0) {
		ERROR("VDEV assign to realm failed\n");
		return rc;
	}

	/* Execute communicate until vdev reaches VDEV_UNLOCKED state */
	rc = host_vdev_transition(realm_ptr, h_vdev, RMI_VDEV_STATE_UNLOCKED);
	if (rc != 0) {
		ERROR("Transitioning to RMI_VDEV_STATE_UNLOCKED state failed\n");
		return rc;
	}

	rc = host_vdev_get_measurements(realm_ptr, h_vdev, RMI_VDEV_STATE_UNLOCKED);
	if (rc != 0) {
		ERROR("Getting measurements failed\n");
		return rc;
	}
	/* Execute communicate until vdev reaches VDEV_LOCKED state */
	rc = host_vdev_transition(realm_ptr, h_vdev, RMI_VDEV_STATE_LOCKED);
	if (rc != 0) {
		ERROR("Transitioning to RMI_VDEV_STATE_LOCKED state failed\n");
		return rc;
	}

	rc = host_vdev_get_measurements(realm_ptr, h_vdev, RMI_VDEV_STATE_LOCKED);
	if (rc != 0) {
		ERROR("Getting measurements failed\n");
		return rc;
	}

	rc = host_vdev_get_interface_report(realm_ptr, h_vdev, RMI_VDEV_STATE_LOCKED);
	if (rc != 0) {
		ERROR("Getting interface report failed\n");
		return rc;
	}
	/* Execute communicate until vdev reaches VDEV_LOCKED state */
	INFO("Start transitioning to RMI_VDEV_STATE_STARTED state\n");
	rc = host_vdev_transition(realm_ptr, h_vdev, RMI_VDEV_STATE_STARTED);
	if (rc != 0) {
		ERROR("Transitioning to RMI_VDEV_STATE_STARTED state failed\n");
	}

	return rc;
}

static int realm_assign_unassign_device(struct realm *realm_ptr,
						struct host_vdev *h_vdev,
						unsigned long tdi_id,
						void *pdev_ptr)
{
	bool realm_rc;
	int rc;

	rc = realm_assign_device(realm_ptr, h_vdev, tdi_id, pdev_ptr);
	if (rc != 0) {
		return rc;
	}

	/* Enter Realm. Execute realm DA commands */
	realm_rc = host_enter_realm_execute(realm_ptr, REALM_DA_RSI_CALLS,
					    RMI_EXIT_HOST_CALL, 0U);

	/* Unassign VDEV */
	INFO("======================================\n");
	INFO("Host: Unassign device: (0x%x) %x:%x.%x from Realm\n",
	     (uint32_t)tdi_id,
	     PCIE_EXTRACT_BDF_BUS((uint32_t)tdi_id),
	     PCIE_EXTRACT_BDF_DEV((uint32_t)tdi_id),
	     PCIE_EXTRACT_BDF_FUNC((uint32_t)tdi_id));
	INFO("======================================\n");

	rc = host_unassign_vdev_from_realm(realm_ptr, h_vdev);
	if (rc != 0) {
		ERROR("VDEV unassign to realm failed\n");
		return rc;
	}

	if (!realm_rc) {
		ERROR("Realm DA_RSI_CALLS failed\n");
		return -1;
	}

	return 0;
}

int realm_assign_unassign_devices(struct realm *realm_ptr)
{
	for (unsigned int i = 0U; i < gbl_host_pdev_count; i++) {
		struct host_pdev *h_pdev = &gbl_host_pdevs[i];

		if (h_pdev->is_connected_to_tsm) {
			struct host_vdev *h_vdev = &gbl_host_vdev;

			int rc = realm_assign_unassign_device(realm_ptr, h_vdev,
							      h_pdev->dev->bdf,
							      h_pdev->pdev);
			if (rc != 0) {
				return rc;
			}
		}
	}

	return 0;
}
