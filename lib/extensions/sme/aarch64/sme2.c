/*
 * Copyright (c) 2023, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stdio.h>

#include <arch.h>
#include <arch_features.h>
#include <arch_helpers.h>
#include <lib/extensions/sme.h>
#include <tftf_lib.h>

/*
 * sme2_enable
 * Enable SME2 for nonsecure use at EL2 for TFTF cases.
 */
void sme2_enable(void)
{
	u_register_t reg;

	/* SME2 is an extended version of SME.
	 * Therefore, SME accesses still must be taken care by setting
	 * appropriate fields in order to avoid traps in CPTR_EL2.
	 */
	sme_enable();

	/*
	 * Make sure ZT0 register access don't cause traps by setting
	 * appropriate field in SMCR_EL2 register.
	 */
	reg = read_smcr_el2();
	reg |= SMCR_ELX_EZT0_BIT;
	write_smcr_el2(reg);
	isb();

}

