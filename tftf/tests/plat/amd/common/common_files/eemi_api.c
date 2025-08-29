/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_defs.h"

xpm_notifier * notifier_store = NULL;

static int do_feature_check(const uint32_t api_id)
{
	smc_args args;
	smc_ret_values ret;

	args.fid = (PM_SIP_SVC | PM_FEATURE_CHECK);
	args.arg1 = api_id;

	ret = tftf_smc(&args);

	return lower_32_bits(ret.ret0);
}

static int eemi_call(const uint32_t arg0, const uint64_t arg1, const uint64_t arg2,
		     const uint64_t arg3, const uint64_t arg4, const uint64_t arg5,
		     const uint64_t arg6, const uint64_t arg7,
		     uint32_t *const ret_payload)
{
	smc_args args;
	smc_ret_values ret;
	int32_t status;

	args.fid = (PM_SIP_SVC | arg0);
	args.arg1 = arg1;
	args.arg2 = arg2;
	args.arg3 = arg3;
	args.arg4 = arg4;
	args.arg5 = arg5;
	args.arg6 = arg6;
	args.arg7 = arg7;

	/*
	 * 'arg0' represents the API ID. This check ensures that the API is supported
	 * by TF-A/PLM before making the actual API call.
	 */
	status = do_feature_check(arg0);
	if (status != PM_RET_SUCCESS) {
		tftf_testcase_printf("%s ERROR Status:0x%x, Feature Check Failed for "
				     "API Id:0x%x\n", __func__, status, arg0);
		return status;
	}

	ret = tftf_smc(&args);

	if (ret_payload) {
		ret_payload[0] = lower_32_bits(ret.ret0);
		ret_payload[1] = upper_32_bits(ret.ret0);
		ret_payload[2] = lower_32_bits(ret.ret1);
		ret_payload[3] = upper_32_bits(ret.ret1);
		ret_payload[4] = lower_32_bits(ret.ret2);
		ret_payload[5] = upper_32_bits(ret.ret2);
		ret_payload[6] = lower_32_bits(ret.ret3);
	}

	return lower_32_bits(ret.ret0);
}

int xpm_get_api_version(uint32_t *version)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];
	int ret;

	ret = eemi_call(PM_GET_API_VERSION, 0, 0, 0, 0, 0, 0, 0, ret_payload);
	if (ret == PM_RET_SUCCESS)
		*version = ret_payload[1];

	return ret;
}

int xpm_get_chip_id(uint32_t *id_code, uint32_t *version)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];
	int ret;

	ret = eemi_call(PM_GET_CHIPID, 0, 0, 0, 0, 0, 0, 0, ret_payload);
	if (ret == PM_RET_SUCCESS) {
		*id_code = ret_payload[1];
		*version = ret_payload[2];
	}

	return ret;
}

int xpm_feature_check(const uint32_t api_id, uint32_t *const version)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];
	int ret;

	ret = eemi_call(PM_FEATURE_CHECK, api_id, 0, 0, 0, 0, 0, 0, ret_payload);
	if (ret == PM_RET_SUCCESS)
		*version = ret_payload[1];

	return ret;
}

int xpm_request_node(const uint32_t device_id, const uint32_t capabilities,
		     const uint32_t qos, const uint32_t ack)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];

	return eemi_call(PM_REQUEST_NODE, ((uint64_t)capabilities << 32 | device_id),
			((uint64_t)ack << 32 | qos), 0, 0, 0, 0, 0, ret_payload);
}

int xpm_release_node(const uint32_t device_id)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];

	return eemi_call(PM_RELEASE_NODE, device_id, 0, 0, 0, 0, 0, 0, ret_payload);
}

int xpm_set_requirement(const uint32_t device_id, const uint32_t capabilities,
			const uint32_t qos, const uint32_t ack)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];

	return eemi_call(PM_SET_REQUIREMENT, ((uint64_t)capabilities << 32 | device_id),
			((uint64_t)ack << 32 | qos), 0, 0, 0, 0, 0, ret_payload);
}

irq_handler_t xpm_notifier_cb(void *data)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];

	/* Get the IPI payload from TF-A */
	(void)eemi_call(PM_GET_CALLBACK_DATA, 0, 0, 0, 0, 0, 0, 0, ret_payload);

	if (!notifier_store)
		return NULL;

	if (ret_payload[0] != PM_NOTIFY_CB) {
		notifier_store->received = 0;
		tftf_testcase_printf("unexpected callback type\n");
	} else {
		notifier_store->received = 1;
	}

	(void)notifier_store->callback(notifier_store);

	return 0;
}

