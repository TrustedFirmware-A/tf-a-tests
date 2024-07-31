/*
 * Copyright (c) 2017-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CACTUS_TESTS_H
#define CACTUS_TESTS_H

#include <spm_common.h>

/*
 * Self test functions
 */

void ffa_tests(struct mailbox_buffers *mb);
void cpu_feature_tests(void);

#endif /* CACTUS_TESTS_H */
