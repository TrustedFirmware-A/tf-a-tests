/*
 * Copyright (c) 2020-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ARCH_FEATURES_H
#define ARCH_FEATURES_H

#include <stdbool.h>
#include <stdint.h>

#include <arch_features.h>
#include <arch_helpers.h>
#include <utils_def.h>

static inline bool is_armv7_gentimer_present(void)
{
	/* The Generic Timer is always present in an ARMv8-A implementation */
	return true;
}

static inline bool is_armv8_1_pan_present(void)
{
	u_register_t id_aa64mmfr1_pan =
		EXTRACT(ID_AA64MMFR1_EL1_PAN, read_id_aa64mmfr1_el1());
	return (id_aa64mmfr1_pan >= ID_AA64MMFR1_EL1_PAN_SUPPORTED) &&
		(id_aa64mmfr1_pan <= ID_AA64MMFR1_EL1_PAN3_SUPPORTED);
}

static inline bool is_armv8_2_pan2_present(void)
{
	u_register_t id_aa64mmfr1_pan =
		EXTRACT(ID_AA64MMFR1_EL1_PAN, read_id_aa64mmfr1_el1());
	return (id_aa64mmfr1_pan >= ID_AA64MMFR1_EL1_PAN2_SUPPORTED) &&
		(id_aa64mmfr1_pan <= ID_AA64MMFR1_EL1_PAN3_SUPPORTED);
}

static inline bool is_armv8_2_sve_present(void)
{
	return ((read_id_aa64pfr0_el1() >> ID_AA64PFR0_SVE_SHIFT) &
		ID_AA64PFR0_SVE_MASK) == 1U;
}

static inline bool is_armv8_2_ttcnp_present(void)
{
	return ((read_id_aa64mmfr2_el1() >> ID_AA64MMFR2_EL1_CNP_SHIFT) &
		ID_AA64MMFR2_EL1_CNP_MASK) != 0U;
}

static inline bool is_feat_pacqarma3_present(void)
{
	uint64_t mask_id_aa64isar2 =
		(ID_AA64ISAR2_GPA3_MASK << ID_AA64ISAR2_GPA3_SHIFT) |
		(ID_AA64ISAR2_APA3_MASK << ID_AA64ISAR2_APA3_SHIFT);

	/* If any of the fields is not zero, QARMA3 algorithm is present */
	return (read_id_aa64isar2_el1() & mask_id_aa64isar2) != 0U;
}

static inline bool is_armv8_3_pauth_present(void)
{
	uint64_t mask_id_aa64isar1 =
		(ID_AA64ISAR1_GPI_MASK << ID_AA64ISAR1_GPI_SHIFT) |
		(ID_AA64ISAR1_GPA_MASK << ID_AA64ISAR1_GPA_SHIFT) |
		(ID_AA64ISAR1_API_MASK << ID_AA64ISAR1_API_SHIFT) |
		(ID_AA64ISAR1_APA_MASK << ID_AA64ISAR1_APA_SHIFT);

	/*
	 * If any of the fields is not zero or QARMA3 is present,
	 * PAuth is present.
	 */
	return ((read_id_aa64isar1_el1() & mask_id_aa64isar1) != 0U ||
		is_feat_pacqarma3_present());
}

static inline bool is_armv8_3_pauth_apa_api_apa3_present(void)
{
	uint64_t mask_id_aa64isar1 =
		(ID_AA64ISAR1_API_MASK << ID_AA64ISAR1_API_SHIFT) |
		(ID_AA64ISAR1_APA_MASK << ID_AA64ISAR1_APA_SHIFT);

	uint64_t mask_id_aa64isar2 =
		(ID_AA64ISAR2_APA3_MASK << ID_AA64ISAR2_APA3_SHIFT);

	return ((read_id_aa64isar1_el1() & mask_id_aa64isar1) |
		(read_id_aa64isar2_el1() & mask_id_aa64isar2)) != 0U;
}

static inline bool is_armv8_3_pauth_gpa_gpi_gpa3_present(void)
{
	uint64_t mask_id_aa64isar1 =
		(ID_AA64ISAR1_GPI_MASK << ID_AA64ISAR1_GPI_SHIFT) |
		(ID_AA64ISAR1_GPA_MASK << ID_AA64ISAR1_GPA_SHIFT);

	uint64_t mask_id_aa64isar2 =
		(ID_AA64ISAR2_GPA3_MASK << ID_AA64ISAR2_GPA3_SHIFT);

	return ((read_id_aa64isar1_el1() & mask_id_aa64isar1) |
		(read_id_aa64isar2_el1() & mask_id_aa64isar2)) != 0U;
}

static inline bool is_armv8_4_dit_present(void)
{
	return ((read_id_aa64pfr0_el1() >> ID_AA64PFR0_DIT_SHIFT) &
		ID_AA64PFR0_DIT_MASK) == 1U;
}

