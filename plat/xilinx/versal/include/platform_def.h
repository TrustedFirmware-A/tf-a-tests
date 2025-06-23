/*
 * Copyright (c) 2022-2023, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLATFORM_DEF_H
#define PLATFORM_DEF_H

#include <arch.h>

#define PLATFORM_LINKER_FORMAT			"elf64-littleaarch64"
#define PLATFORM_LINKER_ARCH			aarch64

#define TFTF_BASE				U(0x8000000)

#define CACHE_WRITEBACK_GRANULE			U(0x40)

#define PLATFORM_CLUSTER_COUNT			1
#define PLATFORM_CORE_COUNT_PER_CLUSTER		2
#define PLATFORM_CORE_COUNT			(PLATFORM_CLUSTER_COUNT * \
						PLATFORM_CORE_COUNT_PER_CLUSTER)
#define PLATFORM_NUM_AFFS			(PLATFORM_CORE_COUNT + \
						PLATFORM_CLUSTER_COUNT + 1)
#define PLATFORM_MAX_AFFLVL			MPIDR_AFFLVL2
#define PLAT_MAX_PWR_LEVEL			MPIDR_AFFLVL2
#define PLAT_MAX_PWR_STATES_PER_LVL		2


#define PLATFORM_STACK_SIZE			U(0x880)
#define PCPU_DV_MEM_STACK_SIZE			U(0x440)


#define PLAT_VIRT_ADDR_SPACE_SIZE		(1ULL << 32)
#define PLAT_PHY_ADDR_SPACE_SIZE		(1ULL << 32)
#define MAX_XLAT_TABLES				U(8)
#define MAX_MMAP_REGIONS			U(16)

#define DRAM_BASE				U(0x0)
#define DRAM_SIZE				U(0x80000000)

/*******************************************************************************
 * LOW DDR MAX defines
 ******************************************************************************/
#define PLAT_DDR_LOWMEM_MAX		U(0x80000000)

/*
 * TFTF_NVM_OFFSET/SIZE correspond to the NVM partition in the partition
 * table
 */
#define TFTF_NVM_SIZE				U(0x600000)
#define TFTF_NVM_OFFSET				U(0x20000000)

/* Local state bit width for each level in the state-ID field of power state */
#define PLAT_LOCAL_PSTATE_WIDTH			U(4)

/* GIC-400 related addresses from datasheet */
#define GICD_REG_BASE				U(0xf9000000)
#define GICC_REG_BASE				U(0xf9040000)
#define GICR_REG_BASE				U(0xf9080000)

/*
 * Memory mapped devices that we must create MMU mappings for them
 */
#define GIC_BASE				GICD_REG_BASE
#define GIC_SIZE				U(0x01000000)

#define TTC_BASE				U(0xff0e0000)
#define TTC_SIZE				U(0x00010000)

#define SYS_CNT_BASE1				TTC_BASE
#define SYS_CNT_SIZE				TTC_SIZE

#define LPD_IOU_SLCR				U(0xff080000)
#define LPD_IOU_SLCR_SIZE			U(0x00010000)

/* ARM PL011 UART */
#define PL011_UART0_BASE			U(0xff000000)
#define PL011_BAUDRATE				U(115200)
#define PL011_UART_CLK_IN_HZ			U(100000000)

#define PLAT_ARM_UART_BASE                      PL011_UART0_BASE
#define PLAT_ARM_UART_SIZE                      U(0x1000)

#define CRASH_CONSOLE_BASE			PL011_UART0_BASE
#define CRASH_CONSOLE_SIZE			PLAT_ARM_UART_SIZE

/* Per-CPU Hypervisor Timer Interrupt ID */
#define IRQ_PCPU_HP_TIMER			U(29)
/* Datasheet: TIME00 event*/
#define IRQ_CNTPSIRQ1				U(29)

/* Refer to AM011(v1.5), Chapter 50, Page 430 */
#define PLAT_MAX_SPI_OFFSET_ID			U(223)

/*
 * Times(in ms) used by test code for completion of different events.
 */
#define PLAT_SUSPEND_ENTRY_TIME			U(15)
#define PLAT_SUSPEND_ENTRY_EXIT_TIME		U(30)

/*
 * Dummy definitions that we need just to compile...
 */
#define ARM_SECURE_SERVICE_BUFFER_BASE		U(0)
#define ARM_SECURE_SERVICE_BUFFER_SIZE		U(100)

/* LPD_SWDT_INT, AM011(v1.5), Chapter 50, Page 428 */
#define IRQ_TWDOG_INTID				U(0x51)

#define TTC_TIMER_IRQ				U(69)
#define TTC_CLK_SEL_OFFSET			U(0x360)

#endif /* PLATFORM_DEF_H */
