/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <events.h>
#include <platform.h>
#include <power_management.h>
#include <psci.h>
#include <smccc.h>
#include <test_helpers.h>
#include <tftf_lib.h>
#include <platform_def.h>
#include <string.h>
#include <arch_helpers.h>
#include <debug.h>
#include <errata_abi.h>

static event_t cpu_has_entered_test[PLATFORM_CORE_COUNT];

/* Forward flag */
#define FORWARD_FLAG_EL1	0x00

/* Extract revision and variant info */
#define EXTRACT_REV_VAR(x)	(x & MIDR_REV_MASK) | ((x >> (MIDR_VAR_SHIFT - MIDR_REV_BITS)) \
				& MIDR_VAR_MASK)

/* Extract the partnumber */
#define EXTRACT_PARTNO(x)	((x >> MIDR_PN_SHIFT) & MIDR_PN_MASK)

#define RXPX_RANGE(x, y, z)	(((x >= y) && (x <= z)) ? true : false)

/* Global pointer to point to individual cpu structs based on midr value */
em_cpu_t *cpu_ptr;

/*
 * Errata list for CPUs. This list needs to be updated
 * for every new errata added to the errata ABI list.
 */
em_cpu_t cortex_A15_errata_list = {
	.cpu_pn = 0xC0F,
	.cpu_errata = {
		{816470, 0x30, 0xFF},
		{827671, 0x30, 0xFF},
		{-1}
	},
};

em_cpu_t cortex_A17_errata_list = {
	.cpu_pn = 0xC0E,
	.cpu_errata = {
		{852421, 0x00, 0x12},
		{852423, 0x00, 0x12},
		{-1}
	},
};

em_cpu_t cortex_A9_errata_list = {
	.cpu_pn = 0xC09,
	.cpu_errata = {
		{790473, 0x00, 0xFF},
		{-1}
	},
};

em_cpu_t cortex_A35_errata_list = {
	.cpu_pn = 0xD04,
	.cpu_errata = {
		{855472, 0x00, 0x00},
		{1234567, 0x00, 0x00},
		{-1}
	},
};

em_cpu_t cortex_A53_errata_list = {
	.cpu_pn = 0xD03,
	.cpu_errata = {
		{819472, 0x00, 0x01},
		{824069, 0x00, 0x02},
		{826319, 0x00, 0x02},
		{827319, 0x00, 0x02},
		{835769, 0x00, 0x04},
		{836870, 0x00, 0x03},
		{843419, 0x00, 0x04},
		{855873, 0x03, 0xFF},
		{1530924, 0x00, 0xFF},
		{-1}
	},
};

em_cpu_t cortex_A55_errata_list = {
	.cpu_pn = 0xD05,
	.cpu_errata = {
		{768277, 0x00, 0x00},
		{778703, 0x00, 0x00},
		{798797, 0x00, 0x00},
		{846532, 0x00, 0x01},
		{903758, 0x00, 0x01},
		{1221012, 0x00, 0x10},
		{1530923, 0x00, 0xFF},
		{-1}
	},
};

em_cpu_t cortex_A57_errata_list = {
	.cpu_pn = 0xD07,
	.cpu_errata = {
		{806969, 0x00, 0x00},
		{813419, 0x00, 0x00},
		{813420, 0x00, 0x00},
		{814670, 0x00, 0x00},
		{817169, 0x00, 0x01},
		{826974, 0x00, 0x11},
		{826977, 0x00, 0x11},
		{828024, 0x00, 0x11},
		{829520, 0x00, 0x12},
		{833471, 0x00, 0x12},
		{859972, 0x00, 0x13},
		{1319537, 0x00, 0xFF},
		{-1}
	},
};

em_cpu_t cortex_A72_errata_list = {
	.cpu_pn = 0xD08,
	.cpu_errata = {
		{859971, 0x00, 0x03},
		{1319367, 0x00, 0xFF},
		{-1}
	},
};

em_cpu_t cortex_A73_errata_list = {
	.cpu_pn = 0xD09,
	.cpu_errata = {
		{852427, 0x00, 0x00},
		{855423, 0x00, 0x01},
		{-1}
	},
};

em_cpu_t cortex_A75_errata_list = {
	.cpu_pn = 0xD0A,
	.cpu_errata = {
		{764081, 0x00, 0x00},
		{790748, 0x00, 0x00},
		{-1}
	},
};

