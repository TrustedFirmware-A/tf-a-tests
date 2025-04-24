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

struct host_pdev gbl_host_pdev;
struct host_vdev gbl_host_vdev;

/*
 * This invokes various RMI calls related to PDEV, VDEV management that does
 * PDEV create/communicate/set_key/abort/stop/destroy and assigns the device
 * to a Realm using RMI VDEV ABIs
 *
 * 1. Create a Realm with DA feature enabled
 * 2. Find a known PCIe endpoint and connect with TSM to get_cert and establish
 *    secure session
 * 3. Assign the PCIe endpoint (a PF) to the Realm
 * 4. Call Realm to do DA related RSI calls
 * 5. Unassign the PCIe endpoint from the Realm
 * 6. Delete the Realm
 * 7. Reclaim the PCIe TDI from TSM
 */
test_result_t host_invoke_rmi_da_flow(void)
{
	u_register_t rmi_feat_reg0;
	uint32_t pdev_bdf, doe_cap_base;
	struct host_pdev *h_pdev;
	struct host_vdev *h_vdev;
	uint8_t public_key_algo;
	int ret, rc;
	bool realm_rc;
	struct realm realm;

	CHECK_DA_SUPPORT_IN_RMI(rmi_feat_reg0);
	SKIP_TEST_IF_DOE_NOT_SUPPORTED(pdev_bdf, doe_cap_base);

	INFO("DA on bdf: 0x%x, doe_cap_base: 0x%x\n", pdev_bdf, doe_cap_base);

	/* Initialize Host NS heap memory */
	ret = page_pool_init((u_register_t)PAGE_POOL_BASE,
				(u_register_t)PAGE_POOL_MAX_SIZE);
	if (ret != HEAP_INIT_SUCCESS) {
		ERROR("Failed to init heap pool %d\n", ret);
		return TEST_RESULT_FAIL;
	}

	/*
	 * 2. Create a Realm with DA feature enabled
	 *
	 * todo: creating this after host_pdev_setup cases Realm create to
	 * fail.
	 */
	rc = host_create_realm_with_feat_da(&realm);
	if (rc != 0) {
		INFO("Realm create with feat_da failed\n");
		return TEST_RESULT_FAIL;
	}

	INFO("Realm created with feat_da enabled\n");

	h_pdev = &gbl_host_pdev;
	h_vdev = &gbl_host_vdev;

	/* Allocate granules. Skip DA ABIs if host_pdev_setup fails */
	rc = host_pdev_setup(h_pdev);
	if (rc == -1) {
		INFO("host_pdev_setup failed. skipping DA ABIs...\n");
		return TEST_RESULT_SKIPPED;
	}

	/* todo: move to tdi_pdev_setup */
	h_pdev->bdf = pdev_bdf;
	h_pdev->doe_cap_base = doe_cap_base;

	/* Call rmi_pdev_create to transition PDEV to STATE_NEW */
	rc = host_pdev_transition(h_pdev, RMI_PDEV_STATE_NEW);
	if (rc != 0) {
		ERROR("PDEV transition: NULL -> STATE_NEW failed\n");
		return TEST_RESULT_FAIL;
	}

	/* Call rmi_pdev_communicate to transition PDEV to NEEDS_KEY */
	rc = host_pdev_transition(h_pdev, RMI_PDEV_STATE_NEEDS_KEY);
	if (rc != 0) {
		ERROR("PDEV transition: PDEV_NEW -> PDEV_NEEDS_KEY failed\n");
		return TEST_RESULT_FAIL;
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
		return TEST_RESULT_FAIL;
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
		INFO("PDEV transition: PDEV_NEEDS_KEY -> PDEV_HAS_KEY failed\n");
		return TEST_RESULT_FAIL;
	}

	/* Call rmi_pdev_comminucate to transition PDEV to READY state */
	rc = host_pdev_transition(h_pdev, RMI_PDEV_STATE_READY);
	if (rc != 0) {
		INFO("PDEV transition: PDEV_HAS_KEY -> PDEV_READY failed\n");
		return TEST_RESULT_FAIL;
	}


	/*
	 * 3 Assign VDEV (the PCIe endpoint) from the Realm
	 */
	rc = host_assign_vdev_to_realm(&realm, h_pdev, h_vdev);
	if (rc != 0) {
		INFO("VDEV assign to realm failed\n");
		return TEST_RESULT_FAIL;
	}

	/*
	 * 4 Call Realm to do DA related RSI calls
	 */
	realm_rc = host_enter_realm_execute(&realm, REALM_DA_RSI_CALLS,
					    RMI_EXIT_HOST_CALL, 0U);
	if (!realm_rc) {
		INFO("Realm DA_RSI_CALLS failed\n");
		return TEST_RESULT_FAIL;
	}

	/*
	 * 5 Unassign VDEV (the PCIe endpoint) from the Realm
	 */
	rc = host_unassign_vdev_from_realm(&realm, h_pdev, h_vdev);
	if (rc != 0) {
		INFO("VDEV unassign to realm failed\n");
		return TEST_RESULT_FAIL;
	}

	/*
	 * 6 Destroy the Realm
	 */
	if (!host_destroy_realm(&realm)) {
		INFO("Realm destroy failed\n");
		return TEST_RESULT_FAIL;
	}

	/*
	 * 7 Reclaim PDEV (the PCIe TDI) from TSM
	 */
	rc = host_pdev_reclaim(h_pdev);
	if (rc != 0) {
		INFO("Reclaim PDEV from TSM failed\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * This function invokes PDEV_CREATE on TRP and tries to test
 * the EL3 RMM-EL3 IDE KM interface. Will be skipped on TF-RMM.
 */
test_result_t host_realm_test_root_port_key_management(void)
{
	struct host_pdev *h_pdev;
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

	h_pdev = &gbl_host_pdev;

	/*
	 * Call rmi_pdev_create with invalid pdev, expect an error
	 * to be returned from TRP.
	 */
	ret = host_pdev_create(h_pdev);
	if (ret != 0) {
		return TEST_RESULT_SUCCESS;
	}

	ERROR("RMI_PDEV_CREATE did not return error as expected\n");
	return TEST_RESULT_FAIL;
}
