/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Versal PGGS access implementation.
 *
 * Uses IOCTL_READ_PGGS / IOCTL_WRITE_PGGS on the APU core node to reach a
 * PGGS scratch register that persists across resets.
 */

#include <assert.h>

#include "eemi_api.h"
#include "xpm_defs.h"
#include "xpm_nodeid_plat.h"
#include "xpm_pggs.h"

/*
 * Index of the PGGS scratch register used as the persistent store.
 * The value selects the PGGS register (0 selects PGGS0); change it to
 * target another PGGS register.
 */
#define PM_PGGS_INDEX		0U

/* Trailing IOCTL arguments that are unused by the PGGS access path. */
#define PGGS_IOCTL_UNUSED_ARG	0U

int xpm_pggs_read(uint32_t *const val)
{
	assert(val != NULL);

	return xpm_ioctl(PM_DEV_ACPU_CORE, IOCTL_READ_PGGS, PM_PGGS_INDEX,
			 PGGS_IOCTL_UNUSED_ARG, PGGS_IOCTL_UNUSED_ARG, val);
}

int xpm_pggs_write(const uint32_t val)
{
	return xpm_ioctl(PM_DEV_ACPU_CORE, IOCTL_WRITE_PGGS, PM_PGGS_INDEX,
			 val, PGGS_IOCTL_UNUSED_ARG, NULL);
}
