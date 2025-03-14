/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <arch.h>
#include <platform.h>
#include <psci.h>

/*
 * State IDs for local power states on rdsaspen.
 */
#define RDASPEN_RUN_STATE_ID		0 /* Valid for CPUs and Clusters */
#define RDASPEN_SLEEP0_STATE_ID		1 /* Valid for CPUs and Clusters */
#define RDASPEN_SLEEP1_STATE_ID		2 /* Valid for CPUs and Clusters */
#define RDASPEN_OFF_STATE_ID		3 /* Valid for CPUs and Clusters */

/*
 * Suspend depth definitions for each power state
 */
typedef enum {
	RDASPEN_RUN_DEPTH = 0,
	RDASPEN_SLEEP0_DEPTH,
	RDASPEN_SLEEP1_DEPTH,
	RDASPEN_OFF_DEPTH,
} suspend_depth_t;

/* The state property array with details of idle state possible for the core */
static const plat_state_prop_t core_state_prop[] = {
	{RDASPEN_SLEEP0_DEPTH, RDASPEN_SLEEP0_STATE_ID, PSTATE_TYPE_STANDBY},
	{RDASPEN_SLEEP1_DEPTH, RDASPEN_SLEEP1_STATE_ID, PSTATE_TYPE_STANDBY},
	{RDASPEN_OFF_DEPTH, RDASPEN_OFF_STATE_ID, PSTATE_TYPE_POWERDOWN},
	{0},
};

/*
 * The state property array with details of idle state possible
 * for the cluster
 */
static const plat_state_prop_t cluster_state_prop[] = {
	{RDASPEN_SLEEP0_DEPTH, RDASPEN_SLEEP0_STATE_ID, PSTATE_TYPE_STANDBY},
	{RDASPEN_SLEEP1_DEPTH, RDASPEN_SLEEP1_STATE_ID, PSTATE_TYPE_STANDBY},
	{RDASPEN_OFF_DEPTH, RDASPEN_OFF_STATE_ID, PSTATE_TYPE_POWERDOWN},
	{0},
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
	return NULL;
}
