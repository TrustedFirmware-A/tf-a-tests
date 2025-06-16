/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <arch.h>
#include <arch_features.h>
#include <arch_helpers.h>
#include <drivers/arm/gic_v5.h>
#include <platform.h>

#include <platform_def.h>

/*
 * Data structure to store the GIC per CPU context before entering
 * a suspend to powerdown (with loss of context).
 */
struct gicv5_pcpu_ctx {
	/* CPU IF registers. Only those currently in use */
	u_register_t icc_cr0_el1;
	/* PPI registers. We don't touch handling mode. */
	u_register_t icc_ppi_enabler[2];
	u_register_t icc_ppi_xpendr[2];
	u_register_t icc_ppi_priorityr[16];
};

static uintptr_t irs_base;
static struct gicv5_pcpu_ctx cpu_ctx[PLATFORM_CORE_COUNT];

/* CPU MPIDR != GICv5 IAFFID. This holds the mapping, initialised on CPU_ON. */
static uint16_t iaffids[PLATFORM_CORE_COUNT];

/* the IST is a power of 2 since that's what goes in IRS_IST_CFGR.LPI_ID_BITS */
struct l2_iste ist[next_power_of_2(PLATFORM_CORE_COUNT) * IRQ_NUM_SGIS];

static inline uint8_t log2(uint32_t num)
{
	return (31 - __builtin_clz(num));
}

static inline bool is_interrupt(unsigned int interrupt_id)
{
	unsigned int_type = EXTRACT(INT_TYPE, interrupt_id);

	return int_type == INT_PPI || int_type == INT_LPI || int_type == INT_SPI;
}

static inline u_register_t read_icc_ppi_priorityrn(unsigned n)
{
	switch (n) {
	case 0:
		return read_icc_ppi_priorityr0();
	case 1:
		return read_icc_ppi_priorityr1();
	case 2:
		return read_icc_ppi_priorityr2();
	case 3:
		return read_icc_ppi_priorityr3();
	case 4:
		return read_icc_ppi_priorityr4();
	case 5:
		return read_icc_ppi_priorityr5();
	case 6:
		return read_icc_ppi_priorityr6();
	case 7:
		return read_icc_ppi_priorityr7();
	case 8:
		return read_icc_ppi_priorityr8();
	case 9:
		return read_icc_ppi_priorityr9();
	case 10:
		return read_icc_ppi_priorityr10();
	case 11:
		return read_icc_ppi_priorityr11();
	case 12:
		return read_icc_ppi_priorityr12();
	case 13:
		return read_icc_ppi_priorityr13();
	case 14:
		return read_icc_ppi_priorityr14();
	case 15:
		return read_icc_ppi_priorityr15();
	default:
		panic();
	}
}

static inline void write_icc_ppi_priorityrn(unsigned n, u_register_t val)
{
	switch (n) {
	case 0:
		return write_icc_ppi_priorityr0(val);
	case 1:
		return write_icc_ppi_priorityr1(val);
	case 2:
		return write_icc_ppi_priorityr2(val);
	case 3:
		return write_icc_ppi_priorityr3(val);
	case 4:
		return write_icc_ppi_priorityr4(val);
	case 5:
		return write_icc_ppi_priorityr5(val);
	case 6:
		return write_icc_ppi_priorityr6(val);
	case 7:
		return write_icc_ppi_priorityr7(val);
	case 8:
		return write_icc_ppi_priorityr8(val);
	case 9:
		return write_icc_ppi_priorityr9(val);
	case 10:
		return write_icc_ppi_priorityr10(val);
	case 11:
		return write_icc_ppi_priorityr11(val);
	case 12:
		return write_icc_ppi_priorityr12(val);
	case 13:
		return write_icc_ppi_priorityr13(val);
	case 14:
		return write_icc_ppi_priorityr14(val);
	case 15:
		return write_icc_ppi_priorityr15(val);
	default:
		panic();
	}
}

