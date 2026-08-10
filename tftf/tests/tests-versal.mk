#
# Copyright (c) 2022-2026, Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Platforms that build the AMD-Xilinx tests; others get standard tests only.
AMD_XILINX_TEST_ALLOWED_PLATS	:= versal versal2

include tftf/tests/tests-versal-common.mk

# AMD common test sources, enumerated for per-platform reviewability.
# Skipped on platforms not in AMD_XILINX_TEST_ALLOWED_PLATS.
ifneq ($(filter $(PLAT),$(AMD_XILINX_TEST_ALLOWED_PLATS)),)
TESTS_SOURCES		+=	$(wildcard tftf/tests/plat/amd/common/clock_test/*.c)		\
				$(wildcard tftf/tests/plat/amd/common/feature_check/*.c)	\
				$(wildcard tftf/tests/plat/amd/common/get_api_version_test/*.c)	\
				$(wildcard tftf/tests/plat/amd/common/get_chipid_test/*.c)	\
				$(wildcard tftf/tests/plat/amd/common/get_trustzone_version/*.c) \
				$(wildcard tftf/tests/plat/amd/common/init_finalize/*.c)	\
				$(wildcard tftf/tests/plat/amd/common/ioctl_test/*.c)		\
				$(wildcard tftf/tests/plat/amd/common/node_test/*.c)		\
				$(wildcard tftf/tests/plat/amd/common/op_characteristics/*.c)	\
				$(wildcard tftf/tests/plat/amd/common/pll_test/*.c)		\
				$(wildcard tftf/tests/plat/amd/common/query_data/*.c)		\
				$(wildcard tftf/tests/plat/amd/common/register_notifier_test/*.c) \
				$(wildcard tftf/tests/plat/amd/common/reset_get_status/*.c)	\
				$(wildcard tftf/tests/plat/amd/common/self_suspend/*.c)		\
				$(wildcard tftf/tests/plat/amd/common/system_shutdown/*.c)	\
				$(wildcard tftf/tests/plat/amd/common/tf_a_feature_check/*.c)	\
				$(wildcard tftf/tests/plat/amd/common/tf_a_register_sgi/*.c)

# pin_test references the pm_pin_fun_ids enum (PIN_FUNC_*) that lives
# in versal/xpm_defs_plat.h only; only compile it on the Versal platform.
ifeq ($(PLAT),versal)
TESTS_SOURCES		+=	$(wildcard tftf/tests/plat/amd/common/pin_test/*.c)
endif
endif # PLAT in AMD_XILINX_TEST_ALLOWED_PLATS

include tftf/tests/tests-standard.mk
TESTS_SOURCES := $(sort ${TESTS_SOURCES})
