/*
 * Copyright (c) 2018-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <cactus_message_loop.h>
#include <drivers/arm/pl011.h>
#include <drivers/console.h>
#include <lib/aarch64/arch_helpers.h>
#include <lib/hob/hob.h>
#include <lib/tftf_lib.h>
#include <lib/xlat_tables/xlat_mmu_helpers.h>
#include <lib/xlat_tables/xlat_tables_v2.h>

#include <ffa_helpers.h>
#include <plat_arm.h>
#include <plat/common/platform.h>
#include <platform_def.h>
#include <sp_debug.h>
#include <sp_helpers.h>
#include <spm_helpers.h>
#include <std_svc.h>

#include "sp_def.h"
#include "sp_tests.h"
#include "cactus.h"

/* Host machine information injected by the build system in the ELF file. */
extern const char build_message[];
extern const char version_string[];

extern void secondary_cold_entry(void);

/* Global ffa_id */
ffa_id_t g_ffa_id;

/*
 *
 * Message loop function
 * Notice we cannot use regular print functions because this serves to both
 * "primary" and "secondary" VMs. Secondary VM cannot access UART directly
 * but rather through Hafnium print hypercall.
 *
 */

static void __dead2 message_loop(ffa_id_t vm_id, struct mailbox_buffers *mb)
{
	struct ffa_value ffa_ret;
	ffa_id_t destination;

	/*
	* This initial wait call is necessary to inform SPMD that
	* SP initialization has completed. It blocks until receiving
	* a direct message request.
	*/

	ffa_ret = ffa_msg_wait();

	for (;;) {
		VERBOSE("Woke up with func:%s  id: %x\n",
			ffa_func_name(ffa_func_id(ffa_ret)),
			ffa_func_id(ffa_ret));

		if (ffa_func_id(ffa_ret) == FFA_ERROR) {
			ERROR("Error: %s\n",
			      ffa_error_name(ffa_error_code(ffa_ret)));
			break;
		}

		if (ffa_func_id(ffa_ret) != FFA_MSG_SEND_DIRECT_REQ_SMC32 &&
		    ffa_func_id(ffa_ret) != FFA_MSG_SEND_DIRECT_REQ_SMC64 &&
		    ffa_func_id(ffa_ret) != FFA_INTERRUPT &&
		    ffa_func_id(ffa_ret) != FFA_RUN) {
			ERROR("%s(%x) unknown func id %s\n", __func__, vm_id,
			      ffa_func_name(ffa_func_id(ffa_ret)));
			break;
		}

		if ((ffa_func_id(ffa_ret) == FFA_INTERRUPT) ||
		    (ffa_func_id(ffa_ret) == FFA_RUN)) {
			/*
			 * Received FFA_INTERRUPT in waiting state.
			 * The interrupt id is passed although this is just
			 * informational as we're running with virtual
			 * interrupts unmasked and the interrupt is processed
			 * by the interrupt handler.
			 */
			if (ffa_func_id(ffa_ret) == FFA_RUN) {
				/*
				 * Received FFA_RUN in waiting state, the
				 * endpoint simply returns by FFA_MSG_WAIT.
				 */
				VERBOSE("Nothing to do. Exit to NWd\n");
			}

			ffa_ret = ffa_msg_wait();
			continue;
		}

		destination = ffa_dir_msg_dest(ffa_ret);
		if (destination != vm_id) {
			ERROR("%s(%u) invalid vm id 0x%x\n",
				__func__, vm_id, destination);
			break;
		}

		if (!cactus_handle_cmd(&ffa_ret, &ffa_ret, mb)) {
			break;
		}
	}

	panic();
}

static const mmap_region_t cactus_mmap[] __attribute__((used)) = {
	MAP_REGION_FLAT(PLAT_CACTUS_DEVICE_BASE, PLAT_CACTUS_DEVICE_SIZE,
			MT_DEVICE | MT_RW),

	/* scratch memory allocated to be used for running SMMU tests */
	MAP_REGION_FLAT(PLAT_CACTUS_MEMCPY_BASE, PLAT_CACTUS_MEMCPY_RANGE,
			MT_MEMORY | MT_RW),
#if PLAT_fvp
	MAP_REGION_FLAT(PLAT_CACTUS_NS_MEMCPY_BASE, PLAT_CACTUS_MEMCPY_RANGE,
			MT_MEMORY | MT_RW | MT_NS),
#endif
	{0}
};

