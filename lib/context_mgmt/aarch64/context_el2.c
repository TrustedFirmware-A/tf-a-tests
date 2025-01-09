/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/context_mgmt/context_el2.h>

/**
 * Internal functions to save groups of EL2 registers.
 */

static void el2_save_common_registers(el2_common_regs_t *ctx)
{
	EL2_SAVE_CTX_REG(ctx, actlr_el2);
	EL2_SAVE_CTX_REG(ctx, afsr0_el2);
	EL2_SAVE_CTX_REG(ctx, afsr1_el2);
	EL2_SAVE_CTX_REG(ctx, amair_el2);
	EL2_SAVE_CTX_REG(ctx, cnthctl_el2);
	EL2_SAVE_CTX_REG(ctx, cntvoff_el2);
	EL2_SAVE_CTX_REG(ctx, cptr_el2);
	/* Accessing DBGVCR32_EL2 may crash the test, so it is omitted. */
	EL2_SAVE_CTX_REG(ctx, elr_el2);
	EL2_SAVE_CTX_REG(ctx, esr_el2);
	EL2_SAVE_CTX_REG(ctx, far_el2);
	EL2_SAVE_CTX_REG(ctx, hacr_el2);
	EL2_SAVE_CTX_REG(ctx, hcr_el2);
	EL2_SAVE_CTX_REG(ctx, hpfar_el2);
	EL2_SAVE_CTX_REG(ctx, hstr_el2);
	EL2_SAVE_CTX_REG(ctx, icc_sre_el2);
	EL2_SAVE_CTX_REG(ctx, ich_hcr_el2);
	EL2_SAVE_CTX_REG(ctx, ich_vmcr_el2);
	EL2_SAVE_CTX_REG(ctx, mair_el2);
	EL2_SAVE_CTX_REG(ctx, mdcr_el2);
	EL2_SAVE_CTX_REG(ctx, pmscr_el2);
	EL2_SAVE_CTX_REG(ctx, sctlr_el2);
	EL2_SAVE_CTX_REG(ctx, spsr_el2);
	EL2_SAVE_CTX_SP(ctx);
	EL2_SAVE_CTX_REG(ctx, tcr_el2);
	EL2_SAVE_CTX_REG(ctx, tpidr_el2);
	EL2_SAVE_CTX_REG(ctx, ttbr0_el2);
	EL2_SAVE_CTX_REG(ctx, vbar_el2);
	EL2_SAVE_CTX_REG(ctx, vmpidr_el2);
	EL2_SAVE_CTX_REG(ctx, vpidr_el2);
	EL2_SAVE_CTX_REG(ctx, vtcr_el2);
	EL2_SAVE_CTX_REG(ctx, vttbr_el2);
}

static void el2_save_mte2_registers(el2_mte2_regs_t *ctx)
{
	if (get_armv8_5_mte_support() == 2) {
		EL2_SAVE_CTX_REG(ctx, tfsr_el2);
	}
}

static void el2_save_fgt_registers(el2_fgt_regs_t *ctx)
{
	if (is_armv8_6_fgt_present()) {
		EL2_SAVE_CTX_REG(ctx, hdfgrtr_el2);
		if (is_feat_amuv1_present())
			EL2_SAVE_CTX_REG(ctx, hafgrtr_el2);
		EL2_SAVE_CTX_REG(ctx, hdfgwtr_el2);
		EL2_SAVE_CTX_REG(ctx, hfgitr_el2);
		EL2_SAVE_CTX_REG(ctx, hfgrtr_el2);
		EL2_SAVE_CTX_REG(ctx, hfgwtr_el2);
	}
}

static void el2_save_fgt2_registers(el2_fgt2_regs_t *ctx)
{
	if (is_armv8_9_fgt2_present()) {
		EL2_SAVE_CTX_REG(ctx, hdfgrtr2_el2);
		EL2_SAVE_CTX_REG(ctx, hdfgwtr2_el2);
		EL2_SAVE_CTX_REG(ctx, hfgitr2_el2);
		EL2_SAVE_CTX_REG(ctx, hfgrtr2_el2);
		EL2_SAVE_CTX_REG(ctx, hfgwtr2_el2);
	}
}

