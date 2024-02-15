/*
 * Copyright (c) 2020-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform.h>
#include <psci.h>

/* State IDs for local power states on TC platform. */
#define TC_PS_RUN_STATE_ID		0 /* Valid for CPUs and Clusters */
#define TC_PS_RETENTION_STATE_ID	1 /* Valid for only CPUs */
#define TC_PS_OFF_STATE_ID		2 /* Valid for CPUs and Clusters */

/* Suspend depth definitions for each power state */
#define TC_PS_RUN_DEPTH		0
#define TC_PS_RETENTION_DEPTH		1
#define TC_PS_OFF_DEPTH		2

/* The state property array with details of idle state possible for the core */
static const plat_state_prop_t core_state_prop[] = {
	{TC_PS_RETENTION_DEPTH, TC_PS_RETENTION_STATE_ID,
		PSTATE_TYPE_STANDBY},
	{TC_PS_OFF_DEPTH, TC_PS_OFF_STATE_ID, PSTATE_TYPE_POWERDOWN},
	{0}
};

/* The state property array with details of idle state possible for the cluster */
static const plat_state_prop_t cluster_state_prop[] = {
	{TC_PS_OFF_DEPTH, TC_PS_OFF_STATE_ID, PSTATE_TYPE_POWERDOWN},
	{0}
};

const plat_state_prop_t *plat_get_state_prop(unsigned int level)
{
	switch (level) {
	case MPIDR_AFFLVL0:
		return core_state_prop;
	case MPIDR_AFFLVL1:
		return cluster_state_prop;
	default:
		return NULL;
	}
}
