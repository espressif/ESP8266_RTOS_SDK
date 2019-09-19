// Copyright 2018-2019 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "sdkconfig.h"

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#include "esp_heap_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get "HEAP_ALIGN_SIZE" bytes aligned data(HEAP_ALIGN(ptr) >= ptr).
 */
#define HEAP_ALIGN(ptr)	        (((size_t)ptr + (HEAP_ALIGN_SIZE - 1)) & ~(HEAP_ALIGN_SIZE - 1))

#define MALLOC_CAP_32BIT	    (1 << 1)    ///< Memory must allow for aligned 32-bit data accesses
#define MALLOC_CAP_8BIT	        (1 << 2)    ///< Memory must allow for 8-bit data accesses
#define MALLOC_CAP_DMA		    (1 << 3)    ///< Memory must be able to accessed by DMA
#define MALLOC_CAP_INTERNAL     (1 << 11)   ///< Just for code compatibility
#define MALLOC_CAP_SPIRAM       (1 << 10)   ///< Just for code compatibility

#define MEM_HEAD_SIZE           sizeof(mem_blk_t)   ///< Size of first type memory block
#define MEM2_HEAD_SIZE          sizeof(mem2_blk_t)  ///< Size of second type memory block

/**
 * First type memory block.
 */
typedef struct mem_blk {
    struct mem_blk  *prev;  ///< Point to previous memory block
    struct mem_blk  *next;  ///< Point to next memory block 
} mem_blk_t;

/**
 * Second type memory block.
 */
typedef struct mem_blk2 {
    struct mem_blk2 *prev;  ///< Point to previous memory block
    struct mem_blk2 *next;  ///< Point to next memory block

    const char      *file;  ///< Which "file" allocate the memory block
    size_t          line;   ///< Which "line" allocate the memory block                      
} mem2_blk_t;

/**
 * User region information.
 */
typedef struct heap_region {
	void *start_addr;    ///< Heap region start address
	size_t total_size;      ///< Heap region total size by byte

	uint32_t caps;          ///< Heap capacity

	void *free_blk;      ///< First free memory block

	size_t free_bytes;      ///< Current free heap size by byte

	size_t min_free_bytes;  ///< Minimum free heap size by byte ever
} heap_region_t;


/**
 * @brief Get the total free size of all the regions that have the given capabilities
 *
 * This function takes all regions capable of having the given capabilities allocated in them
 * and adds up the free space they have.
 *
 * @param caps Bitwise OR of MALLOC_CAP_* flags indicating the type of memory
 *
 * @return Amount of free bytes in the regions
 */
size_t heap_caps_get_free_size(uint32_t caps);

/**
 * @brief Get the total minimum free memory of all regions with the given capabilities
 *
 * This adds all the low water marks of the regions capable of delivering the memory
 * with the given capabilities.
 *
 * @param caps Bitwise OR of MALLOC_CAP_* flags indicating the type of memory
 *
 * @return Amount of free bytes in the regions
 */
size_t heap_caps_get_minimum_free_size(uint32_t caps);

/**
 * @brief Initialize regions of memory to the collection of heaps at runtime.
 *
 * @param region region table head point
 * @param max_num region table size
 */
void esp_heap_caps_init_region(heap_region_t *region, size_t max_num);

/**
 * @brief Allocate a chunk of memory which has the given capabilities
 *
 * Equivalent semantics to libc malloc(), for capability-aware memory.
 *
 * In SDK, ``malloc(s)`` is equivalent to ``heap_caps_malloc(s, MALLOC_CAP_32BIT)``.
 *
 * @param size Size, in bytes, of the amount of memory to allocate
 * @param caps Bitwise OR of MALLOC_CAP_* flags indicating the type of memory to be returned
 *
 * @return A pointer to the memory allocated on success, NULL on failure
 */
#define heap_caps_malloc(size, caps) _heap_caps_malloc(size, caps, __ESP_FILE__, __LINE__)

void *_heap_caps_malloc(size_t size, uint32_t caps, const char *file, size_t line);

/**
 * @brief Free memory previously allocated via heap_caps_(m/c/re/z)alloc().
 *
 * Equivalent semantics to libc free(), for capability-aware memory.
 *
 *  In SDK, ``free(p)`` is equivalent to ``heap_caps_free(p)``.
 *
 * @param ptr Pointer to memory previously returned from heap_caps_(m/c/re/z)alloc(). Can be NULL.
 */
#define heap_caps_free(ptr) _heap_caps_free(ptr, __ESP_FILE__, __LINE__)

void _heap_caps_free(void *ptr, const char *file, size_t line);

/**
 * @brief Allocate a chunk of memory which has the given capabilities. The initialized value in the memory is set to zero.
 *
 * Equivalent semantics to libc calloc(), for capability-aware memory.
 *
 * In IDF, ``calloc(c, s)`` is equivalent to ``heap_caps_calloc(c, s, MALLOC_CAP_32BIT)``.
 *
 * @param n Number of continuing chunks of memory to allocate
 * @param size Size, in bytes, of a chunk of memory to allocate
 * @param caps Bitwise OR of MALLOC_CAP_* flags indicating the type of memory to be returned
 *
 * @return A pointer to the memory allocated on success, NULL on failure
 */
#define heap_caps_calloc(n, size, caps) _heap_caps_calloc(n, size, caps, __ESP_FILE__, __LINE__)

void *_heap_caps_calloc(size_t count, size_t size, uint32_t caps, const char *file, size_t line);

/**
 * @brief Reallocate memory previously allocated via heap_caps_(m/c/re/z)alloc().
 *
 * Equivalent semantics to libc realloc(), for capability-aware memory.
 *
 * In SDK, ``realloc(p, s)`` is equivalent to ``heap_caps_realloc(p, s, MALLOC_CAP_32BIT)``.
 *
 * 'caps' parameter can be different to the capabilities that any original 'ptr' was allocated with. In this way,
 * realloc can be used to "move" a buffer if necessary to ensure it meets a new set of capabilities.
 *
 * @param ptr Pointer to previously allocated memory, or NULL for a new allocation.
 * @param size Size of the new buffer requested, or 0 to free the buffer.
 * @param caps Bitwise OR of MALLOC_CAP_* flags indicating the type of memory desired for the new allocation.
 *
 * @return Pointer to a new buffer of size 'size' with capabilities 'caps', or NULL if allocation failed.
 */
#define heap_caps_realloc(ptr, size, caps) _heap_caps_realloc(ptr, size, caps, __ESP_FILE__, __LINE__)

void *_heap_caps_realloc(void *mem, size_t newsize, uint32_t caps, const char *file, size_t line);

/**
 * @brief Allocate a chunk of memory which has the given capabilities. The initialized value in the memory is set to zero.
 *
 * Equivalent semantics to libc calloc(), for capability-aware memory.
 *
 * In IDF, ``calloc(c, s)`` is equivalent to ``heap_caps_calloc(c, s, MALLOC_CAP_32BIT)``.
 *
 * @param size Size, in bytes, of a chunk of memory to allocate
 * @param caps Bitwise OR of MALLOC_CAP_* flags indicating the type of memory to be returned
 *
 * @return A pointer to the memory allocated on success, NULL on failure
 */
#define heap_caps_zalloc(size, caps) _heap_caps_zalloc(size, caps, __ESP_FILE__, __LINE__)

void *_heap_caps_zalloc(size_t size, uint32_t caps, const char *file, size_t line);

#ifdef __cplusplus
}
#endif
