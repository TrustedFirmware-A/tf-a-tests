/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef XPM_NODEID_H_
#define XPM_NODEID_H_

/*
 * Device Nodes
 */
#define PM_DEV_RPU0_0           0x18110005U
#define PM_DEV_USB_0		0x18224018U
#define PM_DEV_RTC              0x18224034U
#define PM_DEV_GEM_0            0x18224019U
#define PM_DEV_QSPI		0x1822402BU
#define PM_DEV_SOC              0x18428044U

/*
 * Clock Nodes
 */
#define PM_CLK_RPU_PLL          0x8104003U
#define PM_CLK_QSPI_REF         0x8208039U
#define PM_CLK_GEM0_REF         0x8208058U

/*
 * MIO Nodes
 */
#define PM_STMIC_LMIO_0         0x14104001U

/*
 * Reset Nodes
 */
#define PM_RST_GEM_0            0xC104033U

#endif /* XPM_NODEID_H_ */
