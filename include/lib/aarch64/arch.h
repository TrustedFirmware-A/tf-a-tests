/*
 * Copyright (c) 2013-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ARCH_H
#define ARCH_H

#include <utils_def.h>

/*******************************************************************************
 * MIDR bit definitions
 ******************************************************************************/
#define MIDR_IMPL_MASK		U(0xff)
#define MIDR_IMPL_SHIFT		U(0x18)
#define MIDR_VAR_SHIFT		U(20)
#define MIDR_VAR_BITS		U(4)
#define MIDR_VAR_MASK		U(0xf0)
#define MIDR_REV_SHIFT		U(0)
#define MIDR_REV_BITS		U(4)
#define MIDR_REV_MASK		U(0xf)
#define MIDR_PN_MASK		U(0xfff)
#define MIDR_PN_SHIFT		U(0x4)

/******************************************************************************
 * MIDR macros
 *****************************************************************************/
/* Extract the partnumber */
#define EXTRACT_PARTNUM(x)     ((x >> MIDR_PN_SHIFT) & MIDR_PN_MASK)
/* Extract revision and variant info */

#define EXTRACT_REV_VAR(x)	(x & MIDR_REV_MASK) | ((x >> (MIDR_VAR_SHIFT - MIDR_REV_BITS)) \
				& MIDR_VAR_MASK)

/*******************************************************************************
 * MPIDR macros
 ******************************************************************************/
#define MPIDR_MT_MASK		(ULL(1) << 24)
#define MPIDR_CPU_MASK		MPIDR_AFFLVL_MASK
#define MPIDR_CLUSTER_MASK	(MPIDR_AFFLVL_MASK << MPIDR_AFFINITY_BITS)
#define MPIDR_AFFINITY_BITS	U(8)
#define MPIDR_AFFLVL_MASK	ULL(0xff)
#define MPIDR_AFF0_SHIFT	U(0)
#define MPIDR_AFF1_SHIFT	U(8)
#define MPIDR_AFF2_SHIFT	U(16)
#define MPIDR_AFF3_SHIFT	U(32)
#define MPIDR_AFF_SHIFT(_n)	MPIDR_AFF##_n##_SHIFT
#define MPIDR_AFFINITY_MASK	ULL(0xff00ffffff)
#define MPIDR_AFFLVL_SHIFT	U(3)
#define MPIDR_AFFLVL0		ULL(0x0)
#define MPIDR_AFFLVL1		ULL(0x1)
#define MPIDR_AFFLVL2		ULL(0x2)
#define MPIDR_AFFLVL3		ULL(0x3)
#define MPIDR_AFFLVL(_n)	MPIDR_AFFLVL##_n
#define MPIDR_AFFLVL0_VAL(mpidr) \
		(((mpidr) >> MPIDR_AFF0_SHIFT) & MPIDR_AFFLVL_MASK)
#define MPIDR_AFFLVL1_VAL(mpidr) \
		(((mpidr) >> MPIDR_AFF1_SHIFT) & MPIDR_AFFLVL_MASK)
#define MPIDR_AFFLVL2_VAL(mpidr) \
		(((mpidr) >> MPIDR_AFF2_SHIFT) & MPIDR_AFFLVL_MASK)
#define MPIDR_AFFLVL3_VAL(mpidr) \
		(((mpidr) >> MPIDR_AFF3_SHIFT) & MPIDR_AFFLVL_MASK)
/*
 * The MPIDR_MAX_AFFLVL count starts from 0. Take care to
 * add one while using this macro to define array sizes.
 * TODO: Support only the first 3 affinity levels for now.
 */
#define MPIDR_MAX_AFFLVL	U(2)

#define MPID_MASK		(MPIDR_MT_MASK				 | \
				 (MPIDR_AFFLVL_MASK << MPIDR_AFF3_SHIFT) | \
				 (MPIDR_AFFLVL_MASK << MPIDR_AFF2_SHIFT) | \
				 (MPIDR_AFFLVL_MASK << MPIDR_AFF1_SHIFT) | \
				 (MPIDR_AFFLVL_MASK << MPIDR_AFF0_SHIFT))

#define MPIDR_AFF_ID(mpid, n)					\
	(((mpid) >> MPIDR_AFF_SHIFT(n)) & MPIDR_AFFLVL_MASK)

/*
 * An invalid MPID. This value can be used by functions that return an MPID to
 * indicate an error.
 */
#define INVALID_MPID		U(0xFFFFFFFF)

/*******************************************************************************
 * Definitions for CPU system register interface to GICv3
 ******************************************************************************/
#define ICC_IGRPEN1_EL1		S3_0_C12_C12_7
#define ICC_SGI1R		S3_0_C12_C11_5
#define ICC_SRE_EL1		S3_0_C12_C12_5
#define ICC_SRE_EL2		S3_4_C12_C9_5
#define ICC_SRE_EL3		S3_6_C12_C12_5
#define ICC_CTLR_EL1		S3_0_C12_C12_4
#define ICC_CTLR_EL3		S3_6_C12_C12_4
#define ICC_PMR_EL1		S3_0_C4_C6_0
#define ICC_RPR_EL1		S3_0_C12_C11_3
#define ICC_IGRPEN1_EL3		S3_6_C12_C12_7
#define ICC_IGRPEN0_EL1		S3_0_C12_C12_6
#define ICC_HPPIR0_EL1		S3_0_C12_C8_2
#define ICC_HPPIR1_EL1		S3_0_C12_C12_2
#define ICC_IAR0_EL1		S3_0_C12_C8_0
#define ICC_IAR1_EL1		S3_0_C12_C12_0
#define ICC_EOIR0_EL1		S3_0_C12_C8_1
#define ICC_EOIR1_EL1		S3_0_C12_C12_1
#define ICC_SGI0R_EL1		S3_0_C12_C11_7
#define ICV_CTRL_EL1		S3_0_C12_C12_4
#define ICV_IAR1_EL1		S3_0_C12_C12_0
#define ICV_IGRPEN1_EL1		S3_0_C12_C12_7
#define ICV_EOIR1_EL1		S3_0_C12_C12_1
#define ICV_PMR_EL1		S3_0_C4_C6_0

/*******************************************************************************
 * Definitions for EL2 system registers.
 ******************************************************************************/
#define CNTPOFF_EL2		S3_4_C14_C0_6
#define CONTEXTIDR_EL2		S3_4_C13_C0_1
#define DBGVCR32_EL2		S2_4_C0_C7_0
#define HACR_EL2		S3_4_C1_C1_7
#define HAFGRTR_EL2		S3_4_C3_C1_6
#define HDFGRTR_EL2		S3_4_C3_C1_4
#define HDFGRTR2_EL2		S3_4_C3_C1_0
#define HDFGWTR_EL2		S3_4_C3_C1_5
#define HDFGWTR2_EL2		S3_4_C3_C1_1
#define HFGITR_EL2		S3_4_C1_C1_6
#define HFGITR2_EL2		S3_4_C3_C1_7
#define HFGRTR_EL2		S3_4_C1_C1_4
#define HFGRTR2_EL2		S3_4_C3_C1_2
#define HFGWTR_EL2		S3_4_C1_C1_5
#define HFGWTR2_EL2		S3_4_C3_C1_3
#define HPFAR_EL2		S3_4_C6_C0_4
#define ICH_HCR_EL2		S3_4_C12_C11_0
#define ICH_VMCR_EL2		S3_4_C12_C11_7
#define PMSCR_EL2		S3_4_C9_C9_0
#define TFSR_EL2		S3_4_C5_C6_0
#define TPIDR_EL2		S3_4_C13_C0_2
#define TTBR1_EL2		S3_4_C2_C0_1
#define VDISR_EL2		S3_4_C12_C1_1
#define VNCR_EL2		S3_4_C2_C2_0
#define VSESR_EL2		S3_4_C5_C2_3
#define VTCR_EL2		S3_4_C2_C1_2

/*******************************************************************************
 * Generic timer memory mapped registers & offsets
 ******************************************************************************/
#define CNTCR_OFF			U(0x000)
#define CNTFID_OFF			U(0x020)

#define CNTCR_EN			(U(1) << 0)
#define CNTCR_HDBG			(U(1) << 1)
#define CNTCR_FCREQ(x)			((x) << 8)

/*******************************************************************************
 * System register bit definitions
 ******************************************************************************/
/* CLIDR definitions */
#define LOUIS_SHIFT		U(21)
#define LOC_SHIFT		U(24)
#define CLIDR_FIELD_WIDTH	U(3)

/* CSSELR definitions */
#define LEVEL_SHIFT		U(1)

/* Data cache set/way op type defines */
#define DCISW			U(0x0)
#define DCCISW			U(0x1)
#define DCCSW			U(0x2)

/* ID_AA64PFR0_EL1 definitions */
#define ID_AA64PFR0_EL0_SHIFT			U(0)
#define ID_AA64PFR0_EL1_SHIFT			U(4)
#define ID_AA64PFR0_EL2_SHIFT			U(8)
#define ID_AA64PFR0_EL3_SHIFT			U(12)
#define ID_AA64PFR0_ELX_MASK			ULL(0xf)
#define ID_AA64PFR0_FP_SHIFT			U(16)
#define ID_AA64PFR0_FP_WIDTH			U(4)
#define ID_AA64PFR0_FP_MASK			U(0xf)
#define ID_AA64PFR0_ADVSIMD_SHIFT		U(20)
#define ID_AA64PFR0_ADVSIMD_WIDTH		U(4)
#define ID_AA64PFR0_ADVSIMD_MASK		U(0xf)
#define ID_AA64PFR0_GIC_SHIFT			U(24)
#define ID_AA64PFR0_GIC_WIDTH			U(4)
#define ID_AA64PFR0_GIC_MASK			ULL(0xf)
#define ID_AA64PFR0_GIC_NOT_SUPPORTED		ULL(0x0)
#define ID_AA64PFR0_GICV3_GICV4_SUPPORTED	ULL(0x1)
#define ID_AA64PFR0_GICV4_1_SUPPORTED		ULL(0x2)
#define ID_AA64PFR0_RAS_MASK			ULL(0xf)
#define ID_AA64PFR0_RAS_SHIFT			U(28)
#define ID_AA64PFR0_RAS_WIDTH			U(4)
#define ID_AA64PFR0_RAS_NOT_SUPPORTED		ULL(0x0)
#define ID_AA64PFR0_RAS_SUPPORTED		ULL(0x1)
#define ID_AA64PFR0_RASV1P1_SUPPORTED		ULL(0x2)
#define ID_AA64PFR0_SVE_SHIFT			U(32)
#define ID_AA64PFR0_SVE_WIDTH			U(4)
#define ID_AA64PFR0_SVE_MASK			ULL(0xf)
#define ID_AA64PFR0_SVE_LENGTH			U(4)
#define ID_AA64PFR0_MPAM_SHIFT			U(40)
#define ID_AA64PFR0_MPAM_MASK			ULL(0xf)
#define ID_AA64PFR0_AMU_SHIFT			U(44)
#define ID_AA64PFR0_AMU_LENGTH			U(4)
#define ID_AA64PFR0_AMU_MASK			ULL(0xf)
#define ID_AA64PFR0_AMU_NOT_SUPPORTED		U(0x0)
#define ID_AA64PFR0_AMU_V1			U(0x1)
#define ID_AA64PFR0_AMU_V1P1			U(0x2)
#define ID_AA64PFR0_DIT_SHIFT			U(48)
#define ID_AA64PFR0_DIT_MASK			ULL(0xf)
#define ID_AA64PFR0_DIT_LENGTH			U(4)
#define ID_AA64PFR0_DIT_SUPPORTED		U(1)
#define ID_AA64PFR0_FEAT_RME_SHIFT		U(52)
#define ID_AA64PFR0_FEAT_RME_MASK		ULL(0xf)
#define ID_AA64PFR0_FEAT_RME_LENGTH		U(4)
#define ID_AA64PFR0_FEAT_RME_NOT_SUPPORTED	U(0)
#define ID_AA64PFR0_FEAT_RME_V1			U(1)
#define ID_AA64PFR0_CSV2_SHIFT			U(56)
#define ID_AA64PFR0_CSV2_MASK			ULL(0xf)
#define ID_AA64PFR0_CSV2_WIDTH			U(4)
#define ID_AA64PFR0_CSV2_NOT_SUPPORTED		ULL(0x0)
#define ID_AA64PFR0_CSV2_SUPPORTED		ULL(0x1)
#define ID_AA64PFR0_CSV2_2_SUPPORTED		ULL(0x2)

