/*
 * Copyright (c) 2021-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cactus_test_cmds.h>
#include <debug.h>
#include <ffa_endpoints.h>
#include <host_realm_rmi.h>
#include <smccc.h>
#include <spm_test_helpers.h>
#include <test_helpers.h>

#if PLAT_fvp || PLAT_tc
#include <sp_platform_def.h>
static const struct ffa_uuid expected_sp_uuids[] = { {PRIMARY_UUID} };
#endif

#define TEST_DMA_ENGINE_MEMCPY	(2U)
#define TEST_DMA_ENGINE_RAND48	(3U)

/*
 * Attribute encoding for Inner and Outer:
 * Read-Allocate Write-Allocate Write-Back Normal Memory
 */
#define ATTR_ACACHE_RAWAWB_S	(0xffU)
#define ATTR_ACACHE_RAWAWB_NS	(0x2ffU)

/* Source attributes occupy the bottom halfword */
#define DMA_ENGINE_ATTR_SRC_ACACHE_RAWAWB_S	ATTR_ACACHE_RAWAWB_S
#define DMA_ENGINE_ATTR_SRC_ACACHE_RAWAWB_NS	ATTR_ACACHE_RAWAWB_NS

/* Destination attributes occupy the top halfword */
#define DMA_ENGINE_ATTR_DEST_ACACHE_RAWAWB_S	(ATTR_ACACHE_RAWAWB_S << 16)
#define DMA_ENGINE_ATTR_DEST_ACACHE_RAWAWB_NS	(ATTR_ACACHE_RAWAWB_NS << 16)

/**************************************************************************
 * test_smmu_spm
 *
 * Send commands to SP1 initiate DMA service with the help of a peripheral
 * device upstream of an SMMUv3 IP.
 * The scenario involves randomizing a secure buffer (first DMA operation),
 * copying this buffer to another location (second DMA operation),
 * and checking (by CPU) that both buffer contents match.
 **************************************************************************/
test_result_t test_smmu_spm(void)
{
#if PLAT_fvp || PLAT_tc
	struct ffa_value ret;

	/**********************************************************************
	 * Check SPMC has ffa_version and expected FFA endpoints are deployed.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

	VERBOSE("Sending command to SP %x for initiating DMA transfer.\n",
			SP_ID(1));

	/*
	 * Randomize first half of a secure buffer from the secure world
	 * through the SMMU test engine DMA.
	 * Destination memory attributes are secure rawaWB.
	 */
	ret = cactus_send_dma_cmd(HYP_ID, SP_ID(1),
		TEST_DMA_ENGINE_RAND48,
		PLAT_CACTUS_MEMCPY_BASE,
		PLAT_CACTUS_MEMCPY_RANGE / 2,
		DMA_ENGINE_ATTR_DEST_ACACHE_RAWAWB_S);

	/* Expect the SMMU DMA operation to pass. */
	if (cactus_get_response(ret) != CACTUS_SUCCESS) {
		return TEST_RESULT_FAIL;
	}

	/*
	 * Copy first half to second half of the buffer and
	 * check both match.
	 * Source and destination memory attributes are secure rawaWB.
	 */
	ret = cactus_send_dma_cmd(HYP_ID, SP_ID(1),
		TEST_DMA_ENGINE_MEMCPY,
		PLAT_CACTUS_MEMCPY_BASE,
		PLAT_CACTUS_MEMCPY_RANGE,
		DMA_ENGINE_ATTR_DEST_ACACHE_RAWAWB_S |
		DMA_ENGINE_ATTR_SRC_ACACHE_RAWAWB_S);

	/* Expect the SMMU DMA operation to pass. */
	if (cactus_get_response(ret) != CACTUS_SUCCESS) {
		return TEST_RESULT_FAIL;
	}

	/*
	 * Copy first half to second half of the non-secure buffer and
	 * check both match.
	 * Source and destination memory attributes are non-secure rawaWB.
	 * This test helps to validate a scenario where a secure stream
	 * belonging to Cactus SP accesses non-secure IPA space.
	 */
	ret = cactus_send_dma_cmd(HYP_ID, SP_ID(1),
		TEST_DMA_ENGINE_MEMCPY,
		PLAT_CACTUS_NS_MEMCPY_BASE,
		PLAT_CACTUS_MEMCPY_RANGE,
		DMA_ENGINE_ATTR_DEST_ACACHE_RAWAWB_NS |
		DMA_ENGINE_ATTR_SRC_ACACHE_RAWAWB_NS);

	/* Expect the SMMU DMA operation to pass. */
	if (cactus_get_response(ret) != CACTUS_SUCCESS) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
#else
	return TEST_RESULT_SKIPPED;
#endif
}

/**************************************************************************
 * test_smmu_spm_invalid_access
 *
 * The scenario changes a NS buffer PAS into Realm PAS. It then queries a SP
 * to initiate a secure DMA operation on this buffer through the SMMU.
 * The operation is expected to fail as a secure DMA transaction to a Realm
 * region fails SMMU GPC checks.
 **************************************************************************/
test_result_t test_smmu_spm_invalid_access(void)
{
#if PLAT_fvp || PLAT_tc
	struct ffa_value ret;
	u_register_t retmm;

	/* Skip this test if RME is not implemented. */
	if (get_armv9_2_feat_rme_support() == 0U) {
		return TEST_RESULT_SKIPPED;
	}

	/**********************************************************************
	 * Check SPMC has ffa_version and expected FFA endpoints are deployed.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

	/* Update the NS buffer to Realm PAS. */
	retmm = host_rmi_granule_delegate((u_register_t)PLAT_CACTUS_NS_MEMCPY_BASE);
	if (retmm != 0UL) {
		ERROR("Granule delegate failed!\n");
		return TEST_RESULT_FAIL;
	}

	VERBOSE("Sending command to SP %x for initiating DMA transfer.\n",
		SP_ID(1));

	/*
	 * Attempt randomizing the buffer (now turned into Realm PAS)
	 * from the secure world through the SMMU test engine DMA.
	 * Destination memory attributes are non-secure rawaWB.
	 */
	ret = cactus_send_dma_cmd(HYP_ID, SP_ID(1),
		TEST_DMA_ENGINE_RAND48,
		PLAT_CACTUS_NS_MEMCPY_BASE,
		PLAT_CACTUS_MEMCPY_RANGE,
		DMA_ENGINE_ATTR_DEST_ACACHE_RAWAWB_NS);

	/* Update the buffer back to NS PAS. */
	retmm = host_rmi_granule_undelegate((u_register_t)PLAT_CACTUS_NS_MEMCPY_BASE);
	if (retmm != 0UL) {
		ERROR("Granule undelegate failed!\n");
		return TEST_RESULT_FAIL;
	}

	/* Expect the SMMU DMA operation to have failed. */
	if (cactus_get_response(ret) != CACTUS_ERROR) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
#else
	return TEST_RESULT_SKIPPED;
#endif
}
