/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arg_struct_def.h>
#include "constraint.h"
#include <fuzz_names.h>
#include <vendor_fuzz_helper.h>

#include <lib/tftf_lib.h>
#include <runtime_services/ven_el3_svc.h>
#include <smccc.h>
#include <uuid_utils.h>

#ifdef VEN_INCLUDE

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

void inputparameters_to_ven_el3_args(struct inputparameters inp, smc_args *args)
{
	args->arg1 = inp.x1;
	args->arg2 = inp.x2;
	args->arg3 = inp.x3;
	args->arg4 = inp.x4;
	args->arg5 = inp.x5;
	args->arg6 = inp.x6;
	args->arg7 = inp.x7;
}

void run_ven_el3_fuzz(int funcid, struct memmod *mmod)
{
	if (funcid == ven_el3_svc_uuid_funcid) {

		smc_args ven_el3_svc_args;
		smc_ret_values ret;
		uuid_t ven_el3_svc_uuid;
		char uuid_str[UUID_STR_SIZE];

		/* Standard Service Call UID */
		ven_el3_svc_args.fid = VEN_EL3_SVC_UID;
		struct inputparameters inp = generate_args(VEN_EL3_SVC_UUID_CALL, SMC_FUZZ_SANITY_LEVEL);

		inputparameters_to_ven_el3_args(inp, &ven_el3_svc_args);

		ret = tftf_smc(&ven_el3_svc_args);

		make_uuid_from_4words(&ven_el3_svc_uuid,
		ret.ret0, ret.ret1, ret.ret2, ret.ret3);

		if (!uuid_equal(&ven_el3_svc_uuid, &armtf_ven_el3_svc_uuid)) {
			tftf_testcase_printf("Wrong UUID: expected %s,\n",
			uuid_to_str(&armtf_ven_el3_svc_uuid, uuid_str));
			tftf_testcase_printf("		 got %s\n",
			uuid_to_str(&ven_el3_svc_uuid, uuid_str));
		} else {
		#ifdef SMC_FUZZER_DEBUG
			printf("Correct UUID: got %s,\n",
			uuid_to_str(&ven_el3_svc_uuid, uuid_str));
		#endif
		}
	} else if (funcid == ven_el3_svc_count_funcid) {
		smc_args ven_el3_svc_args;
		smc_ret_values ret;

		ven_el3_svc_args.fid = VEN_EL3_SVC_UID + 1;
		struct inputparameters inp = generate_args(VEN_EL3_SVC_COUNT_CALL, SMC_FUZZ_SANITY_LEVEL);

		inputparameters_to_ven_el3_args(inp, &ven_el3_svc_args);

		ret = tftf_smc(&ven_el3_svc_args);

		if (ret.ret0 != SMC_UNKNOWN) {
			tftf_testcase_printf("Querying Vendor-Specific el3 service call count"
			" which is reserved failed\n");
		} else {
		#ifdef SMC_FUZZER_DEBUG
			printf("Querying Vendor-Specific el3 service call count"
			" got %ld\n", ret.ret0);
		#endif
	}
	} else if (funcid == ven_el3_svc_version_funcid) {
		smc_args ven_el3_svc_args;
		smc_ret_values ret;

		ven_el3_svc_args.fid = VEN_EL3_SVC_VERSION;
		struct inputparameters inp = generate_args(VEN_EL3_SVC_UUID_CALL, SMC_FUZZ_SANITY_LEVEL);

		inputparameters_to_ven_el3_args(inp, &ven_el3_svc_args);

		ret = tftf_smc(&ven_el3_svc_args);

		if ((ret.ret0 != VEN_EL3_SVC_VERSION_MAJOR) ||
		(ret.ret1 != VEN_EL3_SVC_VERSION_MINOR)) {
			tftf_testcase_printf(
			"Vendor Specific El3 service reported wrong version: expected {%u.%u}, got {%llu.%llu}\n",
			VEN_EL3_SVC_VERSION_MAJOR, VEN_EL3_SVC_VERSION_MINOR,
			(unsigned long long)ret.ret0,
			(unsigned long long)ret.ret1);
		}
	}
}

#endif
