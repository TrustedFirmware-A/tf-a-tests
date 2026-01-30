/*
 * Copyright (c) 2025-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_PCIE_ENUM_H
#define PLAT_PCIE_ENUM_H

#include <stdio.h>
#include <stdint.h>

/* Header Offset and Type */
#define HEADER_OFFSET		0xC

/* Initial BUS definitions */
#define BUS_NUM_REG_OFFSET	0x18

/* BAR offset */
#define TYPE1_BAR_MAX_OFF	0x14
#define TYPE0_BAR_MAX_OFF	0x24

#define BAR_MEM_SPACE		0
#define BAR_IO_SPACE		0x1

#define MEM_BASE32_LIM_MASK	0xFFF00000
#define MEM_BASE64_LIM_MASK	0xFFFFFFFFFFF00000
#define NON_PRE_FET_OFFSET	0x20
#define PRE_FET_OFFSET		0x24
#define BAR_INCREMENT		0x100000

#define PRI_BUS_CLEAR_MASK	0xFFFFFF00

/* TYPE 0/1 Cmn Cfg reg offsets and mask*/
#define COMMAND_REG_OFFSET	0x04
#define REG_ACC_DATA		0x7
#define SERR_ENABLE		0x10

#define BAR_MASK		0xFFFFFFF0

#define PCIE_HEADER_TYPE(header_value)		((header_value >> 16) & 0x3)
#define BUS_NUM_REG_CFG(sub_bus, sec_bus, pri_bus)	(sub_bus << 16 | sec_bus << 8 | bus)

#define REG_MASK_SHIFT(bar_value)	((bar_value & MEM_BASE32_LIM_MASK) >> 16)

#endif /* PLAT_PCIE_ENUM_H */
