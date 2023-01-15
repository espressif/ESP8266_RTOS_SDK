/*
 * Copyright (c) 2022 <qb4.dev@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <esp_err.h>
#include <esp_log.h>

#include <esp8266/eagle_soc.h>
#include <esp8266/pin_mux_register.h>
#include <esp8266/gpio_register.h>
#include <esp8266/gpio_struct.h>

#include <driver/gpio.h>
#include "driver/sigma_delta.h"

static const char *TAG = "sigma-delta";

#define SIGMA_DELTA_CHECK(a, str, ret_val) \
	if (!(a)) { \
		ESP_LOGE(TAG,"%s(%d): %s", __FUNCTION__, __LINE__, str); \
		return (ret_val); \
	}

#define GPIO_SIGMA_DELTA_TARGET_MASK   (SIGMA_DELTA_TARGET << SIGMA_DELTA_TARGET_S)
#define GPIO_SIGMA_DELTA_TARGET_GET(x) (((x) & GPIO_SIGMA_DELTA_TARGET_MASK) >> SIGMA_DELTA_TARGET_S)
#define GPIO_SIGMA_DELTA_TARGET_SET(x) (((x) << SIGMA_DELTA_TARGET_S) & GPIO_SIGMA_DELTA_TARGET_MASK)

#define GPIO_SIGMA_DELTA_PRESCALE_MASK (SIGMA_DELTA_PRESCALAR << SIGMA_DELTA_PRESCALAR_S)
#define GPIO_SIGMA_DELTA_PRESCALE_GET(x) (((x) & GPIO_SIGMA_DELTA_PRESCALE_MASK) >> SIGMA_DELTA_PRESCALAR_S)
#define GPIO_SIGMA_DELTA_PRESCALE_SET(x) (((x) << SIGMA_DELTA_PRESCALAR_S) & GPIO_SIGMA_DELTA_PRESCALE_MASK)

#define GPIO_AS_PIN_SOURCE 0
#define SIGMA_AS_PIN_SOURCE 1

esp_err_t sigma_delta_init(uint8_t prescaler,uint8_t target)
{
	GPIO.sigma_delta = SIGMA_DELTA_ENABLE |
		GPIO_SIGMA_DELTA_TARGET_SET(target) |
		GPIO_SIGMA_DELTA_PRESCALE_SET(prescaler);

	ESP_LOGI(TAG,"enabled with prescale=%d and target=%d", prescaler, target);
	return ESP_OK;
}

esp_err_t sigma_delta_set_prescale(uint8_t prescale)
{
	GPIO.sigma_delta = (GPIO.sigma_delta & (~GPIO_SIGMA_DELTA_PRESCALE_MASK)) |
		GPIO_SIGMA_DELTA_PRESCALE_SET(prescale);
	return ESP_OK;
}

esp_err_t sigma_delta_get_prescale(uint8_t *prescale)
{
	SIGMA_DELTA_CHECK(prescale, "prescale is null", ESP_ERR_INVALID_ARG);
	*prescale = GPIO_SIGMA_DELTA_PRESCALE_GET(GPIO.sigma_delta);
	return ESP_OK;
}

esp_err_t sigma_delta_set_target(uint8_t target)
{
	GPIO.sigma_delta = (GPIO.sigma_delta & (~GPIO_SIGMA_DELTA_TARGET_MASK)) |
		GPIO_SIGMA_DELTA_TARGET_SET(target);
	return ESP_OK;
}

esp_err_t sigma_delta_get_target(uint8_t *target)
{
	SIGMA_DELTA_CHECK(target, "target is null", ESP_ERR_INVALID_ARG);
	*target = GPIO_SIGMA_DELTA_TARGET_GET(GPIO.sigma_delta);
	return ESP_OK;
}

esp_err_t sigma_delta_deinit(void)
{
	GPIO.sigma_delta = 0x00;

	ESP_LOGI(TAG,"disabled");
	return ESP_OK;
}

esp_err_t sigma_delta_set_output(gpio_num_t gpio_num)
{
	gpio_config_t io_config = {
		.pin_bit_mask = (1 << gpio_num),
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = 0,
		.pull_down_en = 0,
		.intr_type = GPIO_INTR_DISABLE
	};
	ESP_ERROR_CHECK(gpio_config(&io_config));

	GPIO.pin[gpio_num].source = SIGMA_AS_PIN_SOURCE;
	ESP_LOGI(TAG,"signal output set on GPIO[%d]", gpio_num);
	return ESP_OK;
}

esp_err_t sigma_delta_clear_output(gpio_num_t gpio_num)
{
	ESP_ERROR_CHECK(gpio_set_level(gpio_num,0));
	ESP_ERROR_CHECK(gpio_set_direction(gpio_num,GPIO_MODE_DISABLE));

	GPIO.pin[gpio_num].source = GPIO_AS_PIN_SOURCE;
	ESP_LOGI(TAG,"signal output clear on GPIO[%d]", gpio_num);
	return ESP_OK;
}
