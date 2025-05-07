/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>
#include <tftf_lib.h>
#include <rdaspen_cpu_ras_helpers.h>

/*
 * Bit 57 - SBE_BITPOS_VALID
 * Bit 47 - OFO Sticky Overflow
 * Bit 40 - CECO Corrected error Count
 * Bit 0  - 001 Unit L1 cache instruction
 * To Inject a single-bit, correctable RAS fault in the L1-I cache during
 * self-test or error-handling validation.
 */
#define ERX_MISC0_CE_INJ 0x200810000000001ULL

test_result_t test_cpu_ras_ce(void)
{
	uint64_t erx_status;

	/* Select Error Record 1, Error record 0 is for the DSU */
	write_errselr_el1(1);

	/* Enable FHI, CFI and allow Errors to generate interrupts */
	write_erxctlr_el1(ERXCTLR_CFI_BIT | ERXCTLR_FI_BIT |
			  ERXCTLR_ED_BIT);

	/* Clear PFG control register */
	write_cpu_erxpfgctl_el1(0U);

	/* Clear the status register , rewriting ERX_STATUS to itself clears it */
	erx_status = read_erxstatus_el1();
	write_erxstatus_el1(erx_status);

	/* Enable the PFG counter */
	enable_cpu_pfg_cdn_register();

	/* Program the Misc0 register to attach a sticky overflow corrected error */
	write_erxmisc0_el1(read_erxmisc0_el1() | ERX_MISC0_CE_INJ);

	/* Writes the CE bit to Arm the PFG CTRL to inject CE */
	write_cpu_pfg_ctrl_register_ce();

	INFO("ErrStatus = 0x%lx\n", read_erxstatus_el1());

	INFO("Corrected ErrStatus by FFH = 0x%lx\n", read_erxstatus_el1());

	return TEST_RESULT_SUCCESS;
}

test_result_t test_cpu_ras_de(void)
{
	uint64_t erx_status;

	/* Select Error Record 1, Error record 0 is for the DSU */
	write_errselr_el1(1);

	/* Enable FHI, CFI and allow Errors to generate interrupts */
	write_erxctlr_el1(ERXCTLR_CFI_BIT | ERXCTLR_FI_BIT |
			  ERXCTLR_ED_BIT);

	/* Clear PFG control register */
	write_cpu_erxpfgctl_el1(0U);

	/* Clear the status register , rewriting ERX_STATUS to itself clears it */
	erx_status = read_erxstatus_el1();
	write_erxstatus_el1(erx_status);

	/* Enable the PFG counter */
	enable_cpu_pfg_cdn_register();

	/* Writes the UC bit to Arm the PFG CTRL to inject DE */
	write_cpu_pfg_ctrl_register_de();

	INFO("ErrStatus = 0x%lx\n", read_erxstatus_el1());

	INFO("Deferred ErrStatus by FFH = 0x%lx\n", read_erxstatus_el1());

	return TEST_RESULT_SUCCESS;
}
