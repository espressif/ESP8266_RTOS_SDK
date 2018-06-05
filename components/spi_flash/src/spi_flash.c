/*
 * Copyright (c) 2013-2016 Espressif System
 */

#include "ets_sys.h"
#include "spi_flash.h"
#include "spi_register.h"

#include "esp_wifi_osi.h"
//#define SPI_DEBUG

#ifdef SPI_DEBUG
#define spi_debug printf
#else
#define spi_debug(...)
#endif

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

bool IRAM_FUNC_ATTR spi_user_cmd(spi_cmd_dir_t mode, spi_cmd_t *p_cmd);
bool special_flash_read_status(uint8_t command, uint32_t* status, int len);
bool special_flash_write_status(uint8_t command, uint32_t status, int len, bool write_en);
uint8_t en25q16x_read_sfdp();

extern void pp_soft_wdt_stop(void);
extern void pp_soft_wdt_restart(void);
extern bool protect_flag ;
SpiFlashChip flashchip = {
    0x1640ef,
    (32 / 8) * 1024 * 1024,
    64 * 1024,
    4 * 1024,
    256,
    0xffff
};

uint8 FlashIsOnGoing = 0;

typedef SpiFlashOpResult (*user_spi_flash_read)(
        SpiFlashChip *spi,
        uint32 flash_addr,
        uint32 *addr_dest,
        uint32 byte_length);

user_spi_flash_read flash_read;

#ifdef FOR_BAOFENG
#define FLASH_INTR_LOCK()	\
	do{		\
		vTaskSuspendAll();	\
	}while(0)
#define FLASH_INTR_UNLOCK()	\
	do{		\
		xTaskResumeAll();	\
	}while(0)
#else
#define FLASH_INTR_DECLARE(t)   uint32_t t
#define FLASH_INTR_LOCK(t)      wifi_enter_critical(t)
#define FLASH_INTR_UNLOCK(t)    wifi_exit_critical(t)
#endif

#if 0
LOCAL SpiFlashOpResult SPI_sector_erase(SpiFlashChip *spi, uint32 addr);
LOCAL SpiFlashOpResult SPI_page_program(SpiFlashChip *spi, uint32 spi_addr, uint32 *addr_source,
                                        sint32 byte_length);
LOCAL SpiFlashOpResult SPI_read_data(SpiFlashChip *spi, uint32 flash_addr, uint32 *addr_dest,
                                     sint32 byte_length);
LOCAL SpiFlashOpResult SPI_read_status(SpiFlashChip *spi, uint32 *status);
LOCAL SpiFlashOpResult SPI_write_status(SpiFlashChip *spi, uint32 status_value);
LOCAL SpiFlashOpResult SPI_write_enable(SpiFlashChip *spi);

LOCAL SpiFlashOpResult Enable_QMode(SpiFlashChip *spi) ;
LOCAL SpiFlashOpResult Disable_QMode(SpiFlashChip *spi) ;

LOCAL SpiFlashOpResult Wait_SPI_Idle(SpiFlashChip *spi);

LOCAL SpiFlashOpResult IRAM_FUNC_ATTR
SPI_sector_erase(SpiFlashChip *spi, uint32 addr)
{
    //check if addr is 4k alignment
    if (0 != (addr & 0xfff)) {
        return SPI_FLASH_RESULT_ERR;
    }

    Wait_SPI_Idle(spi);

    // sector erase  4Kbytes erase is sector erase.
    WRITE_PERI_REG(PERIPHS_SPI_FLASH_ADDR, addr & 0xffffff);
    WRITE_PERI_REG(PERIPHS_SPI_FLASH_CMD, SPI_FLASH_SE);

    while (READ_PERI_REG(PERIPHS_SPI_FLASH_CMD) != 0);

    Wait_SPI_Idle(spi);

    return SPI_FLASH_RESULT_OK;
}

LOCAL SpiFlashOpResult IRAM_FUNC_ATTR
SPI_page_program(SpiFlashChip *spi, uint32 spi_addr, uint32 *addr_source, sint32 byte_length)
{
    uint32  temp_addr;
    sint32  temp_bl;
    uint8   i;
    uint8   remain_word_num;

    //check 4byte alignment
    if (0 != (byte_length & 0x3)) {
        return SPI_FLASH_RESULT_ERR;
    }

    //check if write in one page
    if ((spi->page_size) < ((spi_addr % (spi->page_size)) + byte_length)) {
        return SPI_FLASH_RESULT_ERR;
    }

    Wait_SPI_Idle(spi);

    temp_addr = spi_addr;
    temp_bl = byte_length;

    while (temp_bl > 0) {
        if (temp_bl >= SPI_BUFF_BYTE_NUM) {
            WRITE_PERI_REG(PERIPHS_SPI_FLASH_ADDR, (temp_addr & 0xffffff) | (SPI_BUFF_BYTE_NUM << SPI_FLASH_BYTES_LEN)); // 32 byte a block

            for (i = 0; i < (SPI_BUFF_BYTE_NUM >> 2); i++) {
                WRITE_PERI_REG(PERIPHS_SPI_FLASH_C0 + i * 4, *addr_source++);
            }

            temp_bl = temp_bl - 32;
            temp_addr = temp_addr + 32;
        } else {
            WRITE_PERI_REG(PERIPHS_SPI_FLASH_ADDR, (temp_addr & 0xffffff) | (temp_bl << SPI_FLASH_BYTES_LEN));

            remain_word_num = (0 == (temp_bl & 0x3)) ? (temp_bl >> 2) : (temp_bl >> 2) + 1;

            for (i = 0; i < remain_word_num; i++) {
                WRITE_PERI_REG(PERIPHS_SPI_FLASH_C0 + i * 4, *addr_source++);
                temp_bl = temp_bl - 4;
            }

            temp_bl = 0;
        }

        if (SPI_FLASH_RESULT_OK != SPI_write_enable(spi)) {
            return SPI_FLASH_RESULT_ERR;
        }

        WRITE_PERI_REG(PERIPHS_SPI_FLASH_CMD, SPI_FLASH_PP);

        while (READ_PERI_REG(PERIPHS_SPI_FLASH_CMD) != 0);

        Wait_SPI_Idle(spi);
    }

    return SPI_FLASH_RESULT_OK;
}

