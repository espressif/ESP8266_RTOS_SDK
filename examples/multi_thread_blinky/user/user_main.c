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

#include "esp_common.h"

// UART and GPIO drivers
#include "../driver_lib/include/gpio.h"
#include "../driver_lib/include/uart.h"

// For vTaskDelay
#include "freertos/task.h"

// CONSTANTS 
#define DEMO_AP_SSID "dusri-line"
#define DEMO_AP_PASSWORD "abkiskopataha"
#define LED_GPIO_NUMBER 2


/******************************************************************************
 * FunctionName : toggle_led
 * Description  : toggle LED_GPIO_NUMBER state
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void toggle_led()
{
	static bool led_on = true;

	led_on = !led_on;
	GPIO_OUTPUT_SET(LED_GPIO_NUMBER, led_on);
}


/******************************************************************************
 * FunctionName : task_blink
 * Description  : entry of task_blink, toggles LED_GPIO_NUMBER forever
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void task_blink(void *pvParameters)
{
	printf("Hello, welcome to task_blink!\r\n");

	const portTickType xDelay = 500 / portTICK_RATE_MS;
	while (1) {
         toggle_led();
         vTaskDelay( xDelay );
	}
	vTaskDelete(NULL);
}

/******************************************************************************
 * FunctionName : task_hello_keepalive
 * Description  : entry of task_hello_keepalive, prints keepalive msgs on UART0
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void task_hello_keepalive(void *pvParameters)
{
	printf("Hello, welcome to task_hello_keepalive!\r\n");

	const portTickType xDelay = 1000 / portTICK_RATE_MS;
	uint32_t counter = 0;
	while (1) {
		printf("task_hello_keepalive is alive! - %u \r\n", counter++);
		vTaskDelay( xDelay );
	}
	vTaskDelete(NULL);
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
	// Init UART0 and set baud rate to 115200 bps
	uart_init_new();

    printf("SDK version:%s\n", system_get_sdk_version());

    // Connect to WiFi AP and print the IP address assigned using DHCP.
	wifi_set_opmode(STATION_MODE);
	struct station_config * config = (struct station_config *)zalloc(sizeof(struct
	station_config));
	sprintf(config->ssid, DEMO_AP_SSID);
	sprintf(config->password, DEMO_AP_PASSWORD);
	wifi_station_set_config(config);
	free(config);
	wifi_station_connect();

	// Set GPIO2 as output.
	GPIO_AS_OUTPUT(LED_GPIO_NUMBER);

	// Create tasks
	xTaskCreate(task_blink, "blink", 256, NULL, 2, NULL);
	xTaskCreate(task_hello_keepalive, "hello_keepalive", 256, NULL, 2, NULL);
}

