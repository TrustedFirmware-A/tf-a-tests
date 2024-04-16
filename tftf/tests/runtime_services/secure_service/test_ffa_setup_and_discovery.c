/*
 * Copyright (c) 2020-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "utils_def.h"
#include <debug.h>

#include <ffa_endpoints.h>
#include <ffa_helpers.h>
#include <ffa_svc.h>
#include <spm_common.h>
#include <spm_test_helpers.h>
#include <test_helpers.h>
#include <tftf_lib.h>
#include <xlat_tables_defs.h>

static bool should_skip_version_test;

/*
 * Used as the RX/TX buffers belonging to VM 1 in the forwarding FFA_RXTX_MAP
 * tests.
 */
static __aligned(PAGE_SIZE) uint8_t vm1_rx_buffer[PAGE_SIZE];
static __aligned(PAGE_SIZE) uint8_t vm1_tx_buffer[PAGE_SIZE];

/*
 * Used as the RX/TX buffers belonging to VM 2 in the forwarding FFA_RXTX_MAP
 * tests.
 */
static __aligned(PAGE_SIZE) uint8_t vm2_rx_buffer[PAGE_SIZE];
static __aligned(PAGE_SIZE) uint8_t vm2_tx_buffer[PAGE_SIZE];

static struct mailbox_buffers mb;

static const struct ffa_uuid sp_uuids[] = {
		{PRIMARY_UUID}, {SECONDARY_UUID}, {TERTIARY_UUID}, {IVY_UUID}
	};

static const struct ffa_partition_info ffa_expected_partition_info[] = {
	/* Primary partition info */
	{
		.id = SP_ID(1),
		.exec_context = PRIMARY_EXEC_CTX_COUNT,
		.properties = FFA_PARTITION_AARCH64_EXEC |
			      FFA_PARTITION_DIRECT_REQ_RECV |
			      FFA_PARTITION_NOTIFICATION,
		.uuid = {PRIMARY_UUID}
	},
	/* Secondary partition info */
	{
		.id = SP_ID(2),
		.exec_context = SECONDARY_EXEC_CTX_COUNT,
		.properties = FFA_PARTITION_AARCH64_EXEC |
			      FFA_PARTITION_DIRECT_REQ_RECV |
			      FFA_PARTITION_NOTIFICATION,
		.uuid = {SECONDARY_UUID}
	},
	/* Tertiary partition info */
	{
		.id = SP_ID(3),
		.exec_context = TERTIARY_EXEC_CTX_COUNT,
		.properties = FFA_PARTITION_AARCH64_EXEC |
			      FFA_PARTITION_DIRECT_REQ_RECV |
			      FFA_PARTITION_NOTIFICATION,
		.uuid = {TERTIARY_UUID}
	},
	/* Ivy partition info */
	{
		.id = SP_ID(4),
		.exec_context = IVY_EXEC_CTX_COUNT,
		.properties = FFA_PARTITION_AARCH64_EXEC |
			      FFA_PARTITION_DIRECT_REQ_RECV,
		.uuid = {IVY_UUID}
	}
};

/*
 * Using FFA version expected for SPM.
 */
#define SPM_VERSION FFA_VERSION_COMPILED

/******************************************************************************
 * FF-A Features ABI Tests
 ******************************************************************************/

test_result_t test_ffa_features(void)
{
	const struct ffa_features_test *func_ids_target;
	const struct ffa_features_test feature_ids_target[] = {
		{"FFA_FEATURE_MEI", FFA_FEATURE_MEI, FFA_ERROR, 0,
			FFA_VERSION_1_1},
		{"FFA_FEATURE_SRI", FFA_FEATURE_SRI, FFA_SUCCESS_SMC32, 0,
			FFA_VERSION_1_1},
		{"FFA_FEATURE_NPI", FFA_FEATURE_NPI, FFA_ERROR, 0,
			FFA_VERSION_1_1},
	};
	unsigned int test_target_size =
		get_ffa_feature_test_target(&func_ids_target);

	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 0);

	/* Check if SPMC is OP-TEE at S-EL1 */
	if (check_spmc_execution_level()) {
		/* FFA_FEATURES is not yet supported in OP-TEE */
		return TEST_RESULT_SKIPPED;
	}

	if (!ffa_features_test_targets(func_ids_target, test_target_size)) {
		return TEST_RESULT_FAIL;
	}

	if (!ffa_features_test_targets(feature_ids_target,
				ARRAY_SIZE(feature_ids_target))) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Test that `FFA_FEATURES(FFA_RXTX_MAP_SMC64)` returns a parameter explaining
 * the maximum and minimum buffer size on v1.2 or greater.
 */