static void el2_save_ecv_registers(el2_ecv_regs_t *ctx)
{
	if (get_armv8_6_ecv_support() == ID_AA64MMFR0_EL1_ECV_SELF_SYNCH) {
		EL2_SAVE_CTX_REG(ctx, cntpoff_el2);
	}
}

static void el2_save_vhe_registers(el2_vhe_regs_t *ctx)
{
	if (is_armv8_1_vhe_present()) {
		EL2_SAVE_CTX_REG(ctx, contextidr_el2);
		EL2_SAVE_CTX_REG(ctx, ttbr1_el2);
	}
}

static void el2_save_ras_registers(el2_ras_regs_t *ctx)
{
	if (is_feat_ras_present() || is_feat_rasv1p1_present()) {
		EL2_SAVE_CTX_REG(ctx, vdisr_el2);
		EL2_SAVE_CTX_REG(ctx, vsesr_el2);
	}
}

static void el2_save_neve_registers(el2_neve_regs_t *ctx)
{
	if (is_armv8_4_nv2_present()) {
		EL2_SAVE_CTX_REG(ctx, vncr_el2);
	}
}

static void el2_save_trf_registers(el2_trf_regs_t *ctx)
{
	if (get_armv8_4_trf_support()) {
		EL2_SAVE_CTX_REG(ctx, trfcr_el2);
	}
}

static void el2_save_csv2_registers(el2_csv2_regs_t *ctx)
{
	if (is_feat_csv2_2_present()) {
		EL2_SAVE_CTX_REG(ctx, scxtnum_el2);
	}
}

static void el2_save_hcx_registers(el2_hcx_regs_t *ctx)
{
	if (get_feat_hcx_support()) {
		EL2_SAVE_CTX_REG(ctx, hcrx_el2);
	}
}

static void el2_save_tcr2_registers(el2_tcr2_regs_t *ctx)
{
	if (is_feat_tcr2_supported()) {
		EL2_SAVE_CTX_REG(ctx, tcr2_el2);
	}
}

static void el2_save_sxpoe_registers(el2_sxpoe_regs_t *ctx)
{
	if (is_feat_sxpoe_present()) {
		EL2_SAVE_CTX_REG(ctx, por_el2);
	}
}

static void el2_save_sxpie_registers(el2_sxpie_regs_t *ctx)
{
	if (is_feat_sxpie_present()) {
		EL2_SAVE_CTX_REG(ctx, pire0_el2);
		EL2_SAVE_CTX_REG(ctx, pir_el2);
	}
}

static void el2_save_s2pie_registers(el2_s2pie_regs_t *ctx)
{
	if (is_feat_s2pie_present()) {
		EL2_SAVE_CTX_REG(ctx, s2pir_el2);
	}
}

static void el2_save_gcs_registers(el2_gcs_regs_t *ctx)
{
	if (is_feat_gcs_present()) {
		EL2_SAVE_CTX_REG(ctx, gcscr_el2);
		EL2_SAVE_CTX_REG(ctx, gcspr_el2);
	}
}

static void el2_save_mpam_registers(el2_mpam_regs_t *ctx)
{
	if (is_feat_mpam_supported()) {
		EL2_SAVE_CTX_REG(ctx, mpam2_el2);
		/**
		 * Registers MPAMHCR_EL2, MPAMVPM0_EL2, MPAMVPM1_EL2, MPAMVPM2_EL2,
		 * MPAMVPM3_EL2, MPAMVPM4_EL2, MPAMVPM5_EL2, MPAMVPM6_EL2, MPAMVPM7_EL2
		 * and MPAMVPMV_EL2 are not included, because an exception is raised
		 * upon accessing them.
		 */
	}
}

/******************************************************************************/

/**
 * Internal functions to corrupt/restore groups of EL2 registers.
 */

