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

test_result_t host_vdev_create_rejects_out_of_range_tdi_id(void)
{
	test_result_t result = TEST_RESULT_SUCCESS;
	bool return_error = false;
	u_register_t ret;
	u_register_t rmi_feat_reg0;
	unsigned long invalid_tdi_id;
	struct realm realm;
	struct host_pdev *h_pdev = NULL;
	struct host_vdev *h_vdev = &gbl_host_vdev;

	INIT_AND_SKIP_DA_TEST_IF_PREREQS_NOT_MET(rmi_feat_reg0);

	ret = host_create_realm_with_feat_da(&realm, true);
	if (ret != 0UL) {
		ERROR("Realm create with feat_da failed\n");
		return TEST_RESULT_FAIL;
	}

	h_pdev = get_host_pdev_by_type(DEV_TYPE_INDEPENDENTLY_ATTESTED);
	if (h_pdev == NULL) {
		result = TEST_RESULT_SKIPPED;
		goto out_destroy_realm;
	}

	if (tsm_connect_device(h_pdev) != 0) {
		ERROR("TSM connect failed for device 0x%x\n", h_pdev->dev->bdf);
		return_error = true;
		goto out_destroy_realm;
	}

	/*
	 * RMI_PDEV_CREATE gives the endpoint PDEV a single-BDF RID range:
	 * [bdf, bdf + 1). Per bet0 RMI_VDEV_CREATE, rid_top is therefore
	 * the first out-of-range TDI ID.
	 */
	invalid_tdi_id = (unsigned long)h_pdev->dev->bdf + 1UL;

	ret = host_create_vdev(&realm, h_vdev, invalid_tdi_id, h_pdev->ep_pdev);
	if (ret == RMI_SUCCESS) {
		ERROR("RMI_VDEV_CREATE with out-of-range tdi_id=0x%lx returned 0x%lx\n",
		      invalid_tdi_id, ret);
		return_error = true;

		if (host_unassign_vdev_from_realm(&realm, h_vdev) != 0) {
			ERROR("VDEV cleanup failed after unexpected create success\n");
		}
	} else if (ret != RMI_ERROR_INPUT) {
		ERROR("RMI_VDEV_CREATE with out-of-range tdi_id=0x%lx returned 0x%lx\n",
		      invalid_tdi_id, ret);
		return_error = true;
	}

	if (tsm_disconnect_device(h_pdev) != 0) {
		ERROR("TSM disconnect failed for device 0x%x\n", h_pdev->dev->bdf);
		return_error = true;
	}

out_destroy_realm:
	if (!destroy_psmmu_realm(&realm)) {
		return_error = true;
	}

	if (return_error) {
		result = TEST_RESULT_FAIL;
	}

	return result;
}

test_result_t host_vdev_create_rejects_realm_vdev_limit(void)
{
	test_result_t result = TEST_RESULT_SUCCESS;
	bool return_error = false;
	int rc;
	u_register_t ret;
	u_register_t rmi_feat_reg0;
	struct realm realm;
	struct host_pdev *h_pdev = NULL;
	struct host_vdev *h_vdev = &gbl_host_vdev;

	INIT_AND_SKIP_DA_TEST_IF_PREREQS_NOT_MET(rmi_feat_reg0);

	rc = page_pool_init((u_register_t)PAGE_POOL_BASE,
			    (u_register_t)PAGE_POOL_MAX_SIZE);
	if (rc != HEAP_INIT_SUCCESS) {
		ERROR("Failed to init heap pool %d\n", rc);
		return TEST_RESULT_FAIL;
	}

	ret = host_create_realm_with_feat_da_vdev_limit(&realm, true, 0UL);
	if (ret != 0UL) {
		ERROR("Realm create with feat_da and max_num_vdevs=0 failed\n");
		return TEST_RESULT_FAIL;
	}

	h_pdev = get_host_pdev_by_type(DEV_TYPE_INDEPENDENTLY_ATTESTED);
	if (h_pdev == NULL) {
		result = TEST_RESULT_SKIPPED;
		goto out_destroy_realm;
	}

	if (tsm_connect_device(h_pdev) != 0) {
		ERROR("TSM connect failed for device 0x%x\n", h_pdev->dev->bdf);
		return_error = true;
		goto out_destroy_realm;
	}

	ret = host_create_vdev(&realm, h_vdev, h_pdev->dev->bdf, h_pdev->ep_pdev);
	if (ret != RMI_ERROR_DEVICE) {
		ERROR("RMI_VDEV_CREATE with realm max_num_vdevs=0 returned 0x%lx\n", ret);
		return_error = true;

		if ((ret == RMI_SUCCESS) &&
		    (host_unassign_vdev_from_realm(&realm, h_vdev) != 0)) {
			ERROR("VDEV cleanup failed after unexpected create success\n");
		}
	}

	if (tsm_disconnect_device(h_pdev) != 0) {
		ERROR("TSM disconnect failed for device 0x%x\n", h_pdev->dev->bdf);
		return_error = true;
	}

out_destroy_realm:
	if (!destroy_psmmu_realm(&realm)) {
		return_error = true;
	}

	if (return_error) {
		result = TEST_RESULT_FAIL;
	}

	return result;
}

