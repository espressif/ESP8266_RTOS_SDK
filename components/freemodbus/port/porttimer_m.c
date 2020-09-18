/* Copyright 2018 Espressif Systems (Shanghai) PTE LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
 * FreeModbus Libary: ESP32 Port Demo Application
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: porttimer_m.c,v 1.60 2013/08/13 15:07:05 Armink add Master Functions$
 */

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb_m.h"
#include "mbport.h"
#include "sdkconfig.h"
#include "esp_timer.h"

/* ----------------------- Variables ----------------------------------------*/
static USHORT msT35TimeOut;
static esp_timer_handle_t xTimerIntHandle;
/* ----------------------- static functions ---------------------------------*/
static void IRAM_ATTR vTimerGroupIsr(void *param)
{
    (void)pxMBMasterPortCBTimerExpired(); // Timer expired callback function
}

/* ----------------------- Start implementation -----------------------------*/
BOOL xMBMasterPortTimersInit(USHORT msTimeOut)
{
    MB_PORT_CHECK((msTimeOut > 0), FALSE,
            "Modbus timeout discreet is incorrect.");
    // Save timer reload value for Modbus T35 period
    msT35TimeOut = msTimeOut;
    esp_timer_create_args_t timer_conf = {
        .callback = vTimerGroupIsr,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "PORT_TIMER_M"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_conf, &xTimerIntHandle));

    return TRUE;
}

// Set alarm value for usTimerTimeOut50us * 50uS
static BOOL xMBMasterPortTimersEnable(USHORT msTimeOut)
{
    MB_PORT_CHECK((msTimeOut > 0), FALSE,
                            "incorrect tick value for timer = (0x%x).",
                            (uint32_t)msTimeOut);
    ESP_ERROR_CHECK(esp_timer_stop(xTimerIntHandle));
    ESP_ERROR_CHECK(esp_timer_start_once(xTimerIntHandle, msTimeOut * 1000));
    return TRUE;
}

void vMBMasterPortTimersT35Enable(void)
{
    USHORT msTimeOut = msT35TimeOut;

    // Set current timer mode, don't change it.
    vMBMasterSetCurTimerMode(MB_TMODE_T35);
    // Set timer period
    (void)xMBMasterPortTimersEnable(msTimeOut);
}

void vMBMasterPortTimersConvertDelayEnable(void)
{
    // Covert time in milliseconds into ticks
    USHORT msTimeOut = MB_MASTER_DELAY_MS_CONVERT;
    // Set current timer mode
    vMBMasterSetCurTimerMode(MB_TMODE_CONVERT_DELAY);
    ESP_LOGD(MB_PORT_TAG,"%s Convert delay enable.", __func__);
    (void)xMBMasterPortTimersEnable(msTimeOut);
}

void vMBMasterPortTimersRespondTimeoutEnable(void)
{
    USHORT msTimeOut = MB_MASTER_TIMEOUT_MS_RESPOND;
    vMBMasterSetCurTimerMode(MB_TMODE_RESPOND_TIMEOUT);
    ESP_LOGD(MB_PORT_TAG,"%s Respond enable timeout.", __func__);
    (void)xMBMasterPortTimersEnable(msTimeOut);
}

void MB_PORT_ISR_ATTR
vMBMasterPortTimersDisable()
{
    ESP_ERROR_CHECK(esp_timer_stop(xTimerIntHandle));
}

void vMBMasterPortTimerClose(void)
{
    ESP_ERROR_CHECK(esp_timer_delete(xTimerIntHandle));
}
