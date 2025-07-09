/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef PCIE_SPEC_H
#define PCIE_SPEC_H

/* Header Type */
#define TYPE0_HEADER		0
#define TYPE1_HEADER		1

/* TYPE 0/1 Cmn Cfg reg offsets */
#define TYPE01_VIDR		0x0
#define TYPE01_CR		0x4
#define TYPE01_RIDR		0x8
#define TYPE01_CLSR		0xc
#define TYPE01_BAR		0x10
#define TYPE01_CPR		0x34
#define TYPE01_ILR		0x3c

/* TYPE 0/1 Cmn Cfg reg shifts and masks */
#define TYPE01_VIDR_SHIFT	0
#define TYPE01_VIDR_MASK	0xffff
#define TYPE01_DIDR_SHIFT	16
#define TYPE01_DIDR_MASK	0xffff
#define TYPE01_CCR_SHIFT	8
#define TYPE01_CCR_MASK		0xffffff
#define TYPE01_CPR_SHIFT	0
#define TYPE01_CPR_MASK		0xff
#define TYPE01_HTR_SHIFT	16
#define TYPE01_HTR_MASK		0xff
#define TYPE01_IPR_SHIFT	8
#define TYPE01_IPR_MASK		0xFF
#define TYPE01_ILR_SHIFT	0
#define TYPE01_ILR_MASK		0xFF
#define TYPE01_BCC_SHIFT	24

#define HB_BASE_CLASS		0x06
#define HB_SUB_CLASS		0x00

/* Header type reg shifts and masks */
#define HTR_HL_SHIFT		0x0
#define HTR_HL_MASK		0x7f
#define HTR_MFD_SHIFT		7
#define HTR_MFD_MASK		0x1

/* BAR offset */
#define BAR0_OFFSET		0x10
#define BAR_TYPE_0_MAX_OFFSET	0x24
#define BAR_TYPE_1_MAX_OFFSET	0x14
#define BAR_NP_TYPE		0x0
#define BAR_P_TYPE		0x1
#define BAR_64_BIT		0x1
#define BAR_32_BIT		0x0
#define BAR_REG(bar_reg_value)	((bar_reg_value >> 2) & 0x1)

/* Type 1 Cfg reg offsets */
#define TYPE1_PBN		0x18
#define TYPE1_SEC_STA		0x1C
#define TYPE1_NP_MEM		0x20
#define TYPE1_P_MEM		0x24
#define TYPE1_P_MEM_BU		0x28	/* Prefetchable Base Upper Offset */
#define TYPE1_P_MEM_LU		0x2C	/* Prefetchable Limit Upper Offset */

/* Bus Number reg shifts */
#define SECBN_SHIFT		8
#define SUBBN_SHIFT		16

/* Bus Number reg masks */
#define PRIBN_MASK		0xff
#define SECBN_MASK		0xff
#define SUBBN_MASK		0xff
#define SECBN_EXTRACT		0xffff00ff

/* Capability header reg shifts */
#define PCIE_CIDR_SHIFT		0
#define PCIE_NCPR_SHIFT		8
#define PCIE_ECAP_CIDR_SHIFT	0
#define PCIE_ECAP_NCPR_SHIFT	20

/* Capability header reg masks */
#define PCIE_CIDR_MASK		0xff
#define PCIE_NCPR_MASK		0xff
#define PCIE_ECAP_CIDR_MASK	0xffff
#define PCIE_ECAP_NCPR_MASK	0xfff

/* PCIe Extended Capability Header */
#define PCIE_ECH_ID_SHIFT			U(0)
#define PCIE_ECH_ID_WIDTH			U(16)
#define PCIE_ECH_CAP_VER_SHIFT			U(16)
#define PCIE_ECH_CAP_VER_WIDTH			U(4)
#define PCIE_ECH_NEXT_CAP_OFFSET_SHIFT		U(20)
#define PCIE_ECH_NEXT_CAP_OFFSET_WIDTH		U(12)

#define PCIE_CAP_START		0x40
#define PCIE_CAP_END		0xFC
#define PCIE_ECAP_START		0x100
#define PCIE_ECAP_END		0xFFC

/* Capability Structure IDs */
#define CID_PCIECS		0x10
#define CID_MSI			0x05
#define CID_MSIX		0x11
#define CID_PMC			0x01
#define CID_EA			0x14
#define ECID_AER		0x0001
#define ECID_RCECEA		0x0007
#define ECID_ACS		0x000D
#define ECID_ARICS		0x000E
#define ECID_ATS		0x000F
#define ECID_PRI		0x0013
#define ECID_PASID		0x001B
#define ECID_DPC		0x001D
#define ECID_DVSEC		0x0023
#define ECID_DOE		0x002E
#define ECID_IDE		0x0030

/* PCI Express capability struct offsets */
#define CIDR_OFFSET		0x0
#define PCIECR_OFFSET		0x2
#define DCAPR_OFFSET		0x4
#define ACSCR_OFFSET		0x4
#define DCTLR_OFFSET		0x8
#define LCAPR_OFFSET		0xC
#define LCTRLR_OFFSET		0x10
#define DCAP2R_OFFSET		0x24
#define DCTL2R_OFFSET		0x28
#define DCTL2R_MASK		0xFFFF
#define LCAP2R_OFFSET		0x2C
#define LCTL2R_OFFSET		0x30
#define DCTL2R_MASK		0xFFFF
#define DSTS_SHIFT		16
#define DS_UNCORR_MASK		0x6
#define DS_CORR_MASK		0x1

/* PCIe capabilities reg shifts and masks */
#define PCIECR_DPT_SHIFT	4
#define PCIECR_DPT_MASK		0xf

/* Device bitmask definitions */
#define RCiEP			(1 << 0b1001)
#define RCEC			(1 << 0b1010)
#define EP			(1 << 0b0000)
#define RP			(1 << 0b0100)
#define UP			(1 << 0b0101)
#define DP			(1 << 0b0110)
#define iEP_EP			(1 << 0b1100)
#define iEP_RP			(1 << 0b1011)
#define PCI_PCIE		(1 << 0b1000)
#define PCIE_PCI		(1 << 0b0111)
#define PCIe_ALL		(iEP_RP | iEP_EP | RP | EP | RCEC | RCiEP)

#endif /* PCIE_SPEC_H */
