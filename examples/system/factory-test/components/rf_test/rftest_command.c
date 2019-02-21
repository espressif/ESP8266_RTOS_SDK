// Copyright 2019-2020 Espressif Systems (Shanghai) PTE LTD
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include "esp_rftest.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp8266/eagle_soc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "driver/soc.h"
#include "nano_console.h"

#define TAG "factory-test"

typedef struct tx_param {
    uint32_t channel;
    uint32_t rate;
    uint32_t attenuation;
} tx_param_t;

typedef struct rx_param {
    uint32_t channel;
    uint32_t rate;
} rx_param_t;

typedef struct wifiscwout_param {
    uint32_t en;
    uint32_t channel;
    uint32_t attenuation;
} wifiscwout_param_t;

static int s_cmdstop = 0;

static int set_wifi_work(void)
{
    int ret;
    esp_irqflag_t flag;

    flag = soc_save_local_irq();

    if (s_cmdstop == 0) {
        s_cmdstop = 3;
        ret = 0;
    } else
        ret = -EINPROGRESS;

    soc_restore_local_irq(flag);

    return ret;
}

static int set_wifi_free(void)
{
    esp_irqflag_t flag;

    flag = soc_save_local_irq();

    s_cmdstop = 0;

    soc_restore_local_irq(flag);

    return 0;
}

static int test_rftest_init(int argc, char **argv)
{
    if (argc != 1) {
        ESP_LOGE(TAG, "%s %d command is error", __func__, __LINE__);
        return -EINVAL;      
    }

    ESP_LOGD(__func__, "%s start initializing WiFi test", __func__);

    if (set_wifi_work())
        goto exit;

    rftest_init();

    set_wifi_free();

    ESP_LOGD(__func__, "%s end initializing WiFi test", __func__);

    return 0;

exit:
    ESP_LOGD(__func__, "%s fail to initialize WiFi test", __func__);

    return -EINPROGRESS;
}

static int test_tx_contin_en(int argc, char **argv)
{
    if (argc != 2) {
        ESP_LOGE(TAG, "%s %d command is error", __func__, __LINE__);
        return -EINVAL;      
    }

    int mode = atoi(argv[1]);
    if (mode < 0 || mode > 1) {
        ESP_LOGE(TAG, "%s %d command is error", __func__, __LINE__);
        return -EINVAL;
    }

    ESP_LOGD(__func__, "%s start setting mode '%d'", __func__ , mode);

    if (set_wifi_work())
        goto exit;

    tx_contin_func(mode);

    set_wifi_free();

    ESP_LOGD(__func__, "%s end setting mode '%d'", __func__ , mode);

    return 0;

exit:
    ESP_LOGD(__func__, "%s fail to set mode '%d'", __func__ , mode);

    return -EINPROGRESS;
}

static void test_wifi_tx_thread(void *param)
{
    tx_param_t *tx_param = (tx_param_t *)param;

    esp_tx_func(tx_param->channel, tx_param->rate, tx_param->attenuation);

    free(tx_param);

    set_wifi_free();

    vTaskDelete(NULL);
}

