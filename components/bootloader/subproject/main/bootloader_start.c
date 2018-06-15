// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
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

#include "sdkconfig.h"

#include <stddef.h>
#include <stdint.h>

#include "load_flash_bin.h"

#include "esp8266/eagle_soc.h"

//#define BOOT_DEBUG

#ifdef BOOT_DEBUG
#define BDEBUG  ets_printf
#else
#define BDEBUG(...)
#endif

#define BOOT_VERSION 0x06

typedef enum {
    SPI_FLASH_QIO_MODE = 0,
    SPI_FLASH_QOUT_MODE,
    SPI_FLASH_DIO_MODE,
    SPI_FLASH_DOUT_MODE,
    SPI_FLASH_FASTRD_MODE,
    SPI_FLASH_SLOWRD_MODE
} SpiFlashRdMode;

enum {
    SPI_MODE_QIO,
    SPI_MODE_QOUT,
    SPI_MODE_DIO,
    SPI_MODE_DOUT
};

enum {
    SPI_SPEED_40M,
    SPI_SPEED_26M,
    SPI_SPEED_20M,
    SPI_SPEED_80M = 0xF
};

enum {
    SPI_SIZE_4M_256_256 = 0,
    SPI_SIZE_2M,
    SPI_SIZE_8M_512_512,
    SPI_SIZE_16M_512_512,
    SPI_SIZE_32M_512_512,
    SPI_SIZE_16M_1024_1024,
    SPI_SIZE_32M_1024_1024
};

enum {
    USER_BIN1,
    USER_BIN2
};

#define SPI_SEC_SIZE    0x1000

struct save_hdr {
    char flag;
    char pad[3];
};

struct boot_hdr {
    char use_bin: 2;    // low bit
    char boot_status: 1;
    char reverse: 5;
    char version: 5;    // low bit
    char test_pass_flag: 1;
    char test_start_flag: 1;
    char enhance_boot_flag: 1;
    char test_bin_addr[3];
    char user_bin_addr[3];
};

struct boot_hdr_1 {
    char use_bin: 4;
    char flag: 4;
    char pad[7];
};

struct boot_hdr_2 {
    char use_bin: 4;
    char flag: 4;
    char version;
    char pad[6];
};

struct flash_hdr {
    char magic;
    char blocks;
    char spi_mode;  //flag of flash read mode in unpackage and usage in future
    char spi_speed: 4;  // low bit
    char spi_size_map: 4;
    unsigned int entry_addr;
} ;

struct block_hdr {
    unsigned int load_addr;
    unsigned int data_len;
} ;

#define WIFI_PARAM_RF         0
#define WIFI_PARAM_SAVE_0     1
#define WIFI_PARAM_SAVE_1     2
#define WIFI_PARAM_FLAG       3

typedef enum {
    SPI_FLASH_RESULT_OK = 0,
    SPI_FLASH_RESULT_ERR = 1,
    SPI_FLASH_RESULT_TIMEOUT = 2
} SpiFlashOpResult;

extern SpiFlashOpResult SPIRead(uint32_t addr, void *dst, uint32_t size);
extern int ets_printf(const char* fmt, ...);
extern void *ets_memcpy(void *restrict to, const void *restrict from, size_t size);

signed int get_flash_bin_addr(unsigned int bin_addr)
{
    char buf[16];
    struct flash_hdr* fhdr;
    struct block_hdr* bhdr;

    SPIRead(bin_addr, (unsigned int*)buf, 16);

    fhdr = (struct flash_hdr*)buf;

    if (fhdr->magic == 0xE9) {
        return 0;
    } else if (fhdr->magic == 0xEA && fhdr->blocks == 0x04) {
        bhdr = (struct block_hdr*)(buf + sizeof(struct flash_hdr));
        return bhdr->data_len;
    } else {
        ets_printf("error magic!\n");
        return -1;
    }
}

// 0---OK, 1---FAIL
char jump_to_run_addr(unsigned int bin_addr)
{
    unsigned int flash_addr;
    char ret = 1;
    char(*jump_to_load_flash_code)(unsigned int addr);

    ets_printf(" @ %x\n\n", bin_addr);

    jump_to_load_flash_code = (void*)(0x4010FC08);
    flash_addr = get_flash_bin_addr(bin_addr);

    if (flash_addr != -1) {
        if (flash_addr == 0) {
            ret = jump_to_load_flash_code(bin_addr);
        } else {
            ret = jump_to_load_flash_code(bin_addr + 16 + flash_addr);
        }
    }

    return ret;
}

unsigned int gen_bin_addr(unsigned char* buf)
{
    unsigned int ret;

    ret = buf[2] << 16 | buf[1] << 8 | buf[0];

    return ret;
}

