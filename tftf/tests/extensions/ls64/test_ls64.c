/*
 * Copyright (c) 2024-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "./test_ls64.h"
#include <test_helpers.h>

/*
 * @brief Test LS64 feature support when the extension is enabled.
 *
 * Execute the LS64 instructions:
 * LD64B   -  single-copy atomic 64-byte load.
 * ST64B   -  single-copy atomic 64-byte store without return.
 *
 * These instructions should not be trapped to EL3, when EL2 access them.
 *
 * @return test_result_t
 */
test_result_t test_ls64_instructions(void)
{
#if PLAT_fvp
#ifdef __aarch64__

	/* Make sure FEAT_LS64 is supported. */
	SKIP_TEST_IF_LS64_NOT_SUPPORTED();

	uint64_t ls64_input_buffer[LS64_ARRAYSIZE] = {1, 2, 3, 4, 5, 6, 7, 8};
	uint64_t ls64_output_buffer[LS64_ARRAYSIZE] = {0};
	/*
	 * Address where the data will be written to/read from with instructions
	 * st64b and ld64b respectively.
	 * Can only be in range (0x1d000000 - 0x1d00ffff) and be 64-byte aligned.
	 */
	uint64_t *store_address = (uint64_t *)LS64_ATOMIC_DEVICE_BASE;

	/**
	 * FEAT_LS64 : Execute LD64B and ST64B Instructions.
	 * This test copies data from input buffer, an array of 8-64bit
	 * unsigned integers to an output buffer via LD64B and ST64B
	 * atomic operation instructions.
	 *
	 * NOTE: As we cannot pre-write into LS64_ATOMIC_DEVICE_BASE memory
	 * via other instructions, we first load the data from a normal
	 * input buffer into the consecutive registers and then copy them in one
	 * atomic operation via st64b to Device memory(LS64_ATOMIC_DEVICE_BASE).
	 * Further we load the data from the same device memory into a normal
	 * output buffer through general registers and verify the buffers to
	 * ensure instructions copied the data as per the architecture.
	 */

	ls64_store(ls64_input_buffer, store_address);
	ls64_load(store_address, ls64_output_buffer);

	for (uint8_t i = 0U; i < LS64_ARRAYSIZE; i++) {
		VERBOSE("Input Buffer[%d]=%lld\n", i, ls64_input_buffer[i]);
		VERBOSE("Output Buffer[%d]=%lld\n", i, ls64_output_buffer[i]);

		if (ls64_input_buffer[i] != ls64_output_buffer[i]) {
			return TEST_RESULT_FAIL;
		}
	}

	return TEST_RESULT_SUCCESS;
#else
	SKIP_TEST_IF_AARCH32();
#endif /* __aarch64_ */
#else
	tftf_testcase_printf("Test supported only on FVP \n");
	return TEST_RESULT_SKIPPED;
#endif /* PLAT_fvp */

}

/*
 * @brief Test LS64_ACCDATA feature support when the extension is enabled.
 *
 * Write to the ACCDATA_EL1 system register, and read it back. This is
 * primarily to see if accesses to this register trap to EL3 (they should not).
 *
 * @return test_result_t
 */
test_result_t test_ls64_accdata_sysreg(void)
{
#ifdef __aarch64__
	SKIP_TEST_IF_LS64_ACCDATA_NOT_SUPPORTED();

	write_sys_accdata_el1(0x1234567890);
	if ((read_sys_accdata_el1() & 0xffffffff) != 0x34567890) {
		NOTICE("SYS_ACCDATA_EL1: 0x%lx\n", read_sys_accdata_el1());
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
#else
	SKIP_TEST_IF_AARCH32();
#endif /* __aarch64_ */
}
