// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "sdkconfig.h"

#ifdef CONFIG_IDF_TARGET_ESP32

#include <stddef.h>

#include <bootloader_flash.h>
#include <esp_log.h>
#include <esp_spi_flash.h> /* including in bootloader for error values */
#include <esp_flash_encrypt.h>

#ifndef BOOTLOADER_BUILD
/* Normal app version maps to esp_spi_flash.h operations...
 */
static const char *TAG = "bootloader_mmap";

static spi_flash_mmap_handle_t map;

const void *bootloader_mmap(uint32_t src_addr, uint32_t size)
{
    if (map) {
        ESP_LOGE(TAG, "tried to bootloader_mmap twice");
        return NULL; /* existing mapping in use... */
    }
    const void *result = NULL;
    uint32_t src_page = src_addr & ~(SPI_FLASH_MMU_PAGE_SIZE-1);
    size += (src_addr - src_page);
    esp_err_t err = spi_flash_mmap(src_page, size, SPI_FLASH_MMAP_DATA, &result, &map);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "spi_flash_mmap failed: 0x%x", err);
        return NULL;
    }
    return (void *)((intptr_t)result + (src_addr - src_page));
}

void bootloader_munmap(const void *mapping)
{
    if(mapping && map) {
        spi_flash_munmap(map);
    }
    map = 0;
}

esp_err_t bootloader_flash_read(size_t src, void *dest, size_t size, bool allow_decrypt)
{
    if (allow_decrypt && esp_flash_encryption_enabled()) {
        return spi_flash_read_encrypted(src, dest, size);
    } else {
        return spi_flash_read(src, dest, size);
    }
}

esp_err_t bootloader_flash_write(size_t dest_addr, void *src, size_t size, bool write_encrypted)
{
    if (write_encrypted) {
        return spi_flash_write_encrypted(dest_addr, src, size);
    } else {
        return spi_flash_write(dest_addr, src, size);
    }
}

esp_err_t bootloader_flash_erase_sector(size_t sector)
{
    return spi_flash_erase_sector(sector);
}

#else
/* Bootloader version, uses ROM functions only */
#include <soc/dport_reg.h>
#include <rom/spi_flash.h>
#include <rom/cache.h>

static const char *TAG = "bootloader_flash";

/* Use first 50 blocks in MMU for bootloader_mmap,
   50th block for bootloader_flash_read
*/
#define MMU_BLOCK0_VADDR  0x3f400000
#define MMU_BLOCK50_VADDR 0x3f720000
#define MMU_FLASH_MASK    0xffff0000
#define MMU_BLOCK_SIZE    0x00010000

static bool mapped;

// Current bootloader mapping (ab)used for bootloader_read()
static uint32_t current_read_mapping = UINT32_MAX;

const void *bootloader_mmap(uint32_t src_addr, uint32_t size)
{
    if (mapped) {
        ESP_LOGE(TAG, "tried to bootloader_mmap twice");
        return NULL; /* can't map twice */
    }
    if (size > 0x320000) {
        /* Allow mapping up to 50 of the 51 available MMU blocks (last one used for reads) */
        ESP_LOGE(TAG, "bootloader_mmap excess size %x", size);
        return NULL;
    }

    uint32_t src_addr_aligned = src_addr & MMU_FLASH_MASK;
    uint32_t count = (size + (src_addr - src_addr_aligned) + 0xffff) / MMU_BLOCK_SIZE;
    Cache_Read_Disable(0);
    Cache_Flush(0);
    ESP_LOGD(TAG, "mmu set paddr=%08x count=%d", src_addr_aligned, count );
    int e = cache_flash_mmu_set(0, 0, MMU_BLOCK0_VADDR, src_addr_aligned, 64, count);
    if (e != 0) {
        ESP_LOGE(TAG, "cache_flash_mmu_set failed: %d\n", e);
        Cache_Read_Enable(0);
        return NULL;
    }
    Cache_Read_Enable(0);

    mapped = true;

    return (void *)(MMU_BLOCK0_VADDR + (src_addr - src_addr_aligned));
}

void bootloader_munmap(const void *mapping)
{
    if (mapped)  {
        /* Full MMU reset */
        Cache_Read_Disable(0);
        Cache_Flush(0);
        mmu_init(0);
        mapped = false;
        current_read_mapping = UINT32_MAX;
    }
}

static esp_err_t spi_to_esp_err(esp_rom_spiflash_result_t r)
{
    switch(r) {
    case ESP_ROM_SPIFLASH_RESULT_OK:
        return ESP_OK;
    case ESP_ROM_SPIFLASH_RESULT_ERR:
        return ESP_ERR_FLASH_OP_FAIL;
    case ESP_ROM_SPIFLASH_RESULT_TIMEOUT:
        return ESP_ERR_FLASH_OP_TIMEOUT;
    default:
        return ESP_FAIL;
    }
}

