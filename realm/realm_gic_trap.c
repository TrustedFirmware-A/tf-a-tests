/*
 * Copyright (c) 2025-2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <realm_tests.h>
#include <arch_helpers.h>
#include <debug.h>
#include <drivers/arm/gic_v3.h>
#include <host_shared_data.h>

/*
 * Test GIC trap handling in Realm
 *
 * This test verifies that GIC register accesses from the Realm are
 * properly trapped and handled. The test attempts to read/write various
 * GIC system registers and verifies the behavior.
 */
bool test_realm_gic_trap(void)
{
	u_register_t icc_ctlr_el1;
	u_register_t icc_pmr_el1;
	u_register_t icc_igrpen1_el1;
	u_register_t icv_ctlr_el1;
	u_register_t icv_pmr_el1;
	bool ret = true;

	realm_printf("Starting GIC trap test in Realm\n");

	/*
	 * Test 1: Read ICC_CTLR_EL1
	 * This should exit to host
	 */
	icc_ctlr_el1 = read_icc_ctlr_el1();
	realm_printf("ICC_CTLR_EL1 = 0x%lx\n", icc_ctlr_el1);

	/*
	 * Test 2: Read ICC_PMR_EL1 (Interrupt Priority Mask Register)
	 */
	icc_pmr_el1 = read_icc_pmr_el1();
	realm_printf("ICC_PMR_EL1 = 0x%lx\n", icc_pmr_el1);

	/*
	 * Test 3: Attempt to write ICC_PMR_EL1
	 * Store original value to restore later
	 * Max valid value is 0xF8, using 0xE0 for test
	 */
	write_icc_pmr_el1(0xE0);
	isb();

	/* Verify write */
	if (read_icc_pmr_el1() != 0xE0) {
		realm_printf("ICC_PMR_EL1 write test failed 0x%lx\n", read_icc_pmr_el1());
		ret = false;
	}

	/* Restore original value */
	write_icc_pmr_el1(icc_pmr_el1);
	isb();

	/*
	 * Test 4: Read ICC_IGRPEN1_EL1 (Interrupt Group 1 Enable)
	 */
	icc_igrpen1_el1 = read_icc_igrpen1_el1();
	realm_printf("ICC_IGRPEN1_EL1 = 0x%lx\n", icc_igrpen1_el1);

	/*
	 * Test 5: Write to ICC_DIR_EL1 (Deactivate Interrupt Register)
	 * Write 0 to deactivate (normally used with interrupt ID)
	 */
	write_icc_dir_el1(0x0UL);
	isb();
	realm_printf("ICC_DIR_EL1 write test completed\n");

	/*
	 * Test 6: Attempt to read ICV registers (virtual interface)
	 */
	icv_ctlr_el1 = read_icv_ctlr_el1();
	realm_printf("ICV_CTLR_EL1 = 0x%lx\n", icv_ctlr_el1);

	icv_pmr_el1 = read_icv_pmr_el1();
	realm_printf("ICV_PMR_EL1 = 0x%lx\n", icv_pmr_el1);

	/*
	 * Test 7: Write to ICC_SGI0R_EL1 (Software Generated Interrupt)
	 * This tests SGI generation from Realm, which should exit to host
	 */
	write_icc_sgi0r_el1(0x0UL);
	isb();
	realm_printf("ICC_SGI0R_EL1 write test completed\n");

	/* Return test results to host */
	realm_shared_data_set_my_realm_val(HOST_ARG1_INDEX, ret ? 1UL : 0UL);
	realm_shared_data_set_my_realm_val(HOST_ARG2_INDEX, icc_ctlr_el1);
	realm_shared_data_set_my_realm_val(HOST_ARG3_INDEX, icc_pmr_el1);

	realm_printf("GIC trap test completed: %s\n",
		     ret ? "PASSED" : "FAILED");

	return ret;
}
