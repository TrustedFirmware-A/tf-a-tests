#
# Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include plat/arm/neoverse_rd/common/nrd_common.mk

PLAT_INCLUDES		+=	-Iplat/arm/neoverse_rd/platform/rdn2/include/

PLAT_SOURCES		+=	plat/arm/neoverse_rd/platform/rdn2/topology.c

PLAT_TESTS_SKIP_LIST	:=	plat/arm/neoverse_rd/platform/rdn2/tests_to_skip.txt

RD_N2_VARIANTS		:=	0 1 3

ifneq ($(CSS_SGI_PLATFORM_VARIANT), \
  $(filter $(CSS_SGI_PLATFORM_VARIANT),$(RD_N2_VARIANTS)))
  $(error "CSS_SGI_PLATFORM_VARIANT for RD-N2 should be 0 1 or 3, currently \
    set to ${CSS_SGI_PLATFORM_VARIANT}.")
endif