test_result_t host_realm_create_rejects_out_of_range_vdev_limit(void)
{
	struct realm realm;
	struct test_realm_params params = {0};
	u_register_t feat_reg2;
	u_register_t rec_flag[1] = {RMI_RUNNABLE};
	u_register_t max_num_vdevs;
	u_register_t s2sz = MAX_IPA_BITS;
	long sl = RTT_MIN_LEVEL;
	bool lpa2 = false;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (host_rmi_features(RMI_FEATURE_REGISTER_2_INDEX, &feat_reg2) !=
	    REALM_SUCCESS) {
		ERROR("Failed to get RMI feat_reg2\n");
		return TEST_RESULT_FAIL;
	}

	if ((feat_reg2 & RMI_FEATURE_REGISTER_2_DA_EN) == 0UL) {
		WARN("DA not in RMI features, skipping\n");
		return TEST_RESULT_SKIPPED;
	}

	if (is_feat_52b_on_4k_2_supported() == true) {
		lpa2 = true;
		s2sz = MAX_IPA_BITS_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	max_num_vdevs = (1UL << EXTRACT(RMI_FEATURE_REGISTER_2_REALM_MAX_VDEVS_ORDER,
					feat_reg2)) + 1UL;

	params.realm_payload_adr = (u_register_t)REALM_IMAGE_BASE;
	params.rec_flag = rec_flag;
	params.rec_count = 1U;
	params.s2sz = s2sz;
	params.sl = sl;
	params.lpa2 = lpa2;
	params.da = true;
	params.set_max_num_vdevs = true;
	params.max_num_vdevs = max_num_vdevs;

	if (host_create_realm_payload(&realm, &params)) {
		ERROR("Realm create unexpectedly accepted max_num_vdevs=%lu\n",
		      max_num_vdevs);
		host_destroy_realm(&realm);
		return TEST_RESULT_FAIL;
	}

	if (RMI_RETURN_STATUS(realm.last_host_rmi_ret) != RMI_ERROR_INPUT) {
		ERROR("Realm create with max_num_vdevs=%lu returned 0x%lx\n",
		      max_num_vdevs, realm.last_host_rmi_ret);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
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
	bool rc, return_error = false;
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

	ret = host_rmi_rtt_readentry(realm.rd, valid_ipa, 3L, &rtt);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_rtt_readentry failed\n");
		goto destroy_realm;
	}

	ret = host_rmi_create_rtt_levels(&realm, valid_ipa, rtt.walk_level, 3L);
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

	ret = host_rmi_rtt_readentry(realm.rd, empty_ipa, 3L, &rtt);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_rtt_readentry failed\n");
		goto destroy_realm;
	}

	ret = host_rmi_create_rtt_levels(&realm, empty_ipa, rtt.walk_level, 3L);
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

	rc = host_assign_vdev_to_realm(&realm, h_vdev, h_pdev->dev->bdf, h_pdev->ep_pdev, NULL, 0);
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
	(void)host_rmi_granule_undelegate(valid_ipa);
	(void)host_rmi_granule_undelegate(empty_ipa);

destroy_realm:
	/* Deactivate PSMMU and destroy the Realm */
	if (!destroy_psmmu_realm(&realm)) {
		ERROR("destroy_psmmu_realm() failed\n");
		return TEST_RESULT_FAIL;
	}
	if (return_error) {
		return TEST_RESULT_FAIL;
	}
	return result;
}

enum pdev_negative_command {
	PDEV_NEG_COMMUNICATE,
	PDEV_NEG_ABORT,
	PDEV_NEG_SET_PUBKEY,
	PDEV_NEG_STOP,
	PDEV_NEG_DESTROY
};

/*
 * SEC 2 P-384 base point, encoded as an uncompressed public key. The leading
 * 0x04 indicates an uncompressed elliptic-curve point. Because it is a known
 * valid P-384 point, negative SET_PUBKEY tests reach lifecycle validation
 * instead of failing earlier due to malformed key data.
 *
 * Generated with:
 * $ openssl ecparam -name secp384r1 -genkey -noout -out p384-private.pem
 * $ openssl ec -in p384-private.pem -pubout -conv_form uncompressed \
 *       -outform DER | tail -c 97 > p384-public.bin
 * $ xxd -i p384-public.bin
 */
static const uint8_t pdev_test_p384_public_key[97] = {
  0x04, 0x70, 0x8e, 0x9f, 0xb6, 0x59, 0x67, 0x06, 0xea, 0x99, 0xc2, 0xbb,
  0x73, 0x64, 0x2b, 0x60, 0xbc, 0x0a, 0x4b, 0xef, 0xa1, 0x26, 0xac, 0xf5,
  0x82, 0xe9, 0x56, 0xc6, 0x58, 0x1b, 0xfa, 0x4b, 0x59, 0xaf, 0x6f, 0x3f,
  0xc6, 0x3e, 0x5b, 0x73, 0x7c, 0xaa, 0x88, 0xfb, 0xaf, 0xf5, 0x91, 0xe2,
  0x40, 0x0b, 0x50, 0x51, 0x0f, 0x3d, 0x1c, 0xd3, 0x0a, 0x77, 0x82, 0x29,
  0x25, 0xaa, 0x61, 0xc8, 0xc3, 0x17, 0xd1, 0xff, 0xd9, 0x9c, 0x1f, 0xfe,
  0x26, 0xc4, 0x7b, 0x6a, 0x28, 0x6c, 0x8a, 0x2c, 0x88, 0x6a, 0x15, 0x08,
  0x9e, 0x21, 0xac, 0xa2, 0xa7, 0x2e, 0xd3, 0xb8, 0x1c, 0xe6, 0x7f, 0x06,
  0x44
};

