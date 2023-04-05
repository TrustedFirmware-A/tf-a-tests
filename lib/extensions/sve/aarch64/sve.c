/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_features.h>
#include <arch_helpers.h>
#include <assert.h>
#include <debug.h>
#include <lib/extensions/sve.h>

static inline uint64_t sve_read_zcr_elx(void)
{
	return IS_IN_EL2() ? read_zcr_el2() : read_zcr_el1();
}

static inline void sve_write_zcr_elx(uint64_t reg_val)
{
	if (IS_IN_EL2()) {
		write_zcr_el2(reg_val);
	} else {
		write_zcr_el1(reg_val);
	}
	isb();
}

static void _sve_config_vq(uint8_t sve_vq)
{
	u_register_t zcr_elx;

	zcr_elx = sve_read_zcr_elx();
	if (IS_IN_EL2()) {
		zcr_elx &= ~(MASK(ZCR_EL2_SVE_VL));
		zcr_elx |= INPLACE(ZCR_EL2_SVE_VL, sve_vq);
	} else {
		zcr_elx &= ~(MASK(ZCR_EL1_SVE_VL));
		zcr_elx |= INPLACE(ZCR_EL1_SVE_VL, sve_vq);
	}
	sve_write_zcr_elx(zcr_elx);
}

/* Set the SVE vector length in the current EL's ZCR_ELx register */
void sve_config_vq(uint8_t sve_vq)
{
	assert(is_armv8_2_sve_present());

	/* cap vq to arch supported max value */
	if (sve_vq > SVE_VQ_ARCH_MAX) {
		sve_vq = SVE_VQ_ARCH_MAX;
	}

	_sve_config_vq(sve_vq);
}

/*
 * Probes all valid vector length upto 'sve_max_vq'. Configures ZCR_ELx with 0
 * to 'sve_max_vq'. And for each step, call sve_rdvl to get the vector length.
 * Convert the vector length to VQ and set the bit corresponding to the VQ.
 * Returns:
 *	bitmap corresponding to each support VL
 */
uint32_t sve_probe_vl(uint8_t sve_max_vq)
{
	uint32_t vl_bitmap = 0;
	uint8_t vq, rdvl_vq;

	assert(is_armv8_2_sve_present());

	/* cap vq to arch supported max value */
	if (sve_max_vq > SVE_VQ_ARCH_MAX) {
		sve_max_vq = SVE_VQ_ARCH_MAX;
	}

	for (vq = 0; vq <= sve_max_vq; vq++) {
		_sve_config_vq(vq);
		rdvl_vq = SVE_VL_TO_VQ(sve_vector_length_get());
		if (vl_bitmap & BIT_32(rdvl_vq)) {
			continue;
		}
		vl_bitmap |= BIT_32(rdvl_vq);
	}

	return vl_bitmap;
}