em_cpu_t cortex_A76_errata_list = {
	.cpu_pn = 0xD0B,
	.cpu_errata = {
		{1073348, 0x00, 0x10},
		{1130799, 0x00, 0x20},
		{1165522, 0x00, 0xFF},
		{1220197, 0x00, 0x20},
		{1257314, 0x00, 0x30},
		{1262606, 0x00, 0x30},
		{1262888, 0x00, 0x30},
		{1275112, 0x00, 0x30},
		{1791580, 0x00, 0x40},
		{1868343, 0x00, 0x40},
		{1946160, 0x30, 0x41},
		{-1}
	},
};

em_cpu_t cortex_A77_errata_list = {
	.cpu_pn = 0xD0D,
	.cpu_errata = {
		{1508412, 0x00, 0x10},
		{1791578, 0x00, 0x11},
		{1800714, 0x00, 0x11},
		{1925769, 0x00, 0x11},
		{1946167, 0x00, 0x11},
		{2356587, 0x00, 0x11},
		{2743100, 0x00, 0x11},
		{-1}
	},
};

em_cpu_t cortex_A78_AE_errata_list = {
	.cpu_pn = 0xD42,
	.cpu_errata = {
		{1941500, 0x00, 0x01},
		{2376748, 0x00, 0x01},
		{2712574, 0x00, 0x02},
		{2376748, 0x00, 0x01},
		{1951502, 0x00, 0x01},
		{-1}
	},
};

em_cpu_t cortex_A78_errata_list = {
	.cpu_pn = 0xD41,
	.cpu_errata = {
		{1688305, 0x00, 0x10},
		{1821534, 0x00, 0x10},
		{1941498, 0x00, 0x11},
		{1951500, 0x10, 0x11},
		{1952683, 0x00, 0x00},
		{2132060, 0x00, 0x12},
		{2242635, 0x10, 0x12},
		{2376745, 0x00, 0x12},
		{2395406, 0x00, 0x12},
		{2712571, 0x00, 0x12},
		{2742426, 0x00, 0x12},
		{2772019, 0x00, 0x12},
		{2779479, 0x00, 0x12},
		{-1}
	},
};

em_cpu_t cortex_A78C_errata_list = {
	.cpu_pn = 0xD4B,
	.cpu_errata = {
		{2132064, 0x01, 0x02},
		{2242638, 0x01, 0x02},
		{2376749, 0x01, 0x02},
		{2395411, 0x01, 0x02},
		{2712575, 0x01, 0x02},
		{2772121, 0x00, 0x02},
		{2779484, 0x01, 0x02},
		{-1}
	},
};


em_cpu_t cortex_X1_errata_list = {
	.cpu_pn = 0xD44,
	.cpu_errata = {
		{1688305, 0x00, 0x10},
		{1821534, 0x00, 0x10},
		{1827429, 0x00, 0x10},
		{-1}
	},

};

em_cpu_t neoverse_N1_errata_list = {
	.cpu_pn = 0xD0C,
	.cpu_errata = {
		{1073348, 0x00, 0x10},
		{1130799, 0x00, 0x20},
		{1165347, 0x00, 0x20},
		{1207823, 0x00, 0x20},
		{1220197, 0x00, 0x20},
		{1257314, 0x00, 0x30},
		{1262606, 0x00, 0x30},
		{1262888, 0x00, 0x30},
		{1275112, 0x00, 0x30},
		{1315703, 0x00, 0x30},
		{1542419, 0x30, 0x40},
		{1868343, 0x00, 0x40},
		{1946160, 0x30, 0x41},
		{2743102, 0x00, 0x41},
		{-1}
	},
};

em_cpu_t neoverse_V1_errata_list = {
	.cpu_pn = 0xD40,
	.cpu_errata = {
		{1618635, 0x00, 0x0F},
		{1774420, 0x00, 0x10},
		{1791573, 0x00, 0x10},
		{1852267, 0x00, 0x10},
		{1925756, 0x00, 0x11},
		{1940577, 0x10, 0x11},
		{1966096, 0x10, 0x11},
		{2108267, 0x00, 0x11},
		{2139242, 0x00, 0x11},
		{2216392, 0x10, 0x11},
		{2294912, 0x00, 0x11},
		{2372203, 0x00, 0x11},
		{2701953, 0x00, 0x11},
		{2743093, 0x00, 0x12},
		{2779461, 0x00, 0x12},
		{-1}
	},
};

