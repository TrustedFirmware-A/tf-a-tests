/*
 * Copyright (c) 2018, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <arch_features.h>
#include <arch_helpers.h>
#include <host_realm_rmi.h>
#include <string.h>
#include <sync.h>
#include <test_helpers.h>
#include <tftf.h>
#include <tftf_lib.h>

void mops_cpy(void *dst, const void *src, size_t len);

static uint64_t mops_exception_esr;
static volatile bool mops_exception_el2;
static volatile bool mops_exception_caught;

static bool exception_handler(void)
{
	mops_exception_caught = true;
	mops_exception_el2 = IS_IN_EL2();
	mops_exception_esr = mops_exception_el2 ? read_esr_el2() : read_esr_el1();

	/* Skip the faulting instruction so the test can report failure. */
	return true;
}

/*
 * Test FEAT_MOPS enablement by RMM
 *
 * If FEAT_MOPS is available in the architecture, RMM should enable it for Realms.
 */
test_result_t host_realm_feat_mops(void)
{
	mops_exception_esr = 0U;
	mops_exception_el2 = false;
	mops_exception_caught = false;

	int i;
	uint8_t src[64];
	uint8_t dst_fp[64];
	size_t len = sizeof(src);

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	/* Fill the source buffer with a pattern */
	for (i = 0; i < (int)sizeof(src); i++) {
		src[i] = (uint8_t)(i ^ 0x5aU);
	}
	memset(dst_fp, 0, sizeof(dst_fp));

	/*
	 * Use FEAT_MOPS CPY instructions to copy memory.
	 * If FEAT_MOPS isn't enabled at EL0, this should trap.
	 */
	register_custom_sync_exception_handler(exception_handler);
	mops_cpy(dst_fp, src, len);
	unregister_custom_sync_exception_handler();

	/* If FEAT_MOPS is unavailable, there should be an exception in EL2 */
	if (!is_feat_mops_present() & mops_exception_caught) {
		if (mops_exception_el2 == false) {
			tftf_testcase_printf("MOPS Instruction Exception at wrong EL: %d\n",
						mops_exception_el2 ? 2 : 1);
			return TEST_RESULT_FAIL;
		}

		/* Check the Exception Syndrome Register */
		if (EC_BITS(mops_exception_esr) != EC_UNKNOWN) {
			tftf_testcase_printf("Unexpected ESR_EL2 for MOPS trap: 0x%llx\n",
				      mops_exception_esr);
			tftf_testcase_printf("Expected ESR_EL2.EC = 0x000000\n");
			return TEST_RESULT_FAIL;
		}

		tftf_testcase_printf("FEAT_MOPS not present, MOPS instructions trapped as expected.\n");
		return TEST_RESULT_SUCCESS;
	}

	/* Exception caught even with FEAT_MOPS implemented */
	if (mops_exception_caught) {
		tftf_testcase_printf("Unknown Exception Caused: ESR_ELX=0x%llx\n",
				  mops_exception_esr);
		return TEST_RESULT_FAIL;
	}

	/* If FEAT_MOPS is enabled, the copy should succeed and the buffers should match. */
	if ((memcmp(src, dst_fp, len) != 0)) {
		return TEST_RESULT_FAIL;
	}

	tftf_testcase_printf("FEAT_MOPS is present, MOPS instructions executed successfully.\n");
	return TEST_RESULT_SUCCESS;
}
