/*
 * Copyright (c) 2022-2024, Arm Limited. All rights reserved.
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
#include <tftf_lib.h>

#include "host_realm_simd_common.h"

#define REALM_TIME_SLEEP	300U
#define SENDER HYP_ID
#define RECEIVER SP_ID(1)
static const struct ffa_uuid expected_sp_uuids[] = { {PRIMARY_UUID} };
static struct mailbox_buffers mb;
static bool secure_mailbox_initialised;

/*
 * Min test iteration count for 'host_and_realm_check_simd' and
 * 'host_realm_swd_check_simd' tests.
 */
#define TEST_ITERATIONS_MIN	(16U)

/* Number of FPU configs: none */
#define NUM_FPU_CONFIGS		(0U)

/* Number of SVE configs: SVE_VL, SVE hint */
#define NUM_SVE_CONFIGS		(2U)

/* Number of SME configs: SVE_SVL, FEAT_FA64, Streaming mode */
#define NUM_SME_CONFIGS		(3U)

typedef enum {
	TEST_FPU = 0U,
	TEST_SVE,
	TEST_SME,
} simd_test_t;

typedef enum security_state {
	NONSECURE_WORLD = 0U,
	REALM_WORLD,
	SECURE_WORLD,
	SECURITY_STATE_MAX
} security_state_t;

/* Defined in host_realm_simd_common.c */
extern sve_z_regs_t ns_sve_z_regs_write;
extern sve_z_regs_t ns_sve_z_regs_read;

static sve_p_regs_t ns_sve_p_regs_write;
static sve_p_regs_t ns_sve_p_regs_read;

static sve_ffr_regs_t ns_sve_ffr_regs_write;
static sve_ffr_regs_t ns_sve_ffr_regs_read;

static fpu_q_reg_t ns_fpu_q_regs_write[FPU_Q_COUNT];
static fpu_q_reg_t ns_fpu_q_regs_read[FPU_Q_COUNT];

static fpu_cs_regs_t ns_fpu_cs_regs_write;
static fpu_cs_regs_t ns_fpu_cs_regs_read;

static struct realm realm;

/* This function helps to initialise secure_mailbox if FFA is supported  */
static test_result_t init_secure_partition(void)
{
	/* Verify that FF-A is there and that it has the correct version. */
	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 1);

	if (!secure_mailbox_initialised) {
		GET_TFTF_MAILBOX(mb);
		CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);
		secure_mailbox_initialised = true;
	}

	return TEST_RESULT_SUCCESS;
}