static inline bool is_armv8_4_ttst_present(void)
{
	return ((read_id_aa64mmfr2_el1() >> ID_AA64MMFR2_EL1_ST_SHIFT) &
		ID_AA64MMFR2_EL1_ST_MASK) == 1U;
}

static inline bool is_armv8_5_bti_present(void)
{
	return ((read_id_aa64pfr1_el1() >> ID_AA64PFR1_EL1_BT_SHIFT) &
		ID_AA64PFR1_EL1_BT_MASK) == BTI_IMPLEMENTED;
}

static inline unsigned int get_armv8_5_mte_support(void)
{
	return ((read_id_aa64pfr1_el1() >> ID_AA64PFR1_EL1_MTE_SHIFT) &
		ID_AA64PFR1_EL1_MTE_MASK);
}

static inline bool is_armv8_6_fgt_present(void)
{
	return ((read_id_aa64mmfr0_el1() >> ID_AA64MMFR0_EL1_FGT_SHIFT) &
		ID_AA64MMFR0_EL1_FGT_MASK) == ID_AA64MMFR0_EL1_FGT_SUPPORTED;
}

static inline unsigned long int get_armv8_6_ecv_support(void)
{
	return ((read_id_aa64mmfr0_el1() >> ID_AA64MMFR0_EL1_ECV_SHIFT) &
		ID_AA64MMFR0_EL1_ECV_MASK);
}

static inline unsigned long int get_pa_range(void)
{
	return ((read_id_aa64mmfr0_el1() >> ID_AA64MMFR0_EL1_PARANGE_SHIFT) &
		ID_AA64MMFR0_EL1_PARANGE_MASK);
}

static inline uint32_t arch_get_debug_version(void)
{
	return ((read_id_aa64dfr0_el1() & ID_AA64DFR0_DEBUG_BITS) >>
		ID_AA64DFR0_DEBUG_SHIFT);
}

static inline bool get_armv9_0_trbe_support(void)
{
	return ((read_id_aa64dfr0_el1() >> ID_AA64DFR0_TRACEBUFFER_SHIFT) &
		ID_AA64DFR0_TRACEBUFFER_MASK) ==
		ID_AA64DFR0_TRACEBUFFER_SUPPORTED;
}

static inline bool get_armv8_4_trf_support(void)
{
	return ((read_id_aa64dfr0_el1() >> ID_AA64DFR0_TRACEFILT_SHIFT) &
		ID_AA64DFR0_TRACEFILT_MASK) ==
		ID_AA64DFR0_TRACEFILT_SUPPORTED;
}

static inline bool get_armv8_0_sys_reg_trace_support(void)
{
	return ((read_id_aa64dfr0_el1() >> ID_AA64DFR0_TRACEVER_SHIFT) &
		ID_AA64DFR0_TRACEVER_MASK) ==
		ID_AA64DFR0_TRACEVER_SUPPORTED;
}

static inline unsigned int get_armv9_2_feat_rme_support(void)
{
	/*
	 * Return the RME version, zero if not supported.  This function can be
	 * used as both an integer value for the RME version or compared to zero
	 * to detect RME presence.
	 */
	return (unsigned int)(read_id_aa64pfr0_el1() >>
		ID_AA64PFR0_FEAT_RME_SHIFT) & ID_AA64PFR0_FEAT_RME_MASK;
}

static inline bool get_feat_hcx_support(void)
{
	return (((read_id_aa64mmfr1_el1() >> ID_AA64MMFR1_EL1_HCX_SHIFT) &
		ID_AA64MMFR1_EL1_HCX_MASK) == ID_AA64MMFR1_EL1_HCX_SUPPORTED);
}

static inline bool get_feat_afp_present(void)
{
	return (((read_id_aa64mmfr1_el1() >> ID_AA64MMFR1_EL1_AFP_SHIFT) &
		  ID_AA64MMFR1_EL1_AFP_MASK) == ID_AA64MMFR1_EL1_AFP_SUPPORTED);
}

static inline bool get_feat_brbe_support(void)
{
	return ((read_id_aa64dfr0_el1() >> ID_AA64DFR0_BRBE_SHIFT) &
		ID_AA64DFR0_BRBE_MASK) ==
		ID_AA64DFR0_BRBE_SUPPORTED;
}

static inline bool get_feat_wfxt_present(void)
{
	return (((read_id_aa64isar2_el1() >> ID_AA64ISAR2_WFXT_SHIFT) &
		ID_AA64ISAR2_WFXT_MASK) == ID_AA64ISAR2_WFXT_SUPPORTED);
}