bool is_gicv5_mode(void)
{
	return is_feat_gcie_supported();
}

bool gicv5_is_irq_spi(unsigned int irq_num)
{
	return EXTRACT(INT_TYPE, irq_num) == INT_SPI;
}

void gicv5_enable_cpuif(void)
{
	write_icc_cr0_el1(read_icc_cr0_el1() | ICC_CR0_EL1_EN_BIT);
	/* make sure effects are visible */
	isb();
}

void gicv5_setup_cpuif(void)
{
	iaffids[platform_get_core_pos(read_mpidr_el1())] = read_icc_iaffidr_el1();

	write_icc_pcr_el1(GICV5_IDLE_PRIORITY);

	gicv5_enable_cpuif();
}

void gicv5_disable_cpuif(void)
{
	write_icc_cr0_el1(read_icc_cr0_el1() & ~ICC_CR0_EL1_EN_BIT);
	/* make sure effects are visible */
	isb();
}

void gicv5_save_cpuif_context(void)
{
	unsigned int core_pos = platform_get_core_pos(read_mpidr_el1());

	cpu_ctx[core_pos].icc_cr0_el1 = read_icc_cr0_el1();
	cpu_ctx[core_pos].icc_ppi_enabler[0] = read_icc_ppi_enabler0();
	cpu_ctx[core_pos].icc_ppi_enabler[1] = read_icc_ppi_enabler1();

	cpu_ctx[core_pos].icc_ppi_xpendr[0] = read_icc_ppi_spendr0();
	cpu_ctx[core_pos].icc_ppi_xpendr[1] = read_icc_ppi_spendr1();

	for (int i = 0; i < 15; i++) {
		cpu_ctx[core_pos].icc_ppi_priorityr[i] = read_icc_ppi_priorityrn(i);
	}

	/* Make sure all PPIs are inactive, i.e. not suspending mid interrupt */
	assert(read_icc_ppi_sactiver0() == 0UL && read_icc_ppi_sactiver1() == 0UL);
}

void gicv5_restore_cpuif_context(void)
{
	unsigned int core_pos = platform_get_core_pos(read_mpidr_el1());

	write_icc_ppi_enabler0(cpu_ctx[core_pos].icc_ppi_enabler[0]);
	write_icc_ppi_enabler1(cpu_ctx[core_pos].icc_ppi_enabler[1]);

	write_icc_ppi_spendr0(cpu_ctx[core_pos].icc_ppi_xpendr[0]);
	write_icc_ppi_spendr1(cpu_ctx[core_pos].icc_ppi_xpendr[1]);
	/* clear interrupts that shouldn't be pending */
	write_icc_ppi_cpendr0(~cpu_ctx[core_pos].icc_ppi_xpendr[0]);
	write_icc_ppi_cpendr1(~cpu_ctx[core_pos].icc_ppi_xpendr[1]);

	for (int i = 0; i < 15; i++) {
		write_icc_ppi_priorityrn(i, cpu_ctx[core_pos].icc_ppi_priorityr[i]);
	}

	/* don't bother saving it, just put the same value in */
	write_icc_pcr_el1(GICV5_IDLE_PRIORITY);

	write_icc_cr0_el1(cpu_ctx[core_pos].icc_cr0_el1);
	/* make sure effects are visible */
	isb();
}

