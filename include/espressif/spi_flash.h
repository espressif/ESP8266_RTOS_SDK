/* 
 * copyright (c) Espressif System 2010
 * 
 */

#ifndef __SPI_FLASH_H__
#define __SPI_FLASH_H__

typedef enum {
    SPI_FLASH_RESULT_OK,
    SPI_FLASH_RESULT_ERR,
    SPI_FLASH_RESULT_TIMEOUT
} SpiFlashOpResult;

#define SPI_FLASH_SEC_SIZE      4096

SpiFlashOpResult spi_flash_erase_sector(uint16 sec);
SpiFlashOpResult spi_flash_write(uint32 des_addr, uint32 *src_addr, uint32 size);
SpiFlashOpResult spi_flash_read(uint32 src_addr, uint32 *des_addr, uint32 size);

#endif
