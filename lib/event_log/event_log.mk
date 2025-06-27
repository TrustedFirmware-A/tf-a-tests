#
# Copyright (c) 2020-2025, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Path to library sources
EVENT_LOG_SRC_DIR	:= lib/event_log/

# Default log level to dump the event log (LOG_LEVEL_INFO)
EVENT_LOG_LEVEL		?= 40

EVENT_LOG_SOURCES	:=	${EVENT_LOG_SRC_DIR}event_print.c

INCLUDES		+=	-Iinclude/lib/event_log \
				-Iinclude/drivers/auth

ifdef CRYPTO_SUPPORT
# Measured Boot hash algorithm.
# SHA-256 (or stronger) is required for all devices that are TPM 2.0 compliant.
ifdef TPM_HASH_ALG
    $(warning "TPM_HASH_ALG is deprecated. Please use MBOOT_EL_HASH_ALG instead.")
    MBOOT_EL_HASH_ALG		:=	${TPM_HASH_ALG}
else
    MBOOT_EL_HASH_ALG		:=	sha256
endif

ifeq (${MBOOT_EL_HASH_ALG}, sha512)
    TPM_ALG_ID			:=	TPM_ALG_SHA512
    TCG_DIGEST_SIZE		:=	64U
else ifeq (${MBOOT_EL_HASH_ALG}, sha384)
    TPM_ALG_ID			:=	TPM_ALG_SHA384
    TCG_DIGEST_SIZE		:=	48U
else
    TPM_ALG_ID			:=	TPM_ALG_SHA256
    TCG_DIGEST_SIZE		:=	32U
endif #MBOOT_EL_HASH_ALG

# Set definitions for event log library.
$(eval $(call add_define,TFTF_DEFINES,TPM_ALG_ID))
$(eval $(call add_define,TFTF_DEFINES,EVENT_LOG_LEVEL))
$(eval $(call add_define,TFTF_DEFINES,TCG_DIGEST_SIZE))

EVENT_LOG_SOURCES	:=	${EVENT_LOG_SRC_DIR}event_log.c

INCLUDES		+=	-Iinclude/lib/event_log \
				-Iinclude/drivers/auth

ifeq (${TRANSFER_LIST}, 1)
EVENT_LOG_SOURCES	+= ${EVENT_LOG_SRC_DIR}/event_handoff.c
INCLUDES		+=	-Iinclude/lib
endif

endif
