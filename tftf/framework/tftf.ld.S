/*
 * Copyright (c) 2020-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform_def.h>
#include <xlat_tables_defs.h>
#include <host_realm_mem_layout.h>


OUTPUT_FORMAT(PLATFORM_LINKER_FORMAT)
OUTPUT_ARCH(PLATFORM_LINKER_ARCH)
ENTRY(tftf_entrypoint)

MEMORY {
    RAM (rwx): ORIGIN = TFTF_BASE, LENGTH = DRAM_SIZE
}


SECTIONS
{
    . = TFTF_BASE;
    __TFTF_BASE__ = .;

    .text . : {
        __TEXT_START__ = .;
        *entrypoint.o(.text*)
        *(.text*)
        *(.vectors)
        . = NEXT(PAGE_SIZE);
        __TEXT_END__ = .;
    } >RAM

    .rodata . : {
        __RODATA_START__ = .;
        *(.rodata*)
        /*
         * Memory page(s) mapped to this section will be marked as
         * read-only, executable.  No RW data from the next section must
         * creep in.  Ensure the rest of the current memory page is unused.
         */
        . = NEXT(PAGE_SIZE);
        __RODATA_END__ = .;
    } >RAM

    .data : {
        __DATA_START__ = .;
        *(.data*)
        . = NEXT(PAGE_SIZE); /* This ensures tftf.bin is aligned to page size. */
        __DATA_END__ = .;
    } >RAM

   /* End of LOAD Sections. NOLOAD sections begin here. */
   /*
    * Memory for Realm Image has to follow next as it will appended to end
    * of tftf.bin.
    */
    realm_payload (NOLOAD) : {
        __REALM_PAYLOAD_START__ = .;
        . = __REALM_PAYLOAD_START__ + REALM_MAX_LOAD_IMG_SIZE;
        __REALM_PAYLOAD_END__ = .;
    } >RAM

    /* Memory pool for Realm payload tests. */
    realm_pool (NOLOAD)  : ALIGN(PAGE_SIZE) {
        __REALM_POOL_START__ = .;
        . = __REALM_POOL_START__ + PAGE_POOL_MAX_SIZE;
        __REALM_POOL_END__ = .;
    } >RAM

    stacks (NOLOAD) : ALIGN(16) {
        __STACKS_START__ = .;
        *(tftf_normal_stacks)
        __STACKS_END__ = .;
    } >RAM

    /*
     * The .bss section gets initialised to 0 at runtime.
     * Its base address is always PAGE_SIZE aligned.
     */
    .bss : {
        __BSS_START__ = .;
        *(SORT_BY_ALIGNMENT(.bss*))
        *(COMMON)
        __BSS_END__ = .;
    } >RAM

    /*
     * The xlat_table section is for full, aligned page tables (4K).
     * Removing them from .bss eliminates the unecessary zero init
     */
    xlat_table (NOLOAD) : ALIGN(PAGE_SIZE) {
        *(xlat_table)
    } >RAM

    /*
     * The SMC fuzzing module requires alignment due to malloc
     * constraints. Also size must be at least around 64K
     */
    smcfuzz (NOLOAD) : {
        *(smcfuzz)
    } >RAM

    /*
     * The base address of the coherent memory section must be page-aligned (4K)
     * to guarantee that the coherent data are stored on their own pages and
     * are not mixed with normal data.  This is required to set up the correct
     * memory attributes for the coherent data page tables.
     */
    coherent_ram (NOLOAD) : ALIGN(PAGE_SIZE) {
        __COHERENT_RAM_START__ = .;
        *(tftf_coherent_stacks)
        *(tftf_coherent_mem)
        __COHERENT_RAM_END_UNALIGNED__ = .;
        /*
         * Memory page(s) mapped to this section will be marked
         * as device memory.  No other unexpected data must creep in.
         * Ensure the rest of the current memory page is unused.
         */
        . = NEXT(PAGE_SIZE);
        __COHERENT_RAM_END__ = .;
    } >RAM

    __COHERENT_RAM_UNALIGNED_SIZE__ =
        __COHERENT_RAM_END_UNALIGNED__ - __COHERENT_RAM_START__;

    __TFTF_END__ = .;

    __BSS_SIZE__ = SIZEOF(.bss);

}
