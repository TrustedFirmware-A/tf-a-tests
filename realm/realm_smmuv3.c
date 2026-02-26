/*
 * Copyright (c) 2025-2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>

#include <realm_da_helpers.h>
#include <realm_helpers.h>
#include <realm_rsi.h>
#include <smmuv3_test_engine.h>

#include <debug.h>

static struct rdev gbl_rdev;

/* RDEV info. Device type and attestation evidence digest */
static struct rsi_vdev_info gbl_vdev_info[2] __aligned(GRANULE_SIZE);
static struct rsi_realm_config realm_config __aligned(GRANULE_SIZE);

uint8_t src_buf[PAGE_SIZE] __aligned(PAGE_SIZE);
uint8_t dst_buf[PAGE_SIZE] __aligned(PAGE_SIZE);

#define BUF_SIZE	sizeof("0123456789abcdef: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")

static void dump_memory(uint8_t *addr, unsigned int num)
{
	char buf[BUF_SIZE];

	do {
		char *str = buf;
		size_t size = sizeof(buf);
		int n = snprintf(str, size, "%lx:", (uintptr_t)addr);

		str += n;
		size -= n;

		for (unsigned int i = 0U; i < 16U; i++) {
			n = snprintf(str, size, " %02x", *addr++);
			str += n;
			size -= n;
			if (--num == 0U) {
				break;
			}
		}

		(void)snprintf(str, size, "\n");
		realm_printf(buf);

	} while (num != 0U);
}

static bool realm_validate_mapping(struct rdev *rdev, struct rsi_vdev_info *vdev_info,
				   u_register_t ipa_base, u_register_t ipa_top,
				   u_register_t pa_base)
{
	u_register_t flags, ret;
	u_register_t new_ipa_base, new_ipa_top;
	rsi_response_type response;
	rsi_ripas_type ripas;

	realm_printf("Validating device memory @0x%lx [0x%lx-0x%lx] mappings\n",
			pa_base, ipa_base, ipa_top - 1UL);

	flags = INPLACE(RSI_DEV_MEM_FLAGS_COH, RSI_DEV_MEM_NON_COHERENT) |
		INPLACE(RSI_DEV_MEM_FLAGS_LOR, RSI_DEV_MEM_NOT_LIMITED_ORDER);

	new_ipa_base = ipa_base;

	while (new_ipa_base < ipa_top) {
		ret = rsi_vdev_vaildate_mapping(rdev->id, new_ipa_base,
						ipa_top, pa_base, flags,
						vdev_info->lock_nonce,
						vdev_info->meas_nonce,
						vdev_info->report_nonce,
						&new_ipa_base, &response);
		if ((ret != RSI_SUCCESS) || (response != RSI_ACCEPT)) {
			realm_printf("%s() failed, 0x%lx\n", "rsi_vdev_vaildate_mapping", ret);
			return false;
		}

		pa_base = new_ipa_base;
	}

	/* Verify that RIAS has changed for [base, top) range */
	ret = rsi_ipa_state_get(ipa_base, ipa_top, &new_ipa_top, &ripas);
	if ((ret != RSI_SUCCESS) || (ripas != RSI_DEV) || (new_ipa_top != ipa_top)) {
		ERROR("rsi_ipa_state_get() failed, 0x%lx ipa_base=0x%lx ipa_top=0x%lx",
				"new_ipa_top=0x%lx ripas=%u\n",
				ret, ipa_base, ipa_top, new_ipa_top, ripas);
		return false;
	}

	realm_printf("Device memory [0x%lx-0x%lx] mappings verified\n",
			ipa_base, ipa_top - 1UL);
	return true;
}

