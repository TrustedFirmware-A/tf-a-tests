/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FPU_H
#define FPU_H

/* The FPU and SIMD register bank is 32 quadword (128 bits) Q registers. */
#define FPU_Q_SIZE		16U
#define FPU_Q_COUNT		32U

#ifndef __ASSEMBLER__

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t fpu_q_reg_t[FPU_Q_SIZE] __aligned(16);
typedef struct fpu_cs_regs {
	unsigned long fpcr;
	unsigned long fpsr;
} fpu_cs_regs_t __aligned(16);

typedef struct fpu_state {
	fpu_q_reg_t q_regs[FPU_Q_COUNT];
	fpu_cs_regs_t cs_regs;
} fpu_state_t __aligned(16);

void fpu_cs_regs_write(const fpu_cs_regs_t *cs_regs);
void fpu_cs_regs_write_rand(fpu_cs_regs_t *cs_regs);
void fpu_cs_regs_read(fpu_cs_regs_t *cs_regs);
int fpu_cs_regs_compare(const fpu_cs_regs_t *s1, const fpu_cs_regs_t *s2);

void fpu_q_regs_write_rand(fpu_q_reg_t q_regs[FPU_Q_COUNT]);
void fpu_q_regs_read(fpu_q_reg_t q_regs[FPU_Q_COUNT]);
int fpu_q_regs_compare(const fpu_q_reg_t s1[FPU_Q_COUNT],
		       const fpu_q_reg_t s2[FPU_Q_COUNT]);

void fpu_state_write_rand(fpu_state_t *fpu_state);
void fpu_state_read(fpu_state_t *fpu_state);
int fpu_state_compare(const fpu_state_t *s1, const fpu_state_t *s2);

#endif /* __ASSEMBLER__ */
#endif /* FPU_H */