LOCAL SpiFlashOpResult IRAM_FUNC_ATTR
SPI_read_data(SpiFlashChip *spi, uint32 flash_addr, uint32 *addr_dest,
              sint32 byte_length)
{
    uint32  temp_addr;
    sint32  temp_length;
    uint8   i;
    uint8   remain_word_num;

    //address range check
    if ((flash_addr + byte_length) > (spi->chip_size)) {
        return SPI_FLASH_RESULT_ERR;
    }

    temp_addr = flash_addr;
    temp_length = byte_length;

    Wait_SPI_Idle(spi);


    while (temp_length > 0) {
        if (temp_length >= SPI_BUFF_BYTE_NUM) {
            WRITE_PERI_REG(PERIPHS_SPI_FLASH_ADDR, temp_addr | (SPI_BUFF_BYTE_NUM << SPI_FLASH_BYTES_LEN));
            WRITE_PERI_REG(PERIPHS_SPI_FLASH_CMD, SPI_FLASH_READ);

            while (READ_PERI_REG(PERIPHS_SPI_FLASH_CMD) != 0);

            for (i = 0; i < (SPI_BUFF_BYTE_NUM >> 2); i++) {
                *addr_dest++ = READ_PERI_REG(PERIPHS_SPI_FLASH_C0 + i * 4);
            }

            temp_length = temp_length - SPI_BUFF_BYTE_NUM;
            temp_addr = temp_addr + SPI_BUFF_BYTE_NUM;
        } else {
            WRITE_PERI_REG(PERIPHS_SPI_FLASH_ADDR, temp_addr | (temp_length << SPI_FLASH_BYTES_LEN));
            WRITE_PERI_REG(PERIPHS_SPI_FLASH_CMD, SPI_FLASH_READ);

            while (READ_PERI_REG(PERIPHS_SPI_FLASH_CMD) != 0);

            remain_word_num = (0 == (temp_length & 0x3)) ? (temp_length >> 2) : (temp_length >> 2) + 1;

            for (i = 0; i < remain_word_num; i++) {
                *addr_dest++ = READ_PERI_REG(PERIPHS_SPI_FLASH_C0 + i * 4);
            }

            temp_length = 0;
        }
    }

    return SPI_FLASH_RESULT_OK;
}

LOCAL SpiFlashOpResult IRAM_FUNC_ATTR
SPI_read_status(SpiFlashChip *spi, uint32 *status)
{
    uint32 status_value = SPI_FLASH_BUSY_FLAG;

    while (SPI_FLASH_BUSY_FLAG == (status_value & SPI_FLASH_BUSY_FLAG)) {
        WRITE_PERI_REG(PERIPHS_SPI_FLASH_STATUS, 0);       // clear regisrter
        WRITE_PERI_REG(PERIPHS_SPI_FLASH_CMD, SPI_FLASH_RDSR);

        while (READ_PERI_REG(PERIPHS_SPI_FLASH_CMD) != 0);

        status_value  = READ_PERI_REG(PERIPHS_SPI_FLASH_STATUS) & (spi->status_mask);
    }

    *status = status_value;

    return SPI_FLASH_RESULT_OK;
}

LOCAL SpiFlashOpResult IRAM_FUNC_ATTR
SPI_write_status(SpiFlashChip *spi, uint32 status_value)
{
    Wait_SPI_Idle(spi);

    // update status value by status_value
    WRITE_PERI_REG(PERIPHS_SPI_FLASH_STATUS, status_value);    // write status regisrter
    WRITE_PERI_REG(PERIPHS_SPI_FLASH_CMD, SPI_FLASH_WRSR);

    while (READ_PERI_REG(PERIPHS_SPI_FLASH_CMD) != 0);

    return SPI_FLASH_RESULT_OK;
}

LOCAL SpiFlashOpResult IRAM_FUNC_ATTR
SPI_write_enable(SpiFlashChip *spi)
{
    uint32 flash_status = 0;

    Wait_SPI_Idle(spi);

    //enable write
    WRITE_PERI_REG(PERIPHS_SPI_FLASH_CMD, SPI_FLASH_WREN);     // enable write operation

    while (READ_PERI_REG(PERIPHS_SPI_FLASH_CMD) != 0);

    // make sure the flash is ready for writing
    while (SPI_FLASH_WRENABLE_FLAG != (flash_status & SPI_FLASH_WRENABLE_FLAG)) {
        SPI_read_status(spi, &flash_status);
    }

    return SPI_FLASH_RESULT_OK;
}

