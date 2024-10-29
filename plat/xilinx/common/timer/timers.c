/*
 * Copyright (c) 2022-2023, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <stddef.h>

#include <debug.h>
#include <mmio.h>
#include <platform.h>
#include <tftf_lib.h>
#include <timer.h>
#include <utils_def.h>

#define TTC_OFFSET_TMR_0		U(0)
#define TTC_OFFSET_TMR_1		U(4)
#define TTC_OFFSET_TMR_2		U(8)

#define TTC_CLK_CNTRL_OFFSET		U(0x00) /* Clock Control Reg, RW */
#define TTC_CNT_CNTRL_OFFSET		U(0x0C) /* Counter Control Reg, RW */
#define TTC_COUNT_VAL_OFFSET		U(0x18) /* Counter Value Reg, RO */
#define TTC_INTR_VAL_OFFSET		U(0x24) /* Interval Count Reg, RW */
#define TTC_ISR_OFFSET			U(0x54) /* Interrupt Status Reg, RO */
#define TTC_IER_OFFSET			U(0x60) /* Interrupt Enable Reg, RW */

#define TTC_CNT_CNTRL_DISABLE_MASK	BIT(0)

#define TTC_CLK_SEL_OFFSET		U(0x360)
#define TTC_CLK_SEL_MASK		GENMASK(1, 0)

#define TTC_CLK_SEL_PS_REF		BIT(0)
#define TTC_CLK_SEL_RPU_REF		BIT(4)

#define RET_SUCCESS			U(0)

/*
 * Setup the timers to use pre-scaling, using a fixed value for now that will
 * work across most input frequency, but it may need to be more dynamic
 */
#define PRESCALE_EXPONENT		U(16) /* 2 ^ PRESCALE_EXPONENT = PRESCALE */
#define PRESCALE			U(65536) /* The exponent must match this */
#define CLK_CNTRL_PRESCALE		((PRESCALE_EXPONENT - 1) << 1U)
#define CLK_CNTRL_PRESCALE_EN		BIT(0)
#define CNT_CNTRL_RESET			BIT(4)

/* Resolution obtained as per the input clock and Prescale value
 * Clock Selected : PS_REF_CLK
 * Clock Value : 33333333Hz (33.33MHz)
 * Prescalar for TTC, N : 15 (highest)
 * Prescalar Applied 2^(N+1) : 65536
 * Input clock : (PS_REF_CLK)/Prescalar) : 508.6263Hz
 * Resolution (1/InputClock) : 1.966miliseconds ~2ms
 */
const unsigned long INTERVAL = 2;

static void timer_write_32(uint32_t offset, uint32_t val)
{
	/* actual write */
	mmio_write_32(SYS_CNT_BASE1 + offset, val);
}

static uint32_t timer_read_32(uint32_t offset)
{
	/* actual read */
	return mmio_read_32(SYS_CNT_BASE1 + offset);
}

static int cancel_timer(void)
{
	/* Disable Interrupt */
	timer_write_32(TTC_IER_OFFSET, 0);

	/* Disable Counter */
	timer_write_32(TTC_CLK_CNTRL_OFFSET, !CLK_CNTRL_PRESCALE_EN);
	timer_write_32(TTC_CNT_CNTRL_OFFSET, !CLK_CNTRL_PRESCALE_EN);

	return RET_SUCCESS;
}

static void clocksetup(void)
{
	timer_write_32(TTC_OFFSET_TMR_0 + TTC_CLK_CNTRL_OFFSET, 0x0);

	mmio_write_32(LPD_IOU_SLCR + TTC_CLK_SEL_OFFSET, TTC_CLK_SEL_PS_REF);

	VERBOSE("%s TTC_CLK_SEL = 0x%x\n", __func__,
			mmio_read_32(LPD_IOU_SLCR + TTC_CLK_SEL_OFFSET));
}

static void setcounts(unsigned long time_out_ms)
{
	unsigned long intrvl = (time_out_ms / INTERVAL) + (time_out_ms % INTERVAL);

	timer_write_32(TTC_INTR_VAL_OFFSET, intrvl);
}

static int program_timer(unsigned long time_out_ms)
{
	uint32_t reg;

	/* Disable and program the counter */
	reg = timer_read_32(TTC_CNT_CNTRL_OFFSET);
	reg |= TTC_CNT_CNTRL_DISABLE_MASK;
	timer_write_32(TTC_CNT_CNTRL_OFFSET, reg);

	setcounts(time_out_ms);

	/* Enable the interrupt */
	timer_write_32(TTC_IER_OFFSET, 0x01);

	/* Enable the counter */
	reg |= CNT_CNTRL_RESET;
	reg &= ~TTC_CNT_CNTRL_DISABLE_MASK;
	timer_write_32(TTC_CNT_CNTRL_OFFSET, reg);

	return RET_SUCCESS;
}

static int handler_timer(void)
{
	uint32_t status;

	/* Disable the interrupts */
	timer_write_32(TTC_IER_OFFSET, 0x00);

	status = timer_read_32(TTC_ISR_OFFSET);
	if (status & 0x1)
		INFO("Timer Event! %x\n", status);
	else
		ERROR("Its not a Timer Event %d\n", status);

	return RET_SUCCESS;
}

static const plat_timer_t timers = {
	.program = program_timer,
	.cancel = cancel_timer,
	.handler = handler_timer,
	.timer_step_value = INTERVAL,
	.timer_irq = TTC_TIMER_IRQ
};

int plat_initialise_timer_ops(const plat_timer_t **timer_ops)
{
	assert(timer_ops != NULL);

	/* Disable all Interrupts on the TTC */
	timer_write_32(TTC_OFFSET_TMR_0 + TTC_IER_OFFSET, 0);
	timer_write_32(TTC_OFFSET_TMR_1 + TTC_IER_OFFSET, 0);
	timer_write_32(TTC_OFFSET_TMR_2 + TTC_IER_OFFSET, 0);

	clocksetup();

	/*
	 * Setup the clock event timer to be an interval timer which
	 * is prescaled by 32 using the interval interrupt. Leave it
	 * disabled for now.
	 */
	timer_write_32(TTC_OFFSET_TMR_0 + TTC_CNT_CNTRL_OFFSET, 0x23);
	timer_write_32(TTC_OFFSET_TMR_0 + TTC_CLK_CNTRL_OFFSET,
		       CLK_CNTRL_PRESCALE | CLK_CNTRL_PRESCALE_EN);
	timer_write_32(TTC_OFFSET_TMR_0 + TTC_IER_OFFSET, 0x01);

	*timer_ops = &timers;

	return RET_SUCCESS;
}
