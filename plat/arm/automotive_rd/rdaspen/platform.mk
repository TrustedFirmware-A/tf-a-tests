#
# Copyright (c) 2025-2026, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

RDASPEN_PATH = plat/arm/automotive_rd/rdaspen
PLAT_INCLUDES	:=	-I ${RDASPEN_PATH}/include


PLAT_SOURCES	:=	drivers/arm/timer/private_timer.c			\
			drivers/arm/timer/system_timer.c			\
			${RDASPEN_PATH}/rdaspen_helpers.S 			\
			${RDASPEN_PATH}/rdaspen_pwr_state.c			\
			${RDASPEN_PATH}/rdaspen_topology.c			\
			${RDASPEN_PATH}/rdaspen_mem_prot.c			\
			${RDASPEN_PATH}/rdaspen_platsetup.c

PLAT_SUPPORTS_NS_RESET	:=	1

# Process PLAT_SUPPORTS_NS_RESET flag
$(eval $(call assert_boolean,PLAT_SUPPORTS_NS_RESET))
$(eval $(call add_define,TFTF_DEFINES,PLAT_SUPPORTS_NS_RESET))

FIRMWARE_UPDATE		:= 0
PLAT_TESTS_SKIP_LIST	:= ${RDASPEN_PATH}/tests_to_skip.txt

include plat/arm/common/arm_common.mk
