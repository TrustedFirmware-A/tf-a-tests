/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * This file is limited to include the CSS specific memory and interrupt map
 * definitions for the first generation platforms based on the A75, N1 and V1
 * CPUs. There are minor differences in the memory map of these platforms and
 * those differences are not in the scope of this file.
 */

#ifndef NRD_CSS_DEF1_H
#define NRD_CSS_DEF1_H

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

#endif /* NRD_CSS_DEF1_H */