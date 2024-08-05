/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_features.h>
#include <arch_helpers.h>
#include <lib/context_mgmt/context_el1.h>

#define EL1_CTX_CORRUPT_MASK		ULL(0xffff0000)
#define EL1_CTX_RESTORE_MASK		ULL(0x0)

/**
 * ---------------------------------------------------------------
 * Private Helper functions to save EL1 system context registers.
 * ---------------------------------------------------------------
 */
static void save_el1_ctx_common_regs(const el1_ctx_regs_t *ctx)
{
	el1_common_regs_t *el1_common = get_el1_common_regs_ctx(ctx);

	write_el1_ctx_reg(el1_common, spsr_el1, "spsr_el1", read_spsr_el1());
	write_el1_ctx_reg(el1_common, elr_el1, "elr_el1", read_elr_el1());
	write_el1_ctx_reg(el1_common, sctlr_el1, "sctlr_el1", read_sctlr_el1());
	write_el1_ctx_reg(el1_common, tcr_el1, "tcr_el1", read_tcr_el1());
	write_el1_ctx_reg(el1_common, cpacr_el1, "cpacr_el1", read_cpacr_el1());
	write_el1_ctx_reg(el1_common, csselr_el1, "csselr_el1", read_csselr_el1());
	write_el1_ctx_reg(el1_common, sp_el1, "sp_el1", read_sp_el1());
	write_el1_ctx_reg(el1_common, esr_el1, "esr_el1", read_esr_el1());
	write_el1_ctx_reg(el1_common, ttbr0_el1, "ttbr0_el1", read_ttbr0_el1());
	write_el1_ctx_reg(el1_common, ttbr1_el1, "ttbr1_el1", read_ttbr1_el1());
	write_el1_ctx_reg(el1_common, mair_el1, "mair_el1", read_mair_el1());
	write_el1_ctx_reg(el1_common, amair_el1, "amair_el1", read_amair_el1());
	write_el1_ctx_reg(el1_common, actlr_el1, "actlr_el1", read_actlr_el1());
	write_el1_ctx_reg(el1_common, tpidr_el1, "tpidr_el1", read_tpidr_el1());
	write_el1_ctx_reg(el1_common, tpidr_el0, "tpidr_el0", read_tpidr_el0());
	write_el1_ctx_reg(el1_common, tpidrro_el0, "tpidrro_el0", read_tpidrro_el0());
	write_el1_ctx_reg(el1_common, par_el1, "par_el1", read_par_el1());
	write_el1_ctx_reg(el1_common, far_el1, "far_el1", read_far_el1());
	write_el1_ctx_reg(el1_common, afsr0_el1, "afsr0_el1", read_afsr0_el1());
	write_el1_ctx_reg(el1_common, afsr1_el1, "afsr1_el1", read_afsr1_el1());
	write_el1_ctx_reg(el1_common, contextidr_el1, "contextidr_el1", read_contextidr_el1());
	write_el1_ctx_reg(el1_common, vbar_el1, "vbar_el1", read_vbar_el1());
	write_el1_ctx_reg(el1_common, mdccint_el1, "mdccint_el1", read_mdccint_el1());
	write_el1_ctx_reg(el1_common, mdscr_el1, "mdscr_el1", read_mdscr_el1());
}

static void save_el1_ctx_aarch32_regs(const el1_ctx_regs_t *ctx)
{
#if CTX_INCLUDE_AARCH32_REGS
	el1_aarch32_regs_t *el1_aarch32 = get_el1_aarch32_regs_ctx(ctx);

	write_el1_ctx_reg(el1_aarch32, spsr_abt, "spsr_abt", read_spsr_abt());
	write_el1_ctx_reg(el1_aarch32, spsr_und, "spsr_und", read_spsr_und());
	write_el1_ctx_reg(el1_aarch32, spsr_irq, "spsr_irq", read_spsr_irq());
	write_el1_ctx_reg(el1_aarch32, spsr_fiq, "spsr_fiq", read_spsr_fiq());
	write_el1_ctx_reg(el1_aarch32, dacr32_el2, "dacr32_el2", read_dacr32_el2());
	write_el1_ctx_reg(el1_aarch32, ifsr32_el2, "ifsr32_el2", read_ifsr32_el2());
#endif
}