static inline bool is_feat_rng_trap_present(void)
{
	return (((read_id_aa64pfr1_el1() >> ID_AA64PFR1_EL1_RNDR_TRAP_SHIFT) &
			ID_AA64PFR1_EL1_RNDR_TRAP_MASK)
			== ID_AA64PFR1_EL1_RNG_TRAP_SUPPORTED);
}

static inline bool is_feat_mpam_supported(void)
{
	/*
	 * If the MPAM version retreived from the Processor Feature registers
	 * is a non-zero value, then MPAM is supported.
	 */

	return (((((read_id_aa64pfr0_el1() >>
		ID_AA64PFR0_MPAM_SHIFT) & ID_AA64PFR0_MPAM_MASK) << 4) |
		((read_id_aa64pfr1_el1() >>
		ID_AA64PFR1_MPAM_FRAC_SHIFT) & ID_AA64PFR1_MPAM_FRAC_MASK)) != 0U);
}

static inline unsigned int spe_get_version(void)
{
	return (unsigned int)((read_id_aa64dfr0_el1() >> ID_AA64DFR0_PMS_SHIFT) &
		ID_AA64DFR0_PMS_MASK);
}

static inline bool get_feat_pmuv3_supported(void)
{
	return (((read_id_aa64dfr0_el1() >> ID_AA64DFR0_PMUVER_SHIFT) &
		ID_AA64DFR0_PMUVER_MASK) != ID_AA64DFR0_PMUVER_NOT_SUPPORTED);
}

static inline bool get_feat_hpmn0_supported(void)
{
	return (((read_id_aa64dfr0_el1() >> ID_AA64DFR0_HPMN0_SHIFT) &
		ID_AA64DFR0_HPMN0_MASK) == ID_AA64DFR0_HPMN0_SUPPORTED);
}

static inline bool is_feat_sme_supported(void)
{
	uint64_t features;

	features = read_id_aa64pfr1_el1() >> ID_AA64PFR1_EL1_SME_SHIFT;
	return (features & ID_AA64PFR1_EL1_SME_MASK) >= ID_AA64PFR1_EL1_SME_SUPPORTED;
}

static inline bool is_feat_sme_fa64_supported(void)
{
	uint64_t features;

	features = read_id_aa64smfr0_el1();
	return (features & ID_AA64SMFR0_EL1_FA64_BIT) != 0U;
}

static inline bool is_feat_sme2_supported(void)
{
	uint64_t features;

	features = read_id_aa64pfr1_el1() >> ID_AA64PFR1_EL1_SME_SHIFT;
	return (features & ID_AA64PFR1_EL1_SME_MASK) >= ID_AA64PFR1_EL1_SME2_SUPPORTED;
}

static inline u_register_t get_id_aa64mmfr0_el0_tgran4(void)
{
	return EXTRACT(ID_AA64MMFR0_EL1_TGRAN4, read_id_aa64mmfr0_el1());
}

static inline u_register_t get_id_aa64mmfr0_el0_tgran4_2(void)
{
	return EXTRACT(ID_AA64MMFR0_EL1_TGRAN4_2, read_id_aa64mmfr0_el1());
}

static inline u_register_t get_id_aa64mmfr0_el0_tgran16(void)
{
	return EXTRACT(ID_AA64MMFR0_EL1_TGRAN16, read_id_aa64mmfr0_el1());
}

static inline u_register_t get_id_aa64mmfr0_el0_tgran16_2(void)
{
	return EXTRACT(ID_AA64MMFR0_EL1_TGRAN16_2, read_id_aa64mmfr0_el1());
}

static inline u_register_t get_id_aa64mmfr0_el0_tgran64(void)
{
	return EXTRACT(ID_AA64MMFR0_EL1_TGRAN64, read_id_aa64mmfr0_el1());
}

static inline u_register_t get_id_aa64mmfr0_el0_tgran64_2(void)
{
	return EXTRACT(ID_AA64MMFR0_EL1_TGRAN64_2, read_id_aa64mmfr0_el1());
}

static inline bool is_feat_52b_on_4k_supported(void)
{
	return (get_id_aa64mmfr0_el0_tgran4() ==
				ID_AA64MMFR0_EL1_TGRAN4_52B_SUPPORTED);
}

static inline bool is_feat_52b_on_4k_2_supported(void)
{
	u_register_t tgran4_2 = get_id_aa64mmfr0_el0_tgran4_2();

	return ((tgran4_2 == ID_AA64MMFR0_EL1_TGRAN4_2_52B_SUPPORTED) ||
		((tgran4_2 == ID_AA64MMFR0_EL1_TGRAN4_2_AS_1)
			&& (is_feat_52b_on_4k_supported() == true)));
}

static inline bool is_feat_specres_present(void)
{
	return EXTRACT(ID_AA64ISAR1_SPECRES, read_id_aa64isar1_el1())
		== ID_AA64ISAR1_SPECRES_SUPPORTED;
}

