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

uint64_t amu_group1_num_counters(void)
{
	return EXTRACT(AMCGCR_CG1NC, read_amcgcr());
}

uint64_t read_amevcntr0(unsigned int idx)
{
	assert(idx < AMU_GROUP0_NR_COUNTERS);

	switch (idx) {
	case 0x0:
		return read64_amevcntr00();
	case 0x1:
		return read64_amevcntr01();
	case 0x2:
		return read64_amevcntr02();
	case 0x3:
		return read64_amevcntr03();
	default:
		return 0ULL;
	}
}

uint64_t read_amevcntr1(unsigned int idx)
{
	assert(idx < amu_group1_num_counters());

	switch (idx) {
	case 0x0:
		return read64_amevcntr10();
	case 0x1:
		return read64_amevcntr11();
	case 0x2:
		return read64_amevcntr12();
	case 0x3:
		return read64_amevcntr13();
	case 0x4:
		return read64_amevcntr14();
	case 0x5:
		return read64_amevcntr15();
	case 0x6:
		return read64_amevcntr16();
	case 0x7:
		return read64_amevcntr17();
	case 0x8:
		return read64_amevcntr18();
	case 0x9:
		return read64_amevcntr19();
	case 0xa:
		return read64_amevcntr1a();
	case 0xb:
		return read64_amevcntr1b();
	case 0xc:
		return read64_amevcntr1c();
	case 0xd:
		return read64_amevcntr1d();
	case 0xe:
		return read64_amevcntr1e();
	case 0xf:
		return read64_amevcntr1f();
	default:
		return 0ULL;
	}
}

uint64_t read_amevtyper1(unsigned int idx)
{
	assert(idx < amu_group1_num_counters());

	switch (idx) {
	case 0x0:
		return read_amevtyper10();
	case 0x1:
		return read_amevtyper11();
	case 0x2:
		return read_amevtyper12();
	case 0x3:
		return read_amevtyper13();
	case 0x4:
		return read_amevtyper14();
	case 0x5:
		return read_amevtyper15();
	case 0x6:
		return read_amevtyper16();
	case 0x7:
		return read_amevtyper17();
	case 0x8:
		return read_amevtyper18();
	case 0x9:
		return read_amevtyper19();
	case 0xa:
		return read_amevtyper1a();
	case 0xb:
		return read_amevtyper1b();
	case 0xc:
		return read_amevtyper1c();
	case 0xd:
		return read_amevtyper1d();
	case 0xe:
		return read_amevtyper1e();
	case 0xf:
		return read_amevtyper1f();
	default:
		return 0ULL;
	}
}

void write_amevtyper1(unsigned int idx, uint64_t val)
{
	uint32_t value32 = (uint32_t)val;

	assert(idx < amu_group1_num_counters());

	switch (idx) {
	case 0x0:
		write_amevtyper10(value32);
		break;
	case 0x1:
		write_amevtyper11(value32);
		break;
	case 0x2:
		write_amevtyper12(value32);
		break;
	case 0x3:
		write_amevtyper13(value32);
		break;
	case 0x4:
		write_amevtyper14(value32);
		break;
	case 0x5:
		write_amevtyper15(value32);
		break;
	case 0x6:
		write_amevtyper16(value32);
		break;
	case 0x7:
		write_amevtyper17(value32);
		break;
	case 0x8:
		write_amevtyper18(value32);
		break;
	case 0x9:
		write_amevtyper19(value32);
		break;
	case 0xa:
		write_amevtyper1a(value32);
		break;
	case 0xb:
		write_amevtyper1b(value32);
		break;
	case 0xc:
		write_amevtyper1c(value32);
		break;
	case 0xd:
		write_amevtyper1d(value32);
		break;
	case 0xe:
		write_amevtyper1e(value32);
		break;
	case 0xf:
		write_amevtyper1f(value32);
		break;
	default:
		break;
	}
}
