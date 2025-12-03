/*
 * Copyright (c) 2018-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amu.h>
#include <arch.h>
#include <arch_features.h>
#include <arch_helpers.h>
#include <assert.h>
#include <utils_def.h>

/*
 * Return the number of counters available for group 1. Architecture is forward
 * compatible
 */
uint64_t amu_group1_num_counters(void)
{
	return EXTRACT(AMCGCR_EL0_CG1NC, read_amcgcr_el0());
}

uint64_t read_amevcntr0(unsigned int idx)
{
	assert(idx < AMU_GROUP0_NR_COUNTERS);

	switch (idx) {
	case 0x0:
		return read_amevcntr00_el0();
	case 0x1:
		return read_amevcntr01_el0();
	case 0x2:
		return read_amevcntr02_el0();
	case 0x3:
		return read_amevcntr03_el0();
	default:
		return 0ULL;
	}
}

uint64_t read_amevcntr1(unsigned int idx)
{
	assert(idx < amu_group1_num_counters());

	switch (idx) {
	case 0x0:
		return read_amevcntr10_el0();
	case 0x1:
		return read_amevcntr11_el0();
	case 0x2:
		return read_amevcntr12_el0();
	case 0x3:
		return read_amevcntr13_el0();
	case 0x4:
		return read_amevcntr14_el0();
	case 0x5:
		return read_amevcntr15_el0();
	case 0x6:
		return read_amevcntr16_el0();
	case 0x7:
		return read_amevcntr17_el0();
	case 0x8:
		return read_amevcntr18_el0();
	case 0x9:
		return read_amevcntr19_el0();
	case 0xa:
		return read_amevcntr1a_el0();
	case 0xb:
		return read_amevcntr1b_el0();
	case 0xc:
		return read_amevcntr1c_el0();
	case 0xd:
		return read_amevcntr1d_el0();
	case 0xe:
		return read_amevcntr1e_el0();
	case 0xf:
		return read_amevcntr1f_el0();
	default:
		return 0ULL;
	}
}

uint64_t read_amevcntvoff0(unsigned int idx)
{
	assert(idx < AMU_GROUP0_NR_COUNTERS);

	switch (idx) {
	case 0:
		return read_amevcntvoff00_el2();
	case 1:
		assert(0);
		return 0ULL;
	case 2:
		return read_amevcntvoff02_el2();
	case 3:
		return read_amevcntvoff03_el2();
	default:
		return 0ULL;
	}
}

void write_amevcntvoff0(unsigned int idx, uint64_t val)
{
	assert(idx < AMU_GROUP0_NR_COUNTERS);

	switch (idx) {
	case 0:
		write_amevcntvoff00_el2(val);
		break;
	case 1:
		assert(0);
		break; /* not defined */
	case 2:
		write_amevcntvoff02_el2(val);
		break;
	case 3:
		write_amevcntvoff03_el2(val);
		break;
	default:
		break;
	}
}

uint64_t read_amevcntvoff1(unsigned int idx)
{
	assert(idx < amu_group1_num_counters());

	switch (idx) {
	case 0x0:
		return read_amevcntvoff10_el2();
	case 0x1:
		return read_amevcntvoff11_el2();
	case 0x2:
		return read_amevcntvoff12_el2();
	case 0x3:
		return read_amevcntvoff13_el2();
	case 0x4:
		return read_amevcntvoff14_el2();
	case 0x5:
		return read_amevcntvoff15_el2();
	case 0x6:
		return read_amevcntvoff16_el2();
	case 0x7:
		return read_amevcntvoff17_el2();
	case 0x8:
		return read_amevcntvoff18_el2();
	case 0x9:
		return read_amevcntvoff19_el2();
	case 0xa:
		return read_amevcntvoff1a_el2();
	case 0xb:
		return read_amevcntvoff1b_el2();
	case 0xc:
		return read_amevcntvoff1c_el2();
	case 0xd:
		return read_amevcntvoff1d_el2();
	case 0xe:
		return read_amevcntvoff1e_el2();
	case 0xf:
		return read_amevcntvoff1f_el2();
	default:
		return 0ULL;
	}
}