static void el2_write_common_registers_with_mask(const el2_common_regs_t *ctx, uint64_t or_mask)
{
	EL2_WRITE_MASK_CTX_REG(ctx, actlr_el2, or_mask);
	EL2_WRITE_MASK_CTX_REG(ctx, afsr0_el2, or_mask);
	EL2_WRITE_MASK_CTX_REG(ctx, afsr1_el2, or_mask);
	EL2_WRITE_MASK_CTX_REG(ctx, amair_el2, or_mask);
	EL2_WRITE_MASK_CTX_REG(ctx, cnthctl_el2, or_mask);
	EL2_WRITE_MASK_CTX_REG(ctx, cntvoff_el2, or_mask);
	EL2_WRITE_MASK_CTX_REG(ctx, cptr_el2, or_mask);
	/* Accessing DBGVCR32_EL2 may crash the test, so it is omitted. */
	EL2_WRITE_MASK_CTX_REG(ctx, elr_el2, or_mask);
	EL2_WRITE_MASK_CTX_REG(ctx, esr_el2, or_mask);
	EL2_WRITE_MASK_CTX_REG(ctx, far_el2, or_mask);
	EL2_WRITE_MASK_CTX_REG(ctx, hacr_el2, or_mask);
	EL2_WRITE_MASK_CTX_REG(ctx, hcr_el2, or_mask);
	EL2_WRITE_MASK_CTX_REG(ctx, hpfar_el2, or_mask);
	EL2_WRITE_MASK_CTX_REG(ctx, hstr_el2, or_mask);
	EL2_WRITE_MASK_CTX_REG(ctx, icc_sre_el2, or_mask);
	EL2_WRITE_MASK_CTX_REG(ctx, ich_hcr_el2, or_mask);
	/* Unable to restore ICH_VMCR_EL2 to original value after modification, so it is omitted. */
	EL2_WRITE_MASK_CTX_REG(ctx, mair_el2, or_mask);
	EL2_WRITE_MASK_CTX_REG(ctx, mdcr_el2, or_mask);
	EL2_WRITE_MASK_CTX_REG(ctx, pmscr_el2, or_mask);
	EL2_WRITE_MASK_CTX_REG(ctx, sctlr_el2, (or_mask & ~SCTLR_EL2_EE));
	EL2_WRITE_MASK_CTX_REG(ctx, spsr_el2, or_mask);
	/* The stack pointer should not be modified, because the control flow depends on it. */
	/* Masking TCR_EL2 may crash the test, so it is omitted. */
	EL2_WRITE_MASK_CTX_REG(ctx, tpidr_el2, or_mask);
	/* Masking TTBR0_EL2 may crash the test, so it is omitted. */
	EL2_WRITE_MASK_CTX_REG(ctx, vbar_el2, or_mask);
	EL2_WRITE_MASK_CTX_REG(ctx, vmpidr_el2, or_mask);
	EL2_WRITE_MASK_CTX_REG(ctx, vpidr_el2, or_mask);
	EL2_WRITE_MASK_CTX_REG(ctx, vtcr_el2, or_mask);
	EL2_WRITE_MASK_CTX_REG(ctx, vttbr_el2, or_mask);
}

static void el2_write_mte2_registers_with_mask(const el2_mte2_regs_t *ctx, uint64_t or_mask)
{
	if (get_armv8_5_mte_support() == 2) {
		EL2_WRITE_MASK_CTX_REG(ctx, tfsr_el2, or_mask);
	}
}

static void el2_write_fgt_registers_with_mask(const el2_fgt_regs_t *ctx, uint64_t or_mask)
{
	if (is_armv8_6_fgt_present()) {
		EL2_WRITE_MASK_CTX_REG(ctx, hdfgrtr_el2, or_mask);
		if (is_feat_amuv1_present())
			EL2_WRITE_MASK_CTX_REG(ctx, hafgrtr_el2, or_mask);
		EL2_WRITE_MASK_CTX_REG(ctx, hdfgwtr_el2, or_mask);
		EL2_WRITE_MASK_CTX_REG(ctx, hfgitr_el2, or_mask);
		EL2_WRITE_MASK_CTX_REG(ctx, hfgrtr_el2, or_mask);
		EL2_WRITE_MASK_CTX_REG(ctx, hfgwtr_el2, or_mask);
	}
}

