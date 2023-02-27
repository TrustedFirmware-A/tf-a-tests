/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __SERROR_H__
#define  __SERROR_H__

typedef bool (*exception_handler_t)(void);
void register_custom_serror_handler(exception_handler_t handler);
void unregister_custom_serror_handler(void);

#endif /* __SERROR_H__ */
