/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_features.h>
#include <arch_helpers.h>
#include <debug.h>
#include <firme.h>
#include <pcie.h>
#include <pcie_spec.h>
#include <test_helpers.h>
#include <tftf_lib.h>

/* IDE KM service feature register definitions. */
#define FIRME_IDE_KM_FR0_KEYSET_PROG			BIT(0)
#define FIRME_IDE_KM_FR0_KEYSET_GO			BIT(1)
#define FIRME_IDE_KM_FR0_KEYSET_STOP			BIT(2)
#define FIRME_IDE_KM_FR0_KEYSET_POLL			BIT(3)

/* KeySet ID */
#define FIRME_KEYSET_ID_KEYSET_SHIFT			U(0)
#define FIRME_KEYSET_ID_KEYSET_WIDTH			U(1)
#define FIRME_KEYSET_ID_DIRECTION_SHIFT			U(1)
#define FIRME_KEYSET_ID_DIRECTION_WIDTH			U(1)
#define FIRME_KEYSET_ID_SUBSTREAM_ID_SHIFT		U(2)
#define FIRME_KEYSET_ID_SUBSTREAM_ID_WIDTH		U(4)
#define FIRME_KEYSET_ID_STREAM_ID_SHIFT			U(6)
#define FIRME_KEYSET_ID_STREAM_ID_WIDTH			U(8)
#define FIRME_KEYSET_ID_ROOTPORT_ID_SHIFT		U(14)
#define FIRME_KEYSET_ID_ROOTPORT_ID_WIDTH		U(16)
#define FIRME_KEYSET_ID_SEGMENT_NUMBER_SHIFT		U(30)
#define FIRME_KEYSET_ID_SEGMENT_NUMBER_WIDTH		U(8)

#define RP_TYPE_PCIE					0
#define RP_TYPE_PCIE_CXL				1

/* SUB_STREAM_PR, SUB_STREAM_NPR, SUB_STREAM_CPL */
#define IDE_SUBSTREAM_MAX				U(3)

#define IDE_KM_KEY_DIRECTION_TX				0
#define IDE_KM_KEY_DIRECTION_RX				1

/* Default busy abd poll timeout in milliseconds  */
#define DEFAULT_BUSY_TIMEOUT				(5)
#define DEFAULT_POLL_TIMEOUT				(10)

#define FIRME_RETRY_MAX					U(20)

/* Construct Keyset ID from key set, key direction, key substream, stream id */
#define FIRME_IDE_MAKE_KEYSET_ID(_kset, _dir, _substream, _stream, _rp, _seg) \
	(INPLACE(FIRME_KEYSET_ID_KEYSET, (_kset))			| \
	 INPLACE(FIRME_KEYSET_ID_DIRECTION, (_dir))			| \
	 INPLACE(FIRME_KEYSET_ID_SUBSTREAM_ID, (_substream))		| \
	 INPLACE(FIRME_KEYSET_ID_STREAM_ID, (_stream))			| \
	 INPLACE(FIRME_KEYSET_ID_ROOTPORT_ID, (_rp))			| \
	 INPLACE(FIRME_KEYSET_ID_SEGMENT_NUMBER, (_seg)))

struct ide_key {
	uint64_t KeyQW[4];
};

/* List of PCIe RootPorts with IDE support */
#define HOST_PCIE_RP_MAX		(32)
static unsigned int gbl_host_pcie_rps_count;
static pcie_dev_t *gbl_host_pcie_rps[HOST_PCIE_RP_MAX];

static void host_pcie_rps_init(void)
{
	static bool gbl_host_pcie_rps_init_done;
	pcie_device_bdf_table_t *bdf_table;
	unsigned int i;
	unsigned int cnt = 0U;
	pcie_dev_t *dev;

	if (gbl_host_pcie_rps_init_done) {
		return;
	}

	/* When called for the first time this does PCIe enumeration */
	pcie_init();

	INFO("Initializing host PCIe root ports\n");
	bdf_table = pcie_get_bdf_table();
	if ((bdf_table == NULL) || (bdf_table->num_entries == 0)) {
		goto out_init;
	}

	for (i = 0U; i < bdf_table->num_entries; i++) {
		dev = &bdf_table->device[i];

		if (dev->dp_type != RP) {
			continue;
		}

		/* Initialize host_pcie_rps */
		gbl_host_pcie_rps[cnt] = dev;
		cnt++;

		if (cnt == HOST_PCIE_RP_MAX) {
			WARN("Max host PCIe root ports count reached.\n");
			break;
		}
	}

out_init:
	gbl_host_pcie_rps_init_done = true;
	gbl_host_pcie_rps_count = cnt;
}

