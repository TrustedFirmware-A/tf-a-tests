/*
 * Copyright (c) 2018, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __FIRMWARE_IMAGE_PACKAGE_H__
#define __FIRMWARE_IMAGE_PACKAGE_H__

#include <stdint.h>
#include <uuid.h>

/* This is used as a signature to validate the blob header */
#define TOC_HEADER_NAME	0xAA640001

/* ToC Entry UUIDs */
#define UUID_FIRMWARE_UPDATE_SCP_BL2U \
	{{0x65, 0x92, 0x27, 0x03}, {0x2f, 0x74}, {0xe6, 0x44}, 0x8d, 0xff, {0x57, 0x9a, 0xc1, 0xff, 0x06, 0x10} }
#define UUID_FIRMWARE_UPDATE_BL2U \
	{{0x60, 0xb3, 0xeb, 0x37}, {0xc1, 0xe5}, {0xea, 0x41}, 0x9d, 0xf3, {0x19, 0xed, 0xa1, 0x1f, 0x68, 0x01} }
#define UUID_FIRMWARE_UPDATE_NS_BL2U \
	{{0x4f, 0x51, 0x1d, 0x11}, {0x2b, 0xe5}, {0x4e, 0x49}, 0xb4, 0xc5, {0x83, 0xc2, 0xf7, 0x15, 0x84, 0x0a} }
#define UUID_FIRMWARE_UPDATE_FWU_CERT \
	{{0x71, 0x40, 0x8a, 0xb2}, {0x18, 0xd6}, {0x87, 0x4c}, 0x8b, 0x2e, {0xc6, 0xdc, 0xcd, 0x50, 0xf0, 0x96} }
#define UUID_TRUSTED_KEY_CERT \
	{{0x82,  0x7e, 0xe8, 0x90}, {0xf8, 0x60}, {0xe4, 0x11}, 0xa1, 0xb4, {0x77, 0x7a, 0x21, 0xb4, 0xf9, 0x4c} }

typedef struct fip_toc_header {
	uint32_t	name;
	uint32_t	serial_number;
	uint64_t	flags;
} fip_toc_header_t;

typedef struct fip_toc_entry {
	uuid_t		uuid;
	uint64_t	offset_address;
	uint64_t	size;
	uint64_t	flags;
} fip_toc_entry_t;

#endif /* __FIRMWARE_IMAGE_PACKAGE_H__ */
