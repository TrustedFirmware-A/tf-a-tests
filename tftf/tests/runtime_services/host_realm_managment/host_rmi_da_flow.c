/*
 * Copyright (c) 2024-2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <host_crypto_utils.h>
#include <host_da_flow_helper.h>
#include <host_da_helper.h>
#include <heap/page_alloc.h>
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

#define HOST_RSI_SUCCESS		0UL
#define HOST_RSI_ERROR_INPUT		1UL

static bool host_da_vdev_get_info(struct realm *realm_ptr,
				       u_register_t vdev_id,
				       u_register_t info_ipa,
				       u_register_t expected_rc)
{
	host_shared_data_t *shared;
	u_register_t realm_rc;
	bool host_call_ok;

	shared = host_get_shared_structure(realm_ptr, PRIMARY_PLANE_ID, 0U);
	shared->realm_out_val[HOST_ARG1_INDEX] = ~0UL;

	host_shared_data_set_host_val(realm_ptr, PRIMARY_PLANE_ID, 0U,
				      HOST_ARG1_INDEX, vdev_id);
	host_shared_data_set_host_val(realm_ptr, PRIMARY_PLANE_ID, 0U,
				      HOST_ARG2_INDEX, info_ipa);

	host_call_ok = host_enter_realm_execute(realm_ptr,
						REALM_DA_RSI_VDEV_GET_INFO,
						RMI_EXIT_HOST_CALL, 0U);
	realm_rc = host_shared_data_get_realm_val(realm_ptr, PRIMARY_PLANE_ID, 0U,
						  HOST_ARG1_INDEX);

	if (realm_rc != expected_rc) {
		ERROR("RSI_VDEV_GET_INFO(vdev_id=0x%lx, addr=0x%lx) rc=0x%lx expected=0x%lx\n",
		      vdev_id, info_ipa, realm_rc, expected_rc);
		return false;
	}

	if ((expected_rc == HOST_RSI_SUCCESS) != host_call_ok) {
		ERROR("RSI_VDEV_GET_INFO(vdev_id=0x%lx, addr=0x%lx) host_call_ok=%u expected_rc=0x%lx\n",
		      vdev_id, info_ipa, host_call_ok, expected_rc);
		return false;
	}

	return true;
}

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
	rc = host_create_realm_with_feat_da(&realm, true);
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

/*
 * Trigger DA RSI commands for a device.
 *
 * Call RSI_VDEV_GET_INFO in various scenarios:
 * Test | vdev_id | IPA     |
 * -------------------------|
 *  1   | valid   | valid   |
 *  2   | valid   | invalid |
 *  3   | invalid | valid   |
 *
 *  vdev_id: valid iff vdev is assigned to the realm with vdev_id.
 *  IPA: valid if RIPAS state is not EMPTY.
 */