static void cactus_print_memory_layout(unsigned int vm_id)
{
	INFO("Secure Partition memory layout:\n");

	INFO("  Text region            : %p - %p\n",
		(void *)CACTUS_TEXT_START, (void *)CACTUS_TEXT_END);

	INFO("  Read-only data region  : %p - %p\n",
		(void *)CACTUS_RODATA_START, (void *)CACTUS_RODATA_END);

	INFO("  Data region            : %p - %p\n",
		(void *)CACTUS_DATA_START, (void *)CACTUS_DATA_END);

	INFO("  BSS region             : %p - %p\n",
		(void *)CACTUS_BSS_START, (void *)CACTUS_BSS_END);

	INFO("  RX                     : %p - %p\n",
		(void *)get_sp_rx_start(vm_id),
		(void *)get_sp_rx_end(vm_id));

	INFO("  TX                     : %p - %p\n",
		(void *)get_sp_tx_start(vm_id),
		(void *)get_sp_tx_end(vm_id));
}

static void cactus_print_hob_list(uint64_t hob_content_addr, size_t size)
{
	INFO("SP Hob List Contents: 0x%llx, size 0x%lx\n", hob_content_addr, size);
	struct efi_hob_handoff_info_table *hob_list = (struct
			efi_hob_handoff_info_table *) hob_content_addr;

	dump_hob_list(hob_list);
}

static void cactus_print_boot_info(struct ffa_boot_info_header *boot_info_header)
{
	struct ffa_boot_info_desc *boot_info_desc;

	if (boot_info_header == NULL) {
		NOTICE("SP doesn't have boot information!\n");
		return;
	}

	VERBOSE("SP boot info:\n");
	VERBOSE("  Signature: %x\n", boot_info_header->signature);
	VERBOSE("  Version: %x\n", boot_info_header->version);
	VERBOSE("  Blob Size: %u\n", boot_info_header->info_blob_size);
	VERBOSE("  Descriptor Size: %u\n", boot_info_header->desc_size);
	VERBOSE("  Descriptor Count: %u\n", boot_info_header->desc_count);

	boot_info_desc = boot_info_header->boot_info;

	if (boot_info_desc == NULL) {
		ERROR("Boot data arguments error...\n");
		return;
	}

	for (uint32_t i = 0; i < boot_info_header->desc_count; i++) {
		VERBOSE("    Boot Data:\n");
		VERBOSE("      Type: %u\n",
				ffa_boot_info_type(&boot_info_desc[i]));
		VERBOSE("      Type ID: %u\n",
				ffa_boot_info_type_id(&boot_info_desc[i]));
		VERBOSE("      Flags:\n");
		VERBOSE("        Name Format: %x\n",
				ffa_boot_info_name_format(&boot_info_desc[i]));
		VERBOSE("        Content Format: %x\n",
				ffa_boot_info_content_format(&boot_info_desc[i]));
		VERBOSE("      Size: %u\n", boot_info_desc[i].size);
		VERBOSE("      Value: %llx\n", boot_info_desc[i].content);
		if (ffa_boot_info_type_id(&boot_info_desc[i]) == 1) {
			cactus_print_hob_list(boot_info_desc[i].content, boot_info_desc[i].size);
		}
	}
}

static void cactus_plat_configure_mmu(unsigned int vm_id)
{
	mmap_add_region(CACTUS_TEXT_START,
			CACTUS_TEXT_START,
			CACTUS_TEXT_END - CACTUS_TEXT_START,
			MT_CODE);
	mmap_add_region(CACTUS_RODATA_START,
			CACTUS_RODATA_START,
			CACTUS_RODATA_END - CACTUS_RODATA_START,
			MT_RO_DATA);
	mmap_add_region(CACTUS_DATA_START,
			CACTUS_DATA_START,
			CACTUS_DATA_END - CACTUS_DATA_START,
			MT_RW_DATA);
	mmap_add_region(CACTUS_BSS_START,
			CACTUS_BSS_START,
			CACTUS_BSS_END - CACTUS_BSS_START,
			MT_RW_DATA);

	mmap_add_region(get_sp_rx_start(vm_id),
			get_sp_rx_start(vm_id),
			(SP_RX_TX_SIZE / 2),
			MT_RO_DATA);

	mmap_add_region(get_sp_tx_start(vm_id),
			get_sp_tx_start(vm_id),
			(SP_RX_TX_SIZE / 2),
			MT_RW_DATA);

	/* Skip additional mapping for cactus StMM. */
	if (vm_id != SP_ID(5)) {
		mmap_add(cactus_mmap);
	}
	init_xlat_tables();
}

static struct ffa_value register_secondary_entrypoint(void)
{
	struct ffa_value args;

	args.fid = FFA_SECONDARY_EP_REGISTER_SMC64;
	args.arg1 = (u_register_t)&secondary_cold_entry;

	return ffa_service_call(&args);
}

