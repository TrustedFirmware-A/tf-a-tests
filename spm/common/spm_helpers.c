/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <spm_helpers.h>

/*******************************************************************************
 * Hypervisor Calls Wrappers
 ******************************************************************************/

uint32_t spm_interrupt_get(void)
{
	hvc_args args = {
		.fid = SPM_INTERRUPT_GET
	};

	hvc_ret_values ret = tftf_hvc(&args);

	return ret.ret0;
}

/**
 * Hypervisor call to enable/disable SP delivery of a virtual interrupt of
 * int_id value through the IRQ or FIQ vector (pin).
 * Returns 0 on success, or -1 if passing an invalid interrupt id.
 */
int64_t spm_interrupt_enable(uint32_t int_id, bool enable, enum interrupt_pin pin)
{
	hvc_args args = {
		.fid = SPM_INTERRUPT_ENABLE,
		.arg1 = int_id,
		.arg2 = enable,
		.arg3 = pin
	};

	hvc_ret_values ret = tftf_hvc(&args);

	return (int64_t)ret.ret0;
}

/**
 * Hypervisor call to drop the priority and de-activate a secure interrupt.
 * Returns 0 on success, or -1 if passing an invalid interrupt id.
 */
int64_t spm_interrupt_deactivate(uint32_t vint_id)
{
	hvc_args args = {
		.fid = SPM_INTERRUPT_DEACTIVATE,
		.arg1 = vint_id, /* pint_id */
		.arg2 = vint_id
	};

	hvc_ret_values ret = tftf_hvc(&args);

	return (int64_t)ret.ret0;
}

/**
 * Return vCPU index for the currently running vCPU.
 * Virtual MPIDR holds the linear vCPU index information in lower bits.
 * Keep only first 24 bits (mapping to Aff0/Aff1/Aff2).
 * Omit Aff3, bit [31], U[30], MT[24].
 */
unsigned int spm_get_my_core_pos(void)
{
	uint64_t mpidr = read_mpidr_el1();

	return (unsigned int)(mpidr & 0xffffff);
}