static test_result_t pdev_test_init(struct host_pdev **h_pdev)
{
	u_register_t rmi_feat_reg0;

	INIT_AND_SKIP_DA_TEST_IF_PREREQS_NOT_MET(rmi_feat_reg0);
	if (!host_rmm_activate()) {
		return TEST_RESULT_FAIL;
	}

	*h_pdev = get_host_pdev_by_type(DEV_TYPE_INDEPENDENTLY_ATTESTED);
	if (*h_pdev == NULL) {
		return TEST_RESULT_SKIPPED;
	}

	return TEST_RESULT_SUCCESS;
}

static int pdev_expect_state(struct host_pdev *h_pdev, bool ep_pdev,
			     u_register_t expected)
{
	u_register_t state;
	int rc;

	rc = host_pdev_get_state(h_pdev, ep_pdev, &state);
	if ((rc != 0) || (state != expected)) {
		ERROR("PDEV state mismatch: got %lu expected %lu\n", state,
		      expected);
		return -1;
	}
	return 0;
}

static void *pdev_ptr(struct host_pdev *h_pdev, bool ep_pdev)
{
	return ep_pdev ? h_pdev->ep_pdev : h_pdev->rp_pdev;
}

/*
 * Verify the architectural error and lifecycle non-mutation.  comm_state and
 * op are not returned by GET_STATE; preserving exit flags and successfully
 * continuing the original transaction provide the behavioural proxy.
 */
static int pdev_expect_negative(struct host_pdev *h_pdev, bool ep_pdev,
				enum pdev_negative_command command)
{
	struct rmi_public_key_params *params = NULL;
	u_register_t destroy_handle;
	u_register_t reclaim_req;
	u_register_t state_before;
	u_register_t state_after;
	u_register_t flags_before;
	u_register_t ret;

	if (host_pdev_get_state(h_pdev, ep_pdev, &state_before) != 0) {
		return -1;
	}
	flags_before = h_pdev->dev_comm_data->exit.flags;

	switch (command) {
	case PDEV_NEG_COMMUNICATE:
		h_pdev->dev_comm_data->enter.status =
			RMI_DEV_COMM_ENTER_STATUS_NONE;
		h_pdev->dev_comm_data->enter.resp_len = 0UL;
		ret = host_rmi_pdev_communicate(
			(u_register_t)pdev_ptr(h_pdev, ep_pdev),
			(u_register_t)h_pdev->dev_comm_data);
		break;
	case PDEV_NEG_ABORT:
		ret = host_rmi_pdev_abort(
			(u_register_t)pdev_ptr(h_pdev, ep_pdev));
		break;
	case PDEV_NEG_SET_PUBKEY:
		params = page_alloc(PAGE_SIZE);
		if (params == NULL) {
			return -1;
		}
		memset(params, 0, GRANULE_SIZE);
		params->key_addr = (u_register_t)h_pdev->public_key;
		params->metadata_addr =
			(u_register_t)h_pdev->public_key_metadata;
		memcpy(h_pdev->public_key, pdev_test_p384_public_key,
		       sizeof(pdev_test_p384_public_key));
		params->key_len = sizeof(pdev_test_p384_public_key);
		params->metadata_len = 0UL;
		ret = host_rmi_pdev_set_pubkey(
			(u_register_t)pdev_ptr(h_pdev, ep_pdev),
			(u_register_t)params);
		page_free((u_register_t)params);
		break;
	case PDEV_NEG_STOP:
		ret = host_rmi_pdev_stop(
			(u_register_t)pdev_ptr(h_pdev, ep_pdev));
		break;
	case PDEV_NEG_DESTROY:
		ret = host_rmi_pdev_destroy(
			(u_register_t)pdev_ptr(h_pdev, ep_pdev),
			&destroy_handle, &reclaim_req);
		break;
	default:
		return -1;
	}

	if (ret != RMI_ERROR_DEVICE) {
		ERROR("Negative PDEV command %u returned 0x%lx\n", command, ret);
		return -1;
	}
	if ((host_pdev_get_state(h_pdev, ep_pdev, &state_after) != 0) ||
	    (state_after != state_before) ||
	    (h_pdev->dev_comm_data->exit.flags != flags_before)) {
		ERROR("Negative PDEV command %u mutated observable state\n", command);
		return -1;
	}

	return 0;
}

static int pdev_create_spdm(struct host_pdev *h_pdev, bool ep_pdev)
{
	if ((host_pdev_create(h_pdev, ep_pdev) != 0) ||
	    (pdev_expect_state(h_pdev, ep_pdev, RMI_PDEV_STATE_NEW) != 0)) {
		return -1;
	}
	/* A successful first COMMUNICATE later proves CREATE left it PENDING. */
	return 0;
}