static void save_el1_ctx_timer_regs(const el1_ctx_regs_t *ctx)
{
	el1_arch_timer_regs_t *el1_arch_timer = get_el1_arch_timer_regs_ctx(ctx);

	write_el1_ctx_reg(el1_arch_timer, cntp_ctl_el0, "cntp_ctl_el0", read_cntp_ctl_el0());
	write_el1_ctx_reg(el1_arch_timer, cntp_cval_el0, "cntp_cval_el0", read_cntp_cval_el0());
	write_el1_ctx_reg(el1_arch_timer, cntv_ctl_el0, "cntv_ctl_el0", read_cntv_ctl_el0());
	write_el1_ctx_reg(el1_arch_timer, cntv_cval_el0, "cntv_cval_el0", read_cntv_cval_el0());
	write_el1_ctx_reg(el1_arch_timer, cntkctl_el1, "cntkctl_el1", read_cntkctl_el1());
}

static void save_el1_ctx_mte2_regs(const el1_ctx_regs_t *ctx)
{
	if (is_feat_mte2_present()) {
		el1_mte2_regs_t *el1_mte2 = get_el1_mte2_regs_ctx(ctx);

		write_el1_ctx_reg(el1_mte2, tfsre0_el1, "tfsre0_el1", read_tfsre0_el1());
		write_el1_ctx_reg(el1_mte2, tfsr_el1, "tfsr_el1", read_tfsr_el1());
		write_el1_ctx_reg(el1_mte2, rgsr_el1, "rgsr_el1", read_rgsr_el1());
		write_el1_ctx_reg(el1_mte2, gcr_el1, "gcr_el1", read_gcr_el1());
	}
}

static void save_el1_ctx_ras_regs(const el1_ctx_regs_t *ctx)
{
	if (is_feat_ras_present()) {
		el1_ras_regs_t *el1_ras = get_el1_ras_regs_ctx(ctx);

		write_el1_ctx_reg(el1_ras, disr_el1, "disr_el1", read_disr_el1());
	}
}

static void save_el1_ctx_s1pie_regs(const el1_ctx_regs_t *ctx)
{
	if (is_feat_s1pie_present()) {
		el1_s1pie_regs_t *el1_s1pie = get_el1_s1pie_regs_ctx(ctx);
		write_el1_ctx_reg(el1_s1pie, pire0_el1, "pire0_el1", read_pire0_el1());
		write_el1_ctx_reg(el1_s1pie, pir_el1, "pir_el1", read_pir_el1());
	}
}

static void save_el1_ctx_s1poe_regs(const el1_ctx_regs_t *ctx)
{
	if (is_feat_s1poe_present()) {
		el1_s1poe_regs_t *el1_s1poe = get_el1_s1poe_regs_ctx(ctx);

		write_el1_ctx_reg(el1_s1poe, por_el1, "por_el1", read_por_el1());
	}
}

static void save_el1_ctx_s2poe_regs(const el1_ctx_regs_t *ctx)
{
	if (is_feat_s2poe_present()) {
		el1_s2poe_regs_t *el1_s2poe = get_el1_s2poe_regs_ctx(ctx);

		write_el1_ctx_reg(el1_s2poe, s2por_el1, "s2por_el1", read_s2por_el1());
	}
}

static void save_el1_ctx_tcr2_regs(const el1_ctx_regs_t *ctx)
{
	if (is_feat_tcr2_supported()) {
		el1_tcr2_regs_t *el1_tcr2 = get_el1_tcr2_regs_ctx(ctx);

		write_el1_ctx_reg(el1_tcr2, tcr2_el1, "tcr2_el1", read_tcr2_el1());
	}
}

static void save_el1_ctx_trf_regs(const el1_ctx_regs_t *ctx)
{
	if (get_armv8_4_trf_support()) {
		el1_trf_regs_t *el1_trf = get_el1_trf_regs_ctx(ctx);

		write_el1_ctx_reg(el1_trf, trfcr_el1, "trfcr_el1", read_trfcr_el1());
	}
}

