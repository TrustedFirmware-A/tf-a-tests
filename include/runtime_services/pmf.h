/*
 * Copyright (c) 2018-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PMF_H__
#define __PMF_H__

/*
 * Constants used for/by PMF services.
 */
#define PMF_ARM_TIF_IMPL_ID	0x41
#define PMF_TID_SHIFT		0
#define PMF_TID_MASK		(0xFF << PMF_TID_SHIFT)
#define PMF_SVC_ID_SHIFT	10
#define PMF_SVC_ID_MASK		(0x3F << PMF_SVC_ID_SHIFT)
#define PMF_IMPL_ID_SHIFT	24
#define PMF_IMPL_ID_MASK	(0xFFU << PMF_IMPL_ID_SHIFT)

/*
 * Flags passed to PMF_GET_TIMESTAMP_XXX and PMF_CAPTURE_TIMESTAMP
 */
#define PMF_CACHE_MAINT		(1 << 0)
#define PMF_NO_CACHE_MAINT	0

#define PMF_SMC_VERSION		U(0x00000001)
/*
 * Defines for PMF SMC function ids.
 */
#ifndef __aarch64__
#define PMF_SMC_GET_TIMESTAMP	0x87000020
#define PMF_SMC_GET_VERSION	0x87000021
#else
#define PMF_SMC_GET_TIMESTAMP	0xC7000020
#define PMF_SMC_GET_VERSION	0xC7000021
#endif

/* Following are the supported PMF service IDs */
#define PMF_PSCI_STAT_SVC_ID	0
#define PMF_RT_INSTR_SVC_ID	1

#endif /* __PMF_H__ */
