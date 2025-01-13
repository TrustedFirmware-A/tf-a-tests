/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef HOST_REALM_MEM_LAYOUT_H
#define HOST_REALM_MEM_LAYOUT_H

#include <realm_def.h>

/*
 * Realm payload Memory Usage Layout in TFTF.bin.
 * The realm.bin is appended to tftf.bin to create a unified
 * tftf.bin.
 *   +---------------------------+
 *   | TFTF.bin                  |
 *   |                           |
 *   +---------------------------+
 *   | Realm Image               |
 *   | (REALM_MAX_LOAD_IMG_SIZE  |
 *   +---------------------------+
 *
 * +--------------------------------+     +---------------------------+
 * |  Memory Pool                   |     | Heap Memory               |
 * |                                |     |                           |
 * |  PAGE_POOL_MAX_SIZE            | ==> | PAGE_POOL_MAX_SIZE        |
 * |                                |     |                           |
 * +--------------------------------+     +---------------------------+*
 * Refer to tftf.lds for the layout.
 */

#if !(defined(__LINKER__) || defined(__ASSEMBLY__))
 /* Base address of each section */
 IMPORT_SYM(uintptr_t, __REALM_PAYLOAD_START__, REALM_IMAGE_BASE);
 IMPORT_SYM(uintptr_t, __REALM_POOL_START__, PAGE_POOL_BASE);
 IMPORT_SYM(uintptr_t, __REALM_POOL_END__, PAGE_POOL_END);
#endif

#ifdef ENABLE_REALM_PAYLOAD_TESTS
 /* 1MB for shared buffer between Realm and Host */
 #define NS_REALM_SHARED_MEM_SIZE	U(0x100000)
 /* 10MB of memory used as a pool for realm's objects creation */
 #define PAGE_POOL_MAX_SIZE		U(0xA00000)
#else
 #define NS_REALM_SHARED_MEM_SIZE       U(0x0)
 #define PAGE_POOL_MAX_SIZE             U(0x0)
#endif

#endif /* HOST_REALM_MEM_LAYOUT_H */
