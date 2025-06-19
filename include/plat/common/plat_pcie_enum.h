/*
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PLAT_PCIE_ENUM_H__
#define __PLAT_PCIE_ENUM_H__

#include <stdio.h>
#include <stdint.h>

/* Header Offset and Type */
#define HEADER_OFFSET		0xC
#define TYPE0_HEADER		0
#define TYPE1_HEADER		1

#define TYPE01_RIDR		0x8

#define DEVICE_ID_OFFSET	16

/* Initial BUS definitions */
#define PRI_BUS			0
#define SEC_BUS			1
#define BUS_NUM_REG_OFFSET	0x18

/* BAR offset */
#define BAR0_OFFSET		0x10
#define TYPE1_BAR_MAX_OFF	0x14
#define TYPE0_BAR_MAX_OFF	0x24

#define BAR_NON_PRE_MEM		0
#define BAR_PRE_MEM		0x1

#define MEM_BASE32_LIM_MASK	0xFFF00000
#define MEM_BASE64_LIM_MASK	0xFFFFFFFFFFF00000
#define NON_PRE_FET_OFFSET	0x20
#define PRE_FET_OFFSET		0x24
#define BAR_INCREMENT		0x100000

#define PRI_BUS_CLEAR_MASK	0xFFFFFF00

#define TYPE0_MAX_BARS		6
#define TYPE1_MAX_BARS		2

/* BAR register masks */
#define BAR_MIT_MASK		0x1
#define BAR_MDT_MASK		0x3
#define BAR_MT_MASK		0x1
#define BAR_BASE_MASK		0xfffffff

/* BAR register shifts */
#define BAR_MIT_SHIFT		0
#define BAR_MDT_SHIFT		1
#define BAR_MT_SHIFT		3
#define BAR_BASE_SHIFT		4

/* TYPE 0/1 Cmn Cfg reg offsets and mask*/
#define TYPE01_CPR		0x34
#define TYPE01_CPR_MASK		0xff
#define COMMAND_REG_OFFSET	0x04
#define REG_ACC_DATA		0x7

#define BAR_MASK		0xFFFFFFF0

#define PCIE_HEADER_TYPE(header_value)		((header_value >> 16) & 0x3)
#define BUS_NUM_REG_CFG(sub_bus, sec_bus, pri_bus)	(sub_bus << 16 | sec_bus << 8 | bus)

#define BAR_REG(bar_reg_value)		((bar_reg_value >> 2) & 0x1)
#define BAR_MEM(bar_reg_value)		((bar_reg_value & 0xF) >> 3)
#define REG_MASK_SHIFT(bar_value)	((bar_value & MEM_BASE32_LIM_MASK) >> 16)

#endif /* __PLAT_PCIE_ENUM_H__ */