em_cpu_t cortex_A710_errata_list = {
	.cpu_pn = 0xD47,
	.cpu_errata = {
		{1987031, 0x00, 0x20},
		{2008768, 0x00, 0x20},
		{2017096, 0x00, 0x20},
		{2055002, 0x10, 0x20},
		{2058056, 0x00, 0x10},
		{2081180, 0x00, 0x20},
		{2083908, 0x20, 0x20},
		{2136059, 0x00, 0x20},
		{2147715, 0x20, 0x20},
		{2216384, 0x00, 0x20},
		{2267065, 0x00, 0x20},
		{2282622, 0x00, 0x21},
		{2291219, 0x00, 0x20},
		{2371105, 0x00, 0x20},
		{2701952, 0x00, 0x21},
		{2768515, 0x00, 0x21},
		{-1}
	},
};

em_cpu_t neoverse_N2_errata_list = {
	.cpu_pn = 0xD49,
	.cpu_errata = {
		{2002655, 0x00, 0x00},
		{2025414, 0x00, 0x00},
		{2067956, 0x00, 0x00},
		{2138953, 0x00, 0x00},
		{2138956, 0x00, 0x00},
		{2138958, 0x00, 0x00},
		{2189731, 0x00, 0x00},
		{2242400, 0x00, 0x00},
		{2242415, 0x00, 0x00},
		{2280757, 0x00, 0x00},
		{2326639, 0x00, 0x00},
		{2376738, 0x00, 0x00},
		{2388450, 0x00, 0x00},
		{2728475, 0x00, 0x02},
		{2743089, 0x00, 0x02},
		{-1}
	},
};

em_cpu_t cortex_X2_errata_list = {
	.cpu_pn = 0xD48,
	.cpu_errata = {
		{2002765, 0x00, 0x20},
		{2017096, 0x00, 0x20},
		{2058056, 0x00, 0x20},
		{2081180, 0x00, 0x20},
		{2083908, 0x00, 0x20},
		{2147715, 0x20, 0x20},
		{2216384, 0x00, 0x20},
		{2282622, 0x00, 0x21},
		{2371105, 0x00, 0x21},
		{2701952, 0x00, 0x21},
		{2768515, 0x00, 0x21},
		{-1}
	},
};

em_cpu_t cortex_A510_errata_list = {
	.cpu_pn = 0xD46,
	.cpu_errata = {
		{1922240, 0x00, 0x00},
		{2041909, 0x02, 0x02},
		{2042739, 0x00, 0x02},
		{2172148, 0x00, 0x10},
		{2218950, 0x00, 0x10},
		{2250311, 0x00, 0x10},
		{2288014, 0x00, 0x10},
		{2347730, 0x00, 0x11},
		{2371937, 0x00, 0x11},
		{2666669, 0x00, 0x11},
		{2684597, 0x00, 0x12},
		{1234567, 0x00, 0x12},
		{-1}
	},
};

em_cpu_t cortex_X4_errata_list = {
	.cpu_pn = 0xD82,
	.cpu_errata = {
		{2701112, 0x00, 0x00},
		{-1}
	},
};

em_cpu_t cortex_A715_errata_list = {
	.cpu_pn = 0xD4D,
	.cpu_errata = {
		{2701951, 0x00, 0x11},
		{-1}
	},
};

em_cpu_t neoverse_V2_errata_list = {
	.cpu_pn = 0xD4F,
	.cpu_errata = {
		{2719103, 0x00, 0x01},
		{-1}
	},
};

/*
 * Test function checks for the em_version implemented
 * - Test fails if the version returned is < 1.0.
 * - Test passes if the version returned is == 1.0
 */
test_result_t test_em_version(void)
{
	int32_t version_return = tftf_em_abi_version();

	if ((version_return == (EM_ABI_VERSION(1, 0))))
		return TEST_RESULT_SUCCESS;

	if (version_return == EM_NOT_SUPPORTED)
		return TEST_RESULT_SKIPPED;

	return TEST_RESULT_FAIL;
}

/*
 * Test function checks for the em_features implemented
 * Test fails if the em_feature is not implemented
 * or if the fid is invalid.
 */