static pcie_dev_t *get_any_pcie_rp(int rp_type, bool with_ide)
{
	if (rp_type != RP_TYPE_PCIE) {
		return NULL;
	}

	for (int i = 0; i < gbl_host_pcie_rps_count; i++) {
		if (with_ide && pcie_dev_has_ide(gbl_host_pcie_rps[i])) {
			return gbl_host_pcie_rps[i];
		}

		if (!with_ide && !pcie_dev_has_ide(gbl_host_pcie_rps[i])) {
			return gbl_host_pcie_rps[i];
		}
	}

	return NULL;
}

static bool prereqs_met_for_firme_ide_test(void)
{
	uint64_t required_abis;
	uint64_t feat_reg;
	int32_t res;

	res = firme_features(FIRME_BASE_SERVICE_ID, 1, &feat_reg);
	if (res != FIRME_SUCCESS) {
		tftf_testcase_printf("FIRME not supported!\n");
		return false;
	}

	if ((feat_reg & FIRME_BASE_SERVICE_IDE_KM_BIT) == 0) {
		tftf_testcase_printf("FIRME IDE KM not supported!\n");
		return false;
	}

	res = firme_features(FIRME_IDE_KM_SERVICE_ID, 0, &feat_reg);
	if (res != FIRME_SUCCESS) {
		tftf_testcase_printf("FIRME IDE KM - get_feature_reg0 failed\n");
		return false;
	}

	required_abis = (FIRME_IDE_KM_FR0_KEYSET_PROG |
			 FIRME_IDE_KM_FR0_KEYSET_GO |
			 FIRME_IDE_KM_FR0_KEYSET_STOP);

	/* Check feature reg0 against required ABIs */
	if ((feat_reg & required_abis) != required_abis) {
		tftf_testcase_printf("FIRME IDE KM - Missing base ABIs\n");
		return false;
	}

	/* Init host_pcie_rps global array */
	host_pcie_rps_init();

	if (gbl_host_pcie_rps_count == 0U) {
		WARN("No PCIe RootPorts detected, skipping\n");
		return false;
	}

	return true;
}

static int do_firme_ide_km_keyset_poll(unsigned long ecam_base,
				       uint64_t keyset_id, uint64_t *handle_ret)
{
	int rc;
	int retry = 0;

	INFO("FIRME IDE KM poll for keyset: 0x%llx\n", keyset_id);
	do {
		rc = firme_ide_km_keyset_poll(ecam_base, keyset_id, handle_ret);

		INFO("do_firme_ide_km_keyset_poll rc: %d\n", rc);
		if (rc == FIRME_INCOMPLETE) {
			VERBOSE("FIRME_INCOMPLETE: delay %dms\n",
				DEFAULT_POLL_TIMEOUT);
			waitms(DEFAULT_POLL_TIMEOUT);
		}

		if (rc == FIRME_BUSY) {
			VERBOSE("FIRME_BUSY: delay %dms\n",
				DEFAULT_BUSY_TIMEOUT);
			waitms(DEFAULT_BUSY_TIMEOUT);
		}
	} while ((++retry < FIRME_RETRY_MAX) && ((rc == FIRME_INCOMPLETE) ||
						 (rc == FIRME_BUSY)));

	return (rc == FIRME_SUCCESS) ? 0 : -1;
}

