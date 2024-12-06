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
#include <host_realm_helper.h>
#include <platform.h>
#include <spm_test_helpers.h>
#include <test_helpers.h>

static const struct ffa_uuid expected_sp_uuids[] = {
		{PRIMARY_UUID},
	};

const char expected_msg[] = "Testing FF-A message.";

/*
 *
 * Used as the RX/TX buffers belonging to VM 1 in the forwarding FFA_RXTX_MAP
 * tests.
 */
static __aligned(PAGE_SIZE) uint8_t vm1_rx_buffer[PAGE_SIZE];
static __aligned(PAGE_SIZE) uint8_t vm1_tx_buffer[PAGE_SIZE];

static int schedule_receiver_interrupt_handler(void *data)
{
	return 0;
}

test_result_t test_ffa_indirect_message_sp_to_vm(void)
{
	struct ffa_value ret;
	struct mailbox_buffers mb;
	ffa_id_t header_sender;
	const ffa_id_t vm_id = VM_ID(1);
	const ffa_id_t sender = SP_ID(1);
	char msg[300];

	/**********************************************************************
	 * Check SPMC has ffa_version and expected FF-A endpoints are deployed.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

	GET_TFTF_MAILBOX(mb);

	tftf_irq_register_handler(FFA_SCHEDULE_RECEIVER_INTERRUPT_ID,
				  schedule_receiver_interrupt_handler);

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

	/*
	 * Managed exit occured while SP was running and it is left in its
	 * fiq handler, resume the SP to reach the FF-A message loop again.
	 */
	ret = cactus_resume_after_managed_exit(HYP_ID, SPM_VM_ID_FIRST);
	if (!is_ffa_direct_response(ret)) {
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

	tftf_irq_unregister_handler(FFA_SCHEDULE_RECEIVER_INTERRUPT_ID);

	return TEST_RESULT_SUCCESS;
}

/**
 * Test message sent from SP to VM when VM's RX is realm, the operation fails
 * smoothly.
 */
test_result_t test_ffa_indirect_message_sp_to_vm_rx_realm_fail(void)
{
	struct ffa_value ret;
	struct mailbox_buffers mb;
	const ffa_id_t vm_id = VM_ID(1);
	const ffa_id_t sender = SP_ID(1);
	ffa_id_t header_sender;
	u_register_t ret_rmm;
	char msg[300];

	if (get_armv9_2_feat_rme_support() == 0U) {
		return TEST_RESULT_SKIPPED;
	}

	/**********************************************************************
	 * Check SPMC has ffa_version and expected FF-A endpoints are deployed.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

	GET_TFTF_MAILBOX(mb);

	/* Map RXTX buffers into SPMC translation. */
	ret = ffa_rxtx_map_forward(mb.send, vm_id, vm1_rx_buffer,
				   vm1_tx_buffer);

	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32)) {
		ERROR("Failed to map buffers RX %p TX %p for VM %x\n",
		      vm1_rx_buffer, vm1_tx_buffer, vm_id);
		return TEST_RESULT_FAIL;
	}

	/*
	 * Create bitmap only to then demonstrate that the message can't
	 * be sent, as there are no pending notifications to the VM.
	 */
	ret = ffa_notification_bitmap_create(vm_id, PLATFORM_CORE_COUNT);

	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32)) {
		ERROR("Failed to create bitmap for vm %x\n", vm_id);
		return TEST_RESULT_FAIL;
	}

	tftf_irq_register_handler(FFA_SCHEDULE_RECEIVER_INTERRUPT_ID,
				  schedule_receiver_interrupt_handler);

	/*
	 * Delegate RX buffer of VM to realm.
	 */
	ret_rmm = host_rmi_granule_delegate((u_register_t)vm1_rx_buffer);

	if (ret_rmm != 0UL) {
		INFO("Delegate operation returns %#lx for address %p\n",
		     ret_rmm, mb.send);
		return TEST_RESULT_FAIL;
	}

	/* Request SP to send message. */
	ret = cactus_req_ind_msg_send_cmd(
			HYP_ID, sender, vm_id, sender, 0);

	if (!is_ffa_direct_response(ret) &&
	    cactus_get_response(ret) != FFA_ERROR_ABORTED) {
		return TEST_RESULT_FAIL;
	}

	/* Undelegate to reestablish the same security state for PAS. */
	ret_rmm = host_rmi_granule_undelegate((u_register_t)vm1_rx_buffer);

	if (ret_rmm != 0UL) {
		INFO("Undelegate operation returns %#lx for address %p\n",
		     ret_rmm, mb.send);
		return TEST_RESULT_FAIL;
	}

	/* Expect that attempting to receive message shall fail. */
	if (receive_indirect_message(&msg, ARRAY_SIZE(msg), (void *)vm1_rx_buffer,
				     NULL, vm_id, 0)) {
		return TEST_RESULT_FAIL;
	}

	/*
	 * Redo the test so we check after undelegating the memory.
	 * After undelegating, the SPMC should be able to complete the
	 * operation.
	 */
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

	/*
	 * Managed exit occured while SP was running and it is left in its
	 * fiq handler, resume the SP to reach the FF-A message loop again.
	 */
	ret = cactus_resume_after_managed_exit(HYP_ID, SPM_VM_ID_FIRST);
	if (!is_ffa_direct_response(ret)) {
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

	/* Cleaning up after the test: */
	tftf_irq_unregister_handler(FFA_SCHEDULE_RECEIVER_INTERRUPT_ID);

	/* Destroy bitmap of VM. */
	ret = ffa_notification_bitmap_destroy(vm_id);

	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32)) {
		ERROR("Failed to destroy bitmap for vm %x\n", vm_id);
		return TEST_RESULT_FAIL;
	}

	/* Unmap RXTX buffers into SPMC translation. */
	ret = ffa_rxtx_unmap_with_id(vm_id);

	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32)) {
		ERROR("Failed to unmap RXTX for vm %x\n", vm_id);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Test message sent from VM to SP, when VM TX is in realm PAS, the operation
 * fails smoothly.
 */
test_result_t test_ffa_indirect_message_vm_to_sp_tx_realm_fail(void)
{
	struct ffa_value ret;
	struct mailbox_buffers mb;
	const ffa_id_t vm_id = 1;
	const ffa_id_t receiver = SP_ID(1);
	u_register_t ret_rmm;
	char payload[] = "Poisonous...";
	struct ffa_partition_msg *message = (struct ffa_partition_msg *)vm1_tx_buffer;

	if (get_armv9_2_feat_rme_support() == 0U) {
		return TEST_RESULT_SKIPPED;
	}

	/**********************************************************************
	 * Check SPMC has ffa_version and expected FF-A endpoints are deployed.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

	GET_TFTF_MAILBOX(mb);

	/* Map RXTX buffers into SPMC translation. */
	ret = ffa_rxtx_map_forward(mb.send, vm_id, vm1_rx_buffer,
				   vm1_tx_buffer);

	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32)) {
		ERROR("Failed to map buffers RX %p TX %p for VM %x\n",
		      vm1_rx_buffer, vm1_tx_buffer, vm_id);
		return TEST_RESULT_FAIL;
	}

	/* Initialize message header. */
	ffa_rxtx_header_init(vm_id, receiver, ARRAY_SIZE(payload), &message->header);

	/* Fill TX buffer with payload. */
	memcpy(message->payload, payload, ARRAY_SIZE(payload));

	/* Delegate TX buffer of VM to realm PAS. */
	ret_rmm = host_rmi_granule_delegate((u_register_t)vm1_tx_buffer);

	if (ret_rmm != 0UL) {
		INFO("Delegate operation returns %#lx for address %p\n",
		     ret_rmm, mb.send);
		return TEST_RESULT_FAIL;
	}

	/* Expect that attempting to send message shall fail. */
	ret = ffa_msg_send2_with_id(0, vm_id);

	if (!is_expected_ffa_error(ret, FFA_ERROR_ABORTED)) {
		return TEST_RESULT_FAIL;
	}

	/* Undelegate to reestablish the same security state for PAS. */
	ret_rmm = host_rmi_granule_undelegate((u_register_t)vm1_tx_buffer);

	if (ret_rmm != 0UL) {
		INFO("Undelegate operation returns %#lx for address %p\n",
		     ret_rmm, mb.send);
		return TEST_RESULT_FAIL;
	}

	/* Unmap RXTX buffers into SPMC translation. */
	ret = ffa_rxtx_unmap_with_id(vm_id);

	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32)) {
		ERROR("Failed to unmap RXTX for vm %x\n", vm_id);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
