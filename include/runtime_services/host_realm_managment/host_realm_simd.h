/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef HOST_REALM_SIMD_H
#define HOST_REALM_SIMD_H

#include <stdint.h>

/* Number of attempts, for plane N, to cause a SIMD access */
#define SIMD_TRAP_ATTEMPTS		(3UL)

struct sve_cmd_rdvl {
	uint64_t rdvl;
};

struct sve_cmd_id_regs {
	uint64_t id_aa64pfr0_el1;
	uint64_t id_aa64zfr0_el1;
};

struct sve_cmd_probe_vl {
	uint32_t vl_bitmap;
};

struct sme_cmd_id_regs {
	uint64_t id_aa64pfr1_el1;
	uint64_t id_aa64smfr0_el1;
};

#endif /* HOST_REALM_SIMD_H */
