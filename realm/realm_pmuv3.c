/*
 * Copyright (c) 2022-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>

#include <arch_helpers.h>
#include <arm_arch_svc.h>
#include <debug.h>
#include <drivers/arm/gic_v3.h>

#include <host_realm_pmu.h>
#include <realm_rsi.h>

/* PMUv3 events */
#define PMU_EVT_SW_INCR		0x0
#define PMU_EVT_INST_RETIRED	0x8
#define PMU_EVT_CPU_CYCLES	0x11
#define PMU_EVT_MEM_ACCESS	0x13

#define NOP_REPETITIONS		50

#define PRE_OVERFLOW		~(0xF)

#define	DELAY_MS		3000UL

static inline void read_all_counters(u_register_t *array, unsigned int num_cnts)
{
	array[0] = read_pmccntr_el0();
	for (unsigned int i = 0U; i < num_cnts; i++) {
		array[i + 1] = read_pmevcntrn_el0(i);
	}
}

static inline void read_all_counter_configs(u_register_t *array, unsigned int num_cnts)
{
	array[0] = read_pmccfiltr_el0();
	for (unsigned int i = 0U; i < num_cnts; i++) {
		array[i + 1] = read_pmevtypern_el0(i);
	}
}

static inline void read_all_pmu_configs(u_register_t *array)
{
	array[0] = read_pmcntenset_el0();
	array[1] = read_pmcr_el0();
	array[2] = read_pmselr_el0();
}

static inline void enable_counting(void)
{
	write_pmcr_el0(read_pmcr_el0() | PMCR_EL0_E_BIT);
	/* This function means we are about to use the PMU, synchronize */
	isb();
}

static inline void disable_counting(void)
{
	write_pmcr_el0(read_pmcr_el0() & ~PMCR_EL0_E_BIT);
	/* We also rely that disabling really did work */
	isb();
}

static inline void clear_counters(void)
{
	write_pmcr_el0(read_pmcr_el0() | PMCR_EL0_C_BIT | PMCR_EL0_P_BIT);
	isb();
}

static void pmu_reset(void)
{
	/* Reset all counters */
	write_pmcr_el0(read_pmcr_el0() |
			PMCR_EL0_DP_BIT | PMCR_EL0_C_BIT | PMCR_EL0_P_BIT);

	/* Disable all counters */
	write_pmcntenclr_el0(PMU_CLEAR_ALL);

	/* Clear overflow status */
	write_pmovsclr_el0(PMU_CLEAR_ALL);

	/* Disable overflow interrupts on all counters */
	write_pmintenclr_el1(PMU_CLEAR_ALL);
	isb();
}

/*
 * This test runs in Realm EL1, don't bother enabling counting at lower ELs
 * and secure world. TF-A has other controls for them and counting there
 * doesn't impact us.
 */
static inline void enable_cycle_counter(void)
{
	/*
	 * Set PMCCFILTR_EL0.U != PMCCFILTR_EL0.RLU
	 * to disable counting in Realm EL0.
	 * Set PMCCFILTR_EL0.P = PMCCFILTR_EL0.RLK
	 * to enable counting in Realm EL1.
	 * Set PMCCFILTR_EL0.NSH = PMCCFILTR_EL0_EL0.RLH
	 * to disable event counting in Realm EL2.
	 */
	write_pmccfiltr_el0(PMCCFILTR_EL0_U_BIT |
			    PMCCFILTR_EL0_P_BIT | PMCCFILTR_EL0_RLK_BIT |
			    PMCCFILTR_EL0_NSH_BIT | PMCCFILTR_EL0_RLH_BIT);
	write_pmcntenset_el0(read_pmcntenset_el0() | PMCNTENSET_EL0_C_BIT);
	isb();
}

