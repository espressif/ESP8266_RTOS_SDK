// Copyright 2019-2020 Espressif Systems (Shanghai) PTE LTD
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

#include <stdint.h>
#include "esp8266/eagle_soc.h"

static uint32_t prev_text_size(const uint32_t pc)
{
    uint32_t size;
    extern uint32_t _text_start, _text_end;

    if (pc > (uint32_t)&_text_start && pc < (uint32_t)&_text_end) {
        size = pc - (uint32_t )&_text_start;
    } else if (IS_IRAM(pc)) {
        size = pc - IRAM_BASE;
    } else if (IS_ROM(pc)) {
        size = pc - ROM_BASE;
    } else {
        size = 0;
    }

    return size;
}

int xt_pc_is_valid(const void *pc)
{
    return prev_text_size((uint32_t)pc) ? 1 : 0;
}

int xt_retaddr_callee(const void *i_pc, const void *i_sp, const void *i_lr, void **o_pc, void **o_sp)
{
    uint32_t lr = (uint32_t)i_lr;
    uint32_t pc = (uint32_t)i_pc;
    uint32_t sp = (uint32_t)i_sp;

    uint32_t off = 0;
    const uint32_t text_size = prev_text_size(pc);

    for (; off < text_size; off++) {
        if ((*(uint8_t *)(pc - off) == 0x12) &&
            (*(uint8_t *)(pc - off + 1) == 0xc1)) {
            const int8_t stk_size = *(int8_t *)(pc - off + 2);

            if (stk_size >= 0 || stk_size % 16 != 0) {
                continue;
            }

            sp -= stk_size;
            pc = *(uint32_t *)(sp - 4);

            *o_sp = (void *)sp;
            *o_pc = (void *)pc;

            break;
        } else if ((*(uint8_t *)(pc - off) == 0x92) &&
                    (((*(uint8_t *)(pc - off + 1)) & 0xf0) == 0xa0) &&
                    (*(uint8_t *)(pc - off + 3) == 0x90) &&
                    (*(uint8_t *)(pc - off + 4) == 0x11) &&
                    (*(uint8_t *)(pc - off + 5) == 0xc0)) {
            const uint16_t stk_size = ((*(uint8_t *)(pc - off + 1)) & 0x0f) + (*(uint8_t *)(pc - off + 2));

            if (!stk_size || stk_size >= 2048) {
                continue;
            }

            sp += stk_size;
            pc = *(uint32_t *)(sp - 4);

            *o_sp = (void *)sp;
            *o_pc = (void *)pc;

            break;
        } else if ((*(uint8_t *)(pc - off) == 0x0d) &&
                    (*(uint8_t *)(pc - off + 1) == 0xf0)) {
            pc = lr;

            *o_sp = (void *)sp;
            *o_pc = (void *)pc;
            
            break;
        }
    }
    
    return off < text_size ? (xt_pc_is_valid(*o_pc) ? 1 : 0) : 0;
}

void *xt_return_address(int lvl)
{
    void *i_sp;
    void *i_lr = NULL;
    void *i_pc = (void *)((uint32_t)xt_return_address + 32);

    void *o_pc = NULL;
    void *o_sp;

    __asm__ __volatile__(
            "mov    %0,  a1\n"
            : "=a"(i_sp)
            : 
            : "memory"
    );

    lvl += 2;
    while(lvl-- && xt_retaddr_callee(i_pc, i_sp, i_lr, &o_pc, &o_sp)) {
        i_pc = o_pc;
        i_sp = o_sp;
    }

    return xt_pc_is_valid(o_pc) ? o_pc : NULL;
}
