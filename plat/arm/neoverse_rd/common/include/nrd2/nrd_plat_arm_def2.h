/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * This file is limited to include the trusted firmware required platform port
 * definitions for the second generation platforms based on the N2/V2 CPUs. The
 * common platform support for Arm platforms expect platforms to define certain
 * definitions and those definitions are referred to as the platform port
 * definitions.
 */

#ifndef NRD_PLAT_ARM_DEF2_H
#define NRD_PLAT_ARM_DEF2_H

#ifndef __ASSEMBLER__
#include <lib/mmio.h>
#endif /* __ASSEMBLER__ */

#include <lib/utils_def.h>
#include "nrd_css_fw_def2.h"

/*******************************************************************************
 * Linker related definitions
 ******************************************************************************/

/* Platform binary types for linking */
#define PLATFORM_LINKER_FORMAT		"elf64-littleaarch64"
#define PLATFORM_LINKER_ARCH		aarch64

/*******************************************************************************
 * Stack size
 ******************************************************************************/

/* Size of cacheable stacks */
#define PLATFORM_STACK_SIZE		U(0x1400) /* 5120 bytes */

/* Size of coherent stacks */
#define PCPU_DV_MEM_STACK_SIZE		U(0x600) /* 1536 bytes */

/*******************************************************************************
 * Core count
 ******************************************************************************/

#define PLATFORM_CORE_COUNT		(PLAT_ARM_CLUSTER_COUNT * \
						NRD_MAX_CPUS_PER_CLUSTER)
#define PLATFORM_NUM_AFFS		(PLAT_ARM_CLUSTER_COUNT + \
						PLATFORM_CORE_COUNT)

/*******************************************************************************
 * Power related definitions
 ******************************************************************************/

#define PLATFORM_MAX_AFFLVL		MPIDR_AFFLVL1

#define PLAT_MAX_PWR_LEVEL		PLATFORM_MAX_AFFLVL
#define PLAT_MAX_PWR_STATES_PER_LVL	U(2)

/* Local state bit width for each level in the state-ID field of power state */
#define PLAT_LOCAL_PSTATE_WIDTH		U(4)

/*******************************************************************************
 * XLAT definitions
 ******************************************************************************/

/* Platform specific page table and MMU setup constants */
#define MAX_XLAT_TABLES			U(6)
#define MAX_MMAP_REGIONS		U(16)

/*******************************************************************************
 * I/O definitions
 ******************************************************************************/

/* I/O Storage NOR flash device */
#define MAX_IO_DEVICES			U(1)
#define MAX_IO_HANDLES			U(1)

/*******************************************************************************
 * Non-Secure Software Generated Interupts IDs
 ******************************************************************************/

/* Non-Secure Software Generated Interupts IDs */
#define IRQ_NS_SGI_0			U(0)
#define IRQ_NS_SGI_7			U(7)

/* Maximum SPI */
#define PLAT_MAX_SPI_OFFSET_ID	U(256)

/*******************************************************************************
 * Timer related config
 ******************************************************************************/

/* Per-CPU Hypervisor Timer Interrupt ID */
#define IRQ_PCPU_HP_TIMER		U(26)

/* Memory mapped Generic timer interfaces */
#define SYS_CNT_BASE1			NRD_CSS_NSEC_CNT_BASE1

/* AP_REFCLK Generic Timer, Non-secure. */
#define IRQ_CNTPSIRQ1			U(109)

/* Times(in ms) used by test code for completion of different events */
#define PLAT_SUSPEND_ENTRY_TIME		U(15)
#define PLAT_SUSPEND_ENTRY_EXIT_TIME	U(30)

/*******************************************************************************
 * Console config
 ******************************************************************************/

#define PLAT_ARM_UART_BASE		NRD_CSS_NSEC_UART_BASE
#define PLAT_ARM_UART_CLK_IN_HZ		NRD_CSS_NSEC_CLK_IN_HZ

/*******************************************************************************
 * DRAM config
 ******************************************************************************/

/* TF-A reserves DRAM space 0xFF000000- 0xFFFFFFFF for TZC */
#define DRAM_BASE			NRD_CSS_DRAM1_BASE
#define DRAM_SIZE			(NRD_CSS_DRAM1_SIZE - 0x1000000)

/*******************************************************************************
 * Cache related config
 ******************************************************************************/
#define CACHE_WRITEBACK_SHIFT		U(6)
#define CACHE_WRITEBACK_GRANULE		(1 << CACHE_WRITEBACK_SHIFT)

/*******************************************************************************
 * Run-time address of the TFTF image.
 * It has to match the location where the Trusted Firmware-A loads the BL33
 * image.
 ******************************************************************************/

#define TFTF_BASE			UL(0xE0000000)

/*******************************************************************************
 * TFTF NVM configs
 ******************************************************************************/

#define TFTF_NVM_OFFSET			U(0x0)
#define TFTF_NVM_SIZE			UL(0x08000000)	/* 128 MB */

/*******************************************************************************
 * Watchdog related config
 ******************************************************************************/

/* Base address of trusted watchdog (SP805) */
#define SP805_TWDOG_BASE		NRD_CSS_TWDOG_BASE

/* Base address of non-trusted watchdog (SP805) */
#define SP805_WDOG_BASE		NRD_CSS_WDOG_BASE

/* Trusted watchdog (SP805) Interrupt ID */
#define IRQ_TWDOG_INTID			U(107)

/*******************************************************************************
 * Flash related config
 ******************************************************************************/

/* Base address and size of external NVM flash */
#define FLASH_BASE			UL(0x08000000)
#define FLASH_SIZE			UL(0x04000000)  /* 64MB */
#define NOR_FLASH_BLOCK_SIZE		UL(0x40000)     /* 256KB */

#endif /* NRD_PLAT_ARM_DEF2_H */