static int pdev_advance_to_ready(struct host_pdev *h_pdev, bool ep_pdev)
{
	/* The current RMM implementation completes Root Port setup directly. */
	if (!ep_pdev) {
		if (host_pdev_communicate_complete(h_pdev, false,
						  RMI_PDEV_STATE_READY) != 0) {
			return -1;
		}
		return pdev_expect_state(h_pdev, false, RMI_PDEV_STATE_READY);
	}

	h_pdev->cert_chain_len = 0UL;
	h_pdev->vca_len = 0UL;
	if ((host_pdev_communicate_complete(h_pdev, ep_pdev,
					  RMI_PDEV_STATE_NEEDS_KEY) != 0) ||
	    (pdev_expect_state(h_pdev, ep_pdev,
			       RMI_PDEV_STATE_NEEDS_KEY) != 0) ||
	    (host_pdev_set_pubkey(h_pdev, ep_pdev) != 0) ||
	    (pdev_expect_state(h_pdev, ep_pdev, RMI_PDEV_STATE_HAS_KEY) != 0) ||
	    (host_pdev_communicate_complete(h_pdev, ep_pdev,
					  RMI_PDEV_STATE_READY) != 0)) {
		return -1;
	}
	return pdev_expect_state(h_pdev, ep_pdev, RMI_PDEV_STATE_READY);
}

static int pdev_stop_and_destroy(struct host_pdev *h_pdev, bool ep_pdev)
{
	if ((host_pdev_stop(h_pdev, ep_pdev) != 0) ||
	    (host_pdev_communicate_complete(h_pdev, ep_pdev,
					  RMI_PDEV_STATE_STOPPED) != 0) ||
	    (pdev_expect_state(h_pdev, ep_pdev, RMI_PDEV_STATE_STOPPED) != 0) ||
	    (host_pdev_destroy(h_pdev, ep_pdev) != 0)) {
		return -1;
	}
	return 0;
}

static int pdev_stream_complete_cmd(struct host_pdev *h_pdev, bool released);

static void pdev_cleanup_one(struct host_pdev *h_pdev, bool ep_pdev)
{
	u_register_t state;

	if (pdev_ptr(h_pdev, ep_pdev) == NULL) {
		return;
	}
	if (host_pdev_get_state(h_pdev, ep_pdev, &state) != 0) {
		return;
	}
	if ((state == RMI_PDEV_STATE_NEW) ||
	    (state == RMI_PDEV_STATE_HAS_KEY)) {
		(void)host_pdev_abort(h_pdev, ep_pdev);
		(void)host_pdev_get_state(h_pdev, ep_pdev, &state);
	}
	if ((state == RMI_PDEV_STATE_NEEDS_KEY) ||
	    (state == RMI_PDEV_STATE_READY) ||
	    (state == RMI_PDEV_STATE_ERROR)) {
		(void)host_pdev_abort(h_pdev, ep_pdev);
		if (host_pdev_stop(h_pdev, ep_pdev) == 0) {
			(void)host_pdev_communicate_complete(h_pdev, ep_pdev,
						   RMI_PDEV_STATE_STOPPED);
		}
		(void)host_pdev_get_state(h_pdev, ep_pdev, &state);
		/*
		 * Current RMM can turn a failed STOP transaction into ERROR.
		 * Retry only for cleanup so the conformance assertion still fails.
		 */
		if ((state == RMI_PDEV_STATE_ERROR) &&
		    (host_pdev_stop(h_pdev, ep_pdev) == 0)) {
			(void)host_pdev_communicate_complete(h_pdev, ep_pdev,
						   RMI_PDEV_STATE_STOPPED);
			(void)host_pdev_get_state(h_pdev, ep_pdev, &state);
		}
	}
	if (state == RMI_PDEV_STATE_STOPPED) {
		(void)host_pdev_destroy(h_pdev, ep_pdev);
	}
}

static test_result_t pdev_fail_cleanup(struct host_pdev *h_pdev)
{
	if (h_pdev->pdev_stream_handle_valid) {
		if (h_pdev->ep_pdev != NULL) {
			(void)host_pdev_abort(h_pdev, true);
		}
		if (h_pdev->rp_pdev != NULL) {
			(void)host_pdev_abort(h_pdev, false);
		}
		if ((h_pdev->ep_pdev != NULL) && (h_pdev->rp_pdev != NULL)) {
			(void)pdev_stream_complete_cmd(h_pdev, true);
		}
	}
	pdev_cleanup_one(h_pdev, true);
	pdev_cleanup_one(h_pdev, false);
	return TEST_RESULT_FAIL;
}

static int pdev_stream_reach_wait(struct host_pdev *h_pdev, bool ep_pdev,
				  bool leave_incomplete)
{
	if (leave_incomplete &&
	    (host_pdev_communicate_incomplete(h_pdev, ep_pdev) != 0)) {
		return -1;
	}
	return host_pdev_communicate_until_wait(h_pdev, ep_pdev);
}

static int pdev_stream_finish(struct host_pdev *h_pdev, bool ep_pdev)
{
	if (host_pdev_communicate_complete(h_pdev, ep_pdev,
					 RMI_PDEV_STATE_READY) != 0) {
		return -1;
	}
	return pdev_expect_state(h_pdev, ep_pdev, RMI_PDEV_STATE_READY);
}

