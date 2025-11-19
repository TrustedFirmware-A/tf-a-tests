#
# Copyright (c) 2024, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

MBEDTLS_DIR ?= contrib/mbedtls
ifeq (${MBEDTLS_DIR},)
$(error Error: MBEDTLS_DIR not set)
endif

MBEDTLS_PRESENT := $(wildcard ${MBEDTLS_DIR}/include/mbedtls)

ifneq (${MBEDTLS_PRESENT},)
$(info Found mbedTLS at ${MBEDTLS_DIR})

TFTF_INCLUDES += -I${MBEDTLS_DIR}/include
MBEDTLS_CONFIG_FILE ?= "<configs/tftf_mbedtls_config.h>"
$(eval $(call add_define,TFTF_DEFINES,MBEDTLS_CONFIG_FILE))

#
# Include mbedtls source required to parse x509 certificate and its helper
# routines. This can be later extended to include other crypto/PSA crypto
# library sources.
#
TESTS_SOURCES	+=				\
	$(addprefix ${MBEDTLS_DIR}/library/,	\
		asn1parse.c			\
		asn1write.c			\
		constant_time.c			\
		bignum.c			\
		oid.c				\
		hmac_drbg.c			\
		memory_buffer_alloc.c		\
		platform.c 			\
		platform_util.c			\
		bignum_core.c			\
		md.c				\
		pk.c 				\
		pk_ecc.c 			\
		pk_wrap.c 			\
		pkparse.c 			\
		sha256.c            		\
		sha512.c            		\
		ecdsa.c				\
		ecp_curves.c			\
		ecp.c				\
		rsa.c				\
		rsa_alt_helpers.c		\
		x509.c 				\
		x509_crt.c 			\
		)
else
$(info MbedTLS not found, some dependent tests will be skipped or fail.)
endif
