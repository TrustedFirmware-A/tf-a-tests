#
# Copyright (c) 2018-2020, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Default, static values for build variables, listed in alphabetic order.
# Dependencies between build options, if any, are handled in the top-level
# Makefile, after this file is included. This ensures that the former is better
# poised to handle dependencies, as all build variables would have a default
# value by then.

# The Target build architecture. Supported values are: aarch64, aarch32.
ARCH			:= aarch64

# ARM Architecture feature modifiers: none by default
ARM_ARCH_FEATURE	:= none

# ARM Architecture major and minor versions: 8.0 by default.
ARM_ARCH_MAJOR		:= 8
ARM_ARCH_MINOR		:= 0

# Base commit to perform code check on
BASE_COMMIT		:= origin/master

# Debug/Release build
DEBUG			:= 0

# Build platform
DEFAULT_PLAT		:= fvp

# Whether the Firmware Update images (i.e. NS_BL1U and NS_BL2U images) should be
# built. The platform makefile is free to override this value.
FIRMWARE_UPDATE		:= 0

# Enable FWU helper functions and inline tests in NS_BL1U and NS_BL2U images.
FWU_BL_TEST := 1

# Whether a new test session should be started every time or whether the
# framework should try to resume a previous one if it was interrupted
NEW_TEST_SESSION	:= 1

# Use non volatile memory for storing results
USE_NVM			:= 0

# Build verbosity
V			:= 0

# Select the branch protection features to use
BRANCH_PROTECTION	:= 0

# Build RME stack
ENABLE_REALM_PAYLOAD_TESTS	:= 0

# Use the Firmware Handoff framework to receive configurations from preceding
# bootloader.
TRANSFER_LIST		:= 0

# This flag is required to match the feature set of Cactus SP that are
# implemented in TF-A EL3 SPMC.
SPMC_AT_EL3		:= 0

# If a Cactus SP subscribes to receiving power management framework message
# through its partition manifest, this flag controls whether the SP supports
# handling the aforementioned message. This option can take either 0
# (unsupported) or 1 (supported). Default value is 1. Note that a value of 0 is
# particularly useful in stress testing of power management handling by the SPMC.
CACTUS_PWR_MGMT_SUPPORT	:= 1