test_result_t host_da_get_info_parameter_test(void)
{
	test_result_t result = TEST_RESULT_SUCCESS;
	bool ret2, rc, return_error = false;
	u_register_t ret, valid_ipa, empty_ipa;
	u_register_t rmi_feat_reg0;
	struct realm realm;
	struct rtt_entry rtt;
	struct host_pdev *h_pdev = NULL;
	struct host_vdev *h_vdev = &gbl_host_vdev;

	INIT_AND_SKIP_DA_TEST_IF_PREREQS_NOT_MET(rmi_feat_reg0);

	/* Create Realm with DA, defer activation */
	ret = host_create_realm_with_feat_da(&realm, false);
	if (ret != 0) {
		ERROR("Realm create with feat_da failed\n");
		return TEST_RESULT_FAIL;
	}

	/* Create 2 IPAs */
	valid_ipa = (u_register_t)page_alloc(PAGE_SIZE);
	empty_ipa = (u_register_t)page_alloc(PAGE_SIZE);

	ret = host_rmi_create_rtt_levels(&realm, valid_ipa, 3L, 3L);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_create_rtt_levels failed\n");
		goto destroy_realm;
	}
	ret = host_rmi_rtt_readentry(realm.rd, valid_ipa, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_UNASSIGNED ||
			(rtt.ripas != RMI_EMPTY) || rtt.walk_level != 3L) {
		ERROR("wrong initial state\n");
		goto destroy_realm;
	}

	ret = host_rmi_create_rtt_levels(&realm, empty_ipa, 3L, 3L);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_create_rtt_levels failed\n");
		goto destroy_realm;
	}
	ret = host_rmi_rtt_readentry(realm.rd, empty_ipa, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_UNASSIGNED ||
			(rtt.ripas != RMI_EMPTY) || rtt.walk_level != 3L) {
		ERROR("wrong initial state\n");
		goto destroy_realm;
	}

	/* Map IPAs, keep one page in EMPTY RIPAS state */
	ret = host_realm_delegate_map_protected_data(true, &realm,
		valid_ipa, PAGE_SIZE, TFTF_BASE);
	if (ret != RMI_SUCCESS) {
		ERROR("host_realm_delegate_map_protected_data failed\n");
		goto destroy_realm;
	}
	ret = host_rmi_rtt_readentry(realm.rd, valid_ipa, 3L, &rtt);
	INFO("valid ipa: state base = 0x%lx rtt.state=0x%lx rtt.ripas=0x%lx\n",
	     valid_ipa, rtt.state, rtt.ripas);
	if (ret != RMI_SUCCESS || rtt.state != RMI_ASSIGNED ||
			(rtt.ripas != RMI_RAM)) {
		ERROR("wrong state after DATA_MAP_INIT\n");
		goto undelegate_destroy;
	}

	ret = host_realm_delegate_map_protected_data(false, &realm,
		empty_ipa, PAGE_SIZE, TFTF_BASE);
	if (ret != RMI_SUCCESS) {
		ERROR("host_realm_delegate_map_protected_data failed\n");
		goto undelegate_destroy;
	}
	ret = host_rmi_rtt_readentry(realm.rd, empty_ipa, 3L, &rtt);
	INFO("empty ipa: state base = 0x%lx rtt.state=0x%lx rtt.ripas=0x%lx\n",
	     empty_ipa, rtt.state, rtt.ripas);

	if (ret != RMI_SUCCESS || rtt.state != RMI_ASSIGNED ||
			(rtt.ripas != RMI_EMPTY)) {
		ERROR("wrong state after DATA_MAP\n");
		goto undelegate_destroy;
	}

	/* Activate Realm and assign vdev */
	if (host_realm_activate(&realm) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_activate");
		goto undelegate_destroy;
	}

	h_pdev = get_host_pdev_by_type(DEV_TYPE_INDEPENDENTLY_ATTESTED);
	if (h_pdev == NULL) {
		result = TEST_RESULT_SKIPPED;
		goto undelegate_destroy;
	}

	rc = tsm_connect_device(h_pdev);
	if (rc != 0) {
		ERROR("TSM connect failed for device 0x%x\n", h_pdev->dev->bdf);
		return_error = true;
		goto undelegate_destroy;
	}

	rc = host_assign_vdev_to_realm(&realm, h_vdev, h_pdev->dev->bdf, h_pdev->pdev);
	if (rc != 0) {
		ERROR("VDEV assign to realm failed\n");
		goto disconnect_device;
	}

	/* Execute communicate until vdev reaches VDEV_UNLOCKED state */
	rc = host_vdev_transition(&realm, h_vdev, RMI_VDEV_STATE_UNLOCKED);
	if (rc != 0) {
		ERROR("Transitioning to RMI_VDEV_STATE_UNLOCKED state failed\n");
		return_error = true;
		goto unassign_device;
	}

	/* TEST 1: vdev_get_info with: Valid vdev_id | Valid IPA state */
	INFO("RSI_VDEV_GET_INFO with valid args\n");
	if (!host_da_vdev_get_info(&realm, h_vdev->vdev_id, valid_ipa,
					HOST_RSI_SUCCESS)) {
		ERROR("Realm DA_RSI_CALLS failed\n");
		result = TEST_RESULT_FAIL;
	}

	/* TEST 2: vdev_get_info with: Valid vdev_id | Invalid IPA state */
	INFO("RSI_VDEV_GET_INFO with invalid IPA\n");
	if (!host_da_vdev_get_info(&realm, h_vdev->vdev_id, empty_ipa,
					HOST_RSI_ERROR_INPUT)) {
		ERROR("Realm DA_RSI_CALLS failed\n");
		result = TEST_RESULT_FAIL;
	}

	/* TEST 3: vdev_get_info with: Invalid vdev_id | Valid IPA state */
	INFO("RSI_VDEV_GET_INFO with invalid vdev_id\n");
	if (!host_da_vdev_get_info(&realm, h_vdev->vdev_id + 1, valid_ipa,
					HOST_RSI_ERROR_INPUT)) {
		ERROR("Realm DA_RSI_CALLS failed\n");
		result = TEST_RESULT_FAIL;
	}

unassign_device:
	rc = host_unassign_vdev_from_realm(&realm, h_vdev);
	if (rc != 0) {
		ERROR("VDEV unassign to realm failed\n");
		return_error = true;
	}
disconnect_device:
	if ((tsm_disconnect_device(h_pdev) != 0)) {
		return_error = true;
	}
undelegate_destroy:
	ret = host_rmi_granule_undelegate(valid_ipa);
	ret = host_rmi_granule_undelegate(empty_ipa);
destroy_realm:
	ret2 = host_destroy_realm(&realm);

	if (!ret2) {
		ERROR("%s(): destroy=%d\n",
		      __func__, ret2);
		return TEST_RESULT_FAIL;
	}
	if (return_error) {
		return TEST_RESULT_FAIL;
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

	rc = host_create_realm_with_feat_da(&realm, true);
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