/* ID_AA64DFR0_EL1.DoubleLock definitions */
#define ID_AA64DFR0_DOUBLELOCK_SHIFT		U(36)
#define ID_AA64DFR0_DOUBLELOCK_MASK		ULL(0xf)
#define ID_AA64DFR0_DOUBLELOCK_WIDTH		U(4)
#define DOUBLELOCK_IMPLEMENTED			ULL(0)

/* ID_AA64DFR0_EL1.PMS definitions (for ARMv8.2+) */
#define ID_AA64DFR0_PMS_SHIFT		U(32)
#define ID_AA64DFR0_PMS_LENGTH		U(4)
#define ID_AA64DFR0_PMS_MASK		ULL(0xf)
#define ID_AA64DFR0_SPE_NOT_SUPPORTED	U(0)
#define ID_AA64DFR0_SPE			U(1)
#define ID_AA64DFR0_SPE_V1P1		U(2)
#define ID_AA64DFR0_SPE_V1P2		U(3)
#define ID_AA64DFR0_SPE_V1P3		U(4)
#define ID_AA64DFR0_SPE_V1P4		U(5)

/* ID_AA64DFR0_EL1.DEBUG definitions */
#define ID_AA64DFR0_DEBUG_SHIFT			U(0)
#define ID_AA64DFR0_DEBUG_LENGTH		U(4)
#define ID_AA64DFR0_DEBUG_MASK			ULL(0xf)
#define ID_AA64DFR0_DEBUG_BITS			(ID_AA64DFR0_DEBUG_MASK << \
						 ID_AA64DFR0_DEBUG_SHIFT)
#define ID_AA64DFR0_V8_DEBUG_ARCH_SUPPORTED	U(6)
#define ID_AA64DFR0_V8_DEBUG_ARCH_VHE_SUPPORTED	U(7)
#define ID_AA64DFR0_V8_2_DEBUG_ARCH_SUPPORTED	U(8)
#define ID_AA64DFR0_V8_4_DEBUG_ARCH_SUPPORTED	U(9)
#define ID_AA64DFR0_V8_9_DEBUG_ARCH_SUPPORTED   U(0xb)

/* ID_AA64DFR0_EL1.HPMN0 definitions */
#define ID_AA64DFR0_HPMN0_SHIFT			U(60)
#define ID_AA64DFR0_HPMN0_MASK			ULL(0xf)
#define ID_AA64DFR0_HPMN0_SUPPORTED		ULL(1)

/* ID_AA64DFR0_EL1.BRBE definitions */
#define ID_AA64DFR0_BRBE_SHIFT			U(52)
#define ID_AA64DFR0_BRBE_MASK			ULL(0xf)
#define ID_AA64DFR0_BRBE_SUPPORTED		ULL(1)

/* ID_AA64DFR0_EL1.TraceBuffer definitions */
#define ID_AA64DFR0_TRACEBUFFER_SHIFT		U(44)
#define ID_AA64DFR0_TRACEBUFFER_MASK		ULL(0xf)
#define ID_AA64DFR0_TRACEBUFFER_SUPPORTED	ULL(1)
#define ID_AA64DFR0_TRACEBUFFER_WIDTH		U(4)

/* ID_DFR0_EL1.Tracefilt definitions */
#define ID_AA64DFR0_TRACEFILT_SHIFT		U(40)
#define ID_AA64DFR0_TRACEFILT_MASK		U(0xf)
#define ID_AA64DFR0_TRACEFILT_WIDTH		U(4)
#define ID_AA64DFR0_TRACEFILT_SUPPORTED		U(1)

/* ID_AA64DFR0_EL1.PMUVer definitions */
#define ID_AA64DFR0_PMUVER_SHIFT		U(8)
#define ID_AA64DFR0_PMUVER_MASK			ULL(0xf)
#define ID_AA64DFR0_PMUVER_NOT_SUPPORTED	ULL(0)

/* ID_AA64DFR0_EL1.TraceVer definitions */
#define ID_AA64DFR0_TRACEVER_SHIFT		U(4)
#define ID_AA64DFR0_TRACEVER_MASK		ULL(0xf)
#define ID_AA64DFR0_TRACEVER_SUPPORTED		ULL(1)

#define EL_IMPL_NONE		ULL(0)
#define EL_IMPL_A64ONLY		ULL(1)
#define EL_IMPL_A64_A32		ULL(2)

/* ID_AA64ISAR0_EL1 definitions */
#define ID_AA64ISAR0_EL1			S3_0_C0_C6_0
#define ID_AA64ISAR0_TLB_MASK			ULL(0xf)
#define ID_AA64ISAR0_TLB_SHIFT			U(56)
#define ID_AA64ISAR0_TLB_WIDTH			U(4)
#define ID_AA64ISAR0_TLBIRANGE_SUPPORTED	ULL(0x2)
#define ID_AA64ISAR0_TLB_NOT_SUPPORTED		ULL(0)

/* ID_AA64ISAR1_EL1 definitions */
#define ID_AA64ISAR1_EL1			S3_0_C0_C6_1
#define ID_AA64ISAR1_GPI_SHIFT			U(28)
#define ID_AA64ISAR1_GPI_WIDTH			U(4)
#define ID_AA64ISAR1_GPI_MASK			ULL(0xf)
#define ID_AA64ISAR1_GPA_SHIFT			U(24)
#define ID_AA64ISAR1_GPA_WIDTH			U(4)
#define ID_AA64ISAR1_GPA_MASK			ULL(0xf)
#define ID_AA64ISAR1_API_SHIFT			U(8)
#define ID_AA64ISAR1_API_WIDTH			U(4)
#define ID_AA64ISAR1_API_MASK			ULL(0xf)
#define ID_AA64ISAR1_APA_SHIFT			U(4)
#define ID_AA64ISAR1_APA_WIDTH			U(4)
#define ID_AA64ISAR1_APA_MASK			ULL(0xf)
#define ID_AA64ISAR1_SPECRES_MASK		ULL(0xf)
#define ID_AA64ISAR1_SPECRES_SHIFT		U(40)
#define ID_AA64ISAR1_SPECRES_WIDTH		U(4)
#define ID_AA64ISAR1_SPECRES_NOT_SUPPORTED	ULL(0x0)
#define ID_AA64ISAR1_SPECRES_SUPPORTED		ULL(0x1)
#define ID_AA64ISAR1_DPB_MASK			ULL(0xf)
#define ID_AA64ISAR1_DPB_SHIFT			U(0)
#define ID_AA64ISAR1_DPB_WIDTH			U(4)
#define ID_AA64ISAR1_DPB_NOT_SUPPORTED		ULL(0x0)
#define ID_AA64ISAR1_DPB_SUPPORTED		ULL(0x1)
#define ID_AA64ISAR1_DPB2_SUPPORTED		ULL(0x2)
#define ID_AA64ISAR1_LS64_MASK			ULL(0xf)
#define ID_AA64ISAR1_LS64_SHIFT			U(60)
#define ID_AA64ISAR1_LS64_WIDTH			U(4)
#define ID_AA64ISAR1_LS64_NOT_SUPPORTED		ULL(0x0)
#define ID_AA64ISAR1_LS64_SUPPORTED		ULL(0x1)
#define ID_AA64ISAR1_LS64_V_SUPPORTED		ULL(0x2)
#define ID_AA64ISAR1_LS64_ACCDATA_SUPPORTED	ULL(0x3)

/* ID_AA64ISAR2_EL1 definitions */
#define ID_AA64ISAR2_EL1		S3_0_C0_C6_2
#define ID_AA64ISAR2_WFXT_MASK		ULL(0xf)
#define ID_AA64ISAR2_WFXT_SHIFT		U(0x0)
#define ID_AA64ISAR2_WFXT_SUPPORTED	ULL(0x2)
#define ID_AA64ISAR2_GPA3_SHIFT		U(8)
#define ID_AA64ISAR2_GPA3_MASK		ULL(0xf)
#define ID_AA64ISAR2_APA3_SHIFT		U(12)
#define ID_AA64ISAR2_APA3_MASK		ULL(0xf)

/* ID_AA64MMFR0_EL1 definitions */
#define ID_AA64MMFR0_EL1_PARANGE_SHIFT	U(0)
#define ID_AA64MMFR0_EL1_PARANGE_MASK	ULL(0xf)

#define PARANGE_0000	U(32)
#define PARANGE_0001	U(36)
#define PARANGE_0010	U(40)
#define PARANGE_0011	U(42)
#define PARANGE_0100	U(44)
#define PARANGE_0101	U(48)
#define PARANGE_0110	U(52)

#define ID_AA64MMFR0_EL1_ECV_SHIFT         U(60)
#define ID_AA64MMFR0_EL1_ECV_MASK          ULL(0xf)
#define ID_AA64MMFR0_EL1_ECV_NOT_SUPPORTED ULL(0x0)
#define ID_AA64MMFR0_EL1_ECV_SUPPORTED     ULL(0x1)
#define ID_AA64MMFR0_EL1_ECV_SELF_SYNCH    ULL(0x2)

#define ID_AA64MMFR0_EL1_FGT_SHIFT		U(56)
#define ID_AA64MMFR0_EL1_FGT_MASK		ULL(0xf)
#define ID_AA64MMFR0_EL1_FGT_NOT_SUPPORTED	ULL(0x0)
#define ID_AA64MMFR0_EL1_FGT_SUPPORTED		ULL(0x1)
#define ID_AA64MMFR0_EL1_FGT2_SUPPORTED		ULL(0x2)

#define ID_AA64MMFR0_EL1_TGRAN4_SHIFT		U(28)
#define ID_AA64MMFR0_EL1_TGRAN4_WIDTH		U(4)
#define ID_AA64MMFR0_EL1_TGRAN4_MASK		ULL(0xf)
#define ID_AA64MMFR0_EL1_TGRAN4_SUPPORTED	ULL(0x0)
#define ID_AA64MMFR0_EL1_TGRAN4_52B_SUPPORTED	ULL(0x1)
#define ID_AA64MMFR0_EL1_TGRAN4_NOT_SUPPORTED	ULL(0xf)

#define ID_AA64MMFR0_EL1_TGRAN4_2_SHIFT		U(40)
#define ID_AA64MMFR0_EL1_TGRAN4_2_WIDTH		U(4)
#define ID_AA64MMFR0_EL1_TGRAN4_2_MASK		ULL(0xf)
#define ID_AA64MMFR0_EL1_TGRAN4_2_AS_1		ULL(0x0)
#define ID_AA64MMFR0_EL1_TGRAN4_2_NOT_SUPPORTED	ULL(0x1)
#define ID_AA64MMFR0_EL1_TGRAN4_2_SUPPORTED	ULL(0x2)
#define ID_AA64MMFR0_EL1_TGRAN4_2_52B_SUPPORTED	ULL(0x3)

#define ID_AA64MMFR0_EL1_TGRAN64_SHIFT		U(24)
#define ID_AA64MMFR0_EL1_TGRAN64_WIDTH		U(4)
#define ID_AA64MMFR0_EL1_TGRAN64_MASK		ULL(0xf)
#define ID_AA64MMFR0_EL1_TGRAN64_SUPPORTED	ULL(0x0)
#define ID_AA64MMFR0_EL1_TGRAN64_NOT_SUPPORTED	ULL(0xf)

#define ID_AA64MMFR0_EL1_TGRAN64_2_SHIFT	U(36)
#define ID_AA64MMFR0_EL1_TGRAN64_2_WIDTH	U(4)
#define ID_AA64MMFR0_EL1_TGRAN64_2_MASK		ULL(0xf)
#define ID_AA64MMFR0_EL1_TGRAN64_2_AS_1		ULL(0x0)
#define ID_AA64MMFR0_EL1_TGRAN64_2_NOT_SUPPORTED ULL(0x1)
#define ID_AA64MMFR0_EL1_TGRAN64_2_SUPPORTED	ULL(0x2)

#define ID_AA64MMFR0_EL1_TGRAN16_SHIFT		U(20)
#define ID_AA64MMFR0_EL1_TGRAN16_WIDTH		U(4)
#define ID_AA64MMFR0_EL1_TGRAN16_MASK		ULL(0xf)
#define ID_AA64MMFR0_EL1_TGRAN16_SUPPORTED	ULL(0x1)
#define ID_AA64MMFR0_EL1_TGRAN16_NOT_SUPPORTED	ULL(0x0)
#define ID_AA64MMFR0_EL1_TGRAN16_52B_SUPPORTED	ULL(0x2)

