/*
 * spiffs_test_params.h
 *
 */

#ifndef __SPIFFS_TEST_PARAMS_H__
#define __SPIFFS_TEST_PARAMS_H__

#define FS1_FLASH_SIZE      (128*1024)
#define FS2_FLASH_SIZE      (128*1024)

#define FS1_FLASH_ADDR      (1024*1024)
#define FS2_FLASH_ADDR      (1280*1024)

#define SECTOR_SIZE         (4*1024) 
#define LOG_BLOCK           (SECTOR_SIZE)
#define LOG_PAGE            (128)

#define FD_BUF_SIZE         32*4
#define CACHE_BUF_SIZE      (LOG_PAGE + 32)*8

#endif /* __SPIFFS_TEST_PARAMS_H__ */
