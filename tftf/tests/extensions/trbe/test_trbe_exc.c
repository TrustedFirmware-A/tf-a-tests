/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifdef __aarch64__
#include <test_helpers.h>
#include <tftf.h>
#include <tftf_lib.h>
#include <sync.h>
#else
#include <test_helpers.h>
#include <tftf_lib.h>
#endif

#define TRACE_BUF_MGMT_EVT U(2)

#ifdef __aarch64__
static volatile test_result_t result = TEST_RESULT_FAIL;

static bool trbe_exc_exception_handler(void)
{
	uint64_t esr_el2 = read_esr_el2();
	uint64_t ec = EC_BITS(esr_el2);
	uint64_t iss = ISS_BITS(esr_el2);
	uint64_t fsc = (iss >> 1) & ISS_FSC_MASK;

	/* Ensure that the exception is what we expect */
	if (ec == EC_PROFILING && fsc == TRACE_BUF_MGMT_EVT) {
		result = TEST_RESULT_SUCCESS;
	}

	/* Disable trace buffer unit so TRBSR_EL2 write is effected */
	write_trblimitr_el1(read_trblimitr_el1() & ~TRBLIMITR_EL1_E_BIT);
	isb();
	/* Clear the exception */
	write_trbsr_el1(0);

	return true;
}
#endif /* __aarch64__ */

/*
 * Tests if MDCR_EL3.TRBEE is correctly configured (nonzero) using architected
 * aliasing behaviour and exception handling.
 */
test_result_t test_trbe_exc_enabled(void)
{
	SKIP_TEST_IF_AARCH32();

#ifdef __aarch64__
	SKIP_TEST_IF_TRBE_NOT_SUPPORTED();
	SKIP_TEST_IF_TRBE_EXC_NOT_SUPPORTED();

	if (IS_IN_EL1()) {
		tftf_testcase_printf("Test not supported in EL1.\n");
		return TEST_RESULT_SKIPPED;
	}

	uint64_t saved_trblimitr_el1 = read_trblimitr_el1();
	uint64_t saved_trfcr_el2 = read_trfcr_el2();
	uint64_t saved_hcr_el2 = read_hcr_el2();

	/* Register an exception handler and disable IRQ - see below */
	register_custom_sync_exception_handler(trbe_exc_exception_handler);
	enable_debug_exceptions();
	disable_irq();

	/*
	 * Temporarily disable the trace buffer unit, as writes to TRBSR_ELx
	 * can be ignored whilst the TBU is active (RDJMDD)
	 */
	write_trblimitr_el1(
		saved_trblimitr_el1 &
		~(TRBLIMITR_EL1_E_BIT | TRBLIMITR_EL1_XE_BIT)
	);
	isb();

	/*
	 * All lower-level TRBE management events should be recorded in
	 * TRBSR_EL2 and exception delivery from EL2 is enabled.
	 *
	 * If MDCR_EL3.TRBEE is zero, then EE is effectively 0.
	 */
	write_trfcr_el2(
		saved_trfcr_el2 |
		TRFCR_EL2_EE(TRFCR_EL2_EE_TRAP_ALL) |
		TRFCR_EL2_KE_BIT
	);

	/*
	 * Enables VHE host. If TRFCR_EL2.EE is effectively nonzero, then
	 * TRBSR_EL1 accesses TRBSR_EL2.
	 */
	write_hcr_el2(saved_hcr_el2 | HCR_E2H_BIT);
	isb();

	/*
	 * This will test if MDCR_EL3.TRBEE is correctly configured to allow
	 * TRBE management events to be reported as exceptions.
	 *
	 * We know that EL2 is implemented and enabled, and that TRFCR_EL2.EE
	 * is set to 0b11. This instruction sets the IRQ bit, which serves to
	 * indicate that a management event has occurred. Combined with the
	 * above, one of two things will happen:
	 *
	 *  - TRBSR_EL1.IRQ = 1: The TRBIRQ signal is asserted and masked.
	 *  - TRBSR_EL2.IRQ = 1: A pending TRBE profiling exception is set.
	 */
	write_trbsr_el1(TRBSR_IRQ_BIT);
	isb();
	/* Re-enable TBU */
	write_trblimitr_el1(read_trblimitr_el1() | TRBLIMITR_EL1_E_BIT);
	isb();

	/*
	 * Next instruction observes the pending exception, if it exists.
	 *
	 * See RYXTTY.
	 */
	__asm__ volatile ("nop");

	unregister_custom_sync_exception_handler();
	disable_debug_exceptions();
	enable_irq();

	/* Restore context */
	write_trblimitr_el1(saved_trblimitr_el1);
	write_trfcr_el2(saved_trfcr_el2);
	write_hcr_el2(saved_hcr_el2);

	return result;
#endif  /* __aarch64__ */
}