static void el2_write_fgt2_registers_with_mask(const el2_fgt2_regs_t *ctx, uint64_t or_mask)
{
	if (is_armv8_9_fgt2_present()) {
		EL2_WRITE_MASK_CTX_REG(ctx, hdfgrtr2_el2, or_mask);
		EL2_WRITE_MASK_CTX_REG(ctx, hdfgwtr2_el2, or_mask);
		EL2_WRITE_MASK_CTX_REG(ctx, hfgitr2_el2, or_mask);
		EL2_WRITE_MASK_CTX_REG(ctx, hfgrtr2_el2, or_mask);
		EL2_WRITE_MASK_CTX_REG(ctx, hfgwtr2_el2, or_mask);
	}
}

static void el2_write_ecv_registers_with_mask(const el2_ecv_regs_t *ctx, uint64_t or_mask)
{
	if (get_armv8_6_ecv_support() == ID_AA64MMFR0_EL1_ECV_SELF_SYNCH) {
		EL2_WRITE_MASK_CTX_REG(ctx, cntpoff_el2, or_mask);
	}
}

static void el2_write_vhe_registers_with_mask(const el2_vhe_regs_t *ctx, uint64_t or_mask)
{
	if (is_armv8_1_vhe_present()) {
		EL2_WRITE_MASK_CTX_REG(ctx, contextidr_el2, or_mask);
		EL2_WRITE_MASK_CTX_REG(ctx, ttbr1_el2, (or_mask & ~TTBR1_EL2_ASID));
	}
}

static void el2_write_ras_registers_with_mask(const el2_ras_regs_t *ctx, uint64_t or_mask)
{
	if (is_feat_ras_present() || is_feat_rasv1p1_present()) {
		EL2_WRITE_MASK_CTX_REG(ctx, vdisr_el2, or_mask);
		EL2_WRITE_MASK_CTX_REG(ctx, vsesr_el2, or_mask);
	}
}

static void el2_write_neve_registers_with_mask(const el2_neve_regs_t *ctx, uint64_t or_mask)
{
	if (is_armv8_4_nv2_present()) {
		EL2_WRITE_MASK_CTX_REG(ctx, vncr_el2, or_mask);
	}
}

static void el2_write_trf_registers_with_mask(const el2_trf_regs_t *ctx, uint64_t or_mask)
{
	if (get_armv8_4_trf_support()) {
		EL2_WRITE_MASK_CTX_REG(ctx, trfcr_el2, or_mask);
	}
}

static void el2_write_csv2_registers_with_mask(const el2_csv2_regs_t *ctx, uint64_t or_mask)
{
	if (is_feat_csv2_2_present()) {
		EL2_WRITE_MASK_CTX_REG(ctx, scxtnum_el2, or_mask);
	}
}

static void el2_write_hcx_registers_with_mask(const el2_hcx_regs_t *ctx, uint64_t or_mask)
{
	if (get_feat_hcx_support()) {
		EL2_WRITE_MASK_CTX_REG(ctx, hcrx_el2, or_mask);
	}
}

static void el2_write_tcr2_registers_with_mask(const el2_tcr2_regs_t *ctx, uint64_t or_mask)
{
	if (is_feat_tcr2_supported()) {
		EL2_WRITE_MASK_CTX_REG(ctx, tcr2_el2, (or_mask & ~TCR2_EL2_POE));
	}
}

static void el2_write_sxpoe_registers_with_mask(const el2_sxpoe_regs_t *ctx, uint64_t or_mask)
{
	if (is_feat_sxpoe_present()) {
		EL2_WRITE_MASK_CTX_REG(ctx, por_el2, or_mask);
	}
}

static void el2_write_sxpie_registers_with_mask(const el2_sxpie_regs_t *ctx, uint64_t or_mask)
{
	if (is_feat_sxpie_present()) {
		EL2_WRITE_MASK_CTX_REG(ctx, pire0_el2, or_mask);
		EL2_WRITE_MASK_CTX_REG(ctx, pir_el2, or_mask);
	}
}

static void el2_write_s2pie_registers_with_mask(const el2_s2pie_regs_t *ctx, uint64_t or_mask)
{
	if (is_feat_s2pie_present()) {
		EL2_WRITE_MASK_CTX_REG(ctx, s2pir_el2, or_mask);
	}
}

