/*
 * Copyright (c) 2025-2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*******************************************************************************
 * Platform definitions used by common code
 *******************************************************************************/
#ifndef __PLATFORM_DEF_H__
#define __PLATFORM_DEF_H__

#include <arch.h>
#include <common_def.h>
#include <utils_def.h>
#include <xlat_tables_v2.h>

/*******************************************************************************
 * Platform binary types for linking
 ******************************************************************************/
#define PLATFORM_LINKER_FORMAT			"elf64-littleaarch64"
#define PLATFORM_LINKER_ARCH			aarch64

/*******************************************************************************
 * Run-time address of the TFTF image.
 * It has to match the location where the Trusted Firmware-A loads the BL33
 * image.
 ******************************************************************************/
#define TFTF_BASE				0xE0000000

/* Size of cacheable stacks */
#define PLATFORM_STACK_SIZE			0x1400

#define PLATFORM_CLUSTER_COUNT			U(4)
#define PLATFORM_CORE_COUNT			U(16)
#define PLAT_MAX_CPUS_PER_CLUSTER		U(4)
#define PLAT_MAX_PE_PER_CPU			U(1)

#define RDASPEN_CLUSTER_CORE_COUNT		PLAT_MAX_CPUS_PER_CLUSTER
#define RDASPEN_CLUSTER_COUNT			PLATFORM_CLUSTER_COUNT

#define DRAM_BASE				ULL(0x80000000)
#define DRAM_SIZE				SZ_2G

#define FLASH_BASE				UL(0x38000000)
#define FLASH_SIZE				SZ_128M
#define NOR_FLASH_BLOCK_SIZE			SZ_256K

/*******************************************************************************
 * GIC Interrupt Values
 * RD-Aspen uses GICv3 does not require GICC
 *******************************************************************************/
#define GICD_BASE				UL(0x20800000)
#define GICR_BASE				UL(0x20880000)
#define GICR_REGION_SIZE			SZ_256K
#define GIC_REGION_SIZE				SZ_128M

#define GICR_BASE_VIEW0_0_0			GICR_BASE
#define GICR_BASE_VIEW0_0_1			(GICR_BASE_VIEW0_0_0 + GICR_REGION_SIZE)
#define GICR_BASE_VIEW0_0_2			(GICR_BASE_VIEW0_0_1 + GICR_REGION_SIZE)
#define GICR_BASE_VIEW0_0_3			(GICR_BASE_VIEW0_0_2 + GICR_REGION_SIZE)
#define GICR_BASE_VIEW0_1_0			(GICR_BASE_VIEW0_0_3 + GICR_REGION_SIZE)
#define GICR_BASE_VIEW0_1_1			(GICR_BASE_VIEW0_1_0 + GICR_REGION_SIZE)
#define GICR_BASE_VIEW0_1_2			(GICR_BASE_VIEW0_1_1 + GICR_REGION_SIZE)
#define GICR_BASE_VIEW0_1_3			(GICR_BASE_VIEW0_1_2 + GICR_REGION_SIZE)
#define GICR_BASE_VIEW0_2_0			(GICR_BASE_VIEW0_1_3 + GICR_REGION_SIZE)
#define GICR_BASE_VIEW0_2_1			(GICR_BASE_VIEW0_2_0 + GICR_REGION_SIZE)
#define GICR_BASE_VIEW0_2_2			(GICR_BASE_VIEW0_2_1 + GICR_REGION_SIZE)
#define GICR_BASE_VIEW0_2_3			(GICR_BASE_VIEW0_2_2 + GICR_REGION_SIZE)
#define GICR_BASE_VIEW0_3_0			(GICR_BASE_VIEW0_2_3 + GICR_REGION_SIZE)
#define GICR_BASE_VIEW0_3_1			(GICR_BASE_VIEW0_3_0 + GICR_REGION_SIZE)
#define GICR_BASE_VIEW0_3_2			(GICR_BASE_VIEW0_3_1 + GICR_REGION_SIZE)
#define GICR_BASE_VIEW0_3_3			(GICR_BASE_VIEW0_3_2 + GICR_REGION_SIZE)

/* Local state bit width for each level in the state-ID field of power state */
#define PLAT_LOCAL_PSTATE_WIDTH			4

#define MAX_IO_DEVICES				3
#define MAX_IO_HANDLES				4

#define CACHE_WRITEBACK_SHIFT			6
#define CACHE_WRITEBACK_GRANULE			(1 << CACHE_WRITEBACK_SHIFT)

/*******************************************************************************
 * Non-Secure Software Generated Interrupts IDs
 ******************************************************************************/
#define IRQ_NS_SGI_0				0
#define IRQ_NS_SGI_1				1
#define IRQ_NS_SGI_2				2
#define IRQ_NS_SGI_3				3
#define IRQ_NS_SGI_4				4
#define IRQ_NS_SGI_5				5
#define IRQ_NS_SGI_6				6
#define IRQ_NS_SGI_7				7

