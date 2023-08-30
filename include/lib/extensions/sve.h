/*
 * Copyright (c) 2021-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SVE_H
#define SVE_H

#include <arch.h>
#include <stdlib.h> /* for rand() */

#define fill_sve_helper(num) "ldr z"#num", [%0, #"#num", MUL VL];"
#define read_sve_helper(num) "str z"#num", [%0, #"#num", MUL VL];"

/*
 * Max. vector length permitted by the architecture:
 * SVE:	 2048 bits = 256 bytes
 */
#define SVE_VECTOR_LEN_BYTES		256
#define SVE_NUM_VECTORS			32

#define SVE_VQ_ARCH_MIN			(0U)
#define SVE_VQ_ARCH_MAX			((1 << ZCR_EL2_SVE_VL_WIDTH) - 1)

/* convert SVE VL in bytes to VQ */
#define SVE_VL_TO_VQ(vl_bytes)		(((vl_bytes) >> 4U) - 1)

/* convert SVE VQ to bits */
#define SVE_VQ_TO_BITS(vq)		(((vq) + 1U) << 7U)

/* convert SVE VQ to bytes */
#define SVE_VQ_TO_BYTES(vq)		(SVE_VQ_TO_BITS(vq) / 8)

/* get a random SVE VQ b/w 0 to SVE_VQ_ARCH_MAX */
#define SVE_GET_RANDOM_VQ		(rand() % (SVE_VQ_ARCH_MAX + 1))

#ifndef __ASSEMBLY__

typedef uint8_t sve_z_regs_t[SVE_NUM_VECTORS * SVE_VECTOR_LEN_BYTES]
		__aligned(16);

void sve_config_vq(uint8_t sve_vq);
uint32_t sve_probe_vl(uint8_t sve_max_vq);

void sve_z_regs_write(const sve_z_regs_t *z_regs);
void sve_z_regs_read(sve_z_regs_t *z_regs);

/* Assembly routines */
bool sve_subtract_arrays_interleaved(int *dst_array, int *src_array1,
				     int *src_array2, int array_size,
				     bool (*world_switch_cb)(void));

void sve_subtract_arrays(int *dst_array, int *src_array1, int *src_array2,
			 int array_size);

#ifdef __aarch64__

/* Returns the SVE implemented VL in bytes (constrained by ZCR_EL3.LEN) */
static inline uint64_t sve_rdvl_1(void)
{
	uint64_t vl;

	__asm__ volatile(
		".arch_extension sve\n"
		"rdvl %0, #1;"
		".arch_extension nosve\n"
		: "=r" (vl)
	);

	return vl;
}

#endif /* __aarch64__ */
#endif /* __ASSEMBLY__ */
#endif /* SVE_H */
