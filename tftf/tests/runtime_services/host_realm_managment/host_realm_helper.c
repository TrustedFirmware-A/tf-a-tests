/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdint.h>

#include <arch_helpers.h>
#include <debug.h>
#include <events.h>
#include <heap/page_alloc.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_realm_rmi.h>
#include <host_shared_data.h>
#include <plat_topology.h>
#include <power_management.h>
#include <realm_def.h>
#include <test_helpers.h>
#include <xlat_tables_v2.h>

static struct realm realm;
static bool realm_payload_created;
static bool shared_mem_created;
static volatile bool timer_enabled;

#define RMI_EXIT(id)	\
	[RMI_EXIT_##id] = #id

const char *rmi_exit[] = {
	RMI_EXIT(SYNC),
	RMI_EXIT(IRQ),
	RMI_EXIT(FIQ),
	RMI_EXIT(FIQ),
	RMI_EXIT(PSCI),
	RMI_EXIT(RIPAS_CHANGE),
	RMI_EXIT(HOST_CALL),
	RMI_EXIT(SERROR)
};

/*
 * The function handler to print the Realm logged buffer,
 * executed by the secondary core
 */
void realm_print_handler(unsigned int rec_num)
{
	size_t str_len = 0UL;
	host_shared_data_t *host_shared_data = host_get_shared_structure(rec_num);
	char *log_buffer = (char *)host_shared_data->log_buffer;

	str_len = strlen((const char *)log_buffer);

	/*
	 * Read Realm message from shared printf location and print
	 * them using UART
	 */
	if (str_len != 0UL) {
		/* Avoid memory overflow */
		log_buffer[MAX_BUF_SIZE - 1] = 0U;
		mp_printf("%s", log_buffer);
		(void)memset((char *)log_buffer, 0, MAX_BUF_SIZE);
	}
}

/*
 * Initialisation function which will clear the shared region,
 * and try to find another CPU other than the lead one to
 * handle the Realm message logging.
 */
static void host_init_realm_print_buffer(void)
{
	host_shared_data_t *host_shared_data;

	for (unsigned int i = 0U; i < realm.rec_count; i++) {
		host_shared_data = host_get_shared_structure(i);
		(void)memset((char *)host_shared_data, 0, sizeof(host_shared_data_t));
	}
}

static bool host_enter_realm(u_register_t *exit_reason,
		unsigned int *host_call_result, unsigned int rec_num)
{
	u_register_t ret;

	if (!realm_payload_created) {
		ERROR("%s() failed\n", "realm_payload_created");
		return false;
	}
	if (!shared_mem_created) {
		ERROR("%s() failed\n", "shared_mem_created");
		return false;
	}

	/* Enter Realm */
	ret = host_realm_rec_enter(&realm, exit_reason, host_call_result, rec_num);
	if (ret != REALM_SUCCESS) {
		ERROR("%s() failed, ret=%lx\n", "host_realm_rec_enter", ret);
		return false;
	}

	return true;
}

bool host_create_realm_payload(u_register_t realm_payload_adr,
				u_register_t plat_mem_pool_adr,
				u_register_t plat_mem_pool_size,
				u_register_t realm_pages_size,
				u_register_t feature_flag,
				const u_register_t *rec_flag,
				unsigned int rec_count)
{
	int8_t value;

	if (realm_payload_adr == TFTF_BASE) {
		ERROR("realm_payload_adr should be grater then TFTF_BASE\n");
		return false;
	}

	if (plat_mem_pool_adr  == 0UL ||
			plat_mem_pool_size == 0UL ||
			realm_pages_size == 0UL) {
		ERROR("plat_mem_pool_size or "
			"plat_mem_pool_size or realm_pages_size is NULL\n");
		return false;
	}

	INFO("Realm base adr=0x%lx\n", realm_payload_adr);
	/* Initialize  Host NS heap memory to be used in Realm creation*/
	if (page_pool_init(plat_mem_pool_adr, realm_pages_size)
		!= HEAP_INIT_SUCCESS) {
		ERROR("%s() failed\n", "page_pool_init");
		return false;
	}

	/* Read Realm Feature Reg 0 */
	if (host_rmi_features(0UL, &realm.rmm_feat_reg0) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_rmi_features");
		return false;
	}

	/* Disable PMU if not required */
	if ((feature_flag & RMI_FEATURE_REGISTER_0_PMU_EN) == 0UL) {
		realm.rmm_feat_reg0 &= ~RMI_FEATURE_REGISTER_0_PMU_EN;
		realm.pmu_num_ctrs = 0U;
	} else {
		value = EXTRACT(FEATURE_PMU_NUM_CTRS, feature_flag);
		if (value != -1) {
			realm.pmu_num_ctrs = (unsigned int)value;
		} else {
			realm.pmu_num_ctrs =
				EXTRACT(RMI_FEATURE_REGISTER_0_PMU_NUM_CTRS,
					realm.rmm_feat_reg0);
		}
	}

	/* Disable SVE if not required */
	if ((feature_flag & RMI_FEATURE_REGISTER_0_SVE_EN) == 0UL) {
		realm.rmm_feat_reg0 &= ~RMI_FEATURE_REGISTER_0_SVE_EN;
		realm.sve_vl = 0U;
	} else {
		realm.sve_vl = EXTRACT(FEATURE_SVE_VL, feature_flag);
	}

	/* Requested number of breakpoints */
	value = EXTRACT(FEATURE_NUM_BPS, feature_flag);
	if (value != -1) {
		realm.num_bps = (unsigned int)value;
	} else {
		realm.num_bps = EXTRACT(RMI_FEATURE_REGISTER_0_NUM_BPS,
					realm.rmm_feat_reg0);
	}

	/* Requested number of watchpoints */
	value = EXTRACT(FEATURE_NUM_WPS, feature_flag);
	if (value != -1) {
		realm.num_wps = (unsigned int)value;
	} else {
		realm.num_wps = EXTRACT(RMI_FEATURE_REGISTER_0_NUM_WPS,
					realm.rmm_feat_reg0);
	}

	/* Set SVE bits from feature_flag */
	realm.rmm_feat_reg0 &= ~(RMI_FEATURE_REGISTER_0_SVE_EN |
				 MASK(RMI_FEATURE_REGISTER_0_SVE_VL));
	if ((feature_flag & RMI_FEATURE_REGISTER_0_SVE_EN) != 0UL) {
		realm.rmm_feat_reg0 |= RMI_FEATURE_REGISTER_0_SVE_EN |
				       INPLACE(RMI_FEATURE_REGISTER_0_SVE_VL,
				       EXTRACT(RMI_FEATURE_REGISTER_0_SVE_VL,
						feature_flag));
	}

	if (realm.rec_count > MAX_REC_COUNT) {
		ERROR("Invalid Rec Count\n");
		return false;
	}
	realm.rec_count = rec_count;
	for (unsigned int i = 0U; i < rec_count; i++) {
		if (rec_flag[i] == RMI_RUNNABLE ||
				rec_flag[i] == RMI_NOT_RUNNABLE) {
			realm.rec_flag[i] = rec_flag[i];
		} else {
			ERROR("Invalid Rec Flag\n");
			return false;
		}
	}

	/* Create Realm */
	if (host_realm_create(&realm) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_create");
		return false;
	}

	if (host_realm_init_ipa_state(&realm, 0U, 0U, 1ULL << 32)
		!= RMI_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_init_ipa_state");
		goto destroy_realm;
	}

	/* RTT map Realm image */
	if (host_realm_map_payload_image(&realm, realm_payload_adr) !=
			REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_map_payload_image");
		goto destroy_realm;
	}

	/* Create REC */
	if (host_realm_rec_create(&realm) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_rec_create");
		goto destroy_realm;
	}

	/* Activate Realm */
	if (host_realm_activate(&realm) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_activate");
		goto destroy_realm;
	}

	realm_payload_created = true;

	return realm_payload_created;

	/* Free test resources */
destroy_realm:
	if (host_realm_destroy(&realm) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_destroy");
	}
	realm_payload_created = false;

	return realm_payload_created;
}

bool host_create_shared_mem(u_register_t ns_shared_mem_adr,
	u_register_t ns_shared_mem_size)
{
	/* RTT map NS shared region */
	if (host_realm_map_ns_shared(&realm, ns_shared_mem_adr,
				ns_shared_mem_size) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_map_ns_shared");
		shared_mem_created = false;
		return false;
	}

	memset((void *)ns_shared_mem_adr, 0, (size_t)ns_shared_mem_size);
	host_init_realm_print_buffer();
	shared_mem_created = true;

	return shared_mem_created;
}

bool host_destroy_realm(void)
{
	/* Free test resources */
	timer_enabled = false;
	page_pool_reset();

	if (!realm_payload_created) {
		ERROR("%s() failed\n", "realm_payload_created");
		return false;
	}

	realm_payload_created = false;
	if (host_realm_destroy(&realm) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_destroy");
		return false;
	}

	return true;
}

bool host_enter_realm_execute(uint8_t cmd, struct realm **realm_ptr,
		int test_exit_reason, unsigned int rec_num)
{
	u_register_t exit_reason = RMI_EXIT_INVALID;
	unsigned int host_call_result = TEST_RESULT_FAIL;

	if (rec_num >= realm.rec_count) {
		ERROR("Invalid Rec Count\n");
		return false;
	}
	host_shared_data_set_realm_cmd(cmd, rec_num);
	if (!host_enter_realm(&exit_reason, &host_call_result, rec_num)) {
		return false;
	}

	if (realm_ptr != NULL) {
		*realm_ptr = &realm;
	}

	if ((exit_reason == RMI_EXIT_HOST_CALL) && (host_call_result == TEST_RESULT_SUCCESS)) {
		return true;
	}

	if (test_exit_reason == exit_reason) {
		 return true;
	}

	if (exit_reason <= RMI_EXIT_SERROR) {
		ERROR("%s(%u) RMI_EXIT_%s host_call_result=%u\n",
		__func__, cmd, rmi_exit[exit_reason], host_call_result);
	} else {
		ERROR("%s(%u) 0x%lx host_call_result=%u\n",
		__func__, cmd, exit_reason, host_call_result);
	}
	return false;
}

test_result_t host_cmp_result(void)
{
	if (host_rmi_get_cmp_result()) {
		return TEST_RESULT_SUCCESS;
	}

	ERROR("RMI registers comparison failed\n");
	return TEST_RESULT_FAIL;
}