static esp_err_t bootloader_flash_read_no_decrypt(size_t src_addr, void *dest, size_t size)
{
    Cache_Read_Disable(0);
    Cache_Flush(0);
    esp_rom_spiflash_result_t r = esp_rom_spiflash_read(src_addr, dest, size);
    Cache_Read_Enable(0);

    return spi_to_esp_err(r);
}

static esp_err_t bootloader_flash_read_allow_decrypt(size_t src_addr, void *dest, size_t size)
{
    uint32_t *dest_words = (uint32_t *)dest;

    /* Use the 51st MMU mapping to read from flash in 64KB blocks.
       (MMU will transparently decrypt if encryption is enabled.)
    */
    for (int word = 0; word < size / 4; word++) {
        uint32_t word_src = src_addr + word * 4;  /* Read this offset from flash */
        uint32_t map_at = word_src & MMU_FLASH_MASK; /* Map this 64KB block from flash */
        uint32_t *map_ptr;
        if (map_at != current_read_mapping) {
            /* Move the 64KB mmu mapping window to fit map_at */
            Cache_Read_Disable(0);
            Cache_Flush(0);
            ESP_LOGD(TAG, "mmu set block paddr=0x%08x (was 0x%08x)", map_at, current_read_mapping);
            int e = cache_flash_mmu_set(0, 0, MMU_BLOCK50_VADDR, map_at, 64, 1);
            if (e != 0) {
                ESP_LOGE(TAG, "cache_flash_mmu_set failed: %d\n", e);
                Cache_Read_Enable(0);
                return ESP_FAIL;
            }
            current_read_mapping = map_at;
            Cache_Read_Enable(0);
        }
        map_ptr = (uint32_t *)(MMU_BLOCK50_VADDR + (word_src - map_at));
        dest_words[word] = *map_ptr;
    }
    return ESP_OK;
}

esp_err_t bootloader_flash_read(size_t src_addr, void *dest, size_t size, bool allow_decrypt)
{
    if (src_addr & 3) {
        ESP_LOGE(TAG, "bootloader_flash_read src_addr 0x%x not 4-byte aligned", src_addr);
        return ESP_FAIL;
    }
    if (size & 3) {
        ESP_LOGE(TAG, "bootloader_flash_read size 0x%x not 4-byte aligned", size);
        return ESP_FAIL;
    }
    if ((intptr_t)dest & 3) {
        ESP_LOGE(TAG, "bootloader_flash_read dest 0x%x not 4-byte aligned", (intptr_t)dest);
        return ESP_FAIL;
    }

    if (allow_decrypt) {
        return bootloader_flash_read_allow_decrypt(src_addr, dest, size);
    } else {
        return bootloader_flash_read_no_decrypt(src_addr, dest, size);
    }
}

esp_err_t bootloader_flash_write(size_t dest_addr, void *src, size_t size, bool write_encrypted)
{
    esp_err_t err;
    size_t alignment = write_encrypted ? 32 : 4;
    if ((dest_addr % alignment) != 0) {
        ESP_LOGE(TAG, "bootloader_flash_write dest_addr 0x%x not %d-byte aligned", dest_addr, alignment);
        return ESP_FAIL;
    }
    if ((size % alignment) != 0) {
        ESP_LOGE(TAG, "bootloader_flash_write size 0x%x not %d-byte aligned", size, alignment);
        return ESP_FAIL;
    }
    if (((intptr_t)src % 4) != 0) {
        ESP_LOGE(TAG, "bootloader_flash_write src 0x%x not 4 byte aligned", (intptr_t)src);
        return ESP_FAIL;
    }

    err = spi_to_esp_err(esp_rom_spiflash_unlock());
    if (err != ESP_OK) {
        return err;
    }

    if (write_encrypted) {
        return spi_to_esp_err(esp_rom_spiflash_write_encrypted(dest_addr, src, size));
    } else {
        return spi_to_esp_err(esp_rom_spiflash_write(dest_addr, src, size));
    }
}

esp_err_t bootloader_flash_erase_sector(size_t sector)
{
    return spi_to_esp_err(esp_rom_spiflash_erase_sector(sector));
}

#endif

#endif

#ifdef CONFIG_IDF_TARGET_ESP8266

#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp8266/rom_functions.h"

#ifndef BOOTLOADER_BUILD
#include "esp_spi_flash.h"
#else
#include "bootloader_flash.h"
#include "priv/esp_spi_flash_raw.h"
#endif