static int pdev_stream_complete_cmd(struct host_pdev *h_pdev, bool released)
{
	u_register_t ret;

	ret = host_rmi_pdev_stream_complete((u_register_t)h_pdev->ep_pdev,
					    (u_register_t)h_pdev->rp_pdev,
					    h_pdev->pdev_stream_handle);
	if (ret != RMI_SUCCESS) {
		ERROR("RMI_PDEV_STREAM_COMPLETE failed: 0x%lx\n", ret);
		return -1;
	}
	if (released) {
		h_pdev->pdev_stream_handle_valid = false;
	}
	return 0;
}

/* Complete SPDM setup, incomplete self-transitions, and state negatives. */
test_result_t host_pdev_test_spdm_lifecycle(void)
{
	struct host_pdev *h_pdev;
	test_result_t result = pdev_test_init(&h_pdev);

	if (result != TEST_RESULT_SUCCESS) {
		return result;
	}
	if ((pdev_create_spdm(h_pdev, true) != 0) ||
	    (pdev_expect_negative(h_pdev, true, PDEV_NEG_SET_PUBKEY) != 0) ||
	    (pdev_expect_negative(h_pdev, true, PDEV_NEG_STOP) != 0) ||
	    (pdev_expect_negative(h_pdev, true, PDEV_NEG_DESTROY) != 0) ||
	    (host_pdev_communicate_incomplete(h_pdev, true) != 0) ||
	    (pdev_expect_state(h_pdev, true, RMI_PDEV_STATE_NEW) != 0) ||
	    (host_pdev_communicate_complete(h_pdev, true,
					  RMI_PDEV_STATE_NEEDS_KEY) != 0) ||
	    (pdev_expect_negative(h_pdev, true, PDEV_NEG_COMMUNICATE) != 0) ||
	    (pdev_expect_negative(h_pdev, true, PDEV_NEG_ABORT) != 0) ||
	    (pdev_expect_negative(h_pdev, true, PDEV_NEG_DESTROY) != 0) ||
	    (host_pdev_set_pubkey(h_pdev, true) != 0) ||
	    (pdev_expect_negative(h_pdev, true, PDEV_NEG_SET_PUBKEY) != 0) ||
	    (pdev_expect_negative(h_pdev, true, PDEV_NEG_STOP) != 0) ||
	    (pdev_expect_negative(h_pdev, true, PDEV_NEG_DESTROY) != 0) ||
	    (host_pdev_communicate_incomplete(h_pdev, true) != 0) ||
	    (pdev_expect_state(h_pdev, true, RMI_PDEV_STATE_HAS_KEY) != 0) ||
	    (host_pdev_communicate_complete(h_pdev, true,
					  RMI_PDEV_STATE_READY) != 0) ||
	    (pdev_expect_negative(h_pdev, true, PDEV_NEG_COMMUNICATE) != 0) ||
	    (pdev_expect_negative(h_pdev, true, PDEV_NEG_ABORT) != 0) ||
	    (pdev_expect_negative(h_pdev, true, PDEV_NEG_SET_PUBKEY) != 0) ||
	    (pdev_expect_negative(h_pdev, true, PDEV_NEG_DESTROY) != 0) ||
	    (host_pdev_stop(h_pdev, true) != 0) ||
	    (host_pdev_communicate_incomplete(h_pdev, true) != 0) ||
	    (host_pdev_communicate_complete(h_pdev, true,
					  RMI_PDEV_STATE_STOPPED) != 0) ||
	    (pdev_expect_negative(h_pdev, true, PDEV_NEG_COMMUNICATE) != 0) ||
	    (pdev_expect_negative(h_pdev, true, PDEV_NEG_ABORT) != 0) ||
	    (pdev_expect_negative(h_pdev, true, PDEV_NEG_SET_PUBKEY) != 0) ||
	    (pdev_expect_negative(h_pdev, true, PDEV_NEG_STOP) != 0) ||
	    (host_pdev_destroy(h_pdev, true) != 0)) {
		return pdev_fail_cleanup(h_pdev);
	}
	return TEST_RESULT_SUCCESS;
}

/* NEEDS_KEY -> STOPPED. */
test_result_t host_pdev_test_stop_from_needs_key(void)
{
	struct host_pdev *h_pdev;
	test_result_t result = pdev_test_init(&h_pdev);

	if (result != TEST_RESULT_SUCCESS) {
		return result;
	}
	if ((pdev_create_spdm(h_pdev, true) != 0) ||
	    (host_pdev_communicate_complete(h_pdev, true,
					  RMI_PDEV_STATE_NEEDS_KEY) != 0) ||
	    (pdev_stop_and_destroy(h_pdev, true) != 0)) {
		return pdev_fail_cleanup(h_pdev);
	}
	return TEST_RESULT_SUCCESS;
}

