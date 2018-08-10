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

#include <string.h>

#include "spi_flash.h"

#include "esp_attr.h"
#include "esp_wifi_osi.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp8266/eagle_soc.h"
#include "esp8266/rom_functions.h"
#include "esp8266/pin_mux_register.h"
#include "driver/spi_register.h"

/* Todo: Refactor SPI debug */
#define spi_debug(fmt, ...)                     //ESP_LOGD(TAG, fmt, ##__VA_ARGS__)

#define SPI_EXT2(i)                             (REG_SPI_BASE(i) + 0xF8)
#define SPI_EXT3(i)                             (REG_SPI_BASE(i) + 0xFC)

#define SPI_ENABLE_AHB                          BIT17

#define SPI_FLASH_CLK_EQU_SYSCLK                BIT12

//SPI flash command
#define SPI_FLASH_READ                          BIT31
#define SPI_FLASH_WREN                          BIT30
#define SPI_FLASH_WRDI                          BIT29
#define SPI_FLASH_RDID                          BIT28
#define SPI_FLASH_RDSR                          BIT27
#define SPI_FLASH_WRSR                          BIT26
#define SPI_FLASH_PP                            BIT25
#define SPI_FLASH_SE                            BIT24
#define SPI_FLASH_BE                            BIT23
#define SPI_FLASH_CE                            BIT22
#define SPI_FLASH_RES                           BIT20
#define SPI_FLASH_DPD                           BIT21
#define SPI_FLASH_HPM                           BIT19

//SPI address register
#define SPI_FLASH_BYTES_LEN                     24
#define SPI_BUFF_BYTE_NUM                       32
#define IODATA_START_ADDR                       BIT0

//SPI status register
#define  SPI_FLASH_BUSY_FLAG                    BIT0
#define  SPI_FLASH_WRENABLE_FLAG                BIT1
#define  SPI_FLASH_BP0                          BIT2
#define  SPI_FLASH_BP1                          BIT3
#define  SPI_FLASH_BP2                          BIT4
#define  SPI_FLASH_TOP_BOT_PRO_FLAG             BIT5
#define  SPI_FLASH_STATUS_PRO_FLAG              BIT7

#define  FLASH_WR_PROTECT                       (SPI_FLASH_BP0|SPI_FLASH_BP1|SPI_FLASH_BP2)

#define SPI 0

#define PERIPHS_SPI_FLASH_C0                    SPI_W0(SPI)
#define PERIPHS_SPI_FLASH_CTRL                  SPI_CTRL(SPI)
#define PERIPHS_SPI_FLASH_CMD                   SPI_CMD(SPI)

//flash cmd
#define SPI_FLASH_READ_UNIQUE_CMD               0x4B
#define SPI_FLASH_WRITE_STATUS_REGISTER         0X01
#define SPI_FLASH_ISSI_ENABLE_QIO_MODE          (BIT(6))

/*gd25q32c*/
#define SPI_FLASH_GD25Q32C_WRITE_STATUSE1_CMD   (0X01)
#define SPI_FLASH_GD25Q32C_WRITE_STATUSE2_CMD   (0X31)
#define SPI_FLASH_GD25Q32C_WRITE_STATUSE3_CMD   (0X11)

#define SPI_FLASH_GD25Q32C_READ_STATUSE1_CMD    (0X05)
#define SPI_FLASH_GD25Q32C_READ_STATUSE2_CMD    (0X35)
#define SPI_FLASH_GD25Q32C_READ_STATUSE3_CMD    (0X15)

#define SPI_FLASH_GD25Q32C_QIO_MODE             (BIT(1))

#define SPI_ISSI_FLASH_WRITE_PROTECT_STATUS     (BIT(2)|BIT(3)|BIT(4)|BIT(5))
#define SPI_EON_25Q16A_WRITE_PROTECT_STATUS     (BIT(2)|BIT(3)|BIT(4)|BIT(5))
#define SPI_EON_25Q16B_WRITE_PROTECT_STATUS     (BIT(2)|BIT(3)|BIT(4)|BIT(5))
#define SPI_GD25Q32_FLASH_WRITE_PROTECT_STATUS  (BIT(2)|BIT(3)|BIT(4)|BIT(5)|BIT(6))
#define SPI_FLASH_RDSR2      0x35
#define SPI_FLASH_PROTECT_STATUS                (BIT(2)|BIT(3)|BIT(4)|BIT(5)|BIT(6)|BIT(14))

#define FLASH_INTR_DECLARE(t)                   uint32_t t
#define FLASH_INTR_LOCK(t)                      wifi_enter_critical(t)
#define FLASH_INTR_UNLOCK(t)                    wifi_exit_critical(t)

#define FLASH_ALIGN_BYTES                       4
#define FLASH_ALIGN(addr)                       ((((size_t)addr) + (FLASH_ALIGN_BYTES - 1)) & (~(FLASH_ALIGN_BYTES - 1)))
#define FLASH_ALIGN_BEFORE(addr)                (FLASH_ALIGN(addr) - 4)
#define NOT_ALIGN(addr)                         (((size_t)addr) & (FLASH_ALIGN_BYTES - 1))
#define IS_ALIGN(addr)                          (NOT_ALIGN(addr) == 0)