static void el2_write_gcs_registers_with_mask(const el2_gcs_regs_t *ctx, uint64_t or_mask)
{
	if (is_feat_gcs_present()) {
		EL2_WRITE_MASK_CTX_REG(ctx, gcscr_el2, (or_mask & ~GCSCR_EL2_PCRSEL));
		EL2_WRITE_MASK_CTX_REG(ctx, gcspr_el2, or_mask);
	}
}

/**
 * Register MPAM2_EL2 is not included, because its contents may change
 * before context switching occurs.
 *
 * Registers MPAMHCR_EL2, MPAMVPM0_EL2, MPAMVPM1_EL2, MPAMVPM2_EL2,
 * MPAMVPM3_EL2, MPAMVPM4_EL2, MPAMVPM5_EL2, MPAMVPM6_EL2, MPAMVPM7_EL2
 * and MPAMVPMV_EL2 are not included, because an exception is raised
 * upon accessing them.
 */

/******************************************************************************/

/**
 * Internal functions to dump the saved EL2 register context.
 */

static void el2_dump_common_register_context(const el2_common_regs_t *ctx)
{
	EL2_PRINT_CTX_HEADING("Common registers");
	EL2_PRINT_CTX_MEMBER(ctx, "ACTLR_EL2", actlr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "AFSR0_EL2", afsr0_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "AFSR1_EL2", afsr1_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "AMAIR_EL2", amair_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "CNTHCTL_EL2", cnthctl_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "CNTVOFF_EL2", cntvoff_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "CPTR_EL2", cptr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "ELR_EL2", elr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "ESR_EL2", esr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "FAR_EL2", far_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "HACR_EL2", hacr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "HCR_EL2", hcr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "HPFAR_EL2", hpfar_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "HSTR_EL2", hstr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "ICC_SRE_EL2", icc_sre_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "ICH_HCR_EL2", ich_hcr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "ICH_VMCR_EL2", ich_vmcr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "MAIR_EL2", mair_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "MDCR_EL2", mdcr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "PMSCR_EL2", pmscr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "SCTLR_EL2", sctlr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "SPSR_EL2", spsr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "SP_EL2", sp_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "TCR_EL2", tcr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "TPIDR_EL2", tpidr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "TTBR0_EL2", ttbr0_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "VBAR_EL2", vbar_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "VMPIDR_EL2", vmpidr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "VPIDR_EL2", vpidr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "VTCR_EL2", vtcr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "VTTBR_EL2", vttbr_el2);
	INFO("\n");
}

static void el2_dump_mte2_register_context(const el2_mte2_regs_t *ctx)
{
	EL2_PRINT_CTX_HEADING("MTE2 registers");
	EL2_PRINT_CTX_MEMBER(ctx, "TFSR_EL2", tfsr_el2);
	INFO("\n");
}

static void el2_dump_fgt_register_context(const el2_fgt_regs_t *ctx)
{
	EL2_PRINT_CTX_HEADING("FGT registers");
	EL2_PRINT_CTX_MEMBER(ctx, "HDFGRTR_EL2", hdfgrtr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "HAFGRTR_EL2", hafgrtr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "HDFGWTR_EL2", hdfgwtr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "HFGITR_EL2", hfgitr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "HFGRTR_EL2", hfgrtr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "HFGWTR_EL2", hfgwtr_el2);
	INFO("\n");
}

static void el2_dump_fgt2_register_context(const el2_fgt2_regs_t *ctx)
{
	EL2_PRINT_CTX_HEADING("FGT2 registers");
	EL2_PRINT_CTX_MEMBER(ctx, "HDFGRTR2_EL2", hdfgrtr2_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "HDFGWTR2_EL2", hdfgwtr2_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "HFGITR2_EL2", hfgitr2_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "HFGRTR2_EL2", hfgrtr2_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "HFGWTR2_EL2", hfgwtr2_el2);
	INFO("\n");
}