/* Abort an ACTIVE NEW transaction, then ERROR -> STOPPED. */
test_result_t host_pdev_test_abort_new(void)
{
	struct host_pdev *h_pdev;
	test_result_t result = pdev_test_init(&h_pdev);

	if (result != TEST_RESULT_SUCCESS) {
		return result;
	}
	if ((pdev_create_spdm(h_pdev, true) != 0) ||
	    (host_pdev_communicate_incomplete(h_pdev, true) != 0) ||
	    (host_pdev_abort(h_pdev, true) != 0) ||
	    (pdev_expect_state(h_pdev, true, RMI_PDEV_STATE_ERROR) != 0) ||
	    (pdev_expect_negative(h_pdev, true, PDEV_NEG_COMMUNICATE) != 0) ||
	    (pdev_expect_negative(h_pdev, true, PDEV_NEG_ABORT) != 0) ||
	    (pdev_expect_negative(h_pdev, true, PDEV_NEG_SET_PUBKEY) != 0) ||
	    (pdev_expect_negative(h_pdev, true, PDEV_NEG_DESTROY) != 0) ||
	    (pdev_stop_and_destroy(h_pdev, true) != 0)) {
		return pdev_fail_cleanup(h_pdev);
	}
	return TEST_RESULT_SUCCESS;
}

/* Abort the secure-session transaction in HAS_KEY. */
test_result_t host_pdev_test_abort_has_key(void)
{
	struct host_pdev *h_pdev;
	test_result_t result = pdev_test_init(&h_pdev);

	if (result != TEST_RESULT_SUCCESS) {
		return result;
	}
	if ((pdev_create_spdm(h_pdev, true) != 0) ||
	    (host_pdev_communicate_complete(h_pdev, true,
					  RMI_PDEV_STATE_NEEDS_KEY) != 0) ||
	    (host_pdev_set_pubkey(h_pdev, true) != 0) ||
	    (host_pdev_communicate_incomplete(h_pdev, true) != 0) ||
	    (host_pdev_abort(h_pdev, true) != 0) ||
	    (pdev_expect_state(h_pdev, true, RMI_PDEV_STATE_ERROR) != 0) ||
	    (pdev_stop_and_destroy(h_pdev, true) != 0)) {
		return pdev_fail_cleanup(h_pdev);
	}
	return TEST_RESULT_SUCCESS;
}

/* READY self-transition, stream completion, and non-revertible abort. */
test_result_t host_pdev_test_stream_lifecycle(void)
{
	struct host_pdev *h_pdev;
	test_result_t result = pdev_test_init(&h_pdev);

	if (result != TEST_RESULT_SUCCESS) {
		return result;
	}
	/* Both PDEVs are created before either is advanced beyond NEW. */
	if ((pdev_create_spdm(h_pdev, true) != 0) ||
	    (pdev_create_spdm(h_pdev, false) != 0) ||
	    (pdev_advance_to_ready(h_pdev, true) != 0) ||
	    (pdev_advance_to_ready(h_pdev, false) != 0) ||
	    (host_pdev_stream_connect(h_pdev) != 0) ||
	    (host_pdev_communicate_incomplete(h_pdev, false) != 0) ||
	    (pdev_expect_negative(h_pdev, false, PDEV_NEG_STOP) != 0) ||
	    (host_pdev_communicate_until_wait(h_pdev, false) != 0) ||
	    (pdev_stream_reach_wait(h_pdev, true, false) != 0) ||
	    (pdev_stream_finish(h_pdev, false) != 0) ||
	    (pdev_stream_finish(h_pdev, true) != 0) ||
	    (pdev_stream_complete_cmd(h_pdev, false) != 0) ||
	    (host_pdev_stream_disconnect(h_pdev) != 0) ||
	    (pdev_stream_reach_wait(h_pdev, false, false) != 0) ||
	    (pdev_stream_reach_wait(h_pdev, true, false) != 0) ||
	    (pdev_stream_finish(h_pdev, false) != 0) ||
	    (pdev_stream_finish(h_pdev, true) != 0) ||
	    (pdev_stream_complete_cmd(h_pdev, true) != 0) ||
	    (host_pdev_stream_connect(h_pdev) != 0) ||
	    (host_pdev_communicate_incomplete(h_pdev, false) != 0) ||
	    (pdev_expect_negative(h_pdev, false, PDEV_NEG_STOP) != 0) ||
	    (host_pdev_abort(h_pdev, false) != 0) ||
	    (pdev_expect_state(h_pdev, false, RMI_PDEV_STATE_ERROR) != 0) ||
	    /* A two-PDEV stream must retire the peer operation as well. */
	    (host_pdev_abort(h_pdev, true) != 0) ||
	    (pdev_expect_state(h_pdev, true, RMI_PDEV_STATE_ERROR) != 0) ||
	    (pdev_stream_complete_cmd(h_pdev, true) != 0) ||
	    (pdev_stop_and_destroy(h_pdev, true) != 0) ||
	    (pdev_stop_and_destroy(h_pdev, false) != 0)) {
		return pdev_fail_cleanup(h_pdev);
	}
	return TEST_RESULT_SUCCESS;
}

