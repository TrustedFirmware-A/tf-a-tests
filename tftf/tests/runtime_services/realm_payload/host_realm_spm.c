/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>

#include <cactus_test_cmds.h>
#include <ffa_endpoints.h>
#include <ffa_helpers.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_shared_data.h>
#include <test_helpers.h>

#define REALM_TIME_SLEEP	300U
#define SENDER HYP_ID
#define RECEIVER SP_ID(1)
static const struct ffa_uuid expected_sp_uuids[] = { {PRIMARY_UUID} };
static struct mailbox_buffers mb;
static bool secure_mailbox_initialised;

/*
 * This function helps to Initialise secure_mailbox, creates realm payload and
 * shared memory to be used between Host and Realm.
 * Skip test if RME is not supported or not the right RMM version is begin used
 */
static test_result_t init_test(void)
{
	u_register_t retrmm;


	/* Verify that FFA is there and that it has the correct version. */
	SKIP_TEST_IF_AARCH32();
	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 1);

	if (!secure_mailbox_initialised) {
		GET_TFTF_MAILBOX(mb);
		CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);
		secure_mailbox_initialised = true;
	}

	if (get_armv9_2_feat_rme_support() == 0U) {
		return TEST_RESULT_SKIPPED;
	}

	retrmm = host_rmi_version();
	VERBOSE("RMM version is: %lu.%lu\n",
			RMI_ABI_VERSION_GET_MAJOR(retrmm),
			RMI_ABI_VERSION_GET_MINOR(retrmm));
	/*
	 * Skip the test if RMM is TRP, TRP version is always null.
	 */
	if (retrmm == 0UL) {
		return TEST_RESULT_SKIPPED;
	}

	/*
	 * Initialise Realm payload
	 */
	if (!host_create_realm_payload((u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)(PAGE_POOL_MAX_SIZE +
			NS_REALM_SHARED_MEM_SIZE),
			(u_register_t)PAGE_POOL_MAX_SIZE, 0UL)) {
		return TEST_RESULT_FAIL;
	}

	/*
	 * Create shared memory between Host and Realm
	 */
	if (!host_create_shared_mem(NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

static bool host_realm_handle_fiq_exit(struct realm *realm_ptr)
{
	struct rmi_rec_run *run = (struct rmi_rec_run *)realm_ptr->run;
	if (run->exit.exit_reason == RMI_EXIT_FIQ) {
		return true;
	}
	return false;
}

/*
 * @Test_Aim@ Test secure interrupt handling while Secure Partition is in waiting
 * state and Realm world runs a busy loop at R-EL1.
 *
 * 1. Send a direct message request command to first Cactus SP to start the
 *    trusted watchdog timer.
 *
 * 2. Once the SP returns with a direct response message, it moves to WAITING
 *    state.
 *
 * 3. Create and execute a busy loop to sleep the PE in the realm world for
 *    REALM_TIME_SLEEP ms.
 *
 * 4. Trusted watchdog timer expires during this time which leads to secure
 *    interrupt being triggered while cpu is executing in realm world.
 *
 * 5. Realm EL1 exits to host, but because the FIQ is still pending,
 *    the Host will be pre-empted to EL3.
 *
 * 6. The interrupt is trapped to BL31/SPMD as FIQ and later synchronously
 *    delivered to SPM.
 *
 * 7. SPM injects a virtual IRQ to first Cactus Secure Partition.
 *
 * 8. Once the SP has handled the interrupt, it returns execution back to normal
 *    world using FFA_MSG_WAIT call.
 *
 * 9. TFTF parses REC's exit reason (FIQ in this case).
 *
 * 10. TFTF sends a direct request message to SP to query the ID of last serviced
 *     secure virtual interrupt.
 *
 * 121. Further, TFTF expects SP to return the ID of Trusted Watchdog timer
 *     interrupt through a direct response message.
 *
 * 13. Test finishes successfully once the TFTF disables the trusted watchdog
 *     interrupt through a direct message request command.
 *
 * 14. TFTF then proceed to destroy the Realm.
 *
 */
test_result_t host_realm_sec_interrupt_can_preempt_rl(void)
{
	struct realm *realm_ptr;
	struct ffa_value ret_values;
	test_result_t res;

	res = init_test();
	if (res != TEST_RESULT_SUCCESS) {
		return res;
	}

	/* Enable trusted watchdog interrupt as IRQ in the secure side. */
	if (!enable_trusted_wdog_interrupt(SENDER, RECEIVER)) {
		return TEST_RESULT_FAIL;
	}

	/*
	 * Send a message to SP1 through direct messaging.
	 */
	ret_values = cactus_send_twdog_cmd(SENDER, RECEIVER, (REALM_TIME_SLEEP/2));

	if (!is_ffa_direct_response(ret_values)) {
		ERROR("Expected a direct response for starting TWDOG timer\n");
		return TEST_RESULT_FAIL;
	}

	/*
	 * Spin Realm payload for REALM_TIME_SLEEP ms, This ensures secure wdog
	 * timer triggers during this time.
	 */
	realm_shared_data_set_host_val(HOST_SLEEP_INDEX, REALM_TIME_SLEEP);
	host_enter_realm_execute(REALM_SLEEP_CMD, &realm_ptr, RMI_EXIT_FIQ);

	/*
	 * Check if Realm exit reason is FIQ.
	 */
	if (!host_realm_handle_fiq_exit(realm_ptr)) {
		ERROR("Trusted watchdog timer interrupt not fired\n");
		host_destroy_realm();
		return TEST_RESULT_FAIL;
	}

	/* Check for the last serviced secure virtual interrupt. */
	ret_values = cactus_get_last_interrupt_cmd(SENDER, RECEIVER);

	if (!is_ffa_direct_response(ret_values)) {
		ERROR("Expected a direct response for last serviced interrupt"
			" command\n");
		return TEST_RESULT_FAIL;
	}

	/* Make sure Trusted Watchdog timer interrupt was serviced*/
	if (cactus_get_response(ret_values) != IRQ_TWDOG_INTID) {
		ERROR("Trusted watchdog timer interrupt not serviced by SP\n");
		return TEST_RESULT_FAIL;
	}

	/* Disable Trusted Watchdog interrupt. */
	if (!disable_trusted_wdog_interrupt(SENDER, RECEIVER)) {
		return TEST_RESULT_FAIL;
	}

	if (!host_destroy_realm()) {
		ERROR("host_destroy_realm error\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
