/*
 * Copyright (c) 2024, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UTIL_H
#define UTIL_H

#include <platform_def.h>

#define CPU_DEF(cluster, cpu)	{ cluster, cpu }

#if (PLATFORM_CORE_COUNT_PER_CLUSTER == 1U)
#define CLUSTER_DEF(cluster)	\
	CPU_DEF(cluster, 0)
#elif (PLATFORM_CORE_COUNT_PER_CLUSTER == 2U)
#define CLUSTER_DEF(cluster)	\
	CPU_DEF(cluster, 0),		\
	CPU_DEF(cluster, 1)
#elif (PLATFORM_CORE_COUNT_PER_CLUSTER == 4U)
#define CLUSTER_DEF(cluster)	\
	CPU_DEF(cluster, 0),		\
	CPU_DEF(cluster, 1),		\
	CPU_DEF(cluster, 2),		\
	CPU_DEF(cluster, 3)
#endif

#endif /* UTIL_H */