//**********************************************************************************
//Function Name: Wait_SPI_Idle
//Input Param:void
//Output Param:void
//Return: void
//Description:wait for spi ready
//
//**********************************************************************************
LOCAL SpiFlashOpResult IRAM_FUNC_ATTR
Wait_SPI_Idle(SpiFlashChip *spi)
{
    uint32 status;

    //wait for spi control ready
    while (GET_PERI_REG_BITS(CACHE_FLASH_CTRL_REG, FLASH_CTRL_BUSY_BIT, FLASH_CTRL_BUSY_BIT)) {
    }

    //wait for flash status ready
    if (SPI_FLASH_RESULT_OK != SPI_read_status(spi, &status)) {
        return SPI_FLASH_RESULT_ERR;
    }

    return  SPI_FLASH_RESULT_OK;
}

LOCAL SpiFlashOpResult IRAM_FUNC_ATTR
Enable_QMode(SpiFlashChip *spi)
{
    //enable 2 byte status writing
    SET_PERI_REG_MASK(PERIPHS_SPI_FLASH_CTRL, TWO_BYTE_STATUS_EN);

    if (SPI_FLASH_RESULT_OK != SPI_write_enable(spi)) {
        return SPI_FLASH_RESULT_ERR;
    }

    if (SPI_FLASH_RESULT_OK != SPI_write_status(spi, 0x0200)) {
        return SPI_FLASH_RESULT_ERR;
    }

    return SPI_FLASH_RESULT_OK;
}

LOCAL SpiFlashOpResult IRAM_FUNC_ATTR
Disable_QMode(SpiFlashChip *spi)
{
    uint32 flash_status;

    //enable 2 byte status writing
    SET_PERI_REG_MASK(PERIPHS_SPI_FLASH_CTRL, TWO_BYTE_STATUS_EN);

    if (SPI_FLASH_RESULT_OK != SPI_write_enable(spi)) {
        return SPI_FLASH_RESULT_ERR;
    }

    SPI_read_status(spi, &flash_status);

    //keep low 8 bit
    if (SPI_FLASH_RESULT_OK != SPI_write_status(spi, flash_status & 0xff)) {
        return SPI_FLASH_RESULT_ERR;
    }

    return SPI_FLASH_RESULT_OK;
}

SpiFlashOpResult IRAM_FUNC_ATTR
SPIReadModeCnfig(SpiFlashRdMode mode)
{
    uint32  modebit;

    //clear old mode bit
    CLEAR_PERI_REG_MASK(PERIPHS_SPI_FLASH_CTRL, SPI_QIO_MODE
                        | SPI_QOUT_MODE
                        | SPI_DIO_MODE
                        | SPI_DOUT_MODE
                        | SPI_FASTRD_MODE);

    //configure read mode
    switch (mode) {
        case SPI_FLASH_QIO_MODE        :
            modebit = SPI_QIO_MODE | SPI_FASTRD_MODE;
            break;

        case SPI_FLASH_QOUT_MODE     :
            modebit = SPI_QOUT_MODE | SPI_FASTRD_MODE;
            break;

        case SPI_FLASH_DIO_MODE        :
            modebit = SPI_DIO_MODE | SPI_FASTRD_MODE;
            break;

        case SPI_FLASH_DOUT_MODE     :
            modebit = SPI_DOUT_MODE | SPI_FASTRD_MODE;
            break;

        case SPI_FLASH_FASTRD_MODE  :
            modebit = SPI_FASTRD_MODE;
            break;

        case SPI_FLASH_SLOWRD_MODE :
            modebit = 0;
            break;

        default :
            modebit = 0;
            break;
    }

    if ((SPI_FLASH_QIO_MODE == mode) || (SPI_FLASH_QOUT_MODE == mode)) {
        Enable_QMode(&flashchip);
    } else {
        Disable_QMode(&flashchip);
    }

    SET_PERI_REG_MASK(PERIPHS_SPI_FLASH_CTRL, modebit);

    return  SPI_FLASH_RESULT_OK;
}
#endif

SpiFlashOpResult IRAM_FUNC_ATTR
SPIWrite(uint32  target, uint32 *src_addr, sint32 len)
{
    uint32  page_size;
    uint32  pgm_len, pgm_num;
    uint8    i;

    //check program size
    if ((target + len) > (flashchip.chip_size)) {
        return SPI_FLASH_RESULT_ERR;
    }

#if 0
    if (SPI_FLASH_RESULT_OK != SPI_write_enable(&flashchip)) {
		return SPI_FLASH_RESULT_ERR;
	}
#endif

    page_size = flashchip.page_size;
    pgm_len = page_size - (target % page_size);

    if (len < pgm_len) {
        if (SPI_FLASH_RESULT_OK != SPI_page_program(&flashchip,  target, src_addr, len)) {
            return SPI_FLASH_RESULT_ERR;
        }
    } else {
        if (SPI_FLASH_RESULT_OK != SPI_page_program(&flashchip,  target, src_addr, pgm_len)) {
            return SPI_FLASH_RESULT_ERR;
        }

        //whole page program
        pgm_num = (len - pgm_len) / page_size;

        for (i = 0; i < pgm_num; i++) {
            if (SPI_FLASH_RESULT_OK != SPI_page_program(&flashchip,  target + pgm_len, src_addr + (pgm_len >> 2), page_size)) {
                return SPI_FLASH_RESULT_ERR;
            }

            pgm_len += page_size;
        }

        //remain parts to program
        if (SPI_FLASH_RESULT_OK != SPI_page_program(&flashchip,  target + pgm_len, src_addr + (pgm_len >> 2), len - pgm_len)) {
            return SPI_FLASH_RESULT_ERR;
        }
    }

    return  SPI_FLASH_RESULT_OK;
}