static void save_el1_ctx_csv2_2_regs(const el1_ctx_regs_t *ctx)
{
	if (is_feat_csv2_2_present()) {
		el1_csv2_2_regs_t *el1_csv2_2 = get_el1_csv2_2_regs_ctx(ctx);

		write_el1_ctx_reg(el1_csv2_2, scxtnum_el0, "scxtnum_el0", read_scxtnum_el0());
		write_el1_ctx_reg(el1_csv2_2, scxtnum_el1, "scxtnum_el1", read_scxtnum_el1());
	}
}

static void save_el1_ctx_gcs_regs(const el1_ctx_regs_t *ctx)
{
	if (is_feat_gcs_present()) {
		el1_gcs_regs_t *el1_gcs = get_el1_gcs_regs_ctx(ctx);

		write_el1_ctx_reg(el1_gcs, gcscr_el1, "gcscr_el1", read_gcscr_el1());
		write_el1_ctx_reg(el1_gcs, gcscre0_el1, "gcscre0_el1", read_gcscre0_el1());
		write_el1_ctx_reg(el1_gcs, gcspr_el1, "gcspr_el1", read_gcspr_el1());
		write_el1_ctx_reg(el1_gcs, gcspr_el0, "gcspr_el0", read_gcspr_el0());
	}
}

/**
 * ------------------------------------------------------------------------
 * Private Helper functions to modify/restore EL1 system context registers.
 * ------------------------------------------------------------------------
 */
static void write_el1_ctx_common_regs(const el1_ctx_regs_t *ctx, uint64_t reg_mask)
{
	el1_common_regs_t *el1_common = get_el1_common_regs_ctx(ctx);

	write_spsr_el1((el1_common->spsr_el1.reg_value) | reg_mask);
	write_elr_el1((el1_common->elr_el1.reg_value) | reg_mask);
	write_sctlr_el1((el1_common->sctlr_el1.reg_value) | reg_mask);
	write_tcr_el1((el1_common->tcr_el1.reg_value) | reg_mask);
	write_cpacr_el1((el1_common->cpacr_el1.reg_value) | reg_mask);
	write_csselr_el1((el1_common->csselr_el1.reg_value) | reg_mask);
	write_sp_el1((el1_common->sp_el1.reg_value) | reg_mask);
	write_esr_el1((el1_common->esr_el1.reg_value) | reg_mask);
	write_ttbr0_el1((el1_common->ttbr0_el1.reg_value) | reg_mask);
	write_ttbr1_el1((el1_common->ttbr1_el1.reg_value) | reg_mask);
	write_mair_el1((el1_common->mair_el1.reg_value) | reg_mask);
	write_amair_el1((el1_common->amair_el1.reg_value) | reg_mask);
	write_actlr_el1((el1_common->actlr_el1.reg_value) | reg_mask);
	write_tpidr_el1((el1_common->tpidr_el1.reg_value) | reg_mask);
	write_tpidr_el0((el1_common->tpidr_el0.reg_value) | reg_mask);
	write_tpidrro_el0((el1_common->tpidrro_el0.reg_value) | reg_mask);
	write_par_el1((el1_common->par_el1.reg_value) | reg_mask);
	write_far_el1((el1_common->far_el1.reg_value) | reg_mask);
	write_afsr0_el1((el1_common->afsr0_el1.reg_value) | reg_mask);
	write_afsr1_el1((el1_common->afsr1_el1.reg_value) | reg_mask);
	write_contextidr_el1((el1_common->contextidr_el1.reg_value) | reg_mask);
	write_vbar_el1((el1_common->vbar_el1.reg_value) | reg_mask);
	write_mdccint_el1((el1_common->mdccint_el1.reg_value) | reg_mask);
	write_mdscr_el1((el1_common->mdscr_el1.reg_value) | reg_mask);
}

static void write_el1_ctx_aarch32_regs(const el1_ctx_regs_t *ctx, uint64_t reg_mask)
{
#if CTX_INCLUDE_AARCH32_REGS
	el1_aarch32_regs_t *el1_aarch32 = get_el1_aarch32_regs_ctx(ctx);

	write_spsr_abt((el1_aarch32->spsr_abt.reg_value) | reg_mask);
	write_spsr_und((el1_aarch32->spsr_und.reg_value) | reg_mask);
	write_spsr_irq((el1_aarch32->spsr_irq.reg_value) | reg_mask);
	write_spsr_fiq((el1_aarch32->spsr_fiq.reg_value) | reg_mask);
	write_dacr32_el2((el1_aarch32->dacr32_el2.reg_value) | reg_mask);
	write_ifsr32_el2((el1_aarch32->ifsr32_el2.reg_value) | reg_mask);
#endif
}