static void cactus_map_boot_info(struct ffa_boot_info_header *boot_info_header,
		bool do_desc_access)
{
	struct ffa_boot_info_desc *boot_info_desc;

	assert(boot_info_header != NULL);

	boot_info_desc = boot_info_header->boot_info;
	/*
	 * TODO: Currently just validating that cactus can
	 * access the boot info descriptors. By default, allocate one page
	 * for boot info. In case we want to use the boot info contents, we should check the
	 * blob and remap if the size is bigger than one page.
	 * Only then access the contents.
	 */
	mmap_add_dynamic_region(
		(unsigned long long)boot_info_header,
		(uintptr_t)boot_info_header,
		PAGE_SIZE, MT_RO_DATA);

	if (do_desc_access) {
		for (uint32_t i = 0; i < boot_info_header->desc_count; i++) {
			mmap_add_dynamic_region(
				(unsigned long long)boot_info_desc[i].content,
				(uintptr_t)boot_info_desc[i].content,
				PAGE_SIZE, MT_RO_DATA);
		}
	}
}

void __dead2 cactus_main(bool primary_cold_boot,
			 struct ffa_boot_info_header *boot_info_header)
{
	assert(IS_IN_EL1() != 0);

	struct mailbox_buffers mb;
	struct ffa_value ret;

	/* Get current FFA id */
	struct ffa_value ffa_id_ret = ffa_id_get();
	ffa_id_t ffa_id = ffa_endpoint_id(ffa_id_ret);
	if (ffa_func_id(ffa_id_ret) != FFA_SUCCESS_SMC32) {
		ERROR("FFA_ID_GET failed.\n");
		panic();
	}

	if (primary_cold_boot == true) {
		/* Clear BSS */
		memset((void *)CACTUS_BSS_START,
		       0, CACTUS_BSS_END - CACTUS_BSS_START);

		mb.send = (void *) get_sp_tx_start(ffa_id);
		mb.recv = (void *) get_sp_rx_start(ffa_id);

		/* Configure and enable Stage-1 MMU, enable D-Cache */
		cactus_plat_configure_mmu(ffa_id);

		/* Initialize locks for tail end interrupt handler */
		sp_handler_spin_lock_init();

		if (boot_info_header != NULL) {
			cactus_map_boot_info(boot_info_header, ffa_id == SP_ID(5));
		}
	}

	/*
	 * The local ffa_id value is held on the stack. The global g_ffa_id
	 * value is set after BSS is cleared.
	 */
	g_ffa_id = ffa_id;

	enable_mmu_el1(0);

	/* Enable IRQ/FIQ */
	enable_irq();
	enable_fiq();

	if (primary_cold_boot == false) {
		goto msg_loop;
	}

	set_putc_impl(FFA_SVC_SMC_CALL_AS_STDOUT);

	/* Below string is monitored by CI expect script. */
	NOTICE("Booting Secure Partition (ID: %x)\n%s\n%s\n",
		ffa_id, build_message, version_string);

	/*
	 * Print FF-A boot info if requested in manifest via FF-A boot info
	 * protocol.
	 */
	if (ffa_id == SP_ID(1) || ffa_id == SP_ID(5)) {
		cactus_print_boot_info(boot_info_header);
	}

	cactus_print_memory_layout(ffa_id);

	/*
	 * Cactus-tertiary and cactus-stmm make use of FFA_RXTX_MAP API
	 * instead of specifying `rx_tx-info` in their manifests, as done by the
	 * primary and secondary cactus partitions.
	 */
	if (ffa_id == SP_ID(3) || ffa_id == SP_ID(5)) {
		VERBOSE("Mapping RXTX Region\n");
		CONFIGURE_AND_MAP_MAILBOX(mb, PAGE_SIZE, ret);
		if (ffa_func_id(ret) != FFA_SUCCESS_SMC32) {
			ERROR("Failed to map RXTX buffers. Error: %s\n",
			      ffa_error_name(ffa_error_code(ret)));
			panic();
		}
	}


	ret = register_secondary_entrypoint();

	/* FFA_SECONDARY_EP_REGISTER interface is not supported for UP SP. */
	if (ffa_id == SP_ID(3) || ffa_id == SP_ID(5)) {
		EXPECT(ffa_func_id(ret), FFA_ERROR);
		EXPECT(ffa_error_code(ret), FFA_ERROR_NOT_SUPPORTED);
	} else {
		EXPECT(ffa_func_id(ret), FFA_SUCCESS_SMC32);
	}

#if !SPMC_AT_EL3
	discover_managed_exit_interrupt_id();
	register_maintenance_interrupt_handlers();

	/* Invoking self tests */
	ffa_tests(&mb, true);
	cpu_feature_tests();
#endif

msg_loop:
	/* End up to message loop */
	message_loop(ffa_id, &mb);

	/* Not reached */
}