test_result_t test_em_features(void)
{
	int32_t version_return = tftf_em_abi_version();

	if (version_return == EM_NOT_SUPPORTED)
		return TEST_RESULT_SKIPPED;

	if (!(tftf_em_abi_feature_implemented(EM_CPU_ERRATUM_FEATURES)))
		return TEST_RESULT_FAIL;

	return TEST_RESULT_SUCCESS;
}

/*
 * Test function checks for the em_cpu_feature implemented
 * Test fails if the em_cpu_feature is not implemented
 * or if the fid is invalid.
 */
test_result_t test_em_cpu_features(void)
{
	test_result_t return_val = TEST_RESULT_FAIL;
	smc_ret_values ret_val;

	uint32_t midr_val = read_midr();
	uint16_t rxpx_val_extracted = EXTRACT_REV_VAR(midr_val);
	midr_val = EXTRACT_PARTNO(midr_val);

	u_register_t mpid = read_mpidr_el1() & MPID_MASK;
	unsigned int core_pos = platform_get_core_pos(mpid);

	INFO("Partnum extracted = %x and rxpx extracted val = %x\n\n", midr_val, \
							rxpx_val_extracted);
	switch (midr_val) {
	case 0xD09:
	{
		VERBOSE("MIDR matches A73 -> %x\n", midr_val);
		cpu_ptr = &cortex_A73_errata_list;
		break;
	}
	case 0xD0B:
	{
		VERBOSE("MIDR matches A76 -> %x\n", midr_val);
		cpu_ptr = &cortex_A76_errata_list;
		break;
	}
	case 0xD4D:
	{
		VERBOSE("MIDR matches A715 -> %x\n", midr_val);
		cpu_ptr = &cortex_A715_errata_list;
		break;
	}
	case 0xD04:
	{
		VERBOSE("MIDR matches A35 -> %x\n", midr_val);
		cpu_ptr = &cortex_A35_errata_list;
		break;
	}
	case 0xD03:
	{
		VERBOSE("MIDR matches A53 = %x\n", midr_val);
		cpu_ptr = &cortex_A53_errata_list;
		break;
	}
	case 0xD07:
	{
		VERBOSE("MIDR matches A57 = %x\n", midr_val);
		cpu_ptr = &cortex_A57_errata_list;
		break;
	}
	case 0xD08:
	{
		VERBOSE("MIDR matches A72 = %x\n", midr_val);
		cpu_ptr = &cortex_A72_errata_list;
		break;
	}
	case 0xD0D:
	{
		VERBOSE("MIDR matches A77 = %x\n", midr_val);
		cpu_ptr = &cortex_A77_errata_list;
		break;
	}
	case 0xD41:
	{
		VERBOSE("MIDR matches A78 = %x\n", midr_val);
		cpu_ptr = &cortex_A78_errata_list;
		break;
	}
	case 0xD0C:
	{
		VERBOSE("MIDR matches Neoverse N1 = %x\n", midr_val);
		cpu_ptr = &neoverse_N1_errata_list;
		break;
	}
	case 0xD4B:
	{
		VERBOSE("MIDR matches A78C = %x\n", midr_val);
		cpu_ptr = &cortex_A78C_errata_list;
		break;
	}
	case 0xD4F:
	{
		VERBOSE("MIDR matches Neoverse V2 -> %x\n", midr_val);
		cpu_ptr = &neoverse_V2_errata_list;
		break;
	}
	case 0xD47:
	{
		VERBOSE("MIDR matches A710 -> %x\n", midr_val);
		cpu_ptr = &cortex_A710_errata_list;
		break;
	}
	case 0xD46:
	{
		VERBOSE("MIDR matches A510 -> %x\n", midr_val);
		cpu_ptr = &cortex_A510_errata_list;
		break;
	}
	case 0xD48:
	{
		VERBOSE("MIDR matches X2 -> %x\n", midr_val);
		cpu_ptr = &cortex_X2_errata_list;
		break;
	}
	case 0xD49:
	{
		VERBOSE("MIDR matches Neoverse N2 -> %x\n", midr_val);
		cpu_ptr = &neoverse_N2_errata_list;
		break;
	}
	case 0xD40:
	{
		VERBOSE("MIDR matches Neoverse V1 -> %x\n", midr_val);
		cpu_ptr = &neoverse_V1_errata_list;
		break;
	}
	case 0xD44:
	{
		VERBOSE("MIDR matches X1 > %x\n", midr_val);
		cpu_ptr = &cortex_X1_errata_list;
		break;
	}
	case 0xD0A:
	{
		VERBOSE("MIDR matches A75 > %x\n", midr_val);
		cpu_ptr = &cortex_A75_errata_list;
		break;
	}
	case 0xD05:
	{
		VERBOSE("MIDR matches A55 > %x\n", midr_val);
		cpu_ptr = &cortex_A55_errata_list;
		break;
	}
	case 0xD42:
	{
		VERBOSE("MIDR matches A78_AE > %x\n", midr_val);
		cpu_ptr = &cortex_A78_AE_errata_list;
		break;
	}
	case 0xD82:
	{
		VERBOSE("MIDR matches Cortex-X4 -> %x\n", midr_val);
		cpu_ptr =  &cortex_X4_errata_list;
		break;
	}
	default:
	{
		ERROR("MIDR did not match any cpu\n");
		return TEST_RESULT_SKIPPED;
		break;
	}
	}

	for (int i = 0; i < ERRATA_COUNT && cpu_ptr->cpu_errata[i].em_errata_id != -1; i++) {

		ret_val = tftf_em_abi_cpu_feature_implemented \
				(cpu_ptr->cpu_errata[i].em_errata_id, \
				FORWARD_FLAG_EL1);

		switch (ret_val.ret0) {

		case EM_NOT_AFFECTED:
		{
			return_val = (RXPX_RANGE(rxpx_val_extracted, \
			cpu_ptr->cpu_errata[i].rxpx_low, cpu_ptr->cpu_errata[i].rxpx_high) \
				== false) ? TEST_RESULT_SUCCESS : TEST_RESULT_FAIL;
			break;
		}
		case EM_AFFECTED:
		{
			return_val = TEST_RESULT_SUCCESS;
			break;
		}
		case EM_HIGHER_EL_MITIGATION:
		{
			return_val = (RXPX_RANGE(rxpx_val_extracted, \
			cpu_ptr->cpu_errata[i].rxpx_low, cpu_ptr->cpu_errata[i].rxpx_high) \
				== true) ? TEST_RESULT_SUCCESS : TEST_RESULT_FAIL;
			break;
		}
		case EM_UNKNOWN_ERRATUM:
		{
			return_val = TEST_RESULT_SUCCESS;
			break;
		}
		default:
		{
			ERROR("Return value did not match the expected returns\n");
			return_val = TEST_RESULT_FAIL;
			break;
		}
		}
		INFO("errata_id = %d and test_em_cpu_erratum_features = %ld\n",\
			cpu_ptr->cpu_errata[i].em_errata_id, ret_val.ret0);
	}
	/* Signal to the lead CPU that the calling CPU has entered the test */
	tftf_send_event(&cpu_has_entered_test[core_pos]);
	return return_val;
}