SpiFlashOpResult IRAM_FUNC_ATTR
SPIRead(uint32 target, uint32 *dest_addr, sint32 len)
{
#if 0
	if (SPI_FLASH_RESULT_OK != SPI_write_enable(&flashchip)) {
		return SPI_FLASH_RESULT_ERR;
	}
#endif

    if (SPI_FLASH_RESULT_OK != SPI_read_data(&flashchip, target, dest_addr, len)) {
        return SPI_FLASH_RESULT_ERR;
    }

    return SPI_FLASH_RESULT_OK;
}

SpiFlashOpResult IRAM_FUNC_ATTR
SPIEraseSector(uint32 sector_num)
{
    if (sector_num >= ((flashchip.chip_size) / (flashchip.sector_size))) {
        return SPI_FLASH_RESULT_ERR;
    }

    if (SPI_FLASH_RESULT_OK != SPI_write_enable(&flashchip)) {
        return SPI_FLASH_RESULT_ERR;
    }

    if (SPI_FLASH_RESULT_OK != SPI_sector_erase(&flashchip, sector_num * (flashchip.sector_size))) {
        return SPI_FLASH_RESULT_ERR;
    }

    return SPI_FLASH_RESULT_OK;
}


#if 1
void IRAM_FUNC_ATTR Cache_Read_Disable_2(void)
{
	CLEAR_PERI_REG_MASK(CACHE_FLASH_CTRL_REG,CACHE_READ_EN_BIT);
//ets_delay_us(10);
	while(REG_READ(SPI_EXT2(0)) != 0){}
	CLEAR_PERI_REG_MASK(PERIPHS_SPI_FLASH_CTRL,SPI_ENABLE_AHB);
}
void IRAM_FUNC_ATTR Cache_Read_Enable_2()
{
	SET_PERI_REG_MASK(PERIPHS_SPI_FLASH_CTRL,SPI_ENABLE_AHB);
	SET_PERI_REG_MASK(CACHE_FLASH_CTRL_REG,CACHE_READ_EN_BIT);
}
#endif


#define SPI 0

//debug
#define USER_SPI_FLASH_DBG ets_printf

//flash cmd
#define SPI_FLASH_READ_UNIQUE_CMD 0x4B
#define SPI_FLASH_WRITE_STATUS_REGISTER 0X01
#define SPI_FLASH_ISSI_ENABLE_QIO_MODE (BIT(6))

//#define SPI_ENABLE_QIO_MODE (0)
/*gd25q32c*/
#define SPI_FLASH_GD25Q32C_WRITE_STATUSE1_CMD (0X01)
#define SPI_FLASH_GD25Q32C_WRITE_STATUSE2_CMD (0X31)
#define SPI_FLASH_GD25Q32C_WRITE_STATUSE3_CMD (0X11)

#define SPI_FLASH_GD25Q32C_READ_STATUSE1_CMD (0X05)
#define SPI_FLASH_GD25Q32C_READ_STATUSE2_CMD (0X35)
#define SPI_FLASH_GD25Q32C_READ_STATUSE3_CMD (0X15)

#define  SPI_FLASH_GD25Q32C_QIO_MODE (BIT(1))

enum GD25Q32C_status{
    GD25Q32C_STATUS1=0,
    GD25Q32C_STATUS2,
    GD25Q32C_STATUS3,
};