static void el2_dump_ecv_register_context(const el2_ecv_regs_t *ctx)
{
	EL2_PRINT_CTX_HEADING("ECV registers");
	EL2_PRINT_CTX_MEMBER(ctx, "CNTPOFF_EL2", cntpoff_el2);
	INFO("\n");
}

static void el2_dump_vhe_register_context(const el2_vhe_regs_t *ctx)
{
	EL2_PRINT_CTX_HEADING("VHE registers");
	EL2_PRINT_CTX_MEMBER(ctx, "CONTEXTIDR_EL2", contextidr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "TTBR1_EL2", ttbr1_el2);
	INFO("\n");
}

static void el2_dump_ras_register_context(const el2_ras_regs_t *ctx)
{
	EL2_PRINT_CTX_HEADING("RAS registers");
	EL2_PRINT_CTX_MEMBER(ctx, "VDISR_EL2", vdisr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "VSESR_EL2", vsesr_el2);
	INFO("\n");
}

static void el2_dump_neve_register_context(const el2_neve_regs_t *ctx)
{
	EL2_PRINT_CTX_HEADING("NEVE registers");
	EL2_PRINT_CTX_MEMBER(ctx, "VNCR_EL2", vncr_el2);
	INFO("\n");
}

static void el2_dump_trf_register_context(const el2_trf_regs_t *ctx)
{
	EL2_PRINT_CTX_HEADING("TRF registers");
	EL2_PRINT_CTX_MEMBER(ctx, "TRFCR_EL2", trfcr_el2);
	INFO("\n");
}

static void el2_dump_csv2_register_context(const el2_csv2_regs_t *ctx)
{
	EL2_PRINT_CTX_HEADING("CSV2 registers");
	EL2_PRINT_CTX_MEMBER(ctx, "SCXTNUM_EL2", scxtnum_el2);
	INFO("\n");
}

static void el2_dump_hcx_register_context(const el2_hcx_regs_t *ctx)
{
	EL2_PRINT_CTX_HEADING("HCX registers");
	EL2_PRINT_CTX_MEMBER(ctx, "HCRX_EL2", hcrx_el2);
	INFO("\n");
}

static void el2_dump_tcr2_register_context(const el2_tcr2_regs_t *ctx)
{
	EL2_PRINT_CTX_HEADING("TCR2 registers");
	EL2_PRINT_CTX_MEMBER(ctx, "TCR2_EL2", tcr2_el2);
	INFO("\n");
}

static void el2_dump_sxpoe_register_context(const el2_sxpoe_regs_t *ctx)
{
	EL2_PRINT_CTX_HEADING("SxPOE registers");
	EL2_PRINT_CTX_MEMBER(ctx, "POR_EL2", por_el2);
	INFO("\n");
}

static void el2_dump_sxpie_register_context(const el2_sxpie_regs_t *ctx)
{
	EL2_PRINT_CTX_HEADING("SxPIE registers");
	EL2_PRINT_CTX_MEMBER(ctx, "PIRE0_EL2", pire0_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "PIR_EL2", pir_el2);
	INFO("\n");
}

static void el2_dump_s2pie_register_context(const el2_s2pie_regs_t *ctx)
{
	EL2_PRINT_CTX_HEADING("S2PIE registers");
	EL2_PRINT_CTX_MEMBER(ctx, "S2PIR_EL2", s2pir_el2);
	INFO("\n");
}

static void el2_dump_gcs_register_context(const el2_gcs_regs_t *ctx)
{
	EL2_PRINT_CTX_HEADING("GCS registers");
	EL2_PRINT_CTX_MEMBER(ctx, "GCSCR_EL2", gcscr_el2);
	EL2_PRINT_CTX_MEMBER(ctx, "GCSPR_EL2", gcspr_el2);
	INFO("\n");
}

static void el2_dump_mpam_register_context(const el2_mpam_regs_t *ctx)
{
	EL2_PRINT_CTX_HEADING("MPAM registers");
	EL2_PRINT_CTX_MEMBER(ctx, "MPAM2_EL2", mpam2_el2);
	INFO("\n");
}

/******************************************************************************/

/**
 * Public function definitions.
 */

/**
 * Read EL2 system registers and save the values into the given context pointer.
 */