#ifdef CONFIG_SOC_FULL_ICACHE
#define SOC_CACHE_SIZE 1 // 32KB
#else
#define SOC_CACHE_SIZE 0 // 16KB
#endif

#define XMC_SUPPORT CONFIG_BOOTLOADER_FLASH_XMC_SUPPORT

#define BYTESHIFT(VAR, IDX)    (((VAR) >> ((IDX) * 8)) & 0xFF)

static const char *TAG = "bootloader_flash";

static bool mapped;

const void *bootloader_mmap(uint32_t src_addr, uint32_t size)
{
    if (mapped) {
        ESP_LOGE(TAG, "tried to bootloader_mmap twice");
        return NULL; /* can't map twice */
    }

    /* 0: 0x000000 - 0x1fffff */
    /* 1: 0x200000 - 0x3fffff */
    /* 2: 0x400000 - 0x5fffff */
    /* 3: 0x600000 - 0x7fffff */

    uint32_t region;
    uint32_t sub_region;
    uint32_t mapped_src;

    if (src_addr < 0x200000) {
        region = 0;
    } else if (src_addr < 0x400000) {
        region = 1;
    } else if (src_addr < 0x600000) {
        region = 2;
    } else if (src_addr < 0x800000) {
        region = 3;
    } else {
        ESP_LOGE(TAG, "flash mapped address %p is invalid", (void *)src_addr);
        while (1);
    }

    /* 0: 0x000000 - 0x0fffff \              */
    /*                         \             */
    /*                           0x40200000  */
    /*                         /             */
    /* 1: 0x100000 - 0x1fffff /              */
    mapped_src =  src_addr & 0x1fffff;
    if (mapped_src < 0x100000) {
        sub_region = 0;
    } else {
        sub_region = 1;
        mapped_src -= 0x100000;
    }

    Cache_Read_Disable();

    Cache_Read_Enable(sub_region, region, SOC_CACHE_SIZE);

    mapped = true;

    return (void *)(0x40200000 + mapped_src);
}

void bootloader_munmap(const void *mapping)
{
    if (mapped)  {
        Cache_Read_Disable();
        mapped = false;
    }
}

static esp_err_t bootloader_flash_read_no_decrypt(size_t src_addr, void *dest, size_t size)
{
#ifdef BOOTLOADER_BUILD
    SPIRead(src_addr, dest, size);
#else
    spi_flash_read(src_addr, dest, size);
#endif

    return ESP_OK;
}

esp_err_t bootloader_flash_read(size_t src_addr, void *dest, size_t size, bool allow_decrypt)
{
    if (src_addr & 3) {
        ESP_LOGE(TAG, "bootloader_flash_read src_addr 0x%x not 4-byte aligned", src_addr);
        return ESP_FAIL;
    }
    if (size & 3) {
        ESP_LOGE(TAG, "bootloader_flash_read size 0x%x not 4-byte aligned", size);
        return ESP_FAIL;
    }
    if ((intptr_t)dest & 3) {
        ESP_LOGE(TAG, "bootloader_flash_read dest 0x%x not 4-byte aligned", (intptr_t)dest);
        return ESP_FAIL;
    }

        return bootloader_flash_read_no_decrypt(src_addr, dest, size);
}

esp_err_t bootloader_flash_write(size_t dest_addr, void *src, size_t size, bool write_encrypted)
{
    size_t alignment = write_encrypted ? 32 : 4;
    if ((dest_addr % alignment) != 0) {
        ESP_LOGE(TAG, "bootloader_flash_write dest_addr 0x%x not %d-byte aligned", dest_addr, alignment);
        return ESP_FAIL;
    }
    if ((size % alignment) != 0) {
        ESP_LOGE(TAG, "bootloader_flash_write size 0x%x not %d-byte aligned", size, alignment);
        return ESP_FAIL;
    }
    if (((intptr_t)src % 4) != 0) {
        ESP_LOGE(TAG, "bootloader_flash_write src 0x%x not 4 byte aligned", (intptr_t)src);
        return ESP_FAIL;
    }


    SPIWrite(dest_addr, src, size);

    return ESP_OK;
}

esp_err_t bootloader_flash_erase_sector(size_t sector)
{
    SPIEraseSector(sector);

    return ESP_OK;
}

#ifdef BOOTLOADER_BUILD
uint32_t bootloader_read_flash_id(void)
{
    uint32_t id = spi_flash_get_id_raw(&g_rom_flashchip);
    id = ((id & 0xff) << 16) | ((id >> 16) & 0xff) | (id & 0xff00);
    return id;
}

