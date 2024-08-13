/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <cactus_test_cmds.h>
#include "ffa_helpers.h"
#include "tftf_lib.h"
#include <debug.h>
#include <ffa_endpoints.h>
#include <ffa_svc.h>
#include <host_realm_helper.h>
#include <irq.h>
#include <platform.h>
#include <smccc.h>
#include <spm_common.h>
#include <spm_test_helpers.h>
#include <test_helpers.h>

#define SENDER HYP_ID
#define RECEIVER SP_ID(1)

static __aligned(PAGE_SIZE) uint64_t share_page[PAGE_SIZE / sizeof(uint64_t)];

static const struct ffa_uuid expected_sp_uuids[] = {
		{PRIMARY_UUID}, {SECONDARY_UUID}, {TERTIARY_UUID}
};

/**
 * @Test_Aim@ Check a realm region cannot be accessed from a secure partition.
 *
 * This test shares a TFTF allocated buffer with a secure partition through
 * FF-A memory sharing operation. The buffer is initially marked NS in the GPT
 * and transitioned to realm after sharing. Then, the SP is invoked to retrieve
 * the region (map it to its S2 translation regime), and maps it to its secure
 * S1 translation regime. It then attempts a read access which results in the
 * PE triggering a GPF caught by a custom synchronous abort handler.
 *
 */
test_result_t rl_memory_cannot_be_accessed_in_s(void)
{
	struct ffa_memory_region_constituent constituents[] = {
		{
			(void *)share_page, 1, 0
		}
	};
	const uint32_t constituents_count = sizeof(constituents) /
		sizeof(struct ffa_memory_region_constituent);
	ffa_memory_handle_t handle;
	struct mailbox_buffers mb;
	struct ffa_value ret;
	u_register_t retmm;

	struct ffa_memory_access receiver =
		ffa_memory_access_init_permissions_from_mem_func(
			RECEIVER, FFA_MEM_SHARE_SMC64);

	if (get_armv9_2_feat_rme_support() == 0U) {
		return TEST_RESULT_SKIPPED;
	}

	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

	GET_TFTF_MAILBOX(mb);

	handle = memory_init_and_send(mb.send, PAGE_SIZE, SENDER, &receiver, 1,
				   constituents, constituents_count,
				   FFA_MEM_SHARE_SMC64, &ret);

	if (handle == FFA_MEMORY_HANDLE_INVALID) {
		return TEST_RESULT_FAIL;
	}

	VERBOSE("TFTF - Handle: %llx Address: %p\n",
		handle, constituents[0].address);

	host_rmi_init_cmp_result();

	/* Delegate the shared page to Realm. */
	retmm = host_rmi_granule_delegate((u_register_t)&share_page);
	if (retmm != 0UL) {
		ERROR("Granule delegate failed, ret=0x%lx\n", retmm);
		return TEST_RESULT_FAIL;
	}

	/*
	 * Retrieve the shared page and attempt accessing it.
	 * Tell SP to expect an exception.
	 */
	ret = cactus_mem_send_cmd(SENDER, RECEIVER, FFA_MEM_SHARE_SMC64,
				  handle, 0, 1, true, true);

	/* Undelegate the shared page. */
	retmm = host_rmi_granule_undelegate((u_register_t)&share_page);
	if (retmm != 0UL) {
		ERROR("Granule undelegate failed, ret=0x%lx\n", retmm);
		return TEST_RESULT_FAIL;
	}

	if (is_ffa_call_error(ffa_mem_reclaim(handle, 0))) {
		ERROR("Memory reclaim failed!\n");
		return TEST_RESULT_FAIL;
	}

	/*
	 * Expect success response with value 1 hinting an exception
	 * triggered while the SP accessed the region.
	 */
	if (!(cactus_get_response(ret) == CACTUS_SUCCESS &&
	      cactus_error_code(ret) == 1)) {
		ERROR("Exceptions test failed!\n");
		return TEST_RESULT_FAIL;
	}

	return host_cmp_result();
}

/**
 * Map the NS RXTX buffers to the SPM, invoke the TRP delegation
 * service to change the RX buffer PAS to realm, invoke the
 * FFA_PARTITON_INFO_GET interface, such that SPM does NS access
 * to realm region and triggers GPF.
 */
test_result_t test_ffa_rxtx_to_realm_pas(void)
{
	struct mailbox_buffers mb;
	u_register_t retmm;
	struct ffa_value ret;

	GET_TFTF_MAILBOX(mb);

	if (get_armv9_2_feat_rme_support() == 0U) {
		return TEST_RESULT_SKIPPED;
	}

	/***********************************************************************
	 * Check if SPMC has ffa_version and expected FFA endpoints are deployed.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

	/* Delegate the shared page to Realm. */
	retmm = host_rmi_granule_delegate((u_register_t)mb.recv);
	if (retmm != 0UL) {
		ERROR("Granule delegate failed, ret=0x%lx\n", retmm);
		return TEST_RESULT_FAIL;
	}

	ret = ffa_partition_info_get((struct ffa_uuid){{0}});

	if (!is_expected_ffa_error(ret, FFA_ERROR_ABORTED)) {
		ERROR("FFA_PARTITON_INFO_GET should terminate with"
		       "FFA_ERROR_ABORTED\n");
		return TEST_RESULT_FAIL;
	}

	/* Undelegate the shared page. */
	retmm = host_rmi_granule_undelegate((u_register_t)mb.recv);
	if (retmm != 0UL) {
		ERROR("Granule undelegate failed, ret=0x%lx\n", retmm);
		return TEST_RESULT_FAIL;
	}

	ret = ffa_partition_info_get((struct ffa_uuid){{0}});

	if (is_ffa_call_error(ret)) {
		ERROR("FFA_PARTITON_INFO_GET should succeed.\n");
		return TEST_RESULT_FAIL;
	}

	ffa_rx_release();

	return TEST_RESULT_SUCCESS;
}
