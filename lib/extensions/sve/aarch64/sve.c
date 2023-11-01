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

/*
 * Write SVE Z[0-31] registers passed in 'z_regs' for Normal SVE or Streaming
 * SVE mode
 */
void sve_z_regs_write(const sve_z_regs_t *z_regs)
{
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

/*
 * Read SVE Z[0-31] and store it in 'zregs' for Normal SVE or Streaming SVE mode
 */
void sve_z_regs_read(sve_z_regs_t *z_regs)
{
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

/*
 * Write SVE P[0-15] registers passed in 'p_regs' for Normal SVE or Streaming
 * SVE mode
 */
void sve_p_regs_write(const sve_p_regs_t *p_regs)
{
	__asm__ volatile(
		".arch_extension sve\n"
		fill_sve_p_helper(0)
		fill_sve_p_helper(1)
		fill_sve_p_helper(2)
		fill_sve_p_helper(3)
		fill_sve_p_helper(4)
		fill_sve_p_helper(5)
		fill_sve_p_helper(6)
		fill_sve_p_helper(7)
		fill_sve_p_helper(8)
		fill_sve_p_helper(9)
		fill_sve_p_helper(10)
		fill_sve_p_helper(11)
		fill_sve_p_helper(12)
		fill_sve_p_helper(13)
		fill_sve_p_helper(14)
		fill_sve_p_helper(15)
		".arch_extension nosve\n"
		: : "r" (p_regs));
}

/*
 * Read SVE P[0-15] registers and store it in 'p_regs' for Normal SVE or
 * Streaming SVE mode
 */
void sve_p_regs_read(sve_p_regs_t *p_regs)
{
	__asm__ volatile(
		".arch_extension sve\n"
		read_sve_p_helper(0)
		read_sve_p_helper(1)
		read_sve_p_helper(2)
		read_sve_p_helper(3)
		read_sve_p_helper(4)
		read_sve_p_helper(5)
		read_sve_p_helper(6)
		read_sve_p_helper(7)
		read_sve_p_helper(8)
		read_sve_p_helper(9)
		read_sve_p_helper(10)
		read_sve_p_helper(11)
		read_sve_p_helper(12)
		read_sve_p_helper(13)
		read_sve_p_helper(14)
		read_sve_p_helper(15)
		".arch_extension nosve\n"
		: : "r" (p_regs));
}

/*
 * Write SVE FFR registers passed in 'ffr_regs' for Normal SVE or Streaming SVE
 * mode
 */
void sve_ffr_regs_write(const sve_ffr_regs_t *ffr_regs)
{
	uint8_t sve_p_reg[SVE_P_REG_LEN_BYTES];

	/* Save p0. Load 'ffr_regs' to p0 and write FFR. Restore p0 */
	__asm__ volatile(
		".arch_extension sve\n"
		"	str	p0, [%1]\n"
		"	ldr	p0, [%0]\n"
		"	wrffr	p0.B\n"
		"	ldr	p0, [%1]\n"
		".arch_extension nosve\n"
		:
		: "r" (ffr_regs), "r" (sve_p_reg)
		: "memory");
}

/*
 * Read SVE FFR registers and store it in 'ffr_regs' for Normal SVE or Streaming
 * SVE mode
 */
void sve_ffr_regs_read(sve_ffr_regs_t *ffr_regs)
{
	uint8_t sve_p_reg[SVE_P_REG_LEN_BYTES];

	/* Save p0. Read FFR to p0 and save p0 (ffr) to 'ffr_regs'. Restore p0 */
	__asm__ volatile(
		".arch_extension sve\n"
		"	str	p0, [%1]\n"
		"	rdffr	p0.B\n"
		"	str	p0, [%0]\n"
		"	ldr	p0, [%1]\n"
		".arch_extension nosve\n"
		:
		: "r" (ffr_regs), "r" (sve_p_reg)
		: "memory");
}

/*
 * Generate random values and write it to 'z_regs', then write it to SVE Z
 * registers for Normal SVE or Streaming SVE mode.
 */
void sve_z_regs_write_rand(sve_z_regs_t *z_regs)
{
	uint32_t rval;
	uint32_t z_size;
	uint8_t *z_reg;

	z_size = (uint32_t)sve_rdvl_1();

	/* Write Z regs */
	rval = rand();
	memset((void *)z_regs, 0, sizeof(sve_z_regs_t));
	for (uint32_t i = 0U; i < SVE_NUM_VECTORS; i++) {
		z_reg = (uint8_t *)z_regs + (i * z_size);

		memset((void *)z_reg, rval * (i + 1), z_size);
	}
	sve_z_regs_write(z_regs);
}

/*
 * Generate random values and write it to 'p_regs', then write it to SVE P
 * registers for Normal SVE or Streaming SVE mode.
 */
void sve_p_regs_write_rand(sve_p_regs_t *p_regs)
{
	uint32_t p_size;
	uint8_t *p_reg;
	uint32_t rval;

	p_size = (uint32_t)sve_rdvl_1() / 8;

	/* Write P regs */
	rval = rand();
	memset((void *)p_regs, 0, sizeof(sve_p_regs_t));
	for (uint32_t i = 0U; i < SVE_NUM_P_REGS; i++) {
		p_reg = (uint8_t *)p_regs + (i * p_size);

		memset((void *)p_reg, rval * (i + 1), p_size);
	}
	sve_p_regs_write(p_regs);
}

/*
 * Generate random values and write it to 'ffr_regs', then write it to SVE FFR
 * registers for Normal SVE or Streaming SVE mode.
 */
void sve_ffr_regs_write_rand(sve_ffr_regs_t *ffr_regs)
{
	uint32_t ffr_size;
	uint8_t *ffr_reg;
	uint32_t rval;

	ffr_size = (uint32_t)sve_rdvl_1() / 8;

	rval = rand();
	memset((void *)ffr_regs, 0, sizeof(sve_ffr_regs_t));
	for (uint32_t i = 0U; i < SVE_NUM_FFR_REGS; i++) {
		ffr_reg = (uint8_t *)ffr_regs + (i * ffr_size);

		memset((void *)ffr_reg, rval * (i + 1), ffr_size);
	}
	sve_ffr_regs_write(ffr_regs);
}

/*
 * Compare Z registers passed in 's1' (old values) with 's2' (new values).
 * This routine works for Normal SVE or Streaming SVE mode.
 *
 * Returns:
 * 0		: All Z[0-31] registers in 's1' and 's2' are equal
 * nonzero	: Sets the Nth bit of the Z register that is not equal
 */
uint64_t sve_z_regs_compare(const sve_z_regs_t *s1, const sve_z_regs_t *s2)
{
	uint32_t z_size;
	uint64_t cmp_bitmap = 0UL;

	/*
	 * 'rdvl' returns Streaming SVE VL if PSTATE.SM=1 else returns normal
	 * SVE VL
	 */
	z_size = (uint32_t)sve_rdvl_1();

	for (uint32_t i = 0U; i < SVE_NUM_VECTORS; i++) {
		uint8_t *s1_z = (uint8_t *)s1 + (i * z_size);
		uint8_t *s2_z = (uint8_t *)s2 + (i * z_size);

		if ((memcmp(s1_z, s2_z, z_size) == 0)) {
			continue;
		}

		cmp_bitmap |= BIT_64(i);
		VERBOSE("SVE Z_%u mismatch\n", i);
	}

	return cmp_bitmap;
}

/*
 * Compare P registers passed in 's1' (old values) with 's2' (new values).
 * This routine works for Normal SVE or Streaming SVE mode.
 *
 * Returns:
 * 0		: All P[0-15] registers in 's1' and 's2' are equal
 * nonzero	: Sets the Nth bit of the P register that is not equal
 */
uint64_t sve_p_regs_compare(const sve_p_regs_t *s1, const sve_p_regs_t *s2)
{
	uint32_t p_size;
	uint64_t cmp_bitmap = 0UL;

	/* Size of one predicate register 1/8 of Z register */
	p_size = (uint32_t)sve_rdvl_1() / 8U;

	for (uint32_t i = 0U; i < SVE_NUM_P_REGS; i++) {
		uint8_t *s1_p = (uint8_t *)s1 + (i * p_size);
		uint8_t *s2_p = (uint8_t *)s2 + (i * p_size);

		if ((memcmp(s1_p, s2_p, p_size) == 0)) {
			continue;
		}

		cmp_bitmap |= BIT_64(i);
		VERBOSE("SVE P_%u mismatch\n", i);
	}

	return cmp_bitmap;
}

/*
 * Compare FFR register passed in 's1' (old values) with 's2' (new values).
 * This routine works for Normal SVE or Streaming SVE mode.
 *
 * Returns:
 * 0		: FFR register in 's1' and 's2' are equal
 * nonzero	: FFR register is not equal
 */
uint64_t sve_ffr_regs_compare(const sve_ffr_regs_t *s1, const sve_ffr_regs_t *s2)
{
	uint32_t ffr_size;
	uint64_t cmp_bitmap = 0UL;

	/* Size of one FFR register 1/8 of Z register */
	ffr_size = (uint32_t)sve_rdvl_1() / 8U;

	for (uint32_t i = 0U; i < SVE_NUM_FFR_REGS; i++) {
		uint8_t *s1_ffr = (uint8_t *)s1 + (i * ffr_size);
		uint8_t *s2_ffr = (uint8_t *)s2 + (i * ffr_size);

		if ((memcmp(s1_ffr, s2_ffr, ffr_size) == 0)) {
			continue;
		}

		cmp_bitmap |= BIT_64(i);
		VERBOSE("SVE FFR_%u mismatch:\n", i);
	}

	return cmp_bitmap;
}