/* A stream communication error produces READY -> ERROR. */
test_result_t host_pdev_test_stream_communication_error(void)
{
	struct host_pdev *h_pdev;
	test_result_t result = pdev_test_init(&h_pdev);

	if (result != TEST_RESULT_SUCCESS) {
		return result;
	}
	if ((pdev_create_spdm(h_pdev, true) != 0) ||
	    (pdev_create_spdm(h_pdev, false) != 0) ||
	    (pdev_advance_to_ready(h_pdev, true) != 0) ||
	    (pdev_advance_to_ready(h_pdev, false) != 0) ||
	    (host_pdev_stream_connect(h_pdev) != 0) ||
	    /* Inject on the RP so the endpoint SPDM session remains usable. */
	    (host_pdev_communicate_incomplete(h_pdev, false) != 0) ||
	    (host_pdev_inject_communication_error(h_pdev, false) != 0) ||
	    (pdev_expect_state(h_pdev, false, RMI_PDEV_STATE_ERROR) != 0) ||
	    /* Retire the blocked peer before completing this NCOH stream. */
	    (host_pdev_abort(h_pdev, true) != 0) ||
	    (pdev_expect_state(h_pdev, true, RMI_PDEV_STATE_ERROR) != 0) ||
	    (pdev_stream_complete_cmd(h_pdev, true) != 0) ||
	    (pdev_stop_and_destroy(h_pdev, true) != 0) ||
	    (pdev_stop_and_destroy(h_pdev, false) != 0)) {
		return pdev_fail_cleanup(h_pdev);
	}
	return TEST_RESULT_SUCCESS;
}

/* STOP completes despite RMI_DEV_COMM_ERROR and stores DEV_COMM_ERROR. */
test_result_t host_pdev_test_stop_communication_error(void)
{
	struct host_pdev *h_pdev;
	test_result_t result = pdev_test_init(&h_pdev);

	if (result != TEST_RESULT_SUCCESS) {
		return result;
	}
	if ((pdev_create_spdm(h_pdev, true) != 0) ||
	    (pdev_advance_to_ready(h_pdev, true) != 0) ||
	    (host_pdev_stop(h_pdev, true) != 0) ||
	    (host_pdev_communicate_incomplete(h_pdev, true) != 0) ||
	    (host_pdev_inject_communication_error(h_pdev, true) != 0) ||
	    (pdev_expect_state(h_pdev, true, RMI_PDEV_STATE_STOPPED) != 0) ||
	    (pdev_expect_negative(h_pdev, true, PDEV_NEG_COMMUNICATE) != 0) ||
	    (host_pdev_destroy(h_pdev, true) != 0)) {
		return pdev_fail_cleanup(h_pdev);
	}
	return TEST_RESULT_SUCCESS;
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
				       h_pdev->dev->bdf, h_pdev->ep_pdev, NULL, 0);
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

	/* RMI_VDEV_DESTROY from NEW must fail. */
	INFO("VDEV states: destroy from NEW must fail\n");
	ret = host_rmi_vdev_destroy(realm.rd, (u_register_t)h_pdev->ep_pdev,
				    (u_register_t)h_vdev->vdev_ptr);
	if (ret != RMI_ERROR_DEVICE) {
		ERROR("VDEV destroy from NEW returned 0x%lx\n", ret);
		return_error = true;
		goto out_unassign_vdev;
	}

	/* RMI_VDEV_LOCK from NEW must fail. */
	INFO("VDEV states: lock from NEW must fail\n");
	ret = host_rmi_vdev_lock(realm.rd, (u_register_t)h_pdev->ep_pdev,
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

	/* Complete the implicit unlock operation started by VDEV_CREATE. */
	INFO("VDEV states: NEW -> UNLOCKED\n");
	rc = host_vdev_transition(&realm, h_vdev, RMI_VDEV_STATE_UNLOCKED);
	if (rc != 0) {
		ERROR("Transition to UNLOCKED failed\n");
		return_error = true;
		goto out_unassign_vdev;
	}

	/* Start a lock operation, leave its communication pending and verify
	 * that destroy still fails.
	 */
	INFO("VDEV states: destroy with pending communication must fail\n");
	ret = host_rmi_vdev_lock(realm.rd, (u_register_t)h_pdev->ep_pdev,
				 (u_register_t)h_vdev->vdev_ptr);
	if (ret != RMI_SUCCESS) {
		ERROR("VDEV lock from UNLOCKED returned 0x%lx\n", ret);
		return_error = true;
		goto out_unassign_vdev;
	}

	ret = host_rmi_vdev_destroy(realm.rd, (u_register_t)h_pdev->ep_pdev,
				    (u_register_t)h_vdev->vdev_ptr);
	if (ret != RMI_ERROR_DEVICE) {
		ERROR("VDEV destroy with pending communication returned 0x%lx\n",
		      ret);
		return_error = true;
		goto out_unassign_vdev;
	}

	/* Abort the pending operation, moving the VDEV to ERROR. */
	INFO("VDEV states: abort pending communication -> ERROR\n");
	ret = host_rmi_vdev_abort(realm.rd,
				  (u_register_t)h_pdev->ep_pdev,
				  (u_register_t)h_vdev->vdev_ptr);
	if (ret != RMI_SUCCESS) {
		ERROR("VDEV abort returned 0x%lx\n", ret);
		return_error = true;
		goto out_unassign_vdev;
	}
	if (host_vdev_expect_state(h_vdev, RMI_VDEV_STATE_ERROR) != 0) {
		return_error = true;
		goto out_unassign_vdev;
	}

	INFO("VDEV states: destroy from ERROR must fail\n");
	ret = host_rmi_vdev_destroy(realm.rd, (u_register_t)h_pdev->ep_pdev,
				    (u_register_t)h_vdev->vdev_ptr);
	if (ret != RMI_ERROR_DEVICE) {
		ERROR("VDEV destroy from ERROR returned 0x%lx\n", ret);
		return_error = true;
		goto out_unassign_vdev;
	}

	/* ERROR -> UNLOCKED via RMI_VDEV_UNLOCK + COMMUNICATE. */
	INFO("VDEV states: ERROR -> UNLOCKED\n");
	rc = host_vdev_unlock(&realm, h_vdev, RMI_VDEV_STATE_UNLOCKED);
	if (rc != 0) {
		ERROR("Unlock from ERROR failed\n");
		return_error = true;
		goto out_unassign_vdev;
	}

	if (host_vdev_expect_state(h_vdev, RMI_VDEV_STATE_UNLOCKED) != 0) {
		return_error = true;
		goto out_unassign_vdev;
	}

	/* RMI_VDEV_START/UNLOCK from UNLOCKED must fail */
	INFO("VDEV states: start from UNLOCKED must fail\n");
	ret = host_rmi_vdev_start(realm.rd, (u_register_t)h_pdev->ep_pdev,
				  (u_register_t)h_vdev->vdev_ptr);
	if (ret != RMI_ERROR_DEVICE) {
		ERROR("VDEV start from UNLOCKED returned 0x%lx\n", ret);
		return_error = true;
		goto out_unassign_vdev;
	}

	INFO("VDEV states: unlock from UNLOCKED must fail\n");
	ret = host_rmi_vdev_unlock(realm.rd, (u_register_t)h_pdev->ep_pdev,
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

	/* RMI_VDEV_DESTROY succeeds only from UNLOCKED. */
	INFO("VDEV states: destroy from UNLOCKED must succeed\n");
	ret = host_rmi_vdev_destroy(realm.rd, (u_register_t)h_pdev->ep_pdev,
				    (u_register_t)h_vdev->vdev_ptr);
	if (ret != RMI_SUCCESS) {
		ERROR("VDEV destroy from UNLOCKED returned 0x%lx\n", ret);
		return_error = true;
		goto out_unassign_vdev;
	}

	ret = host_rmi_granule_undelegate((u_register_t)h_vdev->vdev_ptr);
	if (ret != RMI_SUCCESS) {
		ERROR("VDEV undelegate after destroy returned 0x%lx\n", ret);
		return_error = true;
		goto out_unassign_vdev;
	}
	vdev_assigned = false;

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
		if (!destroy_psmmu_realm(&realm)) {
			return_error = true;
		}
	}

	return return_error ? TEST_RESULT_FAIL : TEST_RESULT_SUCCESS;
}

