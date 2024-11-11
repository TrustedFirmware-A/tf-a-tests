/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cactus_test_cmds.h>
#include <drivers/arm/arm_gic.h>
#include <drivers/arm/gic_v3.h>
#include <ffa_endpoints.h>
#include <ffa_helpers.h>
#include <irq.h>
#include <spm_test_helpers.h>
#include <test_helpers.h>
#include <timer.h>

#define SLEEP_TIME	100U
#define SENDER		HYP_ID
#define RECEIVER	SP_ID(2)
#define EL1_PHYS_TIMER_IRQ 30

static const struct ffa_uuid expected_sp_uuids[] = {
		{PRIMARY_UUID}, {SECONDARY_UUID}
	};

static volatile int arch_timer_received;

static int arch_timer_handler(void *data)
{
	INFO("Nwd irq handler executed\n");

	/* Stop the timer now. */
	write_cntp_ctl_el0(0);

	/* Disable the EL1 physical timer interrupt and unregister the handler. */
	tftf_irq_disable(EL1_PHYS_TIMER_IRQ);
	tftf_irq_unregister_handler(EL1_PHYS_TIMER_IRQ);

	arch_timer_received = 1;

	return 0;
}

static bool check_arch_timer_handled(void)
{
	return (arch_timer_received != 0);
}

static uint64_t ms_to_ticks(uint64_t ms)
{
	return (ms * read_cntfrq_el0()) / 1000;
}

/**
 * The aim is this test is to ensure that the functionality of arch (EL1
 * physical) timer configured by NWd endpoint, such as an hypervisor, is not
 * corrupted by SPMC when an SP also configures the arch timer for its own use.
 * Any endpoint that programs a deadline through arch timer shall receive the
 * timer interrupt eventually.
 */
test_result_t test_ffa_physical_arch_timer_nwd_set_swd_preempt(void)
{
	struct ffa_value ret_values;
	int ret;
	unsigned int core_pos = get_current_core_id();

	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

	/* Register the interrupt handler for EL1 physical timer. */
	ret = tftf_irq_register_handler(EL1_PHYS_TIMER_IRQ, arch_timer_handler);
	if (ret != 0) {
		ERROR("Failed to program EL1 physical timer (%d)\n", ret);
		return TEST_RESULT_FAIL;
	}

	arch_timer_received = 0;
	tftf_irq_enable(EL1_PHYS_TIMER_IRQ, GIC_HIGHEST_NS_PRIORITY);

	/* Configure the EL1 physical timer to expire in 20ms. */
	write_cntp_ctl_el0(0);
	write_cntp_tval_el0(ms_to_ticks(20));
	write_cntp_ctl_el0(1);

	/*
	 * Send a command to SP requesting it to setup its arch timer. SP shall
	 * also wait for enough time to let the normal world interrupt trigger
	 * leading to preemption of the SP's execution context.
	 */
	ret_values = cactus_send_arch_timer_cmd(SENDER, RECEIVER, 100, 30);

	if (ffa_func_id(ret_values) != FFA_INTERRUPT) {
		ERROR("Expected FFA_INTERRUPT as return.\n");
		return TEST_RESULT_FAIL;
	}

	if (!check_arch_timer_handled()) {
		ERROR("Normal world timer interrupt hasn't actually been handled.\n");
		return TEST_RESULT_FAIL;
	}

	/*
	 * Wait for sufficient time to let the SPMC host timer tracking the
	 * SP's deadline, to trigger. SPMC will eventually queue the virtual
	 * secure timer interrupt for receiver SP since it is in PREEMPTED
	 * state.
	 */
	waitms(100);

	/*
	 * Resume the Cactus SP using FFA_RUN ABI for it to complete the
	 * sleep routine and send the direct response message.
	 */
	ret_values = ffa_run(RECEIVER, core_pos);

	if (!is_ffa_direct_response(ret_values)) {
		return TEST_RESULT_FAIL;
	}

	if (cactus_get_response(ret_values) == CACTUS_ERROR) {
		return TEST_RESULT_FAIL;
	}

	/* Check for the last serviced secure virtual interrupt. */
	ret_values = cactus_get_last_interrupt_cmd(SENDER, RECEIVER);

	if (!is_ffa_direct_response(ret_values)) {
		ERROR("Expected a direct response for last serviced interrupt"
			" command\n");
		return TEST_RESULT_FAIL;
	}

	/* Make sure arch timer interrupt was serviced. */
	if (cactus_get_response(ret_values) != TIMER_VIRTUAL_INTID) {
		ERROR("Trusted watchdog timer interrupt not serviced by SP\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
