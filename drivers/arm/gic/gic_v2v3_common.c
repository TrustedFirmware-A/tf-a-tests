/*
 * Copyright (c) 2018-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <arch_helpers.h>
#include <arch_features.h>
#include <assert.h>
#include <drivers/arm/gic_v2v3_common.h>
#include <drivers/arm/gic_v3.h>
#include <mmio.h>

/*******************************************************************************
 * GIC Distributor interface accessors for reading entire registers
 ******************************************************************************/

unsigned int gicd_read_isenabler(uintptr_t base, unsigned int interrupt_id)
{
	unsigned int n = interrupt_id >> ISENABLER_SHIFT;
	return mmio_read_32(base + GICD_ISENABLER + (n << 2));
}

unsigned int gicd_read_icenabler(uintptr_t base, unsigned int interrupt_id)
{
	unsigned int n = interrupt_id >> ICENABLER_SHIFT;
	return mmio_read_32(base + GICD_ICENABLER + (n << 2));
}

unsigned int gicd_read_ispendr(uintptr_t base, unsigned int interrupt_id)
{
	unsigned int n = interrupt_id >> ISPENDR_SHIFT;
	return mmio_read_32(base + GICD_ISPENDR + (n << 2));
}

unsigned int gicd_read_icpendr(uintptr_t base, unsigned int interrupt_id)
{
	unsigned int n = interrupt_id >> ICPENDR_SHIFT;
	return mmio_read_32(base + GICD_ICPENDR + (n << 2));
}

unsigned int gicd_read_isactiver(uintptr_t base, unsigned int interrupt_id)
{
	unsigned int n = interrupt_id >> ISACTIVER_SHIFT;
	return mmio_read_32(base + GICD_ISACTIVER + (n << 2));
}

unsigned int gicd_read_icactiver(uintptr_t base, unsigned int interrupt_id)
{
	unsigned int n = interrupt_id >> ICACTIVER_SHIFT;
	return mmio_read_32(base + GICD_ICACTIVER + (n << 2));
}

unsigned int gicd_read_ipriorityr(uintptr_t base, unsigned int interrupt_id)
{
	unsigned int n = interrupt_id >> IPRIORITYR_SHIFT;
	return mmio_read_32(base + GICD_IPRIORITYR + (n << 2));
}

unsigned int gicd_read_icfgr(uintptr_t base, unsigned int interrupt_id)
{
	unsigned int n = interrupt_id >> ICFGR_SHIFT;
	return mmio_read_32(base + GICD_ICFGR + (n << 2));
}

/*******************************************************************************
 * GIC Distributor interface accessors for writing entire registers
 ******************************************************************************/

void gicd_write_isenabler(uintptr_t base,
				unsigned int interrupt_id, unsigned int val)
{
	unsigned int n = interrupt_id >> ISENABLER_SHIFT;
	mmio_write_32(base + GICD_ISENABLER + (n << 2), val);
}

void gicd_write_icenabler(uintptr_t base,
				unsigned int interrupt_id, unsigned int val)
{
	unsigned int n = interrupt_id >> ICENABLER_SHIFT;
	mmio_write_32(base + GICD_ICENABLER + (n << 2), val);
}

void gicd_write_ispendr(uintptr_t base,
				unsigned int interrupt_id, unsigned int val)
{
	unsigned int n = interrupt_id >> ISPENDR_SHIFT;
	mmio_write_32(base + GICD_ISPENDR + (n << 2), val);
}

void gicd_write_icpendr(uintptr_t base,
				unsigned int interrupt_id, unsigned int val)
{
	unsigned int n = interrupt_id >> ICPENDR_SHIFT;
	mmio_write_32(base + GICD_ICPENDR + (n << 2), val);
}

void gicd_write_isactiver(uintptr_t base,
				unsigned int interrupt_id, unsigned int val)
{
	unsigned int n = interrupt_id >> ISACTIVER_SHIFT;
	mmio_write_32(base + GICD_ISACTIVER + (n << 2), val);
}

void gicd_write_icactiver(uintptr_t base,
				unsigned int interrupt_id, unsigned int val)
{
	unsigned int n = interrupt_id >> ICACTIVER_SHIFT;
	mmio_write_32(base + GICD_ICACTIVER + (n << 2), val);
}

void gicd_write_ipriorityr(uintptr_t base,
				unsigned int interrupt_id, unsigned int val)
{
	unsigned int n = interrupt_id >> IPRIORITYR_SHIFT;
	mmio_write_32(base + GICD_IPRIORITYR + (n << 2), val);
}

void gicd_write_icfgr(uintptr_t base,
				unsigned int interrupt_id, unsigned int val)
{
	unsigned int n = interrupt_id >> ICFGR_SHIFT;
	mmio_write_32(base + GICD_ICFGR + (n << 2), val);
}

/*******************************************************************************
 * GIC Distributor interface accessors for individual interrupt manipulation
 ******************************************************************************/
unsigned int gicd_get_isenabler(uintptr_t base, unsigned int interrupt_id)
{
	unsigned int bit_num = interrupt_id & ((1 << ISENABLER_SHIFT) - 1);

	return gicd_read_isenabler(base, interrupt_id) & (1 << bit_num);
}

void gicd_set_isenabler(uintptr_t base, unsigned int interrupt_id)
{
	unsigned int bit_num = interrupt_id & ((1 << ISENABLER_SHIFT) - 1);

	gicd_write_isenabler(base, interrupt_id, (1 << bit_num));
}

void gicd_set_icenabler(uintptr_t base, unsigned int interrupt_id)
{
	unsigned int bit_num = interrupt_id & ((1 << ICENABLER_SHIFT) - 1);

	gicd_write_icenabler(base, interrupt_id, (1 << bit_num));
}

void gicd_set_ispendr(uintptr_t base, unsigned int interrupt_id)
{
	unsigned int bit_num = interrupt_id & ((1 << ISPENDR_SHIFT) - 1);

	gicd_write_ispendr(base, interrupt_id, (1 << bit_num));
}

void gicd_set_icpendr(uintptr_t base, unsigned int interrupt_id)
{
	unsigned int bit_num = interrupt_id & ((1 << ICPENDR_SHIFT) - 1);

	gicd_write_icpendr(base, interrupt_id, (1 << bit_num));
}

void gicd_set_isactiver(uintptr_t base, unsigned int interrupt_id)
{
	unsigned int bit_num = interrupt_id & ((1 << ISACTIVER_SHIFT) - 1);

	gicd_write_isactiver(base, interrupt_id, (1 << bit_num));
}

void gicd_set_icactiver(uintptr_t base, unsigned int interrupt_id)
{
	unsigned int bit_num = interrupt_id & ((1 << ICACTIVER_SHIFT) - 1);

	gicd_write_icactiver(base, interrupt_id, (1 << bit_num));
}

unsigned int gicd_get_ipriorityr(uintptr_t base, unsigned int interrupt_id)
{
	return gicd_read_ipriorityr(base, interrupt_id) & GIC_PRI_MASK;
}

void gicd_set_ipriorityr(uintptr_t base, unsigned int interrupt_id,
				unsigned int priority)
{
	mmio_write_8(base + GICD_IPRIORITYR + interrupt_id,
			priority & GIC_PRI_MASK);
}

bool is_gicv3_mode(void)
{
	/* Check if GICv3 system register available */
	if (!is_feat_gic_supported())
		return 0;

	/* Check whether the system register interface is enabled */
	return !!is_sre_enabled();
}
