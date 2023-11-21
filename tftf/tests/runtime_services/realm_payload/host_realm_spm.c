/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>

#include <cactus_test_cmds.h>
#include <ffa_endpoints.h>
#include <ffa_helpers.h>
#include <fpu.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_shared_data.h>
#include <spm_test_helpers.h>
#include <test_helpers.h>

#define REALM_TIME_SLEEP	300U
#define SENDER HYP_ID
#define RECEIVER SP_ID(1)
static const struct ffa_uuid expected_sp_uuids[] = { {PRIMARY_UUID} };
static struct mailbox_buffers mb;
static bool secure_mailbox_initialised;

static fpu_state_t ns_fpu_state_write;
static fpu_state_t ns_fpu_state_read;
static struct realm realm;

typedef enum security_state {
	NONSECURE_WORLD = 0U,
	REALM_WORLD,
	SECURE_WORLD,
	SECURITY_STATE_MAX
} security_state_t;

/*
 * This function helps to Initialise secure_mailbox, creates realm payload and
 * shared memory to be used between Host and Realm.
 * Skip test if RME is not supported or not the right RMM version is begin used
 */
static test_result_t init_sp(void)
{
	if (!secure_mailbox_initialised) {
		GET_TFTF_MAILBOX(mb);
		CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);
		secure_mailbox_initialised = true;
	}
	return TEST_RESULT_SUCCESS;
}

