#
# Copyright (c) 2022-2026, Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include tftf/tests/tests-versal-common.mk

# AMD common test sources.  Directories are enumerated explicitly so each
# test suite's inclusion in the Versal/Versal Gen 2 build images is
# reviewable on a per-platform basis.
TESTS_SOURCES		+=	$(wildcard tftf/tests/plat/amd/common/clock_test/*.c)		\
				$(wildcard tftf/tests/plat/amd/common/feature_check/*.c)	\
				$(wildcard tftf/tests/plat/amd/common/force_powerdown/*.c)	\
				$(wildcard tftf/tests/plat/amd/common/get_api_version_test/*.c)	\
				$(wildcard tftf/tests/plat/amd/common/get_chipid_test/*.c)	\
				$(wildcard tftf/tests/plat/amd/common/get_trustzone_version/*.c) \
				$(wildcard tftf/tests/plat/amd/common/init_finalize/*.c)	\
				$(wildcard tftf/tests/plat/amd/common/ioctl_test/*.c)		\
				$(wildcard tftf/tests/plat/amd/common/node_test/*.c)		\
				$(wildcard tftf/tests/plat/amd/common/op_characteristics/*.c)	\
				$(wildcard tftf/tests/plat/amd/common/pin_test/*.c)		\
				$(wildcard tftf/tests/plat/amd/common/pll_test/*.c)		\
				$(wildcard tftf/tests/plat/amd/common/query_data/*.c)		\
				$(wildcard tftf/tests/plat/amd/common/register_notifier_test/*.c) \
				$(wildcard tftf/tests/plat/amd/common/reset_get_status/*.c)	\
				$(wildcard tftf/tests/plat/amd/common/self_suspend/*.c)		\
				$(wildcard tftf/tests/plat/amd/common/system_shutdown/*.c)	\
				$(wildcard tftf/tests/plat/amd/common/tf_a_feature_check/*.c)	\
				$(wildcard tftf/tests/plat/amd/common/tf_a_register_sgi/*.c)

include tftf/tests/tests-standard.mk
TESTS_SOURCES := $(sort ${TESTS_SOURCES})
