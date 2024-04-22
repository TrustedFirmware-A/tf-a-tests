/*
 * Copyright (c) 2024 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "ffa_helpers.h"
#include "spm_common.h"
#include "tftf_lib.h"
#include <debug.h>
#include <smccc.h>

#include <arch_helpers.h>
#include <cactus_test_cmds.h>
#include <ffa_endpoints.h>
#include <ffa_svc.h>
#include <platform.h>
#include <spm_test_helpers.h>
#include <test_helpers.h>

static const struct ffa_uuid expected_sp_uuids[] = {
		{PRIMARY_UUID},
	};

/*
 *
 * Used as the RX/TX buffers belonging to VM 1 in the forwarding FFA_RXTX_MAP
 * tests.
 */
static __aligned(PAGE_SIZE) uint8_t vm1_rx_buffer[PAGE_SIZE];
static __aligned(PAGE_SIZE) uint8_t vm1_tx_buffer[PAGE_SIZE];

test_result_t test_ffa_indirect_message_sp_to_vm(void)
{
	struct ffa_value ret;
	struct mailbox_buffers mb;
	ffa_id_t header_sender;
	const ffa_id_t vm_id = 1;
	const ffa_id_t sender = SP_ID(1);
	const char expected_msg[] = "Testing FF-A message.";
	char msg[300];

	/**********************************************************************
	 * Check SPMC has ffa_version and expected FF-A endpoints are deployed.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

	GET_TFTF_MAILBOX(mb);

	ret = ffa_rxtx_map_forward(mb.send, vm_id, vm1_rx_buffer,
				   vm1_tx_buffer);

	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32)) {
		ERROR("Failed to map buffers RX %p TX %p for VM %x\n",
		      vm1_tx_buffer, vm1_tx_buffer, vm_id);
		return TEST_RESULT_FAIL;
	}

	ret = ffa_notification_bitmap_create(vm_id, PLATFORM_CORE_COUNT);

	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32)) {
		ERROR("Failed to create bitmap for vm %x\n", vm_id);
		return TEST_RESULT_FAIL;
	}

	/* Request SP to send message. */
	ret = cactus_req_ind_msg_send_cmd(
			HYP_ID, sender, vm_id, sender, 0);

	if (!is_ffa_direct_response(ret) &&
	    cactus_get_response(ret) != CACTUS_SUCCESS) {
		return TEST_RESULT_FAIL;
	}

	if (!receive_indirect_message(&msg, ARRAY_SIZE(msg), (void *)vm1_rx_buffer,
				      &header_sender, vm_id, 0)) {
		return TEST_RESULT_FAIL;
	}

	if (strncmp(msg, expected_msg, ARRAY_SIZE(expected_msg)) != 0) {
		ERROR("Unexpected message: %s, expected: %s\n", msg, expected_msg);
		return TEST_RESULT_FAIL;
	}

	if (header_sender != sender) {
		ERROR("Unexpected endpoints. Sender: %x Expected: %x\n",
		      header_sender, sender);
		return TEST_RESULT_FAIL;
	}

	ret = ffa_notification_bitmap_destroy(vm_id);
	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32)) {
		return TEST_RESULT_FAIL;
	}

	ret = ffa_rxtx_unmap_with_id(vm_id);
	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