static test_result_t init_realm_payload(struct realm *realm, bool *sve_en_ret)
{
	u_register_t rmi_feat_reg0;
	test_result_t rc;
	uint8_t sve_vq;
	bool sve_en;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (host_rmi_features(0UL, &rmi_feat_reg0) != REALM_SUCCESS) {
		ERROR("Failed to get RMI feat_reg0\n");
		return TEST_RESULT_FAIL;
	}

	sve_en = rmi_feat_reg0 & RMI_FEATURE_REGISTER_0_SVE_EN;
	sve_vq = EXTRACT(RMI_FEATURE_REGISTER_0_SVE_VL, rmi_feat_reg0);

	/* Create Realm with SVE enabled if RMI features supports it */
	INFO("TFTF: create realm sve_en/sve_vq: %d/%d\n", sve_en, sve_vq);
	rc = host_create_sve_realm_payload(realm, sve_en, sve_vq);
	if (rc != TEST_RESULT_SUCCESS) {
		return rc;
	}

	if (sve_en_ret != NULL) {
		*sve_en_ret = sve_en;
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

	res = init_secure_partition();
	if (res != TEST_RESULT_SUCCESS) {
		return res;
	}

	res = init_realm_payload(&realm, NULL);
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
	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 0U,
			HOST_ARG1_INDEX, REALM_TIME_SLEEP);

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
						  bool is_sp_present,
						  bool is_rl_present)
{
	security_state_t next;

	/*
	 * 3 world config:
	 * Switch between NS world and Realm world if Secure world is not
	 * enabled or SP is not loaded. or
	 * Switch between NS world and Secure world if Realm world is not
	 * enabled.
	 */
	if (is_sp_present ^ is_rl_present) {
		if (current == NONSECURE_WORLD) {
			return is_rl_present ? REALM_WORLD : SECURE_WORLD;
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

/* Generate random values and write it to SVE Z, P and FFR registers */
static void ns_sve_write_rand(void)
{
	bool has_ffr = true;

	if (is_feat_sme_supported() && sme_smstat_sm() &&
	    !sme_feat_fa64_enabled()) {
		has_ffr = false;
	}

	sve_z_regs_write_rand(&ns_sve_z_regs_write);
	sve_p_regs_write_rand(&ns_sve_p_regs_write);
	if (has_ffr) {
		sve_ffr_regs_write_rand(&ns_sve_ffr_regs_write);
	}
}

/* Read SVE Z, P and FFR registers and compare it with the last written values */
static test_result_t ns_sve_read_and_compare(void)
{
	test_result_t rc = TEST_RESULT_SUCCESS;
	uint64_t bitmap;
	bool has_ffr = true;

	if (is_feat_sme_supported() && sme_smstat_sm() &&
	    !sme_feat_fa64_enabled()) {
		has_ffr = false;
	}

	/* Clear old state */
	memset((void *)&ns_sve_z_regs_read, 0, sizeof(ns_sve_z_regs_read));
	memset((void *)&ns_sve_p_regs_read, 0, sizeof(ns_sve_p_regs_read));
	memset((void *)&ns_sve_ffr_regs_read, 0, sizeof(ns_sve_ffr_regs_read));

	/* Read Z, P, FFR registers to compare it with the last written values */
	sve_z_regs_read(&ns_sve_z_regs_read);
	sve_p_regs_read(&ns_sve_p_regs_read);
	if (has_ffr) {
		sve_ffr_regs_read(&ns_sve_ffr_regs_read);
	}

	bitmap = sve_z_regs_compare(&ns_sve_z_regs_write, &ns_sve_z_regs_read);
	if (bitmap != 0UL) {
		ERROR("SVE Z regs compare failed (bitmap: 0x%016llx)\n",
		      bitmap);
		rc = TEST_RESULT_FAIL;
	}

	bitmap = sve_p_regs_compare(&ns_sve_p_regs_write, &ns_sve_p_regs_read);
	if (bitmap != 0UL) {
		ERROR("SVE P regs compare failed (bitmap: 0x%016llx)\n",
		      bitmap);
		rc = TEST_RESULT_FAIL;
	}

	if (has_ffr) {
		bitmap = sve_ffr_regs_compare(&ns_sve_ffr_regs_write,
					      &ns_sve_ffr_regs_read);
		if (bitmap != 0) {
			ERROR("SVE FFR regs compare failed "
			      "(bitmap: 0x%016llx)\n", bitmap);
			rc = TEST_RESULT_FAIL;
		}
	}

	return rc;
}

/*
 * Generate random values and write it to Streaming SVE Z, P and FFR registers.
 */
static void ns_sme_write_rand(void)
{
	/*
	 * TODO: more SME specific registers like ZA, ZT0 can be included later.
	 */

	/* Fill SVE registers in normal or streaming SVE mode */
	ns_sve_write_rand();
}

/*
 * Read streaming SVE Z, P and FFR registers and compare it with the last
 * written values
 */
static test_result_t ns_sme_read_and_compare(void)
{
	/*
	 * TODO: more SME specific registers like ZA, ZT0 can be included later.
	 */

	/* Compares SVE registers in normal or streaming SVE mode */
	return ns_sve_read_and_compare();
}

static char *simd_type_to_str(simd_test_t type)
{
	if (type == TEST_FPU) {
		return "FPU";
	} else if (type == TEST_SVE) {
		return "SVE";
	} else if (type == TEST_SME) {
		return "SME";
	} else {
		return "UNKNOWN";
	}
}

static void ns_simd_print_cmd_config(bool cmd, simd_test_t type)
{
	char __unused *tstr = simd_type_to_str(type);
	char __unused *cstr = cmd ? "write rand" : "read and compare";

	if (type == TEST_SME) {
		if (sme_smstat_sm()) {
			INFO("TFTF: NS [%s] %s. Config: smcr: 0x%llx, SM: on\n",
			     tstr, cstr, (uint64_t)read_smcr_el2());
		} else {
			INFO("TFTF: NS [%s] %s. Config: smcr: 0x%llx, "
			     "zcr: 0x%llx sve_hint: %d SM: off\n", tstr, cstr,
			     (uint64_t)read_smcr_el2(),
			     (uint64_t)sve_read_zcr_elx(),
			     tftf_smc_get_sve_hint());
		}
	} else if (type == TEST_SVE) {
		INFO("TFTF: NS [%s] %s. Config: zcr: 0x%llx, sve_hint: %d\n",
		     tstr, cstr, (uint64_t)sve_read_zcr_elx(),
		     tftf_smc_get_sve_hint());
	} else {
		INFO("TFTF: NS [%s] %s\n", tstr, cstr);
	}
}

/*
 * Randomly select TEST_SME or TEST_FPU. For TEST_SME, randomly select below
 * configurations:
 * - enable/disable streaming mode
 *   For streaming mode:
 *   - enable or disable FA64
 *   - select random streaming vector length
 *   For normal SVE mode:
 *   - select random normal SVE vector length
 */
static simd_test_t ns_sme_select_random_config(void)
{
	simd_test_t type;
	static unsigned int counter;

	/* Use a static counter to mostly select TEST_SME case. */
	if ((counter % 8U) != 0) {
		/* Use counter to toggle between Streaming mode on or off */
		if (is_armv8_2_sve_present() && ((counter % 2U) != 0)) {
			sme_smstop(SMSTOP_SM);
			sve_config_vq(SVE_GET_RANDOM_VQ);

			if ((counter % 3U) != 0) {
				tftf_smc_set_sve_hint(true);
			} else {
				tftf_smc_set_sve_hint(false);
			}
		} else {
			sme_smstart(SMSTART_SM);
			sme_config_svq(SME_GET_RANDOM_SVQ);

			if ((counter % 3U) != 0) {
				sme_enable_fa64();
			} else {
				sme_disable_fa64();
			}
		}
		type = TEST_SME;
	} else {
		type = TEST_FPU;
	}
	counter++;

	return type;
}

/*
 * Randomly select TEST_SVE or TEST_FPU. For TEST_SVE, configure zcr_el2 with
 * random vector length and randomly enable or disable SMC SVE hint bit.
 */
static simd_test_t ns_sve_select_random_config(void)
{
	simd_test_t type;
	static unsigned int counter;

	/* Use a static counter to mostly select TEST_SVE case. */
	if ((counter % 4U) != 0) {
		sve_config_vq(SVE_GET_RANDOM_VQ);

		if ((counter % 2U) != 0) {
			tftf_smc_set_sve_hint(true);
		} else {
			tftf_smc_set_sve_hint(false);
		}

		type = TEST_SVE;
	} else {
		type = TEST_FPU;
	}
	counter++;

	return type;
}

/*
 * Configure NS world SIMD. Randomly choose to test SVE or FPU registers if
 * system supports SVE.
 *
 * Returns either TEST_FPU or TEST_SVE or TEST_SME
 */
static simd_test_t ns_simd_select_random_config(void)
{
	simd_test_t type;

	/* cleanup old config for SME */
	if (is_feat_sme_supported()) {
		sme_smstop(SMSTOP_SM);
		sme_enable_fa64();
	}

	/* Cleanup old config for SVE */
	if (is_armv8_2_sve_present()) {
		tftf_smc_set_sve_hint(false);
	}

	if (is_armv8_2_sve_present() && is_feat_sme_supported()) {
		if (rand() % 2) {
			type = ns_sme_select_random_config();
		} else {
			type = ns_sve_select_random_config();
		}
	} else if (is_feat_sme_supported()) {
		type = ns_sme_select_random_config();
	} else if (is_armv8_2_sve_present()) {
		type = ns_sve_select_random_config();
	} else {
		type = TEST_FPU;
	}

	return type;
}

/* Select random NS SIMD config and write random values to its registers */
static simd_test_t ns_simd_write_rand(void)
{
	simd_test_t type;

	type = ns_simd_select_random_config();

	ns_simd_print_cmd_config(true, type);

	if (type == TEST_SME) {
		ns_sme_write_rand();
	} else if (type == TEST_SVE) {
		ns_sve_write_rand();
	} else {
		fpu_q_regs_write_rand(ns_fpu_q_regs_write);
	}

	/* fpcr, fpsr common to all configs */
	fpu_cs_regs_write_rand(&ns_fpu_cs_regs_write);

	return type;
}

/* Read and compare the NS SIMD registers with the last written values */
static test_result_t ns_simd_read_and_compare(simd_test_t type)
{
	test_result_t rc = TEST_RESULT_SUCCESS;

	ns_simd_print_cmd_config(false, type);

	if (type == TEST_SME) {
		rc = ns_sme_read_and_compare();
	} else if (type == TEST_SVE) {
		rc = ns_sve_read_and_compare();
	} else {
		fpu_q_regs_read(ns_fpu_q_regs_read);
		if (fpu_q_regs_compare(ns_fpu_q_regs_write,
				       ns_fpu_q_regs_read)) {
			ERROR("FPU Q registers compare failed\n");
			rc = TEST_RESULT_FAIL;
		}
	}

	/* fpcr, fpsr common to all configs */
	fpu_cs_regs_read(&ns_fpu_cs_regs_read);
	if (fpu_cs_regs_compare(&ns_fpu_cs_regs_write, &ns_fpu_cs_regs_read)) {
		ERROR("FPCR/FPSR registers compare failed\n");
		rc = TEST_RESULT_FAIL;
	}

	return rc;
}

/* Select random Realm SIMD config and write random values to its registers */
static simd_test_t rl_simd_write_rand(struct realm *realm, bool rl_sve_en)
{
	enum realm_cmd rl_fill_cmd;
	simd_test_t type;
	bool __unused rc;

	/* Select random commands to test. SVE or FPU registers in Realm */
	if (rl_sve_en && (rand() % 2)) {
		type = TEST_SVE;
	} else {
		type = TEST_FPU;
	}

	INFO("TFTF: RL [%s] write random\n", simd_type_to_str(type));
	if (type == TEST_SVE) {
		rl_fill_cmd = REALM_SVE_FILL_REGS;
	} else {
		rl_fill_cmd = REALM_REQ_FPU_FILL_CMD;
	}

	rc = host_enter_realm_execute(realm, rl_fill_cmd, RMI_EXIT_HOST_CALL, 0U);
	assert(rc);

	return type;
}

/* Read and compare the Realm SIMD registers with the last written values */
static bool rl_simd_read_and_compare(struct realm *realm, simd_test_t type)
{
	enum realm_cmd rl_cmp_cmd;

	INFO("TFTF: RL [%s] read and compare\n", simd_type_to_str(type));
	if (type == TEST_SVE) {
		rl_cmp_cmd = REALM_SVE_CMP_REGS;
	} else {
		rl_cmp_cmd = REALM_REQ_FPU_CMP_CMD;
	}

	return host_enter_realm_execute(realm, rl_cmp_cmd, RMI_EXIT_HOST_CALL,
					0U);
}

/* Send request to SP to fill FPU/SIMD regs with secure template values */
static bool sp_fpu_write_rand(void)
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
static bool sp_fpu_read_and_compare(void)
{
	struct ffa_value ret = cactus_req_simd_compare_send_cmd(SENDER,
								RECEIVER);

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

/*
 * This test case verifies whether various SIMD related registers like Q[0-31],
 * FPCR, FPSR, Z[0-31], P[0-15], FFR are preserved by:
 *
 * 1. RMM during world switch between NS world and Realm world.
 * 2. Hafnium during world switch between NS world and Secure world.
 *
 * This testcase runs on below SIMD configs:
 * - SVE only
 * - SME only
 * - with SVE and SME
 * - without SVE and SME
 *
 * This testcase runs on below world configs:
 * - Secure world only
 * - Realm world only
 * - With Secure and Realm world
 */
test_result_t host_realm_swd_check_simd(void)
{
	test_result_t rc;
	bool sve_en = false;
	security_state_t sec_state;
	simd_test_t ns_simd_type;
	simd_test_t rl_simd_type = TEST_FPU;
	unsigned int test_iterations;
	unsigned int num_simd_types;
	unsigned int num_simd_configs;
	bool is_sp_present = false;
	bool is_rl_present = false;

	/* Init and fill FPU registers in Secure world if present */
	if (init_secure_partition() == TEST_RESULT_SUCCESS) {
		is_sp_present = true;
		if (!sp_fpu_write_rand()) {
			ERROR("%s failed %d\n", __func__, __LINE__);
			return TEST_RESULT_FAIL;
		}
	}

	/* Init and fill FPU or SVE registers in Realm if present */
	if (init_realm_payload(&realm, &sve_en) == TEST_RESULT_SUCCESS) {
		is_rl_present = true;
		rl_simd_type = rl_simd_write_rand(&realm, sve_en);
	}

	if (!is_sp_present && !is_rl_present) {
		INFO("Neither SP nor Realm exists. Skipping test case\n");
		return TEST_RESULT_SKIPPED;
	}

	/*
	 * Randomly select and configure NS simd context to test. And fill it
	 * with random values.
	 */
	ns_simd_type = ns_simd_write_rand();
	sec_state = NONSECURE_WORLD;

	/*
	 * Find out test iterations based on if SVE is enabled and the number of
	 * configurations available in the SVE.
	 */

	/* FPU is always available */
	num_simd_types = 1U;
	num_simd_configs = NUM_FPU_CONFIGS;

	if (is_armv8_2_sve_present()) {
		num_simd_types += 1;
		num_simd_configs += NUM_SVE_CONFIGS;
	}

	if (is_feat_sme_supported()) {
		num_simd_types += 1;
		num_simd_configs += NUM_SME_CONFIGS;
	}

	if (num_simd_configs > 0U) {
		test_iterations = TEST_ITERATIONS_MIN * num_simd_types *
			num_simd_configs;
	} else {
		test_iterations = TEST_ITERATIONS_MIN * num_simd_types;
	}

	if (is_sp_present && is_rl_present) {
		test_iterations *= 2U;
	}

	/*
	 * Test loop:
	 *  security_state = get_random_security_state()
	 *
	 *  switch to security_state
	 *    if (SIMD registers read != last filled values)
	 *       break loop; return TC_FAIL
	 *
	 *    Fill SIMD registers with new random values for the next comparison.
	 */
	for (uint32_t i = 0U; i < test_iterations; i++) {
		sec_state = get_random_security_state(sec_state, is_sp_present,
						      is_rl_present);

		switch (sec_state) {
		case NONSECURE_WORLD:
			/*
			 * Read NS simd context and compare it with last written
			 * context.
			 */
			rc = ns_simd_read_and_compare(ns_simd_type);
			if (rc != TEST_RESULT_SUCCESS) {
				ERROR("%s failed %d\n", __func__, __LINE__);
				rc = TEST_RESULT_FAIL;
				goto rm_realm;
			}

			/*
			 * Randomly select and configure NS simd context. And
			 * fill it with random values for the next compare.
			 */
			ns_simd_type = ns_simd_write_rand();
			break;
		case REALM_WORLD:
			/*
			 * Enter Realm and read the simd context and compare it
			 * with last written context.
			 */
			if (!rl_simd_read_and_compare(&realm, rl_simd_type)) {
				ERROR("%s failed %d\n", __func__, __LINE__);
				rc = TEST_RESULT_FAIL;
				goto rm_realm;
			}

			/*
			 * Randomly select and configure Realm simd context to
			 * test. Enter realm and fill simd context with random
			 * values for the next compare.
			 */
			rl_simd_type = rl_simd_write_rand(&realm, sve_en);
			break;
		case SECURE_WORLD:
			INFO("TFTF: S  [FPU] read and compare\n");
			/* Secure world verify its FPU/SIMD state registers */
			if (!sp_fpu_read_and_compare()) {
				ERROR("%s failed %d\n", __func__, __LINE__);
				rc = TEST_RESULT_FAIL;
				goto rm_realm;
			}

			INFO("TFTF: S  [FPU] write random\n");
			/* Fill FPU state with new random values in SP */
			if (!sp_fpu_write_rand()) {
				rc = TEST_RESULT_FAIL;
				goto rm_realm;
			}
			break;
		default:
			break;
		}
	}

	rc = TEST_RESULT_SUCCESS;
rm_realm:
	/* Cleanup old config */
	if (is_feat_sme_supported()) {
		sme_smstop(SMSTOP_SM);
		sme_enable_fa64();
	}

	/* Cleanup old config */
	if (is_armv8_2_sve_present()) {
		tftf_smc_set_sve_hint(false);
	}

	if (is_rl_present && !host_destroy_realm(&realm)) {
		return TEST_RESULT_FAIL;
	}

	return rc;
}