#define ID_AA64MMFR0_EL1_TGRAN16_2_SHIFT	U(32)
#define ID_AA64MMFR0_EL1_TGRAN16_2_WIDTH	U(4)
#define ID_AA64MMFR0_EL1_TGRAN16_2_MASK		ULL(0xf)
#define ID_AA64MMFR0_EL1_TGRAN16_2_AS_1		ULL(0x0)
#define ID_AA64MMFR0_EL1_TGRAN16_2_NOT_SUPPORTED ULL(0x1)
#define ID_AA64MMFR0_EL1_TGRAN16_2_SUPPORTED	ULL(0x2)
#define ID_AA64MMFR0_EL1_TGRAN16_2_52B_SUPPORTED ULL(0x3)

/* ID_AA64MMFR1_EL1 definitions */
#define ID_AA64MMFR1_EL1_PAN_SHIFT		U(20)
#define ID_AA64MMFR1_EL1_PAN_MASK		ULL(0xf)
#define ID_AA64MMFR1_EL1_PAN_WIDTH		U(4)
#define ID_AA64MMFR1_EL1_PAN_SUPPORTED		ULL(0x1)
#define ID_AA64MMFR1_EL1_PAN2_SUPPORTED		ULL(0x2)
#define ID_AA64MMFR1_EL1_PAN3_SUPPORTED		ULL(0x3)
#define ID_AA64MMFR1_EL1_HCX_SHIFT		U(40)
#define ID_AA64MMFR1_EL1_HCX_MASK		ULL(0xf)
#define ID_AA64MMFR1_EL1_HCX_SUPPORTED		ULL(0x1)
#define ID_AA64MMFR1_EL1_HCX_NOT_SUPPORTED	ULL(0x0)
#define ID_AA64MMFR1_EL1_AFP_SHIFT		U(44)
#define ID_AA64MMFR1_EL1_AFP_MASK		ULL(0xf)
#define ID_AA64MMFR1_EL1_AFP_SUPPORTED		ULL(0x1)
#define ID_AA64MMFR1_EL1_LO_SHIFT		U(16)
#define ID_AA64MMFR1_EL1_LO_MASK		ULL(0xf)
#define ID_AA64MMFR1_EL1_LO_WIDTH		U(4)
#define ID_AA64MMFR1_EL1_LOR_NOT_SUPPORTED	ULL(0x0)
#define ID_AA64MMFR1_EL1_LOR_SUPPORTED		ULL(0x1)
#define ID_AA64MMFR1_EL1_VHE_SHIFT		ULL(8)
#define ID_AA64MMFR1_EL1_VHE_MASK		ULL(0xf)

/* ID_AA64MMFR2_EL1 definitions */
#define ID_AA64MMFR2_EL1		S3_0_C0_C7_2

#define ID_AA64MMFR2_EL1_ST_SHIFT	U(28)
#define ID_AA64MMFR2_EL1_ST_MASK	ULL(0xf)

#define ID_AA64MMFR2_EL1_CNP_SHIFT	U(0)
#define ID_AA64MMFR2_EL1_CNP_MASK	ULL(0xf)

#define ID_AA64MMFR2_EL1_NV_SHIFT	U(24)
#define ID_AA64MMFR2_EL1_NV_MASK	ULL(0xf)
#define NV2_IMPLEMENTED			ULL(0x2)

/* ID_AA64MMFR3_EL1 definitions */
#define ID_AA64MMFR3_EL1			S3_0_C0_C7_3

#define ID_AA64MMFR3_EL1_D128_SHIFT		U(32)
#define ID_AA64MMFR3_EL1_D128_MASK		ULL(0xf)
#define ID_AA64MMFR3_EL1_D128_WIDTH		U(4)
#define ID_AA64MMFR3_EL1_D128_SUPPORTED		ULL(0x1)

#define ID_AA64MMFR3_EL1_S2POE_SHIFT		U(20)
#define ID_AA64MMFR3_EL1_S2POE_MASK		ULL(0xf)
#define ID_AA64MMFR3_EL1_S2POE_WIDTH		U(4)
#define ID_AA64MMFR3_EL1_S2POE_SUPPORTED	ULL(0x1)

#define ID_AA64MMFR3_EL1_S1POE_SHIFT		U(16)
#define ID_AA64MMFR3_EL1_S1POE_MASK		ULL(0xf)
#define ID_AA64MMFR3_EL1_S1POE_WIDTH		U(4)
#define ID_AA64MMFR3_EL1_S1POE_SUPPORTED	ULL(0x1)

#define ID_AA64MMFR3_EL1_S2PIE_SHIFT		U(12)
#define ID_AA64MMFR3_EL1_S2PIE_MASK		ULL(0xf)
#define ID_AA64MMFR3_EL1_S2PIE_WIDTH		U(4)
#define ID_AA64MMFR3_EL1_S2PIE_SUPPORTED	ULL(0x1)

#define ID_AA64MMFR3_EL1_S1PIE_SHIFT		U(8)
#define ID_AA64MMFR3_EL1_S1PIE_MASK		ULL(0xf)
#define ID_AA64MMFR3_EL1_S1PIE_WIDTH		U(4)
#define ID_AA64MMFR3_EL1_S1PIE_SUPPORTED	ULL(0x1)

#define ID_AA64MMFR3_EL1_SCTLRX_SHIFT		U(4)
#define ID_AA64MMFR3_EL1_SCTLRX_MASK		ULL(0xf)
#define ID_AA64MMFR3_EL1_SCTLRX_WIDTH		ULL(0x4)
#define ID_AA64MMFR3_EL1_SCTLR2_SUPPORTED	ULL(0x1)

#define ID_AA64MMFR3_EL1_TCRX_SHIFT		U(0)
#define ID_AA64MMFR3_EL1_TCRX_MASK		ULL(0xf)
#define ID_AA64MMFR3_EL1_TCRX_WIDTH		U(4)
#define ID_AA64MMFR3_EL1_TCR2_SUPPORTED		ULL(0x1)

/* ID_AA64PFR1_EL1 definitions */
#define ID_AA64PFR1_EL1_DF2_SHIFT		U(56)
#define ID_AA64PFR1_EL1_DF2_WIDTH		U(4)
#define ID_AA64PFR1_EL1_DF2_MASK		(0xf << ID_AA64PFR1_EL1_DF2_SHIFT)

#define ID_AA64PFR1_EL1_THE_SHIFT		U(48)
#define ID_AA64PFR1_EL1_THE_MASK		ULL(0xf)
#define ID_AA64PFR1_EL1_THE_WIDTH		U(4)
#define ID_AA64PFR1_EL1_THE_SUPPORTED		ULL(1)

#define ID_AA64PFR1_EL1_GCS_SHIFT		U(44)
#define ID_AA64PFR1_EL1_GCS_MASK		ULL(0xf)
#define ID_AA64PFR1_EL1_GCS_WIDTH		U(4)
#define ID_AA64PFR1_EL1_GCS_SUPPORTED		ULL(1)

#define ID_AA64PFR1_CSV2_FRAC_SHIFT		U(32)
#define ID_AA64PFR1_CSV2_FRAC_MASK		ULL(0xf)
#define ID_AA64PFR1_CSV2_FRAC_WIDTH		U(4)
#define ID_AA64PFR1_CSV2_1P1_SUPPORTED		ULL(0x1)
#define ID_AA64PFR1_CSV2_1P2_SUPPORTED		ULL(0x2)

#define ID_AA64PFR1_EL1_RNDR_TRAP_SHIFT		U(28)
#define ID_AA64PFR1_EL1_RNDR_TRAP_MASK		ULL(0xf)
#define ID_AA64PFR1_EL1_RNG_TRAP_SUPPORTED	ULL(0x1)
#define ID_AA64PFR1_EL1_RNG_TRAP_NOT_SUPPORTED	ULL(0x0)

#define ID_AA64PFR1_EL1_SME_SHIFT		U(24)
#define ID_AA64PFR1_EL1_SME_MASK		ULL(0xf)
#define ID_AA64PFR1_EL1_SME_WIDTH		ULL(0x4)
#define ID_AA64PFR1_EL1_SME_NOT_SUPPORTED	ULL(0x0)
#define ID_AA64PFR1_EL1_SME_SUPPORTED		ULL(0x1)
#define ID_AA64PFR1_EL1_SME2_SUPPORTED		ULL(0x2)

#define ID_AA64PFR1_MPAM_FRAC_SHIFT		U(16)
#define ID_AA64PFR1_MPAM_FRAC_MASK		ULL(0xf)

#define ID_AA64PFR1_RAS_FRAC_MASK		ULL(0xf)
#define ID_AA64PFR1_RAS_FRAC_SHIFT		U(12)
#define ID_AA64PFR1_RAS_FRAC_MASK		ULL(0xf)
#define ID_AA64PFR1_RAS_FRAC_WIDTH		U(4)
#define ID_AA64PFR1_RASV1P1_SUPPORTED		ULL(0x1)

#define ID_AA64PFR1_EL1_MTE_SHIFT		U(8)
#define ID_AA64PFR1_EL1_MTE_MASK		ULL(0xf)
#define ID_AA64PFR1_EL1_MTE_WIDTH		U(4)
#define MTE_UNIMPLEMENTED			ULL(0)
#define MTE_IMPLEMENTED_EL0			ULL(1)	/* MTE is only implemented at EL0 */
#define MTE_IMPLEMENTED_ELX			ULL(2)	/* MTE is implemented at all ELs */

#define ID_AA64PFR1_EL1_SSBS_SHIFT		U(4)
#define ID_AA64PFR1_EL1_SSBS_MASK		ULL(0xf)
#define SSBS_UNAVAILABLE			ULL(0)	/* No architectural SSBS support */

#define ID_AA64PFR1_EL1_BT_SHIFT		U(0)
#define ID_AA64PFR1_EL1_BT_MASK			ULL(0xf)
#define BTI_IMPLEMENTED				ULL(1)	/* The BTI mechanism is implemented */

#define ID_AA64PFR1_DF2_SHIFT			U(56)
#define ID_AA64PFR1_DF2_WIDTH			ULL(0x4)

/* ID_AA64PFR2_EL1 definitions */
#define ID_AA64PFR2_EL1				S3_0_C0_C4_2
#define ID_AA64PFR2_EL1_FPMR_SHIFT		U(32)
#define ID_AA64PFR2_EL1_FPMR_MASK		ULL(0xf)
#define ID_AA64PFR2_EL1_FPMR_WIDTH		U(4)
#define ID_AA64PFR2_EL1_FPMR_SUPPORTED		ULL(0x1)

/* ID_PFR1_EL1 definitions */
#define ID_PFR1_VIRTEXT_SHIFT	U(12)
#define ID_PFR1_VIRTEXT_MASK	U(0xf)
#define GET_VIRT_EXT(id)	(((id) >> ID_PFR1_VIRTEXT_SHIFT) \
				 & ID_PFR1_VIRTEXT_MASK)

/* SCTLR definitions */
#define SCTLR_EL2_RES1	((U(1) << 29) | (U(1) << 28) | (U(1) << 23) | \
			 (U(1) << 22) | (U(1) << 18) | (U(1) << 16) | \
			 (U(1) << 11) | (U(1) << 5) | (U(1) << 4))

#define SCTLR_EL1_RES1	((U(1) << 29) | (U(1) << 28) | (U(1) << 23) | \
			 (U(1) << 22) | (U(1) << 20) | (U(1) << 11))
#define SCTLR_AARCH32_EL1_RES1 \
			((U(1) << 23) | (U(1) << 22) | (U(1) << 11) | \
			 (U(1) << 4) | (U(1) << 3))

#define SCTLR_EL3_RES1	((U(1) << 29) | (U(1) << 28) | (U(1) << 23) | \
			(U(1) << 22) | (U(1) << 18) | (U(1) << 16) | \
			(U(1) << 11) | (U(1) << 5) | (U(1) << 4))

