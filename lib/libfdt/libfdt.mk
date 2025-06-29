#
# Copyright (c) 2020, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

LIBFDT_SRCS	:=	$(addprefix lib/libfdt/,	\
			fdt.c				\
			fdt_addresses.c			\
			fdt_empty_tree.c		\
			fdt_ro.c			\
			fdt_rw.c			\
			fdt_strerror.c			\
			fdt_sw.c			\
			fdt_wip.c)			\

INCLUDES	+=	-Iinclude/lib/libfdt
