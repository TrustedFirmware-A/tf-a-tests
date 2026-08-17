/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Platform-agnostic PGGS (persistent scratchpad) access API.  Each
 * platform provides its own implementation of xpm_pggs_read() /
 * xpm_pggs_write() under common_files/$(PLAT)/xpm_pggs.c.
 */

#ifndef XPM_PGGS_H_
#define XPM_PGGS_H_

#include <stdint.h>

/**
 * @brief Read the persistent global scratchpad register.
 *
 * @param val Output pointer that receives the register value on success.
 *            Must not be NULL.
 *
 * @return 0 on success, or an EEMI error code on failure.
 */
int xpm_pggs_read(uint32_t *const val);

/**
 * @brief Write the persistent global scratchpad register.
 *
 * @param val Value to store in the register.
 *
 * @return 0 on success, or an EEMI error code on failure.
 */
int xpm_pggs_write(const uint32_t val);

#endif /* XPM_PGGS_H_ */
