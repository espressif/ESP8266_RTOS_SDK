// Copyright 2018-2025 Espressif Systems (Shanghai) PTE LTD
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#include "driver/i2c.h"
#include "driver/gpio.h"

// Temporary use the FreeRTOS critical function
#include "FreeRTOS.h"
#define ENTER_CRITICAL() portENTER_CRITICAL()
#define EXIT_CRITICAL() portEXIT_CRITICAL()


static const char *I2C_TAG = "i2c";

#define I2C_CHECK(a, str, ret)  if(!(a)) {                                             \
        ESP_LOGE(I2C_TAG,"%s:%d (%s):%s", __FILE__, __LINE__, __FUNCTION__, str);      \
        return (ret);                                                                   \
    }

#define I2C_DRIVER_ERR_STR             "i2c driver install error"
#define I2C_DRIVER_MALLOC_ERR_STR      "i2c driver malloc error"
#define I2C_NUM_ERROR_STR              "i2c number error"
#define I2C_TIMEING_VAL_ERR_STR        "i2c timing value error"
#define I2C_ADDR_ERROR_STR             "i2c null address error"
#define I2C_DRIVER_NOT_INSTALL_ERR_STR "i2c driver not installed"
#define I2C_MASTER_MODE_ERR_STR        "Only allowed in master mode"
#define I2C_CMD_MALLOC_ERR_STR         "i2c command link malloc error"
#define I2C_TRANS_MODE_ERR_STR         "i2c trans mode error"
#define I2C_MODE_ERR_STR               "i2c mode error"
#define I2C_SDA_IO_ERR_STR             "sda gpio number error"
#define I2C_SCL_IO_ERR_STR             "scl gpio number error"
#define I2C_CMD_LINK_INIT_ERR_STR      "i2c command link error"
#define I2C_GPIO_PULLUP_ERR_STR        "this i2c pin does not support internal pull-up"
#define I2C_ACK_TYPE_ERR_STR           "i2c ack type error"
#define I2C_DATA_LEN_ERR_STR           "i2c data read length error"
#define I2C_IO_INIT_LEVEL              (1)
#define I2C_CMD_ALIVE_INTERVAL_TICK    (1000 / portTICK_PERIOD_MS)
#define I2C_ACKERR_CNT_MAX             (10)

#define i2c_master_wait    os_delay_us

typedef struct {
    uint8_t byte_num;     /*!< cmd byte number */
    struct {
        uint8_t en:  1;   /*!< ack check enable */
        uint8_t exp: 1;   /*!< expected ack level to get */
        uint8_t val: 1;   /*!< ack value to send */
    } ack;
    uint8_t *data;        /*!< data address */
    uint8_t byte_cmd;     /*!< to save cmd for one byte command mode */
    i2c_opmode_t op_code; /*!< cmd type */
} i2c_cmd_t;

typedef struct i2c_cmd_link {
    i2c_cmd_t cmd;              /*!< command in current cmd link */
    struct i2c_cmd_link *next;  /*!< next cmd link */
} i2c_cmd_link_t;

typedef struct {
    i2c_cmd_link_t *head;     /*!< head of the command link */
    i2c_cmd_link_t *cur;      /*!< last node of the command link */
    i2c_cmd_link_t *free;     /*!< the first node to free of the command link */
} i2c_cmd_desc_t;

typedef enum {
    I2C_STATUS_READ,      /*!< read status for current master command */
    I2C_STATUS_WRITE,     /*!< write status for current master command */
    I2C_STATUS_IDLE,      /*!< idle status for current master command */
    I2C_STATUS_ACK_ERROR, /*!< ack error status for current master command */
    I2C_STATUS_DONE,      /*!< I2C command done */
    I2C_STATUS_TIMEOUT,   /*!< I2C bus status error, and operation timeout */
} i2c_status_t;

typedef struct {
    i2c_port_t i2c_num;          /*!< I2C port number */
    i2c_mode_t mode;             /*!< I2C mode, master */
    int status;                  /*!< record current command status, for master mode */
    i2c_cmd_desc_t cmd_link;     /*!< I2C command link */
} i2c_obj_t;