int xpm_register_notifier(xpm_notifier * const notifier)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];
	uint32_t wake = 1U;
	uint32_t enable = 1U;
	uint32_t sgi_num = NOTIFIER_SGI;
	uint32_t reset = 0U;
	int ret;

	if (!notifier)
		return PM_RET_ERROR_ARGS;

	/* Save the pointer to the structure so the notifier handler can access it */
	notifier_store = notifier;

	ret = tftf_irq_register_handler(sgi_num, (irq_handler_t)xpm_notifier_cb);
	if (ret != PM_RET_SUCCESS) {
		tftf_testcase_printf("failed to register the irq handler\n");
		return ret;
	}

	tftf_irq_enable(sgi_num, IRQ_PRIORITY);

	/* Register PM event notifier */
	ret = eemi_call(PM_REGISTER_NOTIFIER, ((uint64_t)notifier->event << 32 | notifier->node),
			((uint64_t)enable << 32 | wake), 0, 0, 0, 0, 0, ret_payload);
	if (ret != PM_RET_SUCCESS) {
		tftf_testcase_printf("failed to register the event notifier\n");
		return ret;
	}

	/* Register the SGI number with TF-A */
	ret = eemi_call(TF_A_PM_REGISTER_SGI, ((uint64_t)reset << 32 | sgi_num),
			0, 0, 0, 0, 0, 0, ret_payload);
	if (ret != PM_RET_SUCCESS)
		tftf_testcase_printf("failed to register the SGI with TF-A\n");

	return ret;
}

int xpm_unregister_notifier(xpm_notifier * const notifier)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];
	uint32_t wake = 0U;
	uint32_t enable = 0U;
	uint32_t reset = 1U;
	int ret;

	/* Unregister PM event notifier */
	ret = eemi_call(PM_REGISTER_NOTIFIER, ((uint64_t)notifier->event << 32 | notifier->node),
			((uint64_t)enable << 32 | wake), 0, 0, 0, 0, 0, ret_payload);
	if (ret != PM_RET_SUCCESS) {
		tftf_testcase_printf("failed to unregister the event notifier\n");
		return ret;
	}

	/* Unregister the SGI number with TF-A */
	ret = eemi_call(TF_A_PM_REGISTER_SGI, ((uint64_t)reset << 32 | 0),
			0, 0, 0, 0, 0, 0, ret_payload);
	if (ret != PM_RET_SUCCESS) {
		tftf_testcase_printf("failed to unregister the SGI with TF-A\n");
		return ret;
	}

	tftf_irq_disable(NOTIFIER_SGI);

	tftf_irq_unregister_handler(NOTIFIER_SGI);
	if (ret != PM_RET_SUCCESS)
		tftf_testcase_printf("failed to unregister the IRQ handler\n");

	return ret;
}

int xpm_ioctl(const uint32_t node_id, const uint32_t ioctl_id, const uint32_t arg1,
	      const uint32_t arg2, uint32_t *const response)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];
	int ret;

	ret = eemi_call(PM_IOCTL, ((uint64_t)ioctl_id << 32 | node_id),
			((uint64_t)arg2 << 32 | arg1), 0, 0, 0, 0, 0, ret_payload);
	if (ret == PM_RET_SUCCESS)
		*response = ret_payload[1];

	return ret;
}

int xpm_set_max_latency(const uint32_t device_id, const uint32_t latency)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];

	return eemi_call(PM_SET_MAX_LATENCY, ((uint64_t)latency << 32 | device_id),
			 0, 0, 0, 0, 0, 0, ret_payload);
}

int xpm_get_node_status(const uint32_t device_id, xpm_node_status * const node_status)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];
	int ret;

	if (!node_status)
		return PM_RET_ERROR_ARGS;

	ret = eemi_call(PM_GET_NODE_STATUS, device_id, 0, 0, 0, 0, 0, 0, ret_payload);
	if (ret != PM_RET_SUCCESS)
		return ret;

	node_status->status = ret_payload[1];
	node_status->requirements = ret_payload[2];
	node_status->usage = ret_payload[3];

	return ret;
}

int xpm_clock_get_status(const uint32_t clock_id, uint32_t *const state)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];
	int ret;

	ret = eemi_call(PM_CLOCK_GETSTATE, clock_id, 0, 0, 0, 0, 0, 0, ret_payload);
	if (ret == PM_RET_SUCCESS)
		*state = ret_payload[1];

	return ret;
}

