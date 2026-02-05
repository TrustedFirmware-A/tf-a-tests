/*
 * Copyright (c) 2018, Arm Limited. All rights reserved.
 * Copyright (c) 2024, Linaro Limited.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PRIVATE_TIMER_H__
#define __PRIVATE_TIMER_H__

void private_timer_start(unsigned long timeo);
void private_timer_stop(void);
void private_timer_save(void);
void private_timer_restore(void);

int arm_gen_timer_init(void);
int arm_gen_timer_handler(void);
int arm_gen_timer_cancel(void);
int arm_gen_timer_program(unsigned long time_out_ms);

#endif /* __PRIVATE_TIMER_H__ */