test_result_t test_ffa_features_rxtx_map(void)
{

	struct ffa_value args = {
		.fid = FFA_FEATURES,
		.arg1 = FFA_RXTX_MAP_SMC64,
	};
	struct ffa_value ret;
	uint32_t param;
	const uint32_t expected_param = (FFA_RXTX_MAP_MIN_BUF_4K << 0) |
					(FFA_RXTX_MAP_MAX_BUF_PAGE_COUNT << 16);

	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 2);

	ret = ffa_service_call(&args);
	if (ffa_func_id(ret) != FFA_SUCCESS_SMC32) {
		ERROR("FFA_FEATURES failed: %d(%d)\n", ffa_func_id(ret),
		      ffa_error_code(ret));
		return TEST_RESULT_FAIL;
	}

	param = ret.arg2;
	if (param != expected_param) {
		ERROR("Unexpected param (expected %d, got %d)\n",
		      expected_param, param);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/******************************************************************************
 * FF-A Version ABI Tests
 ******************************************************************************/

/*
 * Calls FFA Version ABI, and checks if the result as expected.
 */
static test_result_t test_ffa_version(uint32_t input_version,
					uint32_t expected_return)
{
	if (should_skip_version_test)
		return TEST_RESULT_SKIPPED;

	struct ffa_value ret_values = ffa_version(input_version);

	uint32_t spm_version = (uint32_t)(0xFFFFFFFF & ret_values.fid);

	if (spm_version == expected_return)
		return TEST_RESULT_SUCCESS;

	tftf_testcase_printf("Input Version: 0x%x\n"
			     "Return: 0x%x\nExpected: 0x%x\n",
			      input_version, spm_version, expected_return);

	return TEST_RESULT_FAIL;
}

/*
 * @Test_Aim@ Validate what happens when using same version as SPM.
 */
test_result_t test_ffa_version_equal(void)
{
	/*
	 * FFA_VERSION interface is used to check that SPM functionality is
	 * supported. On FFA_VERSION invocation from TFTF, the SPMD returns
	 * either NOT_SUPPORTED or the SPMC version value provided in the SPMC
	 * manifest. The variable "should_skip_test" is set to true when the
	 * SPMD returns NOT_SUPPORTED or a mismatched version, which means that
	 * a TFTF physical FF-A endpoint version (SPM_VERSION) does not match
	 * the SPMC's physical FF-A endpoint version. This prevents running the
	 * subsequent FF-A version tests (and break the test flow), as they're
	 * not relevant when the SPMD is not present within BL31
	 * (FFA_VERSION returns NOT_SUPPORTED).
	 */
	test_result_t ret = test_ffa_version(SPM_VERSION, SPM_VERSION);

	if (ret != TEST_RESULT_SUCCESS) {
		should_skip_version_test = true;
		ret = TEST_RESULT_SKIPPED;
	}
	return ret;
}

/*
 * @Test_Aim@ Validate what happens when setting bit 31 in
 * 'input_version'. As per spec, FFA version is 31 bits long.
 * Bit 31 set is an invalid input.
 */
test_result_t test_ffa_version_bit31(void)
{
	return test_ffa_version(FFA_VERSION_MBZ_BIT | SPM_VERSION,
				FFA_ERROR_NOT_SUPPORTED);
}

/*
 * @Test_Aim@ Validate what happens for bigger version than SPM's.
 */
test_result_t test_ffa_version_bigger(void)
{
	return test_ffa_version(make_ffa_version(2, 0),
				FFA_ERROR_NOT_SUPPORTED);
}

/*
 * @Test_Aim@ Validate what happens for smaller version than SPM's.
 */
test_result_t test_ffa_version_smaller(void)
{
	return test_ffa_version(make_ffa_version(0, 9),
				FFA_ERROR_NOT_SUPPORTED);
}

/******************************************************************************
 * FF-A RXTX ABI Tests
 ******************************************************************************/

static test_result_t test_ffa_rxtx_map(uint32_t expected_return)
{
	struct ffa_value ret;

	/**********************************************************************
	 * Verify that FFA is there and that it has the correct version.
	 **********************************************************************/
	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 0);

	/**********************************************************************
	 * If OP-TEE is SPMC skip this test.
	 **********************************************************************/
	if (check_spmc_execution_level()) {
		VERBOSE("OP-TEE as SPMC at S-EL1. Skipping test!\n");
		return TEST_RESULT_SKIPPED;
	}

	/*
	 * Declare RXTX buffers, assign them to the mailbox and call
	 * FFA_RXTX_MAP.
	 */
	CONFIGURE_AND_MAP_MAILBOX(mb, PAGE_SIZE, ret);
	if (ffa_func_id(ret) != expected_return) {
		ERROR("Failed to map RXTX buffers: %s!\n",
		      ffa_error_name(ffa_error_code(ret)));
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Test mapping RXTX buffers from NWd.
 */
test_result_t test_ffa_rxtx_map_success(void)
{
	return test_ffa_rxtx_map(FFA_SUCCESS_SMC32);
}

/**
 * Test to verify that 2nd call to FFA_RXTX_MAP should fail.
 */
test_result_t test_ffa_rxtx_map_fail(void)
{
	VERBOSE("This test expects error log.\n");
	return test_ffa_rxtx_map(FFA_ERROR);
}

/**
 * Test to verify that call to FFA_RXTX_MAP should fail when using
 * secure memory.
 */
test_result_t test_ffa_rxtx_map_secure_memory_fail(void)
{
	uintptr_t send = 0x7200000;
	uintptr_t recv = send + PAGE_SIZE;
	struct ffa_value ret;

	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 2);

	/* Unmap mailbox to remove state from previous tests. */
	reset_tftf_mailbox();

	ret = ffa_rxtx_map(send, recv, 1);
	if (!is_expected_ffa_error(ret, FFA_ERROR_DENIED))
		return TEST_RESULT_FAIL;

	return TEST_RESULT_SUCCESS;
}

/**
 * Test to verify that call to FFA_RXTX_MAP should fail when using non-secure
 * memory outside the non-secure regions specified in the SPMC manifest nodes.
 */
test_result_t test_ffa_rxtx_map_nonsecure_memory_fail(void)
{
	uintptr_t send = 0x0000880080001000;
	uintptr_t recv = send + PAGE_SIZE;
	struct ffa_value ret;

	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 2);

	/* Unmap mailbox to remove state from previous tests. */
	reset_tftf_mailbox();

	ret = ffa_rxtx_map(send, recv, 1);
	if (!is_expected_ffa_error(ret, FFA_ERROR_DENIED))
		return TEST_RESULT_FAIL;

	return TEST_RESULT_SUCCESS;
}