static int do_firme_ide_km_poll(unsigned long ecam_base, uint64_t *handle_ret,
				uint64_t *keyset_id_ret)
{
	int rc;
	int retry = 0;

	INFO("FIRME IDE KM poll for any keyset\n");
	do {
		rc = firme_ide_km_poll(ecam_base, handle_ret, keyset_id_ret);

		if (rc == FIRME_INCOMPLETE) {
			VERBOSE("FIRME_INCOMPLETE: delay %dms\n",
				DEFAULT_POLL_TIMEOUT);
			waitms(DEFAULT_POLL_TIMEOUT);
		}

		if (rc == FIRME_BUSY) {
			VERBOSE("FIRME_BUSY: delay %dms\n",
				DEFAULT_BUSY_TIMEOUT);
			waitms(DEFAULT_BUSY_TIMEOUT);
		}
	} while ((++retry < FIRME_RETRY_MAX) && ((rc == FIRME_INCOMPLETE) ||
						 (rc == FIRME_BUSY)));

	return (rc == FIRME_SUCCESS) ? 0 : -1;
}

static int do_firme_ide_km_prog(pcie_dev_t *rp_dev, struct ide_key *key,
				int kslot, int sid, int ssid, int dir)
{
	int rc;
	uint64_t keyset_id;
	uint64_t handle, handle_polled;
	int retry = 0;

	handle = rand64();
	keyset_id = FIRME_IDE_MAKE_KEYSET_ID(kslot, dir, ssid, sid,
					     rp_dev->bdf, 0);
	do {
		rc = firme_ide_km_keyset_prog(rp_dev->ecam_base, 0x0,
					      keyset_id, key->KeyQW[0],
					      key->KeyQW[1], key->KeyQW[2],
					      key->KeyQW[3], handle);
		if (rc == FIRME_BUSY) {
			VERBOSE("ide_km_prog: FIRME_BUSY: delay %dms\n",
				DEFAULT_BUSY_TIMEOUT);
			waitms(DEFAULT_BUSY_TIMEOUT);
		}
	} while ((++retry < FIRME_RETRY_MAX) && (rc == FIRME_BUSY));

	/* Poll for completion */
	if (rc == FIRME_INCOMPLETE) {
		rc =  do_firme_ide_km_keyset_poll(rp_dev->ecam_base, keyset_id,
						  &handle_polled);
		if ((rc != 0) || (handle != handle_polled)) {
			rc = -1;
		}
	}

	return rc;
}

static int do_firme_ide_km_go(pcie_dev_t *rp_dev, int kslot, int sid, int ssid,
			      int dir)
{
	int rc;
	uint64_t keyset_id;
	uint64_t handle, handle_polled;
	int retry = 0;

	handle = rand64();
	keyset_id = FIRME_IDE_MAKE_KEYSET_ID(kslot, dir, ssid, sid, rp_dev->bdf,
					     0);
	do {
		rc = firme_ide_km_keyset_go(rp_dev->ecam_base, 0x0, keyset_id,
					    handle);
		if (rc == FIRME_BUSY) {
			VERBOSE("ide_km_go: FIRME_BUSY: delay %dms\n",
				DEFAULT_BUSY_TIMEOUT);
			waitms(DEFAULT_BUSY_TIMEOUT);
		}
	} while ((++retry < FIRME_RETRY_MAX) && (rc == FIRME_BUSY));

	/* Poll for completion */
	if (rc == FIRME_INCOMPLETE) {
		rc =  do_firme_ide_km_keyset_poll(rp_dev->ecam_base, keyset_id,
						  &handle_polled);
		if ((rc != 0) || (handle != handle_polled)) {
			rc = -1;
		}
	}

	return rc;
}

static int do_firme_ide_km_stop(pcie_dev_t *rp_dev, int kslot, int sid, int ssid,
				int dir)
{
	int rc;
	uint64_t keyset_id;
	uint64_t handle, handle_polled;
	int retry = 0;

	handle = rand64();
	keyset_id = FIRME_IDE_MAKE_KEYSET_ID(kslot, dir, ssid, sid, rp_dev->bdf,
					     0);
	do {
		rc = firme_ide_km_keyset_stop(rp_dev->ecam_base, 0x0, keyset_id,
					      handle);
		if (rc == FIRME_BUSY) {
			VERBOSE("ide_km_stop: FIRME_BUSY: delay %dms\n",
				DEFAULT_BUSY_TIMEOUT);
			waitms(DEFAULT_BUSY_TIMEOUT);
		}
	} while ((++retry < FIRME_RETRY_MAX) && (rc == FIRME_BUSY));

	/* Poll for completion */
	if (rc == FIRME_INCOMPLETE) {
		rc =  do_firme_ide_km_keyset_poll(rp_dev->ecam_base, keyset_id,
						  &handle_polled);
		if ((rc != 0) || (handle != handle_polled)) {
			rc = -1;
		}
	}

	return rc;
}

