/*
 * Copyright (c) 2018-2024, Arm Limited. All rights reserved.
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
#define FFA_MINOR 2U

static uint32_t spm_version;

static const struct ffa_uuid sp_uuids[] = {
		{PRIMARY_UUID}, {SECONDARY_UUID}, {TERTIARY_UUID}, {IVY_UUID},
		{STMM_UUID}, {EL3_SPMD_LP_UUID}
};

static const struct ffa_partition_info ffa_expected_partition_info[] = {
	/* Primary partition info */
	{
		.id = SP_ID(1),
		.exec_context = PRIMARY_EXEC_CTX_COUNT,
		.properties = (FFA_PARTITION_AARCH64_EXEC |
			       FFA_PARTITION_DIRECT_REQ_RECV |
			       FFA_PARTITION_DIRECT_REQ_SEND |
			       FFA_PARTITION_INDIRECT_MSG |
			       FFA_PARTITION_NOTIFICATION),
		.uuid = {PRIMARY_UUID}
	},
	/* Secondary partition info */
	{
		.id = SP_ID(2),
		.exec_context = SECONDARY_EXEC_CTX_COUNT,
		.properties = (FFA_PARTITION_AARCH64_EXEC |
			       FFA_PARTITION_DIRECT_REQ_RECV |
			       FFA_PARTITION_DIRECT_REQ_SEND |
			       FFA_PARTITION_NOTIFICATION),
		.uuid = {SECONDARY_UUID}
	},
	/* Tertiary partition info */
	{
		.id = SP_ID(3),
		.exec_context = TERTIARY_EXEC_CTX_COUNT,
		.properties = (FFA_PARTITION_AARCH64_EXEC |
			       FFA_PARTITION_DIRECT_REQ_RECV |
			       FFA_PARTITION_DIRECT_REQ_SEND |
			       FFA_PARTITION_NOTIFICATION),
		.uuid = {TERTIARY_UUID}
	},
	/* Ivy partition info */
	{
		.id = SP_ID(4),
		.exec_context = IVY_EXEC_CTX_COUNT,
		.properties = (FFA_PARTITION_AARCH64_EXEC |
			       FFA_PARTITION_DIRECT_REQ_RECV |
			       FFA_PARTITION_DIRECT_REQ_SEND),
		.uuid = {IVY_UUID}
	},
	{
		.id = SP_ID(5),
		.exec_context = 1,
		.properties = (FFA_PARTITION_AARCH64_EXEC |
			       FFA_PARTITION_DIRECT_REQ_RECV |
			       FFA_PARTITION_DIRECT_REQ_SEND),
		.uuid = {STMM_UUID}
	},
	/* EL3 SPMD logical partition */
	{
		.id = SP_ID(0x7FC0),
		.exec_context = EL3_SPMD_LP_EXEC_CTX_COUNT,
		.properties = (FFA_PARTITION_AARCH64_EXEC |
			       FFA_PARTITION_DIRECT_REQ_SEND),
		.uuid = {EL3_SPMD_LP_UUID}
	},
};

/*
 * Test FFA_FEATURES interface.
 */
static void ffa_features_test(bool el1_partition)
{
	const struct ffa_features_test *func_id_targets;
	/* Get common features between tftf and cactus. */
	unsigned int test_target_size =
		get_ffa_feature_test_target(&func_id_targets);
	/* Specific to SPs. */
	struct ffa_features_test feature_id_targets[] = {
		{"FFA_FEATURE_MEI", FFA_FEATURE_MEI, FFA_SUCCESS_SMC32, 0,
			FFA_VERSION_1_1},
		{"FFA_FEATURE_SRI", FFA_FEATURE_SRI, FFA_ERROR, 0,
			FFA_VERSION_1_1},
		{"FFA_FEATURE_NPI", FFA_FEATURE_NPI, FFA_SUCCESS_SMC32, 0,
			FFA_VERSION_1_1},
		{"FFA_YIELD_32", FFA_MSG_YIELD, FFA_SUCCESS_SMC32,
			FFA_VERSION_1_0},
	};

	INFO("Test FFA_FEATURES.\n");
	ffa_features_test_targets(func_id_targets, test_target_size);

	/* Features are expected to be different to tftf. */

	/* EL0 partitions don't support NPI. */
	if (!el1_partition) {
		feature_id_targets[2].expected_ret = FFA_ERROR;
	}

	ffa_features_test_targets(feature_id_targets,
			ARRAY_SIZE(feature_id_targets));
}

static void ffa_partition_info_wrong_test(void)
{
	const struct ffa_uuid uuid = { .uuid = {1} };
	struct ffa_value ret = ffa_partition_info_get(uuid);

	VERBOSE("%s: test request wrong UUID.\n", __func__);

	EXPECT(ffa_func_id(ret), FFA_ERROR);
	EXPECT(ffa_error_code(ret), FFA_ERROR_INVALID_PARAMETER);
}