/**
 * Test to verify that calls to memory sharing functions should fail when the
 * ranges have been mapped by FFA_RXTX_MAP.
 */
test_result_t test_ffa_rxtx_map_memory_share_fail(void)
{

	struct ffa_memory_region_constituent constituent = {
		.page_count = 1,
		.reserved = 0,
	};
	uint32_t mem_funcs[] = {
		FFA_MEM_LEND_SMC64,
		FFA_MEM_SHARE_SMC64,
		FFA_MEM_DONATE_SMC64,
	};
	struct ffa_value ret;

	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 2);
	CONFIGURE_AND_MAP_MAILBOX(mb, PAGE_SIZE, ret);
	if (is_ffa_call_error(ret))
		return TEST_RESULT_FAIL;

	constituent.address = mb.send;

	for (uint32_t i = 0; i < ARRAY_SIZE(mem_funcs); i++) {
		uint32_t mem_func = mem_funcs[i];
		struct ffa_memory_access receiver =
			ffa_memory_access_init_permissions_from_mem_func(
				SP_ID(1), mem_func);
		memory_init_and_send(mb.send, PAGE_SIZE, HYP_ID, &receiver, 1,
				     &constituent, 1, mem_func, &ret);
		if (!is_expected_ffa_error(ret, FFA_ERROR_DENIED))
			return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Test to verify that call to FFA_RXTX_UNMAP should fail when using a
 * non-existent VM ID.
 */
test_result_t test_ffa_rxtx_unmap_nonexistent_vm_id_fail(void)
{
	struct ffa_value ret;

	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 2);
	reset_tftf_mailbox();

	ret = ffa_rxtx_map((uintptr_t)vm1_tx_buffer, (uintptr_t)vm1_rx_buffer,
			   1);
	if (is_ffa_call_error(ret))
		return TEST_RESULT_FAIL;

	ret = ffa_rxtx_unmap_with_id(HYP_ID + 1);
	if (!is_expected_ffa_error(ret, FFA_ERROR_INVALID_PARAMETER))
		return TEST_RESULT_FAIL;

	return TEST_RESULT_SUCCESS;
}