static int ide_refresh(pcie_dev_t *rp_dev, int sid, int kslot)
{
	int rc;
	int ssid;
	struct ide_key rx_key, tx_key;

	rx_key.KeyQW[0] = rand64();
	rx_key.KeyQW[1] = rand64();
	rx_key.KeyQW[2] = rand64();
	rx_key.KeyQW[3] = rand64();

	tx_key.KeyQW[0] = rand64();
	tx_key.KeyQW[1] = rand64();
	tx_key.KeyQW[2] = rand64();
	tx_key.KeyQW[3] = rand64();

	/* PCIe IDE_KM_KEY_PROG: upstream port RX/TX */

	/* FIRME IDE_KM_PROG at the rootport */
	for (ssid = 0; ssid < IDE_SUBSTREAM_MAX; ssid++) {
		INFO("FIRME IDE_KEY_PROG: RP bdf: 0x%x RX stream/substream "
		     "%d/%d\n", rp_dev->bdf, sid, ssid);
		rc = do_firme_ide_km_prog(rp_dev, &rx_key, kslot, sid, ssid,
					  IDE_KM_KEY_DIRECTION_RX);
		if (rc != 0) {
			return -1;
		}

		INFO("FIRME IDE_KEY_PROG: RP bdf: 0x%x TX stream/substream "
		     "%d/%d\n", rp_dev->bdf, sid, ssid);
		rc = do_firme_ide_km_prog(rp_dev, &tx_key, kslot, sid, ssid,
					  IDE_KM_KEY_DIRECTION_TX);
		if (rc != 0) {
			return -1;
		}
	}

	/* FIRME IDE_KM_GO at the rootport - RX */
	for (ssid = 0; ssid < IDE_SUBSTREAM_MAX; ssid++) {
		INFO("FIRME IDE_KEY_GO: RP bdf: 0x%x RX stream/substream "
		     "%d/%d\n", rp_dev->bdf, sid, ssid);
		rc = do_firme_ide_km_go(rp_dev, kslot, sid, ssid,
					IDE_KM_KEY_DIRECTION_RX);
		if (rc != 0) {
			return -1;
		}
	}

	/* FIRME IDE_KM_GO at the rootport - TX */
	for (ssid = 0; ssid < IDE_SUBSTREAM_MAX; ssid++) {
		INFO("FIRME IDE_KEY_GO: RP bdf: 0x%x TX stream/substream "
		     "%d/%d\n", rp_dev->bdf, sid, ssid);
		rc = do_firme_ide_km_go(rp_dev, kslot, sid, ssid,
					IDE_KM_KEY_DIRECTION_TX);
		if (rc != 0) {
			return -1;
		}
	}

	return 0;
}

static int ide_reset(pcie_dev_t *rp_dev, int sid, int kslot)
{
	int rc;
	int ssid;

	/* FIRME IDE_KM_STOP at the rootport - RX */
	for (ssid = 0; ssid < IDE_SUBSTREAM_MAX; ssid++) {
		INFO("FIRME IDE_KEY_STOP: RP bdf: 0x%x RX stream/substream "
		     "%d/%d\n", rp_dev->bdf, sid, ssid);
		rc = do_firme_ide_km_stop(rp_dev, kslot, sid, ssid,
					  IDE_KM_KEY_DIRECTION_RX);
		if (rc != 0) {
			return -1;
		}
	}

	/* FIRME IDE_KM_STOP at the rootport - TX */
	for (ssid = 0; ssid < IDE_SUBSTREAM_MAX; ssid++) {
		INFO("FIRME IDE_KEY_STOP: RP bdf: 0x%x TX stream/substream "
		     "%d/%d\n", rp_dev->bdf, sid, ssid);
		rc = do_firme_ide_km_stop(rp_dev, kslot, sid, ssid,
					  IDE_KM_KEY_DIRECTION_TX);
		if (rc != 0) {
			return -1;
		}
	}

	return rc;
}

