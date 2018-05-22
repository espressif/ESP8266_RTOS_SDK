/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdint.h>

#include "c_types.h"
#include "spi_flash.h"
#include "lwip/err.h"
#include "lwip/ip_addr.h"

#define BUFSIZE        512
#define CRC_BLOCK_SIZE 512

uint32_t start_sec;
static uint32_t *crc_table;

static int init_crc_table(void);

static uint32_t crc32(uint32_t crc, unsigned char *buffer, uint32_t size);

static int init_crc_table(void)
{
    uint32_t c;
    uint32_t i, j;

    crc_table = (uint32_t *)calloc(1, 256 * 4);

    if (crc_table == NULL) {
        printf("malloc crc table failed\n");
        return -1;
    }

    for (i = 0; i < 256; i++) {
        c = (uint32_t)i;

        for (j = 0; j < 8; j++) {
            if (c & 1) {
                c = 0xedb88320L ^ (c >> 1);
            } else {
                c = c >> 1;
            }
        }

        crc_table[i] = c;
    }

    return 0;
}

static uint32_t crc32(uint32_t crc, unsigned char *buffer, uint32_t size)
{
    uint32_t i;

    for (i = 0; i < size; i++) {
        crc = crc_table[(crc ^ buffer[i]) & 0xff] ^ (crc >> 8);
    }

    return crc ;
}

static int calc_img_crc(uint32_t sumlength, uint32_t *img_crc)
{
    int fd;
    int ret;
    int i = 0;
    uint8 error = 0;
    unsigned char *buf = (char *)calloc(1, BUFSIZE);

    if (buf == NULL) {
        printf("malloc crc buf failed\n");
        free(crc_table);
        return -1;
    }

    uint32_t crc = 0xffffffff;

    uint16_t sec_block = sumlength / CRC_BLOCK_SIZE ;
    uint32_t sec_last = sumlength % CRC_BLOCK_SIZE;

    for (i = 0; i < sec_block; i++) {
        if (0 != (error = spi_flash_read(start_sec * SPI_FLASH_SEC_SIZE + i * CRC_BLOCK_SIZE , (uint32 *)buf, BUFSIZE))) {
            free(crc_table);
            free(buf);
            printf("spi_flash_read error %d\n", error);
            return -1;
        }

        crc = crc32(crc, buf, BUFSIZE);
    }

    if (sec_last > 0) {
        if (0 != (error = spi_flash_read(start_sec * SPI_FLASH_SEC_SIZE + i * CRC_BLOCK_SIZE, (uint32 *)buf, sec_last))) {
            free(crc_table);
            free(buf);
            printf("spi_flash_read error %d\n", error);
            return -1;
        }

        crc = crc32(crc, buf, sec_last);
    }

    *img_crc = crc;
    free(crc_table);
    free(buf);
    return 0;
}

bool upgrade_crc_check(uint16 fw_bin_sec , uint32_t sumlength)
{
    int ret;
    uint32_t img_crc;
    uint32_t flash_crc = 0xFF;
    start_sec = fw_bin_sec;

    printf("fw_bin_sec %d sumlength %d\n", fw_bin_sec, sumlength);

    if (0 != init_crc_table()) {
        return false;
    }

    ret = calc_img_crc(sumlength - 4, &img_crc);

    if (ret < 0) {
        return false;
    }

    img_crc = abs(img_crc);
    printf("img_crc = %u\n", img_crc);
    spi_flash_read(start_sec * SPI_FLASH_SEC_SIZE + sumlength - 4, &flash_crc, 4);
    printf("flash_crc = %u\n", flash_crc);

    if (img_crc == flash_crc) {
        return true;
    } else {
        return false;
    }
}
