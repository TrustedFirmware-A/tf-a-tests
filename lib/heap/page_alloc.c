/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdbool.h>
#include <stdint.h>

#include <debug.h>
#include <heap/page_alloc.h>
#include <spinlock.h>
#include <utils_def.h>

#include <platform_def.h>

static uint64_t memory_used;
static uint64_t heap_base_addr;
static u_register_t heap_addr;
static uint64_t heap_size;
static bool heap_initialised = HEAP_INIT_FAILED;
static spinlock_t mem_lock;

/*
 * Initialize the memory heap space to be used
 * @heap_base: heap base address
 * @heap_len: heap size for use
 */
int page_pool_init(uint64_t heap_base, uint64_t heap_len)
{
	const uint64_t plat_max_addr = (uint64_t)DRAM_BASE + (uint64_t)DRAM_SIZE;
	uint64_t max_addr = heap_base + heap_len;

	if (heap_len == 0ULL) {
		ERROR("heap_len must be non-zero value\n");
		heap_initialised = HEAP_INVALID_LEN;
	} else if (max_addr >= plat_max_addr) {
		ERROR("heap_base + heap[0x%llx] must not exceed platform"
			"max address[0x%llx]\n", max_addr, plat_max_addr);

		heap_initialised = HEAP_OUT_OF_RANGE;
	} else {
		heap_base_addr = heap_base;
		memory_used = heap_base;
		heap_size = heap_len;
		heap_initialised = HEAP_INIT_SUCCESS;
	}
	return heap_initialised;
}

/*
 * Return the pointer to the allocated pages
 * @bytes_size: pages to allocate in byte unit
 */
void *page_alloc(u_register_t bytes_size)
{
	if (heap_initialised != HEAP_INIT_SUCCESS) {
		ERROR("heap need to be initialised first\n");
		return HEAP_NULL_PTR;
	}
	if (bytes_size == 0UL) {
		ERROR("bytes_size must be non-zero value\n");
		return HEAP_NULL_PTR;
	}

	spin_lock(&mem_lock);

	if ((memory_used + bytes_size) >= (heap_base_addr + heap_size)) {
		ERROR("Reached to max KB allowed[%llu]\n", (heap_size/1024U));
		goto unlock_failed;
	}

	/* Set pointer to current used heap memory cursor */
	heap_addr = memory_used;

	/* Move used memory cursor by bytes_size */
	memory_used += bytes_size;
	spin_unlock(&mem_lock);

	return (void *)heap_addr;

	/* Failed allocation */
unlock_failed:
	spin_unlock(&mem_lock);
	return HEAP_NULL_PTR;
}

/*
 * Return a pointer to the allocated pages with the specified alignment.
 *
 * @alignment must be a power of 2.
 */
void *page_alloc_aligned(u_register_t bytes_size, u_register_t alignment)
{
	uint64_t aligned_addr;
	u_register_t total_bytes_size;

	if (heap_initialised != HEAP_INIT_SUCCESS) {
		ERROR("heap need to be initialised first\n");
		return HEAP_NULL_PTR;
	}

	if (bytes_size == 0UL) {
		ERROR("bytes_size must be non-zero value\n");
		return HEAP_NULL_PTR;
	}

	if (!is_power_of_2(alignment)) {
		ERROR("alignment must be power of 2\n");
		return HEAP_NULL_PTR;
	}

	spin_lock(&mem_lock);

	/* Calculate the aligned address based on current used memory cursor */
	aligned_addr = round_up(memory_used, alignment);

	/* Calculate the total bytes size needed for allocation */
	total_bytes_size = (aligned_addr - memory_used) + bytes_size;

	if ((memory_used + total_bytes_size) >= (heap_base_addr + heap_size)) {
		ERROR("Reached to max KB allowed[%llu]\n", (heap_size/1024U));
		goto unlock_failed;
	}

	/* Set pointer to current used heap memory cursor */
	heap_addr = memory_used;

	/* Move used memory cursor by total_bytes_size */
	memory_used += total_bytes_size;
	spin_unlock(&mem_lock);

	return (void *)aligned_addr;

	/* Failed allocation */
unlock_failed:
	spin_unlock(&mem_lock);
	return HEAP_NULL_PTR;
}

/*
 * Reset heap memory usage cursor to heap base address
 */
void page_pool_reset(void)
{
	/*
	 * No race condition here, only lead cpu running TFTF test case can
	 * reset the memory allocation
	 */
	memory_used = heap_base_addr;
}

void page_free(u_register_t address)
{
	/* No memory free is needed in current TFTF test scenarios */
}
