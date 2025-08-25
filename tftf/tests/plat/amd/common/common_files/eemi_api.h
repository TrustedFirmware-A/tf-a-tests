/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __EEMI_API_H__
#define __EEMI_API_H__

#include "xpm_defs.h"

int xpm_get_api_version(uint32_t *version);
int xpm_get_chip_id(uint32_t *id_code, uint32_t *version);
int xpm_feature_check(const uint32_t api_id, uint32_t *const version);

#endif /* __EEMI_API_H__ */