static int test_esp_tx_func(int argc, char **argv)
{
    if (argc != 4) {
        ESP_LOGE(TAG, "%s %d command is error", __func__, __LINE__);
        return -EINVAL;      
    }

    int channel = atoi(argv[1]);
    if (channel <= 0 || channel > 14) {
        ESP_LOGE(TAG, "%s %d command is error", __func__, __LINE__);
        return -EINVAL;        
    }

    int rate = atoi(argv[2]);
    if (rate < 0 || rate > 23) {
        ESP_LOGE(TAG, "%s %d command is error", __func__, __LINE__);
        return -EINVAL;
    }

    int attenuation = atoi(argv[3]);
    if (attenuation < -127 || attenuation > 127) {
        ESP_LOGE(TAG, "%s %d command is error", __func__, __LINE__);
        return -EINVAL;
    }

    ESP_LOGD(__func__, "%s start creating task with channel '%d' rate '%d' attenuation '%d'", __func__, channel, rate, attenuation);

    if (set_wifi_work())
        goto exit;

    tx_param_t *tx_param = malloc(sizeof(tx_param_t));
    if (!tx_param)
        goto exit;

    tx_param->channel = channel;
    tx_param->rate = rate;
    tx_param->attenuation = attenuation;

    const size_t wifi_tx_stk_size = 4096;
    const size_t wifi_tx_priority = 3;

    BaseType_t ret = xTaskCreate(test_wifi_tx_thread, "wifi_tx", wifi_tx_stk_size, tx_param, wifi_tx_priority, NULL);
    if (ret != pdPASS)
        goto task_err;

    ESP_LOGD(__func__, "%s end creating task with channel '%d' rate '%d' attenuation '%d'", __func__, channel, rate, attenuation);

    return 0;

task_err:
    free(tx_param);
exit:
    ESP_LOGD(__func__, "%s fail to create task with channel '%d' rate '%d' attenuation '%d'", __func__, channel, rate, attenuation);

    return -ENOMEM;
}

static void test_wifi_rx_thread(void *param)
{
    rx_param_t *rx_param = (rx_param_t *)param;

    esp_rx_func(rx_param->channel, rx_param->rate);

    free(rx_param);

    set_wifi_free();

    vTaskDelete(NULL);
}

static int test_esp_rx_func(int argc, char **argv)
{
    if (argc != 3) {
        ESP_LOGE(TAG, "%s %d command is error", __func__, __LINE__);
        return -EINVAL;      
    }

    int channel = atoi(argv[1]);
    if (channel <= 0 || channel > 14) {
        ESP_LOGE(TAG, "%s %d command is error", __func__, __LINE__);
        return -EINVAL;        
    }

    int rate = atoi(argv[2]);
    if (rate < 0 || rate > 23) {
        ESP_LOGE(TAG, "%s %d command is error", __func__, __LINE__);
        return -EINVAL;
    }

    ESP_LOGD(__func__, "%s start creating task with channel '%d' rate '%d'", __func__, channel, rate);

    if (set_wifi_work())
        goto exit;

    rx_param_t *rx_param = malloc(sizeof(rx_param_t));
    if (!rx_param)
        goto exit;

    rx_param->channel = channel;
    rx_param->rate = rate;

    const size_t wifi_rx_stk_size = 4096;
    const size_t wifi_rx_priority = 3;

    BaseType_t ret = xTaskCreate(test_wifi_rx_thread, "wifi_rx", wifi_rx_stk_size, rx_param, wifi_rx_priority, NULL);
    if (ret != pdPASS)
        goto task_err;

    ESP_LOGD(__func__, "%s end creating task with channel '%d' rate '%d'", __func__, channel, rate);

    return 0;

task_err:
    free(rx_param);
exit:
    ESP_LOGD(__func__, "%s fail to create task with channel '%d' rate '%d'", __func__, channel, rate);

    return -ENOMEM;  
}

static void test_wifi_wifiscwout_thread(void *param)
{
    wifiscwout_param_t *wifiscwout_param = (wifiscwout_param_t *)param;

    wifiscwout_func(wifiscwout_param->en, wifiscwout_param->channel, wifiscwout_param->attenuation);

    free(wifiscwout_param);

    set_wifi_free();

    vTaskDelete(NULL);
}

