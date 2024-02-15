/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Definitions related to the Errata ABI feature
 * as per the SMC Calling Convention.
 */

#ifndef __ERRATA_ABI_H__
#define __ERRATA_ABI_H__

#ifndef __ASSEMBLY__
#include <stdbool.h>
#include <stdint.h>
#include <tftf_lib.h>
#include <platform_def.h>
#endif

/*******************************************************************************
 * Macro to create the array entry for EM_ABI_functions[]
 ******************************************************************************/
#define DEFINE_EM_FUNC(_func_id, _mandatory) \
	{  EM_##_func_id, _mandatory, "SMC_" # _func_id }

/*******************************************************************************
 * Errata ABI feature supported function ids
 ******************************************************************************/
#define EM_VERSION		0x840000F0
#define EM_FEATURES		0x840000F1
#define EM_CPU_ERRATUM_FEATURES	0x840000F2

/*
 * Number of EM ABI calls defined in the specification.
 */
#define TOTAL_ABI_CALLS		(3U)

#define ERRATA_COUNT		(32U)

typedef struct {
	uint32_t id;
	bool mandatory;
	const char *str;
} em_function_t;

typedef struct em_cpu_errata {
	int em_errata_id;
	unsigned int rxpx_low;
	unsigned int rxpx_high;
} em_cpu_errata_t;

typedef struct em_cpu{
	uint16_t cpu_pn;
	em_cpu_errata_t cpu_errata[ERRATA_COUNT];
}em_cpu_t;

extern const em_function_t em_functions[TOTAL_ABI_CALLS];
int32_t tftf_em_abi_version(void);
bool tftf_em_abi_feature_implemented(uint32_t id);
smc_ret_values tftf_em_abi_cpu_feature_implemented(uint32_t cpu_erratum, uint32_t forward_flag);


#define IN_RANGE(x, y, z)		(((x >= y) && (x <= z)) ? true : false)

/*******************************************************************************
 * Errata ABI Version
 ******************************************************************************/
#define EM_MAJOR_VER_SHIFT	(16)
#define EM_MAJOR_VER_MASK	(0xFFFF)
#define EM_MINOR_VER_SHIFT	(0)
#define EM_MINOR_VER_MASK	(0xFFFF)
#define EM_ABI_VERSION(major, minor)	(((major & EM_MAJOR_VER_MASK) << EM_MAJOR_VER_SHIFT) \
					| ((minor & EM_MINOR_VER_MASK) << EM_MINOR_VER_SHIFT))
/*******************************************************************************
 * Error codes
 ******************************************************************************/
#define EM_HIGHER_EL_MITIGATION		(3)
#define EM_NOT_AFFECTED			(2)
#define EM_AFFECTED			(1)
#define EM_SUCCESS			(0)
#define EM_NOT_SUPPORTED		(-1)
#define EM_INVALID_PARAMETERS		(-2)
#define EM_UNKNOWN_ERRATUM		(-3)
#endif /* __ERRATA_ABI_H__ */

