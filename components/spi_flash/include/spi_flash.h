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

#ifndef _SPI_FLASH_H
#define _SPI_FLASH_H

#include <stdint.h>
#include <stddef.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_ERR_FLASH_BASE        0x10010
#define ESP_ERR_FLASH_OP_FAIL     (ESP_ERR_FLASH_BASE + 1)
#define ESP_ERR_FLASH_OP_TIMEOUT  (ESP_ERR_FLASH_BASE + 2)

#define SPI_FLASH_SEC_SIZE        4096    /**< SPI Flash sector size */

#define SPI_READ_BUF_MAX          64

#define SPI_FLASH_CACHE2PHYS_FAIL UINT32_MAX /*<! Result from spi_flash_cache2phys() if flash cache address is invalid */

#ifdef CONFIG_ENABLE_FLASH_MMAP
/**
 * @brief Enumeration which specifies memory space requested in an mmap call
 */
typedef enum {
    SPI_FLASH_MMAP_DATA,    /**< map to data memory (Vaddr0), allows byte-aligned access, 4 MB total */
    SPI_FLASH_MMAP_INST,    /**< map to instruction memory (Vaddr1-3), allows only 4-byte-aligned access, 11 MB total */
} spi_flash_mmap_memory_t;

/**
 * @brief Opaque handle for memory region obtained from spi_flash_mmap.
 */
typedef uint32_t spi_flash_mmap_handle_t;
#endif

/**
 * @brief  Get flash chip size, as set in binary image header
 *
 * @note This value does not necessarily match real flash size.
 *
 * @return size of flash chip, in bytes
 */
size_t spi_flash_get_chip_size();

/**
 * @brief  Erase the Flash sector.
 *
 * @param  sector  Sector number, the count starts at sector 0, 4KB per sector.
 *
 * @return esp_err_t
 */
esp_err_t spi_flash_erase_sector(size_t sector);

/**
 * @brief  Erase a range of flash sectors
 *
 * @param  start_address  Address where erase operation has to start.
 *                                  Must be 4kB-aligned
 * @param  size  Size of erased range, in bytes. Must be divisible by 4kB.
 *
 * @return esp_err_t
 */
esp_err_t spi_flash_erase_range(size_t start_address, size_t size);

/**
 * @brief  Write data to Flash.
 *
 * @note For fastest write performance, write a 4 byte aligned size at a
 * 4 byte aligned offset in flash from a source buffer in DRAM. Varying any of
 * these parameters will still work, but will be slower due to buffering.
 *
 * @note Writing more than 8KB at a time will be split into multiple
 * write operations to avoid disrupting other tasks in the system.
 *
 * @param  dest_addr Destination address in Flash.
 * @param  src       Pointer to the source buffer.
 * @param  size      Length of data, in bytes.
 *
 * @return esp_err_t
 */
esp_err_t spi_flash_write(size_t dest_addr, const void *src, size_t size);

/**
 * @brief  Read data from Flash.
 *
 * @note For fastest read performance, all parameters should be
 * 4 byte aligned. If source address and read size are not 4 byte
 * aligned, read may be split into multiple flash operations. If
 * destination buffer is not 4 byte aligned, a temporary buffer will
 * be allocated on the stack.
 *
 * @note Reading more than 16KB of data at a time will be split
 * into multiple reads to avoid disruption to other tasks in the
 * system. Consider using spi_flash_mmap() to read large amounts
 * of data.
 *
 * @param  src_addr source address of the data in Flash.
 * @param  dest     pointer to the destination buffer
 * @param  size     length of data
 *
 *
 * @return esp_err_t
 */
esp_err_t spi_flash_read(size_t src_addr, void *dest, size_t size);

#ifdef CONFIG_ENABLE_FLASH_MMAP

/**
 * @brief Map region of flash memory into data or instruction address space
 *
 * This function allocates sufficient number of 64kB MMU pages and configures
 * them to map the requested region of flash memory into the address space.
 * It may reuse MMU pages which already provide the required mapping.
 *
 * As with any allocator, if mmap/munmap are heavily used then the address space
 * may become fragmented. To troubleshoot issues with page allocation, use
 * spi_flash_mmap_dump() function.
 *
 * @param src_addr  Physical address in flash where requested region starts.
 *                  This address *must* be aligned to 64kB boundary
 *                  (SPI_FLASH_MMU_PAGE_SIZE)
 * @param size  Size of region to be mapped. This size will be rounded
 *              up to a 64kB boundary
 * @param memory  Address space where the region should be mapped (data or instruction)
 * @param out_ptr  Output, pointer to the mapped memory region
 * @param out_handle  Output, handle which should be used for spi_flash_munmap call
 *
 * @return  ESP_OK on success, ESP_ERR_NO_MEM if pages can not be allocated
 */
esp_err_t spi_flash_mmap(size_t src_addr, size_t size, spi_flash_mmap_memory_t memory,
                         const void** out_ptr, spi_flash_mmap_handle_t* out_handle);

/**
 * @brief Release region previously obtained using spi_flash_mmap
 *
 * @note Calling this function will not necessarily unmap memory region.
 *       Region will only be unmapped when there are no other handles which
 *       reference this region. In case of partially overlapping regions
 *       it is possible that memory will be unmapped partially.
 *
 * @param handle  Handle obtained from spi_flash_mmap
 */
void spi_flash_munmap(spi_flash_mmap_handle_t handle);

#endif /* CONFIG_ENABLE_FLASH_MMAP */

/**
 * @brief Given a memory address where flash is mapped, return the corresponding physical flash offset.
 *
 * Cache address does not have have been assigned via spi_flash_mmap(), any address in memory mapped flash space can be looked up.
 *
 * @param cached Pointer to flashed cached memory.
 *
 * @return
 * - SPI_FLASH_CACHE2PHYS_FAIL If cache address is outside flash cache region, or the address is not mapped.
 * - Otherwise, returns physical offset in flash
 */
uintptr_t spi_flash_cache2phys(const void *cached);

#ifdef CONFIG_ESP8266_OTA_FROM_OLD

/**
 * @brief Check if current firmware updates from V2 firmware and its location is at "APP2", if so, then V3 bootloader 
 *        will copy patition table from "APP2" location to "APP1" location of V2 partition map.
 *
 * @return 0 if success or others if failed
 */
int esp_patition_table_init_location(void);

/**
 * @brief Check if current firmware updates from V2 firmware and its location is at "APP2", if so, then V3 bootloader
 *         will copy firmware from "APP2" location to "APP1" location.
 * 
 * @note All data which is copied is "ota0" application and all data whose location is before "ota0". 
 *
 * @return 0 if success or others if failed
 */
int esp_patition_table_init_data(void *partition_info);
#endif

#ifdef CONFIG_ESP8266_BOOT_COPY_APP
/**
 * @brief Check if current application which is to run is at "ota1" location, if so, bootloader will copy it to "ota0" location,
 *        and clear OTA data partition.
 *
 * @return 0 if success or others if failed
 */
int esp_patition_copy_ota1_to_ota0(const void *partition_info);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _SPI_FLASH_H */
