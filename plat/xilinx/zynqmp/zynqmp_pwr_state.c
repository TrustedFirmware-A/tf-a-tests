/*
 * Copyright (c) 2022-2024, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>

#include <arch.h>
#include <platform.h>
#include <psci.h>

/*
 * State IDs for local power states.
 */
#define ZYNQMP_RETENTION_STATE_ID	1	/* Valid for only CPUs */
#define ZYNQMP_OFF_STATE_ID		0	/* Valid for CPUs and Clusters */

/*
 * Suspend depth definitions for each power state
 */
typedef enum {
	ZYNQMP_RUN_DEPTH = 0,
	ZYNQMP_RETENTION_DEPTH,
	ZYNQMP_OFF_DEPTH,
} suspend_depth_t;

/* The state property array with details of idle state possible for the core */
static const plat_state_prop_t core_state_prop[] = {
	{ZYNQMP_RETENTION_DEPTH, ZYNQMP_RETENTION_STATE_ID, PSTATE_TYPE_STANDBY},
	{ZYNQMP_OFF_DEPTH, ZYNQMP_OFF_STATE_ID, PSTATE_TYPE_POWERDOWN},
	{0},
};

/* The state property array with details of idle state possible for the cluster */
static const plat_state_prop_t cluster_state_prop[] = {
	{ZYNQMP_OFF_DEPTH, ZYNQMP_OFF_STATE_ID, PSTATE_TYPE_POWERDOWN},
	{0},
};

/* The state property array with details of idle state possible for the system level */
static const plat_state_prop_t system_state_prop[] = {
	{ZYNQMP_OFF_DEPTH, ZYNQMP_OFF_STATE_ID, PSTATE_TYPE_POWERDOWN},
	{0},
};

const plat_state_prop_t *plat_get_state_prop(unsigned int level)
{
	switch (level) {
	case MPIDR_AFFLVL0:
		return core_state_prop;
	case MPIDR_AFFLVL1:
		return cluster_state_prop;
	case MPIDR_AFFLVL2:
		return system_state_prop;
	default:
		return NULL;
	}
}