void gicv5_set_priority(unsigned int interrupt_id, unsigned int priority)
{
	unsigned irq_idx, irq_reg;
	u_register_t priorityr;

	assert(priority < (1UL << GICCDPRI_PRIORITY_WIDTH));
	assert(is_interrupt(interrupt_id));

	if (EXTRACT(INT_TYPE, interrupt_id) != INT_PPI) {
		giccdpri(interrupt_id | INPLACE(GICCDPRI_PRIORITY, priority));
		return;
	}

	/* it's a PPI, get rid of the INTR TYPE field */
	interrupt_id = EXTRACT(INT_ID, interrupt_id);
	irq_reg = interrupt_id / ICC_PPI_PRIORITYR_FIELD_NUM;
	irq_idx = interrupt_id % ICC_PPI_PRIORITYR_FIELD_NUM;

	priorityr = read_icc_ppi_priorityrn(irq_reg) &
		    ICC_PPI_PRIORITYR_FIELD_MASK <<
		    (irq_idx * ICC_PPI_PRIORITYR_FIELD_NUM);
	write_icc_ppi_priorityrn(irq_reg, priorityr |
			(priority << (irq_idx * ICC_PPI_PRIORITYR_FIELD_NUM)));
}

void gicv5_send_sgi(unsigned int sgi_id, unsigned int core_pos)
{
	giccdpend(gicv5_get_sgi_num(sgi_id, core_pos) | GICCDPEND_PENDING_BIT);
}

void gicv5_set_intr_route(unsigned int interrupt_id, unsigned int core_pos)
{
	assert(is_interrupt(interrupt_id));

	/* PPIs are local to the CPU, can't be rerouted */
	if (EXTRACT(INT_TYPE, interrupt_id) == INT_PPI) {
		return;
	}

	/*
	 * The expecation is that a core will be up (CPU_ON) before it gets
	 * targetted by interrupts. Otherwise the IAFFID isn't available yet
	 * and the interrupt will be misrouted.
	 */
	assert(iaffids[core_pos] != 0 || core_pos == 0);
	giccdaff(INPLACE(GICCDAFF_IAFFID, iaffids[core_pos]) | interrupt_id);

	/* wait for the target to take effect so retargetting an already
	 * enabled interrupt ends up in the correct destination */
	gsbsys();
}

void gicv5_intr_enable(unsigned int interrupt_id)
{
	unsigned int irq_idx;

	assert(is_interrupt(interrupt_id));

	if (EXTRACT(INT_TYPE, interrupt_id) != INT_PPI) {
		giccden(interrupt_id);
		return;
	}

	/* it's a PPI, get rid of the INTR TYPE field */
	interrupt_id = EXTRACT(INT_ID, interrupt_id);
	irq_idx = interrupt_id % ICC_PPI_ENABLER_FIELD_NUM;

	if (interrupt_id / ICC_PPI_ENABLER_FIELD_NUM == 0) {
		write_icc_ppi_enabler0(read_icc_ppi_enabler0() | (1UL << irq_idx));
	} else {
		write_icc_ppi_enabler1(read_icc_ppi_enabler1() | (1UL << irq_idx));
	}
}

void gicv5_intr_disable(unsigned int interrupt_id)
{
	unsigned int irq_idx;

	assert(is_interrupt(interrupt_id));

	if (EXTRACT(INT_TYPE, interrupt_id) != INT_PPI) {
		giccddis(interrupt_id);
		/* wait for the interrupt to become disabled */
		gsbsys();
		return;
	}

	/* it's a PPI, get rid of the INTR TYPE field */
	interrupt_id = EXTRACT(INT_ID, interrupt_id);
	irq_idx = interrupt_id % ICC_PPI_ENABLER_FIELD_NUM;

	if (interrupt_id / ICC_PPI_ENABLER_FIELD_NUM == 0) {
		write_icc_ppi_enabler0(read_icc_ppi_enabler0() & ~(1UL << irq_idx));
	} else {
		write_icc_ppi_enabler1(read_icc_ppi_enabler1() & ~(1UL << irq_idx));
	}
}

unsigned int gicv5_acknowledge_interrupt(void)
{
	u_register_t iar = gicrcdia();
	assert((iar & GICRCDIA_VALID_BIT) != 0);

	/* wait for the intr ack to complete (i.e. make it Active) and refetch
	 * instructions so they don't operate on anything stale */
	gsback();
	isb();

	return iar & ~GICRCDIA_VALID_BIT;
}

