/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <events.h>
#include <lfa.h>
#include <lfa_test_helpers.h>
#include <plat_topology.h>
#include <platform.h>
#include <power_management.h>
#include <psci.h>
#include <spinlock.h>
#include <test_helpers.h>
#include <tftf_lib.h>

static uint64_t fw_id;
static bool perform_rendezvous = false;
static bool perform_reset = false;

static event_t cpu_has_finished_test[PLATFORM_CORE_COUNT];
static event_t cpu_exit_test[PLATFORM_CORE_COUNT];

static int release = 0;
static int activate_count = 0;

const uint64_t num_ctx_regs = NUM_CTX_REGISTERS;
uint64_t ctx_registers[PLATFORM_CORE_COUNT][NUM_CTX_REGISTERS];

static test_result_t non_lead_cpu_fn_rmm(void);
static test_result_t non_lead_cpu_fn_bl31(void);

static bool lfa_test_is_target(const struct lfa_test_target *target,
			       const struct lfa_test_target *match)
{
	return lfa_test_target_matches(match, target->uuid1, target->uuid2);
}

static void lfa_test_prepare_bl31_args(smc_args *args, unsigned int core_pos)
{
	args->arg2 = 0;
	args->arg3 = (uint64_t)get_ns_ep_context;
	args->arg4 = core_pos;
}

static uintptr_t lfa_test_secondary_entrypoint(const struct lfa_test_target *target)
{
	if (lfa_test_is_target(target, &lfa_test_rmm)) {
		return (uintptr_t)non_lead_cpu_fn_rmm;
	}

	return (uintptr_t)non_lead_cpu_fn_bl31;
}

static void set_timer_period (uint64_t time_period)
{
	uint64_t counter_value;
	uint64_t timer_tick;
	uint64_t timer_ctrl_reg = read_cntp_ctl_el0();

	timer_ctrl_reg &= ~(1 << 0);
	write_cntp_ctl_el0(timer_ctrl_reg);

	if (time_period != 0) {
		timer_tick = time_period * read_cntfrq_el0();
		timer_tick = timer_tick / 10000000U;
		counter_value = read_cntpct_el0();
		write_cntp_cval_el0(counter_value + timer_tick);

		timer_ctrl_reg = read_cntp_ctl_el0();
		timer_ctrl_reg |= (1 << 0);
		write_cntp_ctl_el0(timer_ctrl_reg);
	}
}

/*
 * Test entry point functions for non-lead CPUs.
 *
 * Specified by the lead CPU when bringing up other CPUs.
 */

static test_result_t non_lead_cpu_fn_rmm(void)
{
	smc_args args = lfa_test_init_fw_args(LFA_ACTIVATE, fw_id);
	smc_ret_values ret;

	ret = tftf_smc(&args);
	if (ret.ret0 != SMC_OK) {
		tftf_testcase_printf("%s: LFA_ACTIVATE error: 0x%08lx\n",
				     __func__, ret.ret0);
		return TEST_RESULT_FAIL;
	}
	return TEST_RESULT_SUCCESS;
}

static test_result_t non_lead_cpu_fn_bl31(void)
{
	smc_args args = lfa_test_init_fw_args(LFA_ACTIVATE, fw_id);
	smc_ret_values ret;

	unsigned int mpid = read_mpidr_el1() & MPID_MASK;
	unsigned int core_pos = platform_get_core_pos(mpid);

	/*
	 * If this function is called, it means non-lead CPUs were brought on
	 * to perform CPU rendezvous. Cannot skip CPU rendezvous.
	 */
	lfa_test_prepare_bl31_args(&args, core_pos);

	/* Send signal to core 0 to show core is ready for activation. */
	atomic_add(&activate_count, 1);

	/* Wait for release signal from core 0 */
	while(atomic_read(&release) != 1);

	/* Call LFA Activate to enter CPU rendezvous holding pen. */
	if (set_ns_ep_context(core_pos) == 0) {
		/*
		 * Set a timer to wake the core up after WFI. This triggers the
		 * warm reboot if needed.
		 */
		arm_gic_intr_enable(IRQ_PCPU_NS_TIMER);
		set_timer_period(100);

		ret = tftf_smc(&args);
	} else {
		ret.ret0 = 0;
	}

	if (ret.ret0 != 0) {
		tftf_testcase_printf("%s: LFA_ACTIVATE error: 0x%08lx\n", __func__, ret.ret0);
		return TEST_RESULT_FAIL;
	}

	/* Tell core 0 that this core is finished with activate. */
	tftf_send_event(&cpu_has_finished_test[core_pos]);

	/* Exit once core 0 is ready to exit. */
	tftf_wait_for_event(&cpu_exit_test[core_pos]);

	return TEST_RESULT_SUCCESS;
}