int xpm_clock_enable(const uint32_t clock_id)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];

	return eemi_call(PM_CLOCK_ENABLE, clock_id, 0, 0, 0, 0, 0, 0, ret_payload);
}

int xpm_clock_disable(const uint32_t clock_id)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];

	return eemi_call(PM_CLOCK_DISABLE, clock_id, 0, 0, 0, 0, 0, 0, ret_payload);
}

int xpm_clock_set_parent(const uint32_t clock_id, const uint32_t parent_id)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];

	return eemi_call(PM_CLOCK_SETPARENT, ((uint64_t)parent_id << 32 | clock_id),
			 0, 0, 0, 0, 0, 0, ret_payload);
}

int xpm_clock_get_parent(const uint32_t clock_id, uint32_t *const parent_id)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];
	int ret;

	ret = eemi_call(PM_CLOCK_GETPARENT, clock_id, 0, 0, 0, 0, 0, 0, ret_payload);
	if (ret == PM_RET_SUCCESS)
		*parent_id = ret_payload[1];

	return ret;
}

int xpm_clock_set_divider(const uint32_t clock_id, const uint32_t divider)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];

	return eemi_call(PM_CLOCK_SETDIVIDER, ((uint64_t)divider << 32 | clock_id),
			 0, 0, 0, 0, 0, 0, ret_payload);
}

int xpm_clock_get_divider(const uint32_t clock_id, uint32_t *const divider)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];
	int ret;

	ret = eemi_call(PM_CLOCK_GETDIVIDER, clock_id, 0, 0, 0, 0, 0, 0, ret_payload);
	if (ret == PM_RET_SUCCESS)
		*divider = ret_payload[1];

	return ret;
}

int xpm_pinctrl_set_function(const uint32_t pin_id, const uint32_t function_id)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];

	return eemi_call(PM_PINCTRL_SET_FUNCTION, ((uint64_t)function_id << 32 | pin_id),
			0, 0, 0, 0, 0, 0, ret_payload);
}

int xpm_pinctrl_get_function(const uint32_t pin_id, uint32_t *const function_id)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];
	int32_t ret;

	ret = eemi_call(PM_PINCTRL_GET_FUNCTION, pin_id, 0, 0, 0, 0, 0, 0, ret_payload);
	if (ret == PM_RET_SUCCESS)
		*function_id = ret_payload[1];

	return ret;
}

int xpm_pinctrl_request(const uint32_t pin_id)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];

	return eemi_call(PM_PINCTRL_REQUEST, pin_id, 0, 0, 0, 0, 0, 0, ret_payload);
}

int xpm_pinctrl_release(const uint32_t pin_id)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];

	return eemi_call(PM_PINCTRL_RELEASE, pin_id, 0, 0, 0, 0, 0, 0, ret_payload);
}

int xpm_reset_assert(const uint32_t reset_id, const uint32_t action)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];

	return eemi_call(PM_RESET_ASSERT, ((uint64_t)action << 32 | reset_id),
			 0, 0, 0, 0, 0, 0, ret_payload);
}

int xpm_pinctrl_get_parameter(const uint32_t pin_id, const uint32_t param_id,
			      uint32_t *const param_val)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];
	int32_t ret;

	ret = eemi_call(PM_PINCTRL_CONFIG_PARAM_GET, ((uint64_t)param_id << 32 | pin_id),
			0, 0, 0, 0, 0, 0, ret_payload);
	if (ret == PM_RET_SUCCESS)
		*param_val = ret_payload[1];

	return ret;
}

int xpm_pinctrl_set_parameter(const uint32_t pin_id, const uint32_t param_id,
			      const uint32_t param_val)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];

	return eemi_call(PM_PINCTRL_CONFIG_PARAM_SET, ((uint64_t)param_id << 32 | pin_id),
			 param_val, 0, 0, 0, 0, 0, ret_payload);
}

int xpm_init_finalize(void)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];

	return eemi_call(PM_INIT_FINALIZE, 0, 0, 0, 0, 0, 0, 0, ret_payload);
}

int get_trustzone_version(uint32_t *tz_version)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];
	int ret;

	ret = eemi_call(PM_GET_TRUSTZONE_VERSION, 0, 0, 0, 0, 0, 0, 0, ret_payload);
	if (ret == PM_RET_SUCCESS)
		*tz_version = ret_payload[1];

	return ret;
}