static void write_el1_ctx_timer_regs(const el1_ctx_regs_t *ctx, uint64_t reg_mask)
{
	el1_arch_timer_regs_t *el1_arch_timer = get_el1_arch_timer_regs_ctx(ctx);

	write_cntp_ctl_el0((el1_arch_timer->cntp_ctl_el0.reg_value) | reg_mask);
	write_cntp_cval_el0((el1_arch_timer->cntp_cval_el0.reg_value) | reg_mask);
	write_cntv_ctl_el0((el1_arch_timer->cntv_ctl_el0.reg_value) | reg_mask);
	write_cntv_cval_el0((el1_arch_timer->cntv_cval_el0.reg_value) | reg_mask);
	write_cntkctl_el1((el1_arch_timer->cntkctl_el1.reg_value) | reg_mask);
}

static void write_el1_ctx_mte2_regs(const el1_ctx_regs_t *ctx, uint64_t reg_mask)
{
	if (is_feat_mte2_present()) {
		el1_mte2_regs_t *el1_mte2 = get_el1_mte2_regs_ctx(ctx);

		write_tfsre0_el1((el1_mte2->tfsre0_el1.reg_value) | reg_mask);
		write_tfsr_el1((el1_mte2->tfsr_el1.reg_value) | reg_mask);
		write_rgsr_el1((el1_mte2->rgsr_el1.reg_value) | reg_mask);
		write_gcr_el1((el1_mte2->gcr_el1.reg_value) | reg_mask);
	}
}

static void write_el1_ctx_ras_regs(const el1_ctx_regs_t *ctx, uint64_t reg_mask)
{
	if (is_feat_ras_present()) {
		el1_ras_regs_t *el1_ras = get_el1_ras_regs_ctx(ctx);

		write_disr_el1((el1_ras->disr_el1.reg_value) | reg_mask);
	}
}

static void write_el1_ctx_s1pie_regs(const el1_ctx_regs_t *ctx, uint64_t reg_mask)
{
	if (is_feat_s1pie_present()) {
		el1_s1pie_regs_t *el1_s1pie = get_el1_s1pie_regs_ctx(ctx);

		write_pire0_el1((el1_s1pie->pire0_el1.reg_value) | reg_mask);
		write_pir_el1((el1_s1pie->pir_el1.reg_value) | reg_mask);
	}
}

static void write_el1_ctx_s1poe_regs(const el1_ctx_regs_t *ctx, uint64_t reg_mask)
{
	if (is_feat_s1poe_present()) {
		el1_s1poe_regs_t *el1_s1poe = get_el1_s1poe_regs_ctx(ctx);

		write_por_el1((el1_s1poe->por_el1.reg_value) | reg_mask);
	}
}

static void write_el1_ctx_s2poe_regs(const el1_ctx_regs_t *ctx, uint64_t reg_mask)
{
	if (is_feat_s2poe_present()) {
		el1_s2poe_regs_t *el1_s2poe = get_el1_s2poe_regs_ctx(ctx);

		write_s2por_el1((el1_s2poe->s2por_el1.reg_value) | reg_mask);
	}
}

static void write_el1_ctx_tcr2_regs(const el1_ctx_regs_t *ctx, uint64_t reg_mask)
{
	if (is_feat_tcr2_supported()) {
		el1_tcr2_regs_t *el1_tcr2 = get_el1_tcr2_regs_ctx(ctx);

		write_tcr2_el1((el1_tcr2->tcr2_el1.reg_value) | reg_mask);
	}
}

static void write_el1_ctx_trf_regs(const el1_ctx_regs_t *ctx, uint64_t reg_mask)
{
	if (get_armv8_4_trf_support()) {
		el1_trf_regs_t *el1_trf = get_el1_trf_regs_ctx(ctx);

		write_trfcr_el1((el1_trf->trfcr_el1.reg_value) | reg_mask);
	}
}

