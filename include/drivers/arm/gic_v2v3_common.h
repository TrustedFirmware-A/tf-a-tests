/*
 * Copyright (c) 2018-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __GIC_COMMON_H__
#define __GIC_COMMON_H__

#include <stdbool.h>

#include <drivers/arm/arm_gic.h>

/***************************************************************************
 * Defines and prototypes for ARM GIC driver.
 **************************************************************************/
#define MIN_SGI_ID		0
#define MAX_SGI_ID		15
#define MIN_PPI_ID		16
#define MAX_PPI_ID		31
#define MIN_SPI_ID		32
#define MAX_SPI_ID		1019

#define IS_SGI(irq_num)							\
	(((irq_num) >= MIN_SGI_ID) && ((irq_num) <= MAX_SGI_ID))

#define IS_PPI(irq_num)							\
	(((irq_num) >= MIN_PPI_ID) && ((irq_num) <= MAX_PPI_ID))

#define IS_SPI(irq_num)							\
	(((irq_num) >= MIN_SPI_ID) && ((irq_num) <= MAX_SPI_ID))

#define IS_PLAT_SPI(irq_num)						\
	(((irq_num) >= MIN_SPI_ID) &&					\
	 ((irq_num) <= MIN_SPI_ID + PLAT_MAX_SPI_OFFSET_ID))

#define IS_VALID_INTR_ID(irq_num)					\
	(((irq_num) >= MIN_SGI_ID) && ((irq_num) <= MAX_SPI_ID))

/***************************************************************************
 * Defines and prototypes common to GIC v2 and v3 drivers.
 **************************************************************************/
/* Distributor interface register offsets */
#define GICD_CTLR		0x0
#define GICD_TYPER		0x4
#define GICD_ISENABLER		0x100
#define GICD_ICENABLER		0x180
#define GICD_ISPENDR		0x200
#define GICD_ICPENDR		0x280
#define GICD_ISACTIVER		0x300
#define GICD_ICACTIVER		0x380
#define GICD_IPRIORITYR		0x400
#define GICD_ICFGR		0xC00

/* Distributor interface register shifts */
#define ISENABLER_SHIFT		5
#define ICENABLER_SHIFT		ISENABLER_SHIFT
#define ISPENDR_SHIFT		5
#define ICPENDR_SHIFT		ISPENDR_SHIFT
#define ISACTIVER_SHIFT		5
#define ICACTIVER_SHIFT		ISACTIVER_SHIFT
#define IPRIORITYR_SHIFT	2
#define ICFGR_SHIFT		4

/* GICD_TYPER bit definitions */
#define IT_LINES_NO_MASK	0x1f

/* GICD Priority register mask */
#define GIC_PRI_MASK		0xff

/*
 * Number of per-cpu interrupts to save prior to system suspend.
 * This comprises all SGIs and PPIs.
 */
#define NUM_PCPU_INTR	32

#ifndef __ASSEMBLY__

#include <mmio.h>

/* Helper to detect the GIC mode (GICv2 or GICv3) configured in the system */
bool is_gicv3_mode(void);

/*******************************************************************************
 * Private GIC Distributor function prototypes for use by GIC drivers
 ******************************************************************************/
unsigned int gicd_read_isenabler(uintptr_t base, unsigned int interrupt_id);
unsigned int gicd_read_icenabler(uintptr_t base, unsigned int interrupt_id);
unsigned int gicd_read_ispendr(uintptr_t base, unsigned int interrupt_id);
unsigned int gicd_read_icpendr(uintptr_t base, unsigned int interrupt_id);
unsigned int gicd_read_isactiver(uintptr_t base, unsigned int interrupt_id);
unsigned int gicd_read_icactiver(uintptr_t base, unsigned int interrupt_id);
unsigned int gicd_read_ipriorityr(uintptr_t base, unsigned int interrupt_id);
unsigned int gicd_get_ipriorityr(uintptr_t base, unsigned int interrupt_id);
unsigned int gicd_read_icfgr(uintptr_t base, unsigned int interrupt_id);
void gicd_write_isenabler(uintptr_t base, unsigned int interrupt_id,
					unsigned int val);
void gicd_write_icenabler(uintptr_t base, unsigned int interrupt_id,
					unsigned int val);
void gicd_write_ispendr(uintptr_t base, unsigned int interrupt_id,
					unsigned int val);
void gicd_write_icpendr(uintptr_t base, unsigned int interrupt_id,
					unsigned int val);
void gicd_write_isactiver(uintptr_t base, unsigned int interrupt_id,
					unsigned int val);
void gicd_write_icactiver(uintptr_t base, unsigned int interrupt_id,
					unsigned int val);
void gicd_write_ipriorityr(uintptr_t base, unsigned int interrupt_id,
					unsigned int val);
void gicd_write_icfgr(uintptr_t base, unsigned int interrupt_id,
					unsigned int val);
unsigned int gicd_get_isenabler(uintptr_t base, unsigned int interrupt_id);
void gicd_set_isenabler(uintptr_t base, unsigned int interrupt_id);
void gicd_set_icenabler(uintptr_t base, unsigned int interrupt_id);
void gicd_set_ispendr(uintptr_t base, unsigned int interrupt_id);
void gicd_set_icpendr(uintptr_t base, unsigned int interrupt_id);
void gicd_set_isactiver(uintptr_t base, unsigned int interrupt_id);
void gicd_set_icactiver(uintptr_t base, unsigned int interrupt_id);
void gicd_set_ipriorityr(uintptr_t base, unsigned int interrupt_id,
					unsigned int priority);
bool gicv2v3_is_irq_spi(unsigned int irq_num);
void gicv2v3_irq_setup(void);
irq_handler_t *gicv2v3_get_irq_handler(unsigned int irq_num);

static inline unsigned int gicv2v3_get_sgi_num(unsigned int irq_num,
						unsigned int core_pos)
{
	/* the SGI index is the INTID */
	return irq_num;
}

/*******************************************************************************
 * Private GIC Distributor interface accessors for reading and writing
 * entire registers
 ******************************************************************************/
static inline unsigned int gicd_read_ctlr(uintptr_t base)
{
	return mmio_read_32(base + GICD_CTLR);
}

static inline unsigned int gicd_read_typer(uintptr_t base)
{
	return mmio_read_32(base + GICD_TYPER);
}

static inline void gicd_write_ctlr(uintptr_t base, unsigned int val)
{
	mmio_write_32(base + GICD_CTLR, val);
}


#endif /*__ASSEMBLY__*/
#endif /* __GIC_COMMON_H__ */
