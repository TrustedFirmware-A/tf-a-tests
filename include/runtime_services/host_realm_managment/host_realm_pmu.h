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

void host_set_pmu_state(void);
bool host_check_pmu_state(void);

#endif /* HOST_REALM_PMU_H */
