/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stdlib.h>

#include <arch_helpers.h>
#include <debug.h>
#include <test_helpers.h>

#include <host_realm_helper.h>
#include <host_realm_pmu.h>
#include <platform.h>

#define MAX_COUNTERS		31

/* PMCCFILTR_EL0 mask */
#define PMCCFILTR_EL0_MASK (	  \
	PMCCFILTR_EL0_P_BIT	| \
	PMCCFILTR_EL0_U_BIT	| \
	PMCCFILTR_EL0_NSK_BIT	| \
	PMCCFILTR_EL0_NSH_BIT	| \
	PMCCFILTR_EL0_M_BIT	| \
	PMCCFILTR_EL0_RLK_BIT	| \
	PMCCFILTR_EL0_RLU_BIT	| \
	PMCCFILTR_EL0_RLH_BIT)

/* PMEVTYPER<n>_EL0 mask */
#define PMEVTYPER_EL0_MASK (	  \
	PMEVTYPER_EL0_P_BIT	| \
	PMEVTYPER_EL0_U_BIT	| \
	PMEVTYPER_EL0_NSK_BIT	| \
	PMEVTYPER_EL0_NSU_BIT	| \
	PMEVTYPER_EL0_NSH_BIT	| \
	PMEVTYPER_EL0_M_BIT	| \
	PMEVTYPER_EL0_RLK_BIT	| \
	PMEVTYPER_EL0_RLU_BIT	| \
	PMEVTYPER_EL0_RLH_BIT	| \
	PMEVTYPER_EL0_EVTCOUNT_BITS)

/* PMSELR_EL0 mask */
#define PMSELR_EL0_MASK		0x1F

#define WRITE_PMEV_REGS(n) {					\
	case n:							\
	pmu_ptr->pmevcntr_el0[n] = rand64();			\
	write_pmevcntrn_el0(n, pmu_ptr->pmevcntr_el0[n]);	\
	pmu_ptr->pmevtyper_el0[n] = rand() & PMEVTYPER_EL0_MASK;\
	write_pmevtypern_el0(n, pmu_ptr->pmevtyper_el0[n]);	\
}

#define	CHECK_PMEV_REG(n, reg) {				\
	read_val = read_##reg##n_el0(n);			\
	if (read_val != pmu_ptr->reg##_el0[n]) {		\
		ERROR("Corrupted "#reg"%d_el0=0x%lx (0x%lx)\n",	\
			n, read_val, pmu_ptr->reg##_el0[n]);	\
		return false;					\
	}							\
}

#define CHECK_PMEV_REGS(n) {		\
	case n:				\
	CHECK_PMEV_REG(n, pmevcntr);	\
	CHECK_PMEV_REG(n, pmevtyper);	\
}

#define WRITE_PMREG(reg, mask) {		\
	pmu_ptr->reg = rand64() & mask;	\
	write_##reg(pmu_ptr->reg);		\
}

#define CHECK_PMREG(reg) {					\
	read_val = read_##reg();				\
	val = pmu_ptr->reg;					\
	if (read_val != val) {					\
		ERROR("Corrupted "#reg"=0x%lx (0x%lx)\n",	\
			read_val, val);				\
		return false;					\
	}							\
}

struct pmu_registers {
	unsigned long pmcr_el0;
	unsigned long pmcntenset_el0;
	unsigned long pmovsset_el0;
	unsigned long pmintenset_el1;
	unsigned long pmccntr_el0;
	unsigned long pmccfiltr_el0;
	unsigned long pmuserenr_el0;

	unsigned long pmevcntr_el0[MAX_COUNTERS];
	unsigned long pmevtyper_el0[MAX_COUNTERS];

	unsigned long pmselr_el0;
	unsigned long pmxevcntr_el0;
	unsigned long pmxevtyper_el0;

} __aligned(CACHE_WRITEBACK_GRANULE);

static struct pmu_registers pmu_state[PLATFORM_CORE_COUNT];

