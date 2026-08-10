/*
 * Copyright (c) 2025-2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_defs.h"
#include "xpm_nodeid.h"
#include <timer.h>

#define TIMEOUT_MS     0x1000U

static void test_notify_cb(xpm_notifier *notifier);

xpm_notifier notifier = {
	.callback = test_notify_cb,
	.node = PM_DEV_USB_0,
	/*
	 * EVENT_ZERO_USERS fires when xpm_release_node() drops the device's
	 * user count to zero, and is supported on all platforms currently
	 * exercised by this test.
	 */
	.event = EVENT_ZERO_USERS,
	.flags = 0,
};

/*
 * This callback routine is invoked when the notification arrives.
 */
void test_notify_cb(xpm_notifier *notifier)
{
	tftf_testcase_printf("Received notification: Node=0x%x, Event=%d, OPP=%d\n",
			     notifier->node, (int)notifier->event, (int)notifier->oppoint);
}

/*
 * Register a notifier for PM_DEV_USB_0 and release the node to trigger
 * EVENT_ZERO_USERS.  The node is released here, so the unregister path must
 * not release it again.
 */
static int test_register_notifier(void)
{
	int32_t status;

	status = xpm_request_node(notifier.node, PM_CAP_ACCESS, 0, 0);
	if (status != PM_RET_SUCCESS) {
		tftf_testcase_printf("%s ERROR to request the node for Node Id: 0x%x, "
				     "Status: 0x%x\n", __func__, notifier.node, status);
		return status;
	}

	status = xpm_register_notifier(&notifier);
	if (status != PM_RET_SUCCESS) {
		tftf_testcase_printf("%s ERROR to register the notifier, Status: 0x%x\n",
				     __func__, status);
		(void)xpm_release_node(notifier.node);
		return status;
	}

	/*
	 * The node is released here to trigger EVENT_ZERO_USERS.  If the
	 * release fails, unregister the notifier first so we do not leave it
	 * registered with the firmware on the failure path.
	 */
	status = xpm_release_node(notifier.node);
	if (status != PM_RET_SUCCESS) {
		tftf_testcase_printf("%s ERROR to release Node Id: 0x%x, Status: 0x%x\n",
				     __func__, notifier.node, status);
		(void)xpm_unregister_notifier(&notifier);
	}

	return status;
}

/*
 * Unregister the notifier.  The node was already released in
 * test_register_notifier() to trigger EVENT_ZERO_USERS.
 */
static int test_unregister_notifier(void)
{
	int32_t status;

	/*
	 * If xpm_unregister_notifier() fails, the notifier remains
	 * registered with the firmware until the platform is reset.
	 */
	status = xpm_unregister_notifier(&notifier);
	if (status != PM_RET_SUCCESS)
		tftf_testcase_printf("%s ERROR to unregister the notifier\n", __func__);

	return status;
}

/*
 * This function tests the register notification handling in the test
 * environment. It first attempts to register a notifier using
 * test_register_notifier(). If successful, it waits for a notification to be
 * received within a specified timeout period. If a notification is received
 * before the timeout, it proceeds to unregister the notifier using
 * test_unregister_notifier().
 */
test_result_t test_notifier_api(void)
{
	uint32_t timeout_counter = 0U;
	int32_t status;
	test_result_t result = TEST_RESULT_SUCCESS;

	status = test_register_notifier();
	if (status != PM_RET_SUCCESS)
		result = TEST_RESULT_FAIL;

	while ((notifier.received == 0) && (timeout_counter < TIMEOUT_MS)) {
		tftf_timer_sleep(1); // Sleep for 1 millisecond
		timeout_counter++;
	}

	if (!notifier.received) {
		tftf_testcase_printf("%s ERROR timeout\n", __func__);
		result = TEST_RESULT_FAIL;
	}

	status = test_unregister_notifier();
	if (status != PM_RET_SUCCESS)
		result = TEST_RESULT_FAIL;

	return result;
}