#if XMC_SUPPORT
static bool is_xmc_chip_strict(uint32_t rdid)
{
    uint32_t vendor_id = BYTESHIFT(rdid, 2);
    uint32_t mfid = BYTESHIFT(rdid, 1);
    uint32_t cpid = BYTESHIFT(rdid, 0);

    if (vendor_id != XMC_VENDOR_ID) {
        return false;
    }

    bool matched = false;
    if (mfid == 0x40) {
        if (cpid >= 0x13 && cpid <= 0x20) {
            matched = true;
        }
    } else if (mfid == 0x41) {
        if (cpid >= 0x17 && cpid <= 0x20) {
            matched = true;
        }
    } else if (mfid == 0x50) {
        if (cpid >= 0x15 && cpid <= 0x16) {
            matched =  true;
        }
    }
    return matched;
}

bool bootloader_execute_flash_command(uint8_t command, uint32_t mosi_data, uint8_t mosi_len, uint8_t miso_len)
{
    bool ret;
    spi_cmd_t cmd;

    cmd.cmd        = command;
    cmd.cmd_len    = 1;
    cmd.addr       = NULL;
    cmd.addr_len   = 0;
    cmd.dummy_bits = 0;
    cmd.data       = NULL;
    cmd.data_len   = 0;

    ret = spi_user_cmd_raw(&g_rom_flashchip, SPI_TX, &cmd);
    if (!ret) {
        ESP_LOGE(TAG, "failed to write cmd=%02x", command);
    }

    return ret;
}

uint32_t bootloader_flash_read_sfdp(uint32_t sfdp_addr, unsigned int miso_byte_num)
{
    bool ret;
    spi_cmd_t cmd;
    uint32_t data = 0;
    uint32_t addr = sfdp_addr << 8;

    cmd.cmd        = CMD_RDSFDP;
    cmd.cmd_len    = 1;
    cmd.addr       = &addr;
    cmd.addr_len   = 3;
    cmd.dummy_bits = 8;
    cmd.data       = &data;
    cmd.data_len   = miso_byte_num;

    ret = spi_user_cmd_raw(&g_rom_flashchip, SPI_RX, &cmd);
    if (!ret) {
        ESP_LOGE(TAG, "failed to read sfdp");
    }

    return data;
}

esp_err_t bootloader_flash_xmc_startup(void)
{
    extern void ets_rom_delay_us(uint16_t us);

    uint32_t id = bootloader_read_flash_id();

    // If the RDID value is a valid XMC one, may skip the flow
    const bool fast_check = true;
    if (fast_check && is_xmc_chip_strict(id)) {
        ESP_LOGD(TAG, "XMC chip detected by RDID (%08X), skip.", id);
        return ESP_OK;
    }

    // Check the Manufacturer ID in SFDP registers (JEDEC standard). If not XMC chip, no need to run the flow
    const int sfdp_mfid_addr = 0x10;
    uint8_t mf_id = (bootloader_flash_read_sfdp(sfdp_mfid_addr, 1) & 0xff);
    if (mf_id != XMC_VENDOR_ID) {
        ESP_LOGD(TAG, "non-XMC chip detected by SFDP Read (%02X), skip.", mf_id);
        return ESP_OK;
    }

    ESP_LOGI(TAG, "XM25QHxxC startup flow");
    // Enter DPD
    bootloader_execute_flash_command(0xB9, 0, 0, 0);
    // Enter UDPD
    bootloader_execute_flash_command(0x79, 0, 0, 0);
    // Exit UDPD
    bootloader_execute_flash_command(0xFF, 0, 0, 0);
    // Delay tXUDPD
    ets_rom_delay_us(2000);
    // Release Power-down
    bootloader_execute_flash_command(0xAB, 0, 0, 0);
    ets_rom_delay_us(20);
    // Read flash ID and check again
    id = bootloader_read_flash_id();
    if (!is_xmc_chip_strict(id)) {
        ESP_LOGE(TAG, "XMC flash startup fail");
        return ESP_FAIL;
    }

    return ESP_OK;
}
#else
static bool is_xmc_chip(uint32_t rdid)
{
    uint32_t vendor_id = (rdid >> 16) &0xff;
    
    return vendor_id == XMC_VENDOR_ID;
}

esp_err_t bootloader_flash_xmc_startup(void)
{
    uint32_t id = bootloader_read_flash_id();

    if (is_xmc_chip(id)) {
        ESP_LOGE(TAG, "XMC chip detected(%08X) while support disable.", id);
        return ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "flash chip is %08X", id);
    }

    return ESP_OK;
}
#endif
#endif

#endif
