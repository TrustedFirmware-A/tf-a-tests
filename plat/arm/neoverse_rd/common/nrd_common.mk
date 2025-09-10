#
# Copyright (c) 2018-2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

PLAT_INCLUDES	:=	-Iplat/arm/neoverse_rd/common/include/

PLAT_SOURCES	:=	drivers/arm/sp805/sp805.c			\
			drivers/arm/timer/private_timer.c		\
			drivers/arm/timer/system_timer.c		\
			plat/arm/neoverse_rd/common/arch/${ARCH}/plat_helpers.S\
			plat/arm/neoverse_rd/common/plat_setup.c	\
			plat/arm/neoverse_rd/common/nrd_mem_prot.c	\
			plat/arm/neoverse_rd/common/nrd_pwr_state.c

include plat/arm/common/arm_common.mk

ifeq (${USE_NVM},1)
$(error "USE_NVM is not supported on Neoverse RD platforms")
endif

# Pass NRD_PLATFORM_VARIANT flag to the build system
$(eval $(call add_define,TFTF_DEFINES,NRD_PLATFORM_VARIANT))