static test_result_t init_realm(void)
{
	u_register_t rec_flag[1] = {RMI_RUNNABLE};

	/*
	 * Initialise Realm payload
	 */
	if (!host_create_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)(PAGE_POOL_MAX_SIZE +
			NS_REALM_SHARED_MEM_SIZE),
			(u_register_t)PAGE_POOL_MAX_SIZE, 0UL, rec_flag, 1U)) {
		return TEST_RESULT_FAIL;
	}

	/*
	 * Create shared memory between Host and Realm
	 */
	if (!host_create_shared_mem(&realm, NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

static bool host_realm_handle_fiq_exit(struct realm *realm_ptr,
		unsigned int rec_num)
{
	struct rmi_rec_run *run = (struct rmi_rec_run *)realm_ptr->run[rec_num];
	if (run->exit.exit_reason == RMI_EXIT_FIQ) {
		return true;
	}
	return false;
}

/* Send request to SP to fill FPU/SIMD regs with secure template values */
static bool fpu_fill_sec(void)
{
	struct ffa_value ret = cactus_req_simd_fill_send_cmd(SENDER, RECEIVER);

	if (!is_ffa_direct_response(ret)) {
		ERROR("%s failed %d\n", __func__, __LINE__);
		return false;
	}
	if (cactus_get_response(ret) == CACTUS_ERROR) {
		ERROR("%s failed %d\n", __func__, __LINE__);
		return false;
	}
	return true;
}

/* Send request to SP to compare FPU/SIMD regs with secure template values */
static bool fpu_cmp_sec(void)
{
	struct ffa_value ret = cactus_req_simd_compare_send_cmd(SENDER, RECEIVER);

	if (!is_ffa_direct_response(ret)) {
		ERROR("%s failed %d\n", __func__, __LINE__);
		return false;
	}
	if (cactus_get_response(ret) == CACTUS_ERROR) {
		ERROR("%s failed %d\n", __func__, __LINE__);
		return false;
	}
	return true;
}


/* Send request to Realm to fill FPU/SIMD regs with realm template values */
static bool fpu_fill_rl(void)
{
	if (!host_enter_realm_execute(&realm, REALM_REQ_FPU_FILL_CMD, RMI_EXIT_HOST_CALL, 0U)) {
		ERROR("%s failed %d\n", __func__, __LINE__);
		return false;
	}
	return true;
}

/* Send request to Realm to compare FPU/SIMD regs with previous realm template values */
static bool fpu_cmp_rl(void)
{
	if (!host_enter_realm_execute(&realm, REALM_REQ_FPU_CMP_CMD, RMI_EXIT_HOST_CALL, 0U)) {
		ERROR("%s failed %d\n", __func__, __LINE__);
		return false;
	}
	return true;
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
	struct ffa_value ret_values;
	test_result_t res;

	/* Verify RME is present and RMM is not TRP */
	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	/* Verify that FFA is there and that it has the correct version. */
	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 1);

	res = init_sp();
	if (res != TEST_RESULT_SUCCESS) {
		return res;
	}

	res = init_realm();
	if (res != TEST_RESULT_SUCCESS) {
		return res;
	}

	/* Enable trusted watchdog interrupt as IRQ in the secure side. */
	if (!enable_trusted_wdog_interrupt(SENDER, RECEIVER)) {
		goto destroy_realm;
	}

	/*
	 * Send a message to SP1 through direct messaging.
	 */
	ret_values = cactus_send_twdog_cmd(SENDER, RECEIVER,
					   (REALM_TIME_SLEEP/2));
	if (!is_ffa_direct_response(ret_values)) {
		ERROR("Expected a direct response for starting TWDOG timer\n");
		goto destroy_realm;
	}

	/*
	 * Spin Realm payload for REALM_TIME_SLEEP ms, This ensures secure wdog
	 * timer triggers during this time.
	 */
	host_shared_data_set_host_val(&realm, 0U, HOST_ARG1_INDEX, REALM_TIME_SLEEP);
	host_enter_realm_execute(&realm, REALM_SLEEP_CMD, RMI_EXIT_FIQ, 0U);

	/*
	 * Check if Realm exit reason is FIQ.
	 */
	if (!host_realm_handle_fiq_exit(&realm, 0U)) {
		ERROR("Trusted watchdog timer interrupt not fired\n");
		goto destroy_realm;
	}

	/* Check for the last serviced secure virtual interrupt. */
	ret_values = cactus_get_last_interrupt_cmd(SENDER, RECEIVER);

	if (!is_ffa_direct_response(ret_values)) {
		ERROR("Expected a direct response for last serviced interrupt"
			" command\n");
		goto destroy_realm;
	}

	/* Make sure Trusted Watchdog timer interrupt was serviced*/
	if (cactus_get_response(ret_values) != IRQ_TWDOG_INTID) {
		ERROR("Trusted watchdog timer interrupt not serviced by SP\n");
		goto destroy_realm;
	}

	/* Disable Trusted Watchdog interrupt. */
	if (!disable_trusted_wdog_interrupt(SENDER, RECEIVER)) {
		goto destroy_realm;
	}

	if (!host_destroy_realm(&realm)) {
		ERROR("host_destroy_realm error\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;

destroy_realm:
	host_destroy_realm(&realm);
	return TEST_RESULT_FAIL;
}

/* Choose a random security state that is different from the 'current' state */
static security_state_t get_random_security_state(security_state_t current,
						  bool is_sp_present)
{
	security_state_t next;

	/*
	 * 3 world config: Switch between NS world and Realm world as Secure
	 * world is not enabled or SP is not loaded.
	 */
	if (!is_sp_present) {
		if (current == NONSECURE_WORLD) {
			return REALM_WORLD;
		} else {
			return NONSECURE_WORLD;
		}
	}

	/*
	 * 4 world config: Randomly select a security_state between Realm, NS
	 * and Secure until the new state is not equal to the current state.
	 */
	while (true) {
		next = rand() % SECURITY_STATE_MAX;
		if (next == current) {
			continue;
		}

		break;
	}

	return next;
}

/*
 * Test whether FPU/SIMD state (32 SIMD vectors, FPCR and FPSR registers) are
 * preserved during a random context switch between Secure/Non-Secure/Realm world
 *
 * Below steps are performed by this test:
 *
 * Init:
 * Fill FPU registers with random values in
 * 1. NS World (NS-EL2)
 * 2. Realm world (R-EL1)
 * 3. Secure world (S-EL1) (if SP loaded)
 *
 * Test loop:
 *  security_state_next = get_random_security_state(current, is_sp_present)
 *
 *  switch to security_state_next
 *    if (FPU registers read != last filled values)
 *       break loop; return TC_FAIL
 *
 *    Fill FPU registers with new random values for the next comparison.
 */
test_result_t host_realm_fpu_access_in_rl_ns_se(void)
{
	security_state_t sec_state;
	bool is_sp_present;
	test_result_t res;

	/* Verify RME is present and RMM is not TRP */
	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	/* Verify that FFA is there and that it has the correct version. */
	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 1);

	res = init_realm();
	if (res != TEST_RESULT_SUCCESS) {
		return res;
	}

	/* Fill FPU registers in Non-secure world */
	fpu_state_write_rand(&ns_fpu_state_write);

	/* Fill FPU registers in Realm world */
	if (!fpu_fill_rl()) {
		ERROR("fpu_fill_rl error\n");
		goto destroy_realm;
	}
	sec_state = REALM_WORLD;

	/* Fill FPU registers in Secure world if present */
	res = init_sp();
	if (res == TEST_RESULT_SUCCESS) {
		if (!fpu_fill_sec()) {
			ERROR("fpu_fill_sec error\n");
			goto destroy_realm;
		}

		sec_state = SECURE_WORLD;
		is_sp_present = true;
	} else {
		is_sp_present = false;
	}

	for (uint32_t i = 0; i < 128; i++) {
		sec_state = get_random_security_state(sec_state, is_sp_present);

		switch (sec_state) {
		case NONSECURE_WORLD:
			/* NS world verify its FPU/SIMD state registers */
			fpu_state_read(&ns_fpu_state_read);
			if (fpu_state_compare(&ns_fpu_state_write,
					      &ns_fpu_state_read)) {
				ERROR("%s failed %d\n", __func__, __LINE__);
				goto destroy_realm;
			}

			/* Fill FPU state with new random values in NS world */
			fpu_state_write_rand(&ns_fpu_state_write);
			break;
		case REALM_WORLD:
			/* Realm world verify its FPU/SIMD state registers */
			if (!fpu_cmp_rl()) {
				goto destroy_realm;
			}

			/* Fill FPU state with new random values in Realm */
			if (!fpu_fill_rl()) {
				goto destroy_realm;
			}

			break;
		case SECURE_WORLD:
			/* Secure world verify its FPU/SIMD state registers */
			if (!fpu_cmp_sec()) {
				goto destroy_realm;
			}

			/* Fill FPU state with new random values in SP */
			if (!fpu_fill_sec()) {
				goto destroy_realm;

			}

			break;
		default:
			break;
		}
	}

	if (!host_destroy_realm(&realm)) {
		ERROR("host_destroy_realm error\n");
		return TEST_RESULT_FAIL;
	}
	return TEST_RESULT_SUCCESS;
destroy_realm:
	host_destroy_realm(&realm);
	return TEST_RESULT_FAIL;
}
