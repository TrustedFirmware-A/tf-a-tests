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
		rdvl_vq = SVE_VL_TO_VQ(sve_rdvl_1());
		if (vl_bitmap & BIT_32(rdvl_vq)) {
			continue;
		}
		vl_bitmap |= BIT_32(rdvl_vq);
	}

	return vl_bitmap;
}

void sve_z_regs_write(const sve_z_regs_t *z_regs)
{
	assert(is_armv8_2_sve_present());

	__asm__ volatile(
		".arch_extension sve\n"
		fill_sve_helper(0)
		fill_sve_helper(1)
		fill_sve_helper(2)
		fill_sve_helper(3)
		fill_sve_helper(4)
		fill_sve_helper(5)
		fill_sve_helper(6)
		fill_sve_helper(7)
		fill_sve_helper(8)
		fill_sve_helper(9)
		fill_sve_helper(10)
		fill_sve_helper(11)
		fill_sve_helper(12)
		fill_sve_helper(13)
		fill_sve_helper(14)
		fill_sve_helper(15)
		fill_sve_helper(16)
		fill_sve_helper(17)
		fill_sve_helper(18)
		fill_sve_helper(19)
		fill_sve_helper(20)
		fill_sve_helper(21)
		fill_sve_helper(22)
		fill_sve_helper(23)
		fill_sve_helper(24)
		fill_sve_helper(25)
		fill_sve_helper(26)
		fill_sve_helper(27)
		fill_sve_helper(28)
		fill_sve_helper(29)
		fill_sve_helper(30)
		fill_sve_helper(31)
		".arch_extension nosve\n"
		: : "r" (z_regs));
}

void sve_z_regs_read(sve_z_regs_t *z_regs)
{
	assert(is_armv8_2_sve_present());

	__asm__ volatile(
		".arch_extension sve\n"
		read_sve_helper(0)
		read_sve_helper(1)
		read_sve_helper(2)
		read_sve_helper(3)
		read_sve_helper(4)
		read_sve_helper(5)
		read_sve_helper(6)
		read_sve_helper(7)
		read_sve_helper(8)
		read_sve_helper(9)
		read_sve_helper(10)
		read_sve_helper(11)
		read_sve_helper(12)
		read_sve_helper(13)
		read_sve_helper(14)
		read_sve_helper(15)
		read_sve_helper(16)
		read_sve_helper(17)
		read_sve_helper(18)
		read_sve_helper(19)
		read_sve_helper(20)
		read_sve_helper(21)
		read_sve_helper(22)
		read_sve_helper(23)
		read_sve_helper(24)
		read_sve_helper(25)
		read_sve_helper(26)
		read_sve_helper(27)
		read_sve_helper(28)
		read_sve_helper(29)
		read_sve_helper(30)
		read_sve_helper(31)
		".arch_extension nosve\n"
		: : "r" (z_regs));
}