typedef union {
    struct {
        uint8_t scl: 1;
        uint8_t sda: 1;
    };
    uint8_t val;
} i2c_last_state_t;

static i2c_obj_t *p_i2c_obj[I2C_NUM_MAX] = {0};
static i2c_config_t *i2c_config[I2C_NUM_MAX] = {NULL};
static i2c_last_state_t *i2c_last_state[I2C_NUM_MAX] = {NULL};

static inline void i2c_master_set_dc(i2c_port_t i2c_num, uint8_t sda, uint8_t scl)
{
    uint32_t i = 0;
    uint32_t clk_stretch_tick = i2c_config[i2c_num]->clk_stretch_tick;
    gpio_set_level(i2c_config[i2c_num]->sda_io_num, sda & 0x1);
    gpio_set_level(i2c_config[i2c_num]->scl_io_num, scl & 0x1);
    if ((i2c_last_state[i2c_num]->scl == 0) && ((scl & 0x1) == 1)) {
        // An I2C slave is allowed to hold down the clock if it needs to reduce the bus speed. The master, on the other hand, is required to read back the clock signal after releasing it to the high state and wait until the line has actually gone high.
        while (gpio_get_level(i2c_config[i2c_num]->scl_io_num) == 0 && (i++) < clk_stretch_tick); // Clock stretching
    }
    i2c_last_state[i2c_num]->val = ((sda & 0x1) << 1) | (scl & 0x1);

}

static inline uint8_t i2c_master_get_dc(i2c_port_t i2c_num)
{
    uint8_t sda_out;
    sda_out = gpio_get_level(i2c_config[i2c_num]->sda_io_num);
    return sda_out;
}

