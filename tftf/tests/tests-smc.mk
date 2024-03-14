#
# Copyright (c) 2018-2024, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

TESTS_SOURCES	+=						\
	$(addprefix tftf/tests/runtime_services/,		\
		generic/generic_smc.c				\
		standard_service/query_std_svc.c		\
		standard_service/unknown_smc.c 			\
	)

