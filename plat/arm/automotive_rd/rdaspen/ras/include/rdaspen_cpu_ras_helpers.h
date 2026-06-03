/*
 * Copyright (c) 2025-2026, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RAS_HELPERS_H
#define RAS_HELPERS_H

#include <lib/aarch64/arch_helpers.h>
#include <stdint.h>

/*
 * Enable the pseudo-fault countdown so the programmed injection fires on the
 * next matching access.
 */
static inline void enable_cpu_pfg_cdn_register(void)
{
	write_cpu_erxpfgcdn_el1(ERXPFGCDN_CDN);
}

static inline void write_cpu_pfg_ctrl_register_uc(void)
{
	uint64_t v = read_cpu_erxpfgctl_el1();

	v |= ERXPFGCTL_CDNEN_BIT | ERXPFGCTL_UC_BIT;
	write_cpu_erxpfgctl_el1(v);
}

static inline void write_cpu_pfg_ctrl_register_de(void)
{
	uint64_t v = read_cpu_erxpfgctl_el1();

	v |= ERXPFGCTL_CDNEN_BIT | ERXPFGCTL_DE_BIT;
	write_cpu_erxpfgctl_el1(v);
}

static inline void write_cpu_pfg_ctrl_register_ce(void)
{
	uint64_t v = read_cpu_erxpfgctl_el1();

	v |= ERXPFGCTL_CDNEN_BIT | ERXPFGCTL_R_BIT | ERXPFGCTL_CE_BIT;
	write_cpu_erxpfgctl_el1(v);
}

#endif /* RAS_HELPERS_H */
