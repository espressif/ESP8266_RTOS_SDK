/*
 * ESPRESSIF MIT License
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

#ifndef __PWM_H__
#define __PWM_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
  * @brief  PWM function initialization, including GPIO, frequency and duty cycle.
  *
  * @attention This API can be called only once.
  *
  * @param  uint32_t period : PWM period, unit : us.
  *         e.g.: For 1KHz PWM, period is 1000 us.
  * @param  uint32_t *duty : duty cycle of each channels.
  * @param  uint32_t pwm_channel_num : PWM channel number, maximum is 8
  * @param  uint32_t (*pin_info_list)[3] : GPIO parameter of PWM channel, it is a pointer
  *         of n x 3 array which defines GPIO register, IO reuse of corresponding pin and GPIO number.
  *
  * @return  null
  */
void pwm_init(uint32_t period, uint32_t *duty, uint32_t pwm_channel_num, uint32_t(*pin_info_list)[3]);

/**
  * @brief   Set the duty cycle of a PWM channel.
  *          Set the time that high level or low(if you reverse the output of this channel)
  *          signal will last, the duty cycle cannot exceed the period.
  *        
  * @attention After set configuration, pwm_start needs to be called to take effect.
  *
  * @param   uint32_t duty : duty cycle
  * @param   uint8_t channel_num : PWM channel number
  *          the channel_num cannot exceed the value initialized by pwm_init.
  *
  * @return  null
  */
void pwm_set_duty(uint32_t duty, uint8_t channel_num);

/**
  * @brief   Get the duty cycle of a PWM channel.
  *
  * @param   uint8_t channel_num : PWM channel number
  *          the channel_num cannot exceed the value initialized by pwm_init.
  *
  * @return  Duty cycle of specified channel
  */
uint32_t pwm_get_duty(uint8_t channel_num);

/**
  * @brief   Set PWM period, unit : us.
  *
  * @attention After set configuration, pwm_start needs to be called to take effect.
  *
  * @param   uint32_t period : PWM period, unit : us
  *          For example, for 1KHz PWM, period is 1000.
  *
  * @return  null
  */
void pwm_set_period(uint32_t period);

/**
  * @brief   Get PWM period, unit : us.
  *
  * @param   null
  *
  * @return  PWM period, unit : us
  */
uint32_t pwm_get_period(void);

/**
  * @brief   Starts PWM. 
  *
  * @attention This function needs to be called after PWM configuration is changed.
  *
  * @param   null
  *
  * @return  null
  */
void pwm_start(void);

/**
  * @brief  Stop all PWM channel.
  *         Stop PWM and set the output of each channel to the specified level.
  *         Calling pwm_start can re-start PWM output.
  *
  * @param  uint32_t stop_level : Out put level after PWM is stoped
  *         e.g.: We initialize 8 channels, if stop_level_mask = 0x0f,
  *         channel 0,1,2 and 3 will output high level, and channel 4,5,6 and 7 will output low level.
  *
  * @return null
  */
void pwm_stop(uint32_t stop_level_mask);

/**
  * @brief  Set the duty cycle of all channels.
  *
  * @attention After set configuration, pwm_start needs to be called to take effect.
  *
  * @param  uint32_t *duty : An array that store the duty cycle of each channel,
  *         the array elements number needs to be the same as the number of channels.
  *
  * @return null
  */
void pwm_set_dutys(uint32_t *duty);

/**
  * @brief   Set the phase of a PWM channel.
  *
  * @attention After set configuration, pwm_start needs to be called to take effect.
  *
  * @param   int phase : The phase of this PWM channel, the phase range is (-180 ~ 180).
  * @param   uint8_t channel_num : PWM channel number
  *          the channel_num cannot exceed the value initialized by pwm_init.
  *
  * @return  null
  */
void pwm_set_phase(int phase, uint8_t channel_num);

/**
  * @brief   Set the phase of all channels.
  *
  * @attention After set configuration, pwm_start needs to be called to take effect.
  *
  * @param   int *phase : An array that store the phase of each channel,
  *          the array elements number needs to be the same as the number of channels.
  *
  * @return  null
  */
void pwm_set_phases(int *phase);

/**
  * @brief   Get the phase of a PWM channel.
  *
  * @param   uint8_t channel_num : PWM channel number
  *          the channel_num cannot exceed the value initialized by pwm_init.
  *
  * @return  PWM phase of specified channel.
  */
int pwm_get_phase(uint8_t channel_num);

/**
  * @brief   Set PWM period and duty of each PWM channel.
  *
  * @attention After set configuration, pwm_start needs to be called to take effect.
  *
  * @param   uint32_t period : PWM period, unit : us
  *          For example, for 1KHz PWM, period is 1000.
  * @param  uint32_t *duty : An array that store the duty cycle of each channel,
  *         the array elements number needs to be the same as the number of channels.
  *
  * @return  null
  */
void pwm_set_period_dutys(uint32_t period, uint32_t *duty);

/**
  * @brief  Set the inverting output PWM channel.
  *
  * @attention After set configuration, pwm_start needs to be called to take effect.
  *
  * @param  uint16_t channel_mask : The channel bitmask that used to reverse the output
  *         e.g.: We initialize 8 channels, if channel_mask = 0x0f, channels 0, 1, 2 and 3 will reverse the output.
  *
  * @return null
  */
void pwm_set_channel_reverse(uint16_t channel_mask);

/**
  * @brief  Clear the inverting output PWM channel.
  *         This function only works for the PWM channel that is already in the inverted output states.
  *
  * @attention After set configuration, pwm_start needs to be called to take effect.
  *
  * @param  uint16_t channel_mask : The channel bitmask that need to clear
  *         e.g.: The outputs of channels 0, 1, 2 and 3 are already in inverted state. If channel_mask = 0x07,
  *         the output of channel 0, 1, and 2 will return to normal, the channel 3 will keep inverting output.
  *
  * @return  null
  */
void pwm_clear_channel_reverse(uint16_t channel_mask);


#ifdef __cplusplus
}
#endif

#endif