/* Invoke IDE key refresh and IDE reset on PDEV once it is in READY state */
test_result_t host_pdev_invoke_ide_refresh_reset(void)
{
	struct host_pdev *h_pdev;
	u_register_t rmi_feat_reg0;
	bool return_error = false;
	int rc = 0;

	INIT_AND_SKIP_DA_TEST_IF_PREREQS_NOT_MET(rmi_feat_reg0);

	h_pdev = get_host_pdev_by_type(DEV_TYPE_INDEPENDENTLY_ATTESTED);
	if (h_pdev == NULL) {
		return TEST_RESULT_SKIPPED;
	}

	rc = tsm_connect_device(h_pdev);
	if (rc != 0) {
		ERROR("TSM connect failed for device 0x%x\n", h_pdev->dev->bdf);
		return TEST_RESULT_FAIL;
	}

	INFO("Invoke pdev_ide_stream_key_refresh sequence\n");
	rc = host_pdev_stream_key_refresh(h_pdev);
	if (rc != 0) {
		ERROR("pdev_ide_stream_key_refresh failed\n");
		return_error = true;
	}

	rc = host_pdev_stream_complete(h_pdev);
	if (rc != 0) {
		ERROR("pdev_ide_stream_key_refresh stream complete failed\n");
		return_error = true;
	}

	/* Disconnect the device from TSM */
	rc = tsm_disconnect_device(h_pdev);
	if (rc != 0) {
		ERROR("TSM disconnect failed for device 0x%x\n",
		      h_pdev->dev->bdf);
		return_error = true;
	}

	/* Deactivate PSMMU */
	rc = deactivate_psmmu();
	if (rc != 0) {
		ERROR("PSMMU deactivate failed for device 0x%x\n",
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
	u_register_t create_handle;
	u_register_t donate_req;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_NO_RMM();

	if (host_rmi_version(RMI_ABI_VERSION_VAL) != 0U) {
		tftf_testcase_printf("RMM is not TRP\n");
		return TEST_RESULT_SKIPPED;
	}

	/*
	 * Directly call host_rmi_pdev_create with invalid pdev, expect an error
	 * to be returned from TRP.
	 */
	rmi_rc = host_rmi_pdev_create((u_register_t)0UL, (u_register_t)0UL,
		&create_handle,
		&donate_req);

	/*
	 * Start the RSO flow for RMI_pdev_CREATE. It is expected
	 * that the host will donate all the memory in one go.
	 */
	if (RMI_RETURN_STATUS(rmi_rc) == RMI_INCOMPLETE) {
		rmi_rc = host_realm_sro_continue(rmi_rc, &create_handle,
						 &donate_req, NULL);
	}

	if (rmi_rc != RMI_SUCCESS) {
		return TEST_RESULT_SUCCESS;
	}

	ERROR("RMI_PDEV_CREATE did not return error as expected\n");
	return TEST_RESULT_FAIL;
}
