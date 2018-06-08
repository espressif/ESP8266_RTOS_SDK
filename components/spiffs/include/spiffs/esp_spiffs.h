/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
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

#ifndef __ESP_SPIFFS_H__
#define __ESP_SPIFFS_H__

#include "spiffs/spiffs.h"
#include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup Spiffs_APIs Spiffs APIs
  * @brief Spiffs APIs
  *
  * More details about spiffs on https://github.com/pellepl/spiffs
  *
  */

/** @addtogroup Spiffs_APIs
  * @{
  */

struct esp_spiffs_config {
    u32_t phys_size;        /**< physical size of the SPI Flash */
    u32_t phys_addr;        /**< physical offset in spi flash used for spiffs, must be on block boundary */
    u32_t phys_erase_block; /**< physical size when erasing a block */

    u32_t log_block_size;   /**< logical size of a block, must be on physical block size boundary and must never be less than a physical block */
    u32_t log_page_size;    /**< logical size of a page, at least log_block_size/8  */

    u32_t fd_buf_size;      /**< file descriptor memory area size */
    u32_t cache_buf_size;   /**< cache buffer size */
};

/**
  * @brief  Initialize spiffs
  *
  * @param  struct esp_spiffs_config *config : ESP8266 spiffs configuration
  *
  * @return 0         : succeed
  * @return otherwise : fail
  */
s32_t esp_spiffs_init(struct esp_spiffs_config *config);

/**
  * @brief  Deinitialize spiffs
  *
  * @param  uint8 format : 0, only deinit; otherwise, deinit spiffs and format.
  *
  * @return null
  */
void esp_spiffs_deinit(u8_t format);

/**
  * @}
  */

int _spiffs_open_r(struct _reent *r, const char *filename, int flags, int mode);
_ssize_t _spiffs_read_r(struct _reent *r, int fd, void *buf, size_t len);
_ssize_t _spiffs_write_r(struct _reent *r, int fd, void *buf, size_t len);
_off_t _spiffs_lseek_r(struct _reent *r, int fd, _off_t where, int whence);
int _spiffs_close_r(struct _reent *r, int fd);
int _spiffs_rename_r(struct _reent *r, const char *from, const char *to);
int _spiffs_unlink_r(struct _reent *r, const char *filename);
int _spiffs_fstat_r(struct _reent *r, int fd, struct stat *s);

#ifdef __cplusplus
}
#endif

#endif /* __ESP_SPIFFS_H__ */
