/*
 * Copyright (c) 2025-2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef XPM_NODEID_H_
#define XPM_NODEID_H_

/*
 * Device Nodes
 *
 * The primary APU/RPU core IDs differ between Versal and Versal Gen 2 and
 * live in the per-platform $(PLAT)/xpm_nodeid_plat.h headers (selected via
 * the portable PM_DEV_ACPU_CORE / PM_DEV_RPU_CORE aliases).
 */
#define PM_DEV_USB_0		0x18224018U
#define PM_DEV_TTC_0            0x18224024U
#define PM_DEV_GEM_0            0x18224019U
#define PM_DEV_QSPI		0x1822402BU
#define PM_DEV_RTC              0x18224034U
#define PM_DEV_SOC              0x18428044U
#define PM_DEV_PLD_0            0x18700000U

/*
 * Clock Nodes
 */
#define PM_CLK_PMC_PLL		0x8104001U
#define PM_CLK_APU_PLL          0x8104002U
#define PM_CLK_RPU_PLL          0x8104003U
#define PM_CLK_PMC_PRESRC       0x8208007U
#define PM_CLK_PMC_PLL_OUT      0x8208009U
#define PM_CLK_QSPI_REF         0x8208039U
#define PM_CLK_GEM0_REF         0x8208058U

/*
 * MIO Nodes
 */
#define PM_STMIC_LMIO_0         0x14104001U
#define PM_STMIC_LMIO_3         0x14104003U

/*
 * Reset Nodes
 */
#define PM_RST_GEM_0            0xC104033U

/*
 * Force-powerdown target subsystem and RPU wake-up address.
 *
 * PM_SUBSYS_RPU selects the RPU subsystem the APU asks PLM to power down /
 * wake up; RPU_WAKEUP_ADDR is the entry point the RPU resumes execution at
 * (the start of TCM).  0x1C000005 decodes to a CDO-defined subsystem id
 * (subsystem class, index 5), not a per-platform device id, so a single
 * shared definition is correct for both Versal and Versal Gen 2.
 */
#define PM_SUBSYS_RPU		0x1C000005U
#define RPU_WAKEUP_ADDR		0xFFE00000U

#endif /* XPM_NODEID_H_ */
