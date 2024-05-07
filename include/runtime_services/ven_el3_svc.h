/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef VEN_EL3_SVC_H
#define VEN_EL3_SVC_H

/*
 * Definitions related to the Vendor-Specific El3 as per the SMC Calling Convention.
 */

/* VEN_EL3_SMC_32		0x87000000U */
/* VEN_EL3_SMC_64		0xC7000000U */

#define VEN_EL3_SVC_UID		0x8700ff01
/*				0x8700ff02 is reserved */
#define VEN_EL3_SVC_VERSION	0x8700ff03

#define VEN_EL3_SVC_VERSION_MAJOR	1
#define VEN_EL3_SVC_VERSION_MINOR	0

#endif /* VEN_EL3_SVC_H */