/* Test the IDE KM feature registers ABI */
test_result_t test_firme_ide_km_features(void)
{
	uint64_t feat_reg;
	uint64_t expected1_fr0;
	uint64_t expected2_fr0;
	int32_t res;

	res = firme_features(FIRME_BASE_SERVICE_ID, 1, &feat_reg);
	if (res != FIRME_SUCCESS) {
		tftf_testcase_printf("FIRME not supported!\n");
		return TEST_RESULT_SKIPPED;
	}

	if ((feat_reg & FIRME_BASE_SERVICE_IDE_KM_BIT) == 0) {
		tftf_testcase_printf("FIRME IDE KM not supported for NS\n");
		return TEST_RESULT_SKIPPED;
	}

	tftf_testcase_printf("Checking IDE KM service feature register\n");

	feat_reg = 0xDEADBEEFDEADBEEF;
	res = firme_features(FIRME_IDE_KM_SERVICE_ID, 0, &feat_reg);
	if (res != FIRME_SUCCESS) {
		tftf_testcase_printf("Error: SMC call returned %d\n", res);
		return TEST_RESULT_FAIL;
	}

	expected1_fr0 = (FIRME_IDE_KM_FR0_KEYSET_PROG |
			 FIRME_IDE_KM_FR0_KEYSET_GO |
			 FIRME_IDE_KM_FR0_KEYSET_STOP);

	expected2_fr0 = expected1_fr0 | FIRME_IDE_KM_FR0_KEYSET_POLL;

	/* Check feature reg against expected value. */
	if ((feat_reg != expected1_fr0) && (feat_reg != expected2_fr0)) {
		tftf_testcase_printf("Error: received 0x%llx\n", feat_reg);
		return TEST_RESULT_FAIL;
	}

	/* Try again with invalid reg index 2. */
	res = firme_features(FIRME_IDE_KM_SERVICE_ID, 1, &feat_reg);
	if (res != FIRME_NOT_SUPPORTED) {
		tftf_testcase_printf("Error: Got invalid feature register: %d\n",
				     res);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * Secure PCIe IDE stream at RootPort
 *
 * While securing the stream/link This testcase verifies FIRME KM ABIs
 * FIRME_IDE_KEYSET_PROG//GO/POLL
 *
 * Reset the link, this verifies FIRME KM ABIs
 * FIRME_IDE_KEYSET_STOP/POLL
 */
test_result_t test_firme_ide_km_secure_pcie_ide_stream(void)
{
	int sid, kslot;
	pcie_dev_t *rp_dev;
	int rc;

	if (!prereqs_met_for_firme_ide_test()) {
		return TEST_RESULT_SKIPPED;
	}

	/* get a RootPort that has IDE support */
	rp_dev = get_any_pcie_rp(RP_TYPE_PCIE, true);
	if (!rp_dev) {
		WARN("No PCIe RP of type PCIE with IDE found, skipping\n");
		return TEST_RESULT_SKIPPED;
	}

	/* use the first stream ID */
	sid = 0;

	/* secure the link with keyslot 0 - (secure the stream) */
	kslot = 0;
	INFO("IDE setup at RP: 0x%x\n", rp_dev->bdf);
	rc = ide_refresh(rp_dev, sid, kslot);
	if (rc != 0) {
		tftf_testcase_printf("IDE setup failed\n");
		return TEST_RESULT_FAIL;
	}

	/* secure the link with keyslot 1 - (key refresh)  */
	kslot = 1;
	INFO("IDE refresh at RP: 0x%x\n", rp_dev->bdf);
	rc = ide_refresh(rp_dev, sid, kslot);
	if (rc != 0) {
		tftf_testcase_printf("IDE refresh failed\n");
		return TEST_RESULT_FAIL;
	}

	/* reset the stream */
	INFO("IDE reset at RP: 0x%x\n", rp_dev->bdf);
	rc = ide_reset(rp_dev, sid, kslot);
	if (rc != 0) {
		tftf_testcase_printf("IDE reset failed\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/* Verify FIRME IDE poll ABI for any pending keyset */
test_result_t test_firme_ide_km_poll_any_keyset(void)
{
	int rc;
	pcie_dev_t *rp_dev;
	int poll_cnt, match_cnt;
	uint64_t handle, keyset_id;
	int sid, ssid, kslot, seg, dir;
	uint64_t handles[IDE_SUBSTREAM_MAX] = { 0 };
	uint64_t keysets[IDE_SUBSTREAM_MAX] = { 0 };

	if (!prereqs_met_for_firme_ide_test()) {
		return TEST_RESULT_SKIPPED;
	}

	/* get a RootPort that has IDE support */
	rp_dev = get_any_pcie_rp(RP_TYPE_PCIE, true);
	if (!rp_dev) {
		WARN("No PCIe RP of type PCIE with IDE found, skipping\n");
		return TEST_RESULT_SKIPPED;
	}

	/* use the first stream ID */
	sid = 0;
	kslot = 0;
	seg = 0;
	dir = IDE_KM_KEY_DIRECTION_TX;

	/* Issue IDE KM prog at the rootport to be polled later */
	poll_cnt = 0;
	match_cnt = 0;
	for (ssid = 0; ssid < IDE_SUBSTREAM_MAX; ssid++) {
		int retry;

		retry = 0;
		handle = rand64();
		keyset_id = FIRME_IDE_MAKE_KEYSET_ID(kslot, dir, ssid, sid,
						     rp_dev->bdf, seg);
		do {
			rc = firme_ide_km_keyset_prog(rp_dev->ecam_base, 0x0,
						      keyset_id, rand64(),
						      rand64(), rand64(),
						      rand64(), handle);
			if (rc == FIRME_BUSY) {
				VERBOSE("ide_km_prog: FIRME_BUSY: delay %dms\n",
					DEFAULT_BUSY_TIMEOUT);
				waitms(DEFAULT_BUSY_TIMEOUT);
			}
		} while ((++retry < FIRME_RETRY_MAX) && (rc == FIRME_BUSY));

		if (retry >= FIRME_RETRY_MAX) {
			INFO("FIRME IDE max retry reached for keyset_prog\n");
		}

		if ((rc != FIRME_SUCCESS) && (rc != FIRME_INCOMPLETE)) {
			return TEST_RESULT_FAIL;
		}

		if (rc == FIRME_INCOMPLETE) {
			handles[poll_cnt] = handle;
			keysets[poll_cnt] = keyset_id;
			poll_cnt++;
		}
	}

	if (poll_cnt == 0) {
		return (rc == 0) ? TEST_RESULT_SUCCESS : TEST_RESULT_FAIL;
	}

	for (int i = 0; i < poll_cnt; i++) {
		if (handles[i] != 0) {
			INFO("IDE KM handle/keyset needs poll: 0x%llx/0x%llx\n",
			     handles[i], keysets[i]);
		}
	}

	/* Poll for completion */
	for (ssid = 0; ssid < poll_cnt; ssid++) {
		uint64_t handle_polled = 0;
		uint64_t keyset_polled = 0;

		rc =  do_firme_ide_km_poll(rp_dev->ecam_base, &handle_polled,
					   &keyset_polled);
		if (rc != 0) {
			break;
		}

		INFO("IDE KM polled: handle/keyset 0x%llx/0x%llx\n",
		     handle_polled, keyset_polled);

		/* Check the polled results if it matches the queued handles */
		for (int i = 0; i < poll_cnt; i++) {
			if (handles[i] == 0) {
				continue;
			}

			if ((handle_polled == handles[i]) &&
			    (keyset_polled == keysets[i])) {
				handles[i] = 0;
				match_cnt++;
				break;
			}
		}
	}

	return (poll_cnt == match_cnt) ? TEST_RESULT_SUCCESS : TEST_RESULT_FAIL;
}