static void write_el1_ctx_csv2_2_regs(const el1_ctx_regs_t *ctx, uint64_t reg_mask)
{
	if (is_feat_csv2_2_present()) {
		el1_csv2_2_regs_t *el1_csv2_2 = get_el1_csv2_2_regs_ctx(ctx);

		write_scxtnum_el0((el1_csv2_2->scxtnum_el0.reg_value) | reg_mask);
		write_scxtnum_el1((el1_csv2_2->scxtnum_el1.reg_value) | reg_mask);
	}
}

static void write_el1_ctx_gcs_regs(const el1_ctx_regs_t *ctx, uint64_t reg_mask)
{
	if (is_feat_gcs_present()) {
		el1_gcs_regs_t *el1_gcs = get_el1_gcs_regs_ctx(ctx);

		write_gcscr_el1((el1_gcs->gcscr_el1.reg_value) | reg_mask);
		write_gcscre0_el1((el1_gcs->gcscre0_el1.reg_value) | reg_mask);
		write_gcspr_el1((el1_gcs->gcspr_el1.reg_value) | reg_mask);
		write_gcspr_el0((el1_gcs->gcspr_el0.reg_value) | reg_mask);
	}
}

/**
 * ------------------------------------------------------------------
 * Private helper functions to Print each sub structure of the main
 * EL1 system context structure.
 * ------------------------------------------------------------------
 */
static void print_el1_ctx_common_sysregs(const el1_ctx_regs_t *el1_ctx)
{
	el1_common_regs_t *el1_common = get_el1_common_regs_ctx(el1_ctx);

	PRINT_CTX_MEMBER(el1_common, spsr_el1);
	PRINT_CTX_MEMBER(el1_common, elr_el1);
	PRINT_CTX_MEMBER(el1_common, sctlr_el1);
	PRINT_CTX_MEMBER(el1_common, tcr_el1);
	PRINT_CTX_MEMBER(el1_common, cpacr_el1);
	PRINT_CTX_MEMBER(el1_common, csselr_el1);
	PRINT_CTX_MEMBER(el1_common, sp_el1);
	PRINT_CTX_MEMBER(el1_common, esr_el1);
	PRINT_CTX_MEMBER(el1_common, ttbr0_el1);
	PRINT_CTX_MEMBER(el1_common, ttbr1_el1);
	PRINT_CTX_MEMBER(el1_common, mair_el1);
	PRINT_CTX_MEMBER(el1_common, amair_el1);
	PRINT_CTX_MEMBER(el1_common, actlr_el1);
	PRINT_CTX_MEMBER(el1_common, tpidr_el1);
	PRINT_CTX_MEMBER(el1_common, tpidr_el0);
	PRINT_CTX_MEMBER(el1_common, tpidrro_el0);
	PRINT_CTX_MEMBER(el1_common, par_el1);
	PRINT_CTX_MEMBER(el1_common, far_el1);
	PRINT_CTX_MEMBER(el1_common, afsr0_el1);
	PRINT_CTX_MEMBER(el1_common, afsr1_el1);
	PRINT_CTX_MEMBER(el1_common, contextidr_el1);
	PRINT_CTX_MEMBER(el1_common, vbar_el1);
	PRINT_CTX_MEMBER(el1_common, mdccint_el1);
	PRINT_CTX_MEMBER(el1_common, mdscr_el1);
}

static void print_el1_ctx_aarch32_sysregs(const el1_ctx_regs_t *el1_ctx)
{
#if CTX_INCLUDE_AARCH32_REGS
	el1_aarch32_regs_t *el1_aarch32 = get_el1_aarch32_regs_ctx(el1_ctx);

	PRINT_CTX_MEMBER(el1_aarch32, spsr_abt);
	PRINT_CTX_MEMBER(el1_aarch32, spsr_und);
	PRINT_CTX_MEMBER(el1_aarch32, spsr_irq);
	PRINT_CTX_MEMBER(el1_aarch32, spsr_fiq);
	PRINT_CTX_MEMBER(el1_aarch32, dacr32_el2);
	PRINT_CTX_MEMBER(el1_aarch32, ifsr32_el2);
#endif
}