bool spi_flash_get_unique_id(uint8 *id)
{
    FLASH_INTR_DECLARE(c_tmp);

	FLASH_INTR_LOCK(c_tmp);
    //disable icache
    Cache_Read_Disable_2();
    uint32 status;
    //wait spi idle
    Wait_SPI_Idle(&flashchip);

    //save reg
    uint32 io_mux_reg = READ_PERI_REG(PERIPHS_IO_MUX_CONF_U);
    uint32 spi_clk_reg = READ_PERI_REG(SPI_CLOCK(SPI));
    uint32 spi_ctrl_reg = READ_PERI_REG(SPI_CTRL(SPI));
    uint32 spi_user_reg = READ_PERI_REG(SPI_USER(SPI));

    CLEAR_PERI_REG_MASK(PERIPHS_IO_MUX_CONF_U,SPI0_CLK_EQU_SYS_CLK);

    SET_PERI_REG_MASK(SPI_USER(SPI), SPI_CS_SETUP|SPI_CS_HOLD|SPI_USR_COMMAND|SPI_USR_MOSI);
    CLEAR_PERI_REG_MASK(SPI_USER(SPI), SPI_FLASH_MODE);
    //clear Daul or Quad lines transmission mode
    CLEAR_PERI_REG_MASK(SPI_CTRL(SPI), SPI_QIO_MODE|SPI_DIO_MODE|SPI_DOUT_MODE|SPI_QOUT_MODE);
    WRITE_PERI_REG(SPI_CLOCK(SPI),
                    ((3&SPI_CLKCNT_N)<<SPI_CLKCNT_N_S)|
                    ((1&SPI_CLKCNT_H)<<SPI_CLKCNT_H_S)|
                    ((3&SPI_CLKCNT_L)<<SPI_CLKCNT_L_S)); //clear bit 31,set SPI clock div

    WRITE_PERI_REG(SPI_ADDR(SPI), 0x00);
    WRITE_PERI_REG(SPI_USER2(SPI), 0x70000000|SPI_FLASH_READ_UNIQUE_CMD);
    CLEAR_PERI_REG_MASK(SPI_USER(SPI), SPI_USR_MOSI);//
    SET_PERI_REG_MASK(SPI_USER(SPI), SPI_USR_MISO);//read
    SET_PERI_REG_MASK(SPI_USER(SPI), SPI_USR_DUMMY);
    CLEAR_PERI_REG_MASK(SPI_USER(SPI), SPI_USR_ADDR);

    // 1 byte CMD(MOSI) + 4 BYTE DUMMY  + 8 BYTE DATA(MISO)
    WRITE_PERI_REG(SPI_USER1(SPI), ((0&SPI_USR_ADDR_BITLEN)<<SPI_USR_ADDR_BITLEN_S)
                                  |((63&SPI_USR_MISO_BITLEN)<<SPI_USR_MISO_BITLEN_S)
                                  |((31&SPI_USR_DUMMY_CYCLELEN)<<SPI_USR_DUMMY_CYCLELEN_S));
    WRITE_PERI_REG(SPI_W0(SPI), 0x00);
    WRITE_PERI_REG(SPI_W1(SPI), 0x00);
    SET_PERI_REG_MASK(SPI_CMD(SPI), SPI_USR);
    while(READ_PERI_REG(SPI_CMD(SPI))&SPI_USR);

    //uint8 id[8] = {0};
    uint32 data = 0 ;
    data=READ_PERI_REG(SPI_W0(SPI));
    *(id+0)=(uint8)(data&0xff);
    *(id+1)=(uint8)((data>>8)&0xff);
    *(id+2)=(uint8)((data>>16)&0xff);
    *(id+3)=(uint8)((data>>24)&0xff);

    data=READ_PERI_REG(SPI_W1(SPI));
    *(id+4)=(uint8)(data&0xff);
    *(id+5)=(uint8)((data>>8)&0xff);
    *(id+6)=(uint8)((data>>16)&0xff);
    *(id+7)=(uint8)((data>>24)&0xff);


    //recover
    WRITE_PERI_REG(PERIPHS_IO_MUX_CONF_U,io_mux_reg);
    WRITE_PERI_REG(SPI_CTRL(SPI),spi_ctrl_reg);
    WRITE_PERI_REG(SPI_CLOCK(SPI),spi_clk_reg);
    WRITE_PERI_REG(SPI_USER(SPI),spi_user_reg);
    //wait spi idle
    Wait_SPI_Idle(&flashchip);
    //enable icache
    Cache_Read_Enable_2();
    FLASH_INTR_UNLOCK(c_tmp);
    return true;
}

uint32 IRAM_FUNC_ATTR
spi_flash_get_id(void)
{
	uint32 rdid = 0;
	FLASH_INTR_DECLARE(c_tmp);
	//taskENTER_CRITICAL();
	FLASH_INTR_LOCK(c_tmp);
//pp_soft_wdt_stop();os_printf("stop2\n");
FlashIsOnGoing = 1;

	Cache_Read_Disable();

	Wait_SPI_Idle(&flashchip);

	WRITE_PERI_REG(PERIPHS_SPI_FLASH_C0, 0);    // clear regisrter
	WRITE_PERI_REG(PERIPHS_SPI_FLASH_CMD, SPI_FLASH_RDID);
	while(READ_PERI_REG(PERIPHS_SPI_FLASH_CMD)!=0);

	rdid = READ_PERI_REG(PERIPHS_SPI_FLASH_C0)&0xffffff;

	Cache_Read_Enable_New();

FlashIsOnGoing = 0;
//os_printf("start2\n");
//pp_soft_wdt_restart();
//	taskEXIT_CRITICAL();
	FLASH_INTR_UNLOCK(c_tmp);

	return rdid;
}

SpiFlashOpResult IRAM_FUNC_ATTR
spi_flash_read_status(uint32 *status)
{
	SpiFlashOpResult ret;
	FLASH_INTR_DECLARE(c_tmp);

	//taskENTER_CRITICAL();
	FLASH_INTR_LOCK(c_tmp);

//pp_soft_wdt_stop();os_printf("stop3\n");
FlashIsOnGoing = 1;
	//Cache_Read_Disable();
Cache_Read_Disable_2();

	ret =  SPI_read_status(&flashchip, status);

	//Cache_Read_Enable_New();
Cache_Read_Enable_2();
FlashIsOnGoing = 0;
//pp_soft_wdt_restart();os_printf("start3\n");
//	taskEXIT_CRITICAL();
	FLASH_INTR_UNLOCK(c_tmp);

	return ret;
}

