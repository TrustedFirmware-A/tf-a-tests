/*
 * Copyright (c) 2018-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <arch_features.h>
#include <lib/extensions/sve.h>
#include <stdint.h>
#include <smccc.h>
#include <tftf.h>
#include <utils_def.h>

static void sve_enable(void)
{
	if (IS_IN_EL2()) {
		write_cptr_el2(read_cptr_el2() & ~CPTR_EL2_TZ_BIT);
	} else {
		write_cpacr_el1(read_cpacr_el1() |
				CPACR_EL1_ZEN(CPACR_EL1_ZEN_TRAP_NONE));
	}

	isb();
}

static void sve_disable(void)
{
	if (IS_IN_EL2()) {
		write_cptr_el2(read_cptr_el2() | CPTR_EL2_TZ_BIT);
	} else {
		unsigned long val = read_cpacr_el1();

		val &= ~CPACR_EL1_ZEN(CPACR_EL1_ZEN_TRAP_NONE);
		val |= CPACR_EL1_ZEN(CPACR_EL1_ZEN_TRAP_ALL);
		write_cpacr_el1(val);
	}

	isb();
}

static bool is_sve_enabled(void)
{
	if (IS_IN_EL2()) {
		return ((read_cptr_el2() & CPTR_EL2_TZ_BIT) == 0UL);
	} else {
		return ((read_cpacr_el1() &
			 CPACR_EL1_ZEN(CPACR_EL1_ZEN_TRAP_NONE)) ==
			CPACR_EL1_ZEN(CPACR_EL1_ZEN_TRAP_NONE));
	}
}

/*
 * Use Trap control register SVE flags to represent SVE hint bit. On SVE capable
 * CPU, setting sve_hint_flag = true denotes absence of SVE (disables SVE), else
 * presence of SVE (enables SVE).
 */
void tftf_smc_set_sve_hint(bool sve_hint_flag)
{
	if (!is_armv8_2_sve_present()) {
		return;
	}

	if (sve_hint_flag) {
		sve_disable();
	} else {
		sve_enable();
	}
}

/*
 * On SVE capable CPU, return value of 'true' denotes SVE not used and return
 * value of 'false' denotes SVE used.
 *
 * If the CPU do not support SVE, always return 'false'.
 */
bool tftf_smc_get_sve_hint(void)
{
	if (is_armv8_2_sve_present()) {
		return is_sve_enabled() ? false : true;
	}

	return false;
}

smc_ret_values tftf_smc(const smc_args *args)
{
	uint32_t fid = args->fid;

	if (tftf_smc_get_sve_hint()) {
		fid |= MASK(FUNCID_SVE_HINT);
	} else {
		fid &= ~MASK(FUNCID_SVE_HINT);
	}

	return asm_tftf_smc64(fid,
			      args->arg1,
			      args->arg2,
			      args->arg3,
			      args->arg4,
			      args->arg5,
			      args->arg6,
			      args->arg7);
}
