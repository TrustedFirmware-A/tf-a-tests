/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * This file is limited to include the RoS specific definitions for the first
 * generation platforms based on the A75, N1 and V1 CPUs. RoS (Rest Of System)
 * is used to refer to the part of the reference design platform that excludes
 * CSS.
 */

#ifndef NRD_ROS_DEF1_H
#define NRD_ROS_DEF1_H

/*******************************************************************************
 * ROS configs
 ******************************************************************************/

/* Base address and size of external NVM flash */
#define NRD_ROS_FLASH_BASE			UL(0x08000000)  /* 128MB */
#define NRD_ROS_FLASH_SIZE			UL(0x04000000)  /* 64MB */

#endif /* NRD_ROS_DEF1_H */