#define SCTLR_M_BIT		(ULL(1) << 0)
#define SCTLR_A_BIT		(ULL(1) << 1)
#define SCTLR_C_BIT		(ULL(1) << 2)
#define SCTLR_SA_BIT		(ULL(1) << 3)
#define SCTLR_SA0_BIT		(ULL(1) << 4)
#define SCTLR_CP15BEN_BIT	(ULL(1) << 5)
#define SCTLR_ITD_BIT		(ULL(1) << 7)
#define SCTLR_SED_BIT		(ULL(1) << 8)
#define SCTLR_UMA_BIT		(ULL(1) << 9)
#define SCTLR_I_BIT		(ULL(1) << 12)
#define SCTLR_EnDB_BIT		(ULL(1) << 13)
#define SCTLR_DZE_BIT		(ULL(1) << 14)
#define SCTLR_UCT_BIT		(ULL(1) << 15)
#define SCTLR_NTWI_BIT		(ULL(1) << 16)
#define SCTLR_NTWE_BIT		(ULL(1) << 18)
#define SCTLR_WXN_BIT		(ULL(1) << 19)
#define SCTLR_UWXN_BIT		(ULL(1) << 20)
#define SCTLR_IESB_BIT		(ULL(1) << 21)
#define SCTLR_SPAN_BIT		(ULL(1) << 23)
#define SCTLR_E0E_BIT		(ULL(1) << 24)
#define SCTLR_EE_BIT		(ULL(1) << 25)
#define SCTLR_UCI_BIT		(ULL(1) << 26)
#define SCTLR_EnDA_BIT		(ULL(1) << 27)
#define SCTLR_EnIB_BIT		(ULL(1) << 30)
#define SCTLR_EnIA_BIT		(ULL(1) << 31)
#define SCTLR_DSSBS_BIT		(ULL(1) << 44)
#define SCTLR_RESET_VAL		SCTLR_EL3_RES1

/* SCTLR2 register definitions */
#define SCTLR2_EL2		S3_4_C1_C0_3
#define SCTLR2_EL1		S3_0_C1_C0_3

#define SCTLR2_NMEA_BIT		(UL(1) << 2)
#define SCTLR2_EnADERR_BIT	(UL(1) << 3)
#define SCTLR2_EnANERR_BIT	(UL(1) << 4)
#define SCTLR2_EASE_BIT		(UL(1) << 5)
#define SCTLR2_EnIDCP128_BIT	(UL(1) << 6)

/* CPACR_El1 definitions */
#define CPACR_EL1_FPEN(x)	((x) << 20)
#define CPACR_EL1_FP_TRAP_EL0	U(0x1)
#define CPACR_EL1_FP_TRAP_ALL	U(0x2)
#define CPACR_EL1_FP_TRAP_NONE	U(0x3)

#define CPACR_EL1_ZEN(x)	((x) << 16)
#define CPACR_EL1_ZEN_TRAP_EL0	U(0x1)
#define CPACR_EL1_ZEN_TRAP_ALL	U(0x2)
#define CPACR_EL1_ZEN_TRAP_NONE	U(0x3)

#define CPACR_EL1_SMEN(x)	((x) << 24)
#define CPACR_EL1_SMEN_TRAP_EL0	U(0x1)
#define CPACR_EL1_SMEN_TRAP_ALL	U(0x2)
#define CPACR_EL1_SMEN_TRAP_NONE U(0x3)

/* SCR definitions */
#define SCR_RES1_BITS		((U(1) << 4) | (U(1) << 5))
#define SCR_NSE_SHIFT		U(62)
#define SCR_FGTEN2_BIT		(UL(1) << 59)
#define SCR_NSE_BIT		(ULL(1) << SCR_NSE_SHIFT)
#define SCR_EnIDCP128_BIT	(UL(1) << 55)
#define SCR_PFAREn_BIT		(UL(1) << 53)
#define SCR_TWERR_BIT		(UL(1) << 52)
#define SCR_TMEA_BIT		(UL(1) << 51)
#define SCR_EnFPM_BIT		(UL(1) << 50)
#define SCR_MECEn_BIT		(UL(1) << 49)
#define SCR_GPF_BIT		(UL(1) << 48)
#define SCR_D128En_BIT		(UL(1) << 47)
#define SCR_AIEn_BIT		(UL(1) << 46)
#define SCR_TWEDEL_SHIFT	U(30)
#define SCR_TWEDEL_MASK		ULL(0xf)
#define SCR_PIEN_BIT		(UL(1) << 45)
#define SCR_SCTLR2En_BIT	(UL(1) << 44)
#define SCR_TCR2EN_BIT		(UL(1) << 43)
#define SCR_RCWMASKEn_BIT	(UL(1) << 42)
#define SCR_ENTP2_SHIFT		U(41)
#define SCR_TRNDR_BIT		(UL(1) << 40)
#define SCR_GCSEn_BIT		(UL(1) << 39)
#define SCR_HXEn_BIT		(UL(1) << 38)
#define SCR_ADEn_BIT		(UL(1) << 37)
#define SCR_EnAS0_BIT		(UL(1) << 36)
#define SCR_ENTP2_BIT		(UL(1) << SCR_ENTP2_SHIFT)
#define SCR_AMVOFFEN_SHIFT	U(35)
#define SCR_AMVOFFEN_BIT	(UL(1) << SCR_AMVOFFEN_SHIFT)
#define SCR_TME_BIT		(UL(1) << 34)
#define SCR_TWEDEn_BIT		(UL(1) << 29)
#define SCR_ECVEN_BIT		(UL(1) << 28)
#define SCR_FGTEN_BIT		(UL(1) << 27)
#define SCR_ATA_BIT		(UL(1) << 26)
#define SCR_EnSCXT_BIT		(UL(1) << 25)
#define SCR_FIEN_BIT		(UL(1) << 21)
#define SCR_NMEA_BIT		(UL(1) << 20)
#define SCR_EASE_BIT		(UL(1) << 19)
#define SCR_EEL2_BIT		(UL(1) << 18)
#define SCR_API_BIT		(UL(1) << 17)
#define SCR_APK_BIT		(UL(1) << 16)
#define SCR_TERR_BIT		(UL(1) << 15)
#define SCR_TLOR_BIT		(UL(1) << 14)
#define SCR_TWE_BIT		(UL(1) << 13)
#define SCR_TWI_BIT		(UL(1) << 12)
#define SCR_ST_BIT		(UL(1) << 11)
#define SCR_RW_BIT		(UL(1) << 10)
#define SCR_SIF_BIT		(UL(1) << 9)
#define SCR_HCE_BIT		(UL(1) << 8)
#define SCR_SMD_BIT		(UL(1) << 7)
#define SCR_EA_BIT		(UL(1) << 3)
#define SCR_FIQ_BIT		(UL(1) << 2)
#define SCR_IRQ_BIT		(UL(1) << 1)
#define SCR_NS_BIT		(UL(1) << 0)
#define SCR_RES1_BITS		((U(1) << 4) | (U(1) << 5))
#define SCR_VALID_BIT_MASK	U(0x2f8f)
#define SCR_RESET_VAL		SCR_RES1_BITS

/* MDCR_EL3 definitions */
#define MDCR_EnSTEPOP_BIT	(ULL(1) << 50)
#define MDCR_ETBAD(x)		((x) << 48)
#define MDCR_EnITE_BIT		(ULL(1) << 47)
#define MDCR_EPMSSAD(x)		(ULL(x) << 45)
#define MDCR_EnPMSS_BIT		(ULL(1) << 44)
#define MDCR_EBWE_BIT		(ULL(1) << 43)
#define MDCR_EnPMS3_BIT		(ULL(1) << 42)
#define MDCR_PMEE(x)		((x) << 40)
#define MDCR_EnTB2_BIT		(ULL(1) << 39)
#define MDCR_E3BREC_BIT		(ULL(1) << 38)
#define MDCR_E3BREW_BIT		(ULL(1) << 37)
#define MDCR_EnPMSN_BIT		(ULL(1) << 36)
#define MDCR_MPMX_BIT		(ULL(1) << 35)
#define MDCR_MCCD_BIT		(ULL(1) << 34)
#define MDCR_SBRBE_SHIFT	U(32)
#define MDCR_SBRBE_MASK		ULL(0x3)
#define MDCR_SBRBE(x)		(ULL(x) << MDCR_SBRBE_SHIFT)
#define MDCR_PMSSE(x)		((x) << 30)
#define MDCR_NSTBE_BIT		(ULL(1) << 26)
#define MDCR_NSTB(x)		((x) << 24)
#define MDCR_NSTB_EL1		ULL(0x3)
#define MDCR_NSTBE_BIT		(ULL(1) << 26)
#define MDCR_MTPME_BIT		(ULL(1) << 28)
#define MDCR_TDCC_BIT		(ULL(1) << 27)
#define MDCR_SCCD_BIT		(ULL(1) << 23)
#define MDCR_ETAD_BIT		(ULL(1) << 22)
#define MDCR_EPMAD_BIT		(ULL(1) << 21)
#define MDCR_EDAD_BIT		(ULL(1) << 20)
#define MDCR_TTRF_BIT		(ULL(1) << 19)
#define MDCR_STE_BIT		(ULL(1) << 18)
#define MDCR_SPME_BIT		(ULL(1) << 17)
#define MDCR_SDD_BIT		(ULL(1) << 16)
#define MDCR_SPD32(x)		((x) << 14)
#define MDCR_SPD32_LEGACY	ULL(0x0)
#define MDCR_SPD32_DISABLE	ULL(0x2)
#define MDCR_SPD32_ENABLE	ULL(0x3)
#define MDCR_NSPB(x)		((x) << 12)
#define MDCR_NSPB_EL1		ULL(0x3)
#define MDCR_NSPBE_BIT		(ULL(1) << 11)
#define MDCR_TDOSA_BIT		(ULL(1) << 10)
#define MDCR_TDA_BIT		(ULL(1) << 9)
#define MDCR_EnPM2_BIT		(ULL(1) << 7)
#define MDCR_TPM_BIT		(ULL(1) << 6)
#define MDCR_EDADE_BIT		(ULL(1) << 4)
#define MDCR_ETADE_BIT		(ULL(1) << 3)
#define MDCR_EPMADE_BIT		(ULL(1) << 2)
#define MDCR_RLTE_BIT		(ULL(1) << 0)
#define MDCR_EL3_RESET_VAL	ULL(0x0)

/* MDCR_EL2 definitions */
#define MDCR_EL2_TPMS		(U(1) << 14)
#define MDCR_EL2_E2PB(x)	((x) << 12)
#define MDCR_EL2_E2PB_EL1	U(0x3)
#define MDCR_EL2_TDRA_BIT	(U(1) << 11)
#define MDCR_EL2_TDOSA_BIT	(U(1) << 10)
#define MDCR_EL2_TDA_BIT	(U(1) << 9)
#define MDCR_EL2_TDE_BIT	(U(1) << 8)
#define MDCR_EL2_HPME_BIT	(U(1) << 7)
#define MDCR_EL2_TPM_BIT	(U(1) << 6)
#define MDCR_EL2_TPMCR_BIT	(U(1) << 5)
#define MDCR_EL2_HPMN_SHIFT	U(0)
#define MDCR_EL2_HPMN_MASK	ULL(0x1f)
#define MDCR_EL2_RESET_VAL	U(0x0)

/* HSTR_EL2 definitions */
#define HSTR_EL2_RESET_VAL	U(0x0)
#define HSTR_EL2_T_MASK		U(0xff)

/* CNTHP_CTL_EL2 definitions */
#define CNTHP_CTL_ENABLE_BIT	(U(1) << 0)
#define CNTHP_CTL_RESET_VAL	U(0x0)

/* VTTBR_EL2 definitions */
#define VTTBR_RESET_VAL		ULL(0x0)
#define VTTBR_VMID_MASK		ULL(0xff)
#define VTTBR_VMID_SHIFT	U(48)
#define VTTBR_BADDR_MASK	ULL(0xffffffffffff)
#define VTTBR_BADDR_SHIFT	U(0)

/* HCR definitions */
#define HCR_AMVOFFEN_BIT	(ULL(1) << 51)
#define HCR_API_BIT		(ULL(1) << 41)
#define HCR_APK_BIT		(ULL(1) << 40)
#define HCR_E2H_BIT		(ULL(1) << 34)
#define HCR_TGE_BIT		(ULL(1) << 27)
#define HCR_RW_SHIFT		U(31)
#define HCR_RW_BIT		(ULL(1) << HCR_RW_SHIFT)
#define HCR_AMO_BIT		(ULL(1) << 5)
#define HCR_IMO_BIT		(ULL(1) << 4)
#define HCR_FMO_BIT		(ULL(1) << 3)