enum GD25Q32C_status {
    GD25Q32C_STATUS1=0,
    GD25Q32C_STATUS2,
    GD25Q32C_STATUS3,
};

typedef enum {
    SPI_TX   = 0x1,
    SPI_RX   = 0x2,
    SPI_WRSR = 0x4,
    SPI_RAW  = 0x8,  /*!< No wait spi idle and no read status */
} spi_cmd_dir_t;

typedef struct {
    uint16_t cmd;
    uint8_t cmd_len;
    uint32_t *addr;
    uint8_t addr_len;
    uint32_t *data;
    uint8_t data_len;
    uint8_t dummy_bits;
} spi_cmd_t;

extern uint32_t esp_get_time();

bool IRAM_ATTR spi_user_cmd(spi_cmd_dir_t mode, spi_cmd_t *p_cmd);
bool special_flash_read_status(uint8_t command, uint32_t* status, int len);
bool special_flash_write_status(uint8_t command, uint32_t status, int len, bool write_en);
esp_err_t spi_flash_read(size_t src_addr, void *dest, size_t size);
uint8_t en25q16x_read_sfdp();

extern void pp_soft_wdt_feed(void);
extern void pp_soft_wdt_stop(void);
extern void pp_soft_wdt_restart(void);

esp_spi_flash_chip_t flashchip = {
    0x1640ef,
    (32 / 8) * 1024 * 1024,
    64 * 1024,
    4 * 1024,
    256,
    0xffff
};

uint8_t FlashIsOnGoing = 0;

const char *TAG = "spi_flash";

esp_err_t IRAM_ATTR SPIWrite(uint32_t  target, uint32_t *src_addr, size_t len)
{
    uint32_t  page_size;
    uint32_t  pgm_len, pgm_num;
    uint8_t    i;

    //check program size
    if ((target + len) > (flashchip.chip_size)) {
        return ESP_ERR_FLASH_OP_FAIL;
    }

    page_size = flashchip.page_size;
    pgm_len = page_size - (target % page_size);

    if (len < pgm_len) {
        if (ESP_OK != SPI_page_program(&flashchip,  target, src_addr, len)) {
            return ESP_ERR_FLASH_OP_FAIL;
        }
    } else {
        if (ESP_OK != SPI_page_program(&flashchip,  target, src_addr, pgm_len)) {
            return ESP_ERR_FLASH_OP_FAIL;
        }

        //whole page program
        pgm_num = (len - pgm_len) / page_size;

        for (i = 0; i < pgm_num; i++) {
            if (ESP_OK != SPI_page_program(&flashchip,  target + pgm_len, src_addr + (pgm_len >> 2), page_size)) {
                return ESP_ERR_FLASH_OP_FAIL;
            }

            pgm_len += page_size;
        }

        //remain parts to program
        if (ESP_OK != SPI_page_program(&flashchip,  target + pgm_len, src_addr + (pgm_len >> 2), len - pgm_len)) {
            return ESP_ERR_FLASH_OP_FAIL;
        }
    }

    return  ESP_OK;
}

