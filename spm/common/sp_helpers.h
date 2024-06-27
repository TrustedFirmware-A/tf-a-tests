/*
 * Copyright (c) 2018-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SP_HELPERS_H
#define SP_HELPERS_H

#include <stdint.h>
#include <tftf_lib.h>
#include <spm_common.h>
#include <spinlock.h>

/*
 * Use extended SPI interrupt ID range, hafnium/SPMC maps virtual interrupts
 * to physical interrupts 1 to 1.
 */
#define NUM_VINT_ID	5120

typedef struct {
	u_register_t fid;
	u_register_t arg1;
	u_register_t arg2;
	u_register_t arg3;
	u_register_t arg4;
	u_register_t arg5;
	u_register_t arg6;
	u_register_t arg7;
} svc_args;

/*
 * Trigger an SVC call.
 *
 * The arguments to pass through the SVC call must be stored in the svc_args
 * structure. The return values of the SVC call will be stored in the same
 * structure (overriding the input arguments).
 *
 * Return the first return value. It is equivalent to args.fid but is also
 * provided as the return value for convenience.
 */
u_register_t sp_svc(svc_args *args);

/*
 * Choose a pseudo-random number within the [min,max] range (both limits are
 * inclusive).
 */
uintptr_t bound_rand(uintptr_t min, uintptr_t max);

/*
 * Check that expr == expected.
 * If not, loop forever.
 */
#define EXPECT(lhs, rhs)                                                  \
	do {                                                              \
		int lhs_value = (lhs);                                    \
		int rhs_value = (rhs);                                    \
		if (lhs_value != rhs_value) {                             \
			ERROR("%s:%d: Assertion failed: `%s == %s`\n",    \
			      __FILE__, __LINE__, #lhs, #rhs);            \
			ERROR("lhs = %d (0x%x)\n", lhs_value, lhs_value); \
			ERROR("rhs = %d (0x%x)\n", rhs_value, rhs_value); \
			while (1) {                                       \
				continue;                                 \
			}                                                 \
		}                                                         \
	} while (0)

/*
 * Test framework functions
 */

void announce_test_section_start(const char *test_sect_desc);
void announce_test_section_end(const char *test_sect_desc);

void announce_test_start(const char *test_desc);
void announce_test_end(const char *test_desc);

/* Sleep for at least 'ms' milliseconds and return the elapsed time(ms). */
uint64_t sp_sleep_elapsed_time(uint32_t ms);

/* Sleep for at least 'ms' milliseconds. */
void sp_sleep(uint32_t ms);

void sp_handler_spin_lock_init(void);

/* Handler invoked by SP while processing interrupt. */
extern void (*sp_interrupt_handler[NUM_VINT_ID])(void);

/* Register the handler. */
void sp_register_interrupt_handler(void (*handler)(void),
						uint32_t interrupt_id);

/* Un-register the handler. */
void sp_unregister_interrupt_handler(uint32_t interrupt_id);

void discover_managed_exit_interrupt_id(void);

void register_maintenance_interrupt_handlers(void);

#endif /* SP_HELPERS_H */
