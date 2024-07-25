#
# Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include plat/arm/neoverse_rd/common/nrd_common.mk

PLAT_INCLUDES		+=	-Iplat/arm/neoverse_rd/platform/rdv3/include/

PLAT_SOURCES		+=	plat/arm/neoverse_rd/platform/rdv3/topology.c

PLAT_TESTS_SKIP_LIST	:=	plat/arm/neoverse_rd/platform/rdv3/tests_to_skip.txt

RD_V3_VARIANTS		:=	0

ifneq ($(NRD_PLATFORM_VARIANT), \
  $(filter $(NRD_PLATFORM_VARIANT),$(RD_V3_VARIANTS)))
  $(error "NRD_PLATFORM_VARIANT for RD-V3 should be 0, currently \
    set to ${NRD_PLATFORM_VARIANT}.")
endif