/*
 * Copyright (c) 2020-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * This file contains common defines for a secure partition. The correct
 * platform_def.h header file is selected according to the secure partition
 * and platform being built using the make scripts.
 */

#ifndef SP_PLATFORM_DEF_H
#define SP_PLATFORM_DEF_H

#include <platform_def.h>

#define PLAT_SP_RX_BASE			ULL(0x7300000)
#define PLAT_SP_CORE_COUNT		U(8)

/*
 * Map the device memory starting from UART2
 * so UART0 can be lent by tftf in the device memory sharing tests.
 */
#define PLAT_CACTUS_DEVICE_BASE		PL011_UART2_BASE
#define PLAT_CACTUS_DEVICE_SIZE		DEVICE0_SIZE - \
					(PLAT_CACTUS_DEVICE_BASE - DEVICE0_BASE)

/* Scratch memory used for SMMUv3 driver testing purposes in Cactus SP */
#define PLAT_CACTUS_MEMCPY_BASE			ULL(0x7400000)
#define PLAT_CACTUS_NS_MEMCPY_BASE		ULL(0x90000000)
#define PLAT_CACTUS_MEMCPY_RANGE		ULL(0x8000)

/* Base address of user and PRIV frames in SMMUv3TestEngine */
#define USR_BASE_FRAME			ULL(0x2B500000)
#define PRIV_BASE_FRAME			ULL(0x2B510000)

/* Base address for memory sharing tests. */
#define CACTUS_SP1_MEM_SHARE_BASE 0x7500000
#define CACTUS_SP2_MEM_SHARE_BASE 0x7501000
#define CACTUS_SP3_MEM_SHARE_BASE 0x7502000
#define CACTUS_SP3_NS_MEM_SHARE_BASE 0x880080001000ULL

#endif /* SP_PLATFORM_DEF_H */
