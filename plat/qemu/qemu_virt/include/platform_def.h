/*
 * Copyright (c) 2018, Arm Limited. All rights reserved.
 * Copyright (c) 2014, Linaro Limited
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef PLATFORM_DEF_H
#define PLATFORM_DEF_H

#include <arch.h>

#define PLATFORM_LINKER_FORMAT		"elf64-littleaarch64"
#define PLATFORM_LINKER_ARCH		aarch64

#define TFTF_BASE			0x60000000


#define CACHE_WRITEBACK_GRANULE		0x40

#define PLATFORM_CLUSTER_COUNT		2
#define PLATFORM_CORES_PER_CLUSTER_SHIFT	4
#define PLATFORM_CORE_COUNT_PER_CLUSTER	(1U << PLATFORM_CORES_PER_CLUSTER_SHIFT)
#define PLATFORM_CORE_COUNT		(PLATFORM_CLUSTER_COUNT * \
					PLATFORM_CORE_COUNT_PER_CLUSTER)
#define PLATFORM_NUM_AFFS		(PLATFORM_CORE_COUNT + \
						PLATFORM_CLUSTER_COUNT + 1)
#define PLATFORM_MAX_AFFLVL		MPIDR_AFFLVL2
#define PLAT_MAX_PWR_LEVEL		PLATFORM_MAX_AFFLVL
#define PLAT_MAX_PWR_STATES_PER_LVL	2


#define PLATFORM_STACK_SIZE		0x2000
#define PCPU_DV_MEM_STACK_SIZE		0x100


#define PLAT_VIRT_ADDR_SPACE_SIZE	(1ULL << 32)
#define PLAT_PHY_ADDR_SPACE_SIZE	(1ULL << 32)
#define MAX_XLAT_TABLES			8
#define MAX_MMAP_REGIONS		16

#define DRAM_BASE			0x40000000
#define DRAM_SIZE			0xc0000000

#define FLASH_BASE			0x0
#define FLASH_SIZE			0xc0000000

/* Local state bit width for each level in the state-ID field of power state */
#define PLAT_LOCAL_PSTATE_WIDTH		4

#define DEVICE0_BASE			0x8000000
#define DEVICE0_SIZE			0x0400000


#define GICD_BASE			0x8000000
#define GICC_BASE			0x8010000
#define GICR_BASE			0x80A0000

#define UART0_BASE			0x09000000
#define UART1_BASE			0x09040000
#define CRASH_CONSOLE_BASE		UART0_BASE
#define CRASH_CONSOLE_SIZE		0x1000
#define PL011_BAUDRATE			115200
#define PL011_UART_CLK_IN_HZ		1


/*******************************************************************************
 * Non-Secure Software Generated Interupts IDs
 ******************************************************************************/
#define IRQ_NS_SGI_0			0
#define IRQ_NS_SGI_1			1
#define IRQ_NS_SGI_2			2
#define IRQ_NS_SGI_3			3
#define IRQ_NS_SGI_4			4
#define IRQ_NS_SGI_5			5
#define IRQ_NS_SGI_6			6
#define IRQ_NS_SGI_7			7

/* Per-CPU Hypervisor Timer Interrupt ID */
#define IRQ_PCPU_HP_TIMER		(10 + 16)
/* Per-CPU Physical Timer Interrupt ID */
#define IRQ_PCPU_EL1_TIMER		(14 + 16)

#define PLAT_MAX_SPI_OFFSET_ID		256

/*
 * Times(in ms) used by test code for completion of different events.
 * TODO: Tune (if required)
 */
#define PLAT_SUSPEND_ENTRY_TIME		15
#define PLAT_SUSPEND_ENTRY_EXIT_TIME	30

#if USE_NVM
#error
#else
/*
 * If you want to run without support for non-volatile memory (due to
 * e.g. unavailability of a flash driver), DRAM can be used instead as
 * a workaround. The TFTF binary itself is loaded at 0x88000000 so the
 * first 128MB can be used
 */
#define TFTF_NVM_OFFSET		0x0
#define TFTF_NVM_SIZE		(TFTF_BASE - DRAM_BASE - TFTF_NVM_OFFSET)
#endif

/*
 * DT related constants
 */
#define PLAT_QEMU_DT_BASE		DRAM_BASE
#define PLAT_QEMU_DT_MAX_SIZE		0x100000

/*
 * FIXME: Dummy definitions that we need just to compile...
 */
#define ARM_SECURE_SERVICE_BUFFER_BASE	0
#define ARM_SECURE_SERVICE_BUFFER_SIZE	100
#define SYS_CNT_BASE1			0
#define IRQ_TWDOG_INTID			56

#endif /* PLATFORM_DEF_H */