/**
 * Test to verify that a forwarded FFA_RXTX_MAP call succeeds when the RX/TX
 * regions have not already been mapped.
 */
test_result_t test_ffa_rxtx_map_forward_success(void)
{
	struct ffa_value ret;

	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 2);
	CONFIGURE_AND_MAP_MAILBOX(mb, PAGE_SIZE, ret);
	if (is_ffa_call_error(ret))
		return TEST_RESULT_FAIL;

	ret = ffa_rxtx_map_forward(mb.send, VM_ID(1), vm1_rx_buffer,
				   vm1_tx_buffer);

	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32)) {
		return TEST_RESULT_FAIL;
	}

	ret = ffa_rxtx_unmap_with_id(VM_ID(1));
	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32)) {
		return TEST_RESULT_FAIL;
	}

	ret = ffa_rxtx_unmap();
	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Test to verify that consecutive forwarding of the FFA_RXTX_MAP call succeeds
 * if using different VM IDs and different addresses.
 */
test_result_t test_ffa_rxtx_map_forward_consecutive_success(void)
{
	struct ffa_value ret;

	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 2);
	CONFIGURE_AND_MAP_MAILBOX(mb, PAGE_SIZE, ret);
	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32))
		return TEST_RESULT_FAIL;

	ret = ffa_rxtx_map_forward(mb.send, VM_ID(1), vm1_rx_buffer,
				   vm1_tx_buffer);
	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32))
		return TEST_RESULT_FAIL;

	ret = ffa_rxtx_map_forward(mb.send, VM_ID(2), vm2_rx_buffer,
				   vm2_tx_buffer);
	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32))
		return TEST_RESULT_FAIL;

	ret = ffa_rxtx_unmap_with_id(VM_ID(1));
	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32))
		return TEST_RESULT_FAIL;

	ret = ffa_rxtx_unmap_with_id(VM_ID(2));
	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32))
		return TEST_RESULT_FAIL;

	ret = ffa_rxtx_unmap();
	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32))
		return TEST_RESULT_FAIL;

	return TEST_RESULT_SUCCESS;
}

/**
 * Test to verify that forwarding of the FFA_RXTX_MAP call with the VM's buffers
 * fails if the hypervisor's RXTX buffers are not mapped.
 */
test_result_t test_ffa_rxtx_map_forward_unmapped_buffers_fail(void)
{
	struct ffa_value ret;

	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 2);
	/*
	 * Unmap mailbox to recreate case where hypervisor's buffers aren't
	 * mapped.
	 */
	reset_tftf_mailbox();

	ret = ffa_rxtx_map_forward(mb.send, VM_ID(1), vm1_rx_buffer,
				   vm1_tx_buffer);
	if (!is_expected_ffa_error(ret, FFA_ERROR_INVALID_PARAMETER))
		return TEST_RESULT_FAIL;

	return TEST_RESULT_SUCCESS;
}

/**
 * Test to verify that FFA_RXTX_MAP forwarding fails if trying to forward
 * buffers that have already been forwarded.
 */
test_result_t test_ffa_rxtx_map_forward_different_ids_fail(void)
{
	struct ffa_value ret;

	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 2);

	CONFIGURE_AND_MAP_MAILBOX(mb, PAGE_SIZE, ret);
	if (is_ffa_call_error(ret))
		return TEST_RESULT_FAIL;

	ret = ffa_rxtx_map_forward(mb.send, VM_ID(1), vm1_rx_buffer,
				   vm1_tx_buffer);
	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32))
		return TEST_RESULT_FAIL;

	ret = ffa_rxtx_map_forward(mb.send, VM_ID(2), vm1_rx_buffer,
				   vm1_tx_buffer);
	if (!is_expected_ffa_error(ret, FFA_ERROR_DENIED))
		return TEST_RESULT_FAIL;

	ret = ffa_rxtx_unmap_with_id(VM_ID(1));
	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32))
		return TEST_RESULT_FAIL;

	return TEST_RESULT_SUCCESS;
}