SpiFlashOpResult IRAM_FUNC_ATTR
spi_flash_write_status(uint32 status_value)
{
	//taskENTER_CRITICAL();
    FLASH_INTR_DECLARE(c_tmp);

	FLASH_INTR_LOCK(c_tmp);

//pp_soft_wdt_stop();os_printf("stop4\n");
FlashIsOnGoing = 1;
	//Cache_Read_Disable();
Cache_Read_Disable_2();

	Wait_SPI_Idle(&flashchip);
	if(SPI_FLASH_RESULT_OK != SPI_write_enable(&flashchip))
		return SPI_FLASH_RESULT_ERR;
	if(SPI_FLASH_RESULT_OK != SPI_write_status(&flashchip,status_value))
		return SPI_FLASH_RESULT_ERR;
	Wait_SPI_Idle(&flashchip);

	//Cache_Read_Enable_New();
Cache_Read_Enable_2();
FlashIsOnGoing = 0;
//pp_soft_wdt_restart();os_printf("start4\n");
//	taskEXIT_CRITICAL();
	FLASH_INTR_UNLOCK(c_tmp);

	return SPI_FLASH_RESULT_OK;
}

#define SPI_ISSI_FLASH_WRITE_PROTECT_STATUS      (BIT(2)|BIT(3)|BIT(4)|BIT(5))
#define SPI_EON_25Q16A_WRITE_PROTECT_STATUS       (BIT(2)|BIT(3)|BIT(4)|BIT(5))
#define SPI_EON_25Q16B_WRITE_PROTECT_STATUS       (BIT(2)|BIT(3)|BIT(4)|BIT(5))
#define SPI_GD25Q32_FLASH_WRITE_PROTECT_STATUS   (BIT(2)|BIT(3)|BIT(4)|BIT(5)|BIT(6))
#define SPI_FLASH_RDSR2      0x35
#define SPI_FLASH_PROTECT_STATUS                 (BIT(2)|BIT(3)|BIT(4)|BIT(5)|BIT(6)|BIT(14))

LOCAL uint8 IRAM_FUNC_ATTR
flash_gd25q32c_read_status(enum GD25Q32C_status status_index)
{
    uint8_t rdsr_cmd=0;
    if(GD25Q32C_STATUS1 == status_index){
        rdsr_cmd = SPI_FLASH_GD25Q32C_READ_STATUSE1_CMD;
    }
    else if(GD25Q32C_STATUS2 == status_index){
        rdsr_cmd = SPI_FLASH_GD25Q32C_READ_STATUSE2_CMD;
    }
    else if(GD25Q32C_STATUS3 == status_index){
        rdsr_cmd = SPI_FLASH_GD25Q32C_READ_STATUSE3_CMD;
    }
    else {

    }
    uint32_t status;
    special_flash_read_status(rdsr_cmd, &status, 1);
    return ((uint8_t)status);
}

