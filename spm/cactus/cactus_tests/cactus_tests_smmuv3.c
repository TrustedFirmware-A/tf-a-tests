/*
 * Copyright (c) 2021-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

#include <arch_helpers.h>
#include "cactus.h"
#include "cactus_message_loop.h"
#include <sp_platform_def.h>
#include "cactus_test_cmds.h"
#include <debug.h>
#include <ffa_helpers.h>
#include <mmio.h>
#include "smmuv3_test_engine.h"
#include <sp_helpers.h>
#include "sp_tests.h"
#include <spm_common.h>

/* Source and target address for memcopy operation */
#define MEMCPY_SOURCE_BASE	PLAT_CACTUS_MEMCPY_BASE
#define MEMPCY_TOTAL_SIZE	(PLAT_CACTUS_MEMCPY_RANGE / 2)
#define MEMCPY_TARGET_BASE	(MEMCPY_SOURCE_BASE + MEMPCY_TOTAL_SIZE)

/* Miscellaneous */
#define NO_SUBSTREAMID	(0xFFFFFFFFU)
#define TRANSFER_SIZE	(MEMPCY_TOTAL_SIZE / FRAME_COUNT)
#define LOOP_COUNT	(5000U)

/*
 * This function will configure an SMMUv3TestEngine to make access to the Secure
 * PAS. To do that it uses a regularly (non-PCIe) connected device in place of
 * the PL 330 cluster. This is non standard and must be enabled on the FVP
 * commandline with:
 * -C pci.dma330x4.use_smmuv3testengine_not_dmacs=1
 *
 * Some notes about the model:
 *
 * A DMAC is a DMA-Controller a DMA-330 (a.k.a. DMAC_PL_330, a.k.a. PL_330). It
 * is an ancient (32b) device that has its own little instruction set and
 * several concurrent threads of execution that can be used to move memory
 * about.
 *
 * The original DMAC cluster was put in so that it provides an easy work-load to
 * program for the SMMU rather than a full PCIe device.
 *
 * The PCIe block diagram is not quite right depending on your point of view.
 * The PCIe Subsystem is above the SMMU – i.e. all accesses made by PCIe devices
 * go through the SMMU.
 *
 * The DMAC cluster is also above the same SMMU and so is (mostly)
 * indistinguishable from the PCIe device traffic.
 */