/**
 * Test to verify that calls to memory sharing functions should fail when the
 * ranges have been mapped by a forwarded FFA_RXTX_MAP.
 */
test_result_t test_ffa_rxtx_map_forward_memory_share_fail(void)
{

	struct ffa_memory_region_constituent constituent = {
		.page_count = 1,
		.reserved = 0,
		.address = vm1_tx_buffer,
	};
	uint32_t mem_funcs[] = {
		FFA_MEM_LEND_SMC64,
		FFA_MEM_SHARE_SMC64,
		FFA_MEM_DONATE_SMC64,
	};
	struct ffa_value ret;

	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 2);

	ret = ffa_rxtx_map_forward(mb.send, VM_ID(1), vm1_rx_buffer,
				   vm1_tx_buffer);
	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32))
		return TEST_RESULT_FAIL;

	for (uint32_t i = 0; i < ARRAY_SIZE(mem_funcs); i++) {
		uint32_t mem_func = mem_funcs[i];
		struct ffa_memory_access receiver =
			ffa_memory_access_init_permissions_from_mem_func(
				SP_ID(1), mem_func);
		memory_init_and_send(mb.send, PAGE_SIZE, HYP_ID, &receiver, 1,
				     &constituent, 1, mem_func, &ret);
		if (!is_expected_ffa_error(ret, FFA_ERROR_DENIED))
			return TEST_RESULT_FAIL;
	}

	ret = ffa_rxtx_unmap_with_id(VM_ID(1));
	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32))
		return TEST_RESULT_FAIL;

	ret = ffa_rxtx_unmap();
	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32))
		return TEST_RESULT_FAIL;

	return TEST_RESULT_SUCCESS;
}

