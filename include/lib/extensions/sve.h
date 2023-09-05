/*
 * Copyright (c) 2021-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SVE_H
#define SVE_H

#include <arch.h>
#include <stdlib.h> /* for rand() */
#include <lib/extensions/sme.h>

#define fill_sve_helper(num) "ldr z"#num", [%0, #"#num", MUL VL];"
#define read_sve_helper(num) "str z"#num", [%0, #"#num", MUL VL];"

#define fill_sve_p_helper(num) "ldr p"#num", [%0, #"#num", MUL VL];"
#define read_sve_p_helper(num) "str p"#num", [%0, #"#num", MUL VL];"

/*
 * Max. vector length permitted by the architecture:
 * SVE:	 2048 bits = 256 bytes
 */
#define SVE_VECTOR_LEN_BYTES		(256U)
#define SVE_NUM_VECTORS			(32U)

/* Max size of one predicate register is 1/8 of Z register */
#define SVE_P_REG_LEN_BYTES		(SVE_VECTOR_LEN_BYTES / 8U)
#define SVE_NUM_P_REGS			(16U)

/* Max size of one FFR register is 1/8 of Z register */
#define SVE_FFR_REG_LEN_BYTES		(SVE_VECTOR_LEN_BYTES / 8U)
#define SVE_NUM_FFR_REGS		(1U)

#define SVE_VQ_ARCH_MIN			(0U)
#define SVE_VQ_ARCH_MAX			((1U << ZCR_EL2_SVE_VL_WIDTH) - 1U)

/* convert SVE VL in bytes to VQ */
#define SVE_VL_TO_VQ(vl_bytes)		(((vl_bytes) >> 4U) - 1U)

/* convert SVE VQ to bits */
#define SVE_VQ_TO_BITS(vq)		(((vq) + 1U) << 7U)

/* convert SVE VQ to bytes */
#define SVE_VQ_TO_BYTES(vq)		(SVE_VQ_TO_BITS(vq) / 8U)

/* get a random SVE VQ b/w 0 to SVE_VQ_ARCH_MAX */
#define SVE_GET_RANDOM_VQ		(rand() % (SVE_VQ_ARCH_MAX + 1U))

#ifndef __ASSEMBLY__

typedef uint8_t sve_z_regs_t[SVE_NUM_VECTORS * SVE_VECTOR_LEN_BYTES]
		__aligned(16);
typedef uint8_t sve_p_regs_t[SVE_NUM_P_REGS * SVE_P_REG_LEN_BYTES]
		__aligned(16);
typedef uint8_t sve_ffr_regs_t[SVE_NUM_FFR_REGS * SVE_FFR_REG_LEN_BYTES]
		__aligned(16);

uint64_t sve_rdvl_1(void);
void sve_config_vq(uint8_t sve_vq);
uint32_t sve_probe_vl(uint8_t sve_max_vq);
uint64_t sve_read_zcr_elx(void);
void sve_write_zcr_elx(uint64_t rval);

void sve_z_regs_write(const sve_z_regs_t *z_regs);
void sve_z_regs_write_rand(sve_z_regs_t *z_regs);
void sve_z_regs_read(sve_z_regs_t *z_regs);
uint64_t sve_z_regs_compare(const sve_z_regs_t *s1, const sve_z_regs_t *s2);

void sve_p_regs_write(const sve_p_regs_t *p_regs);
void sve_p_regs_write_rand(sve_p_regs_t *p_regs);
void sve_p_regs_read(sve_p_regs_t *p_regs);
uint64_t sve_p_regs_compare(const sve_p_regs_t *s1, const sve_p_regs_t *s2);

void sve_ffr_regs_write(const sve_ffr_regs_t *ffr_regs);
void sve_ffr_regs_write_rand(sve_ffr_regs_t *ffr_regs);
void sve_ffr_regs_read(sve_ffr_regs_t *ffr_regs);
uint64_t sve_ffr_regs_compare(const sve_ffr_regs_t *s1,
			      const sve_ffr_regs_t *s2);

/* Assembly routines */
bool sve_subtract_arrays_interleaved(int *dst_array, int *src_array1,
				     int *src_array2, int array_size,
				     bool (*world_switch_cb)(void));

void sve_subtract_arrays(int *dst_array, int *src_array1, int *src_array2,
			 int array_size);

#endif /* __ASSEMBLY__ */
#endif /* SVE_H */
