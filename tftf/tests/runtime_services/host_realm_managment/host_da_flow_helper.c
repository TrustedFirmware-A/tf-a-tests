/*
 * Copyright (c) 2024-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <host_crypto_utils.h>
#include <host_da_helper.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_shared_data.h>
#include <pcie.h>
#include <pcie_doe.h>
#include <platform.h>
#include <test_helpers.h>

extern unsigned int gbl_host_pdev_count;
extern struct host_pdev gbl_host_pdevs[HOST_PDEV_MAX];
extern struct host_vdev gbl_host_vdev;

int tsm_disconnect_device(struct host_pdev *h_pdev)
{
	int rc;

	assert(h_pdev->is_connected_to_tsm);

	INFO("===========================================\n");
	INFO("Host: TSM disconnect device: (0x%x) %x:%x.%x\n",
	     h_pdev->dev->bdf,
	     PCIE_EXTRACT_BDF_BUS(h_pdev->dev->bdf),
	     PCIE_EXTRACT_BDF_DEV(h_pdev->dev->bdf),
	     PCIE_EXTRACT_BDF_FUNC(h_pdev->dev->bdf));
	INFO("===========================================\n");

	rc = host_pdev_reclaim(h_pdev);
	if (rc != 0) {
		return -1;
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
static int tsm_connect_device(struct host_pdev *h_pdev)
{
	int rc;
	uint8_t public_key_algo;

	INFO("======================================\n");
	INFO("Host: TSM connect device: (0x%x) %x:%x.%x\n",
	     h_pdev->dev->bdf,
	     PCIE_EXTRACT_BDF_BUS(h_pdev->dev->bdf),
	     PCIE_EXTRACT_BDF_DEV(h_pdev->dev->bdf),
	     PCIE_EXTRACT_BDF_FUNC(h_pdev->dev->bdf));
	INFO("======================================\n");

	/* Allocate granules. Skip DA ABIs if host_pdev_setup fails */
	rc = host_pdev_setup(h_pdev);
	if (rc == -1) {
		ERROR("host_pdev_setup failed.\n");
		return -1;
	}

	/* Call rmi_pdev_create to transition PDEV to STATE_NEW */
	rc = host_pdev_transition(h_pdev, RMI_PDEV_STATE_NEW);
	if (rc != 0) {
		ERROR("PDEV transition: NULL -> STATE_NEW failed\n");
		goto err_pdev_reclaim;
	}

	/* Call rmi_pdev_communicate to transition PDEV to NEEDS_KEY */
	rc = host_pdev_transition(h_pdev, RMI_PDEV_STATE_NEEDS_KEY);
	if (rc != 0) {
		ERROR("PDEV transition: PDEV_NEW -> PDEV_NEEDS_KEY failed\n");
		goto err_pdev_reclaim;
	}

	/* Get public key. Verifying cert_chain not done by host but by Realm? */
	rc = host_get_public_key_from_cert_chain(h_pdev->cert_chain,
						 h_pdev->cert_chain_len,
						 h_pdev->public_key,
						 &h_pdev->public_key_len,
						 h_pdev->public_key_metadata,
						 &h_pdev->public_key_metadata_len,
						 &public_key_algo);
	if (rc != 0) {
		ERROR("Get public key failed\n");
		goto err_pdev_reclaim;
	}

	if (public_key_algo == PUBLIC_KEY_ALGO_ECDSA_ECC_NIST_P256) {
		h_pdev->public_key_sig_algo = RMI_SIGNATURE_ALGORITHM_ECDSA_P256;
	} else if (public_key_algo == PUBLIC_KEY_ALGO_ECDSA_ECC_NIST_P384) {
		h_pdev->public_key_sig_algo = RMI_SIGNATURE_ALGORITHM_ECDSA_P384;
	} else {
		h_pdev->public_key_sig_algo = RMI_SIGNATURE_ALGORITHM_RSASSA_3072;
	}
	INFO("DEV public key len/sig_algo: %ld/%d\n", h_pdev->public_key_len,
	     h_pdev->public_key_sig_algo);

	/* Call rmi_pdev_set_key transition PDEV to HAS_KEY */
	rc = host_pdev_transition(h_pdev, RMI_PDEV_STATE_HAS_KEY);
	if (rc != 0) {
		ERROR("PDEV transition: PDEV_NEEDS_KEY -> PDEV_HAS_KEY failed\n");
		goto err_pdev_reclaim;
	}

	/* Call rmi_pdev_communicate to transition PDEV to READY state */
	rc = host_pdev_transition(h_pdev, RMI_PDEV_STATE_READY);
	if (rc != 0) {
		ERROR("PDEV transition: PDEV_HAS_KEY -> PDEV_READY failed\n");
		goto err_pdev_reclaim;
	}

	h_pdev->is_connected_to_tsm = true;

	return 0;

