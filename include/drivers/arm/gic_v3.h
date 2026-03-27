/*
 * Copyright (c) 2018-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __GIC_V3_H__
#define __GIC_V3_H__

/***************************************************************************
 * Defines and prototypes specific to GIC v3.
 *************************************************************************/

/* GICD register offsets */
#define GICD_IROUTER		0x6000

/* GICD_CTLR bit definitions */
#define GICD_CTLR_ENABLE_GRP1A		(1 << 1)
#define GICD_CTLR_ARE_NS_SHIFT		4
#define GICD_CTLR_ARE_NS_MASK		0x1

/* GICR_TYPER bit definitions */
#define TYPER_AFF_VAL_SHIFT	32
#define TYPER_PROC_NUM_SHIFT	8
#define TYPER_LAST_SHIFT	4

#define TYPER_AFF_VAL_MASK	0xffffffff
#define TYPER_PROC_NUM_MASK	0xffff
#define TYPER_LAST_MASK		0x1

#define TYPER_LAST_BIT		(1 << TYPER_LAST_SHIFT)

/* GICD_IROUTER shifts and masks */
#define IROUTER_IRM_SHIFT	31
#define IROUTER_IRM_MASK	0x1

/* GICD_TYPER shifts and masks */
#define	TYPER_ESPI		U(1 << 8)
#define	TYPER_DVIS		U(1 << 18)
#define	TYPER_ESPI_RANGE_MASK	U(0x1f)
#define	TYPER_ESPI_RANGE_SHIFT	U(27)
#define	TYPER_ESPI_RANGE	U(TYPER_ESPI_MASK << TYPER_ESPI_SHIFT)

/*******************************************************************************
 * GICv3 Re-distributor interface registers & constants
 ******************************************************************************/
#define GICR_PCPUBASE_SHIFT	0x11
#define GICR_SGIBASE_OFFSET	(1 << 0x10)	/* 64 KB */
#define GICR_CTLR		0x0
#define GICR_TYPER		0x08
#define GICR_WAKER		0x14
#define GICR_IGROUPR0		(GICR_SGIBASE_OFFSET + 0x80)
#define GICR_ISENABLER0		(GICR_SGIBASE_OFFSET + 0x100)
#define GICR_ICENABLER0		(GICR_SGIBASE_OFFSET + 0x180)
#define GICR_ISPENDR0		(GICR_SGIBASE_OFFSET + 0x200)
#define GICR_ICPENDR0		(GICR_SGIBASE_OFFSET + 0x280)
#define GICR_IPRIORITYR		(GICR_SGIBASE_OFFSET + 0x400)
#define GICR_ICFGR0		(GICR_SGIBASE_OFFSET + 0xc00)
#define GICR_ICFGR1		(GICR_SGIBASE_OFFSET + 0xc04)
#define GICR_IGRPMODR0		(GICR_SGIBASE_OFFSET + 0xd00)

/*******************************************************************************
 * GICv3 CPU interface registers & constants
 ******************************************************************************/
/* ICC_SRE bit definitions*/
#define ICC_SRE_EN_BIT		(1 << 3)
#define ICC_SRE_DIB_BIT		(1 << 2)
#define ICC_SRE_DFB_BIT		(1 << 1)
#define ICC_SRE_SRE_BIT		(1 << 0)

/* ICC_IAR1_EL1 bit definitions */
#define IAR1_EL1_INTID_SHIFT		0
#define IAR1_EL1_INTID_MASK		0xffffff

/* ICC_SGI1R bit definitions */
#define SGI1R_TARGET_LIST_MASK		0xffff
#define SGI1R_TARGET_LIST_SHIFT		0x0
#define SGI1R_AFF_MASK			0xff
#define SGI1R_AFF1_SHIFT		16ULL
#define SGI1R_AFF2_SHIFT		32ULL
#ifdef __aarch64__
#define SGI1R_AFF3_SHIFT		48ULL
#endif
#define SGI1R_INTID_MASK		0xf
#define SGI1R_INTID_SHIFT		24
#define SGI1R_IRM_MASK			0x1
#define SGI1R_IRM_SHIFT			0x40

/* ICC_IGRPEN1_EL1 bit definitions */
#define IGRPEN1_EL1_ENABLE_SHIFT	0
#define IGRPEN1_EL1_ENABLE_BIT		(1 << IGRPEN1_EL1_ENABLE_SHIFT)