bool test_realm_smmuv3(void)
{
	u_register_t rsi_feature_reg0, rsi_rc;
	u_register_t flags, ret;
	u_register_t range_count, non_ats_plane;
	struct rdev *rdev;
	struct rsi_vdev_info *vdev_info;
	struct rmi_address_range addr_range[MAX_ADDR_RANGE_NUM];
	bool res = true;
	int rc;

	/* Check if RSI_FEATURES support DA */
	rsi_rc = rsi_features(RSI_FEATURE_REGISTER_0_INDEX, &rsi_feature_reg0);
	if (rsi_rc != RSI_SUCCESS) {
		return false;
	}

	if (EXTRACT(RSI_FEATURE_REGISTER_0_DA, rsi_feature_reg0) !=
		RSI_FEATURE_TRUE) {
		realm_printf("RSI feature DA not supported for current Realm\n");
		return false;
	}

	/* Get the global RDEV. Currently only one RDEV is supported */
	rdev = &gbl_rdev;

	if (rdev == NULL) {
		realm_printf("realm_rdev_init failed\n");
		return false;
	}

	/* Use idx 1 to have struct size alignment */
	vdev_info = &gbl_vdev_info[1];

	/* After meas and ifc_report, get device info */
	rsi_rc = realm_rsi_vdev_get_info(rdev, vdev_info);
	if (rsi_rc != RSI_SUCCESS) {
		return false;
	}

	/* Get MMIO ranges from Host */
	range_count = realm_shared_data_get_mmio_range(&addr_range[0]);

	for (unsigned int i = 0U; i < range_count; i++) {
		realm_printf("addr_range[%u]: 0x%lx-0x%lx\n",
			i, addr_range[i].base, addr_range[i].top - 1UL);
	}

	flags = INPLACE(RSI_DEV_MEM_FLAGS_COH, RSI_DEV_MEM_NON_COHERENT) |
		INPLACE(RSI_DEV_MEM_FLAGS_LOR, RSI_DEV_MEM_NOT_LIMITED_ORDER);

	for (unsigned long i = 0UL; i < range_count; i++) {
		u_register_t ipa_base = addr_range[i].base;
		u_register_t ipa_top = addr_range[i].top;
		/* 1:1 mapping */
		u_register_t pa_base = ipa_base;

		if (!realm_validate_mapping(rdev, vdev_info,
					    ipa_base, ipa_top, pa_base)) {
			return false;
		}
	}

	ret = rsi_realm_config(&realm_config);
	if (ret != RSI_SUCCESS) {
		ERROR("%s() failed, 0x%lx", "rsi_realm_config", ret);
		return false;
	}

	flags = INPLACE(RSI_VDEV_DMA_FLAGS_ATS, RSI_FEATURE_FALSE);

	/*
	 * TODO:
	 * Get 'non_ats_plane' from VDEV when implemented in RMM.
	 */
	non_ats_plane = realm_config.ats_plane + 1UL;
	if (non_ats_plane > realm_config.num_aux_planes) {
		non_ats_plane = 0UL;
	}

	ret = rsi_vdev_dma_enable(rdev->id, flags, non_ats_plane,
					vdev_info->lock_nonce,
					vdev_info->meas_nonce,
					vdev_info->report_nonce);
	if (ret != RSI_SUCCESS) {
		ERROR("%s() failed, 0x%lx", "rsi_vdev_dma_enable", ret);
		return false;
	}

	for (unsigned long i = 0U; i < sizeof(src_buf); i++) {
		src_buf[i] = i % 255;
	}

	realm_printf("DMA copy 0x%lx -> 0x%lx\n",
			(uintptr_t)src_buf, (uintptr_t)dst_buf);

	rc = smmute_memcpy_dma(addr_range[0].base, (uintptr_t)src_buf, (uintptr_t)dst_buf);

	realm_printf("Source:\n");
	dump_memory(src_buf, 32U);

	realm_printf("Destination:\n");
	dump_memory(dst_buf, 32U);

	/* Check SMMUv3TestEngine return code and memcpm() result */
	res = ((rc >= 0) && (memcmp((const void *)src_buf,
				(const void *)dst_buf, PAGE_SIZE) == 0));

	realm_printf("DMA copy %s\n", res ? "successful" : "failed");

	ret = rsi_vdev_dma_disable(rdev->id);
	if (ret != RSI_SUCCESS) {
		ERROR("%s() failed, 0x%lx", "rsi_vdev_dma_disable", ret);
		return false;
	}

	return res;
}
