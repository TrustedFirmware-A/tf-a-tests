/*
 * Copyright (c) 2018-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <arch_features.h>
#include <drivers/console.h>
#include <lib/extensions/sve.h>
#include <smccc.h>
#include <stdint.h>
#include <stdio.h>
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

#ifdef SMC_FUZZ_VARIABLE_COVERAGE
	tftf_switch_console_state(CONSOLE_FLAG_SMC_FUZZER);
	printf("SMC FUZZER CALL fid:%x arg1:%lx arg2:%lx arg3:%lx arg4:%lx arg5:%lx arg6:%lx arg7:%lx\n",
	fid, args->arg1, args->arg2, args->arg3, args->arg4, args->arg5, args->arg6, args->arg7);
	tftf_switch_console_state(CONSOLE_FLAG_PLAT_UART);
#endif
	return asm_tftf_smc64(fid,
			args->arg1,
			args->arg2,
			args->arg3,
			args->arg4,
			args->arg5,
			args->arg6,
			args->arg7);
}

void tftf_smc_no_retval_x8(const smc_args_ext *args, smc_ret_values_ext *ret)
{
	uint32_t fid = args->fid;
	/* Copy args into new structure so the fid field can be modified */
	smc_args_ext args_copy = *args;

	if (tftf_smc_get_sve_hint()) {
		fid |= MASK(FUNCID_SVE_HINT);
	} else {
		fid &= ~MASK(FUNCID_SVE_HINT);
	}
	args_copy.fid = fid;

	asm_tftf_smc64_no_retval_x8(&args_copy, ret);
}