/* ISR definitions */
#define ISR_A_SHIFT		U(8)
#define ISR_I_SHIFT		U(7)
#define ISR_F_SHIFT		U(6)

/* CNTHCTL_EL2 definitions */
#define CNTHCTL_RESET_VAL	U(0x0)
#define EVNTEN_BIT		(U(1) << 2)
#define EL1PCEN_BIT		(U(1) << 1)
#define EL1PCTEN_BIT		(U(1) << 0)

/* CNTKCTL_EL1 definitions */
#define EL0PTEN_BIT		(U(1) << 9)
#define EL0VTEN_BIT		(U(1) << 8)
#define EL0PCTEN_BIT		(U(1) << 0)
#define EL0VCTEN_BIT		(U(1) << 1)
#define EVNTEN_BIT		(U(1) << 2)
#define EVNTDIR_BIT		(U(1) << 3)
#define EVNTI_SHIFT		U(4)
#define EVNTI_MASK		U(0xf)

/* CPTR_EL3 definitions */
#define CPTR_EL3_TCPAC_BIT	(ULL(1) << 31)
#define CPTR_EL3_TAM_BIT	(ULL(1) << 30)
#define CPTR_EL3_TTA_BIT	(ULL(1) << 20)
#define CPTR_EL3_ESM_BIT	(ULL(1) << 12)
#define CPTR_EL3_TFP_BIT	(ULL(1) << 10)
#define CPTR_EL3_EZ_BIT		(ULL(1) << 8)

/* CPTR_EL2 definitions */
#define CPTR_EL2_RES1		((ULL(1) << 13) | (ULL(1) << 9) | (ULL(0xff)))
#define CPTR_EL2_TCPAC_BIT	(ULL(1) << 31)
#define CPTR_EL2_TAM_BIT	(ULL(1) << 30)
#define CPTR_EL2_TTA_BIT	(ULL(1) << 20)
#define CPTR_EL2_TSM_BIT	(ULL(1) << 12)
#define CPTR_EL2_TFP_BIT	(ULL(1) << 10)
#define CPTR_EL2_TZ_BIT		(ULL(1) << 8)
#define CPTR_EL2_RESET_VAL	CPTR_EL2_RES1

/* CPSR/SPSR definitions */
#define DAIF_FIQ_BIT		(U(1) << 0)
#define DAIF_IRQ_BIT		(U(1) << 1)
#define DAIF_ABT_BIT		(U(1) << 2)
#define DAIF_DBG_BIT		(U(1) << 3)
#define SPSR_DAIF_SHIFT		U(6)
#define SPSR_DAIF_MASK		U(0xf)

#define SPSR_AIF_SHIFT		U(6)
#define SPSR_AIF_MASK		U(0x7)

#define SPSR_E_SHIFT		U(9)
#define SPSR_E_MASK		U(0x1)
#define SPSR_E_LITTLE		U(0x0)
#define SPSR_E_BIG		U(0x1)

#define SPSR_T_SHIFT		U(5)
#define SPSR_T_MASK		U(0x1)
#define SPSR_T_ARM		U(0x0)
#define SPSR_T_THUMB		U(0x1)

#define SPSR_M_SHIFT		U(4)
#define SPSR_M_MASK		U(0x1)
#define SPSR_M_AARCH64		U(0x0)
#define SPSR_M_AARCH32		U(0x1)

#define DISABLE_ALL_EXCEPTIONS \
		(DAIF_FIQ_BIT | DAIF_IRQ_BIT | DAIF_ABT_BIT | DAIF_DBG_BIT)

#define DISABLE_INTERRUPTS	(DAIF_FIQ_BIT | DAIF_IRQ_BIT)

/*
 * RMR_EL3 definitions
 */
#define RMR_EL3_RR_BIT		(U(1) << 1)
#define RMR_EL3_AA64_BIT	(U(1) << 0)

/*
 * HI-VECTOR address for AArch32 state
 */
#define HI_VECTOR_BASE		U(0xFFFF0000)

/*
 * TCR defintions
 */
#define TCR_EL3_RES1		((ULL(1) << 31) | (ULL(1) << 23))
#define TCR_EL2_RES1		((ULL(1) << 31) | (ULL(1) << 23))
#define TCR_EL1_IPS_SHIFT	U(32)
#define TCR_EL2_PS_SHIFT	U(16)
#define TCR_EL3_PS_SHIFT	U(16)

#define TCR_TxSZ_MIN		ULL(16)
#define TCR_TxSZ_MAX		ULL(39)
#define TCR_TxSZ_MAX_TTST	ULL(48)

#define TCR_T0SZ_SHIFT		U(0)
#define TCR_T1SZ_SHIFT		U(16)

/* (internal) physical address size bits in EL3/EL1 */
#define TCR_PS_BITS_4GB		ULL(0x0)
#define TCR_PS_BITS_64GB	ULL(0x1)
#define TCR_PS_BITS_1TB		ULL(0x2)
#define TCR_PS_BITS_4TB		ULL(0x3)
#define TCR_PS_BITS_16TB	ULL(0x4)
#define TCR_PS_BITS_256TB	ULL(0x5)

#define ADDR_MASK_48_TO_63	ULL(0xFFFF000000000000)
#define ADDR_MASK_44_TO_47	ULL(0x0000F00000000000)
#define ADDR_MASK_42_TO_43	ULL(0x00000C0000000000)
#define ADDR_MASK_40_TO_41	ULL(0x0000030000000000)
#define ADDR_MASK_36_TO_39	ULL(0x000000F000000000)
#define ADDR_MASK_32_TO_35	ULL(0x0000000F00000000)

#define TCR_RGN_INNER_NC	(ULL(0x0) << 8)
#define TCR_RGN_INNER_WBA	(ULL(0x1) << 8)
#define TCR_RGN_INNER_WT	(ULL(0x2) << 8)
#define TCR_RGN_INNER_WBNA	(ULL(0x3) << 8)

#define TCR_RGN_OUTER_NC	(ULL(0x0) << 10)
#define TCR_RGN_OUTER_WBA	(ULL(0x1) << 10)
#define TCR_RGN_OUTER_WT	(ULL(0x2) << 10)
#define TCR_RGN_OUTER_WBNA	(ULL(0x3) << 10)

#define TCR_SH_NON_SHAREABLE	(ULL(0x0) << 12)
#define TCR_SH_OUTER_SHAREABLE	(ULL(0x2) << 12)
#define TCR_SH_INNER_SHAREABLE	(ULL(0x3) << 12)

#define TCR_RGN1_INNER_NC	(ULL(0x0) << 24)
#define TCR_RGN1_INNER_WBA	(ULL(0x1) << 24)
#define TCR_RGN1_INNER_WT	(ULL(0x2) << 24)
#define TCR_RGN1_INNER_WBNA	(ULL(0x3) << 24)

#define TCR_RGN1_OUTER_NC	(ULL(0x0) << 26)
#define TCR_RGN1_OUTER_WBA	(ULL(0x1) << 26)
#define TCR_RGN1_OUTER_WT	(ULL(0x2) << 26)
#define TCR_RGN1_OUTER_WBNA	(ULL(0x3) << 26)

#define TCR_SH1_NON_SHAREABLE	(ULL(0x0) << 28)
#define TCR_SH1_OUTER_SHAREABLE	(ULL(0x2) << 28)
#define TCR_SH1_INNER_SHAREABLE	(ULL(0x3) << 28)

#define TCR_TG0_SHIFT		U(14)
#define TCR_TG0_MASK		ULL(3)
#define TCR_TG0_4K		(ULL(0) << TCR_TG0_SHIFT)
#define TCR_TG0_64K		(ULL(1) << TCR_TG0_SHIFT)
#define TCR_TG0_16K		(ULL(2) << TCR_TG0_SHIFT)

#define TCR_TG1_SHIFT		U(30)
#define TCR_TG1_MASK		ULL(3)
#define TCR_TG1_16K		(ULL(1) << TCR_TG1_SHIFT)
#define TCR_TG1_4K		(ULL(2) << TCR_TG1_SHIFT)
#define TCR_TG1_64K		(ULL(3) << TCR_TG1_SHIFT)

#define TCR_EPD0_BIT		(ULL(1) << 7)
#define TCR_EPD1_BIT		(ULL(1) << 23)

#define MODE_SP_SHIFT		U(0x0)
#define MODE_SP_MASK		U(0x1)
#define MODE_SP_EL0		U(0x0)
#define MODE_SP_ELX		U(0x1)

#define MODE_RW_SHIFT		U(0x4)
#define MODE_RW_MASK		U(0x1)
#define MODE_RW_64		U(0x0)
#define MODE_RW_32		U(0x1)

#define MODE_EL_SHIFT		U(0x2)
#define MODE_EL_MASK		U(0x3)
#define MODE_EL3		U(0x3)
#define MODE_EL2		U(0x2)
#define MODE_EL1		U(0x1)
#define MODE_EL0		U(0x0)

#define MODE32_SHIFT		U(0)
#define MODE32_MASK		U(0xf)
#define MODE32_usr		U(0x0)
#define MODE32_fiq		U(0x1)
#define MODE32_irq		U(0x2)
#define MODE32_svc		U(0x3)
#define MODE32_mon		U(0x6)
#define MODE32_abt		U(0x7)
#define MODE32_hyp		U(0xa)
#define MODE32_und		U(0xb)
#define MODE32_sys		U(0xf)

#define GET_RW(mode)		(((mode) >> MODE_RW_SHIFT) & MODE_RW_MASK)
#define GET_EL(mode)		(((mode) >> MODE_EL_SHIFT) & MODE_EL_MASK)
#define GET_SP(mode)		(((mode) >> MODE_SP_SHIFT) & MODE_SP_MASK)
#define GET_M32(mode)		(((mode) >> MODE32_SHIFT) & MODE32_MASK)

#define SPSR_64(el, sp, daif)				\
	((MODE_RW_64 << MODE_RW_SHIFT) |		\
	(((el) & MODE_EL_MASK) << MODE_EL_SHIFT) |	\
	(((sp) & MODE_SP_MASK) << MODE_SP_SHIFT) |	\
	(((daif) & SPSR_DAIF_MASK) << SPSR_DAIF_SHIFT))

#define SPSR_MODE32(mode, isa, endian, aif)		\
	((MODE_RW_32 << MODE_RW_SHIFT) |		\
	(((mode) & MODE32_MASK) << MODE32_SHIFT) |	\
	(((isa) & SPSR_T_MASK) << SPSR_T_SHIFT) |	\
	(((endian) & SPSR_E_MASK) << SPSR_E_SHIFT) |	\
	(((aif) & SPSR_AIF_MASK) << SPSR_AIF_SHIFT))

/*
 * TTBR Definitions
 */
#define TTBR_CNP_BIT		ULL(0x1)

/*
 * CTR_EL0 definitions
 */
#define CTR_CWG_SHIFT		U(24)
#define CTR_CWG_MASK		U(0xf)
#define CTR_ERG_SHIFT		U(20)
#define CTR_ERG_MASK		U(0xf)
#define CTR_DMINLINE_SHIFT	U(16)
#define CTR_DMINLINE_MASK	U(0xf)
#define CTR_L1IP_SHIFT		U(14)
#define CTR_L1IP_MASK		U(0x3)
#define CTR_IMINLINE_SHIFT	U(0)
#define CTR_IMINLINE_MASK	U(0xf)

#define MAX_CACHE_LINE_SIZE	U(0x800) /* 2KB */

/*
 * FPCR definitions
 */
#define FPCR_FIZ_BIT		(ULL(1) << 0)
#define FPCR_AH_BIT		(ULL(1) << 1)
#define FPCR_NEP_BIT		(ULL(1) << 2)

/* Physical timer control register bit fields shifts and masks */
#define CNTP_CTL_ENABLE_SHIFT   U(0)
#define CNTP_CTL_IMASK_SHIFT    U(1)
#define CNTP_CTL_ISTATUS_SHIFT  U(2)

#define CNTP_CTL_ENABLE_MASK    U(1)
#define CNTP_CTL_IMASK_MASK     U(1)
#define CNTP_CTL_ISTATUS_MASK   U(1)