err_pdev_reclaim:
	(void)host_pdev_reclaim(h_pdev);

	return -1;
}

/* Get the first pdev from host_pdevs and try to connect to TSM */
int tsm_connect_first_device(struct host_pdev **h_pdev)
{
	int result = -1;

	*h_pdev = &gbl_host_pdevs[0];

	if (!is_host_pdev_independently_attested(*h_pdev)) {
		ERROR("%s: 1st dev is not independently attested\n", __func__);
	}

	result = tsm_connect_device(*h_pdev);
	if (result != 0) {
		ERROR("tsm_connect_device: 0x%x failed\n", (*h_pdev)->dev->bdf);
	}

	return result;
}

/* Iterate thorough all host_pdevs and try to connect to TSM */
int tsm_connect_devices(unsigned int *count)
{
	int rc = 0;
	unsigned int i;
	unsigned int count_ret = 0U;
	struct host_pdev *h_pdev;

	for (i = 0U; i < gbl_host_pdev_count; i++) {
		h_pdev = &gbl_host_pdevs[i];

		if (!is_host_pdev_independently_attested(h_pdev)) {
			continue;
		}

		rc = tsm_connect_device(h_pdev);
		if (rc != 0) {
			ERROR("tsm_connect_device: 0x%x failed\n",
			      h_pdev->dev->bdf);
			rc = -1;
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
	uint32_t i;
	struct host_pdev *h_pdev;
	int rc;
	bool return_error = false;

	for (i = 0; i < gbl_host_pdev_count; i++) {
		h_pdev = &gbl_host_pdevs[i];

		if (h_pdev->is_connected_to_tsm) {
			rc = tsm_disconnect_device(h_pdev);
			if (rc != 0) {
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

static int realm_assign_unassign_device(struct realm *realm_ptr,
						struct host_vdev *h_vdev,
						unsigned long tdi_id,
						void *pdev_ptr)
{
	int rc;
	bool realm_rc;

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
		return -1;
	}

	/* execute communicate until vdev reaches VDEV_UNLOCKED state */
	rc = host_vdev_transition(realm_ptr, h_vdev, RMI_VDEV_STATE_UNLOCKED);
	if (rc != 0) {
		ERROR("Transitioning to RMI_VDEV_STATE_UNLOCKED state failed\n");
		return -1;
	}

	rc = host_vdev_get_measurements(realm_ptr, h_vdev, RMI_VDEV_STATE_UNLOCKED);
	if (rc != 0) {
		ERROR("Getting measurements failed\n");
		return -1;
	}
	/* execute communicate until vdev reaches VDEV_LOCKED state */
	rc = host_vdev_transition(realm_ptr, h_vdev, RMI_VDEV_STATE_LOCKED);
	if (rc != 0) {
		ERROR("Transitioning to RMI_VDEV_STATE_LOCKED state failed\n");
		return -1;
	}

	rc = host_vdev_get_measurements(realm_ptr, h_vdev, RMI_VDEV_STATE_LOCKED);
	if (rc != 0) {
		ERROR("Getting measurements failed\n");
		return -1;
	}

	rc = host_vdev_get_interface_report(realm_ptr, h_vdev, RMI_VDEV_STATE_LOCKED);
	if (rc != 0) {
		ERROR("Getting if report failed\n");
		return -1;
	}
	/* execute communicate until vdev reaches VDEV_LOCKED state */
	INFO("Start transitioning to RMI_VDEV_STATE_STARTED state\n");
	rc = host_vdev_transition(realm_ptr, h_vdev, RMI_VDEV_STATE_STARTED);
	if (rc != 0) {
		ERROR("Transitioning to RMI_VDEV_STATE_STARTED state failed\n");
		return -1;
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
		return -1;
	}

	if (!realm_rc) {
		ERROR("Realm DA_RSI_CALLS failed\n");
		return -1;
	}

	return 0;
}

int realm_assign_unassign_devices(struct realm *realm_ptr)
{
	uint32_t i;
	int rc = 0;
	struct host_pdev *h_pdev;
	struct host_vdev *h_vdev;

	for (i = 0; i < gbl_host_pdev_count; i++) {
		h_pdev = &gbl_host_pdevs[i];

		if (h_pdev->is_connected_to_tsm) {
			h_vdev = &gbl_host_vdev;

			rc = realm_assign_unassign_device(realm_ptr, h_vdev,
							  h_pdev->dev->bdf,
							  h_pdev->pdev);
			if (rc != 0) {
				break;
			}
		}
	}

	return rc;
}