static inline void enable_event_counter(unsigned int ctr_num)
{
	/*
	 * Set PMEVTYPER_EL0.U != PMEVTYPER_EL0.RLU
	 * to disable event counting in Realm EL0.
	 * Set PMEVTYPER_EL0.P = PMEVTYPER_EL0.RLK
	 * to enable counting in Realm EL1.
	 * Set PMEVTYPER_EL0.NSH = PMEVTYPER_EL0.RLH
	 * to disable event counting in Realm EL2.
	 */
	write_pmevtypern_el0(ctr_num,
			PMEVTYPER_EL0_U_BIT |
			PMEVTYPER_EL0_P_BIT | PMEVTYPER_EL0_RLK_BIT |
			PMEVTYPER_EL0_NSH_BIT | PMEVTYPER_EL0_RLH_BIT |
			(PMU_EVT_INST_RETIRED & PMEVTYPER_EL0_EVTCOUNT_BITS));
	write_pmcntenset_el0(read_pmcntenset_el0() |
		PMCNTENSET_EL0_P_BIT(ctr_num));
	isb();
}

/* Doesn't really matter what happens, as long as it happens a lot */
static inline void execute_nops(void)
{
	for (unsigned int i = 0U; i < NOP_REPETITIONS; i++) {
		__asm__ ("orr x0, x0, x0\n");
	}
}

/*
 * Try the cycle counter with some NOPs to see if it works
 */
bool test_pmuv3_cycle_works_realm(void)
{
	u_register_t ccounter_start;
	u_register_t ccounter_end;

	pmu_reset();

	enable_cycle_counter();
	enable_counting();

	ccounter_start = read_pmccntr_el0();
	execute_nops();
	ccounter_end = read_pmccntr_el0();
	disable_counting();
	clear_counters();

	realm_printf("cycle counter counted from %lu to %lu\n",
			ccounter_start, ccounter_end);
	return (ccounter_start != ccounter_end);
}

/* Test if max counter available is same as that programmed by host */
bool test_pmuv3_counter(void)
{
	unsigned int num_cnts, num_cnts_host;

	num_cnts_host = realm_shared_data_get_my_host_val(HOST_ARG1_INDEX);
	num_cnts = GET_PMU_CNT;
	realm_printf("CPU=%u num_cnts=%u num_cnts_host=%u\n", read_mpidr_el1() & MPID_MASK,
			num_cnts, num_cnts_host);
	return (num_cnts == num_cnts_host);
}

/*
 * Try an event counter with some NOPs to see if it works.
 */
bool test_pmuv3_event_works_realm(void)
{
	u_register_t evcounter_start;
	u_register_t evcounter_end;
	unsigned int num_cnts = GET_PMU_CNT;
	unsigned int ctr_num;

	/* Seed the random number generator */
	srand((unsigned int)read_cntpct_el0());

	/* Select a random number of event counter */
	ctr_num = (unsigned int)rand() % num_cnts;

	pmu_reset();

	enable_event_counter(ctr_num);
	enable_counting();

	/*
	 * If any is enabled it will be in the first range.
	 */
	evcounter_start = read_pmevcntrn_el0(ctr_num);
	execute_nops();
	disable_counting();
	evcounter_end = read_pmevcntrn_el0(ctr_num);
	clear_counters();

	realm_printf("event counter #%u counted from %lu to %lu\n",
			ctr_num, evcounter_start, evcounter_end);
	if (evcounter_start != evcounter_end) {
		return true;
	}
	return false;
}

/*
 * Check if entering/exiting RMM (with a NOP) preserves all PMU registers.
 */
