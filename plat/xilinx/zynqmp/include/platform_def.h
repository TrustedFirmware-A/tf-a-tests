/*
 * Copyright (c) 2022-2024, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <utils_def.h>

#ifndef __PLATFORM_DEF_H__
#define __PLATFORM_DEF_H__

#define PLATFORM_LINKER_FORMAT		"elf64-littleaarch64"
#define PLATFORM_LINKER_ARCH		aarch64

#define TFTF_BASE			U(0x8000000)

#define DRAM_BASE			0x0
#define DRAM_SIZE			0x80000000

#define PLATFORM_CLUSTER_COUNT		U(1)
#define PLATFORM_CORE_COUNT_PER_CLUSTER	4
#define PLATFORM_CORE_COUNT		(PLATFORM_CLUSTER_COUNT * \
					 PLATFORM_CORE_COUNT_PER_CLUSTER)
#define PLATFORM_CORES_PER_CLUSTER	PLATFORM_CORE_COUNT_PER_CLUSTER

#define PLATFORM_NUM_AFFS		(PLATFORM_CORE_COUNT + \
					PLATFORM_CLUSTER_COUNT + 1)

#define PLATFORM_MAX_AFFLVL		MPIDR_AFFLVL2
#define PLAT_MAX_PWR_LEVEL		MPIDR_AFFLVL2

#define PLAT_MAX_PWR_STATES_PER_LVL	2

#define PLATFORM_STACK_SIZE		0x440
#define PCPU_DV_MEM_STACK_SIZE		0x100

#define TFTF_NVM_SIZE			0x600000
#define TFTF_NVM_OFFSET			0x20000000

/* total number of system nodes implemented by the platform */
#define PLATFORM_SYSTEM_COUNT		U(1)

/* UG1085 - system interrupts table */
#define PLAT_MAX_SPI_OFFSET_ID		229

/* Local state bit width for each level in the state-ID field of power state */
#define PLAT_LOCAL_PSTATE_WIDTH		4

#define PLAT_MAX_PWR_STATES_PER_LVL	2

#define IRQ_PCPU_NS_TIMER		51

#define IRQ_CNTPSIRQ1			80

#define PLAT_SUSPEND_ENTRY_TIME		15
#define PLAT_SUSPEND_ENTRY_EXIT_TIME	30

#define IRQ_PCPU_HP_TIMER		26

#define ZYNQMP_UART0_BASE		0xFF000000
#define ZYNQMP_UART1_BASE		0xFF010000

#define ZYNQMP_UART_BASE		ZYNQMP_UART0_BASE
#define CRASH_CONSOLE_SIZE		0x1000

#define ZYNQMP_CRASH_UART_CLK_IN_HZ	100000000
#define ZYNQMP_UART_BAUDRATE		115200

#define CACHE_WRITEBACK_SHIFT		6
#define CACHE_WRITEBACK_GRANULE		(1 << CACHE_WRITEBACK_SHIFT)

/* Platform specific page table and MMU setup constants */

#define PLAT_PHY_ADDR_SPACE_SIZE	(ULL(1) << 32)
#define PLAT_VIRT_ADDR_SPACE_SIZE	(ULL(1) << 32)

/* Translation table constants */
#define MAX_XLAT_TABLES			8
#define MAX_MMAP_REGIONS		16

/* ZYNQMP memory map related constants */

/* Aggregate of all devices in the first GB */
#define DEVICE0_BASE		U(0xFF000000)
#define DEVICE0_SIZE		U(0x00E00000)
#define DEVICE1_BASE		U(0xF9000000)
#define DEVICE1_SIZE		U(0x00800000)

/* GIC-400 & interrupt handling related constants */

#define GIC_BASE		DEVICE1_BASE
#define GIC_SIZE		0x00080000
#define BASE_GICD_BASE		0xF9010000
#define BASE_GICC_BASE		0xF9020000
#define BASE_GICH_BASE		0xF9040000
#define BASE_GICV_BASE		0xF9060000

#define TTC_BASE		U(0xff140000)
#define TTC_SIZE		U(0x00010000)

#define SYS_CNT_BASE1		TTC_BASE
#define SYS_CNT_SIZE		TTC_SIZE

/* timer */
#define LPD_IOU_SLCR		U(0xff180000)
#define LPD_IOU_SLCR_SIZE	U(0x00010000)
#define TTC_TIMER_IRQ		U(77)
#define TTC_CLK_SEL_OFFSET	U(0x380)
#define IRQ_TWDOG_INTID		TTC_TIMER_IRQ
#endif /* __PLATFORM_DEF_H__ */
