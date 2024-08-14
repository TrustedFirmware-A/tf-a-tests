/*
 * Copyright (c) 2020-2023, Arm Limited. All rights reserved.
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

#define PLAT_SP_RX_BASE			ULL(0xfe300000)
#define PLAT_SP_CORE_COUNT		U(8)

#define PLAT_CACTUS_DEVICE_BASE		TC_DEVICE0_BASE
#define PLAT_CACTUS_DEVICE_SIZE		TC_DEVICE0_SIZE

/* Scratch memory used for SMMUv3 driver testing purposes in Cactus SP */
/* SMMUv3 tests are disabled for TC platform */
#define PLAT_CACTUS_MEMCPY_BASE		ULL(0xfe400000)
#define PLAT_CACTUS_NS_MEMCPY_BASE	ULL(0x90000000)
#define PLAT_CACTUS_MEMCPY_RANGE	ULL(0x8000)

/* Base address of user and PRIV frames in SMMUv3TestEngine */
/* SMMUv3 tests are disabled for TC platform */
#define USR_BASE_FRAME			ULL(0x0)
#define PRIV_BASE_FRAME			ULL(0x0)

/* Base address for memory sharing tests. */
#define CACTUS_SP1_MEM_SHARE_BASE 0xfe500000
#define CACTUS_SP2_MEM_SHARE_BASE 0xfe501000
#define CACTUS_SP3_MEM_SHARE_BASE 0xfe502000
#define CACTUS_SP3_NS_MEM_SHARE_BASE 0x880080001000ULL

#endif /* SP_PLATFORM_DEF_H */
