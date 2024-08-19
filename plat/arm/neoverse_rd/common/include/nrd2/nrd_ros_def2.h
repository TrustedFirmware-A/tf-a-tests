/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * This file contains the RoS specific definitions for the second generation
 * platforms based on the N2/V2 CPU.
 */

#ifndef NRD_ROS_DEF2_H
#define NRD_ROS_DEF2_H

/*******************************************************************************
 * ROS configs
 ******************************************************************************/

/* Base address and size of external NVM flash */
#define NRD_ROS_FLASH_BASE			UL(0x08000000)  /* 128MB */
#define NRD_ROS_FLASH_SIZE			UL(0x04000000)  /* 64MB */

#endif /* NRD_ROS_DEF2_H */