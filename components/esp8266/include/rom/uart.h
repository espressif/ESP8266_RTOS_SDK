// Copyright 2010-2016 Espressif Systems (Shanghai) PTE LTD
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

#ifndef _ROM_UART_H_
#define _ROM_UART_H_

#include "esp_types.h"
#include "esp_attr.h"
#include "ets_sys.h"

#include "esp8266/uart_struct.h"
#include "esp8266/uart_register.h"
#include "esp8266/pin_mux_register.h"
#include "esp8266/eagle_soc.h"
#include "esp8266/rom_functions.h"

#include "driver/soc.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup uart_apis, uart configuration and communication related apis
  * @brief uart apis
  */

/** @addtogroup uart_apis
  * @{
  */

/**
  * @brief Wait until uart tx full empty and the last char send ok.
  *
  * @param  uart_no : 0 for UART0, 1 for UART1, 2 for UART2
  *
  * The function defined in ROM code has a bug, so we define the correct version
  * here for compatibility.
  */
void uart_tx_wait_idle(uint8_t uart_no);

/**
  * @brief Output a char to printf channel, wait until fifo not full.
  *
  * @param  None
  *
  * @return OK.
  */
STATUS uart_tx_one_char(uint8_t TxChar);

/**
  * @brief Get an input char from message channel.
  *        Please do not call this function in SDK.
  *
  * @param  uint8_t *pRxChar : the pointer to store the char.
  *
  * @return OK for successful.
  *         FAIL for failed.
  */
STATUS uart_rx_one_char(uint8_t *pRxChar);

/**
  * @brief Get an input string line from message channel.
  *        Please do not call this function in SDK.
  *
  * @param  uint8_t *pString : the pointer to store the string.
  *
  * @param  uint8_t MaxStrlen : the max string length, incude '\0'.
  *
  * @return OK.
  */
static inline STATUS UartRxString(uint8_t *pString, uint8_t MaxStrlen)
{
    int rx_bytes = 0;

    while(1) {
        uint8_t data;

        while (uart_rx_one_char(&data) != OK);
        
        if (data == '\n' || data == '\r')
            data = '\0';

        pString[rx_bytes++] = data;
        if (data == '\0')
            return OK;
        if (rx_bytes >= MaxStrlen)
            return FAIL;
    }

    return OK;
}

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* _ROM_UART_H_ */