bool test_pmuv3_rmm_preserves(void)
{
	u_register_t ctr_start[MAX_COUNTERS] = {0};
	u_register_t ctr_cfg_start[MAX_COUNTERS] = {0};
	u_register_t pmu_cfg_start[3];
	u_register_t ctr_end[MAX_COUNTERS] = {0};
	u_register_t ctr_cfg_end[MAX_COUNTERS] = {0};
	u_register_t pmu_cfg_end[3];
	unsigned int num_cnts = GET_PMU_CNT;

	if (num_cnts  == 0U) {
		realm_printf("testing cycle counter\n");
	} else {
		realm_printf("testing %u event counters\n", num_cnts);
	}

	pmu_reset();

	/* Pretend all counters have just been used */
	enable_cycle_counter();

	for (unsigned int i = 0U; i < num_cnts; i++) {
		enable_event_counter(i);
	}

	enable_counting();
	execute_nops();
	disable_counting();

	/* Get before reading */
	read_all_counters(ctr_start, num_cnts);
	read_all_counter_configs(ctr_cfg_start, num_cnts);
	read_all_pmu_configs(pmu_cfg_start);

	/* Give RMM a chance to scramble everything */
	(void)rsi_get_version(RSI_ABI_VERSION_VAL);

	/* Get after reading */
	read_all_counters(ctr_end, num_cnts);
	read_all_counter_configs(ctr_cfg_end, num_cnts);
	read_all_pmu_configs(pmu_cfg_end);

	if (memcmp(ctr_start, ctr_end, sizeof(ctr_start)) != 0) {
		realm_printf("SMC call did not preserve %s\n",
				"counters");
		return false;
	}

	if (memcmp(ctr_cfg_start, ctr_cfg_end, sizeof(ctr_cfg_start)) != 0) {
		realm_printf("SMC call did not preserve %s\n",
				"counter config");
		return false;
	}

	if (memcmp(pmu_cfg_start, pmu_cfg_end, sizeof(pmu_cfg_start)) != 0) {
		realm_printf("SMC call did not preserve %s\n",
				"PMU registers");
		return false;
	}

	return true;
}

bool test_pmuv3_overflow_interrupt(bool cycle_cnt)
{
	unsigned long priority_bits, priority;
	unsigned long delay_time = DELAY_MS;
	unsigned int num_cnts, ctr_num;

	pmu_reset();

	/* Get the number of priority bits implemented */
	priority_bits = ((read_icv_ctrl_el1() >> ICV_CTLR_EL1_PRIbits_SHIFT) &
				ICV_CTLR_EL1_PRIbits_MASK) + 1UL;

	/* Unimplemented bits are RES0 and start from LSB */
	priority = (0xFFUL << (8UL - priority_bits)) & 0xFFUL;

	/* Set the priority mask register to allow all interrupts */
	write_icv_pmr_el1(priority);

	/* Enable Virtual Group 1 interrupts */
	write_icv_igrpen1_el1(ICV_IGRPEN1_EL1_Enable);

	/* Enable IRQ */
	enable_irq();

	if (cycle_cnt) {
		write_pmccntr_el0(PRE_OVERFLOW);
		enable_cycle_counter();

		/* Enable interrupt on cycle counter */
		write_pmintenset_el1(PMINTENSET_EL1_C_BIT);
		realm_printf("waiting for PMU cycle counter vIRQ...\n");
	} else {
		num_cnts = GET_PMU_CNT;

		/* Seed the random number generator */
		srand((unsigned int)read_cntpct_el0());

		/* Select a random number of event counter */
		ctr_num = (unsigned int)rand() % num_cnts;

		write_pmevcntrn_el0(ctr_num, PRE_OVERFLOW);
		enable_event_counter(ctr_num);

		/* Enable interrupt on event counter */
		write_pmintenset_el1(PMINTENSET_EL1_P_BIT(ctr_num));
		realm_printf("waiting for PMU event counter #%u vIRQ...\n",
				ctr_num);
	}

	enable_counting();
	execute_nops();

	/*
	 * Interrupt handler will clear
	 * Performance Monitors Interrupt Enable Set register
	 * as part of handling the overflow interrupt.
	 */
	while ((read_pmintenset_el1() != 0UL) && (delay_time != 0UL)) {
		--delay_time;
	}

	/* Disable IRQ */
	disable_irq();

	pmu_reset();

	if (delay_time == 0UL) {
		realm_printf("PMU vIRQ %sreceived in %lums\n",	"not ",
				DELAY_MS);
		return false;
	}

	realm_printf("PMU vIRQ %sreceived in %lums\n", "",
			DELAY_MS - delay_time);
	return true;
}