static test_result_t test_ffa_rxtx_unmap(uint32_t expected_return)
{
	struct ffa_value ret;

	/**********************************************************************
	 * Verify that FFA is there and that it has the correct version.
	 **********************************************************************/
	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 0);

	/**********************************************************************
	 * If OP-TEE is SPMC skip this test.
	 **********************************************************************/
	if (check_spmc_execution_level()) {
		VERBOSE("OP-TEE as SPMC at S-EL1. Skipping test!\n");
		return TEST_RESULT_SKIPPED;
	}

	ret = ffa_rxtx_unmap();
	if (!is_expected_ffa_return(ret, expected_return)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Test unmapping RXTX buffers from NWd.
 */
test_result_t test_ffa_rxtx_unmap_success(void)
{
	return test_ffa_rxtx_unmap(FFA_SUCCESS_SMC32);
}

/**
 * Test to verify that 2nd call to FFA_RXTX_UNMAP should fail.
 */
test_result_t test_ffa_rxtx_unmap_fail(void)
{
	VERBOSE("This test expects error log.\n");
	return test_ffa_rxtx_unmap(FFA_ERROR);
}

/**
 * Test mapping RXTX buffers that have been previously unmapped from NWd.
 */
test_result_t test_ffa_rxtx_map_unmapped_success(void)
{
	test_result_t ret =  test_ffa_rxtx_map(FFA_SUCCESS_SMC32);
	/*
	 * Unmapping buffers such that further tests can map and use RXTX
	 * buffers.
	 * Subsequent attempts to map the RXTX buffers will fail, if this is
	 * invoked at this point.
	 */
	ffa_rxtx_unmap();
	return ret;
}

/*
 * The FFA_RXTX_UNMAP specification at the NS physical FF-A instance allows for
 * an ID to be given to the SPMC. The ID should relate to a VM that had its ID
 * previously forwarded to the SPMC.
 * This test validates that calls to FFA_RXTX_UNMAP from the NS physical
 * instance can't unmap RXTX buffer pair of an SP.
 */
test_result_t test_ffa_rxtx_unmap_fail_if_sp(void)
{
	struct ffa_value ret;

	CHECK_SPMC_TESTING_SETUP(1, 1, sp_uuids);

	/* Invoked FFA_RXTX_UNMAP, providing the ID of an SP in w1. */
	ret = ffa_rxtx_unmap_with_id(SP_ID(1));

	if (!is_expected_ffa_error(ret, FFA_ERROR_INVALID_PARAMETER)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/******************************************************************************
 * FF-A SPM_ID_GET ABI Tests
 ******************************************************************************/

test_result_t test_ffa_spm_id_get(void)
{
	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 1);

	struct ffa_value ffa_ret = ffa_spm_id_get();

	if (is_ffa_call_error(ffa_ret)) {
		ERROR("FFA_SPM_ID_GET call failed! Error code: %s\n",
		      ffa_error_name(ffa_error_code(ffa_ret)));
		return TEST_RESULT_FAIL;
	}

	/* Check the SPMC value given in the fvp_spmc_manifest is returned */
	ffa_id_t spm_id = ffa_endpoint_id(ffa_ret);

	if (spm_id != SPMC_ID) {
		ERROR("Expected SPMC_ID of 0x%x\n received: 0x%x\n",
			SPMC_ID, spm_id);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/******************************************************************************
 * FF-A PARTITION_INFO_GET ABI Tests
 ******************************************************************************/

/**
 * Attempt to get the SP partition information for individual partitions as well
 * as all secure partitions.
 */
test_result_t test_ffa_partition_info(void)
{
	/***********************************************************************
	 * Check if SPMC has ffa_version and expected FFA endpoints are deployed.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 1, sp_uuids);

	GET_TFTF_MAILBOX(mb);

	if (!ffa_partition_info_helper(&mb, sp_uuids[0],
		&ffa_expected_partition_info[0], 1)) {
		return TEST_RESULT_FAIL;
	}
	if (!ffa_partition_info_helper(&mb, sp_uuids[1],
		&ffa_expected_partition_info[1], 1)) {
		return TEST_RESULT_FAIL;
	}
	if (!ffa_partition_info_helper(&mb, sp_uuids[2],
		&ffa_expected_partition_info[2], 1)) {
		return TEST_RESULT_FAIL;
	}
	if (!ffa_partition_info_helper(&mb, NULL_UUID,
		ffa_expected_partition_info,
		ARRAY_SIZE(ffa_expected_partition_info))) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Attempt to get v1.0 partition info descriptors.
 */
test_result_t test_ffa_partition_info_v1_0(void)
{
	/**************************************************************
	 * Check if SPMC has ffa_version and expected FFA endpoints
	 * are deployed.
	 *************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 0, sp_uuids);

	GET_TFTF_MAILBOX(mb);

	test_result_t result = TEST_RESULT_SUCCESS;
	struct ffa_value ret = ffa_partition_info_get(NULL_UUID);
	uint64_t expected_size = ARRAY_SIZE(ffa_expected_partition_info);

	if (ffa_func_id(ret) == FFA_SUCCESS_SMC32) {
		if (ffa_partition_info_count(ret) != expected_size) {
			ERROR("Unexpected number of partitions %d\n",
			      ffa_partition_info_count(ret));
			return TEST_RESULT_FAIL;
		}
		if (ffa_partition_info_desc_size(ret) !=
		    sizeof(struct ffa_partition_info_v1_0)) {
			ERROR("Unexepcted partition info descriptor size %d\n",
			      ffa_partition_info_desc_size(ret));
			return TEST_RESULT_FAIL;
		}
		const struct ffa_partition_info_v1_0 *info =
			(const struct ffa_partition_info_v1_0 *)(mb.recv);

		for (unsigned int i = 0U; i < expected_size; i++) {
			uint32_t expected_properties_v1_0 =
				ffa_expected_partition_info[i].properties &
				~FFA_PARTITION_v1_0_RES_MASK;

			if (info[i].id != ffa_expected_partition_info[i].id) {
				ERROR("Wrong ID. Expected %x, got %x\n",
				      ffa_expected_partition_info[i].id,
				      info[i].id);
				result = TEST_RESULT_FAIL;
			}
			if (info[i].exec_context !=
			    ffa_expected_partition_info[i].exec_context) {
				ERROR("Wrong context. Expected %d, got %d\n",
				      ffa_expected_partition_info[i].exec_context,
				      info[i].exec_context);
				result = TEST_RESULT_FAIL;
			}
			if (info[i].properties !=
			    expected_properties_v1_0) {
				ERROR("Wrong properties. Expected %d, got %d\n",
				      expected_properties_v1_0,
				      info[i].properties);
				result = TEST_RESULT_FAIL;
			}
		}
	}

	ret = ffa_rx_release();
	if (is_ffa_call_error(ret)) {
		ERROR("Failed to release RX buffer\n");
		result = TEST_RESULT_FAIL;
	}
	return result;
}