void call_start_cpu(void)
{
    struct save_hdr shdr;
    struct boot_hdr bhdr;
    struct flash_hdr fhdr;

    unsigned int sys_start;

    ets_printf("\n2nd boot version : 2.0\n");

    SPIRead(0, (unsigned int*)&fhdr, sizeof(struct flash_hdr));

    BDEBUG("[D]: magic %02x\n", fhdr.magic);
    BDEBUG("[D]: blocks %02x\n", fhdr.blocks);
    BDEBUG("[D]: spi_mode %02x\n", fhdr.spi_mode);
    BDEBUG("[D]: spi_speed %02x\n", fhdr.spi_speed);
    BDEBUG("[D]: spi_size_map %02x\n", fhdr.spi_size_map);

    ets_printf("  SPI Speed      : ");

    switch (fhdr.spi_speed) {
        case SPI_SPEED_40M:
            ets_printf("40MHz\n");
            break;

        case SPI_SPEED_26M:
            ets_printf("26.7MHz\n");
            break;

        case SPI_SPEED_20M:
            ets_printf("20MHz\n");
            break;

        case SPI_SPEED_80M:
            ets_printf("80MHz\n");
            break;
    }

    ets_printf("  SPI Mode       : ");

    switch (fhdr.spi_mode) {
        case SPI_MODE_QIO:
            ets_printf("QIO\n");
            break;

        case SPI_MODE_QOUT:
            ets_printf("QOUT\n");
            break;

        case SPI_MODE_DIO:
            ets_printf("DIO\n");
            break;

        case SPI_MODE_DOUT:
            ets_printf("DOUT\n");
            break;

        default:
            fhdr.spi_mode = SPI_MODE_QIO;
            ets_printf("QIO\n");
            break;
    }

    ets_printf("  SPI Flash Size & Map: ");

    switch (fhdr.spi_size_map) {
        case SPI_SIZE_4M_256_256:
            sys_start = 124;
            ets_printf("4Mbit(256KB+256KB)\n");
            break;

        case SPI_SIZE_2M:
            sys_start =  60;
            ets_printf("2Mbit\n");
            break;

        case SPI_SIZE_8M_512_512:
            sys_start = 252;
            ets_printf("8Mbit(512KB+512KB)\n");
            break;

        case SPI_SIZE_16M_512_512:
            sys_start = 508;
            ets_printf("16Mbit(512KB+512KB)\n");
            break;

        case SPI_SIZE_32M_512_512:
            sys_start = 1020;
            ets_printf("32Mbit(512KB+512KB)\n");
            break;

        case SPI_SIZE_16M_1024_1024:
            sys_start = 508;
            ets_printf("16Mbit(1024KB+1024KB)\n");
            break;

        case SPI_SIZE_32M_1024_1024:
            sys_start = 1020;
            ets_printf("32Mbit(1024KB+1024KB)\n");
            break;

        default:
            sys_start = 124;
            ets_printf("4Mbit\n");
            break;
    }

    SPIRead((sys_start + WIFI_PARAM_FLAG) * SPI_SEC_SIZE,
            (unsigned int*)&shdr, sizeof(struct save_hdr));

    SPIRead((sys_start + ((shdr.flag == 0) ? WIFI_PARAM_SAVE_0 : WIFI_PARAM_SAVE_1)) * SPI_SEC_SIZE,
            (unsigned int*)&bhdr, sizeof(struct boot_hdr));

    BDEBUG("[D]: use_bin %02x\n", bhdr.use_bin);
    BDEBUG("[D]: boot_status %02x\n", bhdr.boot_status);
    BDEBUG("[D]: reverse %02x\n", bhdr.reverse);
    BDEBUG("[D]: version %02x\n", bhdr.version);
    BDEBUG("[D]: test_pass_flag %02x\n", bhdr.test_pass_flag);
    BDEBUG("[D]: test_start_flag %02x\n", bhdr.test_start_flag);
    BDEBUG("[D]: enhance_boot_flag %02x\n", bhdr.enhance_boot_flag);
    BDEBUG("[D]: test_bin_addr %02x %02x %02x\n", bhdr.test_bin_addr[0], bhdr.test_bin_addr[1], bhdr.test_bin_addr[2]);
    BDEBUG("[D]: user_bin_addr %02x %02x %02x\n", bhdr.user_bin_addr[0], bhdr.user_bin_addr[1], bhdr.user_bin_addr[2]);

    ets_memcpy((void *)0x4010FC00, load_bin, load_bin_len);

    ets_printf("jump to run");

    jump_to_run_addr(CONFIG_PARTITION_TABLE_CUSTOM_APP_BIN_OFFSET);
}
