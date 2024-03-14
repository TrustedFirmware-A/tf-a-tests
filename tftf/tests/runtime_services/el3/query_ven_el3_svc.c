/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <psci.h>
#include <smccc.h>
#include <ven_el3_svc.h>
#include <tftf_lib.h>
#include <uuid_utils.h>

/*
 * Vendor-Specific EL3 UUID as returned by the implementation in the Trusted
 * Firmware.
 */
static const uuid_t armtf_ven_el3_svc_uuid = {
	{0xb6, 0x01, 0x1d, 0xca},
	{0x57, 0xc4},
	{0x40, 0x7e},
	0x83, 0xf0,
	{0xa7, 0xed, 0xda, 0xf0, 0xdf, 0x6c}
};

/**
 * @Test_Aim@ Query the Vendor-Specific El3 Service
 *
 * This test targets the implementation of the vendor-specific el3 in the Trusted
 * Firmware. If it is interfaced with a different implementation then this test
 * will most likely fail because the values returned by the service won't be the
 * ones expected.
 *
 * The following queries are performed:
 * 1) Call UID
 * 2) Call count - is reserved and is expected to fail
 * 3) Call revision details
 */
test_result_t test_query_ven_el3_svc(void)
{
	smc_args ven_el3_svc_args;
	smc_ret_values ret;
	uuid_t ven_el3_svc_uuid;
	char uuid_str[UUID_STR_SIZE];
	test_result_t test_result = TEST_RESULT_SUCCESS;

	/* Standard Service Call UID */
	ven_el3_svc_args.fid = VEN_EL3_SVC_UID;
	ret = tftf_smc(&ven_el3_svc_args);

	make_uuid_from_4words(&ven_el3_svc_uuid,
			ret.ret0, ret.ret1, ret.ret2, ret.ret3);
	if (!uuid_equal(&ven_el3_svc_uuid, &armtf_ven_el3_svc_uuid)) {
		tftf_testcase_printf("Wrong UUID: expected %s,\n",
				uuid_to_str(&armtf_ven_el3_svc_uuid, uuid_str));
		tftf_testcase_printf("                 got %s\n",
				uuid_to_str(&ven_el3_svc_uuid, uuid_str));
		test_result = TEST_RESULT_FAIL;
	}

	/*
	 * Standard Service Call Count which is reserved for vendor-specific el3
	 * This will return an unkown smc.
	 */
	ven_el3_svc_args.fid = VEN_EL3_SVC_UID + 1;
	ret = tftf_smc(&ven_el3_svc_args);

	if (ret.ret0 != SMC_UNKNOWN) {
		tftf_testcase_printf("Querying Vendor-Specific el3 service call count"
				" which is reserved failed\n");
		test_result = TEST_RESULT_FAIL;
	}

	/* Vendor-Specific El3 Service Call for version details */
	ven_el3_svc_args.fid = VEN_EL3_SVC_VERSION;
	ret = tftf_smc(&ven_el3_svc_args);

	if ((ret.ret0 != VEN_EL3_SVC_VERSION_MAJOR) ||
	    (ret.ret1 != VEN_EL3_SVC_VERSION_MINOR)) {
		tftf_testcase_printf(
			"Vendor Specific El3 service reported wrong version: expected {%u.%u}, got {%llu.%llu}\n",
			VEN_EL3_SVC_VERSION_MAJOR, VEN_EL3_SVC_VERSION_MINOR,
			(unsigned long long)ret.ret0,
			(unsigned long long)ret.ret1);
		test_result = TEST_RESULT_FAIL;
	}

	return test_result;
}
