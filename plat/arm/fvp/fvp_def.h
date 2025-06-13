/*
 * Copyright (c) 2018-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*******************************************************************************
 * FVP specific definitions. Used only by FVP specific code.
 ******************************************************************************/

#ifndef __FVP_DEF_H__
#define __FVP_DEF_H__

#include <common_def.h>
#include <platform_def.h>

/*******************************************************************************
 * Cluster Topology definitions
 ******************************************************************************/
#ifndef FVP_CLUSTER_COUNT
#error "FVP_CLUSTER_COUNT is not set in makefile"
#endif

#ifndef FVP_MAX_CPUS_PER_CLUSTER
#error "FVP_MAX_CPUS_PER_CLUSTER is not set in makefile"
#endif

/*******************************************************************************
 * FVP memory map related constants
 ******************************************************************************/

#define DEVICE0_BASE			0x1a000000
#define DEVICE0_SIZE			0x12200000

#define DEVICE1_BASE			0x2f000000
#define DEVICE1_SIZE			0x400000

/**
 * NOTE: LS64_ATOMIC_DEVICE Memory Region (0x1d000000 - 0x1d00ffff) has been
 * configured within the FVP to support only st64b/ld64b instructions.
 * ldr/str instructions cannot be used to access this memory.
 */
#define LS64_ATOMIC_DEVICE_BASE		0x1d000000
#define LS64_ATOMIC_DEVICE_SIZE		0x10000

/*
 * Base Platform RevC only
 *
 * PCIe config region
 */
#define PCIE_CONFIG_BASE		0x40000000
#define PCIE_CONFIG_SIZE		SZ_256M

/* PCIe memory region #1 256MB */
#define PCIE_MEM_1_BASE			0x50000000
#define PCIE_MEM_1_SIZE			SZ_256M

/*
 * PCIe memory region #2 size is 256GB, but only 3GB
 * are currently supported by GPT library
 */
#define PCIE_MEM_2_BASE			0x4000000000
#define PCIE_MEM_2_SIZE			(SZ_1G * 3U)

/*******************************************************************************
 * GIC-400 & interrupt handling related constants
 ******************************************************************************/
/* Base FVP compatible GIC memory map */
#define GICD_BASE		0x2f000000
#define GICR_BASE		0x2f100000
#define GICC_BASE		0x2c000000

/*******************************************************************************
 * PL011 related constants
 ******************************************************************************/
#define PL011_UART0_BASE	0x1c090000
#define PL011_UART1_BASE	0x1c0a0000
#define PL011_UART2_BASE	0x1c0b0000
#define PL011_UART3_BASE	0x1c0c0000

#define PL011_UART0_CLK_IN_HZ	24000000
#define PL011_UART1_CLK_IN_HZ	24000000
#define PL011_UART2_CLK_IN_HZ	24000000
#define PL011_UART3_CLK_IN_HZ	24000000

#define PLAT_ARM_UART_BASE		PL011_UART0_BASE
#define PLAT_ARM_UART_CLK_IN_HZ		PL011_UART0_CLK_IN_HZ

#endif /* __FVP_DEF_H__ */