static test_result_t test_lfa_activate_flow(const struct lfa_test_target *target)
{
	unsigned int lead_mpid;
	unsigned int cpu_mpid, cpu_node;
	unsigned int core_pos, lead_core_pos;
	int psci_ret;
	bool bl31_activation = lfa_test_is_target(target, &lfa_test_bl31);
	bool rmm_activation = lfa_test_is_target(target, &lfa_test_rmm);

	/*
	 * Reset activation count. Don't need atomic access here since this
	 * is the only core powered up at this point.
	 */
	activate_count = 0;

	lead_mpid = read_mpidr_el1() & MPID_MASK;
	lead_core_pos = platform_get_core_pos(lead_mpid);

	SKIP_TEST_IF_LESS_THAN_N_CPUS(2);

	smc_args args = { .fid = LFA_GET_INFO, .arg1 = 0 };
	smc_ret_values ret;
	uint64_t i, j;
	bool found = false;

	ret = tftf_smc(&args);
	if (ret.ret0 != SMC_OK) {
		tftf_testcase_printf("%s: LFA_GET_INFO error: 0x%08lx\n",
				     __func__, ret.ret0);
		return TEST_RESULT_FAIL;
	}
	j = ret.ret1;

	for (i = 0U; i < j; i++) {
		args.fid = LFA_GET_INVENTORY;
		args.arg1 = i;
		ret = tftf_smc(&args);
		if (ret.ret0 != 0) {
			tftf_testcase_printf("%s: LFA_GET_INVENTORY error: 0x%08lx\n",
					     __func__, ret.ret0);
			return TEST_RESULT_FAIL;
		}
		tftf_testcase_printf("ID %lld: 0x%16lx 0x%16lx - capable: %s, pending: %s\n",
			i, ret.ret1, ret.ret2,
			(ret.ret4 & 0x1) ? "yes" : "no",
			(ret.ret4 & 0x2) ? "yes" : "no");

		if (lfa_test_target_matches(target, ret.ret1, ret.ret2)) {
			found = true;
			fw_id = i;

			if (bl31_activation) {
				perform_rendezvous = ((ret.ret3 >> 3) & 0x1) ? 0 : 1;
				perform_reset = ((ret.ret3 >> 2) & 0x1);
			}
		}
	}

	if (found == false) {
		tftf_testcase_printf("%s: Firmware not found\n", __func__);
		return TEST_RESULT_SKIPPED;
	}

	args = lfa_test_init_fw_args(LFA_PRIME, fw_id);
	ret = tftf_smc(&args);
	if (ret.ret0 != SMC_OK) {
		tftf_testcase_printf("%s: LFA_PRIME error: 0x%08lx\n",
				     __func__, ret.ret0);
		return TEST_RESULT_FAIL;
	}

	if(perform_rendezvous || rmm_activation) {
		/* Power on all CPUs */
		for_each_cpu(cpu_node) {
			cpu_mpid = tftf_get_mpidr_from_node(cpu_node);
			core_pos = platform_get_core_pos(cpu_mpid);

			if (perform_rendezvous) {
				/* Initialize flow control events for BL31 rendezvous. */
				tftf_init_event(&cpu_has_finished_test[core_pos]);
				tftf_init_event(&cpu_exit_test[core_pos]);
			}

			/* Skip lead CPU as it is already powered on */
			if (cpu_mpid == lead_mpid)
				continue;

			psci_ret = tftf_cpu_on(cpu_mpid,
					       lfa_test_secondary_entrypoint(target),
					       0);

			if (psci_ret != PSCI_E_SUCCESS) {
				tftf_testcase_printf(
					"Failed to power on CPU 0x%x (%d)\n",
					cpu_mpid, psci_ret);
				return TEST_RESULT_SKIPPED;
			}
		}

		if (perform_rendezvous) {
			while(atomic_read(&activate_count) != (PLATFORM_CORE_COUNT-1));
			atomic_add(&release, 1);
		}
	}

	args = lfa_test_init_fw_args(LFA_ACTIVATE, fw_id);

	/* BL31 live activation specific stuff */
	if (bl31_activation) {
		/* Set skip rendezvous flag if allowed. */
		if(!perform_rendezvous) {
			args.arg2 = args.arg2 | 0x1;
		} else {
			args.arg2 = 0;
		}

		lfa_test_prepare_bl31_args(&args, lead_core_pos);

		if (set_ns_ep_context(lead_core_pos) == 0) {
			/* Set a timer to wake ourselves up after WFI. */
			arm_gic_intr_enable(IRQ_PCPU_NS_TIMER);
			set_timer_period(100);
			ret = tftf_smc(&args);
		} else {
			ret.ret0 = 0;
		}
	} else {
		ret = tftf_smc(&args);
	}

	if (ret.ret0 != SMC_OK) {
		tftf_testcase_printf("%s: LFA_ACTIVATE error: 0x%08lx\n", __func__, ret.ret0);
		return TEST_RESULT_FAIL;
	}

	if(bl31_activation) {
		tftf_send_event(&cpu_has_finished_test[lead_core_pos]);

		/* Wait until all CPUs have finished activation */
		for_each_cpu(cpu_node) {
			cpu_mpid = tftf_get_mpidr_from_node(cpu_node);
			core_pos = platform_get_core_pos(cpu_mpid);
			tftf_wait_for_event(&cpu_has_finished_test[core_pos]);
		}

		/* Send finish test to each core TODO probably not necessary? */
		for_each_cpu(cpu_node) {
			cpu_mpid = tftf_get_mpidr_from_node(cpu_node);
			core_pos = platform_get_core_pos(cpu_mpid);
			tftf_send_event(&cpu_exit_test[core_pos]);
		}
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * @Test_Aim@ Test RMM Live activation.
 */
test_result_t test_lfa_activate_rmm_multiple_cpu(void)
{
	return test_lfa_activate_flow(&lfa_test_rmm);
}

/*
 * @Test_Aim@ Test BL31 Live activation.
 */
test_result_t test_lfa_activate_bl31(void)
{
	return test_lfa_activate_flow(&lfa_test_bl31);
}
