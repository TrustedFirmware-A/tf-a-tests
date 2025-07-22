/*
 * Copyright (c) 2024-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <heap/page_alloc.h>
#include <host_crypto_utils.h>
#include <host_da_flow_helper.h>
#include <host_da_helper.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <pcie_doe.h>
#include <platform.h>
#include <test_helpers.h>

/*
 * Iterate through all host_pdevs and do
 * TSM connect
 * TSM disconnect
 */
test_result_t host_da_workflow_on_all_offchip_devices(void)
{
	int rc;
	unsigned int count;
	struct realm realm;
	test_result_t result = TEST_RESULT_SUCCESS;
	bool return_error = false;
	u_register_t rmi_feat_reg0;

	INIT_AND_SKIP_DA_TEST_IF_PREREQS_NOT_MET(rmi_feat_reg0);

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
	rc = tsm_connect_devices(&count);
	if (rc != 0) {
		return_error = true;
	}

	/* If no devices are connected to TSM, then skip the test */
	if (count == 0U) {
		result = TEST_RESULT_SKIPPED;
		goto out_rm_realm;
	}

	/* Assign all TSM connected devices to a Realm */
	rc = realm_assign_unassign_devices(&realm);
	if (rc != 0) {
		return_error = true;
	}

	rc = tsm_disconnect_devices();
	if (rc != 0) {
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

/* Test all possible valid state transitions for PDEV */
test_result_t host_pdev_test_valid_state_transition(void)
{
	int rc = 0;
	struct host_pdev *h_pdev;
	unsigned int i;
	u_register_t rmi_feat_reg0;
	const unsigned char pdev_valid_state_transition[][PDEV_STATE_TRANSITION_MAX] = {
		{
			RMI_PDEV_STATE_NEW,
			RMI_PDEV_STATE_STOPPING,
			RMI_PDEV_STATE_STOPPED,
			-1
		},
		{
			RMI_PDEV_STATE_NEW,
			RMI_PDEV_STATE_NEEDS_KEY,
			RMI_PDEV_STATE_STOPPING,
			RMI_PDEV_STATE_STOPPED,
			-1
		},
		{
			RMI_PDEV_STATE_NEW,
			RMI_PDEV_STATE_NEEDS_KEY,
			RMI_PDEV_STATE_HAS_KEY,
			RMI_PDEV_STATE_STOPPING,
			RMI_PDEV_STATE_STOPPED,
			-1
		},
		{
			RMI_PDEV_STATE_NEW,
			RMI_PDEV_STATE_NEEDS_KEY,
			RMI_PDEV_STATE_HAS_KEY,
			RMI_PDEV_STATE_READY,
			RMI_PDEV_STATE_STOPPING,
			RMI_PDEV_STATE_STOPPED,
			-1
		}
	};

	INIT_AND_SKIP_DA_TEST_IF_PREREQS_NOT_MET(rmi_feat_reg0);

	h_pdev = get_host_pdev_by_type(DEV_TYPE_INDEPENDENTLY_ATTESTED);
	if (h_pdev == NULL) {
		return TEST_RESULT_SKIPPED;
	}

	/* Initialize Host NS heap memory */
	rc = page_pool_init((u_register_t)PAGE_POOL_BASE,
			     (u_register_t)PAGE_POOL_MAX_SIZE);
	if (rc != HEAP_INIT_SUCCESS) {
		ERROR("Failed to init heap pool %d\n", rc);
		return TEST_RESULT_FAIL;
	}

	for (i = 0U; i < sizeof(pdev_valid_state_transition)/
		     sizeof(pdev_valid_state_transition[0]); i++) {
		INFO("pdev_valid_state_transition sequence: %d\n", i);
		rc = host_pdev_state_transition(h_pdev,
					pdev_valid_state_transition[i],
					sizeof(pdev_valid_state_transition[i]));
		if (rc != 0) {
			ERROR("pdev_valid_state_transition failed at "
			      "sequence index: %d\n", i);
			break;
		}
	}

	return (rc == 0) ? TEST_RESULT_SUCCESS : TEST_RESULT_FAIL;
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
