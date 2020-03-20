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

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>

#include "sdkconfig.h"

#include "nvs_flash.h"
#include "tcpip_adapter.h"

#include "esp_log.h"
#include "esp_image_format.h"
#include "esp_phy_init.h"
#include "esp_heap_caps_init.h"
#include "esp_task_wdt.h"
#include "internal/esp_wifi_internal.h"
#include "internal/esp_system_internal.h"
#include "esp8266/eagle_soc.h"

#include "FreeRTOS.h"
#include "task.h"

#ifndef CONFIG_NEWLIB_LIBRARY_CUSTOMER
#include "esp_newlib.h"
#endif

extern void chip_boot(void);
extern int rtc_init(void);
extern int mac_init(void);
extern int base_gpio_init(void);
extern int watchdog_init(void);
extern int wifi_timer_init(void);
extern int wifi_nvs_init(void);
extern esp_err_t esp_pthread_init(void);
extern void phy_get_bb_evm(void);
extern void uart_div_modify(uint8_t uart_no, uint16_t DivLatchValue);

static inline int should_load(uint32_t load_addr)
{
    if (IS_USR_RTC(load_addr)) {
        if (esp_reset_reason_early() == ESP_RST_DEEPSLEEP)
            return 0;
    }

    if (IS_FLASH(load_addr))
        return 0;

    return 1;
}

static void user_init_entry(void *param)
{
    void (**func)(void);

    extern void (*__init_array_start)(void);
    extern void (*__init_array_end)(void);

    extern void app_main(void);

    extern void phy_close_rf(void);

    extern void esp_sleep_unlock();

    /* initialize C++ construture function */
    for (func = &__init_array_start; func < &__init_array_end; func++)
        func[0]();

    phy_get_bb_evm();

    /*enable tsf0 interrupt for pwm*/
    REG_WRITE(PERIPHS_DPORT_BASEADDR, (REG_READ(PERIPHS_DPORT_BASEADDR) & ~0x1F) | 0x1);
    REG_WRITE(INT_ENA_WDEV, REG_READ(INT_ENA_WDEV) | WDEV_TSF0_REACH_INT);

    assert(nvs_flash_init() == 0);
    assert(rtc_init() == 0);
    assert(mac_init() == 0);
    assert(base_gpio_init() == 0);
    esp_phy_load_cal_and_init(0);
    phy_close_rf();
    esp_sleep_unlock();

    esp_wifi_set_rx_pbuf_mem_type(WIFI_RX_PBUF_DRAM);

#if CONFIG_RESET_REASON
    esp_reset_reason_init();
#endif

#ifdef CONFIG_TASK_WDT
    esp_task_wdt_init();
#endif

#ifdef CONFIG_ENABLE_PTHREAD
    assert(esp_pthread_init() == 0);
#endif

#ifdef CONFIG_ESP8266_DEFAULT_CPU_FREQ_160
    esp_set_cpu_freq(ESP_CPU_FREQ_160M);
#endif

    app_main();

    vTaskDelete(NULL);
}

void call_start_cpu(size_t start_addr)
{
    int i;
    int *p;

    extern int _bss_start, _bss_end;
    extern int _iram_bss_start, _iram_bss_end;

    esp_image_header_t *head = (esp_image_header_t *)(FLASH_BASE + (start_addr & (FLASH_SIZE - 1)));
    esp_image_segment_header_t *segment = (esp_image_segment_header_t *)((uintptr_t)head + sizeof(esp_image_header_t));

    /* The data in flash cannot be accessed by byte in this stage, so just access by word and get the segment count. */
    uint8_t segment_count = ((*(volatile uint32_t *)head) & 0xFF00) >> 8;

    for (i = 0; i < segment_count - 1; i++) {
        segment = (esp_image_segment_header_t *)((uintptr_t)segment + sizeof(esp_image_segment_header_t) + segment->data_len);

        if (!should_load(segment->load_addr))
            continue;

        uint32_t *dest = (uint32_t *)segment->load_addr;
        uint32_t *src = (uint32_t *)((uintptr_t)segment + sizeof(esp_image_segment_header_t));
        uint32_t size = segment->data_len / sizeof(uint32_t);

        while (size--)
            *dest++ = *src++;
    }

    /* 
     * When finish copying IRAM program, the exception vect must be initialized.
     * And then user can load/store data which is not aligned by 4-byte.
     */
    __asm__ __volatile__(
        "movi       a0, 0x40100000\n"
        "wsr        a0, vecbase\n"
        : : :"memory");

#ifndef CONFIG_BOOTLOADER_INIT_SPI_FLASH
    chip_boot();
#endif

    /* clear bss data */
    for (p = &_bss_start; p < &_bss_end; p++)
        *p = 0;

    /* clear iram_bss data */
    for (p = &_iram_bss_start; p < &_iram_bss_end; p++)
        *p = 0;

    __asm__ __volatile__(
        "rsil       a2, 2\n"
        "movi       a1, _chip_interrupt_tmp\n"
        : : :"memory");

    heap_caps_init();

#ifdef CONFIG_INIT_OS_BEFORE_START
    extern int __esp_os_init(void);
    assert(__esp_os_init() == 0);
#endif

#ifndef CONFIG_NEWLIB_LIBRARY_CUSTOMER
    esp_newlib_init();
#endif

    assert(xTaskCreate(user_init_entry, "uiT", CONFIG_MAIN_TASK_STACK_SIZE, NULL, configMAX_PRIORITIES, NULL) == pdPASS);

    vTaskStartScheduler();
}
