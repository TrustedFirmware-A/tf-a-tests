/*
 * Copyright (c) 2016-2018, ARM Limited and Contributors. All rights reserved.
 * Copyright (c) 2022-2024, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CDNS_UART_H
#define CDNS_UART_H

#include <drivers/console.h>
#include <lib/utils_def.h>
/* This is very minimalistic and will only work in QEMU.  */

/* CADENCE Registers */
#define R_UART_CR	0
#define R_UART_CR_RXRST	(1 << 0) /* RX logic reset */
#define R_UART_CR_TXRST	(1 << 1) /* TX logic reset */
#define R_UART_CR_RX_EN	(1 << 2) /* RX enabled */
#define R_UART_CR_TX_EN	(1 << 4) /* TX enabled */

#define R_UART_SR		0x2C
#define UART_SR_INTR_REMPTY_BIT	1
#define UART_SR_INTR_TFUL_BIT	4
#define UART_SR_INTR_TEMPTY_BIT	3

#define R_UART_TX	0x30
#define R_UART_RX	0x30

#define CONSOLE_T_BASE		(U(5) * REGSZ)

#endif /* CDNS_UART_H */
