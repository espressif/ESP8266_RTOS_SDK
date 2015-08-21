#ifndef __PWM_H__
#define __PWM_H__

struct pwm_param {
    uint32 period;
    uint32 freq;
    uint32 duty[8];
};

#define PWM_DEPTH 1023

void pwm_init(uint32 period, uint32 *duty, uint32 pwm_channel_num, uint32 (*pin_info_list)[3]);

void pwm_set_duty(uint32 duty, uint8 channel);
uint32 pwm_get_duty(uint8 channel);
void pwm_set_period(uint32 period);
uint32 pwm_get_period(void);

#endif
