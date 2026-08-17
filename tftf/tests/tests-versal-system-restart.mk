#
# Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# AMD-Xilinx-only image; refuse to build for a platform without AMD support.
AMD_XILINX_TEST_ALLOWED_PLATS	:= versal versal2

ifeq ($(filter $(PLAT),$(AMD_XILINX_TEST_ALLOWED_PLATS)),)
$(error tests-versal-system-restart.mk: PLAT=$(PLAT) is not supported; AMD_XILINX_TEST_ALLOWED_PLATS = $(AMD_XILINX_TEST_ALLOWED_PLATS))
endif

include tftf/tests/tests-versal-common.mk

TESTS_SOURCES		+=	$(wildcard tftf/tests/plat/amd/common/restart_test/*.c)

TESTS_SOURCES := $(sort ${TESTS_SOURCES})
