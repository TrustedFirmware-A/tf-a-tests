/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
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

static fpu_reg_state_t g_fpu_read;

static void read_fpu_state_registers(fpu_reg_state_t *fpu_template_in)
{
#ifdef __aarch64__

	u_register_t fpsr;
	u_register_t fpcr;

	/* Read current FPCR FPSR and write to template. */
	__asm__ volatile ("mrs %0, fpsr\n" : "=r" (fpsr));
	__asm__ volatile ("mrs %0, fpcr\n" : "=r" (fpcr));
	fpu_template_in->fpsr = fpsr;
	fpu_template_in->fpcr = fpcr;

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
			: : "r" (fpu_template_in->q));
#endif
}

void fpu_state_fill_regs_and_template(fpu_reg_state_t *fpu_template_in)
{
	u_register_t fpsr;
	u_register_t fpcr;
	u_register_t temp;

	temp = rand();
	(void)memset((void *)fpu_template_in, 0, sizeof(fpu_reg_state_t));

	/*
	 * Write random value to FPCR FPSR.
	 * Note write will be ignored for reserved bits.
	 */
	__asm__ volatile ("mrs %0, fpsr\n" : "=r" (temp));
	__asm__ volatile ("mrs %0, fpcr\n" : "=r" (temp));

	/*
	 * Read back current FPCR FPSR and write to template,
	 */
	__asm__ volatile ("mrs %0, fpsr\n" : "=r" (fpsr));
	__asm__ volatile ("mrs %0, fpcr\n" : "=r" (fpcr));
	fpu_template_in->fpsr = fpsr;
	fpu_template_in->fpcr = fpcr;

	for (unsigned int num = 0U; num < FPU_Q_COUNT; num++) {
		memset((uint8_t *)fpu_template_in->q[num], temp * num,
				sizeof(fpu_template_in->q[0]));
	}
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
			: : "r" (fpu_template_in->q));
}

void fpu_state_print(fpu_reg_state_t *vec)
{
	INFO("dumping FPU registers :\n");
	for (unsigned int num = 0U; num < FPU_Q_COUNT; num++) {
		INFO("Q[%u]=0x%llx%llx\n", num, (uint64_t)vec->q[num * FPU_Q_SIZE],
				(uint64_t)(vec->q[num * FPU_Q_SIZE + 1]));
	}
	INFO("FPCR=0x%lx FPSR=0x%lx\n", vec->fpcr, vec->fpsr);
}

bool fpu_state_compare_template(fpu_reg_state_t *fpu_template_in)
{
	(void)memset((void *)&g_fpu_read, 0, sizeof(fpu_reg_state_t));
	read_fpu_state_registers(&g_fpu_read);

	if (memcmp((uint8_t *)fpu_template_in,
			(uint8_t *)&g_fpu_read,
			sizeof(fpu_reg_state_t)) != 0U) {
		ERROR("%s failed\n", __func__);
		ERROR("Read values\n");
		fpu_state_print(&g_fpu_read);
		ERROR("Template values\n");
		fpu_state_print(fpu_template_in);
		return false;
	} else {
		return true;
	}
}
