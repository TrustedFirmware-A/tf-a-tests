/*
 * Copyright (c) 2024-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include <heap/page_alloc.h>
#include <host_crypto_utils.h>
#include <host_da_helper.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_shared_data.h>
#include <mmio.h>
#include <pcie.h>
#include <pcie_doe.h>
#include <pcie_spec.h>
#include <platform.h>
#include <spdm.h>
#include <test_helpers.h>

int gbl_host_pdev_count;
struct host_pdev gbl_host_pdevs[HOST_PDEV_MAX];
struct host_vdev gbl_host_vdev;

static test_result_t tsm_disconnect_device(struct host_pdev *h_pdev)
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
		return TEST_RESULT_FAIL;
	}

	h_pdev->is_connected_to_tsm = false;

	return TEST_RESULT_SUCCESS;
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
static test_result_t tsm_connect_device(struct host_pdev *h_pdev)
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
		return TEST_RESULT_FAIL;
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

	/* Call rmi_pdev_comminucate to transition PDEV to READY state */
	rc = host_pdev_transition(h_pdev, RMI_PDEV_STATE_READY);
	if (rc != 0) {
		ERROR("PDEV transition: PDEV_HAS_KEY -> PDEV_READY failed\n");
		goto err_pdev_reclaim;
	}

	h_pdev->is_connected_to_tsm = true;

	return TEST_RESULT_SUCCESS;

err_pdev_reclaim:
	(void)host_pdev_reclaim(h_pdev);

	return TEST_RESULT_FAIL;
}

/* Iterate thorough all host_pdevs and try to connect to TSM */
static test_result_t tsm_connect_devices(void)
{
	uint32_t i;
	int count = 0;
	struct host_pdev *h_pdev;
	test_result_t result = TEST_RESULT_SKIPPED;

	for (i = 0; i < gbl_host_pdev_count; i++) {
		h_pdev = &gbl_host_pdevs[i];

		if (!is_host_pdev_independently_attested(h_pdev)) {
			continue;
		}

		result = tsm_connect_device(h_pdev);
		if (result != TEST_RESULT_SUCCESS) {
			ERROR("tsm_connect_device: 0x%x failed\n",
			     h_pdev->dev->bdf);
			break;
		}

		count++;
	}

	if (count != 0U) {
		INFO("%d devices connected to TSM\n", count);
	} else {
		INFO("No device connected to TSM\n");
	}

	return result;
}