/* Exception Syndrome register bits and bobs */
#define ESR_EC_SHIFT			U(26)
#define ESR_EC_MASK			U(0x3f)
#define ESR_EC_LENGTH			U(6)
#define ESR_ISS_SHIFT			U(0x0)
#define ESR_ISS_MASK			U(0x1ffffff)
#define EC_UNKNOWN			U(0x0)
#define EC_WFE_WFI			U(0x1)
#define EC_AARCH32_CP15_MRC_MCR		U(0x3)
#define EC_AARCH32_CP15_MRRC_MCRR	U(0x4)
#define EC_AARCH32_CP14_MRC_MCR		U(0x5)
#define EC_AARCH32_CP14_LDC_STC		U(0x6)
#define EC_FP_SIMD			U(0x7)
#define EC_AARCH32_CP10_MRC		U(0x8)
#define EC_AARCH32_CP14_MRRC_MCRR	U(0xc)
#define EC_ILLEGAL			U(0xe)
#define EC_AARCH32_SVC			U(0x11)
#define EC_AARCH32_HVC			U(0x12)
#define EC_AARCH32_SMC			U(0x13)
#define EC_AARCH64_SVC			U(0x15)
#define EC_AARCH64_HVC			U(0x16)
#define EC_AARCH64_SMC			U(0x17)
#define EC_AARCH64_SYS			U(0x18)
#define EC_IABORT_LOWER_EL		U(0x20)
#define EC_IABORT_CUR_EL		U(0x21)
#define EC_PC_ALIGN			U(0x22)
#define EC_DABORT_LOWER_EL		U(0x24)
#define EC_DABORT_CUR_EL		U(0x25)
#define EC_SP_ALIGN			U(0x26)
#define EC_AARCH32_FP			U(0x28)
#define EC_AARCH64_FP			U(0x2c)
#define EC_SERROR			U(0x2f)

/* Common DFSC/IFSC code */
#define ISS_FSC_MASK			U(0x3f)
#define FSC_L0_ADR_SIZE_FAULT		U(0)
#define FSC_L0_TRANS_FAULT		U(4)
#define FSC_L1_TRANS_FAULT		U(5)
#define FSC_L2_TRANS_FAULT		U(6)
#define FSC_L3_TRANS_FAULT		U(7)
#define FSC_L_MINUS1_TRANS_FAULT	U(0x2B)
#define FSC_L0_PERM_FAULT		U(0xC)
#define FSC_L1_PERM_FAULT		U(0xD)
#define FSC_L2_PERM_FAULT		U(0xE)
#define FSC_L3_PERM_FAULT		U(0xF)

/* Data Fault Status code, not all error codes listed */
#define ISS_DFSC_MASK			U(0x3f)
#define DFSC_NO_WALK_SEA		U(0x10)
#define DFSC_L0_SEA			U(0x14)
#define DFSC_L1_SEA			U(0x15)
#define DFSC_L2_SEA			U(0x16)
#define DFSC_L3_SEA			U(0x17)
#define DFSC_EXT_DABORT			U(0x10)
#define DFSC_GPF_DABORT			U(0x28)

/* Instr Fault Status code, not all error codes listed */
#define ISS_IFSC_MASK			U(0x3f)
#define IFSC_NO_WALK_SEA		U(0x10)
#define IFSC_L0_SEA			U(0x24)
#define IFSC_L1_SEA			U(0x25)
#define IFSC_L2_SEA			U(0x26)
#define IFSC_L3_SEA			U(0x27)

/* ISS encoding an exception from HVC or SVC instruction execution */
#define ISS_HVC_SMC_IMM16_MASK		U(0xffff)

/*
 * External Abort bit in Instruction and Data Aborts synchronous exception
 * syndromes.
 */
#define ESR_ISS_EABORT_EA_BIT		U(9)

#define EC_BITS(x)			(((x) >> ESR_EC_SHIFT) & ESR_EC_MASK)
#define ISS_BITS(x)			(((x) >> ESR_ISS_SHIFT) & ESR_ISS_MASK)

/* Reset bit inside the Reset management register for EL3 (RMR_EL3) */
#define RMR_RESET_REQUEST_SHIFT 	U(0x1)
#define RMR_WARM_RESET_CPU		(U(1) << RMR_RESET_REQUEST_SHIFT)

/*******************************************************************************
 * Definitions of register offsets, fields and macros for CPU system
 * instructions.
 ******************************************************************************/

#define TLBI_ADDR_SHIFT		U(12)
#define TLBI_ADDR_MASK		ULL(0x00000FFFFFFFFFFF)
#define TLBI_ADDR(x)		(((x) >> TLBI_ADDR_SHIFT) & TLBI_ADDR_MASK)

/*******************************************************************************
 * Definitions of register offsets and fields in the CNTCTLBase Frame of the
 * system level implementation of the Generic Timer.
 ******************************************************************************/
#define CNTCTLBASE_CNTFRQ	U(0x0)
#define CNTNSAR			U(0x4)
#define CNTNSAR_NS_SHIFT(x)	(x)

#define CNTACR_BASE(x)		(U(0x40) + ((x) << 2))
#define CNTACR_RPCT_SHIFT	U(0x0)
#define CNTACR_RVCT_SHIFT	U(0x1)
#define CNTACR_RFRQ_SHIFT	U(0x2)
#define CNTACR_RVOFF_SHIFT	U(0x3)
#define CNTACR_RWVT_SHIFT	U(0x4)
#define CNTACR_RWPT_SHIFT	U(0x5)

/*******************************************************************************
 * Definitions of register offsets and fields in the CNTBaseN Frame of the
 * system level implementation of the Generic Timer.
 ******************************************************************************/
/* Physical Count register. */
#define CNTPCT_LO		U(0x0)
/* Counter Frequency register. */
#define CNTBASEN_CNTFRQ		U(0x10)
/* Physical Timer CompareValue register. */
#define CNTP_CVAL_LO		U(0x20)
/* Physical Timer Control register. */
#define CNTP_CTL		U(0x2c)

/* PMCR_EL0 definitions */
#define PMCR_EL0_RESET_VAL	U(0x0)
#define PMCR_EL0_N_SHIFT	U(11)
#define PMCR_EL0_N_MASK		U(0x1f)
#define PMCR_EL0_N_BITS		(PMCR_EL0_N_MASK << PMCR_EL0_N_SHIFT)
#define PMCR_EL0_LC_BIT		(U(1) << 6)
#define PMCR_EL0_DP_BIT		(U(1) << 5)
#define PMCR_EL0_X_BIT		(U(1) << 4)
#define PMCR_EL0_D_BIT		(U(1) << 3)
#define PMCR_EL0_C_BIT		(U(1) << 2)
#define PMCR_EL0_P_BIT		(U(1) << 1)
#define PMCR_EL0_E_BIT		(U(1) << 0)

/* PMCNTENSET_EL0 definitions */
#define PMCNTENSET_EL0_C_BIT		(U(1) << 31)
#define PMCNTENSET_EL0_P_BIT(x)		(U(1) << x)

/* PMEVTYPER<n>_EL0 definitions */
#define PMEVTYPER_EL0_P_BIT		(U(1) << 31)
#define PMEVTYPER_EL0_U_BIT		(U(1) << 30)
#define PMEVTYPER_EL0_NSK_BIT		(U(1) << 29)
#define PMEVTYPER_EL0_NSU_BIT		(U(1) << 28)
#define PMEVTYPER_EL0_NSH_BIT		(U(1) << 27)
#define PMEVTYPER_EL0_M_BIT		(U(1) << 26)
#define PMEVTYPER_EL0_MT_BIT		(U(1) << 25)
#define PMEVTYPER_EL0_SH_BIT		(U(1) << 24)
#define PMEVTYPER_EL0_T_BIT		(U(1) << 23)
#define PMEVTYPER_EL0_RLK_BIT		(U(1) << 22)
#define PMEVTYPER_EL0_RLU_BIT		(U(1) << 21)
#define PMEVTYPER_EL0_RLH_BIT		(U(1) << 20)
#define PMEVTYPER_EL0_EVTCOUNT_BITS	U(0x0000FFFF)

/* PMCCFILTR_EL0 definitions */
#define PMCCFILTR_EL0_P_BIT		(U(1) << 31)
#define PMCCFILTR_EL0_U_BIT		(U(1) << 30)
#define PMCCFILTR_EL0_NSK_BIT		(U(1) << 29)
#define PMCCFILTR_EL0_NSH_BIT		(U(1) << 27)
#define PMCCFILTR_EL0_M_BIT		(U(1) << 26)
#define PMCCFILTR_EL0_SH_BIT		(U(1) << 24)
#define PMCCFILTR_EL0_T_BIT		(U(1) << 23)
#define PMCCFILTR_EL0_RLK_BIT		(U(1) << 22)
#define PMCCFILTR_EL0_RLU_BIT		(U(1) << 21)
#define PMCCFILTR_EL0_RLH_BIT		(U(1) << 20)

/* PMSELR_EL0 definitions */
#define PMSELR_EL0_SEL_SHIFT		U(0)
#define PMSELR_EL0_SEL_MASK		U(0x1f)

/* PMU event counter ID definitions */
#define PMU_EV_PC_WRITE_RETIRED		U(0x000C)

/*******************************************************************************
 * Definitions for system register interface to SVE
 ******************************************************************************/
#define ID_AA64ZFR0_EL1		S3_0_C0_C4_4

/* ZCR_EL2 definitions */
#define ZCR_EL2			S3_4_C1_C2_0
#define ZCR_EL2_SVE_VL_SHIFT	UL(0)
#define ZCR_EL2_SVE_VL_WIDTH	UL(4)

/* ZCR_EL1 definitions */
#define ZCR_EL1			S3_0_C1_C2_0
#define ZCR_EL1_SVE_VL_SHIFT	UL(0)
#define ZCR_EL1_SVE_VL_WIDTH	UL(4)

/*******************************************************************************
 * Definitions for system register interface to SME
 ******************************************************************************/
#define ID_AA64SMFR0_EL1		S3_0_C0_C4_5
#define SVCR				S3_3_C4_C2_2
#define TPIDR2_EL0			S3_3_C13_C0_5
#define SMCR_EL2			S3_4_C1_C2_6

/* ID_AA64SMFR0_EL1 definitions */
#define ID_AA64SMFR0_EL1_FA64_BIT	(UL(1) << 63)

/* SVCR definitions */
#define SVCR_ZA_BIT			(U(1) << 1)
#define SVCR_SM_BIT			(U(1) << 0)

/* SMPRI_EL1 definitions */
#define SMPRI_EL1_PRIORITY_SHIFT	U(0)
#define SMPRI_EL1_PRIORITY_MASK		U(0xf)

/* SMPRIMAP_EL2 definitions */
/* Register is composed of 16 priority map fields of 4 bits numbered 0-15. */
#define SMPRIMAP_EL2_MAP_SHIFT(pri)	U((pri) * 4)
#define SMPRIMAP_EL2_MAP_MASK		U(0xf)

/* SMCR_ELx definitions */
#define SMCR_ELX_LEN_SHIFT		U(0)
#define SMCR_ELX_LEN_WIDTH		U(4)
/*
 * SMCR_ELX_RAZ_LEN is defined to find the architecturally permitted SVL. This
 * is a combination of RAZ and LEN bit fields.
 */
#define SMCR_ELX_RAZ_LEN_SHIFT		UL(0)
#define SMCR_ELX_RAZ_LEN_WIDTH		UL(9)
#define SMCR_ELX_EZT0_BIT		(U(1) << 30)
#define SMCR_ELX_FA64_BIT		(U(1) << 31)
#define SMCR_EL2_RESET_VAL		(SMCR_ELX_EZT0_BIT | SMCR_ELX_FA64_BIT)

/*******************************************************************************
 * Definitions of MAIR encodings for device and normal memory
 ******************************************************************************/
/*
 * MAIR encodings for device memory attributes.
 */
#define MAIR_DEV_nGnRnE		ULL(0x0)
#define MAIR_DEV_nGnRE		ULL(0x4)
#define MAIR_DEV_nGRE		ULL(0x8)
#define MAIR_DEV_GRE		ULL(0xc)

/*
 * MAIR encodings for normal memory attributes.
 *
 * Cache Policy
 *  WT:	 Write Through
 *  WB:	 Write Back
 *  NC:	 Non-Cacheable
 *
 * Transient Hint
 *  NTR: Non-Transient
 *  TR:	 Transient
 *
 * Allocation Policy
 *  RA:	 Read Allocate
 *  WA:	 Write Allocate
 *  RWA: Read and Write Allocate
 *  NA:	 No Allocation
 */