/* ICH_ICH_LR<n>_EL2 definitions */
/*
 * When ICH_LR<n>_EL2.HW is 0 (there is no corresponding physical
 * interrupt), this field has the following meaning:
 * Bits[44:42] : RES0.
 * Bit[41] : EOI. If this bit is 1, then when the interrupt identified by
 * vINTID is deactivated, a maintenance interrupt is asserted.
 * Bits[40:32] : RES0.
 */
#define ICH_LRn_EL2_EOI				(1UL << 41)
#define ICH_LRn_EL2_STATE_Invalid		(0UL << 62)
#define ICH_LRn_EL2_STATE_Pending		(1UL << 62)
#define ICH_LRn_EL2_STATE_Active		(2UL << 62)
#define ICH_LRn_EL2_STATE_Pending_Active	(3UL << 62)
#define ICH_LRn_EL2_Group_0			(0UL << 60)
#define ICH_LRn_EL2_Group_1			(1UL << 60)
#define ICH_LRn_EL2_Priority_SHIFT		48
#define ICH_LRn_EL2_Priority_MASK		0xFF
#define ICH_LRn_EL2_vINTID_SHIFT		0
#define ICH_LRn_EL2_vINTID_MASK			0xFFFF

/* Global enable bit for the virtual CPU interface */
#define ICH_HCR_EL2_EN_BIT              (UL(1) << 0)

/* Underflow Interrupt Enable */
#define ICH_HCR_EL2_UIE_BIT             (UL(1) << 1)

/* List Register Entry Not Present Interrupt Enable */
#define ICH_HCR_EL2_LRENPIE_BIT         (UL(1) << 2)

/* No Pending Interrupt Enable */
#define ICH_HCR_EL2_NPIE_BIT            (UL(1) << 3)

/* VM Group 0 Enabled Interrupt Enable */
#define ICH_HCR_EL2_VGRP0EIE_BIT        (UL(1) << 4)

/* VM Group 0 Disabled Interrupt Enable */
#define ICH_HCR_EL2_VGRP0DIE_BIT        (UL(1) << 5)

/* VM Group 1 Enabled Interrupt Enable */
#define ICH_HCR_EL2_VGRP1EIE_BIT        (UL(1) << 6)

/* VM Group 1 Disabled Interrupt Enable */
#define ICH_HCR_EL2_VGRP1DIE_BIT        (UL(1) << 7)

/*
 * When FEAT_GICv4p1 is implemented:
 * Controls whether deactivation of virtual SGIs
 * can increment ICH_HCR_EL2.EOIcount
 */
#define ICH_HCR_EL2_VSGIEEOICOUNT_BIT   (UL(1) << 8)

/*
 * Trap all EL1 accesses to System registers
 * that are common to Group 0 and Group 1 to EL2
 */
#define ICH_HCR_EL2_TC_BIT              (UL(1) << 10)

/*
 * Trap all EL1 accesses to ICC_* and ICV_* System registers
 * for Group 0 interrupts to EL2
 */
#define ICH_HCR_EL2_TALL0_BIT           (UL(1) << 11)

/*
 * Trap all EL1 accesses to ICC_* and ICV_* System registers
 * for Group 1 interrupts to EL2
 */
#define ICH_HCR_EL2_TALL1_BIT           (UL(1) << 12)

/* Trap all locally generated SEIs */
#define ICH_HCR_EL2_TSEI_BIT            (UL(1) << 13)

/*
 * When FEAT_GICv3_TDIR is implemented:
 * Trap EL1 writes to ICC_DIR_EL1 and ICV_DIR_EL1
 */
#define ICH_HCR_EL2_TDIR_BIT            (UL(1) << 14)

/*
 * When ICH_VTR_EL2.DVIM == 1:
 * Directly-injected Virtual Interrupt Mask
 */
#define ICH_HCR_EL2_DVIM_BIT            (UL(1) << 15)

#define ICH_HCR_EL2_EOI_COUNT_SHIFT     UL(27)
#define ICH_HCR_EL2_EOI_COUNT_WIDTH     UL(5)
#define ICH_HCR_EL2_EOI_COUNT_MASK      MASK(ICH_HCR_EL2_EOI_COUNT)

/* ICH_MISR_EL2 definitions */
/*
 * Bit[0] : EOI. End of Interrupt maintenance interrupt.
 * Set to 1 when at least one bit in ICH_LR<n>_EL2.EOI is 1 and the
 * corresponding interrupt has been deactivated.
 */
#define ICH_MISR_EL2_EOI			(1UL << 0)

/* ICV_CTLR_EL1 definitions */
#define ICV_CTLR_EL1_PRIbits_SHIFT		8
#define ICV_CTLR_EL1_PRIbits_MASK		7