int tf_a_feature_check(const uint32_t api_id, uint32_t *const version)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];
	int ret;

	ret = eemi_call(TF_A_FEATURE_CHECK, api_id, 0, 0, 0, 0, 0, 0, ret_payload);
	if (ret == PM_RET_SUCCESS)
		*version = ret_payload[1];

	return ret;
}

int tf_a_pm_register_sgi(uint32_t sgi_num, uint32_t reset)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];

	return eemi_call(TF_A_PM_REGISTER_SGI, ((uint64_t)reset << 32 | sgi_num),
			 0, 0, 0, 0, 0, 0, ret_payload);
}

int xpm_op_characteristics(uint32_t const device_id, uint32_t const type, uint32_t *result)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];
	int ret;

	ret = eemi_call(PM_GET_OP_CHARACTERISTIC, ((uint64_t)type << 32 | device_id),
			0, 0, 0, 0, 0, 0, ret_payload);
	if (ret == PM_RET_SUCCESS)
		*result = ret_payload[1];

	return ret;
}

int xpm_system_shutdown(const uint32_t type, const uint32_t subtype)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];

	return eemi_call(PM_SYSTEM_SHUTDOWN, ((uint64_t)subtype << 32 | type),
			 0, 0, 0, 0, 0, 0, ret_payload);
}

int xpm_pll_set_parameter(const uint32_t clock_id, const uint32_t param_id, const uint32_t value)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];

	return eemi_call(PM_PLL_SET_PARAMETER, ((uint64_t)param_id << 32 | clock_id),
			 value, 0, 0, 0, 0, 0, ret_payload);
}

int xpm_pll_get_parameter(const uint32_t clock_id, const uint32_t param_id, uint32_t *value)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];
	int ret;

	ret = eemi_call(PM_PLL_GET_PARAMETER, clock_id, param_id, 0, 0, 0, 0, 0, ret_payload);
	if (ret == PM_RET_SUCCESS)
		*value = ret_payload[1];

	return ret;
}

int xpm_pll_set_mode(const uint32_t clock_id, const uint32_t value)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];

	return eemi_call(PM_PLL_SET_MODE, ((uint64_t)value << 32 | clock_id), 0,
			 0, 0, 0, 0, 0, ret_payload);
}

int xpm_pll_get_mode(const uint32_t clock_id, uint32_t *value)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];
	int ret;

	ret = eemi_call(PM_PLL_GET_MODE, clock_id, 0, 0, 0, 0, 0, 0, ret_payload);
	if (ret == PM_RET_SUCCESS)
		*value = ret_payload[1];

	return ret;
}

int xpm_self_suspend(const uint32_t device_id, const uint32_t latency, const uint8_t state,
		     uint32_t address)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];

	return eemi_call(PM_SELF_SUSPEND, ((uint64_t)latency << 32 | device_id),
			 (uint64_t)address << 32 | state, 0, 0, 0, 0, 0, ret_payload);
}

int xpm_set_wakeup_source(const uint32_t target_node_id, const uint32_t source_node_id,
			  const uint32_t enable)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];

	return eemi_call(PM_SET_WAKEUP_SOURCE, ((uint64_t)source_node_id << 32 | target_node_id),
			 enable, 0, 0, 0, 0, 0, ret_payload);
}

int xpm_query_data(const uint32_t qid, const uint32_t arg1, const uint32_t arg2,
		   const uint32_t arg3, uint32_t *output)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];
	int32_t ret;

	ret = eemi_call(PM_QUERY_DATA, qid, arg1, arg2, arg3, 0, 0, 0, ret_payload);
	*output = ret_payload[1];

	return ret;
}

int xpm_force_powerdown(const uint32_t node_id, const uint32_t ack)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];

	return eemi_call(PM_FORCE_POWERDOWN, ((uint64_t)ack << 32 | node_id), 0,
					      0, 0, 0, 0, 0, ret_payload);
}

int xpm_request_wakeup(const uint32_t node_id, const uint32_t set_address,
		       const uint32_t address, const uint32_t ack)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];

	return eemi_call(PM_REQUEST_WAKEUP, ((uint64_t)set_address << 32 | node_id),
			 ((uint64_t)ack << 32 | address), 0, 0, 0, 0, 0, ret_payload);
}

int xpm_reset_get_status(const uint32_t reset_id, uint32_t *status)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];
	int32_t ret;

	ret = eemi_call(PM_RESET_GET_STATUS, reset_id, 0, 0, 0, 0, 0, 0, ret_payload);
	*status = ret_payload[1];

	return ret;
}