void host_set_pmu_state(void)
{
	unsigned int core_pos = platform_get_core_pos(read_mpidr_el1());
	struct pmu_registers *pmu_ptr = &pmu_state[core_pos];
	unsigned int num_cnts = GET_PMU_CNT;
	unsigned long val;

	val = read_pmcr_el0() | PMCR_EL0_DP_BIT;
	pmu_ptr->pmcr_el0 = val;

	/* Disable cycle counting and reset all counters */
	write_pmcr_el0(val | PMCR_EL0_C_BIT | PMCR_EL0_P_BIT);

	/* Disable all counters */
	pmu_ptr->pmcntenset_el0 = 0UL;
	write_pmcntenclr_el0(PMU_CLEAR_ALL);

	/* Clear overflow status */
	pmu_ptr->pmovsset_el0 = 0UL;
	write_pmovsclr_el0(PMU_CLEAR_ALL);

	/* Disable overflow interrupts on all counters */
	pmu_ptr->pmintenset_el1 = 0UL;
	write_pmintenclr_el1(PMU_CLEAR_ALL);

	WRITE_PMREG(pmccntr_el0, UINT64_MAX);
	WRITE_PMREG(pmccfiltr_el0, PMCCFILTR_EL0_MASK);

	pmu_ptr->pmuserenr_el0 = read_pmuserenr_el0();

	if (num_cnts != 0U) {
		switch (--num_cnts) {
		WRITE_PMEV_REGS(30);
		WRITE_PMEV_REGS(29);
		WRITE_PMEV_REGS(28);
		WRITE_PMEV_REGS(27);
		WRITE_PMEV_REGS(26);
		WRITE_PMEV_REGS(25);
		WRITE_PMEV_REGS(24);
		WRITE_PMEV_REGS(23);
		WRITE_PMEV_REGS(22);
		WRITE_PMEV_REGS(21);
		WRITE_PMEV_REGS(20);
		WRITE_PMEV_REGS(19);
		WRITE_PMEV_REGS(18);
		WRITE_PMEV_REGS(17);
		WRITE_PMEV_REGS(16);
		WRITE_PMEV_REGS(15);
		WRITE_PMEV_REGS(14);
		WRITE_PMEV_REGS(13);
		WRITE_PMEV_REGS(12);
		WRITE_PMEV_REGS(11);
		WRITE_PMEV_REGS(10);
		WRITE_PMEV_REGS(9);
		WRITE_PMEV_REGS(8);
		WRITE_PMEV_REGS(7);
		WRITE_PMEV_REGS(6);
		WRITE_PMEV_REGS(5);
		WRITE_PMEV_REGS(4);
		WRITE_PMEV_REGS(3);
		WRITE_PMEV_REGS(2);
		WRITE_PMEV_REGS(1);
		default:
		WRITE_PMEV_REGS(0);
		}

		/* Generate a random number between 0 and num_cnts */
		val = rand() % ++num_cnts;
	} else {
		val = 0UL;
	}

	pmu_ptr->pmselr_el0 = val;
	write_pmselr_el0(val);

	pmu_ptr->pmxevcntr_el0 = read_pmxevcntr_el0();
	pmu_ptr->pmxevtyper_el0 = read_pmxevtyper_el0();
}

bool host_check_pmu_state(void)
{
	unsigned int core_pos = platform_get_core_pos(read_mpidr_el1());
	struct pmu_registers *pmu_ptr = &pmu_state[core_pos];
	unsigned int num_cnts = GET_PMU_CNT;
	unsigned long val, read_val;

	CHECK_PMREG(pmcr_el0);
	CHECK_PMREG(pmcntenset_el0);
	CHECK_PMREG(pmovsset_el0);
	CHECK_PMREG(pmintenset_el1);
	CHECK_PMREG(pmccntr_el0);
	CHECK_PMREG(pmccfiltr_el0);
	CHECK_PMREG(pmuserenr_el0);
	CHECK_PMREG(pmselr_el0);
	CHECK_PMREG(pmxevcntr_el0);
	CHECK_PMREG(pmxevtyper_el0);

	if (num_cnts != 0UL) {
		switch (--num_cnts) {
		CHECK_PMEV_REGS(30);
		CHECK_PMEV_REGS(29);
		CHECK_PMEV_REGS(28);
		CHECK_PMEV_REGS(27);
		CHECK_PMEV_REGS(26);
		CHECK_PMEV_REGS(25);
		CHECK_PMEV_REGS(24);
		CHECK_PMEV_REGS(23);
		CHECK_PMEV_REGS(22);
		CHECK_PMEV_REGS(21);
		CHECK_PMEV_REGS(20);
		CHECK_PMEV_REGS(19);
		CHECK_PMEV_REGS(18);
		CHECK_PMEV_REGS(17);
		CHECK_PMEV_REGS(16);
		CHECK_PMEV_REGS(15);
		CHECK_PMEV_REGS(14);
		CHECK_PMEV_REGS(13);
		CHECK_PMEV_REGS(12);
		CHECK_PMEV_REGS(11);
		CHECK_PMEV_REGS(10);
		CHECK_PMEV_REGS(9);
		CHECK_PMEV_REGS(8);
		CHECK_PMEV_REGS(7);
		CHECK_PMEV_REGS(6);
		CHECK_PMEV_REGS(5);
		CHECK_PMEV_REGS(4);
		CHECK_PMEV_REGS(3);
		CHECK_PMEV_REGS(2);
		CHECK_PMEV_REGS(1);
		default:
		CHECK_PMEV_REGS(0);
		}
	}

	return true;
}
