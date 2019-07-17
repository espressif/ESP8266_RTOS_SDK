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

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MEM_BLK_TAG             0x80000000  ///< Mark the memory block used
#define MEM_BLK_TRACE           0x80000000  ///< Mark the memory block traced

#define _mem_blk_get_ptr(_mem_blk, _offset, _mask)                          \
    ((mem_blk_t *)((((uint32_t *)(_mem_blk))[_offset]) & (~_mask)))

#define _mem_blk_set_ptr(_mem_blk, _val, _offset, _mask)                    \
{                                                                           \
    uint32_t *__p = (uint32_t *)(_mem_blk);                                 \
    uint32_t __bits = __p[_offset] & (_mask);                               \
                                                                            \
    __p[_offset] = (uint32_t)(_val) | __bits;                               \
}

#define mem_blk_prev(_mem_blk) _mem_blk_get_ptr(_mem_blk, 0, MEM_BLK_TAG)
#define mem_blk_next(_mem_blk) _mem_blk_get_ptr(_mem_blk, 1, MEM_BLK_TRACE)

#define mem_blk_set_prev(_mem_blk, _prev) _mem_blk_set_ptr(_mem_blk, _prev, 0, MEM_BLK_TAG)
#define mem_blk_set_next(_mem_blk, _next) _mem_blk_set_ptr(_mem_blk, _next, 1, MEM_BLK_TRACE)

static inline size_t mem_blk_head_size(bool trace)
{
    return trace ? MEM2_HEAD_SIZE : MEM_HEAD_SIZE;    
}

static inline void mem_blk_set_traced(mem2_blk_t *mem_blk, const char *file, size_t line)
{
    uint32_t *val = (uint32_t *)mem_blk + 1;

    *val |= MEM_BLK_TRACE;

    mem_blk->file = file;
    mem_blk->line = line | MEM_BLK_TRACE;
}

static inline void mem_blk_set_untraced(mem2_blk_t *mem_blk)
{
    uint32_t *val = (uint32_t *)mem_blk + 1;

    *val &= ~MEM_BLK_TRACE;
}

static inline int mem_blk_is_traced(mem_blk_t *mem_blk)
{
    uint32_t *val = (uint32_t *)mem_blk + 1;

    return *val & MEM_BLK_TRACE;
}

static inline void mem_blk_set_used(mem_blk_t *mem_blk)
{
    uint32_t *val = (uint32_t *)mem_blk;

    *val |= MEM_BLK_TAG;
}

static inline void mem_blk_set_unused(mem_blk_t *mem_blk)
{
    uint32_t *val = (uint32_t *)mem_blk;

    *val &= ~MEM_BLK_TAG;
}

static inline int mem_blk_is_used(mem_blk_t *mem_blk)
{
    uint32_t *val = (uint32_t *)mem_blk;

    return *val & MEM_BLK_TAG;
}

static inline int mem_blk_is_end(mem_blk_t *mem_blk)
{
    return mem_blk_next(mem_blk) == NULL;
}

static inline size_t blk_link_size(mem_blk_t *blk)
{
    return (size_t)mem_blk_next(blk) - (size_t)blk;
}

static inline size_t get_blk_region(void *ptr)
{
    size_t num;
    extern size_t g_heap_region_num;
    extern heap_region_t g_heap_region[];


    for (num = 0; num < g_heap_region_num; num++) {
        if ((uint8_t *)ptr > (uint8_t *)g_heap_region[num].start_addr
            && (uint8_t *)ptr < ((uint8_t *)g_heap_region[num].start_addr + g_heap_region[num].total_size)) {
            break;
        }
    }

    return num;
}

static inline size_t ptr2memblk_size(size_t size, bool trace)
{
    size_t head_size = trace ? MEM2_HEAD_SIZE : MEM_HEAD_SIZE;

    return HEAP_ALIGN(size + head_size);
}

static inline bool ptr_is_traced(void *ptr)
{
    uint32_t *p = (uint32_t *)ptr - 1;

    return p[0] & MEM_BLK_TRACE ? true : false;
}

static inline mem_blk_t *ptr2blk(void *ptr, bool trace)
{
    size_t head_size = trace ? MEM2_HEAD_SIZE : MEM_HEAD_SIZE;

    return (mem_blk_t *)((uint8_t *)ptr - head_size);
}

static inline void *blk2ptr(mem_blk_t *mem_blk, bool trace)
{
    size_t head_size = trace ? MEM2_HEAD_SIZE : MEM_HEAD_SIZE;

    return (void *)((uint8_t *)mem_blk + head_size);
}

static inline size_t ptr_size(void *ptr)
{
    bool trancd = ptr_is_traced(ptr);
    mem_blk_t *blk_mem = ptr2blk(ptr, trancd);
    size_t size = blk_link_size(blk_mem) - mem_blk_head_size(trancd);

    return size;
}

static inline size_t mem2_blk_line(mem2_blk_t *mem2_blk)
{
    return mem2_blk->line & ~MEM_BLK_TRACE;
}

#ifdef __cplusplus
}
#endif
