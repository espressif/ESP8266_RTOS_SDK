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

#include "stdlib.h"

#include "esp_attr.h"
#include "esp_libc.h"
#include "esp_system.h"

#include "esp8266/rom_functions.h"
#include "esp8266/backtrace.h"
#include "rom/ets_sys.h"
#include "esp_err.h"

#include "FreeRTOS.h"

#define PANIC(_fmt, ...)    ets_printf(_fmt, ##__VA_ARGS__)

#ifndef CONFIG_ESP_PANIC_SILENT_REBOOT

typedef struct panic_frame {
    uint32_t    exit;

    uint32_t    pc;
    uint32_t    ps;

    uint32_t    a0;
    uint32_t    a1;
    uint32_t    a2;
    uint32_t    a3;

    uint32_t    a4;
    uint32_t    a5;
    uint32_t    a6;
    uint32_t    a7;

    uint32_t    a8;
    uint32_t    a9;
    uint32_t    a10;
    uint32_t    a11;

    uint32_t    a12;
    uint32_t    a13;
    uint32_t    a14;
    uint32_t    a15;

    uint32_t    sar;
    uint32_t    exccause;
} panic_frame_t;

static inline void panic_frame(panic_frame_t *frame)
{
    static const char *sdesc[] = {
        "PC",   "PS",   "A0",   "A1",
        "A2",   "A3",   "A4",   "A5",
        "A6",   "A7",   "A8",   "A9",
        "A10",  "A11",  "A12",  "A13",
        "A14",  "A15",  "SAR",  "EXCCAUSE"
    };
    static const char *edesc[] = {
        "IllegalInstruction", "Syscall", "InstructionFetchError", "LoadStoreError",
        "Level1Interrupt", "Alloca", "IntegerDivideByZero", "PCValue",
        "Privileged", "LoadStoreAlignment", "res", "res",
        "InstrPDAddrError", "LoadStorePIFDataError", "InstrPIFAddrError", "LoadStorePIFAddrError",
        "InstTLBMiss", "InstTLBMultiHit", "InstFetchPrivilege", "res",
        "InstrFetchProhibited", "res", "res", "res",
        "LoadStoreTLBMiss", "LoadStoreTLBMultihit", "LoadStorePrivilege", "res",
        "LoadProhibited", "StoreProhibited",
    };

    void *i_lr = (void *)frame->a0;
    void *i_pc = (void *)frame->pc;
    void *i_sp = (void *)frame->a1;

    void *o_pc;
    void *o_sp;

    const char *reason = frame->exccause < 30 ? edesc[frame->exccause] : "unknown";
    const uint32_t *regs = (const uint32_t *)frame;

    PANIC("Guru Meditation Error: Core  0 panic'ed (%s). Exception was unhandled.\r\n", reason);
    PANIC("Core 0 register dump:\n");

    for (int i = 0; i < 20; i += 4) {
        for (int j = 0; j < 4; j++) {
            PANIC("%-8s: 0x%08x  ", sdesc[i + j], regs[i + j + 1]);
        }
        PANIC("\r\n");
    }

    PANIC("\r\nBacktrace: %p:%p ", i_pc, i_sp);
    while(xt_retaddr_callee(i_pc, i_sp, i_lr, &o_pc, &o_sp)) {
        PANIC("%p:%p ", o_pc, o_sp);
        i_pc = o_pc;
        i_sp = o_sp;
    }
    PANIC("\r\n");
}
#endif /* !CONFIG_ESP_PANIC_SILENT_REBOOT */

void panicHandler(void *frame, int wdt)
{
#ifndef CONFIG_ESP_PANIC_SILENT_REBOOT
    extern int _chip_nmi_cnt;

    _chip_nmi_cnt = 0;

    /* NMI can interrupt exception. */
    vPortEnterCritical();
    do {
        REG_WRITE(INT_ENA_WDEV, 0);
    } while (REG_READ(INT_ENA_WDEV) != 0);

    if (wdt) {
        PANIC("Task watchdog got triggered.\r\n\r\n");
    }

    panic_frame(frame);

#ifdef CONFIG_ESP_PANIC_PRINT_HALT
    while (1);
#else
    esp_restart();
#endif

#else  /* CONFIG_ESP_PANIC_SILENT_REBOOT */
    esp_restart();
#endif /* !CONFIG_ESP_PANIC_SILENT_REBOOT */
}

static void esp_error_check_failed_print(const char *msg, esp_err_t rc, const char *file, int line, const char *function, const char *expression)
{
    PANIC("%s failed: esp_err_t 0x%x", msg, rc);
#ifdef CONFIG_ESP_ERR_TO_NAME_LOOKUP
    PANIC(" (%s)", esp_err_to_name(rc));
#endif //CONFIG_ESP_ERR_TO_NAME_LOOKUP
    PANIC(" at 0x%08x\n", (intptr_t)__builtin_return_address(0) - 3);

    // ESP8266 put main FreeRTOS code at flash
    //if (spi_flash_cache_enabled()) { // strings may be in flash cache
    PANIC("file: \"%s\" line %d\nfunc: %s\nexpression: %s\n", file, line, function, expression);
    //}
}

void _esp_error_check_failed_without_abort(esp_err_t rc, const char *file, int line, const char *function, const char *expression)
{
    esp_error_check_failed_print("ESP_ERROR_CHECK_WITHOUT_ABORT", rc, file, line, function, expression);
}

void _esp_error_check_failed(esp_err_t rc, const char *file, int line, const char *function, const char *expression)
{
    esp_error_check_failed_print("ESP_ERROR_CHECK", rc, file, line, function, expression);
    abort();
}