/* ICV_IGRPEN1_EL1 definition */
#define ICV_IGRPEN1_EL1_Enable			1UL

/* The highest affinity 0 that can be a SGI target*/
#define SGI_TARGET_MAX_AFF0		16

#ifndef ASSEMBLY

/*******************************************************************************
 * Helper GICv3 macros
 ******************************************************************************/
#define gicv3_acknowledge_interrupt()		read_icc_iar1_el1() &\
							IAR1_EL1_INTID_MASK
#define gicv3_end_of_interrupt(id)		write_icc_eoir1_el1(id)

#define is_sre_enabled()	\
	(IS_IN_EL2() ? (read_icc_sre_el2() & ICC_SRE_SRE_BIT) :\
	(read_icc_sre_el1() & ICC_SRE_SRE_BIT))

/******************************************************************************
 * GICv3 public driver API
 *****************************************************************************/
 /*
  * Initialize the GICv3 driver. The base addresses of GIC Re-distributor
  * interface `gicr_base` and the Distributor interface `gicd_base` must
  * be provided as arguments.
  */
void gicv3_init(uintptr_t gicr_base, uintptr_t gicd_base);

/*
 * Setup the GIC Distributor interface.
 */
void gicv3_setup_distif(void);

/*
 * Probe the Re-distributor base corresponding to this core.
 * This function is required to be invoked on successful boot of a core.
 * The base address will be stored internally by the driver and will be
 * used when accessing the Re-distributor interface.
 */
void gicv3_probe_redistif_addr(void);

/*
 * Set the bit corresponding to `interrupt_id` in the ICPENDR register
 * at either Distributor or Re-distributor depending on the interrupt.
 */
void gicv3_set_icpendr(unsigned int interrupt_id);

/*
 * Get the bit corresponding to `interrupt_id` in the ISPENDR register
 * at either Distributor or Re-distributor depending on the interrupt.
 */
unsigned int gicv3_get_ispendr(unsigned int interrupt_id);

/*
 * Set the bit corresponding to `interrupt_id` in the ICENABLER register
 * at either Distributor or Re-distributor depending on the interrupt.
 */
void gicv3_set_icenabler(unsigned int interrupt_id);

/*
 * Get the bit corresponding to `interrupt_id` in the ISENABLER register
 * at either Distributor or Re-distributor depending on the interrupt.
 */
unsigned int gicv3_get_isenabler(unsigned int interrupt_id);

/*
 * Set the bit corresponding to `interrupt_id` in the ISENABLER register
 * at either Distributor or Re-distributor depending on the interrupt.
 */
void gicv3_set_isenabler(unsigned int interrupt_id);

/*
 * Set the `route` corresponding to `interrupt_id` in the IROUTER register
 * at Distributor.
 */
void gicv3_set_intr_route(unsigned int interrupt_id, unsigned int core_pos);

/*
 * Send SGI with ID `sgi_id` to core with index `core_pos`.
 */
void gicv3_send_sgi(unsigned int sgi_id, unsigned int core_pos);

/*
 * Get the priority of the interrupt `interrupt_id`.
 */
unsigned int gicv3_get_ipriorityr(unsigned int interrupt_id);

/*
 * Set the priority of the interrupt `interrupt_id` to `priority`.
 */
void gicv3_set_ipriorityr(unsigned int interrupt_id, unsigned int priority);

/*
 * Restore the GICv3 SGI and PPI context after powering up the
 * GIC Re-distributor.
 */
void gicv3_restore_sgi_ppi_context(void);

/*
 * Save the GICv3 SGI and PPI context prior to powering down the
 * GIC Re-distributor.
 */
void gicv3_save_sgi_ppi_context(void);

/*
 * Restore the GICv3 CPU interface after powering up the CPU interface.
 */
void gicv3_restore_cpuif_context(void);

/*
 * Save the GICv3 CPU interface prior to powering down the CPU interface.
 */
void gicv3_save_cpuif_context(void);

/*
 * Disable the GIC CPU interface.
 */
void gicv3_disable_cpuif(void);

/*
 * Setup the GIC CPU interface.
 */
void gicv3_setup_cpuif(void);

/*
 * Enable the GIC CPU interface.
 */
void gicv3_enable_cpuif(void);

/*
 * Return the value of GICD_TYPER.
 */
unsigned int gicv3_get_gicd_typer(void);

#endif /*__ASSEMBLY__*/
#endif /* __GIC_V3_H__ */
