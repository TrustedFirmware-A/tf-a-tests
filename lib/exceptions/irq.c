/*
 * Copyright (c) 2018-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <assert.h>
#include <debug.h>
#include <drivers/arm/arm_gic.h>
#include <irq.h>
#include <plat_topology.h>
#include <platform.h>
#include <platform_def.h>
#include <power_management.h>
#include <spinlock.h>
#include <string.h>
#include <tftf.h>
#include <tftf_lib.h>

/*
 * For a given SPI, the associated IRQ handler is common to all CPUs.
 * Therefore, we need a lock to prevent simultaneous updates.
 *
 * We use one lock for all SPIs. This will make it impossible to update
 * different SPIs' handlers at the same time (although it would be fine) but it
 * saves memory. Updating an SPI handler shouldn't occur that often anyway so we
 * shouldn't suffer from this restriction too much.
 */
static spinlock_t shared_irq_lock;


unsigned int tftf_irq_get_my_sgi_num(unsigned int seq_id)
{
	unsigned int core_pos = platform_get_core_pos(read_mpidr_el1());
	return arm_gic_get_sgi_num(seq_id, core_pos);
}

void tftf_send_sgi(unsigned int sgi_id, unsigned int core_pos)
{
	/*
	 * Ensure that all memory accesses prior to sending the SGI are
	 * completed.
	 */
	dsbish();

	/*
	 * Don't send interrupts to CPUs that are powering down. That would be a
	 * violation of the PSCI CPU_OFF caller responsibilities. The PSCI
	 * specification explicitely says:
	 * "Asynchronous wake-ups on a core that has been switched off through a
	 * PSCI CPU_OFF call results in an erroneous state. When this erroneous
	 * state is observed, it is IMPLEMENTATION DEFINED how the PSCI
	 * implementation reacts."
	 */
	assert(tftf_is_core_pos_online(core_pos));
	arm_gic_send_sgi(sgi_id, core_pos);
}

void tftf_irq_enable(unsigned int irq_num, uint8_t irq_priority)
{
	arm_gic_set_intr_target(irq_num, platform_get_core_pos(read_mpidr_el1()));
	arm_gic_set_intr_priority(irq_num, irq_priority);
	arm_gic_intr_enable(irq_num);

	VERBOSE("Enabled IRQ #%u\n", irq_num);
}

void tftf_irq_enable_sgi(unsigned int sgi_id, uint8_t irq_priority)
{
	unsigned int irq_num = tftf_irq_get_my_sgi_num(sgi_id);

	tftf_irq_enable(irq_num, irq_priority);
}

void tftf_irq_disable(unsigned int irq_num)
{
	/* Disable the interrupt */
	arm_gic_intr_disable(irq_num);

	VERBOSE("Disabled IRQ #%u\n", irq_num);
}

void tftf_irq_disable_sgi(unsigned int sgi_id)
{
	unsigned int irq_num = tftf_irq_get_my_sgi_num(sgi_id);

	tftf_irq_disable(irq_num);
}

#define HANDLER_VALID(handler, expect_handler)		\
	((expect_handler) ? ((handler) != NULL) : ((handler) == NULL))

static int tftf_irq_update_handler(unsigned int irq_num,
				   irq_handler_t irq_handler,
				   bool expect_handler)
{
	irq_handler_t *cur_handler;
	int ret = -1;

	cur_handler = arm_gic_get_irq_handler(irq_num);
	if (arm_gic_is_irq_shared(irq_num))
		spin_lock(&shared_irq_lock);

	/*
	 * Update the IRQ handler, if the current handler is in the expected
	 * state
	 */
	assert(HANDLER_VALID(*cur_handler, expect_handler));
	if (HANDLER_VALID(*cur_handler, expect_handler)) {
		*cur_handler = irq_handler;
		ret = 0;
	}

	if (arm_gic_is_irq_shared(irq_num))
		spin_unlock(&shared_irq_lock);

	return ret;
}

int tftf_irq_register_handler(unsigned int irq_num, irq_handler_t irq_handler)
{
	int ret;

	ret = tftf_irq_update_handler(irq_num, irq_handler, false);
	if (ret == 0)
		INFO("Registered IRQ handler %p for IRQ #%u\n",
			(void *)(uintptr_t) irq_handler, irq_num);

	return ret;
}

int tftf_irq_register_handler_sgi(unsigned int sgi_id, irq_handler_t irq_handler)
{
	unsigned int irq_num = tftf_irq_get_my_sgi_num(sgi_id);
	return tftf_irq_register_handler(irq_num, irq_handler);
}

int tftf_irq_unregister_handler(unsigned int irq_num)
{
	int ret;

	ret = tftf_irq_update_handler(irq_num, NULL, true);
	if (ret == 0)
		INFO("Unregistered IRQ handler for IRQ #%u\n", irq_num);

	return ret;
}

int tftf_irq_unregister_handler_sgi(unsigned int sgi_id)
{
	unsigned int irq_num = tftf_irq_get_my_sgi_num(sgi_id);
	return tftf_irq_unregister_handler(irq_num);
}

int tftf_irq_handler_dispatcher(void)
{
	unsigned int raw_iar;
	unsigned int irq_num;
	irq_handler_t *handler;
	void *irq_data = NULL;
	int rc = 0;

	/* Acknowledge the interrupt */
	irq_num = arm_gic_intr_ack(&raw_iar);

	handler = arm_gic_get_irq_handler(irq_num);
	irq_data = &irq_num;

	if (*handler != NULL)
		rc = (*handler)(irq_data);

	/* Mark the processing of the interrupt as complete */
	if (irq_num != GIC_SPURIOUS_INTERRUPT)
		arm_gic_end_of_intr(raw_iar);

	return rc;
}

void tftf_irq_setup(void)
{
	init_spinlock(&shared_irq_lock);
}
