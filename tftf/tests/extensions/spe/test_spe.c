/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <test_helpers.h>

test_result_t test_spe_support(void)
{
	/* SPE is an AArch64-only feature.*/
	SKIP_TEST_IF_AARCH32();

#ifdef __aarch64__
	unsigned int spe_ver = spe_get_version();

	if (spe_ver > ID_AA64DFR0_SPE_V1P4) {
		WARN("Detected SPE version %d, please update test.\n", spe_ver);
	}

	if (spe_ver == ID_AA64DFR0_SPE_NOT_SUPPORTED) {
		return TEST_RESULT_SKIPPED;
	}

	/*
	 * If runtime-EL3 does not enable access to SPE system
	 * registers from NS-EL2/NS-EL1 then read of these
	 * registers traps in EL3
	 */
	read_pmscr_el1();
	read_pmsfcr_el1();
	read_pmsicr_el1();
	read_pmsidr_el1();
	read_pmsirr_el1();
	read_pmslatfr_el1();
	read_pmblimitr_el1();
	read_pmbptr_el1();
	read_pmbsr_el1();
	read_pmsevfr_el1();
	if (IS_IN_EL2()) {
		read_pmscr_el2();
	}
	if (spe_ver == ID_AA64DFR0_SPE_V1P2) {
		read_pmsnevfr_el1();
	}

	return TEST_RESULT_SUCCESS;
#endif /* __aarch64__ */
}
