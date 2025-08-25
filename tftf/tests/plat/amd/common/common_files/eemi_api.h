/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __EEMI_API_H__
#define __EEMI_API_H__

#include "xpm_defs.h"

typedef struct xpm_notifier {
	void(*const callback)(struct xpm_notifier * const notifier);	/**< Callback handler */
	const uint32_t node;		/**< Node to receive notifications about */
	uint32_t event;			/**< Event type to receive notifications about */
	uint32_t received_event;	/**< Event received from PLM */
	uint32_t flags;			/**< Flags */
	uint32_t oppoint;		/**< Operating point in node */
	uint32_t received;		/**< How many times the notification has been received */
	struct xpm_notifier *next;	/**< Pointer to next notifier in linked list */
} xpm_notifier;

struct test_ioctl {
	uint32_t node_id;        /**<  Node ID */
	uint32_t ioctl_id;      /**<  Ioctl ID */
	uint32_t ioctl_arg1;     /**<  Arg1 for Ioctl-ID if required */
	uint32_t ioctl_arg2;     /**<  Arg2 for Ioctl-ID if required */
};

typedef struct xpm_ndstatus {
	uint32_t status;                /**< Node power state */
	uint32_t requirements;          /**< Requirements on node (slaves only) */
	uint32_t usage;                 /**< Indicates which master is using the slave */
} xpm_node_status;

int xpm_get_api_version(uint32_t *version);
int xpm_get_chip_id(uint32_t *id_code, uint32_t *version);
int xpm_feature_check(const uint32_t api_id, uint32_t *const version);
int xpm_request_node(const uint32_t device_id, const uint32_t capabilities,
		     const uint32_t qos, const uint32_t ack);
int xpm_release_node(const uint32_t device_id);
int xpm_set_requirement(const uint32_t device_id, const uint32_t capabilities,
			const uint32_t qos, const uint32_t ack);
int xpm_register_notifier(xpm_notifier * const notifier);
int xpm_unregister_notifier(xpm_notifier * const notifier);
int xpm_ioctl(const uint32_t node_id, const uint32_t ioctl_id, const uint32_t arg1,
	      const uint32_t arg2, uint32_t *const response);
int xpm_set_max_latency(const uint32_t device_id, const uint32_t latency);
int xpm_get_node_status(const uint32_t device_id, xpm_node_status * const node_status);

#endif /* __EEMI_API_H__ */
