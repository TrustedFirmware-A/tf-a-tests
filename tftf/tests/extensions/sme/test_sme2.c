/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include <arch_features.h>
#include <arch_helpers.h>
#include <lib/extensions/sme.h>
#include <test_helpers.h>
#include <tftf_lib.h>

#ifdef __aarch64__

#define SME2_ARRAYSIZE		(512/64)
#define SME2_INPUT_DATA		(0x1fffffffffffffff)

/* Global buffers */
static __aligned(16) uint64_t sme2_input_buffer[SME2_ARRAYSIZE] = {0};
static __aligned(16) uint64_t sme2_output_buffer[SME2_ARRAYSIZE] = {0};

/*
 * clear_ZT0: ZERO all bytes of the ZT0 register.
 *
 */
static void clear_ZT0(void)
{
	/**
	 * Due to the lack of support from the toolchain, instruction
	 * opcodes are used here.
	 * TODO: Further, once the toolchain adds support for SME features
	 * this could be replaced with the instruction ZERO {ZT0}.
	 */
	asm volatile(".inst 0xc0480001" : : : );
}

#endif /* __aarch64__ */

/*
 * test_sme2_support: Test SME2 support when the extension is enabled.
 *
 * Execute some SME2 instructions. These should not be trapped to EL3,
 * as TF-A is responsible for enabling SME2 for Non-secure world.
 *
 */
test_result_t test_sme2_support(void)
{
	/* SME2 is an AArch64-only feature.*/
	SKIP_TEST_IF_AARCH32();

#ifdef __aarch64__
	/* Skip the test if SME2 is not supported. */
	SKIP_TEST_IF_SME2_NOT_SUPPORTED();

	/*
	 * FEAT_SME2 adds a 512 BIT architectural register ZT0 to support
	 * the lookup-table feature.
	 * System register SMCR_ELx defines a bit SMCR_ELx.EZT0 bit [30] to
	 * enable/disable access to this register. SMCR_EL2_RESET_VAL enables
	 * this bit by default.
	 *
	 * Instructions to access ZT0 register are being tested to ensure
	 * SMCR_EL3.EZT0 bit is set by EL3 firmware so that EL2 access are not
	 * trapped.
	 */

	/* Make sure we can acesss SME2 ZT0 storage, PSTATE.ZA = 1*/
	VERBOSE("Enabling SME ZA storage and ZT0 storage.\n");

	sme_smstart(SMSTART_ZA);

	/*
	 * LDR (ZT0) : Load ZT0 register.
	 *             Load the 64-byte ZT0 register from the memory address
	 *             provided in the 64-bit scalar base register.
	 */
	for (int i = 0; i < SME2_ARRAYSIZE; i++) {
		sme2_input_buffer[i] = SME2_INPUT_DATA;
	}
	sme2_load_zt0_instruction(sme2_input_buffer);

	/*
	 * STR (ZT0) : Store ZT0 register.
	 *             Store the 64-byte ZT0 register to the memory address
	 *             provided in the 64-bit scalar base register
	 */

	sme2_store_zt0_instruction(sme2_output_buffer);

	/**
	 * compare the input and output buffer to verify the operations of
	 * LDR and STR instructions with ZT0 register.
	 */
	for (int i = 0; i < SME2_ARRAYSIZE; i++) {
		if (sme2_input_buffer[i] != sme2_output_buffer[i]) {
			return TEST_RESULT_FAIL;
		}
	}

	/* ZER0 (ZT0) */
	clear_ZT0();

	/* Finally disable the acesss to SME2 ZT0 storage, PSTATE.ZA = 0*/
	VERBOSE("Disabling SME ZA storage and ZT0 storage.\n");

	sme_smstop(SMSTOP_ZA);

	return TEST_RESULT_SUCCESS;
#endif /* __aarch64__ */
}
