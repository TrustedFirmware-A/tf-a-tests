/*
 * Copyright (c) 2018-2026, Arm Limited. All rights reserved.
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
	/* Configure EL2 state only when TFTF is executing at NS-EL2. */
	if (IS_IN_EL2()) {
		u_register_t hcr_el2 = HCR_TGE_BIT;

		/* Enable asynchronous SError aborts to EL2 */
		enable_serror();

		/*
		 * Route physical interrupts to EL2 regardless of the value of
		 * the IMO/FMO bits. Without this, interrupts would not be taken
		 * and would remain pending, regardless of the PSTATE.{A, I, F}
		 * interrupt masks. When FEAT_E2H0 is not implemented, HCR_EL2.E2H becomes RES1,
		 * but might still be writable. Make sure the bit reflects the implementation.
		 */
		if (!is_feat_e2h0_present()) {
			hcr_el2 |= HCR_E2H_BIT;
		}

		write_hcr_el2(hcr_el2);

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
