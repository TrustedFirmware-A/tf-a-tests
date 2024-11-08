/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef PCIE_DOE_H
#define PCIE_DOE_H

#include <stdbool.h>
#include <stddef.h>
#include <pcie.h>
#include <test_helpers.h>

/* DOE Extended Capability */
#define DOE_CAP_ID			0x002E

#define DOE_CAP_REG			0x4
#define DOE_CTRL_REG			0x8
#define DOE_STATUS_REG			0xC
#define DOE_WRITE_DATA_MAILBOX_REG	0x10
#define DOE_READ_DATA_MAILBOX_REG	0x14

#define DOE_CTRL_ABORT_BIT		(1 << 0)
#define DOE_CTRL_GO_BIT			(1 << 31)

#define DOE_STATUS_BUSY_BIT		(1 << 0)
#define DOE_STATUS_ERROR_BIT		(1 << 2)
#define DOE_STATUS_READY_BIT		(1 << 31)

/* Time intervals in ms */
#define PCI_DOE_TIMEOUT			1000
#define PCI_DOE_POLL_TIME		10

#define PCI_DOE_POLL_LOOP		(PCI_DOE_TIMEOUT / PCI_DOE_POLL_TIME)

/* DOE Data Object Header 2 Reserved field [31:18] */
#define PCI_DOE_RESERVED_SHIFT		18
#define PCI_DOE_RESERVED_MASK		0xFFFC0000

/* Max data object length is 2^18 DW */
#define PCI_DOE_MAX_LENGTH		(1 << PCI_DOE_RESERVED_SHIFT)

/* SPDM GET_VERSION response DW length */
#define SPDM_GET_VERS_RESP_LEN						\
	((sizeof(spdm_version_response_t) +				\
	(sizeof(spdm_version_number_t) * SPDM_MAX_VERSION_COUNT) +	\
	(sizeof(uint32_t) - 1)) << 2)

/* PCI-SIG Vendor ID */
#define PSI_SIG_VENDOR_ID	1

/* Data Object Protocols */
#define DOE_DISC_PROTOCOL	0
#define CMA_SPDM_PROTOCOL	1
#define SEC_CMA_SPDM_PROTOCOL	2

#define DOE_HEADER(_type)	((_type << 16) | PSI_SIG_VENDOR_ID)

#define DOE_HEADER_0		DOE_HEADER(DOE_DISC_PROTOCOL)
#define DOE_HEADER_1		DOE_HEADER(CMA_SPDM_PROTOCOL)
#define DOE_HEADER_2		DOE_HEADER(SEC_CMA_SPDM_PROTOCOL)
#define DOE_HEADER_LENGTH	2

/*
 * SPDM VERSION structure:
 * bit[15:12] major_version
 * bit[11:8]  minor_version
 * bit[7:4]   update_version_number
 * bit[3:0]   alpha
 */
#define SPDM_VER_MAJOR_SHIFT	12
#define SPDM_VER_MAJOR_WIDTH	4
#define SPDM_VER_MINOR_SHIFT	8
#define SPDM_VER_MINOR_WIDTH	4
#define SPDM_VER_UPDATE_SHIFT	4
#define SPDM_VER_UPDATE_WIDTH	4
#define SPDM_VER_ALPHA_SHIFT	0
#define SPDM_VER_ALPHA_WIDTH	4

/* DOE Discovery */
typedef struct {
	uint8_t index;
	uint8_t reserved[3];
} pcie_doe_disc_req_t;

typedef struct {
	uint16_t vendor_id;
	uint8_t data_object_type;
	uint8_t next_index;
} pcie_doe_disc_resp_t;

/* Skip test if DA is not supported in RMI features */
#define CHECK_DA_SUPPORT_IN_RMI(_reg0)						\
	do {									\
		SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();			\
		/* Get feature register0 */					\
		if (host_rmi_features(0UL, &_reg0) != REALM_SUCCESS) {		\
			ERROR("Failed to get RMI feat_reg0\n");			\
			return TEST_RESULT_FAIL;				\
		}								\
										\
		/* DA not supported in RMI features? */				\
		if ((_reg0 & RMI_FEATURE_REGISTER_0_DA_EN) == 0UL) {		\
			WARN("DA not in RMI features, skipping\n");		\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (false)

#define SKIP_TEST_IF_DOE_NOT_SUPPORTED(_bdf, _doe_cap_base)			\
	do {									\
		/* Test PCIe DOE only for RME */				\
		if (get_armv9_2_feat_rme_support() == 0U) {			\
			tftf_testcase_printf("FEAT_RME not supported\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
										\
		pcie_init();							\
		if (pcie_find_doe_device(&(_bdf), &(_doe_cap_base)) != 0) {	\
			tftf_testcase_printf("PCIe DOE not supported\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (false)

void print_doe_disc(pcie_doe_disc_resp_t *data);
int pcie_doe_send_req(uint32_t header, uint32_t bdf, uint32_t doe_cap_base,
			uint32_t *req_addr, uint32_t req_len);
int pcie_doe_recv_resp(uint32_t bdf, uint32_t doe_cap_base,
			uint32_t *resp_addr, uint32_t *resp_len);
int pcie_doe_communicate(uint32_t header, uint32_t bdf, uint32_t doe_cap_base, void *req_buf,
			 size_t req_sz, void *rsp_buf, size_t *rsp_sz);
int pcie_find_doe_device(uint32_t *bdf_ptr, uint32_t *cap_base_ptr);

#endif /* PCIE_DOE_H */
