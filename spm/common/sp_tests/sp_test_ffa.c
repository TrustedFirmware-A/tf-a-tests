/*
 * Copyright (c) 2018-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "ffa_helpers.h"
#include <assert.h>
#include <debug.h>
#include <errno.h>

#include <sp_def.h>
#include <ffa_endpoints.h>
#include <sp_helpers.h>
#include <spm_helpers.h>
#include <spm_common.h>

#include <lib/libc/string.h>

/* FFA version test helpers */
#define FFA_MAJOR 1U
#define FFA_MINOR 1U

static uint32_t spm_version;

static const struct ffa_uuid sp_uuids[] = {
		{PRIMARY_UUID}, {SECONDARY_UUID}, {TERTIARY_UUID}, {IVY_UUID}
	};

static const struct ffa_partition_info ffa_expected_partition_info[] = {
	/* Primary partition info */
	{
		.id = SP_ID(1),
		.exec_context = PRIMARY_EXEC_CTX_COUNT,
		.properties = (FFA_PARTITION_AARCH64_EXEC |
			       FFA_PARTITION_DIRECT_REQ_RECV |
			       FFA_PARTITION_DIRECT_REQ_SEND |
			       FFA_PARTITION_NOTIFICATION),
		.uuid = sp_uuids[0]
	},
	/* Secondary partition info */
	{
		.id = SP_ID(2),
		.exec_context = SECONDARY_EXEC_CTX_COUNT,
		.properties = (FFA_PARTITION_AARCH64_EXEC |
			       FFA_PARTITION_DIRECT_REQ_RECV |
			       FFA_PARTITION_DIRECT_REQ_SEND |
			       FFA_PARTITION_NOTIFICATION),
		.uuid = sp_uuids[1]
	},
	/* Tertiary partition info */
	{
		.id = SP_ID(3),
		.exec_context = TERTIARY_EXEC_CTX_COUNT,
		.properties = (FFA_PARTITION_AARCH64_EXEC |
			       FFA_PARTITION_DIRECT_REQ_RECV |
			       FFA_PARTITION_DIRECT_REQ_SEND |
			       FFA_PARTITION_NOTIFICATION),
		.uuid = sp_uuids[2]
	},
	/* Ivy partition info */
	{
		.id = SP_ID(4),
		.exec_context = IVY_EXEC_CTX_COUNT,
		.properties = (FFA_PARTITION_AARCH64_EXEC |
			       FFA_PARTITION_DIRECT_REQ_RECV |
			       FFA_PARTITION_DIRECT_REQ_SEND),
		.uuid = sp_uuids[3]
	}
};

/*
 * Test FFA_FEATURES interface.
 */
static void ffa_features_test(void)
{
	struct ffa_value ffa_ret;
	unsigned int expected_ret;
	const struct ffa_features_test *ffa_feature_test_target;
	unsigned int i, test_target_size =
		get_ffa_feature_test_target(&ffa_feature_test_target);
	struct ffa_features_test test_target;

	INFO("Test FFA_FEATURES.\n");

	for (i = 0U; i < test_target_size; i++) {
		test_target = ffa_feature_test_target[i];

		ffa_ret = ffa_features_with_input_property(test_target.feature,
							   test_target.param);
		expected_ret = FFA_VERSION_COMPILED
				>= test_target.version_added ?
				test_target.expected_ret : FFA_ERROR;

		if (ffa_func_id(ffa_ret) != expected_ret) {
			ERROR("Unexpected return: %x (expected %x)."
			      " FFA_FEATURES test: %s.\n",
			      ffa_func_id(ffa_ret), expected_ret,
			      test_target.test_name);
		}

		if (expected_ret == FFA_ERROR) {
			if (ffa_error_code(ffa_ret) !=
			    FFA_ERROR_NOT_SUPPORTED) {
				ERROR("Unexpected error code: %x (expected %x)."
				      " FFA_FEATURES test: %s.\n",
				      ffa_error_code(ffa_ret), expected_ret,
				      test_target.test_name);
			}
		}
	}
}

static void ffa_partition_info_wrong_test(void)
{
	const struct ffa_uuid uuid = { .uuid = {1} };
	struct ffa_value ret = ffa_partition_info_get(uuid);

	VERBOSE("%s: test request wrong UUID.\n", __func__);

	expect(ffa_func_id(ret), FFA_ERROR);
	expect(ffa_error_code(ret), FFA_ERROR_INVALID_PARAMETER);
}

static void ffa_partition_info_get_test(struct mailbox_buffers *mb)
{
	INFO("Test FFA_PARTITION_INFO_GET.\n");

	expect(ffa_partition_info_helper(mb, sp_uuids[2],
		&ffa_expected_partition_info[2], 1), true);

	expect(ffa_partition_info_helper(mb, sp_uuids[1],
		&ffa_expected_partition_info[1], 1), true);

	expect(ffa_partition_info_helper(mb, sp_uuids[0],
		&ffa_expected_partition_info[0], 1), true);

	expect(ffa_partition_info_helper(mb, NULL_UUID,
		ffa_expected_partition_info,
		ARRAY_SIZE(ffa_expected_partition_info)), true);

	ffa_partition_info_wrong_test();
}

void ffa_version_test(void)
{
	struct ffa_value ret = ffa_version(MAKE_FFA_VERSION(FFA_MAJOR,
							    FFA_MINOR));

	spm_version = (uint32_t)ret.fid;

	bool ffa_version_compatible =
		((spm_version >> FFA_VERSION_MAJOR_SHIFT) == FFA_MAJOR &&
		 (spm_version & FFA_VERSION_MINOR_MASK) >= FFA_MINOR);

	INFO("Test FFA_VERSION. Return %u.%u; Compatible: %i\n",
		spm_version >> FFA_VERSION_MAJOR_SHIFT,
		spm_version & FFA_VERSION_MINOR_MASK,
		(int)ffa_version_compatible);

	expect((int)ffa_version_compatible, (int)true);
}

void ffa_spm_id_get_test(void)
{
	if (spm_version >= MAKE_FFA_VERSION(1, 1)) {
		struct ffa_value ret = ffa_spm_id_get();

		expect(ffa_func_id(ret), FFA_SUCCESS_SMC32);

		ffa_id_t spm_id = ffa_endpoint_id(ret);

		INFO("Test FFA_SPM_ID_GET. Return: 0x%x\n", spm_id);

		/*
		 * Check the SPMC value given in the fvp_spmc_manifest
		 * is returned.
		 */
		expect(spm_id, SPMC_ID);
	} else {
		INFO("FFA_SPM_ID_GET not supported in this version of FF-A."
			" Test skipped.\n");
	}
}

void ffa_tests(struct mailbox_buffers *mb)
{
	const char *test_ffa = "FF-A setup and discovery";

	announce_test_section_start(test_ffa);

	ffa_features_test();
	ffa_version_test();
	ffa_spm_id_get_test();
	ffa_partition_info_get_test(mb);

	announce_test_section_end(test_ffa);
}