#define MAIR_NORM_WT_TR_WA	ULL(0x1)
#define MAIR_NORM_WT_TR_RA	ULL(0x2)
#define MAIR_NORM_WT_TR_RWA	ULL(0x3)
#define MAIR_NORM_NC		ULL(0x4)
#define MAIR_NORM_WB_TR_WA	ULL(0x5)
#define MAIR_NORM_WB_TR_RA	ULL(0x6)
#define MAIR_NORM_WB_TR_RWA	ULL(0x7)
#define MAIR_NORM_WT_NTR_NA	ULL(0x8)
#define MAIR_NORM_WT_NTR_WA	ULL(0x9)
#define MAIR_NORM_WT_NTR_RA	ULL(0xa)
#define MAIR_NORM_WT_NTR_RWA	ULL(0xb)
#define MAIR_NORM_WB_NTR_NA	ULL(0xc)
#define MAIR_NORM_WB_NTR_WA	ULL(0xd)
#define MAIR_NORM_WB_NTR_RA	ULL(0xe)
#define MAIR_NORM_WB_NTR_RWA	ULL(0xf)

#define MAIR_NORM_OUTER_SHIFT	U(4)

#define MAKE_MAIR_NORMAL_MEMORY(inner, outer)	\
		((inner) | ((outer) << MAIR_NORM_OUTER_SHIFT))

/* PAR_EL1 fields */
#define PAR_F_SHIFT	U(0)
#define PAR_F_MASK	ULL(0x1)
#define PAR_ADDR_SHIFT	U(12)
#define PAR_ADDR_MASK	(BIT(40) - ULL(1)) /* 40-bits-wide page address */

/*******************************************************************************
 * Definitions for system register interface to SPE
 ******************************************************************************/
#define PMSCR_EL1		S3_0_C9_C9_0
#define PMSNEVFR_EL1		S3_0_C9_C9_1
#define PMSICR_EL1		S3_0_C9_C9_2
#define PMSIRR_EL1		S3_0_C9_C9_3
#define PMSFCR_EL1		S3_0_C9_C9_4
#define PMSEVFR_EL1		S3_0_C9_C9_5
#define PMSLATFR_EL1		S3_0_C9_C9_6
#define PMSIDR_EL1		S3_0_C9_C9_7
#define PMBLIMITR_EL1		S3_0_C9_C10_0
#define PMBPTR_EL1		S3_0_C9_C10_1
#define PMBSR_EL1		S3_0_C9_C10_3
#define PMSCR_EL2		S3_4_C9_C9_0

/*******************************************************************************
 * Definitions for system register interface to MPAM
 ******************************************************************************/
#define MPAMIDR_EL1		S3_0_C10_C4_4
#define MPAM2_EL2		S3_4_C10_C5_0
#define MPAMHCR_EL2		S3_4_C10_C4_0
#define MPAM3_EL3		S3_6_C10_C5_0

/*******************************************************************************
 * Definitions for system register interface to AMU for ARMv8.4 onwards
 ******************************************************************************/
#define AMCR_EL0		S3_3_C13_C2_0
#define AMCFGR_EL0		S3_3_C13_C2_1
#define AMCGCR_EL0		S3_3_C13_C2_2
#define AMUSERENR_EL0		S3_3_C13_C2_3
#define AMCNTENCLR0_EL0		S3_3_C13_C2_4
#define AMCNTENSET0_EL0		S3_3_C13_C2_5
#define AMCNTENCLR1_EL0		S3_3_C13_C3_0
#define AMCNTENSET1_EL0		S3_3_C13_C3_1

/* Activity Monitor Group 0 Event Counter Registers */
#define AMEVCNTR00_EL0		S3_3_C13_C4_0
#define AMEVCNTR01_EL0		S3_3_C13_C4_1
#define AMEVCNTR02_EL0		S3_3_C13_C4_2
#define AMEVCNTR03_EL0		S3_3_C13_C4_3

/* Activity Monitor Group 0 Event Type Registers */
#define AMEVTYPER00_EL0		S3_3_C13_C6_0
#define AMEVTYPER01_EL0		S3_3_C13_C6_1
#define AMEVTYPER02_EL0		S3_3_C13_C6_2
#define AMEVTYPER03_EL0		S3_3_C13_C6_3

/* Activity Monitor Group 1 Event Counter Registers */
#define AMEVCNTR10_EL0		S3_3_C13_C12_0
#define AMEVCNTR11_EL0		S3_3_C13_C12_1
#define AMEVCNTR12_EL0		S3_3_C13_C12_2
#define AMEVCNTR13_EL0		S3_3_C13_C12_3
#define AMEVCNTR14_EL0		S3_3_C13_C12_4
#define AMEVCNTR15_EL0		S3_3_C13_C12_5
#define AMEVCNTR16_EL0		S3_3_C13_C12_6
#define AMEVCNTR17_EL0		S3_3_C13_C12_7
#define AMEVCNTR18_EL0		S3_3_C13_C13_0
#define AMEVCNTR19_EL0		S3_3_C13_C13_1
#define AMEVCNTR1A_EL0		S3_3_C13_C13_2
#define AMEVCNTR1B_EL0		S3_3_C13_C13_3
#define AMEVCNTR1C_EL0		S3_3_C13_C13_4
#define AMEVCNTR1D_EL0		S3_3_C13_C13_5
#define AMEVCNTR1E_EL0		S3_3_C13_C13_6
#define AMEVCNTR1F_EL0		S3_3_C13_C13_7

/* Activity Monitor Group 1 Event Type Registers */
#define AMEVTYPER10_EL0		S3_3_C13_C14_0
#define AMEVTYPER11_EL0		S3_3_C13_C14_1
#define AMEVTYPER12_EL0		S3_3_C13_C14_2
#define AMEVTYPER13_EL0		S3_3_C13_C14_3
#define AMEVTYPER14_EL0		S3_3_C13_C14_4
#define AMEVTYPER15_EL0		S3_3_C13_C14_5
#define AMEVTYPER16_EL0		S3_3_C13_C14_6
#define AMEVTYPER17_EL0		S3_3_C13_C14_7
#define AMEVTYPER18_EL0		S3_3_C13_C15_0
#define AMEVTYPER19_EL0		S3_3_C13_C15_1
#define AMEVTYPER1A_EL0		S3_3_C13_C15_2
#define AMEVTYPER1B_EL0		S3_3_C13_C15_3
#define AMEVTYPER1C_EL0		S3_3_C13_C15_4
#define AMEVTYPER1D_EL0		S3_3_C13_C15_5
#define AMEVTYPER1E_EL0		S3_3_C13_C15_6
#define AMEVTYPER1F_EL0		S3_3_C13_C15_7

/* AMCFGR_EL0 definitions */
#define AMCFGR_EL0_NCG_SHIFT		U(28)
#define AMCFGR_EL0_NCG_MASK		U(0xf)

/* AMCGCR_EL0 definitions */
#define AMCGCR_EL0_CG1NC_SHIFT		U(8)
#define AMCGCR_EL0_CG1NC_LENGTH		U(8)
#define AMCGCR_EL0_CG1NC_MASK		U(0xff)

/* MPAM register definitions */
#define MPAM3_EL3_MPAMEN_BIT		(ULL(1) << 63)
#define MPAM3_EL3_TRAPLOWER_BIT		(ULL(1) << 62)
#define MPAMHCR_EL2_TRAP_MPAMIDR_EL1	(ULL(1) << 31)

#define MPAM2_EL2_TRAPMPAM0EL1		(ULL(1) << 49)
#define MPAM2_EL2_TRAPMPAM1EL1		(ULL(1) << 48)

#define MPAMIDR_HAS_HCR_BIT		(ULL(1) << 17)

/*******************************************************************************
 * Definitions for system register interface to AMU for ARMv8.6 enhancements
 ******************************************************************************/

/* Definition for register defining which virtual offsets are implemented. */
#define AMCG1IDR_EL0		S3_3_C13_C2_6
#define AMCG1IDR_CTR_MASK	ULL(0xffff)
#define AMCG1IDR_CTR_SHIFT	U(0)
#define AMCG1IDR_VOFF_MASK	ULL(0xffff)
#define AMCG1IDR_VOFF_SHIFT	U(16)

/* New bit added to AMCR_EL0 */
#define AMCR_CG1RZ_BIT		(ULL(0x1) << 17)

/* Definitions for virtual offset registers for architected event counters. */
/* AMEVCNTR01_EL0 intentionally left undefined, as it does not exist. */
#define AMEVCNTVOFF00_EL2	S3_4_C13_C8_0
#define AMEVCNTVOFF02_EL2	S3_4_C13_C8_2
#define AMEVCNTVOFF03_EL2	S3_4_C13_C8_3

/* Definitions for virtual offset registers for auxiliary event counters. */
#define AMEVCNTVOFF10_EL2	S3_4_C13_C10_0
#define AMEVCNTVOFF11_EL2	S3_4_C13_C10_1
#define AMEVCNTVOFF12_EL2	S3_4_C13_C10_2
#define AMEVCNTVOFF13_EL2	S3_4_C13_C10_3
#define AMEVCNTVOFF14_EL2	S3_4_C13_C10_4
#define AMEVCNTVOFF15_EL2	S3_4_C13_C10_5
#define AMEVCNTVOFF16_EL2	S3_4_C13_C10_6
#define AMEVCNTVOFF17_EL2	S3_4_C13_C10_7
#define AMEVCNTVOFF18_EL2	S3_4_C13_C11_0
#define AMEVCNTVOFF19_EL2	S3_4_C13_C11_1
#define AMEVCNTVOFF1A_EL2	S3_4_C13_C11_2
#define AMEVCNTVOFF1B_EL2	S3_4_C13_C11_3
#define AMEVCNTVOFF1C_EL2	S3_4_C13_C11_4
#define AMEVCNTVOFF1D_EL2	S3_4_C13_C11_5
#define AMEVCNTVOFF1E_EL2	S3_4_C13_C11_6
#define AMEVCNTVOFF1F_EL2	S3_4_C13_C11_7

/*******************************************************************************
 * RAS system registers
 ******************************************************************************/
#define DISR_EL1		S3_0_C12_C1_1
#define DISR_A_BIT		U(31)

#define ERRIDR_EL1		S3_0_C5_C3_0
#define ERRIDR_MASK		U(0xffff)

#define ERRSELR_EL1		S3_0_C5_C3_1

/* System register access to Standard Error Record registers */
#define ERXFR_EL1		S3_0_C5_C4_0
#define ERXCTLR_EL1		S3_0_C5_C4_1
#define ERXSTATUS_EL1		S3_0_C5_C4_2
#define ERXADDR_EL1		S3_0_C5_C4_3
#define ERXPFGF_EL1		S3_0_C5_C4_4
#define ERXPFGCTL_EL1		S3_0_C5_C4_5
#define ERXPFGCDN_EL1		S3_0_C5_C4_6
#define ERXMISC0_EL1		S3_0_C5_C5_0
#define ERXMISC1_EL1		S3_0_C5_C5_1

#define ERXCTLR_ED_BIT		(U(1) << 0)
#define ERXCTLR_UE_BIT		(U(1) << 4)

#define ERXPFGCTL_UC_BIT	(U(1) << 1)
#define ERXPFGCTL_UEU_BIT	(U(1) << 2)
#define ERXPFGCTL_CDEN_BIT	(U(1) << 31)

/*******************************************************************************
 * Armv8.1 Registers - Privileged Access Never Registers
 ******************************************************************************/
#define PAN			S3_0_C4_C2_3
#define PAN_BIT			BIT(22)

/*******************************************************************************
 * Armv8.3 Pointer Authentication Registers
 ******************************************************************************/
#define APIAKeyLo_EL1		S3_0_C2_C1_0
#define APIAKeyHi_EL1		S3_0_C2_C1_1
#define APIBKeyLo_EL1		S3_0_C2_C1_2
#define APIBKeyHi_EL1		S3_0_C2_C1_3
#define APDAKeyLo_EL1		S3_0_C2_C2_0
#define APDAKeyHi_EL1		S3_0_C2_C2_1
#define APDBKeyLo_EL1		S3_0_C2_C2_2
#define APDBKeyHi_EL1		S3_0_C2_C2_3
#define APGAKeyLo_EL1		S3_0_C2_C3_0
#define APGAKeyHi_EL1		S3_0_C2_C3_1

/*******************************************************************************
 * Armv8.4 Data Independent Timing Registers
 ******************************************************************************/
#define DIT			S3_3_C4_C2_5
#define DIT_BIT			BIT(24)

