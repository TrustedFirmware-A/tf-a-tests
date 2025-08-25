/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef XPM_DEFS_H_
#define XPM_DEFS_H_

#include <irq.h>
#include <smccc.h>
#include <tftf_lib.h>

#define PM_SIP_SVC		0xC2000000U
#define PAYLOAD_ARG_CNT		7U
#define NOTIFIER_SGI           15U
#define IRQ_PRIORITY		0U

#define upper_32_bits(n)	((uint32_t)(((n) >> 32U)))
#define lower_32_bits(n)	((uint32_t)((n) & 0xffffffffU))

#define VERSION_MAJOR(v)	((v) >> 16)
#define VERSION_MINOR(v)	((v) & 0xFFFF)

#define PM_RET_SUCCESS		0U
#define PM_RET_ERROR_ARGS	1U
#define PM_NO_ACCESS		2002L
#define PM_ERR_VERSION		2014L

/* TF-A only commands */
#define PM_GET_CALLBACK_DATA	0xa01U
#define TF_A_PM_REGISTER_SGI	0xa04U

/* API IDs */
enum pm_api_id {
	PM_API_MIN,                                     /**< 0x0 */
	PM_GET_API_VERSION,                             /**< 0x1 */
	PM_SET_CONFIGURATION,                           /**< 0x2 */
	PM_GET_NODE_STATUS,                             /**< 0x3 */
	PM_GET_OP_CHARACTERISTIC,                       /**< 0x4 */
	PM_REGISTER_NOTIFIER,                           /**< 0x5 */
	PM_REQUEST_SUSPEND,                             /**< 0x6 */
	PM_SELF_SUSPEND,                                /**< 0x7 */
	PM_FORCE_POWERDOWN,                             /**< 0x8 */
	PM_ABORT_SUSPEND,                               /**< 0x9 */
	PM_REQUEST_WAKEUP,                              /**< 0xA */
	PM_SET_WAKEUP_SOURCE,                           /**< 0xB */
	PM_SYSTEM_SHUTDOWN,                             /**< 0xC */
	PM_REQUEST_NODE,                                /**< 0xD */
	PM_RELEASE_NODE,                                /**< 0xE */
	PM_SET_REQUIREMENT,                             /**< 0xF */
	PM_SET_MAX_LATENCY,                             /**< 0x10 */
	PM_RESET_ASSERT,                                /**< 0x11 */
	PM_RESET_GET_STATUS,                            /**< 0x12 */
	PM_MMIO_WRITE,                                  /**< 0x13 */
	PM_MMIO_READ,                                   /**< 0x14 */
	PM_INIT_FINALIZE,                               /**< 0x15 */
	PM_FPGA_LOAD,                                   /**< 0x16 */
	PM_FPGA_GET_STATUS,                             /**< 0x17 */
	PM_GET_CHIPID,                                  /**< 0x18 */
	PM_SECURE_RSA_AES,                              /**< 0x19 */
	PM_SECURE_SHA,                                  /**< 0x1A */
	PM_SECURE_RSA,                                  /**< 0x1B */
	PM_PINCTRL_REQUEST,                             /**< 0x1C */
	PM_PINCTRL_RELEASE,                             /**< 0x1D */
	PM_PINCTRL_GET_FUNCTION,                        /**< 0x1E */
	PM_PINCTRL_SET_FUNCTION,                        /**< 0x1F */
	PM_PINCTRL_CONFIG_PARAM_GET,                    /**< 0x20 */
	PM_PINCTRL_CONFIG_PARAM_SET,                    /**< 0x21 */
	PM_IOCTL,                                       /**< 0x22 */
	PM_QUERY_DATA,                                  /**< 0x23 */
	PM_CLOCK_ENABLE,                                /**< 0x24 */
	PM_CLOCK_DISABLE,                               /**< 0x25 */
	PM_CLOCK_GETSTATE,                              /**< 0x26 */
	PM_CLOCK_SETDIVIDER,                            /**< 0x27 */
	PM_CLOCK_GETDIVIDER,                            /**< 0x28 */
	PM_CLOCK_SETRATE,                               /**< 0x29 */
	/* PM_CLOCK_GETRATE API is deprecated */
	PM_RESERVE_ID,                                  /**< 0x2A */
	PM_CLOCK_SETPARENT,                             /**< 0x2B */
	PM_CLOCK_GETPARENT,                             /**< 0x2C */
	PM_SECURE_IMAGE,                                /**< 0x2D */
	PM_FPGA_READ,                                   /**< 0x2E */
	PM_SECURE_AES,                                  /**< 0x2F */
	PM_PLL_SET_PARAMETER,                           /**< 0x30 */
	PM_PLL_GET_PARAMETER,                           /**< 0x31 */
	PM_PLL_SET_MODE,                                /**< 0x32 */
	PM_PLL_GET_MODE,                                /**< 0x33 */
	PM_REGISTER_ACCESS,                             /**< 0x34 */
	PM_EFUSE_ACCESS,                                /**< 0x35 */
	PM_ADD_SUBSYSTEM,                               /**< 0x36 */
	PM_DESTROY_SUBSYSTEM,                           /**< 0x37 */
	PM_DESCRIBE_NODES,                              /**< 0x38 */
	PM_ADD_NODE,                                    /**< 0x39 */
	PM_ADD_NODE_PARENT,                             /**< 0x3A */
	PM_ADD_NODE_NAME,                               /**< 0x3B */
	PM_ADD_REQUIREMENT,                             /**< 0x3C */
	PM_SET_CURRENT_SUBSYSTEM,                       /**< 0x3D */
	PM_INIT_NODE,                                   /**< 0x3E */
	PM_FEATURE_CHECK,                               /**< 0x3F */
	PM_ISO_CONTROL,                                 /**< 0x40 */
	PM_ACTIVATE_SUBSYSTEM,                          /**< 0x41 */
	PM_SET_NODE_ACCESS,                             /**< 0x42 */
	PM_BISR,                                        /**< 0x43 */
	PM_APPLY_TRIM,                                  /**< 0x44 */
	PM_NOC_CLOCK_ENABLE,                            /**< 0x45 */
	PM_IF_NOC_CLOCK_ENABLE,                         /**< 0x46 */
	PM_FORCE_HOUSECLEAN,                            /**< 0x47 */
	PM_FPGA_GET_VERSION,                            /**< 0x48 */
	PM_FPGA_GET_FEATURE_LIST,                       /**< 0x49 */
	PM_HNICX_NPI_DATA_XFER,                         /**< 0x4A */
	PM_API_MAX                                      /**< 0x4B */
};

/* Node capabilities */
#define        PM_CAP_ACCESS            1U
#define        PM_CAP_CONTEXT           2U

/*
 * PM notify events
 */
enum xpm_notify_event {
	EVENT_STATE_CHANGE = 1U,                        /**< State change event */
	EVENT_ZERO_USERS = 2U,                          /**< Zero user event */
	EVENT_CPU_IDLE_FORCE_PWRDWN = 4U,               /**< CPU idle event during force pwr down */
};

/**
 * PM API callback IDs
 */
enum pm_api_cb_id {
	PM_INIT_SUSPEND_CB = 30U,                       /**< Suspend callback */
	PM_ACKNOWLEDGE_CB = 31U,                        /**< Acknowledge callback */
	PM_NOTIFY_CB = 32U,                             /**< Notify callback */
};

#endif /* XPM_DEFS_H_ */
