/*
 * Copyright (c) 2022-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef REALM_DEF_H
#define REALM_DEF_H

#include <xlat_tables_defs.h>

/* 1 MB for Realm payload as a default value */
#define REALM_MAX_LOAD_IMG_SIZE		U(0x100000)
#define REALM_STACK_SIZE		0x1000U
#define DATA_PATTERN_1			0x12345678U
#define DATA_PATTERN_2			0x11223344U
#define REALM_SUCCESS			0U
#define REALM_ERROR			1U
#define MAX_REC_COUNT			17U
#define MAX_REALM_COUNT			U(2)
#define MAX_AUX_PLANE_COUNT		U(3)
#define MAX_PLANE_COUNT			MAX_AUX_PLANE_COUNT + U(1)
#define PRIMARY_RTT_INDEX		0U
#define PRIMARY_PLANE_ID		0U

/* Only support 4KB at the moment */

#if (PAGE_SIZE == PAGE_SIZE_4KB)
#define PAGE_ALIGNMENT			PAGE_SIZE_4KB
#define TCR_TG0				TCR_TG0_4K
#define GRANULE_SHIFT			U(12)
#else
#error "Undefined value for PAGE_SIZE"
#endif

/*
 * TTE_STRIDE: The number of bits resolved in a single level of translation
 * walk (except for the starting level which may resolve more or fewer bits)
 */
#define MAX_IPA_BITS			U(48)
#define TTE_STRIDE			(GRANULE_SHIFT - 3U)

/*
 * TTE_STRIDE_LM1: The number of bits resolved al level -1 when FEAT_LPA2
 * is enabled. This value is equal to
 * MAX_IPA_BITS_LPA2 - ((4 * S2TTE_STRIDE) + GRANULE_SHIFT)
 * as Level -1 only has 4 bits for the index (bits [51:48])
 */
#define MAX_IPA_BITS_LPA2		U(52)
#define TTE_STRIDE_LM1			U(4)

#define TT_PAGE_LEVEL			U(3)
#define tte_map_size(level)						\
	(1ULL << (unsigned int)(((TT_PAGE_LEVEL - (level)) *		\
				 (int)TTE_STRIDE) + (int)GRANULE_SHIFT))

/*
 * 'MPIDR_EL1_AFF<n>_VAL_SHIFT' constants specify the left shift
 * for affinity <n> value that gives the field's actual value.
 */

/* Aff0[3:0] - Affinity level 0
 * For compatibility with GICv3 only Aff0[3:0] field is used,
 * and Aff0[7:4] of MPIDR_EL1 value is RES0 to match RmiRecMpidr.
 */
#define MPIDR_EL1_AFF0_VAL_SHIFT	0

/* Aff1[15:8] - Affinity level 1 */
#define MPIDR_EL1_AFF1_VAL_SHIFT	4

/* Aff2[23:16] - Affinity level 2 */
#define MPIDR_EL1_AFF2_VAL_SHIFT	12

/* Aff3[39:32] - Affinity level 3 */
#define MPIDR_EL1_AFF3_VAL_SHIFT	20

/*
 * Extract the value of MPIDR_EL1.Aff<n> register field
 * and shift it left so it can be evaluated directly.
 */
#define MPIDR_EL1_AFF(mpidr, n)	\
	(MPIDR_AFF_ID(mpidr, n) << MPIDR_EL1_AFF##n##_VAL_SHIFT)

/*
 * Convert MPIDR_EL1 register type value
 * Aff3[39:32].Aff2[23:16].Aff1[15:8].RES0[7:4].Aff0[3:0]
 * to REC linear index
 * Aff3[27:20].Aff2[19:12].Aff1[11:4].Aff0[3:0].
 */
#define REC_IDX(mpidr)				\
		(MPIDR_EL1_AFF(mpidr, 0) |	\
		 MPIDR_EL1_AFF(mpidr, 1) |	\
		 MPIDR_EL1_AFF(mpidr, 2) |	\
		 MPIDR_EL1_AFF(mpidr, 3))

/*
 * RmiRecMpidr type definitions.
 *
 * Aff0[3:0] - Affinity level 0
 * For compatibility with GICv3 only Aff0[3:0] field is used,
 * and Aff0[7:4] of a REC MPIDR value is RES0.
 */
#define RMI_MPIDR_AFF0_SHIFT		0
#define RMI_MPIDR_AFF0_WIDTH		U(4)

/* Aff1[15:8] - Affinity level 1 */
#define RMI_MPIDR_AFF1_SHIFT		8
#define RMI_MPIDR_AFF1_WIDTH		U(8)

/* Aff2[23:16] - Affinity level 2 */
#define RMI_MPIDR_AFF2_SHIFT		16
#define RMI_MPIDR_AFF2_WIDTH		U(8)

/* Aff3[31:24] - Affinity level 3 */
#define RMI_MPIDR_AFF3_SHIFT		24
#define RMI_MPIDR_AFF3_WIDTH		U(8)

/*
 * Convert REC linear index
 * Aff3[27:20].Aff2[19:12].Aff1[11:4].Aff0[3:0]
 * to RmiRecMpidr type value
 * Aff3[31:24].Aff2[23:16].Aff1[15:8].RES0[7:4].Aff0[3:0].
 */
#define RMI_REC_MPIDR(idx)				\
		(((idx) & MASK(RMI_MPIDR_AFF0)) |	\
		(((idx) & ~MASK(RMI_MPIDR_AFF0)) << 	\
		(RMI_MPIDR_AFF1_SHIFT - RMI_MPIDR_AFF0_WIDTH)));

#define REALM_TOKEN_BUF_SIZE		GRANULE_SIZE

#define DEFAULT_MECID			(unsigned short)0

#endif /* REALM_DEF_H */