static void print_el1_ctx_timer_sysregs(const el1_ctx_regs_t *el1_ctx)
{
	el1_arch_timer_regs_t *el1_arch_timer = get_el1_arch_timer_regs_ctx(el1_ctx);

	PRINT_CTX_MEMBER(el1_arch_timer, cntp_ctl_el0);
	PRINT_CTX_MEMBER(el1_arch_timer, cntp_cval_el0);
	PRINT_CTX_MEMBER(el1_arch_timer, cntv_ctl_el0);
	PRINT_CTX_MEMBER(el1_arch_timer, cntv_cval_el0);
	PRINT_CTX_MEMBER(el1_arch_timer, cntkctl_el1);
}

static void print_el1_ctx_mte2_sysregs(const el1_ctx_regs_t *el1_ctx)
{
	el1_mte2_regs_t *el1_mte2 = get_el1_mte2_regs_ctx(el1_ctx);

	PRINT_CTX_MEMBER(el1_mte2, tfsre0_el1);
	PRINT_CTX_MEMBER(el1_mte2, tfsr_el1);
	PRINT_CTX_MEMBER(el1_mte2, rgsr_el1);
	PRINT_CTX_MEMBER(el1_mte2, gcr_el1);
}

static void print_el1_ctx_ras_sysregs(const el1_ctx_regs_t *el1_ctx)
{
	el1_ras_regs_t *el1_ras = get_el1_ras_regs_ctx(el1_ctx);

	PRINT_CTX_MEMBER(el1_ras, disr_el1);
}

static void print_el1_ctx_s1pie_sysregs(const el1_ctx_regs_t *el1_ctx)
{
	el1_s1pie_regs_t *el1_s1pie = get_el1_s1pie_regs_ctx(el1_ctx);

	PRINT_CTX_MEMBER(el1_s1pie, pire0_el1);
	PRINT_CTX_MEMBER(el1_s1pie, pir_el1);
}

static void print_el1_ctx_s1poe_sysregs(const el1_ctx_regs_t *el1_ctx)
{
	el1_s1poe_regs_t *el1_s1poe = get_el1_s1poe_regs_ctx(el1_ctx);

	PRINT_CTX_MEMBER(el1_s1poe, por_el1);
}

static void print_el1_ctx_s2poe_sysregs(const el1_ctx_regs_t *el1_ctx)
{
	el1_s2poe_regs_t *el1_s2poe = get_el1_s2poe_regs_ctx(el1_ctx);

	PRINT_CTX_MEMBER(el1_s2poe, s2por_el1);

}

static void print_el1_ctx_tcr2_sysregs(const el1_ctx_regs_t *el1_ctx)
{
	el1_tcr2_regs_t *el1_tcr2 = get_el1_tcr2_regs_ctx(el1_ctx);

	PRINT_CTX_MEMBER(el1_tcr2, tcr2_el1);

}

static void print_el1_ctx_trf_sysregs(const el1_ctx_regs_t *el1_ctx)
{
	el1_trf_regs_t *el1_trf = get_el1_trf_regs_ctx(el1_ctx);

	PRINT_CTX_MEMBER(el1_trf, trfcr_el1);
}

static void print_el1_ctx_csv2_2_sysregs(const el1_ctx_regs_t *el1_ctx)
{
	el1_csv2_2_regs_t *el1_csv2_2 = get_el1_csv2_2_regs_ctx(el1_ctx);

	PRINT_CTX_MEMBER(el1_csv2_2, scxtnum_el0);
	PRINT_CTX_MEMBER(el1_csv2_2, scxtnum_el1);
}

static void print_el1_ctx_gcs_sysregs(const el1_ctx_regs_t *el1_ctx)
{
	el1_gcs_regs_t *el1_gcs = get_el1_gcs_regs_ctx(el1_ctx);

	PRINT_CTX_MEMBER(el1_gcs, gcscr_el1);
	PRINT_CTX_MEMBER(el1_gcs, gcscre0_el1);
	PRINT_CTX_MEMBER(el1_gcs, gcspr_el1);
	PRINT_CTX_MEMBER(el1_gcs, gcspr_el0);
}

/**
 * Public Function: save_el1_sysregs_context.
 *
 * @brief: To read the EL1 registers and save it into the context structure.
 * @param: EL1 system register context struct(el1_ctx_regs_t)
 */
