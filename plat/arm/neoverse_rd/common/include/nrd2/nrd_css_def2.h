/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * This file contains the CSS specific definitions for the second generation
 * platforms based on the N2/V2 CPU.
 */

#ifndef NRD_CSS_DEF2_H
#define NRD_CSS_DEF2_H

/*******************************************************************************
 * CSS memory map related defines
 ******************************************************************************/

/* Sub-system Peripherals */
#define NRD_CSS_PERIPH0_BASE		UL(0x2A000000)
#define NRD_CSS_PERIPH0_SIZE		UL(0x26000000)

/* Peripherals and PCIe expansion area */
#define NRD_CSS_PERIPH1_BASE		UL(0x60000000)
#define NRD_CSS_PERIPH1_SIZE		UL(0x20000000)

/* DRAM base address and size */
#define NRD_CSS_DRAM1_BASE		UL(0x80000000)
#define NRD_CSS_DRAM1_SIZE		UL(0x80000000)

/* AP Non-Secure UART related constants */
#define NRD_CSS_NSEC_UART_BASE		UL(0x2A400000)

#endif /* NRD_CSS_DEF2_H */