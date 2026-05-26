#
# Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Shared base for the Versal/Versal Gen 2 test images.  Defines the AMD
# common-files include path and pulls in the EEMI API sources that every
# test image links against.  Per-image .mk wrappers include this file and
# then append the test source directories they want to compile.

COMMON_FILES_DIR	:= tftf/tests/plat/amd/common/common_files

TFTF_INCLUDES		+=	-I$(COMMON_FILES_DIR)/

TESTS_SOURCES		+=	$(wildcard $(COMMON_FILES_DIR)/*.c)
