/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PCIE_H
#define PCIE_H

#include <cdefs.h>
#include <stdint.h>
#include <utils_def.h>

/* platforms need to ensure that number of entries is less that this value */
#define MAX_PCIE_INFO_ENTRIES 5

typedef struct {
	unsigned long ecam_base;	/* ECAM base address */
	unsigned int segment_num;	/* Segment number of this ECAM */
	unsigned int start_bus_num;	/* Start bus number for this ECAM space */
	unsigned int end_bus_num;	/* Last bus number */
} pcie_info_block_t;

struct pcie_info_table{
	unsigned int num_entries;	/* Number of entries */
	pcie_info_block_t block[MAX_PCIE_INFO_ENTRIES];
};

typedef struct {
	uint32_t bdf;
	uint32_t rp_bdf;
} pcie_device_attr_t;

typedef struct __packed {
	uint32_t num_entries;
	pcie_device_attr_t device[]; /* in the format of Segment/Bus/Dev/Func */
} pcie_device_bdf_table_t;

/* Address initialisation structure */
typedef struct {
	/* 64 bit prefetchable memory start address */
	uint64_t bar64_p_start;
	uint64_t rp_bar64_value;
	/* 32 bit non-prefetchable memory start address */
	uint32_t bar32_np_start;
	/* 32 bit prefetchable memory start address */
	uint32_t bar32_p_start;
	uint32_t rp_bar32_value;
} pcie_bar_init_t;

#define PCIE_EXTRACT_BDF_SEG(bdf)	((bdf >> 24) & 0xFF)
#define PCIE_EXTRACT_BDF_BUS(bdf)	((bdf >> 16) & 0xFF)
#define PCIE_EXTRACT_BDF_DEV(bdf)	((bdf >> 8) & 0xFF)
#define PCIE_EXTRACT_BDF_FUNC(bdf)	(bdf & 0xFF)

/* PCI-compatible region */
#define PCI_CMP_CFG_SIZE	256

/* PCI Express Extended Configuration Space */
#define PCIE_CFG_SIZE		4096

#define PCIE_MAX_BUS		256
#define PCIE_MAX_DEV		32
#define PCIE_MAX_FUNC		8

#define PCIE_CREATE_BDF(Seg, Bus, Dev, Func)	\
			((Seg << 24) | (Bus << 16) | (Dev << 8) | Func)

#define PCIE_SUCCESS		0x00000000  /* Operation completed successfully */
#define PCIE_NO_MAPPING		0x10000001  /* A mapping to a Function does not exist */
#define PCIE_CAP_NOT_FOUND	0x10000010  /* The specified capability was not found */
#define PCIE_UNKNOWN_RESPONSE	0xFFFFFFFF  /* Function not found or UR response from completer */

/* Allows storage of 2048 valid BDFs */
#define PCIE_DEVICE_BDF_TABLE_SZ	8192

typedef enum {
	HEADER = 0,
	PCIE_CAP = 1,
	PCIE_ECAP = 2
} bitfield_reg_type_t;

typedef enum {
	HW_INIT = 0,
	READ_ONLY = 1,
	STICKY_RO = 2,
	RSVDP_RO = 3,
	RSVDZ_RO = 4,
	READ_WRITE = 5,
	STICKY_RW = 6
} bitfield_attr_type_t;

/* Class Code Masks */
#define CC_SUB_MASK	0xFF	/* Sub Class */
#define CC_BASE_MASK	0xFF	/* Base Class */

/* Class Code Shifts */
#define CC_SHIFT	8
#define CC_SUB_SHIFT	16
#define CC_BASE_SHIFT	24

void pcie_create_info_table(void);
pcie_device_bdf_table_t *pcie_get_bdf_table(void);
uint32_t pcie_find_capability(uint32_t bdf, uint32_t cid_type, uint32_t cid,
				uint32_t *cid_offset);
uint32_t pcie_read_cfg(uint32_t bdf, uint32_t offset);
void pcie_write_cfg(uint32_t bdf, uint32_t offset, uint32_t data);

#endif /* PCIE_H */