static int test_wifiscwout_func(int argc, char **argv)
{
    if (argc != 4) {
        ESP_LOGE(TAG, "%s %d command is error", __func__, __LINE__);
        return -EINVAL;      
    }

    int en = atoi(argv[1]);
    if (en < 0 || en > 2) {
        ESP_LOGE(TAG, "%s %d command is error", __func__, __LINE__);
        return -EINVAL;
    }

    int channel = atoi(argv[2]);
    if (channel <= 0 || channel > 14) {
        ESP_LOGE(TAG, "%s %d command is error", __func__, __LINE__);
        return -EINVAL;
    }

    int attenuation = atoi(argv[3]);
    if (attenuation < -127 || attenuation > 127) {
        ESP_LOGE(TAG, "%s %d command is error", __func__, __LINE__);
        return -EINVAL;
    }

    ESP_LOGD(__func__, "%s start creating task with enable '%d' channel '%d' attenuation '%d'", __func__, en, channel, attenuation);

    if (set_wifi_work())
        goto exit;

    wifiscwout_param_t *wifiscwout_param = malloc(sizeof(wifiscwout_param_t));
    if (!wifiscwout_param)
        goto exit;

    wifiscwout_param->en = en;
    wifiscwout_param->channel = channel;
    wifiscwout_param->attenuation = attenuation;

    const size_t wifi_wifiscwout_stk_size = 4096;
    const size_t wifi_wifiscwout_priority = 3;

    BaseType_t ret = xTaskCreate(test_wifi_wifiscwout_thread, "wifi_tx", wifi_wifiscwout_stk_size, wifiscwout_param, wifi_wifiscwout_priority, NULL);
    if (ret != pdPASS)
        goto task_err;

    ESP_LOGD(__func__, "%s end creating task with enable '%d' channel '%d' attenuation '%d'", __func__, en, channel, attenuation);

    return 0;

task_err:
    free(wifiscwout_param);
exit:
    ESP_LOGD(__func__, "%s fail to create task with enable '%d' channel '%d' attenuation '%d'", __func__, en, channel, attenuation);

    return -ENOMEM; 
}

static int test_cmdstop_func(int argc, char **argv)
{
    if (argc != 1) {
        ESP_LOGE(TAG, "%s %d command is error", __func__, __LINE__);
        return -EINVAL;
    }

    ESP_LOGD(__func__, "%s cmdstop '%d'", __func__, s_cmdstop);

    s_cmdstop = 0;

    ESP_LOGD(__func__,  "status is %x\n", REG_READ(INT_ENA_WDEV));

    return 0;    
}

static void register_rftest_init(void)
{
    nc_cmd_t *nc_cmd;

    ESP_ERROR_CHECK(nc_register_cmd(&nc_cmd, "rftest_init", test_rftest_init));
}

static void register_tx_contin_en(void)
{
    nc_cmd_handle_t nc_cmd;

    ESP_ERROR_CHECK(nc_register_cmd(&nc_cmd, "tx_contin_en", test_tx_contin_en));
}

static void register_esp_tx_func(void)
{
    nc_cmd_handle_t nc_cmd;

    ESP_ERROR_CHECK(nc_register_cmd(&nc_cmd, "esp_tx", test_esp_tx_func));
    ESP_ERROR_CHECK(nc_register_cmd(&nc_cmd, "wifitxout", test_esp_tx_func));
}

static void register_esp_rx_func(void)
{
    nc_cmd_handle_t nc_cmd;

    ESP_ERROR_CHECK(nc_register_cmd(&nc_cmd, "esp_rx", test_esp_rx_func));
}

static void register_wifiscwout_func(void)
{
    nc_cmd_handle_t nc_cmd;

    ESP_ERROR_CHECK(nc_register_cmd(&nc_cmd, "wifiscwout", test_wifiscwout_func));
}

static void register_cmdstop_func(void)
{
    nc_cmd_handle_t nc_cmd;

    ESP_ERROR_CHECK(nc_register_cmd(&nc_cmd, "cmdstop", test_cmdstop_func));
    ESP_ERROR_CHECK(nc_register_cmd(&nc_cmd, "CmdStop", test_cmdstop_func));
}

void esp_console_register_rftest_command(void)
{
    extern void esp_dport_close_nmi(void);

    esp_dport_close_nmi();

    register_rftest_init();
    register_tx_contin_en();
    register_esp_tx_func();
    register_esp_rx_func();
    register_wifiscwout_func();
    register_cmdstop_func();
}

int __attribute__((weak)) cmdstop_callback(void)
{
    return s_cmdstop;
}
