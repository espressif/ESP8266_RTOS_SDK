
#include "sdkconfig.h"
#include <stdint.h>
#include <stdarg.h>
#include "esp_image_format.h"

#define FLASH_MAP_ADDR 0x40200000

void call_user_start(void)
{
    int i;
    extern void user_start(void);

    esp_image_header_t *head = (esp_image_header_t *)(FLASH_MAP_ADDR + CONFIG_PARTITION_TABLE_CUSTOM_APP_BIN_OFFSET);
    esp_image_segment_header_t *segment = (esp_image_segment_header_t *)((uintptr_t)head + sizeof(esp_image_header_t));

    for (i = 0; i < 3; i++) {
        segment = (esp_image_segment_header_t *)((uintptr_t)segment + sizeof(esp_image_segment_header_t) + segment->data_len);

        uint32_t *dest = (uint32_t *)segment->load_addr;
        uint32_t *src = (uint32_t *)((uintptr_t)segment + sizeof(esp_image_segment_header_t));
        uint32_t size = segment->data_len / sizeof(uint32_t);

        while (size--)
            *dest++ = *src++;
    }

    __asm__ __volatile__(
        "movi       a2, 0x40100000\n"
        "wsr        a2, vecbase\n");

    user_start();
}
    
    