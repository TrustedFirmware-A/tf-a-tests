/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef HOST_REALM_SVE_H
#define HOST_REALM_SVE_H

#include <stdint.h>

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

#endif /* HOST_REALM_SVE_H */