void save_el1_sysregs_context(const el1_ctx_regs_t *el1_ctx)
{
	save_el1_ctx_common_regs(el1_ctx);
	save_el1_ctx_aarch32_regs(el1_ctx);
	save_el1_ctx_timer_regs(el1_ctx);
	save_el1_ctx_mte2_regs(el1_ctx);
	save_el1_ctx_ras_regs(el1_ctx);
	save_el1_ctx_s1pie_regs(el1_ctx);
	save_el1_ctx_s1poe_regs(el1_ctx);
	save_el1_ctx_s2poe_regs(el1_ctx);
	save_el1_ctx_tcr2_regs(el1_ctx);
	save_el1_ctx_trf_regs(el1_ctx);
	save_el1_ctx_csv2_2_regs(el1_ctx);
	save_el1_ctx_gcs_regs(el1_ctx);
}

/**
 * Public Function: modify_el1_context_sysregs.
 *
 * @brief: To modify the EL1 registers
 * @param: EL1 system register context struct(el1_ctx_regs_t)
 */
void modify_el1_context_sysregs(const el1_ctx_regs_t *el1_ctx, const bool modify_option)
{
	uint64_t mask;
	if (modify_option == NS_CORRUPT_EL1_REGS)
		mask = EL1_CTX_CORRUPT_MASK;
	else
		mask = EL1_CTX_RESTORE_MASK;

	write_el1_ctx_common_regs(el1_ctx, mask);
	write_el1_ctx_aarch32_regs(el1_ctx, mask);
	write_el1_ctx_timer_regs(el1_ctx, mask);
	write_el1_ctx_mte2_regs(el1_ctx, mask);
	write_el1_ctx_ras_regs(el1_ctx, mask);
	write_el1_ctx_s1pie_regs(el1_ctx, mask);
	write_el1_ctx_s1poe_regs(el1_ctx, mask);
	write_el1_ctx_s2poe_regs(el1_ctx, mask);
	write_el1_ctx_tcr2_regs(el1_ctx, mask);
	write_el1_ctx_trf_regs(el1_ctx, mask);
	write_el1_ctx_csv2_2_regs(el1_ctx, mask);
	write_el1_ctx_gcs_regs(el1_ctx, mask);
}

/**
 * Public Function: print_el1_sysregs_context
 *
 * @brief: To print all the members of the EL1 context structure.
 * @param: EL1 system register context struct pointer (el1_ctx_regs_t *)
 */
void print_el1_sysregs_context(const el1_ctx_regs_t *el1_ctx)
{
	PRINT_CTX_MEM_SEPARATOR();
	printf("| EL1 Context Registers |        Value       |\n");
	PRINT_CTX_MEM_SEPARATOR();
	print_el1_ctx_common_sysregs(el1_ctx);
	print_el1_ctx_aarch32_sysregs(el1_ctx);
	print_el1_ctx_timer_sysregs(el1_ctx);
	print_el1_ctx_mte2_sysregs(el1_ctx);
	print_el1_ctx_ras_sysregs(el1_ctx);
	print_el1_ctx_s1pie_sysregs(el1_ctx);
	print_el1_ctx_s1poe_sysregs(el1_ctx);
	print_el1_ctx_s2poe_sysregs(el1_ctx);
	print_el1_ctx_tcr2_sysregs(el1_ctx);
	print_el1_ctx_trf_sysregs(el1_ctx);
	print_el1_ctx_csv2_2_sysregs(el1_ctx);
	print_el1_ctx_gcs_sysregs(el1_ctx);
	printf("\n");
}

/**
 * Public Function: compare the EL1 context structures.
 *
 * @brief: To compare two explicit EL1 context structure values and return
 *          the status as (TRUE) if equal or (FALSE) if not-equal.
 * @param: EL1 system register context struct pointers
 *         ( el1_ctx_regs_t *, el1_ctx_regs_t *)
 */
bool compare_el1_contexts(const el1_ctx_regs_t *el1_ctx1, const el1_ctx_regs_t *el1_ctx2)
{
	if (!memcmp(el1_ctx1, el1_ctx2, sizeof(el1_ctx_regs_t)))
		return true;
	else
		return false;
}
