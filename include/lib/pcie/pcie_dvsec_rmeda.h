/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PCIE_DVSEC_RME_DA_H
#define PCIE_DVSEC_RME_DA_H

/* PCI RootPort Extended Capability RMEDA registers offset */

/*
 * Extended Capability Header
 * DVSEC Headers
 * RME-DA Control registers
 */
#define PCIE_ECAP_ECH_OFFSET			U(0)
#define PCIE_ECAP_DVSEC_HDR1_OFFSET		U(4)
#define PCIE_ECAP_DVSEC_HDR2_OFFSET		U(8)
#define PCIE_ECAP_DVSEC_RME_DA_CTL_REG1_OFFSET	U(12)
#define PCIE_ECAP_DVSEC_RME_DA_CTL_REG2_OFFSET	U(16)

/* RME-DA DVSEC Header1 */
#define DVSEC_HDR1_VENDOR_ID_SHIFT		U(0)
#define DVSEC_HDR1_VENDOR_ID_WIDTH		U(16)
#define DVSEC_HDR1_REVISION_SHIFT		U(16)
#define DVSEC_HDR1_REVISION_WIDTH		U(4)
#define DVSEC_HDR1_LENGTH_SHIFT			U(20)
#define DVSEC_HDR1_LENGTH_WIDTH			U(12)

/* RME-DA DVSEC Header1 - Values */
#define DVSEC_VENDOR_ID_ARM			U(0x13b5)
#define DVSEC_REVISION_0			U(0x0)

/* RME-DA DVSEC Header2 */
#define DVSEC_HDR2_DVSEC_ID_SHIFT		U(0)
#define DVSEC_HDR2_DVSEC_ID_WIDTH		U(16)

/* RME-DA DVSEC Header2 - Values */
#define DVSEC_ID_RME_DA				U(0xFF01)

/* RME-DA Control register 1 */
#define DVSEC_RMEDA_CTL_REG1_TDISP_EN_SHIFT	U(0)
#define DVSEC_RMEDA_CTL_REG1_TDISP_EN_WIDTH	U(1)

/* RME-DA Control register 1 - Values */
#define RME_DA_TDISP_DISABLE			U(0)
#define RME_DA_TDISP_ENABLE			U(1)

/* RME-DA Control register 2. 32 IDE Selective Stream Lock bits */
#define DVSEC_RME_DA_CTL_REG2_SEL_STR_LOCK_SHIFT	U(0)
#define DVSEC_RME_DA_CTL_REG2_SEL_STR_LOCK_WIDTH	U(32)

uint32_t pcie_find_rmeda_capability(uint32_t bdf, uint32_t *cid_offset);

#endif /* PCIE_DVSEC_RME_DA_H */