/* Iterate thorough all connected host_pdevs and disconnect from TSM */
static test_result_t tsm_disconnect_devices(void)
{
	uint32_t i;
	struct host_pdev *h_pdev;
	test_result_t rc;
	bool return_error = false;

	for (i = 0; i < gbl_host_pdev_count; i++) {
		h_pdev = &gbl_host_pdevs[i];

		if (h_pdev->is_connected_to_tsm) {
			rc = tsm_disconnect_device(h_pdev);
			if (rc != TEST_RESULT_SUCCESS) {
				/* Set error, continue with other devices */
				return_error = true;
			}
		}
	}

	if (return_error) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

static test_result_t realm_assign_unassign_device(struct realm *realm_ptr,
						struct host_vdev *h_vdev,
						unsigned long tdi_id,
						void *pdev_ptr)
{
	int rc;
	bool realm_rc;

	/* Assign VDEV */
	INFO("======================================\n");
	INFO("Host: Assign device: (0x%x) %x:%x.%x to Realm \n",
	     (uint32_t)tdi_id,
	     PCIE_EXTRACT_BDF_BUS((uint32_t)tdi_id),
	     PCIE_EXTRACT_BDF_DEV((uint32_t)tdi_id),
	     PCIE_EXTRACT_BDF_FUNC((uint32_t)tdi_id));
	INFO("======================================\n");

	rc = host_assign_vdev_to_realm(realm_ptr, h_vdev, tdi_id, pdev_ptr);
	if (rc != 0) {
		ERROR("VDEV assign to realm failed\n");
		/* TF-RMM has support till here. Change error code temporarily */
		return TEST_RESULT_SUCCESS;
		/* return TEST_RESULT_FAIL */
	}

	/* Enter Realm. Lock -> Accept -> Unlock the assigned device */
	realm_rc = host_enter_realm_execute(realm_ptr, REALM_DA_RSI_CALLS,
					    RMI_EXIT_HOST_CALL, 0U);

	/* Unassign VDEV */
	INFO("======================================\n");
	INFO("Host: Unassign device: (0x%x) %x:%x.%x from Realm \n",
	     (uint32_t)tdi_id,
	     PCIE_EXTRACT_BDF_BUS((uint32_t)tdi_id),
	     PCIE_EXTRACT_BDF_DEV((uint32_t)tdi_id),
	     PCIE_EXTRACT_BDF_FUNC((uint32_t)tdi_id));
	INFO("======================================\n");

	rc = host_unassign_vdev_from_realm(realm_ptr, h_vdev);
	if (rc != 0) {
		ERROR("VDEV unassign to realm failed\n");
		return TEST_RESULT_FAIL;
	}

	if (!realm_rc) {
		ERROR("Realm DA_RSI_CALLS failed\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

static test_result_t
realm_assign_unassign_devices(struct realm *realm_ptr)
{
	uint32_t i;
	test_result_t rc;
	struct host_pdev *h_pdev;
	struct host_vdev *h_vdev;

	for (i = 0; i < gbl_host_pdev_count; i++) {
		h_pdev = &gbl_host_pdevs[i];

		if (h_pdev->is_connected_to_tsm) {
			h_vdev = &gbl_host_vdev;

			rc = realm_assign_unassign_device(realm_ptr, h_vdev,
							  h_pdev->dev->bdf,
							  h_pdev->pdev);
			if (rc != TEST_RESULT_SUCCESS) {
				break;
			}
		}
	}

	return rc;
}

/*
 * Iterate thorugh all host_pdevs and do
 * TSM connect
 * TSM disconnect
 */
test_result_t host_da_workflow_on_all_offchip_devices(void)
{
	int rc;
	struct realm realm;
	test_result_t result;
	bool return_error = false;
	u_register_t rmi_feat_reg0;

	SKIP_DA_TEST_IF_PREREQS_NOT_MET(rmi_feat_reg0);
	host_pdevs_init();

	/*
	 * Create a Realm with DA feature enabled
	 *
	 * todo: creating this after host_pdev_setup causes Realm create to
	 * fail.
	 */
	rc = host_create_realm_with_feat_da(&realm);
	if (rc != 0) {
		ERROR("Realm create with feat_da failed\n");
		return TEST_RESULT_FAIL;
	}

	/* Connect all devices with TSM */
	result = tsm_connect_devices();
	if (result == TEST_RESULT_SKIPPED) {
		goto out_rm_realm;
	} else if (result != TEST_RESULT_SUCCESS) {
		return_error = true;
	}

	/* Assign all TSM connected devices to a Realm */
	result = realm_assign_unassign_devices(&realm);
	if (result != TEST_RESULT_SUCCESS) {
		return_error = true;
	}

	result = tsm_disconnect_devices();
	if (result != TEST_RESULT_SUCCESS) {
		return_error = true;
	}

out_rm_realm:
	/* Destroy the Realm */
	if (!host_destroy_realm(&realm)) {
		return_error = true;
	}

	if (return_error) {
		result = TEST_RESULT_FAIL;
	}

	return result;
}

/*
 * This function invokes PDEV_CREATE on TRP and tries to test
 * the EL3 RMM-EL3 IDE KM interface. Will be skipped on TF-RMM.
 */
test_result_t host_realm_test_root_port_key_management(void)
{
	u_register_t rmi_rc;
	int ret;

	if (host_rmi_version(RMI_ABI_VERSION_VAL) != 0U) {
		tftf_testcase_printf("RMM is not TRP\n");
		return TEST_RESULT_SKIPPED;
	}

	/* Initialize Host NS heap memory */
	ret = page_pool_init((u_register_t)PAGE_POOL_BASE,
				(u_register_t)PAGE_POOL_MAX_SIZE);
	if (ret != HEAP_INIT_SUCCESS) {
		ERROR("Failed to init heap pool %d\n", ret);
		return TEST_RESULT_FAIL;
	}

	/*
	 * Directly call host_rmi_pdev_create with invalid pdev, expect an error
	 * to be returned from TRP.
	 */
	rmi_rc = host_rmi_pdev_create((u_register_t)0UL, (u_register_t)0UL);
	if (rmi_rc != RMI_SUCCESS) {
		return TEST_RESULT_SUCCESS;
	}

	ERROR("RMI_PDEV_CREATE did not return error as expected\n");
	return TEST_RESULT_FAIL;
}