void flash_gd25q32c_write_status(enum GD25Q32C_status status_index,uint8 status)
{
    uint32 wrsr_cmd=0;
    uint32 new_status = status;
    if(GD25Q32C_STATUS1 == status_index){
        wrsr_cmd = SPI_FLASH_GD25Q32C_WRITE_STATUSE1_CMD;
    }
    else if(GD25Q32C_STATUS2 == status_index){
        wrsr_cmd = SPI_FLASH_GD25Q32C_WRITE_STATUSE2_CMD;
    }
    else if(GD25Q32C_STATUS3 == status_index){
        wrsr_cmd = SPI_FLASH_GD25Q32C_WRITE_STATUSE3_CMD;
    }
    else {
        //ets_printf("[ERR]Not konw GD25Q32C status idx %d\n ",spi_wr_status_cmd);
    }
    special_flash_write_status(wrsr_cmd, new_status, 1, true);
}
LOCAL bool spi_flash_check_wr_protect(void)
{
    uint32 flash_id=spi_flash_get_id();
    uint32 status=0;
    //check for EN25Q16A/B flash chips
    if ((flash_id & 0xffffff) == 0x15701c) {
        uint8_t sfdp = en25q16x_read_sfdp();
        if (sfdp == 0xE5) {
            spi_debug("EN25Q16A\n");
            //This is EN25Q16A, set bit6 in the same way as issi flash chips.
            if(spi_flash_read_status(&status)==0){//Read status Ok
                if(status&(SPI_EON_25Q16A_WRITE_PROTECT_STATUS)){//Write_protect
                    special_flash_write_status(0x1, status&(~(SPI_EON_25Q16A_WRITE_PROTECT_STATUS)), 1, true);
                }
            }
        } else if (sfdp == 0xED) {
            spi_debug("EN25Q16B\n");
            //This is EN25Q16B
            if(spi_flash_read_status(&status)==0){//Read status Ok
                if(status&(SPI_EON_25Q16B_WRITE_PROTECT_STATUS)){//Write_protect
                    special_flash_write_status(0x1, status&(~(SPI_EON_25Q16B_WRITE_PROTECT_STATUS)), 1, true);
                }
            }
        }
    }
    //MXIC :0XC2
    //ISSI :0X9D
    // ets_printf("spi_flash_check_wr_protect\r\n");
    else if(((flash_id&0xFF)==0X9D)||((flash_id&0xFF)==0XC2)||((flash_id & 0xFF) == 0x1C)){
        if(spi_flash_read_status(&status)==0){//Read status Ok
            if(status&(SPI_ISSI_FLASH_WRITE_PROTECT_STATUS)){//Write_protect
                special_flash_write_status(0x1, status&(~(SPI_ISSI_FLASH_WRITE_PROTECT_STATUS)), 1, true);
            }
        }
    }
    //GD25Q32C:0X16409D
    //GD25Q128
    else if(((flash_id&0xFFFFFFFF)==0X1640C8)||((flash_id&0xFFFFFFFF)==0X1840C8)){
        if(spi_flash_read_status(&status)==0){//Read status Ok
            if(status&SPI_GD25Q32_FLASH_WRITE_PROTECT_STATUS){
                special_flash_write_status(0x01, status&(~(SPI_GD25Q32_FLASH_WRITE_PROTECT_STATUS)), 1, true);
            }
        }
    }
    //Others
    else{
         if(spi_flash_read_status(&status)==0){//Read status Ok
            uint32 status1 = 0; //flash_gd25q32c_read_status(GD25Q32C_STATUS2);
            special_flash_read_status(SPI_FLASH_RDSR2, &status1, 1);
            status=(status1 << 8)|(status & 0xff);
            if(status&SPI_FLASH_PROTECT_STATUS){
                status=((status&(~SPI_FLASH_PROTECT_STATUS))&0xffff);
                spi_flash_write_status(status);
            }
         }
    }
    return true;
}
#if 0
LOCAL bool spi_flash_check_wr_protect(void)
{
    uint32 status;

    if (spi_flash_read_status(&status) == SPI_FLASH_RESULT_OK) {
        if (status != 0) {
            os_printf("flash status %x\n", status);
#define SPI_FLASH_STATUS    0x00
            if (spi_flash_write_status(SPI_FLASH_STATUS) != SPI_FLASH_RESULT_OK) {
                os_printf("flash write status failed\n");
                return false;
            } else {
                if (spi_flash_read_status(&status) == SPI_FLASH_RESULT_OK) {
                    if ((status != 0 )&&(status != 0x02 )) {
                        os_printf("flash status check failed， status %x\n", status);
                        return false;
                    } else {
                        os_printf("flash status check ok\n");
                    }
                } else {
                    return false;
                }
            }
        }
    } else {
        return false;
    }

    return true;
}
#endif
/******************************************************************************
 * FunctionName : spi_flash_erase_sector
 * Description  : a
 * Parameters   :
 * Returns      :
*******************************************************************************/
SpiFlashOpResult IRAM_FUNC_ATTR
spi_flash_erase_sector(uint16 sec)
{
	if (protect_flag == true)
	{
		if (false == spi_flash_erase_sector_check(sec))
		    return SPI_FLASH_RESULT_ERR;
	}

    SpiFlashOpResult ret;

    if (spi_flash_check_wr_protect() == false) {
        return SPI_FLASH_RESULT_ERR;
    }

    spi_debug("E[%x] %d-", sec, system_get_time());

    FLASH_INTR_DECLARE(c_tmp);

//	vTaskSuspendAll();
//    taskENTER_CRITICAL();
	FLASH_INTR_LOCK(c_tmp);

pp_soft_wdt_stop();
FlashIsOnGoing = 1;
    //Cache_Read_Disable();
Cache_Read_Disable_2();

    ret = SPIEraseSector(sec);

Cache_Read_Enable_2();
	//Cache_Read_Enable_New();
FlashIsOnGoing = 0;
//    xTaskResumeAll();
pp_soft_wdt_restart();
//    taskEXIT_CRITICAL();
	FLASH_INTR_UNLOCK(c_tmp);

    spi_debug("%d\n", system_get_time());

    return ret;
}

SpiFlashOpResult spi_flash_enable_qmode(void)
{
    SpiFlashOpResult ret;

    Cache_Read_Disable_2();
    ret = Enable_QMode(&flashchip);
    Wait_SPI_Idle(&flashchip);
    Cache_Read_Enable_2();

    return ret;
}

SpiFlashOpResult IRAM_FUNC_ATTR
spi_flash_write(uint32 des_addr, uint32 *src_addr, uint32 size)
{
    SpiFlashOpResult ret;

    if (src_addr == NULL) {
        return SPI_FLASH_RESULT_ERR;
    }

    if (spi_flash_check_wr_protect() == false) {
        return SPI_FLASH_RESULT_ERR;
    }

    if (size % 4) {
        size = (size / 4 + 1) * 4;
    }

    spi_debug("W[%x] %d-", des_addr / 4096, system_get_time());

    FLASH_INTR_DECLARE(c_tmp);

//    vTaskSuspendAll();
//    taskENTER_CRITICAL();
	FLASH_INTR_LOCK(c_tmp);

pp_soft_wdt_stop();
FlashIsOnGoing = 1;
    //Cache_Read_Disable();
Cache_Read_Disable_2();

    ret = SPIWrite(des_addr, src_addr, size);

Cache_Read_Enable_2();
	//Cache_Read_Enable_New();
FlashIsOnGoing = 0;
//    xTaskResumeAll();
pp_soft_wdt_restart();
//    taskEXIT_CRITICAL();
	FLASH_INTR_UNLOCK(c_tmp);


    spi_debug("%d\n", system_get_time());

    return ret;
}