static bool run_smmuv3_test(void)
{
	uint64_t source_addr, cpy_range, target_addr;
	uint64_t begin_addr, end_addr, dest_addr;
	uint32_t status;
	unsigned int i, f, attempts;

	/*
	 * The test engine's MEMCPY command copies data from the region in
	 * range [begin, end_incl] to the region with base address as udata.
	 * In this test, we configure the test engine to initiate memcpy from
	 * scratch page located at MEMCPY_SOURCE_BASE to the page located at
	 * address MEMCPY_TARGET_BASE
	 */

	VERBOSE("CACTUS: Running SMMUv3 test\n");

	source_addr = MEMCPY_SOURCE_BASE;
	cpy_range = MEMPCY_TOTAL_SIZE;
	target_addr = MEMCPY_TARGET_BASE;
	uint32_t streamID_list[] = { 0U, 1U };

	uint64_t data[] = {
		ULL(0xBAADFEEDCEEBDAAF),
		ULL(0x0123456776543210)
	};

	/* Write pre-determined content to source pages */
	for (i = 0U; i < (cpy_range / 8U); i++) {
		mmio_write64_offset(source_addr, i * 8, data[i%2]);
	}

	/* Clean the data caches */
	clean_dcache_range(source_addr, cpy_range);

	/*
	 * Make sure above load, store and cache maintenance instructions
	 * complete before we start writing to TestEngine frame configuration
	 * fields
	 */
	dsbsy();

	for (f = 0U; f < FRAME_COUNT; f++) {
		attempts = 0U;
		begin_addr = source_addr + (TRANSFER_SIZE * f);
		end_addr = begin_addr + TRANSFER_SIZE - 1U;
		dest_addr = target_addr + (TRANSFER_SIZE * f);

		/* Initiate DMA sequence */
		mmio_write32_offset(PRIV_BASE_FRAME + F_IDX(f), PCTRL_OFF, 0);
		mmio_write32_offset(PRIV_BASE_FRAME + F_IDX(f), DOWNSTREAM_PORT_OFF, 0);
		mmio_write32_offset(PRIV_BASE_FRAME + F_IDX(f), STREAM_ID_OFF, streamID_list[f%2]);
		mmio_write32_offset(PRIV_BASE_FRAME + F_IDX(f), SUBSTREAM_ID_OFF, NO_SUBSTREAMID);

		mmio_write32_offset(USR_BASE_FRAME + F_IDX(f), UCTRL_OFF, 0);
		mmio_write32_offset(USR_BASE_FRAME + F_IDX(f), SEED_OFF, 0);
		mmio_write64_offset(USR_BASE_FRAME + F_IDX(f), BEGIN_OFF, begin_addr);
		mmio_write64_offset(USR_BASE_FRAME + F_IDX(f), END_CTRL_OFF, end_addr);

		/* Legal values for stride: 1 and any multiples of 8 */
		mmio_write64_offset(USR_BASE_FRAME + F_IDX(f), STRIDE_OFF, 1);
		mmio_write64_offset(USR_BASE_FRAME + F_IDX(f), UDATA_OFF, dest_addr);

		mmio_write32_offset(USR_BASE_FRAME + F_IDX(f), CMD_OFF, ENGINE_MEMCPY);
		VERBOSE("SMMUv3TestEngine: Waiting for MEMCPY completion for frame: %u\n", f);

		/*
		 * It is guaranteed that a read of "cmd" fields after writing to it will
		 * immediately return ENGINE_FRAME_MISCONFIGURED if the command was
		 * invalid.
		 */
		if (mmio_read32_offset(USR_BASE_FRAME + F_IDX(f), CMD_OFF) == ENGINE_MIS_CFG) {
			ERROR("SMMUv3TestEngine: Misconfigured for frame: %u\n", f);
			return false;
		}

		/* Wait for mem copy to be complete */
		while (attempts++ < LOOP_COUNT) {
			status = mmio_read32_offset(USR_BASE_FRAME + F_IDX(f), CMD_OFF);
			if (status == ENGINE_HALTED) {
				break;
			} else if (status == ENGINE_ERROR) {
				ERROR("SMMUv3: Test failed\n");
				return false;
			}

			/*
			 * TODO: Introduce a small delay here to make sure the
			 * CPU memory accesses do not starve the interconnect
			 * due to continuous polling.
			 */
		}

		if (attempts == LOOP_COUNT) {
			ERROR("SMMUv3: Test failed\n");
			return false;
		}

		dsbsy();
	}

	/*
	 * Invalidate cached entries to force the CPU to fetch the data from
	 * Main memory
	 */
	inv_dcache_range(source_addr, cpy_range);
	inv_dcache_range(target_addr, cpy_range);

	/* Compare source and destination memory locations for data */
	for (i = 0U; i < (cpy_range / 8U); i++) {
		if (mmio_read_64(source_addr + 8 * i) != mmio_read_64(target_addr + 8 * i)) {
			ERROR("SMMUv3: Mem copy failed: %llx\n", target_addr + 8 * i);
			return false;
		}
	}

	return true;
}

CACTUS_CMD_HANDLER(smmuv3_cmd, CACTUS_DMA_SMMUv3_CMD)
{
	struct ffa_value ffa_ret;
	ffa_id_t vm_id = ffa_dir_msg_dest(*args);
	ffa_id_t source = ffa_dir_msg_source(*args);

	VERBOSE("Received request through direct message for DMA service\n");

	/*
	 * At present, the test cannot be run concurrently on multiple SPs as
	 * there is only one SMMUv3TestEngine IP in the FVP model. Hence, run
	 * the test only on the first SP.
	 */
	if (vm_id != SPM_VM_ID_FIRST) {
		return cactus_error_resp(vm_id, source, 0);
	}

	if (run_smmuv3_test()) {
		ffa_ret = cactus_success_resp(vm_id, source, 0);
	} else {
		ffa_ret = cactus_error_resp(vm_id, source, 0);
	}

	return ffa_ret;
}
