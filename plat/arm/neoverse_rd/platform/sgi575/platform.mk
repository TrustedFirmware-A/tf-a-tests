#
# Copyright (c) 2018-2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include plat/arm/neoverse_rd/common/nrd_common.mk

PLAT_INCLUDES		+=	-Iplat/arm/neoverse_rd/platform/sgi575/include/

PLAT_SOURCES		+=	plat/arm/neoverse_rd/platform/sgi575/sgi575_topology.c

PLAT_TESTS_SKIP_LIST	:=	plat/arm/neoverse_rd/platform/sgi575/tests_to_skip.txt

ifdef CSS_SGI_PLATFORM_VARIANT
$(error "CSS_SGI_PLATFORM_VARIANT should not be set for SGI-575, \
    currently set to ${CSS_SGI_PLATFORM_VARIANT}.")
endif
