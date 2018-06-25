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

#include "esp_attr.h"
#include "esp_libc.h"

#include "esp8266/eagle_soc.h"
#include "esp8266/ets_sys.h"
#include "esp_err.h"

/*
 * @brief output xtensa register value map when crash
 * 
 * @param frame xtensa register value map pointer
 * 
 * @return none
 */
void IRAM_ATTR panicHandler(void *frame)
{
    int *regs = (int *)frame;
    int x, y;
    const char *sdesc[] = {
        "PC",   "PS",   "A0",   "A1",
        "A2",   "A3",   "A4",   "A5",
        "A6",   "A7",   "A8",   "A9",
        "A10",  "A11",  "A12",  "A13",
        "A14",  "A15",  "SAR",  "EXCCAUSE"
    };

    /* NMI can interrupt exception. */
    ETS_INTR_LOCK();

    for (x = 0; x < 20; x += 4) {
        for (y = 0; y < 4; y++) {
            ets_printf("%8s: 0x%08x  ", sdesc[x + y], regs[x + y + 1]);
        }
        ets_printf("\n");
    }

    /*
     * Todo: add more option to select here to 'Kconfig':
     *     1. blocking
     *     2. restart
     *     3. GBD break
     */
    while (1);
}

void _esp_error_check_failed(esp_err_t rc, const char *file, int line, const char *function, const char *expression)
{
    printf("ESP_ERROR_CHECK failed: esp_err_t 0x%x at %p\n", rc, __builtin_return_address(0));
    printf("file: \"%s\" line %d\nfunc: %s\nexpression: %s\n", file, line, function, expression);
    abort();
}