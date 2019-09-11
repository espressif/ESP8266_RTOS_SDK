

// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
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

#ifndef _IRQFLAGS_H
#define _IRQFLAGS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_IDF_TARGET_ESP8266
/**
 * @brief save IRQ state and disable IRQ
 *
 * @return saved IRQ state
 */
static inline uint32_t arch_local_irq_save(void)
{
    uint32_t tmp;

    __asm__ __volatile__ ("rsil %0, 3" : "=a" (tmp) :: "memory");

    return tmp;
}

/**
 * @brief restore IRQ state
 *
 * @param tmp saved IRQ state
 */
static inline void arch_local_irq_restore(uint32_t tmp)
{
    __asm__ __volatile__ ("wsr %0, ps" :: "a" (tmp) : "memory");
}

#define local_irq_declare(_t)       uint32_t (_t)
#define local_irq_save(_t)          (_t) = arch_local_irq_save()
#define local_irq_restore(_t)       arch_local_irq_restore(_t)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _IRQFLAGS_H */
