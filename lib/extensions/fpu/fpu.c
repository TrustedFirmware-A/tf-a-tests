/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <debug.h>
#include <lib/extensions/fpu.h>

#define __STR(x) #x
#define STR(x) __STR(x)

#define fill_simd_helper(num1, num2) "ldp q"#num1", q"#num2",\
	[%0], #"STR(2 * FPU_Q_SIZE)";"
#define read_simd_helper(num1, num2) "stp q"#num1", q"#num2",\
	[%0], #"STR(2 * FPU_Q_SIZE)";"

/* Read FPU Q[0-31] and strore it in 'q_regs' */
void fpu_q_regs_read(fpu_q_reg_t q_regs[FPU_Q_COUNT])
{
	__asm__ volatile(
			read_simd_helper(0, 1)
			read_simd_helper(2, 3)
			read_simd_helper(4, 5)
			read_simd_helper(6, 7)
			read_simd_helper(8, 9)
			read_simd_helper(10, 11)
			read_simd_helper(12, 13)
			read_simd_helper(14, 15)
			read_simd_helper(16, 17)
			read_simd_helper(18, 19)
			read_simd_helper(20, 21)
			read_simd_helper(22, 23)
			read_simd_helper(24, 25)
			read_simd_helper(26, 27)
			read_simd_helper(28, 29)
			read_simd_helper(30, 31)
			"sub %0, %0, #" STR(FPU_Q_COUNT * FPU_Q_SIZE) ";"
			: : "r" (q_regs));
}

/* Write FPU Q[0-31] registers passed in 'q_regs' */
static void fpu_q_regs_write(const fpu_q_reg_t q_regs[FPU_Q_COUNT])
{
	__asm__ volatile(
			fill_simd_helper(0, 1)
			fill_simd_helper(2, 3)
			fill_simd_helper(4, 5)
			fill_simd_helper(6, 7)
			fill_simd_helper(8, 9)
			fill_simd_helper(10, 11)
			fill_simd_helper(12, 13)
			fill_simd_helper(14, 15)
			fill_simd_helper(16, 17)
			fill_simd_helper(18, 19)
			fill_simd_helper(20, 21)
			fill_simd_helper(22, 23)
			fill_simd_helper(24, 25)
			fill_simd_helper(26, 27)
			fill_simd_helper(28, 29)
			fill_simd_helper(30, 31)
			"sub %0, %0, #" STR(FPU_Q_COUNT * FPU_Q_SIZE) ";"
			: : "r" (q_regs));
}

/* Read FPCR and FPSR and store it in 'cs_regs' */
void fpu_cs_regs_read(fpu_cs_regs_t *cs_regs)
{
	cs_regs->fpcr = read_fpcr();
	cs_regs->fpsr = read_fpsr();
}

/* Write FPCR and FPSR passed in 'cs_regs' */
void fpu_cs_regs_write(const fpu_cs_regs_t *cs_regs)
{
	write_fpcr(cs_regs->fpcr);
	write_fpsr(cs_regs->fpsr);
}

/*
 * Generate random values and write it to 'q_regs', then write it to FPU Q
 * registers.
 */
void fpu_q_regs_write_rand(fpu_q_reg_t q_regs[FPU_Q_COUNT])
{
	uint32_t rval;

	rval = rand();

	memset((void *)q_regs, 0, sizeof(fpu_q_reg_t) * FPU_Q_COUNT);
	for (unsigned int num = 0U; num < FPU_Q_COUNT; num++) {
		memset((uint8_t *)q_regs[num], rval * (num + 1),
		       sizeof(fpu_q_reg_t));
	}
	fpu_q_regs_write(q_regs);
}

/*
 * Generate random values and write it to 'cs_regs', then write it to FPU FPCR
 * and FPSR.
 */
void fpu_cs_regs_write_rand(fpu_cs_regs_t *cs_regs)
{
	memset((void *)cs_regs, 0, sizeof(fpu_cs_regs_t));

	cs_regs->fpcr = rand();
	cs_regs->fpsr = rand();

	/*
	 * Write random value to FPCR FPSR.
	 * Note write will be ignored for reserved bits.
	 */
	fpu_cs_regs_write(cs_regs);

	/* Read back current FPCR and FPSR */
	fpu_cs_regs_read(cs_regs);
}

/*
 * Generate random values and write it to 'fpu_state', then write it to FPU Q
 * registers, FPCR and FPSR.
 */
void fpu_state_write_rand(fpu_state_t *fpu_state)
{
	fpu_q_regs_write_rand(fpu_state->q_regs);
	fpu_cs_regs_write_rand(&fpu_state->cs_regs);
}

/* Read FPU Q registers, FPCR and FPSR write it to 'fpu_state' */
void fpu_state_read(fpu_state_t *fpu_state)
{
	fpu_q_regs_read(fpu_state->q_regs);
	fpu_cs_regs_read(&fpu_state->cs_regs);
}

/* Return zero if FPU Q registers 's1', 's2' matches else nonzero */
int fpu_q_regs_compare(const fpu_q_reg_t s1[FPU_Q_COUNT],
		       const fpu_q_reg_t s2[FPU_Q_COUNT])
{
	return memcmp(s1, s2, sizeof(fpu_q_reg_t) * FPU_Q_COUNT);
}

/*
 * Return zero if FPU control and status registers 's1', 's2' matches else
 * nonzero
 */
int fpu_cs_regs_compare(const fpu_cs_regs_t *s1, const fpu_cs_regs_t *s2)
{
	return memcmp(s1, s2, sizeof(fpu_cs_regs_t));
}

/* Returns 0, if FPU state 's1', 's2' matches else non-zero */
int fpu_state_compare(const fpu_state_t *s1, const fpu_state_t *s2)
{
	if (fpu_q_regs_compare(s1->q_regs, s2->q_regs) != 0) {
		return 1;
	}

	if (fpu_cs_regs_compare(&s1->cs_regs, &s2->cs_regs) != 0) {
		return 1;
	}

	return 0;
}