void write_amevcntvoff1(unsigned int idx, uint64_t val)
{
	assert(idx < amu_group1_num_counters());

	switch (idx) {
	case 0x0:
		write_amevcntvoff10_el2(val);
		break;
	case 0x1:
		write_amevcntvoff11_el2(val);
		break;
	case 0x2:
		write_amevcntvoff12_el2(val);
		break;
	case 0x3:
		write_amevcntvoff13_el2(val);
		break;
	case 0x4:
		write_amevcntvoff14_el2(val);
		break;
	case 0x5:
		write_amevcntvoff15_el2(val);
		break;
	case 0x6:
		write_amevcntvoff16_el2(val);
		break;
	case 0x7:
		write_amevcntvoff17_el2(val);
		break;
	case 0x8:
		write_amevcntvoff18_el2(val);
		break;
	case 0x9:
		write_amevcntvoff19_el2(val);
		break;
	case 0xa:
		write_amevcntvoff1a_el2(val);
		break;
	case 0xb:
		write_amevcntvoff1b_el2(val);
		break;
	case 0xc:
		write_amevcntvoff1c_el2(val);
		break;
	case 0xd:
		write_amevcntvoff1d_el2(val);
		break;
	case 0xe:
		write_amevcntvoff1e_el2(val);
		break;
	case 0xf:
		write_amevcntvoff1f_el2(val);
		break;
	default:
		break;
	}
}

uint64_t read_amevtyper1(unsigned int idx)
{
	assert(idx < amu_group1_num_counters());

	switch (idx) {
	case 0x0:
		return read_amevtyper10_el0();
	case 0x1:
		return read_amevtyper11_el0();
	case 0x2:
		return read_amevtyper12_el0();
	case 0x3:
		return read_amevtyper13_el0();
	case 0x4:
		return read_amevtyper14_el0();
	case 0x5:
		return read_amevtyper15_el0();
	case 0x6:
		return read_amevtyper16_el0();
	case 0x7:
		return read_amevtyper17_el0();
	case 0x8:
		return read_amevtyper18_el0();
	case 0x9:
		return read_amevtyper19_el0();
	case 0xa:
		return read_amevtyper1a_el0();
	case 0xb:
		return read_amevtyper1b_el0();
	case 0xc:
		return read_amevtyper1c_el0();
	case 0xd:
		return read_amevtyper1d_el0();
	case 0xe:
		return read_amevtyper1e_el0();
	case 0xf:
		return read_amevtyper1f_el0();
	default:
		return 0ULL;
	}
}

void write_amevtyper1(unsigned int idx, uint64_t val)
{
	assert(idx < amu_group1_num_counters());

	switch (idx) {
	case 0x0:
		write_amevtyper10_el0(val);
		break;
	case 0x1:
		write_amevtyper11_el0(val);
		break;
	case 0x2:
		write_amevtyper12_el0(val);
		break;
	case 0x3:
		write_amevtyper13_el0(val);
		break;
	case 0x4:
		write_amevtyper14_el0(val);
		break;
	case 0x5:
		write_amevtyper15_el0(val);
		break;
	case 0x6:
		write_amevtyper16_el0(val);
		break;
	case 0x7:
		write_amevtyper17_el0(val);
		break;
	case 0x8:
		write_amevtyper18_el0(val);
		break;
	case 0x9:
		write_amevtyper19_el0(val);
		break;
	case 0xa:
		write_amevtyper1a_el0(val);
		break;
	case 0xb:
		write_amevtyper1b_el0(val);
		break;
	case 0xc:
		write_amevtyper1c_el0(val);
		break;
	case 0xd:
		write_amevtyper1d_el0(val);
		break;
	case 0xe:
		write_amevtyper1e_el0(val);
		break;
	case 0xf:
		write_amevtyper1f_el0(val);
		break;
	default:
		break;
	}
}
