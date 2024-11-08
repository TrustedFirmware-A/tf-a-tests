/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CONTEXT_EL1_H
#define CONTEXT_EL1_H

#include <stdint.h>
#include <stdio.h>
#include <tftf_lib.h>

#define NS_CORRUPT_EL1_REGS		1
#define NS_RESTORE_EL1_REGS		0
/**
 * Structure template to define individual EL1 register.
 */
typedef struct el1_reg {
	char *reg_name;
	uint64_t reg_value;
} el1_reg_t;

/*******************************************************************************
 * EL1 Registers:
 * AArch64 EL1 system registers which are intended to be saved and restored as
 * part of context management library during world switch.
 ******************************************************************************/

typedef struct el1_common_regs {
	el1_reg_t spsr_el1;
	el1_reg_t elr_el1;
	el1_reg_t sctlr_el1;
	el1_reg_t tcr_el1;
	el1_reg_t cpacr_el1;
	el1_reg_t csselr_el1;
	el1_reg_t sp_el1;
	el1_reg_t esr_el1;
	el1_reg_t ttbr0_el1;
	el1_reg_t ttbr1_el1;
	el1_reg_t mair_el1;
	el1_reg_t amair_el1;
	el1_reg_t actlr_el1;
	el1_reg_t tpidr_el1;
	el1_reg_t tpidr_el0;
	el1_reg_t tpidrro_el0;
	el1_reg_t par_el1;
	el1_reg_t far_el1;
	el1_reg_t afsr0_el1;
	el1_reg_t afsr1_el1;
	el1_reg_t contextidr_el1;
	el1_reg_t vbar_el1;
	el1_reg_t mdccint_el1;
	el1_reg_t mdscr_el1;
} el1_common_regs_t;

typedef struct el1_aarch32_regs {
	el1_reg_t spsr_abt;
	el1_reg_t spsr_und;
	el1_reg_t spsr_irq;
	el1_reg_t spsr_fiq;
	el1_reg_t dacr32_el2;
	el1_reg_t ifsr32_el2;
} el1_aarch32_regs_t;

typedef struct el1_arch_timer_regs {
	el1_reg_t cntp_ctl_el0;
	el1_reg_t cntp_cval_el0;
	el1_reg_t cntv_ctl_el0;
	el1_reg_t cntv_cval_el0;
	el1_reg_t cntkctl_el1;
} el1_arch_timer_regs_t;

typedef struct el1_mte2_regs {
	el1_reg_t tfsre0_el1;
	el1_reg_t tfsr_el1;
	el1_reg_t rgsr_el1;
	el1_reg_t gcr_el1;
} el1_mte2_regs_t;

typedef struct el1_ras_regs {
	el1_reg_t disr_el1;
} el1_ras_regs_t;

typedef struct el1_s1pie_regs {
	el1_reg_t pire0_el1;
	el1_reg_t pir_el1;
} el1_s1pie_regs_t;

typedef struct el1_s1poe_regs {
	el1_reg_t por_el1;
} el1_s1poe_regs_t;

typedef struct el1_s2poe_regs {
	el1_reg_t s2por_el1;
} el1_s2poe_regs_t;

typedef struct el1_tcr2_regs {
	el1_reg_t tcr2_el1;
} el1_tcr2_regs_t;

typedef struct el1_trf_regs {
	el1_reg_t trfcr_el1;
} el1_trf_regs_t;

typedef struct el1_csv2_2_regs {
	el1_reg_t scxtnum_el0;
	el1_reg_t scxtnum_el1;
} el1_csv2_2_regs_t;

typedef struct el1_gcs_regs {
	el1_reg_t gcscr_el1;
	el1_reg_t gcscre0_el1;
	el1_reg_t gcspr_el1;
	el1_reg_t gcspr_el0;
} el1_gcs_regs_t;
typedef struct el1_ctx_regs {
	el1_common_regs_t common;
	el1_aarch32_regs_t el1_aarch32;
	el1_arch_timer_regs_t arch_timer;
	el1_mte2_regs_t mte2;
	el1_ras_regs_t ras;
	el1_s1pie_regs_t s1pie;
	el1_s1poe_regs_t s1poe;
	el1_s2poe_regs_t s2poe;
	el1_tcr2_regs_t tcr2;
	el1_trf_regs_t trf;
	el1_csv2_2_regs_t csv2_2;
	el1_gcs_regs_t gcs;
} el1_ctx_regs_t;

/*
 * Helper macros to access and print members of the el1_ctx_regs_t structure.
 */

#define PRINT_CTX_MEM_SEPARATOR()						\
		printf("+-----------------------+--------------------+\n");	\

#define PRINT_CTX_MEMBER(feat, reg)					\
	{								\
		printf("|    %-15s    | 0x%016llx |\n", 		\
			(feat->reg).reg_name, (feat->reg).reg_value);	\
		PRINT_CTX_MEM_SEPARATOR();				\
	}

#define write_el1_ctx_reg(feat, reg, name, val)	\
{						\
	(feat->reg).reg_name = name;		\
	(feat->reg).reg_value = (uint64_t)val;	\
}

/* Macros to access members of the 'el1_ctx_regs_t' structure */
#define get_el1_common_regs_ctx(h)	(&((el1_ctx_regs_t *) h)->common)
#define get_el1_aarch32_regs_ctx(h)	(&((el1_ctx_regs_t *) h)->el1_aarch32)
#define get_el1_arch_timer_regs_ctx(h)	(&((el1_ctx_regs_t *) h)->arch_timer)
#define get_el1_mte2_regs_ctx(h)	(&((el1_ctx_regs_t *) h)->mte2)
#define get_el1_ras_regs_ctx(h)		(&((el1_ctx_regs_t *) h)->ras)
#define get_el1_s1pie_regs_ctx(h)	(&((el1_ctx_regs_t *) h)->s1pie)
#define get_el1_s1poe_regs_ctx(h)	(&((el1_ctx_regs_t *) h)->s1poe)
#define get_el1_s2poe_regs_ctx(h)	(&((el1_ctx_regs_t *) h)->s2poe)
#define get_el1_tcr2_regs_ctx(h)	(&((el1_ctx_regs_t *) h)->tcr2)
#define get_el1_trf_regs_ctx(h)		(&((el1_ctx_regs_t *) h)->trf)
#define get_el1_csv2_2_regs_ctx(h)	(&((el1_ctx_regs_t *) h)->csv2_2)
#define get_el1_gcs_regs_ctx(h)		(&((el1_ctx_regs_t *) h)->gcs)

/**
 * --------------------------------------
 * EL1 context accessor public functions.
 * --------------------------------------
 */
void print_el1_sysregs_context(const el1_ctx_regs_t *el1_ctx);
void save_el1_sysregs_context(const el1_ctx_regs_t *el1_ctx);
void modify_el1_context_sysregs(const el1_ctx_regs_t *el1_ctx, const bool modify_option);
bool compare_el1_contexts(const el1_ctx_regs_t *el1_ctx1, const el1_ctx_regs_t *el1_ctx2);

#endif /* CONTEXT_EL1_H */