unsigned int gicv5_is_intr_pending(unsigned int interrupt_id)
{
	u_register_t icsr;
	u_register_t ppi_spendr;

	assert(is_interrupt(interrupt_id));

	if (EXTRACT(INT_TYPE, interrupt_id) != INT_PPI) {
		/* request interrupt information */
		giccdrcfg(interrupt_id);
		/* wait for the register to update */
		isb();
		icsr = read_icc_icsr_el1();

		/* interrupt is unreachable, something has gone wrong */
		assert((icsr & ICC_ICSR_EL1_F_BIT) == 0);
		return !!(icsr & ICC_ICSR_EL1_PENDING_BIT);
	}

	/* it's a PPI, get rid of the INTR TYPE field */
	interrupt_id = EXTRACT(INT_ID, interrupt_id);

	if (interrupt_id / ICC_PPI_XPENDR_FIELD_NUM == 0) {
		ppi_spendr = read_icc_ppi_spendr0();
	} else {
		ppi_spendr = read_icc_ppi_spendr1();
	}

	return !!(ppi_spendr & BIT(interrupt_id % ICC_PPI_XPENDR_FIELD_NUM));
}

void gicv5_intr_clear(unsigned int interrupt_id)
{
	unsigned int irq_idx;

	assert(is_interrupt(interrupt_id));

	if (EXTRACT(INT_TYPE, interrupt_id) != INT_PPI) {
		giccdpend(interrupt_id);
		return;
	}

	/* it's a PPI, get rid of the INTR TYPE field */
	interrupt_id = EXTRACT(INT_ID, interrupt_id);
	irq_idx = interrupt_id % ICC_PPI_XPENDR_FIELD_NUM;

	if (interrupt_id / ICC_PPI_XPENDR_FIELD_NUM == 0) {
		write_icc_ppi_cpendr0(read_icc_ppi_cpendr0() | (1UL << irq_idx));
	} else {
		write_icc_ppi_cpendr1(read_icc_ppi_cpendr1() | (1UL << irq_idx));
	}
}

void gicv5_end_of_interrupt(unsigned int raw_iar)
{
	giccddi(raw_iar);
	giccdeoi();
	/* no isb as we won't interact with the gic before the eret */
}

/* currently a single IRS is expected and an ITS/IWB are not used */
void gicv5_setup(void)
{
#if ENABLE_ASSERTIONS
	uint32_t irs_idr2 = read_IRS_IDR2(irs_base);
#endif
	uint8_t id_bits = log2(ARRAY_SIZE(ist));
	/* min_id_bits <= log2(length(ist)) <= id_bits */
	assert(EXTRACT(IRS_IDR2_MIN_LPI_ID_BITS, irs_idr2) <= id_bits);
	assert(EXTRACT(IRS_IDR2_ID_BITS, irs_idr2) >= id_bits);

	/* make sure all ISTEs aren't enabled */
	memset(ist, 0x0, sizeof(ist));

	/* write zeroes throughout except LPI_ID_Bits which is the lowest 5 bits */
	write_IRS_IST_CFGR(irs_base, id_bits);
	/* make the IST valid */
	write_IRS_IST_BASER(irs_base,
			    (((uintptr_t) &ist) & MASK(IRS_IST_BASER_ADDR)) |
			    IRS_IST_BASER_VALID_BIT);
	WAIT_FOR_IDLE(irs_base, IRS_IST_STATUSR);

	/* enable the IRS */
	write_IRS_CR0(irs_base, IRS_CR0_IRSEN_BIT);
	WAIT_FOR_IDLE(irs_base, IRS_CR0);
}

uint32_t gicv5_get_sgi_num(uint32_t index, unsigned int core_pos)
{
	assert(index <= IRQ_NUM_SGIS);

	return (core_pos * IRQ_NUM_SGIS + index) | INPLACE(INT_TYPE, INT_LPI);
}

void gicv5_init(uintptr_t irs_base_addr)
{
	irs_base = irs_base_addr;
}