/*******************************************************************************
 * Armv8.5 - new MSR encoding to directly access PSTATE.SSBS field
 ******************************************************************************/
#define SSBS			S3_3_C4_C2_6

/*******************************************************************************
 * Armv8.5 - Memory Tagging Extension Registers
 ******************************************************************************/
#define TFSRE0_EL1		S3_0_C5_C6_1
#define TFSR_EL1		S3_0_C5_C6_0
#define RGSR_EL1		S3_0_C1_C0_5
#define GCR_EL1			S3_0_C1_C0_6

/*******************************************************************************
 * Armv8.6 - Fine Grained Virtualization Traps Registers
 ******************************************************************************/
#define HFGRTR_EL2		S3_4_C1_C1_4
#define HFGWTR_EL2		S3_4_C1_C1_5
#define HFGITR_EL2		S3_4_C1_C1_6
#define HDFGRTR_EL2		S3_4_C3_C1_4
#define HDFGWTR_EL2		S3_4_C3_C1_5

/*******************************************************************************
 * Armv8.9 - Fine Grained Virtualization Traps 2 Registers
 ******************************************************************************/
#define HFGRTR2_EL2            S3_4_C3_C1_2
#define HFGWTR2_EL2            S3_4_C3_C1_3
#define HFGITR2_EL2            S3_4_C3_C1_7
#define HDFGRTR2_EL2           S3_4_C3_C1_0
#define HDFGWTR2_EL2           S3_4_C3_C1_1

/*******************************************************************************
 * Armv8.6 - Enhanced Counter Virtualization Registers
 ******************************************************************************/
#define CNTPOFF_EL2  S3_4_C14_C0_6

/*******************************************************************************
 * Armv8.7 - LoadStore64Bytes Registers
 ******************************************************************************/
#define SYS_ACCDATA_EL1		S3_0_C13_C0_5

/******************************************************************************
 * Armv8.9 - Breakpoint and Watchpoint Selection Register
 ******************************************************************************/
#define MDSELR_EL1		S2_0_C0_C4_2

/******************************************************************************
 * Armv8.9 - Translation Hardening Extension Registers
 ******************************************************************************/
#define RCWMASK_EL1		S3_0_C13_C0_6
#define RCWSMASK_EL1		S3_0_C13_C0_3

/*******************************************************************************
 * Armv9.0 - Trace Buffer Extension System Registers
 ******************************************************************************/
#define TRBLIMITR_EL1	S3_0_C9_C11_0
#define TRBPTR_EL1	S3_0_C9_C11_1
#define TRBBASER_EL1	S3_0_C9_C11_2
#define TRBSR_EL1	S3_0_C9_C11_3
#define TRBMAR_EL1	S3_0_C9_C11_4
#define TRBTRG_EL1	S3_0_C9_C11_6
#define TRBIDR_EL1	S3_0_C9_C11_7

/*******************************************************************************
 * FEAT_BRBE - Branch Record Buffer Extension System Registers
 ******************************************************************************/

#define BRBCR_EL1	S2_1_C9_C0_0
#define BRBCR_EL2	S2_4_C9_C0_0
#define BRBFCR_EL1	S2_1_C9_C0_1
#define BRBTS_EL1	S2_1_C9_C0_2
#define BRBINFINJ_EL1	S2_1_C9_C1_0
#define BRBSRCINJ_EL1	S2_1_C9_C1_1
#define BRBTGTINJ_EL1	S2_1_C9_C1_2
#define BRBIDR0_EL1	S2_1_C9_C2_0

/*******************************************************************************
 * FEAT_TCR2 - Extended Translation Control Registers
 ******************************************************************************/
#define TCR2_EL1		S3_0_C2_C0_3
#define TCR2_EL2		S3_4_C2_C0_3

/*******************************************************************************
 * Armv8.4 - Trace Filter System Registers
 ******************************************************************************/
#define TRFCR_EL1	S3_0_C1_C2_1
#define TRFCR_EL2	S3_4_C1_C2_1

/*******************************************************************************
 * Trace System Registers
 ******************************************************************************/
#define TRCAUXCTLR	S2_1_C0_C6_0
#define TRCRSR		S2_1_C0_C10_0
#define TRCCCCTLR	S2_1_C0_C14_0
#define TRCBBCTLR	S2_1_C0_C15_0
#define TRCEXTINSELR0	S2_1_C0_C8_4
#define TRCEXTINSELR1	S2_1_C0_C9_4
#define TRCEXTINSELR2	S2_1_C0_C10_4
#define TRCEXTINSELR3	S2_1_C0_C11_4
#define TRCCLAIMSET	S2_1_c7_c8_6
#define TRCCLAIMCLR	S2_1_c7_c9_6
#define TRCDEVARCH	S2_1_c7_c15_6

/*******************************************************************************
 * FEAT_HCX - Extended Hypervisor Configuration Register
 ******************************************************************************/
#define HCRX_EL2		S3_4_C1_C2_2
#define HCRX_EL2_MSCEn_BIT	(UL(1) << 11)
#define HCRX_EL2_MCE2_BIT	(UL(1) << 10)
#define HCRX_EL2_CMOW_BIT	(UL(1) << 9)
#define HCRX_EL2_VFNMI_BIT	(UL(1) << 8)
#define HCRX_EL2_VINMI_BIT	(UL(1) << 7)
#define HCRX_EL2_TALLINT_BIT	(UL(1) << 6)
#define HCRX_EL2_SMPME_BIT	(UL(1) << 5)
#define HCRX_EL2_FGTnXS_BIT	(UL(1) << 4)
#define HCRX_EL2_FnXS_BIT	(UL(1) << 3)
#define HCRX_EL2_EnASR_BIT	(UL(1) << 2)
#define HCRX_EL2_EnALS_BIT	(UL(1) << 1)
#define HCRX_EL2_EnAS0_BIT	(UL(1) << 0)
#define HCRX_EL2_INIT_VAL	ULL(0x0)

/*******************************************************************************
 * PFR0_EL1 - Definitions for AArch32 Processor Feature Register 0
 ******************************************************************************/
#define ID_PFR0_EL1				S3_0_C0_C1_0
#define ID_PFR0_EL1_RAS_MASK			ULL(0xf)
#define ID_PFR0_EL1_RAS_SHIFT			U(28)
#define ID_PFR0_EL1_RAS_WIDTH			U(4)
#define ID_PFR0_EL1_RAS_SUPPORTED		ULL(0x1)
#define ID_PFR0_EL1_RASV1P1_SUPPORTED		ULL(0x2)

/*******************************************************************************
 * PFR2_EL1 - Definitions for AArch32 Processor Feature Register 2
 ******************************************************************************/
#define ID_PFR2_EL1				S3_0_C0_C3_4
#define ID_PFR2_EL1_RAS_FRAC_MASK		ULL(0xf)
#define ID_PFR2_EL1_RAS_FRAC_SHIFT		U(8)
#define ID_PFR2_EL1_RAS_FRAC_WIDTH		U(4)
#define ID_PFR2_EL1_RASV1P1_SUPPORTED		ULL(0x1)

/*******************************************************************************
 * FEAT_FGT - Definitions for Fine-Grained Trap registers
 ******************************************************************************/
#define HFGITR_EL2_INIT_VAL			ULL(0x180000000000000)
#define HFGITR_EL2_FEAT_BRBE_MASK		ULL(0x180000000000000)
#define HFGITR_EL2_FEAT_SPECRES_MASK		ULL(0x7000000000000)
#define HFGITR_EL2_FEAT_TLBIRANGE_MASK		ULL(0x3fc00000000)
#define HFGITR_EL2_FEAT_TLBIRANGE_TLBIOS_MASK	ULL(0xf000000)
#define HFGITR_EL2_FEAT_TLBIOS_MASK		ULL(0xfc0000)
#define HFGITR_EL2_FEAT_PAN2_MASK		ULL(0x30000)
#define HFGITR_EL2_FEAT_DPB2_MASK		ULL(0x200)
#define HFGITR_EL2_NON_FEAT_DEPENDENT_MASK	ULL(0x78fc03f000fdff)

#define HFGRTR_EL2_INIT_VAL			ULL(0xc4000000000000)
#define HFGRTR_EL2_FEAT_SME_MASK		ULL(0xc0000000000000)
#define HFGRTR_EL2_FEAT_LS64_ACCDATA_MASK	ULL(0x4000000000000)
#define HFGRTR_EL2_FEAT_RAS_MASK		ULL(0x27f0000000000)
#define HFGRTR_EL2_FEAT_RASV1P1_MASK		ULL(0x1800000000000)
#define HFGRTR_EL2_FEAT_GICV3_MASK		ULL(0x800000000)
#define HFGRTR_EL2_FEAT_CSV2_2_CSV2_1P2_MASK	ULL(0xc0000000)
#define HFGRTR_EL2_FEAT_LOR_MASK		ULL(0xf80000)
#define HFGRTR_EL2_FEAT_PAUTH_MASK		ULL(0x1f0)
#define HFGRTR_EL2_NON_FEAT_DEPENDENT_MASK	ULL(0x7f3f07fe0f)

#define HFGWTR_EL2_INIT_VAL			ULL(0xc4000000000000)
#define HFGWTR_EL2_FEAT_SME_MASK		ULL(0xc0000000000000)
#define HFGWTR_EL2_FEAT_LS64_ACCDATA_MASK	ULL(0x4000000000000)
#define HFGWTR_EL2_FEAT_RAS_MASK		ULL(0x23a0000000000)
#define HFGWTR_EL2_FEAT_RASV1P1_MASK		ULL(0x1800000000000)
#define HFGWTR_EL2_FEAT_GICV3_MASK		ULL(0x8000000000)
#define HFGWTR_EL2_FEAT_CSV2_2_CSV2_1P2_MASK	ULL(0xc0000000)
#define HFGWTR_EL2_FEAT_LOR_MASK		ULL(0xf80000)
#define HFGWTR_EL2_FEAT_PAUTH_MASK		ULL(0x1f0)
#define HFGWTR_EL2_NON_FEAT_DEPENDENT_MASK	ULL(0x7f2903380b)

/*******************************************************************************
 * Permission indirection and overlay Registers
 ******************************************************************************/
#define PIRE0_EL2		S3_4_C10_C2_2
#define PIR_EL2			S3_4_C10_C2_3
#define POR_EL2			S3_4_C10_C2_4
#define S2PIR_EL2		S3_4_C10_C2_5
#define PIRE0_EL1		S3_0_C10_C2_2
#define PIR_EL1			S3_0_C10_C2_3
#define POR_EL1			S3_0_C10_C2_4
#define S2POR_EL1		S3_0_C10_C2_5

/* Perm value encoding for S2POR_EL1 */
#define PERM_LABEL_NO_ACCESS		U(0)
#define PERM_LABEL_RESERVED_1		U(1)
#define PERM_LABEL_MRO			U(2)
#define PERM_LABEL_MRO_TL1		U(3)
#define PERM_LABEL_WO			U(4)
#define PERM_LABEL_RESERVED_5		U(5)
#define PERM_LABEL_MRO_TL0		U(6)
#define PERM_LABEL_MRO_TL01		U(7)
#define PERM_LABEL_RO			U(8)
#define PERM_LABEL_RO_uX		U(9)
#define PERM_LABEL_RO_pX		U(10)
#define PERM_LABEL_RO_upX		U(11)
#define PERM_LABEL_RW			U(12)
#define PERM_LABEL_RW_uX		U(13)
#define PERM_LABEL_RW_pX		U(14)
#define PERM_LABEL_RW_upX		U(15)

/*******************************************************************************
 * FEAT_GCS - Guarded Control Stack Registers
 ******************************************************************************/
#define GCSCR_EL2		S3_4_C2_C5_0
#define GCSPR_EL2		S3_4_C2_C5_1
#define GCSCR_EL1		S3_0_C2_C5_0
#define GCSCRE0_EL1		S3_0_C2_C5_2
#define GCSPR_EL1		S3_0_C2_C5_1
#define GCSPR_EL0		S3_3_C2_C5_1

/*******************************************************************************
 * Realm management extension register definitions
 ******************************************************************************/
#define SCXTNUM_EL2		S3_4_C13_C0_7
#define SCXTNUM_EL1		S3_0_C13_C0_7
#define SCXTNUM_EL0		S3_3_C13_C0_7

/*******************************************************************************
 * Floating Point Mode Register definitions
 ******************************************************************************/
#define FPMR			S3_3_C4_C4_2

#endif /* ARCH_H */
