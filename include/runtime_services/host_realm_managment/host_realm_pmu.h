/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef HOST_REALM_PMU_H
#define HOST_REALM_PMU_H

#include <arch_helpers.h>

/* PMU physical interrupt */
#define PMU_PPI		23UL

/* PMU virtual interrupt */
#define PMU_VIRQ	PMU_PPI

/* Clear bits P0-P30, C and F0 */
#define PMU_CLEAR_ALL	0x1FFFFFFFF

/* Number of event counters implemented */
#define GET_PMU_CNT	\
	((read_pmcr_el0() >> PMCR_EL0_N_SHIFT) & PMCR_EL0_N_MASK)

#define MAX_COUNTERS	31U

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
} __aligned(64);

void host_set_pmu_state(struct pmu_registers *pmu_ptr);

bool host_check_pmu_state(struct pmu_registers *pmu_ptr);

#endif /* HOST_REALM_PMU_H */
