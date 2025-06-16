/*
 * Copyright (c) 2018-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>

#include <arch.h>
#include <arch_helpers.h>
#include <assert.h>
#include <debug.h>
#include <drivers/arm/gic_v2v3_common.h>
#include <drivers/arm/gic_v2.h>
#include <drivers/arm/gic_v3.h>
#include <drivers/arm/gic_v5.h>

/* Record whether a GICv3 was detected on the system */
static unsigned int gicv3_detected;
static unsigned int gicv5_detected;
static bool gic_done_init;

int arm_gic_get_version(void)
{
	assert(gic_done_init);

	if (gicv3_detected) {
		return 3;
	} else if (gicv5_detected) {
		return 5;
	} else {
		return 2;
	}
}

void arm_gic_enable_interrupts_local(void)
{
	if (gicv3_detected)
		gicv3_enable_cpuif();
	else if (gicv5_detected)
		gicv5_enable_cpuif();
	else
		gicv2_enable_cpuif();
}

void arm_gic_setup_local(void)
{
	if (gicv3_detected) {
		gicv3_probe_redistif_addr();
		gicv3_setup_cpuif();
	} else if (gicv5_detected) {
		gicv5_setup_cpuif();
	} else {
		gicv2_probe_gic_cpu_id();
		gicv2_setup_cpuif();
	}
}

void arm_gic_disable_interrupts_local(void)
{
	if (gicv3_detected)
		gicv3_disable_cpuif();
	else if (gicv5_detected)
		gicv5_disable_cpuif();
	else
		gicv2_disable_cpuif();
}

void arm_gic_save_context_local(void)
{
	if (gicv3_detected)
		gicv3_save_cpuif_context();
	else if (gicv5_detected)
		gicv5_save_cpuif_context();
	else
		gicv2_save_cpuif_context();
}

void arm_gic_restore_context_local(void)
{
	if (gicv3_detected)
		gicv3_restore_cpuif_context();
	else if (gicv5_detected)
		gicv5_restore_cpuif_context();
	else
		gicv2_restore_cpuif_context();
}

void arm_gic_save_context_global(void)
{
	if (gicv3_detected) {
		gicv3_save_sgi_ppi_context();
	} else if (gicv5_detected) {
		/* NOP, done by EL3 */
	} else {
		gicv2_save_sgi_ppi_context();
	}
}

void arm_gic_restore_context_global(void)
{
	if (gicv3_detected) {
		gicv3_setup_distif();
		gicv3_restore_sgi_ppi_context();
	} else if (gicv5_detected) {
		/* NOP, done by EL3 */
	} else {
		gicv2_setup_distif();
		gicv2_restore_sgi_ppi_context();
	}
}

void arm_gic_setup_global(void)
{
	if (gicv3_detected)
		gicv3_setup_distif();
	else if (gicv5_detected)
		gicv5_setup();
	else
		gicv2_setup_distif();
}

unsigned int arm_gic_get_intr_priority(unsigned int num)
{
	if (gicv3_detected)
		return gicv3_get_ipriorityr(num);
	/* TODO only used for SDEI, currently not supported/ported */
	else if (gicv5_detected) {
		assert(0);
		return 0;
	} else
		return gicv2_gicd_get_ipriorityr(num);
}

void arm_gic_set_intr_priority(unsigned int num,
				unsigned int priority)
{
	if (gicv3_detected)
		gicv3_set_ipriorityr(num, priority);
	else if (gicv5_detected)
		gicv5_set_priority(num, priority);
	else
		gicv2_gicd_set_ipriorityr(num, priority);
}

void arm_gic_send_sgi(unsigned int sgi_id, unsigned int core_pos)
{
	if (gicv3_detected)
		gicv3_send_sgi(sgi_id, core_pos);
	else if (gicv5_detected)
		gicv5_send_sgi(sgi_id, core_pos);
	else
		gicv2_send_sgi(sgi_id, core_pos);
}

void arm_gic_set_intr_target(unsigned int num, unsigned int core_pos)
{
	if (gicv3_detected)
		gicv3_set_intr_route(num, core_pos);
	else if (gicv5_detected)
		gicv5_set_intr_route(num, core_pos);
	else
		gicv2_set_itargetsr(num, core_pos);
}

unsigned int arm_gic_intr_enabled(unsigned int num)
{
	if (gicv3_detected)
		return gicv3_get_isenabler(num) != 0;
	/* TODO only used for SDEI, currently not supported/ported */
	else if (gicv5_detected) {
		assert(0);
		return 0;
	} else
		return gicv2_gicd_get_isenabler(num) != 0;
}

void arm_gic_intr_enable(unsigned int num)
{
	if (gicv3_detected)
		gicv3_set_isenabler(num);
	else if (gicv5_detected)
		gicv5_intr_enable(num);
	else
		gicv2_gicd_set_isenabler(num);
}

void arm_gic_intr_disable(unsigned int num)
{
	if (gicv3_detected)
		gicv3_set_icenabler(num);
	else if (gicv5_detected)
		gicv5_intr_disable(num);
	else
		gicv2_gicd_set_icenabler(num);
}

unsigned int arm_gic_intr_ack(unsigned int *raw_iar)
{
	assert(raw_iar);

	if (gicv3_detected) {
		*raw_iar = gicv3_acknowledge_interrupt();
		return *raw_iar;
	} else if (gicv5_detected) {
		*raw_iar = gicv5_acknowledge_interrupt();
		return *raw_iar;
	} else {
		*raw_iar = gicv2_gicc_read_iar();
		return get_gicc_iar_intid(*raw_iar);
	}
}

unsigned int arm_gic_is_intr_pending(unsigned int num)
{
	if (gicv3_detected)
		return gicv3_get_ispendr(num);
	else if (gicv5_detected)
		return gicv5_is_intr_pending(num);
	else
		return gicv2_gicd_get_ispendr(num);
}

void arm_gic_intr_clear(unsigned int num)
{
	if (gicv3_detected)
		gicv3_set_icpendr(num);
	else if (gicv5_detected)
		gicv5_intr_clear(num);
	else
		gicv2_gicd_set_icpendr(num);
}

void arm_gic_end_of_intr(unsigned int raw_iar)
{
	if (gicv3_detected)
		gicv3_end_of_interrupt(raw_iar);
	else if (gicv5_detected)
		gicv5_end_of_interrupt(raw_iar);
	else
		gicv2_gicc_write_eoir(raw_iar);
}

void arm_gic_probe(void)
{
	if (is_gicv3_mode()) {
		gicv3_detected = 1;
		INFO("GICv3 mode detected\n");
	} else if (is_gicv5_mode()) {
		gicv5_detected = 1;
		INFO("GICv5 mode detected\n");
	} else {
		INFO("GICv2 mode detected\n");
	}

	gic_done_init = true;
}

/* to not change the API, pretend the IRS is a distributor */
void arm_gic_init(uintptr_t gicc_base,
		uintptr_t gicd_base,
		uintptr_t gicr_base)
{
	if (gicv3_detected) {
		gicv3_init(gicr_base, gicd_base);
	} else if (gicv5_detected) {
		gicv5_init(gicd_base);
	} else {
		gicv2_init(gicc_base, gicd_base);
	}
}

bool arm_gic_is_espi_supported(void)
{
	/* TODO only used for FFA, currently not supported/ported on GICv5 */
	if (!gicv3_detected) {
		return false;
	}

	/* Check if extended SPI range is implemented. */
	if ((gicv3_get_gicd_typer() & TYPER_ESPI) != 0U) {
		return true;
	}

	return false;
}
