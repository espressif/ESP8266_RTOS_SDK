# _ledc Example_

- Ledc is implemented by pwm, and this example will show you how to use ledc module by running four channels, but this is only designed to be compatible with the ESP32 ledc interface, many parameters are actually invalid.
- Observe ledc signal with logic analyzer or oscilloscope.
- Since ledc is based on PWM, using ledc will occupy PWM channel.
- Since the step value of the gradient is 10ms, there may be an error of about 10ms between different channels.

## Pin assignment

    * GPIO13 is assigned as the LEDC_HS_CH0_GPIO .
    * GPIO14 is assigned as the LEDC_HS_CH1_GPIO .
    * GPIO15 is assigned as the LEDC_HS_CH2_GPIO .
    * GPIO12 is assigned as the LEDC_HS_CH3_GPIO .

## How to use example

### Hardware Required

- Connection:
  - Connect the ledc channel to a logic analyzer or oscilloscope.

### Configure the project

```
make menuconfig
```

- Set serial port under Serial Flasher Options.

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
make -j4 flash monitor
```

(To exit the serial monitor, type `Ctrl-]`.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

## Example Output

```

I (497) gpio: GPIO[12]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (507) gpio: GPIO[13]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (517) gpio: GPIO[14]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (527) gpio: GPIO[15]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (547) pwm: --- PWM v3.2

I (547) ledc: ledc_channel_max:4

I (557) ledc: gpio:13

I (567) ledc: gpio:14

I (567) ledc: gpio:15

I (567) ledc: gpio:12

I (577) main: 1. LEDC fade up to duty = 980

I (587) ledc: channel_num = 0 | duty = 980; duty_p = 0 | step_duty = 3 | step_01duty = 2 | step_001duty = 6

I (597) ledc: channel_num = 1 | duty = 980; duty_p = 0 | step_duty = 3 | step_01duty = 2 | step_001duty = 6

I (617) ledc: channel_num = 2 | duty = 980; duty_p = 0 | step_duty = 3 | step_01duty = 2 | step_001duty = 6

I (637) ledc: channel_num = 3 | duty = 980; duty_p = 0 | step_duty = 3 | step_01duty = 2 | step_001duty = 6

I (647) ledc: channel0 is start
I (657) ledc: channel1 is start
I (657) ledc: channel2 is start
I (667) ledc: channel3 is start
I (3677) ledc: channel0 is end
I (3677) ledc: channel1 is end
I (3677) ledc: channel2 is end
I (3677) ledc: channel3 is end

```

- WAVE FORM:

![wave](wave.png)