static void ffa_partition_info_get_regs_test(void)
{
	struct ffa_value ret = { 0 };

	VERBOSE("FF-A Partition Info regs interface tests\n");
	ret = ffa_version(FFA_VERSION_1_2);
	uint32_t version = ret.fid;

	if (version == FFA_ERROR_NOT_SUPPORTED) {
		ERROR("FFA_VERSION 1.2 not supported, skipping"
			" FFA_PARTITION_INFO_GET_REGS test.\n");
		return;
	}

	ret = ffa_features(FFA_PARTITION_INFO_GET_REGS_SMC64);
	if (ffa_func_id(ret) != FFA_SUCCESS_SMC32) {
		ERROR("FFA_PARTITION_INFO_GET_REGS not supported skipping tests.\n");
		return;
	}

	EXPECT(ffa_partition_info_regs_helper(sp_uuids[3],
		&ffa_expected_partition_info[3], 1), true);
	EXPECT(ffa_partition_info_regs_helper(sp_uuids[2],
		&ffa_expected_partition_info[2], 1), true);
	EXPECT(ffa_partition_info_regs_helper(sp_uuids[1],
		&ffa_expected_partition_info[1], 1), true);
	EXPECT(ffa_partition_info_regs_helper(sp_uuids[0],
		&ffa_expected_partition_info[0], 1), true);

	/*
	 * Check partition information if there is support for SPMD EL3
	 * partitions. calling partition_info_get_regs with the SPMD EL3
	 * UUID successfully, indicates the presence of it (there is no
	 * spec defined way to discover presence of el3 spmd logical
	 * partitions). If the call fails with a not supported error,
	 * we assume they dont exist and skip further tests to avoid
	 * failures on platforms without el3 spmd logical partitions.
	 */
	ret = ffa_partition_info_get_regs(sp_uuids[5], 0, 0);
	if ((ffa_func_id(ret) == FFA_ERROR) &&
	    ((ffa_error_code(ret) == FFA_ERROR_NOT_SUPPORTED) ||
	    (ffa_error_code(ret) == FFA_ERROR_INVALID_PARAMETER))) {
		INFO("Skipping register based EL3 SPMD Logical partition"
				" discovery\n");
		EXPECT(ffa_partition_info_regs_helper(NULL_UUID,
			ffa_expected_partition_info,
			(ARRAY_SIZE(ffa_expected_partition_info) - 1)), true);
	} else {
		EXPECT(ffa_partition_info_regs_helper(sp_uuids[4],
			&ffa_expected_partition_info[4], 1), true);
		EXPECT(ffa_partition_info_regs_helper(NULL_UUID,
			ffa_expected_partition_info,
			ARRAY_SIZE(ffa_expected_partition_info)), true);
	}
}

static void ffa_partition_info_get_test(struct mailbox_buffers *mb)
{
	INFO("Test FFA_PARTITION_INFO_GET.\n");

	EXPECT(ffa_partition_info_helper(mb, sp_uuids[2],
		&ffa_expected_partition_info[2], 1), true);

	EXPECT(ffa_partition_info_helper(mb, sp_uuids[1],
		&ffa_expected_partition_info[1], 1), true);

	EXPECT(ffa_partition_info_helper(mb, sp_uuids[0],
		&ffa_expected_partition_info[0], 1), true);

	/*
	 * TODO: ffa_partition_info_get_regs returns EL3 SPMD LP information
	 * but partition_info_get does not. Ignore the last entry, that is
	 * assumed to be the EL3 SPMD LP information. ffa_partition_info_get
	 * uses the rx/tx buffer and the SPMD does not support the use of
	 * rx/tx buffer to return SPMD logical partition information.
	 */
	EXPECT(ffa_partition_info_helper(mb, NULL_UUID,
		ffa_expected_partition_info,
		(ARRAY_SIZE(ffa_expected_partition_info) - 1)), true);

	ffa_partition_info_wrong_test();
}

static void ffa_version_test(void)
{
	struct ffa_value ret = ffa_version(FFA_VERSION_COMPILED);

	spm_version = (uint32_t)ret.fid;
	EXPECT(spm_version, FFA_VERSION_COMPILED);

	bool compatible = ffa_versions_are_compatible(spm_version, FFA_VERSION_COMPILED);

	INFO("Test FFA_VERSION. Return %u.%u; Compatible: %i\n",
		ffa_version_get_major(spm_version),
		ffa_version_get_minor(spm_version),
		(int)compatible);

	EXPECT((int)compatible, (int)true);
}

static void ffa_spm_id_get_test(void)
{
	if (spm_version >= FFA_VERSION_1_1) {
		struct ffa_value ret = ffa_spm_id_get();

		EXPECT(ffa_func_id(ret), FFA_SUCCESS_SMC32);

		ffa_id_t spm_id = ffa_endpoint_id(ret);

		INFO("Test FFA_SPM_ID_GET. Return: 0x%x\n", spm_id);

		/*
		 * Check the SPMC value given in the fvp_spmc_manifest
		 * is returned.
		 */
		EXPECT(spm_id, SPMC_ID);
	} else {
		INFO("FFA_SPM_ID_GET not supported in this version of FF-A."
			" Test skipped.\n");
	}
}

void ffa_tests(struct mailbox_buffers *mb, bool el1_partition)
{
	const char *test_ffa_str = "FF-A setup and discovery";

	announce_test_section_start(test_ffa_str);

	ffa_features_test(el1_partition);
	ffa_version_test();
	ffa_spm_id_get_test();
	ffa_partition_info_get_test(mb);
	ffa_partition_info_get_regs_test();

	announce_test_section_end(test_ffa_str);
}
