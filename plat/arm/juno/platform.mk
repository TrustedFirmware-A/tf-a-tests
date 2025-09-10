#
# Copyright (c) 2018-2025, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

PLAT_INCLUDES	:=	-Iplat/arm/juno/include/

PLAT_SOURCES	:=	drivers/arm/sp805/sp805.c			\
			drivers/arm/timer/private_timer.c		\
			drivers/arm/timer/sp804.c			\
			plat/arm/juno/${ARCH}/plat_helpers.S		\
			plat/arm/juno/juno_pwr_state.c			\
			plat/arm/juno/juno_timers.c			\
			plat/arm/juno/juno_topology.c			\
			plat/arm/juno/juno_mem_prot.c			\
			plat/arm/juno/plat_setup.c

ifeq (${ARCH},aarch32)
# Some tests are not supported on Juno AArch32.
	PLAT_TESTS_SKIP_LIST	:=	plat/arm/juno/juno32_tests_to_skip.txt
else
	PLAT_TESTS_SKIP_LIST	:=	plat/arm/juno/juno64_tests_to_skip.txt
endif

PLAT_SUPPORTS_NS_RESET	:=	1

# Process PLAT_SUPPORTS_NS_RESET flag
$(eval $(call assert_boolean,PLAT_SUPPORTS_NS_RESET))
$(eval $(call add_define,TFTF_DEFINES,PLAT_SUPPORTS_NS_RESET))

ifeq (${ARCH},aarch32)
ifeq (${FIRMWARE_UPDATE},1)
$(error "FIRMWARE_UPDATE is not supported on Juno aarch32")
endif
else
FIRMWARE_UPDATE := 1
endif

include plat/arm/common/arm_common.mk
