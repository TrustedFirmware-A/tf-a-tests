/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __GIC_V5_H__
#define __GIC_V5_H__

#include <drivers/arm/arm_gic.h>
#include <lib/mmio.h>

#define INT_PPI					1
#define INT_LPI					2
#define INT_SPI					3

#define GICV5_MAX_PPI_ID			128

#define GICV5_IDLE_PRIORITY			0xff

#define INT_TYPE_SHIFT				U(29)
#define INT_TYPE_WIDTH				U(3)
#define INT_ID_SHIFT				U(0)
#define INT_ID_WIDTH				U(24)

#define IRM_TARGETTED				0
#define IRM_1OFN				1

#define HM_EDGE					0
#define HM_LEVEL				1

struct l2_iste {
	uint16_t iaffid;
	uint8_t priority	: 5;
	uint8_t hwu		: 2;
	uint8_t res0		: 4;
	uint8_t irm		: 1;
	uint8_t enable		: 1;
	uint8_t hm		: 1;
	uint8_t active		: 1;
	uint8_t pending		: 1;
};

#define DEFINE_GICV5_MMIO_WRITE_FUNC(_name, _offset)				\
static inline void write_##_name(uintptr_t base, uint32_t val)			\
{										\
	mmio_write_32(base + _offset, val);					\
}

#define DEFINE_GICV5_MMIO_READ_FUNC(_name, _offset)				\
static inline uint32_t read_##_name(uintptr_t base)				\
{										\
	return mmio_read_32(base + _offset);					\
}

#define DEFINE_GICV5_MMIO_RW_FUNCS(_name, _offset)				\
	DEFINE_GICV5_MMIO_READ_FUNC(_name, _offset)				\
	DEFINE_GICV5_MMIO_WRITE_FUNC(_name, _offset)

#define IRS_IDR2_MIN_LPI_ID_BITS_WIDTH		UL(4)
#define IRS_IDR2_MIN_LPI_ID_BITS_SHIFT		U(5)
#define IRS_IDR2_ID_BITS_WIDTH			UL(5)
#define IRS_IDR2_ID_BITS_SHIFT			U(0)
#define IRS_CR0_IRSEN_BIT			BIT(0)
#define IRS_CR0_IDLE_BIT			BIT(1)
#define IRS_IST_STATUSR_IDLE_BIT		BIT(0)
#define IRS_IST_BASER_VALID_BIT			BIT(0)
#define IRS_IST_BASER_ADDR_SHIFT		6UL
#define IRS_IST_BASER_ADDR_WIDTH		50UL

DEFINE_GICV5_MMIO_READ_FUNC(IRS_IDR2,				0x0008)
DEFINE_GICV5_MMIO_RW_FUNCS(  IRS_CR0,				0x0080)
DEFINE_GICV5_MMIO_RW_FUNCS(  IRS_IST_BASER,			0x0180)
DEFINE_GICV5_MMIO_RW_FUNCS(  IRS_IST_CFGR,			0x0190)
DEFINE_GICV5_MMIO_RW_FUNCS(  IRS_IST_STATUSR,			0x0194)

#define WAIT_FOR_IDLE(base, reg)						\
	do {									\
		while ((read_##reg(base) & reg##_IDLE_BIT) == 0) {}		\
	} while (0)

#ifdef __aarch64__
bool is_gicv5_mode(void);
bool gicv5_is_irq_spi(unsigned int interrupt_id);
void gicv5_enable_cpuif(void);
void gicv5_setup_cpuif(void);
void gicv5_disable_cpuif(void);
void gicv5_save_cpuif_context(void);
void gicv5_restore_cpuif_context(void);
void gicv5_set_priority(unsigned int interrupt_id, unsigned int priority);
void gicv5_send_sgi(unsigned int sgi_id, unsigned int core_pos);
void gicv5_set_intr_route(unsigned int interrupt_id, unsigned int core_pos);
void gicv5_intr_enable(unsigned int interrupt_id);
void gicv5_intr_disable(unsigned int interrupt_id);
unsigned int gicv5_acknowledge_interrupt(void);
unsigned int gicv5_is_intr_pending(unsigned int interrupt_id);
void gicv5_intr_clear(unsigned int interrupt_id);
void gicv5_end_of_interrupt(unsigned int raw_iar);
void gicv5_setup(void);
uint32_t gicv5_get_sgi_num(uint32_t index, unsigned int core_pos);
irq_handler_t *gicv5_get_irq_handler(unsigned int interrupt_id);
void gicv5_init(uintptr_t irs_base_addr);
#else
static inline bool is_gicv5_mode(void) { return false; }
static inline bool gicv5_is_irq_spi(unsigned int interrupt_id) { return false; }
static inline void gicv5_enable_cpuif(void) {}
static inline void gicv5_setup_cpuif(void) {}
static inline void gicv5_disable_cpuif(void) {}
static inline void gicv5_save_cpuif_context(void) {}
static inline void gicv5_restore_cpuif_context(void) {}
static inline void gicv5_set_priority(unsigned int interrupt_id, unsigned int priority) {}
static inline void gicv5_send_sgi(unsigned int sgi_id, unsigned int core_pos) {}
static inline void gicv5_set_intr_route(unsigned int interrupt_id, unsigned int core_pos) {}
static inline void gicv5_intr_enable(unsigned int interrupt_id) {}
static inline void gicv5_intr_disable(unsigned int interrupt_id) {}
static inline unsigned int gicv5_acknowledge_interrupt(void) { return -1; }
static inline unsigned int gicv5_is_intr_pending(unsigned int interrupt_id) { return -1; }
static inline void gicv5_intr_clear(unsigned int interrupt_id) {}
static inline void gicv5_end_of_interrupt(unsigned int raw_iar) {}
static inline void gicv5_setup(void) {}
static inline uint32_t gicv5_get_sgi_num(uint32_t index, unsigned int core_pos) { return 0; }
static inline irq_handler_t *gicv5_get_irq_handler(unsigned int interrupt_id) {return NULL; }
static inline void gicv5_init(uintptr_t irs_base_addr) {}
#endif

#endif /* __GIC_V5_H__ */
