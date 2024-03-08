/*
 * Copyright (c) 2021-2024, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stdio.h>

#include <arch.h>
#include <arch_features.h>
#include <arch_helpers.h>
#include <assert.h>
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

/* Set the Streaming SVE vector length (SVL) in the SMCR_EL2 register */
void sme_config_svq(uint32_t svq)
{
	u_register_t smcr_el2_val;

	/* Cap svq to arch supported max value. */
	if (svq > SME_SVQ_ARCH_MAX) {
		svq = SME_SVQ_ARCH_MAX;
	}

	smcr_el2_val = read_smcr_el2();

	smcr_el2_val &= ~(MASK(SMCR_ELX_LEN));
	smcr_el2_val |= INPLACE(SMCR_ELX_LEN, svq);

	write_smcr_el2(smcr_el2_val);
	isb();
}

static void set_smcr_fa64(bool enable)
{
	if (enable) {
		write_smcr_el2(read_smcr_el2() | SMCR_ELX_FA64_BIT);
	} else {
		write_smcr_el2(read_smcr_el2() & ~SMCR_ELX_FA64_BIT);
	}

	isb();
}

/*
 * Enable FEAT_SME_FA64, This control causes all implemented A64 instructions
 * to be treated as legal in Streaming SVE mode at EL2, if they are treated as
 * legal at EL3.
 */
void sme_enable_fa64(void)
{
	return set_smcr_fa64(true);
}

/*
 * Disable FEAT_SME_FA64, This control does not cause any instruction to be
 * treated as legal in Streaming SVE mode.
 */
void sme_disable_fa64(void)
{
	return set_smcr_fa64(false);
}

/* Returns 'true' if the CPU is in Streaming SVE mode */
bool sme_smstat_sm(void)
{
	return ((read_svcr() & SVCR_SM_BIT) != 0U);
}

bool sme_feat_fa64_enabled(void)
{
	return ((read_smcr_el2() & SMCR_ELX_FA64_BIT) != 0U);
}

uint32_t sme_probe_svl(uint8_t sme_max_svq)
{
	uint32_t svl_bitmap = 0;
	uint8_t svq, rdsvl_vq;

	/* Cap svq to arch supported max value. */
	if (sme_max_svq > SME_SVQ_ARCH_MAX) {
		sme_max_svq = SME_SVQ_ARCH_MAX;
	}

	for (svq = 0; svq <= sme_max_svq; svq++) {
		sme_config_svq(svq);
		rdsvl_vq = SME_SVL_TO_SVQ(sme_rdsvl_1());
		if (svl_bitmap & BIT_32(rdsvl_vq)) {
			continue;
		}
		svl_bitmap |= BIT_32(rdsvl_vq);
	}

	return svl_bitmap;
}
