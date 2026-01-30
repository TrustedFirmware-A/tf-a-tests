/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

#include "smmuv3_test_engine.h"

#include <debug.h>
#include <mmio.h>

int smmute_memcpy_dma(uintptr_t bar, uintptr_t source_pa, uintptr_t dest_pa)
{
	engine_pair_t *pairs = (engine_pair_t *)bar;
	uint32_t cur_cnt, trans_cnt, cmd;

	/* Configure DMA engine */
	privileged_frame_t *pframe = &pairs->privileged[0];
	user_frame_t *uframe = &pairs->user[0];

	/* Non-secure */
	mmio_write_32((uintptr_t)&pframe->pctrl, 1U);

	/* Not used with PCI */
	mmio_write_32((uintptr_t)&pframe->downstream_port_index, 0U);
	mmio_write_32((uintptr_t)&pframe->streamid, 0U);
	mmio_write_32((uintptr_t)&pframe->substreamid, NO_SUBSTREAMID);

	mmio_write_32((uintptr_t)&uframe->cmd, ENGINE_HALTED);
	mmio_write_32((uintptr_t)&uframe->uctrl, 0U);

	/* Configure source */
	mmio_write_64((uintptr_t)&uframe->begin, source_pa);
	mmio_write_64((uintptr_t)&uframe->end_incl, source_pa + SZ_4K - 1UL);

	/*
	 * Configure attributes for source and destination:
	 * rawWB, inner shareability, non-secure
	 */
	mmio_write_32((uintptr_t)&uframe->attributes, 0x42ff42ff);

	/* Copy from start to end */
	mmio_write_32((uintptr_t)&uframe->seed, 0U);

	/* Don't send MSI-X */
	mmio_write_64((uintptr_t)&uframe->msiaddress, 0UL);
	mmio_write_32((uintptr_t)&uframe->msidata, 0U);
	mmio_write_32((uintptr_t)&uframe->msiattr, 0U);

	/* Configure destination */
	mmio_write_64((uintptr_t)&uframe->udata, dest_pa);

	/* Copy everything */
	mmio_write_64((uintptr_t)&uframe->stride, 1UL);

	/* Read the current number of transactions */
	cur_cnt = mmio_read_32((uintptr_t)&uframe->count_of_transactions_returned);

	/* Start memcpy */
	mmio_write_32((uintptr_t)&uframe->cmd, ENGINE_MEMCPY);

	/* Wait for completion */
	do {
		trans_cnt =
			mmio_read_32((uintptr_t)&uframe->count_of_transactions_returned);
		cmd = mmio_read_32((uintptr_t)&uframe->cmd);
	} while (((int)cmd >= ENGINE_NO_FRAME) && (trans_cnt == cur_cnt));

	cmd = mmio_read_32((uintptr_t)&uframe->cmd);

	realm_printf("%u transaction(s) completed with %d\n",
		(trans_cnt - cur_cnt), (int)cmd);

	return (int)cmd;
}

