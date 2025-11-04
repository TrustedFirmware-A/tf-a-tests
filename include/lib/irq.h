/*
 * Copyright (c) 2018-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __IRQ_H__
#define  __IRQ_H__

#include <cdefs.h>
#include <platform_def.h> /* For CACHE_WRITEBACK_GRANULE */
#include <stdint.h>
#include <drivers/arm/arm_gic.h>

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
#define IRQ_NUM_SGIS			(IRQ_NS_SGI_7 + 1)

/*
 * SGI sent by the timer management framework to notify CPUs when the system
 * timer fires off
 */
#define IRQ_WAKE_SGI		IRQ_NS_SGI_7
#define IRQ_NUM_SGIS		(IRQ_NS_SGI_7 + 1)

#ifndef __ASSEMBLY__

/* Keep track of the IRQ handler registered for a given SPI */
typedef struct {
	irq_handler_t handler;
} spi_desc;

/* Keep track of the IRQ handler registered for a spurious interrupt */
typedef irq_handler_t spurious_desc;

/*
 * PPIs and SGIs are interrupts that are private to a GIC CPU interface. These
 * interrupts are banked in the GIC Distributor. Therefore, each CPU can
 * set up a different IRQ handler for a given PPI/SGI.
 *
 * So we define a data structure representing an IRQ handler aligned on the
 * size of a cache line. This guarantees that in an array of these, each element
 * is loaded in a separate cache line. This allows efficient concurrent
 * manipulation of these elements on different CPUs.
 */
typedef struct {
	irq_handler_t handler;
} __aligned(CACHE_WRITEBACK_GRANULE) irq_handler_banked_t;

typedef irq_handler_banked_t ppi_desc;
typedef irq_handler_banked_t sgi_desc;

void tftf_irq_setup(void);

/*
 * Get the INTID for an SGI with sequential number seq_id
 */
unsigned int tftf_irq_get_my_sgi_num(unsigned int seq_id);

/*
 * Generic handler called upon reception of an IRQ.
 *
 * This function acknowledges the interrupt, calls the user-defined handler
 * if one has been registered then marks the processing of the interrupt as
 * complete.
 */
int tftf_irq_handler_dispatcher(void);

/*
 * Send an SGI to a given core.
 */
void tftf_send_sgi(unsigned int sgi_id, unsigned int core_pos);

/*
 * Enable interrupt #irq_num for the calling core.
 */
void tftf_irq_enable(unsigned int irq_num, uint8_t irq_priority);
void tftf_irq_enable_sgi(unsigned int sgi_id, uint8_t irq_priority);

/*
 * Disable interrupt #irq_num for the calling core.
 */
void tftf_irq_disable(unsigned int irq_num);
void tftf_irq_disable_sgi(unsigned int sgi_id);

/*
 * Register an interrupt handler for a given interrupt number.
 * Will fail if there is already an interrupt handler registered for the same
 * interrupt.
 *
 * Return 0 on success, a negative value otherwise.
 */
int tftf_irq_register_handler(unsigned int num, irq_handler_t irq_handler);
int tftf_irq_register_handler_sgi(unsigned int sgi_id, irq_handler_t irq_handler);

/*
 * Unregister an interrupt handler for a given interrupt number.
 * Will fail if there is no interrupt handler registered for that interrupt.
 *
 * Return 0 on success, a negative value otherwise.
 */
int tftf_irq_unregister_handler(unsigned int irq_num);
int tftf_irq_unregister_handler_sgi(unsigned int sgi_id);

#endif /* __ASSEMBLY__ */

#endif /* __IRQ_H__ */