/*
    For i2c master mode, we don't need to use a buffer for the data, the APIs will execute the master commands
and return after all of the commands have been sent out or when error occurs. So when we send master commands,
we should free or modify the source data only after the i2c_master_cmd_begin function returns.
*/
esp_err_t i2c_driver_install(i2c_port_t i2c_num, i2c_mode_t mode)
{
    I2C_CHECK((i2c_num >= 0) && (i2c_num < I2C_NUM_MAX), I2C_NUM_ERROR_STR, ESP_ERR_INVALID_ARG);
    I2C_CHECK(mode < I2C_MODE_MAX, I2C_MODE_ERR_STR, ESP_ERR_INVALID_ARG);

    if (p_i2c_obj[i2c_num] == NULL) {
        p_i2c_obj[i2c_num] = (i2c_obj_t *) heap_caps_calloc(1, sizeof(i2c_obj_t), MALLOC_CAP_8BIT);

        if (p_i2c_obj[i2c_num] == NULL) {
            ESP_LOGE(I2C_TAG, I2C_DRIVER_MALLOC_ERR_STR);
            return ESP_FAIL;
        }

        i2c_obj_t *p_i2c = p_i2c_obj[i2c_num];
        p_i2c->i2c_num = i2c_num;
        p_i2c->mode = mode;
        p_i2c->status = I2C_STATUS_IDLE;
        p_i2c->cmd_link.cur = NULL;
        p_i2c->cmd_link.head = NULL;
        p_i2c->cmd_link.free = NULL;
    } else {
        ESP_LOGE(I2C_TAG, I2C_DRIVER_ERR_STR);
        return ESP_FAIL;
    }

    if (i2c_config[i2c_num] == NULL) {
        i2c_config[i2c_num] = (i2c_config_t *) heap_caps_calloc(1, sizeof(i2c_config_t), MALLOC_CAP_8BIT);

        if (i2c_config[i2c_num] == NULL) {
            ESP_LOGE(I2C_TAG, I2C_DRIVER_MALLOC_ERR_STR);
            heap_caps_free(p_i2c_obj[i2c_num]);
            p_i2c_obj[i2c_num] = NULL;
            return ESP_FAIL;
        }

        memset(i2c_config[i2c_num], 0, sizeof(i2c_config_t));
    } else {
        ESP_LOGE(I2C_TAG, I2C_DRIVER_ERR_STR);
        return ESP_FAIL;
    }

    if (i2c_last_state[i2c_num] == NULL) {
        i2c_last_state[i2c_num] = (i2c_last_state_t *) heap_caps_calloc(1, sizeof(i2c_last_state_t), MALLOC_CAP_8BIT);

        if (i2c_last_state[i2c_num] == NULL) {
            ESP_LOGE(I2C_TAG, I2C_DRIVER_MALLOC_ERR_STR);
            heap_caps_free(p_i2c_obj[i2c_num]);
            p_i2c_obj[i2c_num] = NULL;
            heap_caps_free(i2c_config[i2c_num]);
            i2c_config[i2c_num] = NULL;
            return ESP_FAIL;
        }

        memset(i2c_last_state[i2c_num], 0, sizeof(i2c_last_state_t));
    } else {
        ESP_LOGE(I2C_TAG, I2C_DRIVER_ERR_STR);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t i2c_driver_delete(i2c_port_t i2c_num)
{
    I2C_CHECK((i2c_num >= 0) && (i2c_num < I2C_NUM_MAX), I2C_NUM_ERROR_STR, ESP_ERR_INVALID_ARG);
    I2C_CHECK(i2c_last_state[i2c_num] != NULL, I2C_DRIVER_ERR_STR, ESP_FAIL);
    I2C_CHECK(i2c_config[i2c_num] != NULL, I2C_DRIVER_ERR_STR, ESP_FAIL);
    I2C_CHECK(p_i2c_obj[i2c_num] != NULL, I2C_DRIVER_ERR_STR, ESP_FAIL);

    heap_caps_free(i2c_last_state[i2c_num]);
    i2c_last_state[i2c_num] = NULL;
    heap_caps_free(i2c_config[i2c_num]);
    i2c_config[i2c_num] = NULL;
    heap_caps_free(p_i2c_obj[i2c_num]);
    p_i2c_obj[i2c_num] = NULL;

    return ESP_OK;
}

esp_err_t i2c_set_pin(i2c_port_t i2c_num, int sda_io_num, int scl_io_num, gpio_pullup_t sda_pullup_en, gpio_pullup_t scl_pullup_en, i2c_mode_t mode)
{
    I2C_CHECK((i2c_num >= 0) && (i2c_num < I2C_NUM_MAX), I2C_NUM_ERROR_STR, ESP_ERR_INVALID_ARG);
    I2C_CHECK(mode < I2C_MODE_MAX, I2C_MODE_ERR_STR, ESP_ERR_INVALID_ARG);

    gpio_config_t io_conf;

    if (sda_io_num >= 0) {
        // disable interrupt
        io_conf.intr_type = GPIO_INTR_DISABLE;
        // set as output mode
        io_conf.mode = GPIO_MODE_OUTPUT_OD;
        // bit mask of the pins that you want to set
        io_conf.pin_bit_mask = (1ULL << sda_io_num);
        // disable pull-down mode
        io_conf.pull_down_en = 0;
        // disable pull-up mode
        io_conf.pull_up_en = sda_pullup_en;
        // configure GPIO with the given settings
        ESP_ERROR_CHECK(gpio_config(&io_conf));
        ESP_ERROR_CHECK(gpio_set_level(sda_io_num, 1));
    }

    if (scl_io_num >= 0) {
        // disable interrupt
        io_conf.intr_type = GPIO_INTR_DISABLE;
        // set as output mode
        io_conf.mode = GPIO_MODE_OUTPUT_OD;
        // bit mask of the pins that you want to set
        io_conf.pin_bit_mask = (1ULL << scl_io_num);
        // disable pull-down mode
        io_conf.pull_down_en = 0;
        // disable pull-up mode
        io_conf.pull_up_en = scl_pullup_en;
        // configure GPIO with the given settings
        ESP_ERROR_CHECK(gpio_config(&io_conf));
        ESP_ERROR_CHECK(gpio_set_level(scl_io_num, 1));
    }

    return ESP_OK;
}

esp_err_t i2c_param_config(i2c_port_t i2c_num, const i2c_config_t *i2c_conf)
{
    I2C_CHECK((i2c_num >= 0) && (i2c_num < I2C_NUM_MAX), I2C_NUM_ERROR_STR, ESP_ERR_INVALID_ARG);
    I2C_CHECK(i2c_conf != NULL, I2C_ADDR_ERROR_STR, ESP_ERR_INVALID_ARG);
    I2C_CHECK(i2c_conf->mode < I2C_MODE_MAX, I2C_MODE_ERR_STR, ESP_ERR_INVALID_ARG);

    esp_err_t ret = i2c_set_pin(i2c_num, i2c_conf->sda_io_num, i2c_conf->scl_io_num,
                                i2c_conf->sda_pullup_en, i2c_conf->scl_pullup_en, i2c_conf->mode);

    if (ret != ESP_OK) {
        return ret;
    }

    memcpy((i2c_config_t *)(i2c_config[i2c_num]), (i2c_config_t *)i2c_conf, sizeof(i2c_config_t));
    return ESP_OK;
}

i2c_cmd_handle_t i2c_cmd_link_create()
{
    i2c_cmd_desc_t *cmd_desc = (i2c_cmd_desc_t *) heap_caps_calloc(1, sizeof(i2c_cmd_desc_t), MALLOC_CAP_8BIT);
    return (i2c_cmd_handle_t) cmd_desc;
}

void i2c_cmd_link_delete(i2c_cmd_handle_t cmd_handle)
{
    if (cmd_handle == NULL) {
        return;
    }

    i2c_cmd_desc_t *cmd = (i2c_cmd_desc_t *) cmd_handle;

    while (cmd->free) {
        i2c_cmd_link_t *ptmp = cmd->free;
        cmd->free = cmd->free->next;
        heap_caps_free(ptmp);
    }

    cmd->cur = NULL;
    cmd->free = NULL;
    cmd->head = NULL;
    heap_caps_free(cmd_handle);
    return;
}

static esp_err_t i2c_cmd_link_append(i2c_cmd_handle_t cmd_handle, i2c_cmd_t *cmd)
{
    i2c_cmd_desc_t *cmd_desc = (i2c_cmd_desc_t *) cmd_handle;

    if (cmd_desc->head == NULL) {
        cmd_desc->head = (i2c_cmd_link_t *) heap_caps_calloc(1, sizeof(i2c_cmd_link_t), MALLOC_CAP_8BIT);

        if (cmd_desc->head == NULL) {
            ESP_LOGE(I2C_TAG, I2C_CMD_MALLOC_ERR_STR);
            goto err;
        }

        cmd_desc->cur = cmd_desc->head;
        cmd_desc->free = cmd_desc->head;
    } else {
        cmd_desc->cur->next = (i2c_cmd_link_t *) heap_caps_calloc(1, sizeof(i2c_cmd_link_t), MALLOC_CAP_8BIT);

        if (cmd_desc->cur->next == NULL) {
            ESP_LOGE(I2C_TAG, I2C_CMD_MALLOC_ERR_STR);
            goto err;
        }

        cmd_desc->cur = cmd_desc->cur->next;
    }

    memcpy((uint8_t *) &cmd_desc->cur->cmd, (uint8_t *) cmd, sizeof(i2c_cmd_t));
    cmd_desc->cur->next = NULL;
    return ESP_OK;

err:
    return ESP_FAIL;
}

esp_err_t i2c_master_start(i2c_cmd_handle_t cmd_handle)
{
    I2C_CHECK(cmd_handle != NULL, I2C_CMD_LINK_INIT_ERR_STR, ESP_ERR_INVALID_ARG);
    i2c_cmd_t cmd;
    cmd.ack.en = 0;
    cmd.ack.exp = 0;
    cmd.ack.val = 0;
    cmd.byte_num = 0;
    cmd.data = NULL;
    cmd.op_code = I2C_CMD_RESTART;
    return i2c_cmd_link_append(cmd_handle, &cmd);
}

esp_err_t i2c_master_stop(i2c_cmd_handle_t cmd_handle)
{
    I2C_CHECK(cmd_handle != NULL, I2C_CMD_LINK_INIT_ERR_STR, ESP_ERR_INVALID_ARG);
    i2c_cmd_t cmd;
    cmd.ack.en = 0;
    cmd.ack.exp = 0;
    cmd.ack.val = 0;
    cmd.byte_num = 0;
    cmd.data = NULL;
    cmd.op_code = I2C_CMD_STOP;
    return i2c_cmd_link_append(cmd_handle, &cmd);
}

esp_err_t i2c_master_write_byte(i2c_cmd_handle_t cmd_handle, uint8_t data, bool ack_en)
{
    I2C_CHECK(cmd_handle != NULL, I2C_CMD_LINK_INIT_ERR_STR, ESP_ERR_INVALID_ARG);
    i2c_cmd_t cmd;
    cmd.ack.en = ack_en;
    cmd.ack.exp = 0;
    cmd.ack.val = 0;
    cmd.byte_num = 1;
    cmd.op_code = I2C_CMD_WRITE;
    cmd.data = NULL;
    cmd.byte_cmd = data;
    return i2c_cmd_link_append(cmd_handle, &cmd);
}

esp_err_t i2c_master_write(i2c_cmd_handle_t cmd_handle, uint8_t *data, size_t data_len, bool ack_en)
{
    I2C_CHECK((data != NULL), I2C_ADDR_ERROR_STR, ESP_ERR_INVALID_ARG);
    I2C_CHECK(cmd_handle != NULL, I2C_CMD_LINK_INIT_ERR_STR, ESP_ERR_INVALID_ARG);

    uint8_t len_tmp;
    int data_offset = 0;
    esp_err_t ret;

    while (data_len > 0) {
        len_tmp = data_len > 0xff ? 0xff : data_len;
        data_len -= len_tmp;
        i2c_cmd_t cmd;
        cmd.ack.en = ack_en;
        cmd.ack.exp = 0;
        cmd.ack.val = 0;
        cmd.byte_num = len_tmp;
        cmd.op_code = I2C_CMD_WRITE;
        cmd.data = data + data_offset;
        ret = i2c_cmd_link_append(cmd_handle, &cmd);
        data_offset += len_tmp;

        if (ret != ESP_OK) {
            return ret;
        }
    }

    return ESP_OK;
}

static esp_err_t i2c_master_read_static(i2c_cmd_handle_t cmd_handle, uint8_t *data, size_t data_len, i2c_ack_type_t ack)
{
    int len_tmp;
    int data_offset = 0;
    esp_err_t ret;

    while (data_len > 0) {
        len_tmp = data_len > 0xff ? 0xff : data_len;
        data_len -= len_tmp;
        i2c_cmd_t cmd;
        cmd.ack.en = 0;
        cmd.ack.exp = 0;
        cmd.ack.val = ack & 0x1;
        cmd.byte_num = len_tmp;
        cmd.op_code = I2C_CMD_READ;
        cmd.data = data + data_offset;
        ret = i2c_cmd_link_append(cmd_handle, &cmd);
        data_offset += len_tmp;

        if (ret != ESP_OK) {
            return ret;
        }
    }

    return ESP_OK;
}

esp_err_t i2c_master_read_byte(i2c_cmd_handle_t cmd_handle, uint8_t *data, i2c_ack_type_t ack)
{
    I2C_CHECK((data != NULL), I2C_ADDR_ERROR_STR, ESP_ERR_INVALID_ARG);
    I2C_CHECK(cmd_handle != NULL, I2C_CMD_LINK_INIT_ERR_STR, ESP_ERR_INVALID_ARG);
    I2C_CHECK(ack < I2C_MASTER_ACK_MAX, I2C_ACK_TYPE_ERR_STR, ESP_ERR_INVALID_ARG);

    i2c_cmd_t cmd;
    cmd.ack.en = 0;
    cmd.ack.exp = 0;
    cmd.ack.val = ((ack == I2C_MASTER_LAST_NACK) ? I2C_MASTER_NACK : (ack & 0x1));
    cmd.byte_num = 1;
    cmd.op_code = I2C_CMD_READ;
    cmd.data = data;
    return i2c_cmd_link_append(cmd_handle, &cmd);
}

esp_err_t i2c_master_read(i2c_cmd_handle_t cmd_handle, uint8_t *data, size_t data_len, i2c_ack_type_t ack)
{
    I2C_CHECK((data != NULL), I2C_ADDR_ERROR_STR, ESP_ERR_INVALID_ARG);
    I2C_CHECK(cmd_handle != NULL, I2C_CMD_LINK_INIT_ERR_STR, ESP_ERR_INVALID_ARG);
    I2C_CHECK(ack < I2C_MASTER_ACK_MAX, I2C_ACK_TYPE_ERR_STR, ESP_ERR_INVALID_ARG);
    I2C_CHECK(data_len > 0, I2C_DATA_LEN_ERR_STR, ESP_ERR_INVALID_ARG);

    if (ack != I2C_MASTER_LAST_NACK) {
        return i2c_master_read_static(cmd_handle, data, data_len, ack);
    } else {
        if (data_len == 1) {
            return i2c_master_read_byte(cmd_handle, data, I2C_MASTER_NACK);
        } else {
            esp_err_t ret;

            // first read (data_len - 1) byte data with ACK,than read the end byte with NACK.
            if ((ret =  i2c_master_read_static(cmd_handle, data, data_len - 1, I2C_MASTER_ACK)) != ESP_OK) {
                return ret;
            }

            return i2c_master_read_byte(cmd_handle, data + data_len - 1, I2C_MASTER_NACK);
        }
    }
}

static void i2c_master_cmd_begin_static(i2c_port_t i2c_num)
{
    i2c_obj_t *p_i2c = p_i2c_obj[i2c_num];
    i2c_cmd_t *cmd;
    uint8_t dat;
    uint8_t len;
    int8_t i, k;
    uint8_t retVal;

    // This should never happen
    if (p_i2c->mode != I2C_MODE_MASTER) {
        return;
    }

    while (p_i2c->cmd_link.head) {
        cmd = &p_i2c->cmd_link.head->cmd;

        switch (cmd->op_code) {
            case (I2C_CMD_RESTART): {
                i2c_master_set_dc(i2c_num, 1, i2c_last_state[i2c_num]->scl);
                i2c_master_set_dc(i2c_num, 1, 1);
                i2c_master_wait(1);     // sda 1, scl 1
                i2c_master_set_dc(i2c_num, 0, 1);
                i2c_master_wait(1);     // sda 0, scl 1
            }
            break;

            case (I2C_CMD_WRITE): {
                p_i2c->status = I2C_STATUS_WRITE;

                for (len = 0; len < cmd->byte_num; len++) {
                    dat = 0;
                    retVal = 0;
                    i2c_master_set_dc(i2c_num, i2c_last_state[i2c_num]->sda, 0);

                    for (i = 7; i >= 0; i--) {
                        if (cmd->byte_num == 1 && cmd->data == NULL) {
                            dat = (cmd->byte_cmd) >> i;
                        } else {
                            dat = ((uint8_t) * (cmd->data + len)) >> i;
                        }

                        i2c_master_set_dc(i2c_num, dat, 0);
                        i2c_master_wait(1);
                        i2c_master_set_dc(i2c_num, dat, 1);
                        i2c_master_wait(2);

                        if (i == 0) {
                            i2c_master_wait(1);   // wait slaver ack
                        }

                        i2c_master_set_dc(i2c_num, dat, 0);
                    }

                    i2c_master_set_dc(i2c_num, i2c_last_state[i2c_num]->sda, 0);
                    i2c_master_set_dc(i2c_num, 1, 0);
                    i2c_master_set_dc(i2c_num, 1, 1);
                    i2c_master_wait(1);
                    retVal = i2c_master_get_dc(i2c_num);
                    i2c_master_wait(1);
                    i2c_master_set_dc(i2c_num, 1, 0);

                    if (cmd->ack.en == 1) {
                        if ((retVal & 0x01) != cmd->ack.exp) {
                            p_i2c->status = I2C_STATUS_ACK_ERROR;
                            return ;
                        }
                    }
                }
            }
            break;

            case (I2C_CMD_READ): {
                p_i2c->status = I2C_STATUS_READ;

                for (len = 0; len < cmd->byte_num; len++) {
                    retVal = 0;
                    i2c_master_set_dc(i2c_num, i2c_last_state[i2c_num]->sda, 0);

                    for (i = 0; i < 8; i++) {
                        i2c_master_set_dc(i2c_num, 1, 0);
                        i2c_master_wait(2);
                        i2c_master_set_dc(i2c_num, 1, 1);
                        i2c_master_wait(1);     // sda 1, scl 1
                        k = i2c_master_get_dc(i2c_num);
                        i2c_master_wait(1);

                        if (i == 7) {
                            i2c_master_wait(1);
                        }

                        k <<= (7 - i);
                        retVal |= k;
                    }

                    i2c_master_set_dc(i2c_num, 1, 0);
                    memcpy((uint8_t *)(cmd->data + len), (uint8_t *)&retVal, 1);
                    i2c_master_set_dc(i2c_num, i2c_last_state[i2c_num]->sda, 0);
                    i2c_master_set_dc(i2c_num, cmd->ack.val, 0);
                    i2c_master_set_dc(i2c_num, cmd->ack.val, 1);
                    i2c_master_wait(4);     // sda level, scl 1
                    i2c_master_set_dc(i2c_num, cmd->ack.val, 0);
                    i2c_master_set_dc(i2c_num, 1, 0);
                    i2c_master_wait(1);
                }
            }
            break;

            case (I2C_CMD_STOP): {
                i2c_master_wait(1);
                i2c_master_set_dc(i2c_num, 0, i2c_last_state[i2c_num]->scl);
                i2c_master_set_dc(i2c_num, 0, 1);
                i2c_master_wait(2);     // sda 0, scl 1
                i2c_master_set_dc(i2c_num, 1, 1);
                i2c_master_wait(2);     // sda 1, scl 1
            }
            break;
        }

        p_i2c->cmd_link.head = p_i2c->cmd_link.head->next;
    }

    p_i2c->status = I2C_STATUS_DONE;
    return;
}

esp_err_t i2c_master_cmd_begin(i2c_port_t i2c_num, i2c_cmd_handle_t cmd_handle, TickType_t ticks_to_wait)
{
    I2C_CHECK((i2c_num >= 0) && (i2c_num < I2C_NUM_MAX), I2C_NUM_ERROR_STR, ESP_ERR_INVALID_ARG);
    I2C_CHECK(p_i2c_obj[i2c_num] != NULL, I2C_DRIVER_NOT_INSTALL_ERR_STR, ESP_ERR_INVALID_STATE);
    I2C_CHECK(p_i2c_obj[i2c_num]->mode == I2C_MODE_MASTER, I2C_MASTER_MODE_ERR_STR, ESP_ERR_INVALID_STATE);
    I2C_CHECK(cmd_handle != NULL, I2C_CMD_LINK_INIT_ERR_STR, ESP_ERR_INVALID_ARG);

    i2c_obj_t *p_i2c = p_i2c_obj[i2c_num];
    i2c_cmd_desc_t *cmd = (i2c_cmd_desc_t *) cmd_handle;
    p_i2c->cmd_link.free = cmd->free;
    p_i2c->cmd_link.cur = cmd->cur;
    p_i2c->cmd_link.head = cmd->head;
    p_i2c->status = I2C_STATUS_IDLE;
    ENTER_CRITICAL();
    // start send commands, at most 32 bytes one time, isr handler will process the remaining commands.
    i2c_master_cmd_begin_static(i2c_num);
    EXIT_CRITICAL();

    // TODO: Timeout check
    if (p_i2c->status == I2C_STATUS_DONE) {
        return ESP_OK;
    } else {
        return ESP_FAIL;
    }

}
