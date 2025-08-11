/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <debug.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_shared_data.h>
#include <test_helpers.h>

/*
 * This test verifies that the values of BRBCR_EL2 and
 * BRBCR_EL1 registers are being preserved by EL3 and
 * R-EL2 as expected.
 */

test_result_t host_realm_test_brbe_save_restore(void)
{
	unsigned long rec_flag[] = {RMI_RUNNABLE};
	unsigned long feature_flag =  0UL;
	unsigned long feature_flag1 =  0UL;
	struct realm realm;
	long sl = RTT_MIN_LEVEL;
	static unsigned long brbcr_el2_saved;
	static unsigned long brbcr_el1_saved;
	test_result_t test_result = TEST_RESULT_SUCCESS;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();
	SKIP_TEST_IF_BRBE_NOT_SUPPORTED();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
						feature_flag, feature_flag1, sl, rec_flag, 1U, 0U,
						get_test_mecid())) {
		return TEST_RESULT_FAIL;
	}

	/*
	 * Program BRBCR_EL2 to enable branch recording, now
	 * since MDCR_EL3.SBRBE is set to 0b01 in EL3, accesses
	 * to BRBCR_EL2 should not trap to EL3.
	 */

	INFO ("Enable Branch recording in NS-EL2\n");

	/* SET BRBCR_EL2 */
	unsigned long val = read_brbcr_el2();
	val = val | BRBCR_EL2_INIT;
	write_brbcr_el2(val);

	/* SET BRBCR_EL1 */
	unsigned long val_brbcr_el1 = read_brbcr_el1();
	val_brbcr_el1 = BRBCR_EL1_INIT & (~MASK(BRBCR_EL1_CC));
	write_brbcr_el1(val_brbcr_el1);

	brbcr_el2_saved = read_brbcr_el2();
	brbcr_el1_saved	= read_brbcr_el1();

	/*
	 * Enter Realm and try to access BRBCR_EL1 and enable
	 * branch recording for R-EL1.
	 */

	if (!host_enter_realm_execute(&realm, REALM_WRITE_BRBCR_EL1,
					RMI_EXIT_HOST_CALL, 0)) {
		test_result = TEST_RESULT_FAIL;
		goto destroy_realm;
	}

	/*
	 * Verify if EL3 has saved and restored the value in
	 * BRBCR_EL2 register.
	 */
	INFO("NS Read and compare BRBCR_EL2\n");
	if (read_brbcr_el2() != brbcr_el2_saved) {
		test_result = TEST_RESULT_FAIL;
		INFO("brbcr_el2 did not match, read = %lx\n and saved = %lx\n", read_brbcr_el2(), \
									brbcr_el2_saved);
		goto destroy_realm;
	}

	if (read_brbcr_el1() != brbcr_el1_saved) {
		test_result = TEST_RESULT_FAIL;
		INFO("brbcr_el1 did not match, read = %lx\n and saved = %lx\n", read_brbcr_el1(), \
									brbcr_el1_saved);
		goto destroy_realm;
	}

destroy_realm:
	if (!host_destroy_realm(&realm)) {
		return TEST_RESULT_FAIL;
	}

	return test_result;
}