static inline bool is_feat_tlbirange_present(void)
{
	return EXTRACT(ID_AA64ISAR0_TLB, read_id_aa64isar0_el1())
		== ID_AA64ISAR0_TLBIRANGE_SUPPORTED;
}

static inline bool is_feat_tlbios_present(void)
{
	return EXTRACT(ID_AA64ISAR0_TLB, read_id_aa64isar0_el1())
		!= ID_AA64ISAR0_TLB_NOT_SUPPORTED;
}

static inline bool is_feat_dpb_present(void)
{
	return EXTRACT(ID_AA64ISAR1_DPB, read_id_aa64isar1_el1())
		>= ID_AA64ISAR1_DPB_SUPPORTED;
}

static inline bool is_feat_dpb2_present(void)
{
	return EXTRACT(ID_AA64ISAR1_DPB, read_id_aa64isar1_el1())
		>= ID_AA64ISAR1_DPB2_SUPPORTED;
}

static inline bool is_feat_ls64_present(void)
{
	return EXTRACT(ID_AA64ISAR1_LS64, read_id_aa64isar1_el1())
		>= ID_AA64ISAR1_LS64_SUPPORTED;
}

static inline bool is_feat_ls64_v_present(void)
{
	return EXTRACT(ID_AA64ISAR1_LS64, read_id_aa64isar1_el1())
		>= ID_AA64ISAR1_LS64_V_SUPPORTED;
}

static inline bool is_feat_ls64_accdata_present(void)
{
	return EXTRACT(ID_AA64ISAR1_LS64, read_id_aa64isar1_el1())
		>= ID_AA64ISAR1_LS64_ACCDATA_SUPPORTED;
}

static inline bool is_feat_ras_present(void)
{
	return EXTRACT(ID_AA64PFR0_RAS, read_id_aa64pfr0_el1())
		== ID_AA64PFR0_RAS_SUPPORTED;
}

static inline bool is_feat_rasv1p1_present(void)
{
	return (EXTRACT(ID_AA64PFR0_RAS, read_id_aa64pfr0_el1())
		== ID_AA64PFR0_RASV1P1_SUPPORTED)
		|| (is_feat_ras_present() &&
			(EXTRACT(ID_AA64PFR1_RAS_FRAC, read_id_aa64pfr1_el1())
				== ID_AA64PFR1_RASV1P1_SUPPORTED))
		|| (EXTRACT(ID_PFR0_EL1_RAS, read_id_pfr0_el1())
			== ID_PFR0_EL1_RASV1P1_SUPPORTED)
		|| ((EXTRACT(ID_PFR0_EL1_RAS, read_id_pfr0_el1())
			== ID_PFR0_EL1_RAS_SUPPORTED) &&
			(EXTRACT(ID_PFR2_EL1_RAS_FRAC, read_id_pfr2_el1())
				== ID_PFR2_EL1_RASV1P1_SUPPORTED));
}

static inline bool is_feat_gicv3_gicv4_present(void)
{
	return EXTRACT(ID_AA64PFR0_GIC, read_id_aa64pfr0_el1())
		== ID_AA64PFR0_GICV3_GICV4_SUPPORTED;
}

static inline bool is_feat_csv2_present(void)
{
	return EXTRACT(ID_AA64PFR0_CSV2, read_id_aa64pfr0_el1())
		== ID_AA64PFR0_CSV2_SUPPORTED;
}

static inline bool is_feat_csv2_2_present(void)
{
	return EXTRACT(ID_AA64PFR0_CSV2, read_id_aa64pfr0_el1())
		== ID_AA64PFR0_CSV2_2_SUPPORTED;
}

static inline bool is_feat_csv2_1p1_present(void)
{
	return is_feat_csv2_present() &&
		(EXTRACT(ID_AA64PFR1_CSV2_FRAC, read_id_aa64pfr1_el1())
			== ID_AA64PFR1_CSV2_1P1_SUPPORTED);
}

static inline bool is_feat_csv2_1p2_present(void)
{
	return is_feat_csv2_present() &&
		(EXTRACT(ID_AA64PFR1_CSV2_FRAC, read_id_aa64pfr1_el1())
			== ID_AA64PFR1_CSV2_1P2_SUPPORTED);
}

static inline bool is_feat_lor_present(void)
{
	return EXTRACT(ID_AA64MMFR1_EL1_LO, read_id_aa64mmfr1_el1())
		!= ID_AA64MMFR1_EL1_LOR_NOT_SUPPORTED;
}

static inline unsigned int get_feat_ls64_support(void)
{
	return ((read_id_aa64isar1_el1() >> ID_AA64ISAR1_LS64_SHIFT) &
		ID_AA64ISAR1_LS64_MASK);
}

#endif /* ARCH_FEATURES_H */