/******************************************************************************
 * FunctionName : spi_flash_read
 * Description  : a
 * Parameters   :
 * Returns      :
*******************************************************************************/
SpiFlashOpResult IRAM_FUNC_ATTR
spi_flash_read(uint32 src_addr, uint32 *des_addr, uint32 size)
{
    SpiFlashOpResult ret;

    if (des_addr == NULL) {
        return SPI_FLASH_RESULT_ERR;
    }

    spi_debug("R[%x] %d-", src_addr / 4096, system_get_time());

    FLASH_INTR_DECLARE(c_tmp);

//    vTaskSuspendAll();
//    taskENTER_CRITICAL();
	FLASH_INTR_LOCK(c_tmp);

//pp_soft_wdt_stop();os_printf("stop7\n");
FlashIsOnGoing = 1;

    if (flash_read == NULL) {
	//Cache_Read_Disable();
Cache_Read_Disable_2();

    ret = SPIRead(src_addr, des_addr, size);

Cache_Read_Enable_2();
	//Cache_Read_Enable_New();
    } else {
        ret = flash_read(&flashchip, src_addr, des_addr, size);
    }
FlashIsOnGoing = 0;
//    xTaskResumeAll();
//pp_soft_wdt_restart();os_printf("start7\n");
    //taskEXIT_CRITICAL();
	FLASH_INTR_UNLOCK(c_tmp);

    spi_debug("%d\n", system_get_time());

    return ret;
}

void ICACHE_FLASH_ATTR
spi_flash_set_read_func(user_spi_flash_read read)
{
    flash_read = read;
}

LOCAL void spi_flash_enable_qio_bit6(void)
{
    uint8_t wrsr_cmd = 0x1;
    uint32_t issi_qio = SPI_FLASH_ISSI_ENABLE_QIO_MODE;
    special_flash_write_status(wrsr_cmd, issi_qio, 1, true);
}

/*
if the QIO_ENABLE bit not in Bit9,in Bit6
the SPI send 0x01(CMD)+(1<<6)
*/
bool spi_flash_issi_enable_QIO_mode(void)
{
    uint32 status = 0;
    if(spi_flash_read_status(&status) == 0) {
        if((status&SPI_FLASH_ISSI_ENABLE_QIO_MODE)) {
            USER_SPI_FLASH_DBG("already:QIO!\n");
            return true;
        }
    }
    else{
    	//USER_SPI_FLASH_DBG("spi flash read status err %s %u\r\n ",__FUNCTION__,__LINE__);
    	return false;
    }

    spi_flash_enable_qio_bit6();

    if(spi_flash_read_status(&status) == 0){
        if((status&SPI_FLASH_ISSI_ENABLE_QIO_MODE)) {
        	USER_SPI_FLASH_DBG("mode:QIO.\n");
            //USER_SPI_FLASH_DBG("ENABLE QIO(0X1+(BIT6)),QIO ENABLE OK,!\n");
            return true;
        } else {
            //USER_SPI_FLASH_DBG("ENABLE QIO(0X1+(BIT6ENABLE QIO(0X31+(BIT2)))),QIO ENABLE ERR!!\n");
            return false;
        }
    } else {
       //USER_SPI_FLASH_DBG("spi flash read status err %s %u\r\n ",__FUNCTION__,__LINE__);
       return false;
    }
}

bool flash_gd25q32c_enable_QIO_mode()
{
    uint8 data = 0;
    if((data=flash_gd25q32c_read_status(GD25Q32C_STATUS2))&SPI_FLASH_GD25Q32C_QIO_MODE){
    	USER_SPI_FLASH_DBG("already:QIO!\n");
       return true;
    }
    else{
        flash_gd25q32c_write_status(GD25Q32C_STATUS2,SPI_FLASH_GD25Q32C_QIO_MODE);
        if(flash_gd25q32c_read_status(GD25Q32C_STATUS2)&SPI_FLASH_GD25Q32C_QIO_MODE){
        	USER_SPI_FLASH_DBG("mode:QIO.\n");
            return true;

        }
        else{
        	//USER_SPI_FLASH_DBG("ENABLE QIO(0X31+(BIT2)),QIO ENABLE Fail\n");
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

bool IRAM_FUNC_ATTR spi_user_cmd(spi_cmd_dir_t mode, spi_cmd_t *p_cmd)
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
    uint32 io_mux_reg = READ_PERI_REG(PERIPHS_IO_MUX_CONF_U);
    uint32 spi_clk_reg = READ_PERI_REG(SPI_CLOCK(SPI));
    uint32 spi_ctrl_reg = READ_PERI_REG(SPI_CTRL(SPI));
    uint32 spi_user_reg = READ_PERI_REG(SPI_USER(SPI));

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

void __attribute__((weak))
user_spi_flash_dio_to_qio_pre_init(void)
{
    uint32 flash_id = spi_flash_get_id();
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
                printf("mode: QIO\n");
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
        if (spi_flash_enable_qmode() == SPI_FLASH_RESULT_OK) {
            printf("mode: QIO\n");
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