void el2_save_registers(el2_sysregs_t *ctx)
{
	el2_save_common_registers(&ctx->common);
	el2_save_mte2_registers(&ctx->mte2);
	el2_save_fgt_registers(&ctx->fgt);
	el2_save_fgt2_registers(&ctx->fgt2);
	el2_save_ecv_registers(&ctx->ecv);
	el2_save_vhe_registers(&ctx->vhe);
	el2_save_ras_registers(&ctx->ras);
	el2_save_neve_registers(&ctx->neve);
	el2_save_trf_registers(&ctx->trf);
	el2_save_csv2_registers(&ctx->csv2);
	el2_save_hcx_registers(&ctx->hcx);
	el2_save_tcr2_registers(&ctx->tcr2);
	el2_save_sxpoe_registers(&ctx->sxpoe);
	el2_save_sxpie_registers(&ctx->sxpie);
	el2_save_s2pie_registers(&ctx->s2pie);
	el2_save_gcs_registers(&ctx->gcs);
	el2_save_mpam_registers(&ctx->mpam);
}

/**
 * Modify EL2 system registers from a given context pointer
 * by applying the appropriate OR mask depending on the operation.
 */
void el2_modify_registers(const el2_sysregs_t *ctx, const el2_modify_operation_t op)
{
	uint64_t or_mask = (op == CORRUPT_REGS) ? REG_CORRUPTION_MASK : 0;
	el2_write_common_registers_with_mask(&ctx->common, or_mask);
	el2_write_mte2_registers_with_mask(&ctx->mte2, or_mask);
	el2_write_fgt_registers_with_mask(&ctx->fgt, or_mask);
	el2_write_fgt2_registers_with_mask(&ctx->fgt2, or_mask);
	el2_write_ecv_registers_with_mask(&ctx->ecv, or_mask);
	el2_write_vhe_registers_with_mask(&ctx->vhe, or_mask);
	el2_write_ras_registers_with_mask(&ctx->ras, or_mask);
	el2_write_neve_registers_with_mask(&ctx->neve, or_mask);
	el2_write_trf_registers_with_mask(&ctx->trf, or_mask);
	el2_write_csv2_registers_with_mask(&ctx->csv2, or_mask);
	el2_write_hcx_registers_with_mask(&ctx->hcx, or_mask);
	el2_write_tcr2_registers_with_mask(&ctx->tcr2, or_mask);
	el2_write_sxpoe_registers_with_mask(&ctx->sxpoe, or_mask);
	el2_write_sxpie_registers_with_mask(&ctx->sxpie, or_mask);
	el2_write_s2pie_registers_with_mask(&ctx->s2pie, or_mask);
	el2_write_gcs_registers_with_mask(&ctx->gcs, or_mask);
	/* There is nothing to do in regards to MPAM registers. */
}

/**
 * Dump (print out) the EL2 system registers from a given context pointer.
 */
void el2_dump_register_context(const char *ctx_name, const el2_sysregs_t *ctx)
{
	INFO("%s:\n", ctx_name);
	el2_dump_common_register_context(&ctx->common);
	el2_dump_mte2_register_context(&ctx->mte2);
	el2_dump_fgt_register_context(&ctx->fgt);
	el2_dump_fgt2_register_context(&ctx->fgt2);
	el2_dump_ecv_register_context(&ctx->ecv);
	el2_dump_vhe_register_context(&ctx->vhe);
	el2_dump_ras_register_context(&ctx->ras);
	el2_dump_neve_register_context(&ctx->neve);
	el2_dump_trf_register_context(&ctx->trf);
	el2_dump_csv2_register_context(&ctx->csv2);
	el2_dump_hcx_register_context(&ctx->hcx);
	el2_dump_tcr2_register_context(&ctx->tcr2);
	el2_dump_sxpoe_register_context(&ctx->sxpoe);
	el2_dump_sxpie_register_context(&ctx->sxpie);
	el2_dump_s2pie_register_context(&ctx->s2pie);
	el2_dump_gcs_register_context(&ctx->gcs);
	el2_dump_mpam_register_context(&ctx->mpam);
}
