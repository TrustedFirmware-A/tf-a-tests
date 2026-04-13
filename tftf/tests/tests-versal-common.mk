#
# Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Shared base for the AMD/Xilinx test images.  Wrappers define
# AMD_XILINX_TEST_ALLOWED_PLATS and include this file before adding sources.

COMMON_FILES_DIR	:= tftf/tests/plat/amd/common/common_files

# Only pull in the AMD common-files for allowed platforms.
ifneq ($(filter $(PLAT),$(AMD_XILINX_TEST_ALLOWED_PLATS)),)
TFTF_INCLUDES		+=	-I$(COMMON_FILES_DIR)			\
				-I$(COMMON_FILES_DIR)/$(PLAT)

TESTS_SOURCES		+=	$(wildcard $(COMMON_FILES_DIR)/*.c)		\
				$(wildcard $(COMMON_FILES_DIR)/$(PLAT)/*.c)
endif
