/*
 * Copyright (c) 2018-2019, Arm Limited. All rights reserved.
 * Copyright (c) 2020, NVIDIA Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_features.h>
#include <arch_helpers.h>
#include <arch_features.h>
#include <tftf_lib.h>

void tftf_arch_setup(void)
{
	/* Do not try to configure EL2 if TFTF is running at NS-EL1 */
	if (IS_IN_EL2()) {
		/* Enable asynchronous SError aborts to EL2 */
		enable_serror();

		/*
		 * Route physical interrupts to EL2 regardless of the value of
		 * the IMO/FMO bits. Without this, interrupts would not be taken
		 * and would remain pending, regardless of the PSTATE.{A, I, F}
		 * interrupt masks.
		 */
		write_hcr_el2(HCR_TGE_BIT);

		/*
		 * Disable trap of SVE, SME instructions to EL2.
		 * The fields of the CPTR_EL2 register reset to an
		 * architecturally UNKNOWN value.
		 */
		if (EL2_IS_IN_HOST()) {
			write_cptr_el2(CPACR_EL1_RESET_VAL);
		} else {
			write_cptr_el2(CPTR_EL2_RESET_VAL);
		}
		isb();

		/*
		 * Enable access to ZT0 storage when FEAT_SME2 is implemented
		 * and enable FA64 when FEAT_SME_FA64 is implemented
		 */
		if (is_feat_sme_supported()) {
			write_smcr_el2(SMCR_EL2_RESET_VAL);
			isb();
		}

		/* Clear SVE hint bit */
		if (is_armv8_2_sve_present()) {
			tftf_smc_set_sve_hint(false);
		}
	}
}
