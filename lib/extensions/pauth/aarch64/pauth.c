/*
 * Copyright (c) 2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <cdefs.h>
#include <stdbool.h>
#include <stdint.h>
#include <debug.h>
#include <pauth.h>

/*
 * This is only a toy implementation to generate a seemingly random
 * 128-bit key from sp, x30 and cntpct_el0 values.
 */
uint128_t init_apkey(void)
{
	uint64_t return_addr = (uint64_t)__builtin_return_address(0U);
	uint64_t frame_addr = (uint64_t)__builtin_frame_address(0U);

	uint64_t cntpct = read_cntpct_el0();

	uint64_t key_lo = (return_addr << 13) ^ frame_addr ^ cntpct;
	uint64_t key_hi = (frame_addr << 15) ^ return_addr ^ cntpct;

	return ((uint128_t)(key_hi) << 64) | key_lo;
}

/* Check if ARMv8.3-PAuth key is enabled */
static bool is_pauth_key_enabled(uint64_t key_bit)
{
	unsigned int el = (unsigned int)GET_EL(read_CurrentEl());

	if (el == 1U) {
		return ((read_sctlr_el1() & key_bit) != 0U);
	} else if (el == 2U) {
		return ((read_sctlr_el2() & key_bit) != 0U);
	}
	return false;
}

bool pauth_test_lib_compare_template(uint128_t *pauth_keys_before, uint128_t *pauth_keys_after)
{
	bool result = true;

	pauth_test_lib_read_keys(pauth_keys_after);
	for (unsigned int i = 0U; i < NUM_KEYS; ++i) {
		if (pauth_keys_before[i] != pauth_keys_after[i]) {
			ERROR("AP%sKey_EL1 read 0x%llx:%llx "
			"expected 0x%llx:%llx\n", key_name[i],
			(uint64_t)(pauth_keys_after[i] >> 64U),
			(uint64_t)(pauth_keys_after[i]),
			(uint64_t)(pauth_keys_before[i] >> 64U),
			(uint64_t)(pauth_keys_before[i]));

			result = false;
		}
	}
	return result;
}

/*
 * Program or read ARMv8.3-PAuth keys (if already enabled)
 * and store them in <pauth_keys_before> buffer
 */
void pauth_test_lib_fill_regs_and_template(uint128_t *pauth_keys_before)
{
	uint128_t plat_key;

	(void)memset(pauth_keys_before, 0, NUM_KEYS * sizeof(uint128_t));

	if (is_pauth_key_enabled(SCTLR_EnIA_BIT)) {
		/* Read APIAKey_EL1 */
		plat_key = read_apiakeylo_el1() |
			((uint128_t)(read_apiakeyhi_el1()) << 64U);
		INFO("EnIA is set\n");
	} else {
		/* Program APIAKey_EL1 */
		plat_key = init_apkey();
		write_apiakeylo_el1((uint64_t)plat_key);
		write_apiakeyhi_el1((uint64_t)(plat_key >> 64U));
	}
	pauth_keys_before[0] = plat_key;

	if (is_pauth_key_enabled(SCTLR_EnIB_BIT)) {
		/* Read APIBKey_EL1 */
		plat_key = read_apibkeylo_el1() |
			((uint128_t)(read_apibkeyhi_el1()) << 64U);
		INFO("EnIB is set\n");
	} else {
		/* Program APIBKey_EL1 */
		plat_key = init_apkey();
		write_apibkeylo_el1((uint64_t)plat_key);
		write_apibkeyhi_el1((uint64_t)(plat_key >> 64U));
	}
	pauth_keys_before[1] = plat_key;

	if (is_pauth_key_enabled(SCTLR_EnDA_BIT)) {
		/* Read APDAKey_EL1 */
		plat_key = read_apdakeylo_el1() |
			((uint128_t)(read_apdakeyhi_el1()) << 64U);
		INFO("EnDA is set\n");
	} else {
		/* Program APDAKey_EL1 */
		plat_key = init_apkey();
		write_apdakeylo_el1((uint64_t)plat_key);
		write_apdakeyhi_el1((uint64_t)(plat_key >> 64U));
	}
	pauth_keys_before[2] = plat_key;

	if (is_pauth_key_enabled(SCTLR_EnDB_BIT)) {
		/* Read APDBKey_EL1 */
		plat_key = read_apdbkeylo_el1() |
			((uint128_t)(read_apdbkeyhi_el1()) << 64U);
		INFO("EnDB is set\n");
	} else {
		/* Program APDBKey_EL1 */
		plat_key = init_apkey();
		write_apdbkeylo_el1((uint64_t)plat_key);
		write_apdbkeyhi_el1((uint64_t)(plat_key >> 64U));
	}
	pauth_keys_before[3] = plat_key;

	pauth_keys_before[4] = read_apgakeylo_el1() |
			       ((uint128_t)(read_apgakeyhi_el1()) << 64U);
	if (pauth_keys_before[4] == 0ULL) {
		/* Program APGAKey_EL1 */
		plat_key = init_apkey();
		write_apgakeylo_el1((uint64_t)plat_key);
		write_apgakeyhi_el1((uint64_t)(plat_key >> 64U));
		pauth_keys_before[4] = plat_key;
	}

	isb();
}

/*
 * Read ARMv8.3-PAuth keys and store them in
 * <pauth_keys_arr> buffer
 */
void pauth_test_lib_read_keys(uint128_t *pauth_keys_arr)
{
	(void)memset(pauth_keys_arr, 0, NUM_KEYS * sizeof(uint128_t));

	/* Read APIAKey_EL1 */
	pauth_keys_arr[0] = read_apiakeylo_el1() |
		((uint128_t)(read_apiakeyhi_el1()) << 64U);

	/* Read APIBKey_EL1 */
	pauth_keys_arr[1] = read_apibkeylo_el1() |
		((uint128_t)(read_apibkeyhi_el1()) << 64U);

	/* Read APDAKey_EL1 */
	pauth_keys_arr[2] = read_apdakeylo_el1() |
		((uint128_t)(read_apdakeyhi_el1()) << 64U);

	/* Read APDBKey_EL1 */
	pauth_keys_arr[3] = read_apdbkeylo_el1() |
		((uint128_t)(read_apdbkeyhi_el1()) << 64U);

	/* Read APGAKey_EL1 */
	pauth_keys_arr[4] = read_apgakeylo_el1() |
		((uint128_t)(read_apgakeyhi_el1()) << 64U);
}

/* Test execution of ARMv8.3-PAuth instructions */
void pauth_test_lib_test_intrs(void)
{
	/* Pointer authentication instructions */
	__asm__ volatile (
			"paciasp\n"
			"autiasp\n"
			"paciasp\n"
			"xpaclri");
}