test_result_t test_errata_abi_features(void)
{
	unsigned int lead_mpid;
	unsigned int cpu_mpid, cpu_node, core_pos;
	int psci_ret;

	int32_t version_return = tftf_em_abi_version();

	SKIP_TEST_IF_LESS_THAN_N_CPUS(1);

	if (version_return == EM_NOT_SUPPORTED) {
		return TEST_RESULT_SKIPPED;
	}

	if (!(tftf_em_abi_feature_implemented(EM_CPU_ERRATUM_FEATURES))) {
		return TEST_RESULT_FAIL;
	}

	lead_mpid = read_mpidr_el1() & MPID_MASK;
	/* Power on all CPUs */
	for_each_cpu(cpu_node) {
		cpu_mpid = tftf_get_mpidr_from_node(cpu_node);

		/* Skip lead CPU as it is already powered on */
		if (cpu_mpid == lead_mpid)
			continue;

		psci_ret = tftf_cpu_on(cpu_mpid, (uintptr_t)test_em_cpu_features, 0);
		if (psci_ret != PSCI_E_SUCCESS) {
			tftf_testcase_printf("Failed to power on CPU 0x%x (%d)\n", \
			cpu_mpid, psci_ret);
			return TEST_RESULT_FAIL;
		}

		core_pos = platform_get_core_pos(cpu_mpid);
		tftf_wait_for_event(&cpu_has_entered_test[core_pos]);
	}
	return TEST_RESULT_SUCCESS;
}
