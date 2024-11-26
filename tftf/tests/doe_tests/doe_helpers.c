/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

#include <debug.h>
#include <pcie.h>
#include <pcie_doe.h>
#include <spdm.h>

void pcie_init(void)
{
	static bool is_init;

	/* Create PCIe table and enumeration */
	if (!is_init) {
		pcie_create_info_table();
		is_init = true;
	}
}

/*
 * @brief  Returns the BDF Table pointer
 *
 * @param  None
 *
 * @return BDF Table pointer
 */
pcie_device_bdf_table_t *get_pcie_bdf_table(void)
{
	return pcie_get_bdf_table();
}

int find_doe_device(uint32_t *bdf_ptr, uint32_t *cap_base_ptr)
{
	pcie_device_bdf_table_t	*bdf_table_ptr = pcie_get_bdf_table();
	uint32_t num_bdf = bdf_table_ptr->num_entries;

	INFO("PCI BDF table entries: %u\n", num_bdf);

	/* If no entries in BDF table return error */
	if (num_bdf == 0) {
		ERROR("No BDFs entries found\n");
		return -ENODEV;
	}

	INFO("PCI BDF table 0x%lx\n", (uintptr_t)bdf_table_ptr);

	while (num_bdf-- != 0) {
		uint32_t bdf = bdf_table_ptr->device[num_bdf].bdf;
		uint32_t status, doe_cap_base;

		/* Check for DOE capability */
		status = pcie_find_capability(bdf, PCIE_ECAP, DOE_CAP_ID, &doe_cap_base);
		if (status == PCIE_SUCCESS) {
			INFO("PCIe DOE capability: bdf 0x%x cap_base 0x%x\n", bdf, doe_cap_base);
			*bdf_ptr = bdf;
			*cap_base_ptr = doe_cap_base;
			return 0;
		}
	}

	ERROR("No PCIe DOE capability found\n");
	return -ENODEV;
}

int get_spdm_version(uint32_t bdf, uint32_t doe_cap_base)
{
	uint32_t response[SPDM_GET_VERS_RESP_LEN], resp_len;
	spdm_version_response_t *ver_resp;
	spdm_version_number_t *ver_ptr;
	uint8_t entry_count;
	int ret;
	const spdm_get_version_request_t ver_req = {
		{ SPDM_MESSAGE_VERSION, SPDM_GET_VERSION, }
	};

	ret = pcie_doe_send_req(DOE_HEADER_1, bdf, doe_cap_base,
				(uint32_t *)&ver_req,
				sizeof(spdm_get_version_request_t));
	if (ret != 0) {
		return ret;
	}

	ret = pcie_doe_recv_resp(bdf, doe_cap_base, response, &resp_len);
	if (ret != 0) {
		return ret;
	}

	ver_resp = (spdm_version_response_t *)response;

	if (ver_resp->header.spdm_version != SPDM_MESSAGE_VERSION) {
		ERROR("SPDM response v.%u doesn't match requested %u\n",
			ver_resp->header.spdm_version,
			SPDM_MESSAGE_VERSION);
		return -EPROGMISMATCH;
	}

	if (ver_resp->header.request_response_code != SPDM_VERSION) {
		ERROR("SPDM response code %u doesn't match expected %u\n",
			ver_resp->header.request_response_code,
			SPDM_VERSION);
		return -EBADMSG;
	}

	entry_count = ver_resp->version_number_entry_count;
	INFO("SPDM version entries: %u\n", entry_count);

	ver_ptr = (spdm_version_number_t *)(
			(uintptr_t)&ver_resp->version_number_entry_count +
			sizeof(ver_resp->version_number_entry_count));

	while (entry_count-- != 0) {
		spdm_version_number_t ver __unused = *ver_ptr++;

		INFO("SPDM v%llu.%llu.%llu.%llu\n",
			EXTRACT(SPDM_VER_MAJOR, ver),
			EXTRACT(SPDM_VER_MINOR, ver),
			EXTRACT(SPDM_VER_UPDATE, ver),
			EXTRACT(SPDM_VER_ALPHA, ver));
	}
	return ret;
}

int doe_discovery(uint32_t bdf, uint32_t doe_cap_base)
{
	pcie_doe_disc_req_t request = { 0, };
	pcie_doe_disc_resp_t response;
	uint32_t resp_len;
	int ret;

	do {
		ret = pcie_doe_send_req(DOE_HEADER_0, bdf, doe_cap_base,
					(uint32_t *)&request,
					sizeof(pcie_doe_disc_req_t));
		if (ret != 0) {
			return ret;
		}

		ret = pcie_doe_recv_resp(bdf, doe_cap_base,
					(uint32_t *)&response, &resp_len);
		if (ret != 0) {
			return ret;
		}

		print_doe_disc(&response);
		request.index = response.next_index;

	} while (response.next_index != 0);

	return 0;
}