esp_err_t IRAM_ATTR SPIRead(uint32_t target, uint32_t *dest_addr, size_t len)
{
    if (ESP_OK != SPI_read_data(&flashchip, target, dest_addr, len)) {
        return ESP_ERR_FLASH_OP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t IRAM_ATTR SPIEraseSector(uint32_t sector_num)
{
    if (sector_num >= ((flashchip.chip_size) / (flashchip.sector_size))) {
        return ESP_ERR_FLASH_OP_FAIL;
    }

    if (ESP_OK != SPI_write_enable(&flashchip)) {
        return ESP_ERR_FLASH_OP_FAIL;
    }

    if (ESP_OK != SPI_sector_erase(&flashchip, sector_num * (flashchip.sector_size))) {
        return ESP_ERR_FLASH_OP_FAIL;
    }

    return ESP_OK;
}

static void IRAM_ATTR Cache_Read_Disable_2(void)
{
    CLEAR_PERI_REG_MASK(CACHE_FLASH_CTRL_REG,CACHE_READ_EN_BIT);
    while(REG_READ(SPI_EXT2(0)) != 0) { }
    CLEAR_PERI_REG_MASK(PERIPHS_SPI_FLASH_CTRL,SPI_ENABLE_AHB);
}

void IRAM_ATTR Cache_Read_Enable_2()
{
    SET_PERI_REG_MASK(PERIPHS_SPI_FLASH_CTRL,SPI_ENABLE_AHB);
    SET_PERI_REG_MASK(CACHE_FLASH_CTRL_REG,CACHE_READ_EN_BIT);
}
void Cache_Read_Enable_New(void) __attribute__((alias("Cache_Read_Enable_2")));

static uint32_t IRAM_ATTR spi_flash_get_id(void)
{
    uint32_t rdid = 0;
    FLASH_INTR_DECLARE(c_tmp);

    FLASH_INTR_LOCK(c_tmp);

    FlashIsOnGoing = 1;

    Cache_Read_Disable();

    Wait_SPI_Idle(&flashchip);

    WRITE_PERI_REG(PERIPHS_SPI_FLASH_C0, 0);    // clear regisrter
    WRITE_PERI_REG(PERIPHS_SPI_FLASH_CMD, SPI_FLASH_RDID);
    while(READ_PERI_REG(PERIPHS_SPI_FLASH_CMD)!=0);

    rdid = READ_PERI_REG(PERIPHS_SPI_FLASH_C0)&0xffffff;

    Cache_Read_Enable_New();

    FlashIsOnGoing = 0;

    FLASH_INTR_UNLOCK(c_tmp);

    return rdid;
}

static esp_err_t IRAM_ATTR spi_flash_read_status(uint32_t *status)
{
    esp_err_t ret;
    FLASH_INTR_DECLARE(c_tmp);

    FLASH_INTR_LOCK(c_tmp);

    FlashIsOnGoing = 1;

    Cache_Read_Disable_2();

    ret =  SPI_read_status(&flashchip, status);

    Cache_Read_Enable_2();

    FlashIsOnGoing = 0;

    FLASH_INTR_UNLOCK(c_tmp);

    return ret;
}

static esp_err_t IRAM_ATTR spi_flash_write_status(uint32_t status_value)
{
    FLASH_INTR_DECLARE(c_tmp);

    FLASH_INTR_LOCK(c_tmp);

    FlashIsOnGoing = 1;

    Cache_Read_Disable_2();

    Wait_SPI_Idle(&flashchip);
    if(ESP_OK != SPI_write_enable(&flashchip)){
        FLASH_INTR_UNLOCK(c_tmp);
        return ESP_ERR_FLASH_OP_FAIL;
    }
    if(ESP_OK != SPI_write_status(&flashchip,status_value)){
        FLASH_INTR_UNLOCK(c_tmp);
        return ESP_ERR_FLASH_OP_FAIL;
    }
    Wait_SPI_Idle(&flashchip);

    Cache_Read_Enable_2();

    FlashIsOnGoing = 0;

    FLASH_INTR_UNLOCK(c_tmp);

    return ESP_OK;
}

static uint8_t IRAM_ATTR flash_gd25q32c_read_status(enum GD25Q32C_status status_index)
{
    uint8_t rdsr_cmd=0;
    if(GD25Q32C_STATUS1 == status_index) {
        rdsr_cmd = SPI_FLASH_GD25Q32C_READ_STATUSE1_CMD;
    }
    else if(GD25Q32C_STATUS2 == status_index) {
        rdsr_cmd = SPI_FLASH_GD25Q32C_READ_STATUSE2_CMD;
    }
    else if(GD25Q32C_STATUS3 == status_index) {
        rdsr_cmd = SPI_FLASH_GD25Q32C_READ_STATUSE3_CMD;
    }
    else {

    }
    uint32_t status;
    special_flash_read_status(rdsr_cmd, &status, 1);
    return ((uint8_t)status);
}

static void flash_gd25q32c_write_status(enum GD25Q32C_status status_index,uint8_t status)
{
    uint32_t wrsr_cmd=0;
    uint32_t new_status = status;
    if(GD25Q32C_STATUS1 == status_index) {
        wrsr_cmd = SPI_FLASH_GD25Q32C_WRITE_STATUSE1_CMD;
    }
    else if(GD25Q32C_STATUS2 == status_index) {
        wrsr_cmd = SPI_FLASH_GD25Q32C_WRITE_STATUSE2_CMD;
    }
    else if(GD25Q32C_STATUS3 == status_index) {
        wrsr_cmd = SPI_FLASH_GD25Q32C_WRITE_STATUSE3_CMD;
    }
    else {
        //ets_printf("[ERR]Not konw GD25Q32C status idx %d\n ",spi_wr_status_cmd);
    }
    special_flash_write_status(wrsr_cmd, new_status, 1, true);
}

static bool spi_flash_check_wr_protect(void)
{
    uint32_t flash_id=spi_flash_get_id();
    uint32_t status=0;
    //check for EN25Q16A/B flash chips
    if ((flash_id & 0xffffff) == 0x15701c) {
        uint8_t sfdp = en25q16x_read_sfdp();
        if (sfdp == 0xE5) {
            spi_debug("EN25Q16A\n");
            //This is EN25Q16A, set bit6 in the same way as issi flash chips.
            if(spi_flash_read_status(&status)==0) { //Read status Ok
                if(status&(SPI_EON_25Q16A_WRITE_PROTECT_STATUS)) { //Write_protect
                    special_flash_write_status(0x1, status&(~(SPI_EON_25Q16A_WRITE_PROTECT_STATUS)), 1, true);
                }
            }
        } else if (sfdp == 0xED) {
            spi_debug("EN25Q16B\n");
            //This is EN25Q16B
            if(spi_flash_read_status(&status)==0) { //Read status Ok
                if(status&(SPI_EON_25Q16B_WRITE_PROTECT_STATUS)) { //Write_protect
                    special_flash_write_status(0x1, status&(~(SPI_EON_25Q16B_WRITE_PROTECT_STATUS)), 1, true);
                }
            }
        }
    }
    //MXIC :0XC2
    //ISSI :0X9D
    // ets_printf("spi_flash_check_wr_protect\r\n");
    else if(((flash_id&0xFF)==0X9D)||((flash_id&0xFF)==0XC2)||((flash_id & 0xFF) == 0x1C)) {
        if(spi_flash_read_status(&status)==0) { //Read status Ok
            if(status&(SPI_ISSI_FLASH_WRITE_PROTECT_STATUS)) { //Write_protect
                special_flash_write_status(0x1, status&(~(SPI_ISSI_FLASH_WRITE_PROTECT_STATUS)), 1, true);
            }
        }
    }
    //GD25Q32C:0X16409D
    //GD25Q128
    else if(((flash_id&0xFFFFFFFF)==0X1640C8)||((flash_id&0xFFFFFFFF)==0X1840C8)) {
        if(spi_flash_read_status(&status)==0) { //Read status Ok
            if(status&SPI_GD25Q32_FLASH_WRITE_PROTECT_STATUS) {
                special_flash_write_status(0x01, status&(~(SPI_GD25Q32_FLASH_WRITE_PROTECT_STATUS)), 1, true);
            }
        }
    }
    //Others
    else {
        if(spi_flash_read_status(&status)==0) { //Read status Ok
            uint32_t status1 = 0; //flash_gd25q32c_read_status(GD25Q32C_STATUS2);
            special_flash_read_status(SPI_FLASH_RDSR2, &status1, 1);
            status=(status1 << 8)|(status & 0xff);
            if(status&SPI_FLASH_PROTECT_STATUS) {
                status=((status&(~SPI_FLASH_PROTECT_STATUS))&0xffff);
                spi_flash_write_status(status);
            }
        }
    }
    return true;
}

/******************************************************************************
 * FunctionName : spi_flash_erase_sector
 * Description  : a
 * Parameters   :
 * Returns      :
*******************************************************************************/
esp_err_t IRAM_ATTR spi_flash_erase_sector(size_t sec)
{
    FLASH_INTR_DECLARE(c_tmp);

    esp_err_t ret;

    if (spi_flash_check_wr_protect() == false) {
        return ESP_ERR_FLASH_OP_FAIL;
    }

    spi_debug("E[%x] %d-", sec, esp_get_time());

    FLASH_INTR_LOCK(c_tmp);
    pp_soft_wdt_stop();
    FlashIsOnGoing = 1;
    Cache_Read_Disable_2();

    ret = SPIEraseSector(sec);

    Cache_Read_Enable_2();
    FlashIsOnGoing = 0;
    pp_soft_wdt_restart();
    FLASH_INTR_UNLOCK(c_tmp);

    spi_debug("%d\n", esp_get_time());

    return ret;
}

esp_err_t spi_flash_enable_qmode(void)
{
    esp_err_t ret;

    Cache_Read_Disable_2();
    ret = Enable_QMode(&flashchip);
    Wait_SPI_Idle(&flashchip);
    Cache_Read_Enable_2();

    return ret;
}

static esp_err_t IRAM_ATTR spi_flash_write_raw(size_t dest_addr, const void *src, size_t size)
{
    esp_err_t ret;
    FLASH_INTR_DECLARE(c_tmp);

    spi_debug("W[%x] %d-", dest_addr / 4096, esp_get_time());

    FLASH_INTR_LOCK(c_tmp);
    pp_soft_wdt_stop();
    FlashIsOnGoing = 1;
    Cache_Read_Disable_2();

    ret = SPIWrite(dest_addr, (uint32_t *)src, size);

    Cache_Read_Enable_2();
    FlashIsOnGoing = 0;
    pp_soft_wdt_restart();
    FLASH_INTR_UNLOCK(c_tmp);

    spi_debug("%d\n", esp_get_time());

    return ret;
}

esp_err_t IRAM_ATTR spi_flash_write(size_t dest_addr, const void *src, size_t size)
{
#undef FLASH_WRITE
#define FLASH_WRITE(dest, src, size)                \
{                                                   \
    ret = spi_flash_write_raw(dest, src, size);     \
    pp_soft_wdt_feed();                             \
    if (ret) {                                      \
        return ret;                                 \
    }                                               \
}

#undef FLASH_READ
#define FLASH_READ(dest, src, size)                 \
{                                                   \
    ret = spi_flash_read(dest, src, size);          \
    if (ret) {                                      \
        return ret;                                 \
    }                                               \
}

    esp_err_t ret = ESP_ERR_FLASH_OP_FAIL;
    uint8_t *tmp = (uint8_t *)src;

    if (!size)
        return ESP_OK;

    if (src == NULL) {
        return ESP_ERR_FLASH_OP_FAIL;
    }

    if (spi_flash_check_wr_protect() == false) {
        return ESP_ERR_FLASH_OP_FAIL;
    }

    if (NOT_ALIGN(dest_addr)
        || NOT_ALIGN(tmp)
        || NOT_ALIGN(size)
        || IS_FLASH(src)) {
        uint8_t buf[SPI_READ_BUF_MAX];

        if (NOT_ALIGN(dest_addr)) {
            size_t r_addr = FLASH_ALIGN_BEFORE(dest_addr);
            size_t c_off = dest_addr - r_addr;
            size_t wbytes = FLASH_ALIGN_BYTES - c_off;

            wbytes = wbytes > size ? size : wbytes;

            FLASH_READ(r_addr, buf, FLASH_ALIGN_BYTES);
            memcpy(&buf[c_off], tmp, wbytes);
            FLASH_WRITE(r_addr, buf, FLASH_ALIGN_BYTES);

            dest_addr += wbytes;
            tmp += wbytes;
            size -= wbytes;
        }

        while (size > 0) {
            size_t len = size >= SPI_READ_BUF_MAX ? SPI_READ_BUF_MAX : size;
            size_t wlen = FLASH_ALIGN(len);

            if (wlen != len) {
                size_t l_b = wlen - FLASH_ALIGN_BYTES;

                FLASH_READ(dest_addr + l_b, &buf[l_b], FLASH_ALIGN_BYTES);                
            }

            memcpy(buf, tmp, len);

            FLASH_WRITE(dest_addr, buf, wlen);

            dest_addr += len;
            tmp += len;
            size -= len;
        }
    } else {
        FLASH_WRITE(dest_addr, src, size);
    }

    return ret;
}


/******************************************************************************
 * FunctionName : spi_flash_read_raw
 * Description  : a
 * Parameters   :
 * Returns      :
*******************************************************************************/
static esp_err_t IRAM_ATTR spi_flash_read_raw(size_t src_addr, void *dest, size_t size)
{
    esp_err_t ret;
    FLASH_INTR_DECLARE(c_tmp);

    spi_debug("R[%x] %d-", src_addr / 4096, esp_get_time());

    FLASH_INTR_LOCK(c_tmp);
    pp_soft_wdt_stop();
    FlashIsOnGoing = 1;
    Cache_Read_Disable_2();

    ret = SPIRead(src_addr, dest, size);

    Cache_Read_Enable_2();
    FlashIsOnGoing = 0;
    pp_soft_wdt_restart();
    FLASH_INTR_UNLOCK(c_tmp);

    spi_debug("%d\n", esp_get_time());

    return ret;
}

esp_err_t IRAM_ATTR spi_flash_read(size_t src_addr, void *dest, size_t size)
{
#undef FLASH_READ
#define FLASH_READ(addr, dest, size)                        \
{                                                           \
    ret = spi_flash_read_raw(addr, dest, size);             \
    pp_soft_wdt_feed();                                     \
    if (ret)                                                \
        return ret;                                         \
}

    esp_err_t ret;
    uint8_t *tmp = (uint8_t *)dest;

    if (!size)
        return ESP_OK;

    if (tmp == NULL) {
        return ESP_ERR_FLASH_OP_FAIL;
    }

    if (NOT_ALIGN(src_addr)
        || NOT_ALIGN(tmp)
        || NOT_ALIGN(size)) {
        uint8_t buf[SPI_READ_BUF_MAX];

        if (NOT_ALIGN(src_addr)) {
            size_t r_addr = FLASH_ALIGN_BEFORE(src_addr);
            size_t c_off = src_addr - r_addr;
            size_t wbytes = FLASH_ALIGN_BYTES - c_off;

            wbytes = wbytes > size ? size : wbytes;

            FLASH_READ(r_addr, buf, FLASH_ALIGN_BYTES);
            memcpy(tmp, &buf[c_off], wbytes);

            tmp += wbytes;
            src_addr += wbytes;
            size -= wbytes;
        }

        while (size) {
            size_t len = size >= SPI_READ_BUF_MAX ? SPI_READ_BUF_MAX : size;
            size_t wlen = FLASH_ALIGN(len);

            FLASH_READ(src_addr, buf, wlen);

            memcpy(tmp, buf, len);

            src_addr += len;
            tmp += len;
            size -= len;            
        }
    } else {
        FLASH_READ(src_addr, tmp, size);
    }

    return ESP_OK;
}

static void spi_flash_enable_qio_bit6(void)
{
    uint8_t wrsr_cmd = 0x1;
    uint32_t issi_qio = SPI_FLASH_ISSI_ENABLE_QIO_MODE;
    special_flash_write_status(wrsr_cmd, issi_qio, 1, true);
}

/*
if the QIO_ENABLE bit not in Bit9,in Bit6
the SPI send 0x01(CMD)+(1<<6)
*/
static bool spi_flash_issi_enable_QIO_mode(void)
{
    uint32_t status = 0;
    if(spi_flash_read_status(&status) == 0) {
        if((status&SPI_FLASH_ISSI_ENABLE_QIO_MODE)) {
            return true;
        }
    }
    else {
        return false;
    }

    spi_flash_enable_qio_bit6();

    if(spi_flash_read_status(&status) == 0) {
        if((status&SPI_FLASH_ISSI_ENABLE_QIO_MODE)) {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

static bool flash_gd25q32c_enable_QIO_mode()
{
    uint8_t data = 0;
    if((data=flash_gd25q32c_read_status(GD25Q32C_STATUS2))&SPI_FLASH_GD25Q32C_QIO_MODE) {
        return true;
    }
    else {
        flash_gd25q32c_write_status(GD25Q32C_STATUS2,SPI_FLASH_GD25Q32C_QIO_MODE);
        if(flash_gd25q32c_read_status(GD25Q32C_STATUS2)&SPI_FLASH_GD25Q32C_QIO_MODE) {
            return true;

        }
        else {
            return false;
        }
    }
}

bool special_flash_read_status(uint8_t command, uint32_t* status, int len)
{
    if( len > 2 || len < 1) {
        return false;
    }
    spi_cmd_t cmd;
    cmd.cmd = command;
    cmd.cmd_len = 1;
    cmd.addr = NULL;
    cmd.addr_len = 0;
    cmd.dummy_bits = 0;
    cmd.data = status;
    cmd.data_len = len;
    spi_user_cmd(SPI_RX, &cmd);
    return true;
}

bool special_flash_write_status(uint8_t command, uint32_t status, int len, bool write_en)
{
    if (len > 2 || len < 1) {
        return false;
    }
    spi_cmd_t cmd;
    cmd.cmd = command;
    cmd.cmd_len = 1;
    cmd.addr = NULL;
    cmd.addr_len = 0;
    cmd.dummy_bits = 0;
    cmd.data = &status;
    cmd.data_len = len > 1 ? 2 : 1;
    if (write_en) {
        spi_user_cmd(SPI_WRSR, &cmd);
    } else {
        spi_user_cmd(SPI_TX | SPI_RAW, &cmd);
    }
    return true;
}

void special_flash_set_mode(uint8_t command, bool disable_wait_idle)
{
    spi_cmd_t cmd;
    cmd.cmd = command;
    cmd.cmd_len = 1;
    cmd.addr = NULL;
    cmd.addr_len = 0;
    cmd.dummy_bits = 0;
    cmd.data = NULL;
    cmd.data_len = 0;
    if (disable_wait_idle) {
        spi_user_cmd(SPI_TX | SPI_RAW, &cmd);
    } else {
        spi_user_cmd(SPI_TX, &cmd);
    }
}

bool en25q16x_write_volatile_status(uint8_t vsr)
{
    //enter OTP mode
    special_flash_set_mode(0x3a, true);
    //volatile status register write enable
    special_flash_set_mode(0x50, true);
    //send 0x01 + 0x40 to set WHDIS bit
    special_flash_write_status(0x01, vsr, 1, false);
    //check
    uint32_t status = 0;
    special_flash_read_status(0x05, &status, 1);
    //Leave OTP mode
    special_flash_set_mode(0x04, false);

    if (status == 0x40) {
        return true;
    } else {
        return false;
    }
}

uint8_t en25q16x_read_sfdp()
{
    spi_cmd_t cmd;
    cmd.cmd = 0x5a;
    cmd.cmd_len = 1;
    uint32_t addr = 0x00003000;
    cmd.addr = &addr;
    cmd.addr_len = 3;
    cmd.dummy_bits = 8;
    uint32_t data = 0;
    cmd.data = &data;
    cmd.data_len = 1;
    spi_user_cmd(SPI_RX, &cmd);
    spi_debug("0x5a cmd: 0x%08x\n", data);
    return ((uint8_t) data);
}

bool IRAM_ATTR spi_user_cmd(spi_cmd_dir_t mode, spi_cmd_t *p_cmd)
{
    if ((p_cmd->addr_len != 0 && p_cmd->addr == NULL)
            || (p_cmd->data_len != 0 && p_cmd->data == NULL)
            || (p_cmd == NULL)) {
        return false;
    }
    int idx = 0;
    FLASH_INTR_DECLARE(c_tmp);

    FLASH_INTR_LOCK(c_tmp);

    FlashIsOnGoing = 1;
    // Cache Disable
    Cache_Read_Disable_2();
    //wait spi idle
    if((mode & SPI_RAW) == 0) {
        Wait_SPI_Idle(&flashchip);
    }
    //save reg
    uint32_t io_mux_reg = READ_PERI_REG(PERIPHS_IO_MUX_CONF_U);
    uint32_t spi_clk_reg = READ_PERI_REG(SPI_CLOCK(SPI));
    uint32_t spi_ctrl_reg = READ_PERI_REG(SPI_CTRL(SPI));
    uint32_t spi_user_reg = READ_PERI_REG(SPI_USER(SPI));

    if (mode & SPI_WRSR) {
        // enable write register
        SPI_write_enable(&flashchip);
    }

    SET_PERI_REG_MASK(SPI_USER(SPI),SPI_USR_COMMAND);

    //Disable flash operation mode
    CLEAR_PERI_REG_MASK(SPI_USER(SPI), SPI_FLASH_MODE);
    //SET SPI SEND BUFFER MODE
    CLEAR_PERI_REG_MASK(SPI_USER(SPI), SPI_USR_MISO_HIGHPART);
    //CLEAR EQU SYS CLK
    CLEAR_PERI_REG_MASK(PERIPHS_IO_MUX_CONF_U,SPI0_CLK_EQU_SYS_CLK);

    SET_PERI_REG_MASK(SPI_USER(SPI), SPI_CS_SETUP|SPI_CS_HOLD|SPI_USR_COMMAND|SPI_USR_MOSI);
    CLEAR_PERI_REG_MASK(SPI_USER(SPI), SPI_FLASH_MODE);
    //CLEAR DAUL OR QUAD LINES TRANSMISSION MODE
    CLEAR_PERI_REG_MASK(SPI_CTRL(SPI), SPI_QIO_MODE|SPI_DIO_MODE|SPI_DOUT_MODE|SPI_QOUT_MODE);
    WRITE_PERI_REG(SPI_CLOCK(SPI),
                   ((3&SPI_CLKCNT_N)<<SPI_CLKCNT_N_S)|
                   ((1&SPI_CLKCNT_H)<<SPI_CLKCNT_H_S)|
                   ((3&SPI_CLKCNT_L)<<SPI_CLKCNT_L_S)); //clear bit 31,set SPI clock div
    //Enable fast read mode
    SET_PERI_REG_MASK(SPI_CTRL(SPI), SPI_FASTRD_MODE);

    //WAIT COMMAND DONE
    while(READ_PERI_REG(SPI_CMD(SPI)) & SPI_USR);

    //SET USER CMD
    if (p_cmd->cmd_len != 0) {
        //Max CMD length is 16 bits
        SET_PERI_REG_BITS(SPI_USER2(SPI), SPI_USR_COMMAND_BITLEN, p_cmd->cmd_len * 8 - 1, SPI_USR_COMMAND_BITLEN_S);
        //Enable CMD
        SET_PERI_REG_MASK(SPI_USER(SPI), SPI_USR_COMMAND);
        //LOAD CMD
        SET_PERI_REG_BITS(SPI_USER2(SPI), SPI_USR_COMMAND_VALUE, p_cmd->cmd, SPI_USR_COMMAND_VALUE_S);
    } else {
        //CLEAR CMD
        CLEAR_PERI_REG_MASK(SPI_USER(SPI), SPI_USR_COMMAND);
        SET_PERI_REG_BITS(SPI_USER2(SPI), SPI_USR_COMMAND_BITLEN, 0, SPI_USR_COMMAND_BITLEN_S);
    }
    if (p_cmd->dummy_bits != 0) {
        //SET dummy bits
        SET_PERI_REG_BITS(SPI_USER1(SPI), SPI_USR_DUMMY_CYCLELEN, p_cmd->dummy_bits - 1, SPI_USR_DUMMY_CYCLELEN_S);
        //Enable dummy
        SET_PERI_REG_MASK(SPI_USER(SPI), SPI_USR_DUMMY);
    } else {
        //CLEAR DUMMY
        SET_PERI_REG_BITS(SPI_USER1(SPI), SPI_USR_DUMMY_CYCLELEN, 0, SPI_USR_DUMMY_CYCLELEN_S);
        CLEAR_PERI_REG_MASK(SPI_USER(SPI), SPI_USR_DUMMY);
    }

    //SET USER ADDRESS
    if (p_cmd->addr_len != 0) {
        //Set addr lenght
        SET_PERI_REG_BITS(SPI_USER1(SPI), SPI_USR_ADDR_BITLEN, p_cmd->addr_len * 8 - 1, SPI_USR_ADDR_BITLEN_S);
        //Enable user address
        SET_PERI_REG_MASK(SPI_USER(SPI), SPI_USR_ADDR);
        WRITE_PERI_REG(SPI_ADDR(SPI), *p_cmd->addr);
    } else {
        //CLEAR ADDR
        CLEAR_PERI_REG_MASK(SPI_USER(SPI), SPI_USR_ADDR);
        SET_PERI_REG_BITS(SPI_USER1(SPI), SPI_USR_ADDR_BITLEN, 0, SPI_USR_ADDR_BITLEN_S);
    }

    uint32_t *value = p_cmd->data;
    if (((mode & SPI_TX) || (mode & SPI_WRSR)) && p_cmd->data_len != 0) {
        //Enable MOSI, disable MISO
        SET_PERI_REG_MASK(SPI_USER(SPI), SPI_USR_MOSI);
        CLEAR_PERI_REG_MASK(SPI_USER(SPI), SPI_USR_MISO);
        do {
            WRITE_PERI_REG((SPI_W0(SPI) + (idx << 2)), *value++);
        } while ( ++idx < (p_cmd->data_len / 4));
        SET_PERI_REG_BITS(SPI_USER1(SPI), SPI_USR_MOSI_BITLEN, ((p_cmd->data_len) * 8 - 1), SPI_USR_MOSI_BITLEN_S);

    } else if ((mode & SPI_RX) && p_cmd->data_len != 0) {
        //Enable MISO, disable MOSI
        CLEAR_PERI_REG_MASK(SPI_USER(SPI), SPI_USR_MOSI);
        SET_PERI_REG_MASK(SPI_USER(SPI), SPI_USR_MISO);
        SET_PERI_REG_BITS(SPI_USER1(SPI),SPI_USR_MISO_BITLEN, p_cmd->data_len * 8 - 1, SPI_USR_MISO_BITLEN_S);
        int fifo_idx = 0;
        do {
            WRITE_PERI_REG((SPI_W0(SPI) + (fifo_idx << 2)), 0);
        } while ( ++fifo_idx < (p_cmd->data_len / 4));
    } else {
        CLEAR_PERI_REG_MASK(SPI_USER(SPI), SPI_USR_MOSI);
        CLEAR_PERI_REG_MASK(SPI_USER(SPI), SPI_USR_MISO);
        SET_PERI_REG_BITS(SPI_USER1(SPI), SPI_USR_MISO_BITLEN, 0, SPI_USR_MISO_BITLEN_S);
        SET_PERI_REG_BITS(SPI_USER1(SPI), SPI_USR_MOSI_BITLEN, 0, SPI_USR_MOSI_BITLEN_S);
    }

    //Start command
    SET_PERI_REG_MASK(SPI_CMD(SPI), SPI_USR);
    while (READ_PERI_REG(SPI_CMD(SPI)) & SPI_USR);

    if (mode & SPI_RX) {
        do {
            *p_cmd->data ++ = READ_PERI_REG(SPI_W0(SPI) + (idx << 2));
        } while (++idx < (p_cmd->data_len / 4));
    }

    //recover
    WRITE_PERI_REG(PERIPHS_IO_MUX_CONF_U,io_mux_reg);
    WRITE_PERI_REG(SPI_CTRL(SPI),spi_ctrl_reg);
    WRITE_PERI_REG(SPI_CLOCK(SPI),spi_clk_reg);
    WRITE_PERI_REG(SPI_USER(SPI),spi_user_reg);

    if((mode & SPI_RAW) == 0) {
        Wait_SPI_Idle(&flashchip);
    }
    //enable icache
    Cache_Read_Enable_2();

    FlashIsOnGoing = 0;
    FLASH_INTR_UNLOCK(c_tmp);
    return true;
}

void user_spi_flash_dio_to_qio_pre_init(void)
{
    uint32_t flash_id = spi_flash_get_id();
    bool to_qio = false;
    //check for EN25Q16A/B flash chips
    if ((flash_id & 0xffffff) == 0x15701c) {
        uint8_t sfdp = en25q16x_read_sfdp();
        if (sfdp == 0xE5) {
            //This is EN25Q16A, set bit6 in the same way as issi flash chips.
            if (spi_flash_issi_enable_QIO_mode() == true) {
                to_qio = true;
            }
        } else if (sfdp == 0xED) {
            //This is EN25Q16B
            if (en25q16x_write_volatile_status(0x40) == true) {
                to_qio = true;
            }
        }
    }
    // ISSI : 0x9D
    // MXIC : 0xC2
    // GD25Q32C & GD25Q128C : 0x1640C8
    // EON : 0X1C
    // ENABLE FLASH QIO 0X01H+BIT6
    else if (((flash_id & 0xFF) == 0x9D) || ((flash_id & 0xFF) == 0xC2) || ((flash_id & 0xFF) == 0x1C) ) {
        if (spi_flash_issi_enable_QIO_mode() == true) {
            to_qio = true;
        }
        //ENABLE FLASH QIO 0X31H+BIT2
    } else if (((flash_id & 0xFFFFFF) == 0x1640C8) || ((flash_id & 0xFFFFFF) == 0x1840C8)) {
        if (flash_gd25q32c_enable_QIO_mode() == true) {
            to_qio = true;
        }
        //ENBALE FLASH QIO 0X01H+0X00+0X02
    } else {
        if (spi_flash_enable_qmode() == ESP_OK) {
            to_qio = true;
        }
    }

    if (to_qio == true) {
        CLEAR_PERI_REG_MASK(PERIPHS_SPI_FLASH_CTRL, SPI_QIO_MODE
                            |SPI_QOUT_MODE
                            |SPI_DIO_MODE
                            |SPI_DOUT_MODE
                            |SPI_FASTRD_MODE);
        SET_PERI_REG_MASK(PERIPHS_SPI_FLASH_CTRL, SPI_QIO_MODE | SPI_FASTRD_MODE);
    }
}

/**
 * @brief  Erase a range of flash sectors
 */
esp_err_t IRAM_ATTR spi_flash_erase_range(size_t start_address, size_t size)
{
    esp_err_t ret;
    size_t sec, num;

    if (start_address % SPI_FLASH_SEC_SIZE
            || size % SPI_FLASH_SEC_SIZE) {
        return ESP_ERR_FLASH_OP_FAIL;
    }

    if (spi_flash_check_wr_protect() == false) {
        return ESP_ERR_FLASH_OP_FAIL;
    }

    sec = start_address / SPI_FLASH_SEC_SIZE;
    num = size / SPI_FLASH_SEC_SIZE;

    /*
     * call "spi_flash_erase_sector" continuely to make the function to be able
     * to enter/exit critical state so that system core can feed watch
     */
    do {
        ret = spi_flash_erase_sector(sec++);

        pp_soft_wdt_feed();
    } while (ret == ESP_OK && --num);

    return ret;
}
