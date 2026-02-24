/*
 * Copyright (c) 2024-2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <heap/page_alloc.h>
#include <host_crypto_utils.h>
#include <host_da_flow_helper.h>
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

static int host_vdev_expect_state(struct host_vdev *h_vdev,
				  unsigned char exp_state)
{
	u_register_t state;
	u_register_t ret;

	ret = host_rmi_vdev_get_state((u_register_t)h_vdev->vdev_ptr, &state);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_vdev_get_state failed: 0x%lx\n", ret);
		return -1;
	}

	if (state != exp_state) {
		ERROR("VDEV state mismatch: got %lu expected %u\n",
		      state, (unsigned int)exp_state);
		return -1;
	}

	return 0;
}

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
	/* Deactivate PSMMU and destroy the Realm */
	if (!destroy_psmmu_realm(&realm)) {
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
 * Test valid VDEV state transitions per the "Virtual device lifecycle"
 * flow in the spec.
 */
test_result_t host_vdev_test_valid_state_transition(void)
{
	int rc = 0;
	u_register_t rmi_feat_reg0;
	u_register_t ret;
	bool return_error = false;
	bool realm_created = false;
	bool tsm_connected = false;
	bool vdev_assigned = false;
	struct realm realm;
	struct host_pdev *h_pdev;
	struct host_vdev *h_vdev = &gbl_host_vdev;

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

	rc = host_create_realm_with_feat_da(&realm);
	if (rc != 0) {
		ERROR("Realm create with feat_da failed\n");
		return TEST_RESULT_FAIL;
	}
	realm_created = true;

	rc = tsm_connect_device(h_pdev);
	if (rc != 0) {
		ERROR("TSM connect failed for device 0x%x\n", h_pdev->dev->bdf);
		return_error = true;
		goto out_rm_realm;
	}
	tsm_connected = true;

	rc = host_assign_vdev_to_realm(&realm, h_vdev,
				       h_pdev->dev->bdf, h_pdev->pdev);
	if (rc != 0) {
		ERROR("VDEV assign to realm failed\n");
		return_error = true;
		goto out_tsm_disconnect;
	}
	vdev_assigned = true;

	/* VDEV must start in NEW state after create */
	if (host_vdev_expect_state(h_vdev, RMI_VDEV_STATE_NEW) != 0) {
		return_error = true;
		goto out_unassign_vdev;
	}

	/* RMI_VDEV_LOCK from NEW must fail */
	INFO("VDEV states: lock from NEW must fail\n");
	ret = host_rmi_vdev_lock(realm.rd, (u_register_t)h_pdev->pdev,
				 (u_register_t)h_vdev->vdev_ptr);
	if (ret != RMI_ERROR_DEVICE) {
		ERROR("VDEV lock from NEW returned 0x%lx\n", ret);
		return_error = true;
		goto out_unassign_vdev;
	}

	INFO("VDEV states: verify still NEW after failed lock\n");
	if (host_vdev_expect_state(h_vdev, RMI_VDEV_STATE_NEW) != 0) {
		return_error = true;
		goto out_unassign_vdev;
	}

	/* NEW -> UNLOCKED via RMI_VDEV_COMMUNICATE */
	INFO("VDEV states: NEW -> UNLOCKED\n");
	rc = host_vdev_transition(&realm, h_vdev, RMI_VDEV_STATE_UNLOCKED);
	if (rc != 0) {
		ERROR("Transition to UNLOCKED failed\n");
		return_error = true;
		goto out_unassign_vdev;
	}

	if (host_vdev_expect_state(h_vdev, RMI_VDEV_STATE_UNLOCKED) != 0) {
		return_error = true;
		goto out_unassign_vdev;
	}

	/* RMI_VDEV_START/UNLOCK from UNLOCKED must fail */
	INFO("VDEV states: start from UNLOCKED must fail\n");
	ret = host_rmi_vdev_start(realm.rd, (u_register_t)h_pdev->pdev,
				  (u_register_t)h_vdev->vdev_ptr);
	if (ret != RMI_ERROR_DEVICE) {
		ERROR("VDEV start from UNLOCKED returned 0x%lx\n", ret);
		return_error = true;
		goto out_unassign_vdev;
	}

	INFO("VDEV states: unlock from UNLOCKED must fail\n");
	ret = host_rmi_vdev_unlock(realm.rd, (u_register_t)h_pdev->pdev,
				   (u_register_t)h_vdev->vdev_ptr);
	if (ret != RMI_ERROR_DEVICE) {
		ERROR("VDEV unlock from UNLOCKED returned 0x%lx\n", ret);
		return_error = true;
		goto out_unassign_vdev;
	}

	/* UNLOCKED -> LOCKED via RMI_VDEV_LOCK + COMMUNICATE */
	INFO("VDEV states: UNLOCKED -> LOCKED\n");
	rc = host_vdev_transition(&realm, h_vdev, RMI_VDEV_STATE_LOCKED);
	if (rc != 0) {
		ERROR("Transition to LOCKED failed\n");
		return_error = true;
		goto out_unassign_vdev;
	}

	if (host_vdev_expect_state(h_vdev, RMI_VDEV_STATE_LOCKED) != 0) {
		return_error = true;
		goto out_unassign_vdev;
	}

	/* LOCKED -> UNLOCKED via RMI_VDEV_UNLOCK + COMMUNICATE */
	INFO("VDEV states: LOCKED -> UNLOCKED\n");
	rc = host_vdev_unlock(&realm, h_vdev, RMI_VDEV_STATE_UNLOCKED);
	if (rc != 0) {
		ERROR("Unlock to UNLOCKED failed\n");
		return_error = true;
		goto out_unassign_vdev;
	}

	if (host_vdev_expect_state(h_vdev, RMI_VDEV_STATE_UNLOCKED) != 0) {
		return_error = true;
		goto out_unassign_vdev;
	}

	/* UNLOCKED -> LOCKED -> STARTED */
	INFO("VDEV states: UNLOCKED -> LOCKED\n");
	rc = host_vdev_transition(&realm, h_vdev, RMI_VDEV_STATE_LOCKED);
	if (rc != 0) {
		ERROR("Transition to LOCKED failed\n");
		return_error = true;
		goto out_unassign_vdev;
	}

	INFO("VDEV states: LOCKED -> STARTED\n");
	rc = host_vdev_transition(&realm, h_vdev, RMI_VDEV_STATE_STARTED);
	if (rc != 0) {
		ERROR("Transition to STARTED failed\n");
		return_error = true;
		goto out_unassign_vdev;
	}

	if (host_vdev_expect_state(h_vdev, RMI_VDEV_STATE_STARTED) != 0) {
		return_error = true;
		goto out_unassign_vdev;
	}

	/* STARTED -> UNLOCKED via RMI_VDEV_UNLOCK + COMMUNICATE */
	INFO("VDEV states: STARTED -> UNLOCKED\n");
	rc = host_vdev_unlock(&realm, h_vdev, RMI_VDEV_STATE_UNLOCKED);
	if (rc != 0) {
		ERROR("Unlock to UNLOCKED failed\n");
		return_error = true;
		goto out_unassign_vdev;
	}

	if (host_vdev_expect_state(h_vdev, RMI_VDEV_STATE_UNLOCKED) != 0) {
		return_error = true;
	}

out_unassign_vdev:
	if (vdev_assigned) {
		rc = host_unassign_vdev_from_realm(&realm, h_vdev);
		if (rc != 0) {
			ERROR("VDEV unassign failed\n");
			return_error = true;
		}
	}

out_tsm_disconnect:
	if (tsm_connected) {
		rc = tsm_disconnect_device(h_pdev);
		if (rc != 0) {
			ERROR("TSM disconnect failed for device 0x%x\n",
			      h_pdev->dev->bdf);
			return_error = true;
		}
	}

out_rm_realm:
	if (realm_created) {
		if (!host_destroy_realm(&realm)) {
			return_error = true;
		}
	}

	return return_error ? TEST_RESULT_FAIL : TEST_RESULT_SUCCESS;
}

/* Invoke IDE key refresh and IDE reset on PDEV once it is in READY state */
test_result_t host_pdev_invoke_ide_refresh_reset(void)
{
	int rc = 0;
	struct host_pdev *h_pdev;
	u_register_t rmi_feat_reg0;
	bool return_error = false;
	const unsigned char pdev_ide_refresh_reset[] = {
		RMI_PDEV_STATE_COMMUNICATING,
		RMI_PDEV_STATE_READY,
		RMI_PDEV_STATE_IDE_RESETTING,
		RMI_PDEV_STATE_READY
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

	rc = tsm_connect_device(h_pdev);
	if (rc != 0) {
		ERROR("TSM connect failed for device 0x%x\n", h_pdev->dev->bdf);
		return TEST_RESULT_FAIL;
	}

	INFO("Invoke pdev_ide_refresh_reset sequence\n");
	rc = host_pdev_state_transition(h_pdev, pdev_ide_refresh_reset,
					sizeof(pdev_ide_refresh_reset));
	if (rc != 0) {
		ERROR("pdev_ide_refresh_reset failed\n");
		return_error = true;
	}

	/* Disconnect the device from TSM */
	rc = tsm_disconnect_device(h_pdev);
	if (rc != 0) {
		ERROR("TSM disconnect failed for device 0x%x\n",
		      h_pdev->dev->bdf);
		return_error = true;
	}

	return return_error ? TEST_RESULT_FAIL : TEST_RESULT_SUCCESS;
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
