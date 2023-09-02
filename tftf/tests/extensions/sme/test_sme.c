/*
 * Copyright (c) 2021-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <arch_features.h>
#include <arch_helpers.h>
#include <lib/extensions/sme.h>
#include <test_helpers.h>
#include <tftf_lib.h>

#ifdef __aarch64__

/* Global buffers*/
static __aligned(16) uint64_t ZA_In_vector[8] = {0xaaff, 0xbbff, 0xccff, 0xddff, 0xeeff,
					0xffff, 0xff00, 0xff00};
static __aligned(16) uint64_t ZA_Out_vector[8] = {0};

/**
 * sme_zero_ZA
 * ZER0 : Zero a list of upto eight 64bit element ZA tiles.
 * ZERO {<mask>} , where mask=ff, to clear all the 8, 64 bit elements.
 */
static void sme_zero_ZA(void)
{
	/**
	 * Due to the lack of support from the toolchain, instruction
	 * opcodes are used here.
	 * Manual Encoding Instruction, to Zero all the tiles of ZA array.
	 *
	 * TODO: Further, once the toolchain adds support for SME features
	 * this could be replaced with the actual instruction ZERO { <mask>}.
	 */
	asm volatile(".inst 0xc008000f" : : : );
}

/**
 * This function compares two buffers/vector elements
 * Inputs: uint64_t *ZA_In_vector, ZA_Out_vector
 * @return true : If both are equal
 * @return false : If both are not equal
 */
static bool sme_cmp_vector(const uint64_t *ZA_In_vector, const uint64_t *ZA_Out_vector)
{
	bool ret = true;

	for (int i = 0; i < (MAX_VL_B/8); i++) {
		if (ZA_In_vector[i] != ZA_Out_vector[i]) {
			ret = false;
		}
	}

	return ret;
}

#endif /* __aarch64__ */

test_result_t test_sme_support(void)
{
	/* SME is an AArch64-only feature.*/
	SKIP_TEST_IF_AARCH32();

#ifdef __aarch64__
	u_register_t reg;
	unsigned int current_vector_len;
	unsigned int requested_vector_len;
	unsigned int len_max;
	unsigned int __unused svl_max = 0U;
	u_register_t saved_smcr;

	/* Skip the test if SME is not supported. */
	SKIP_TEST_IF_SME_NOT_SUPPORTED();

	/* Make sure TPIDR2_EL0 is accessible. */
	write_tpidr2_el0(0);
	if (read_tpidr2_el0() != 0) {
		ERROR("Could not read TPIDR2_EL0.\n");
		return TEST_RESULT_FAIL;
	}
	write_tpidr2_el0(0xb0bafe77);
	if (read_tpidr2_el0() != 0xb0bafe77) {
		ERROR("Could not write TPIDR2_EL0.\n");
		return TEST_RESULT_FAIL;
	}

	/*
	 * Iterate through values for LEN to detect supported vector lengths.
	 */

	/* Entering Streaming SVE mode */
	sme_smstart(SMSTART_SM);

	saved_smcr = read_smcr_el2();

	/* Write SMCR_EL2 with the LEN max to find implemented width. */
	write_smcr_el2(SME_SMCR_LEN_MAX);
	isb();

	len_max = (unsigned int)read_smcr_el2();
	VERBOSE("Maximum SMCR_EL2.LEN value: 0x%x\n", len_max);
	VERBOSE("Enumerating supported vector lengths...\n");
	for (unsigned int i = 0; i <= len_max; i++) {
		/* Load new value into SMCR_EL2.LEN */
		reg = read_smcr_el2();
		reg &= ~(SMCR_ELX_LEN_MASK << SMCR_ELX_LEN_SHIFT);
		reg |= (i << SMCR_ELX_LEN_SHIFT);
		write_smcr_el2(reg);
		isb();

		/* Compute current and requested vector lengths in bits. */
		current_vector_len = ((unsigned int)sme_rdvl_1() * 8U);
		requested_vector_len = (i+1U)*128U;

		/*
		 * We count down from the maximum SMLEN value, so if the values
		 * match, we've found the largest supported value for SMLEN.
		 */
		if (current_vector_len == requested_vector_len) {
			svl_max = current_vector_len;
			VERBOSE("SUPPORTED:     %u bits (LEN=%u)\n", requested_vector_len, i);
		} else {
			VERBOSE("NOT SUPPORTED:     %u bits (LEN=%u)\n", requested_vector_len, i);
		}
	}

	INFO("Largest Supported Streaming Vector Length(SVL): %u bits \n", svl_max);

	/* Exiting Streaming SVE mode */
	sme_smstop(SMSTOP_SM);

	/**
	 * Perform/Execute SME Instructions.
	 * SME Data processing instructions LDR, STR, and ZERO instructions that
	 * access the SME ZA storage are legal only if ZA is enabled.
	 */

	/* Enable SME ZA Array Storage */
	sme_smstart(SMSTART_ZA);

	/* LDR : Load vector to ZA Array */
	sme_vector_to_ZA(ZA_In_vector);

	/* STR : Store Vector from ZA Array. */
	sme_ZA_to_vector(ZA_Out_vector);

	/* Compare both vectors to ensure load and store instructions have
	 * executed precisely.
	 */
	if (!sme_cmp_vector(ZA_In_vector, ZA_Out_vector)) {
		return TEST_RESULT_FAIL;
	}

	/* Zero or clear the entire ZA Array Storage/Tile */
	sme_zero_ZA();

	/* Disable the SME ZA array storage. */
	sme_smstop(SMSTOP_ZA);

	/* If FEAT_SME_FA64 then attempt to execute an illegal instruction. */
	if (is_feat_sme_fa64_supported()) {
		VERBOSE("FA64 supported, trying illegal instruction.\n");
		sme_try_illegal_instruction();
	}

	write_smcr_el2(saved_smcr);
	isb();

	return TEST_RESULT_SUCCESS;
#endif /* __aarch64__ */
}
