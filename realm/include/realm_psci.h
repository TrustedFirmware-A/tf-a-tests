/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdint.h>

void realm_cpu_off(void);
u_register_t realm_cpu_on(u_register_t mpidr, uintptr_t entrypoint,
		u_register_t context_id);
u_register_t realm_psci_affinity_info(u_register_t target_affinity,
		uint32_t lowest_affinity_level);
u_register_t realm_psci_features(uint32_t psci_func_id);
