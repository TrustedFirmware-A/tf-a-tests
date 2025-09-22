/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <uuid.h>
#include <io_storage.h>
#include <platform.h>
#include <platform_def.h>
#include <psci.h>
#include <smccc.h>
#include <status.h>
#include <tftf_lib.h>
#include <uuid_utils.h>
#include "neg_scenario_test_infra.h"

#define CRYPTO_SUPPORT 1

static fip_toc_entry_t *
find_fiptoc_entry_t(const int fip_base, const uuid_t *uuid)
{
	fip_toc_entry_t *current_file =
		(fip_toc_entry_t *) (fip_base + sizeof(fip_toc_header_t));

	while (!is_uuid_null(&(current_file->uuid))) {
		if (uuid_equal(&(current_file->uuid), uuid)){
			return current_file;
		}

		current_file += 1;
	};

	return NULL;
}

test_result_t test_invalid_rotpk(void)
{
	smc_args args = { SMC_PSCI_SYSTEM_RESET };
	smc_ret_values ret = (smc_ret_values){0};
	const uuid_t trusted_cert = UUID_TRUSTED_KEY_CERT;

	uintptr_t handle;
	fip_toc_entry_t * cert;
	size_t exp_len, len;
	int address, rc;
	void * paramOut = NULL;

	if(tftf_is_rebooted() ){
		/* ROTPK is tampered with and upon reboot tfa should not reach this point */
		return TEST_RESULT_FAIL;
	}

	/* Locate Trusted Key certificate memory address by using UUID */
	cert = find_fiptoc_entry_t(PLAT_ARM_FIP_BASE, &trusted_cert);
	if (cert == NULL){
		return TEST_RESULT_FAIL;
	}

	address = (uintptr_t)cert->offset_address;
	exp_len = cert->size;
	if (exp_len == 0U){
		return TEST_RESULT_FAIL;
	}

	/* Runtime-sized buffer on stack */
	uint8_t cert_buffer[exp_len];

	/* Open NVM and Read certicate */
	plat_get_nvm_handle(&handle);
	if(handle < 0) {
		return TEST_RESULT_FAIL;
	}

	rc = io_seek(handle, IO_SEEK_SET, address);
	if (rc < 0){
		return TEST_RESULT_FAIL;
	}

	rc = io_read(handle, (uintptr_t) &cert_buffer, exp_len, &len);
	if (rc < 0 || len != exp_len){
		return TEST_RESULT_FAIL;
	}

	/* Parse certifacte to retrieve public key */
	rc = get_pubKey_from_cert(&cert_buffer, len, &paramOut);
	if ( rc != 0){
		return TEST_RESULT_FAIL;
	}

	/*
	* Corrupt part of the certificate in storage.
	* Simple overwrite: just clobber the first 32 bytes so parsing/verification fails.
	*/
	{
		uint8_t junk[32] = {0};

		rc = io_seek(handle, IO_SEEK_SET, address);
		if (rc < 0){
			return TEST_RESULT_FAIL;
		}

		rc = io_write(handle, (uintptr_t)junk, sizeof(junk), &len);
		if (rc < 0 || len != sizeof(junk)){
			return TEST_RESULT_FAIL;
		}
	}

	/* Reboot */
	tftf_notify_reboot();
	ret = tftf_smc(&args);

	/* The PSCI SYSTEM_RESET call is not supposed to return */
	tftf_testcase_printf("System didn't reboot properly (%d)\n",
						(unsigned int)ret.ret0);

	/* If this point is reached, reboot failed to trigger*/
	return TEST_RESULT_FAIL;
}
