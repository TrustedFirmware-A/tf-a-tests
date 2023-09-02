/*
 * Copyright (c) 2021-2023, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stdio.h>

#include <arch.h>
#include <arch_features.h>
#include <arch_helpers.h>
#include <debug.h>
#include <lib/extensions/sme.h>

/*
 * Function: sme_smstart
 * This function enables streaming mode and ZA array storage access
 * independently or together based on the type of instruction variant.
 *
 * Parameters
 * smstart_type: If SMSTART, streaming mode and ZA access is enabled.
 *               If SMSTART_SM, streaming mode enabled.
 *               If SMSTART_ZA enables SME ZA storage and, ZT0 storage access.
 */
void sme_smstart(smestart_instruction_type_t smstart_type)
{
	u_register_t svcr = 0ULL;

	switch (smstart_type) {
	case SMSTART:
		svcr = (SVCR_SM_BIT | SVCR_ZA_BIT);
		break;

	case SMSTART_SM:
		svcr = SVCR_SM_BIT;
		break;

	case SMSTART_ZA:
		svcr = SVCR_ZA_BIT;
		break;

	default:
		ERROR("Illegal SMSTART Instruction Variant\n");
		break;
	}
	write_svcr(read_svcr() | svcr);

	isb();
}

/*
 * sme_smstop
 * This function exits streaming mode and disables ZA array storage access
 * independently or together based on the type of instruction variant.
 *
 * Parameters
 * smstop_type: If SMSTOP, exits streaming mode and ZA access is disabled
 *              If SMSTOP_SM, exits streaming mode.
 *              If SMSTOP_ZA disables SME ZA storage and, ZT0 storage access.
 */
void sme_smstop(smestop_instruction_type_t smstop_type)
{
	u_register_t svcr = 0ULL;

	switch (smstop_type) {
	case SMSTOP:
		svcr = (~SVCR_SM_BIT) & (~SVCR_ZA_BIT);
		break;

	case SMSTOP_SM:
		svcr = ~SVCR_SM_BIT;
		break;

	case SMSTOP_ZA:
		svcr = ~SVCR_ZA_BIT;
		break;

	default:
		ERROR("Illegal SMSTOP Instruction Variant\n");
		break;
	}
	write_svcr(read_svcr() & svcr);

	isb();
}