#define PLATFORM_MAX_AFFLVL			MPIDR_AFFLVL1
#define PLATFORM_NUM_AFFS			(PLATFORM_CORE_COUNT + PLATFORM_CLUSTER_COUNT)
#define PLAT_MAX_PWR_LEVEL			PLATFORM_MAX_AFFLVL
#define PLAT_MAX_PWR_STATES_PER_LVL		U(2)

/*******************************************************************************
 * Memory mapped Peripheral Interfaces
 *******************************************************************************/
/* AP_REFCLK CNTBase1, Generic Timer. */
#define SYS_CNT_BASE1				UL(0x1A830000)
#define SYS_CNT_SIZE				SZ_64K

/* UART Base Addresses */
#define PLAT_ARM_UART_BASE			UL(0x1A400000)
#define PLAT_ARM_UART_CLK_IN_HZ			UL(24000000)

/*******************************************************************************
 * Interrupt IDs
 * 1- AP0 Non Secure Refclk Generic Timer.
 * 2- AP0 non secure watchdog timer interrupt
 * 3- Per-CPU Hypervisor Timer Interrupt ID
 * 4- Non Secure System Timer Event
 *****************************************************************************/
#define IRQ_PCPU_NS_TIMER			U(30)
#define IRQ_TWDOG_INTID				U(82)
#define IRQ_PCPU_HP_TIMER			U(26)
#define IRQ_CNTPSIRQ1				U(81)

/*******************************************************************************
 * Platform specific page table and MMU setup constants
 ******************************************************************************/
#define SZ_256T					(1ULL << 48)

/* Physical and Virtual address spaces for MMU */
#define PLAT_PHY_ADDR_SPACE_SIZE		SZ_256T
#define PLAT_VIRT_ADDR_SPACE_SIZE		SZ_256T

#define MAX_XLAT_TABLES				20
#define MAX_MMAP_REGIONS			50

/*******************************************************************************
 * Non-Secure Software Generated Interrupts IDs
 ******************************************************************************/
#define PLAT_MAX_SPI_OFFSET_ID			195


/* Size of coherent stacks for debug and release builds */
#if DEBUG
#define PCPU_DV_MEM_STACK_SIZE			0x600
#else
#define PCPU_DV_MEM_STACK_SIZE			0x500
#endif

/******************************************************************************
 * mmap regions
 * 1- Consists of 3 Device regions, UART ,Timer and GIC
 * 2- DRAM
 *****************************************************************************/
#define RDASPEN_DEVICE0_BASE			PLAT_ARM_UART_BASE
#define RDASPEN_DEVICE0_SIZE			SZ_64K
#define RDASPEN_MAP_DEVICE0			MAP_REGION_FLAT(RDASPEN_DEVICE0_BASE, \
								RDASPEN_DEVICE0_SIZE, \
								MT_DEVICE | MT_RW | \
								MT_NS)

#define RDASPEN_DEVICE1_BASE			SYS_CNT_BASE1
#define RDASPEN_DEVICE1_SIZE			SYS_CNT_SIZE
#define RDASPEN_MAP_DEVICE1			MAP_REGION_FLAT(RDASPEN_DEVICE1_BASE, \
								RDASPEN_DEVICE1_SIZE, \
								MT_DEVICE | MT_RW | \
								MT_NS)

#define RDASPEN_DEVICE2_BASE			GICD_BASE
#define RDASPEN_DEVICE2_SIZE			GIC_REGION_SIZE
#define RDASPEN_MAP_DEVICE2			MAP_REGION_FLAT(RDASPEN_DEVICE2_BASE, \
								RDASPEN_DEVICE2_SIZE, \
								MT_DEVICE | MT_RW | \
								MT_NS)

/* TODO: OP-TEE port will require us to modify this region */
#define RDASPEN_MAP_NS_DRAM1			MAP_REGION_FLAT(DRAM_BASE, \
								DRAM_SIZE, \
								MT_MEMORY | MT_RW | \
								MT_NS)

/* If you want to run without support for non-volatile memory (due to
 * e.g. unavailability of a flash driver), DRAM can be used instead as
 * a workaround. The TFTF binary itself is loaded at 0xE0000000 so the
 * first 128MB can be used.
 * Please note that this won't be suitable for all test scenarios and
 * for this reason some tests will be disabled in this configuration.
 */
#define TFTF_NVM_OFFSET				0x0
#define TFTF_NVM_SIZE				0x8000000	/* 128 MB */

/*
 * Times(in ms) used by test code for completion of different events.
 * Suspend entry time for debug build is high due to the time taken
 * by the VERBOSE/INFO prints. The value considers the worst case scenario
 * where all CPUs are going and coming out of suspend continuously.
 */
#if DEBUG
#define PLAT_SUSPEND_ENTRY_TIME			100
#define PLAT_SUSPEND_ENTRY_EXIT_TIME		200
#else
#define PLAT_SUSPEND_ENTRY_TIME			10
#define PLAT_SUSPEND_ENTRY_EXIT_TIME		20
#endif

#endif /* __PLATFORM_DEF_H__ */
