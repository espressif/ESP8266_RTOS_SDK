# IR TX Example

(See the README.md file in the upper level 'examples' directory for more information about examples.)

In this example, we use IO14 to transmit the nec infrared signal and i2s to generate the 38Khz carrier.

## How to Use Example

### Hardware Required

* A development board with ESP8266 SoC (e.g., ESP8266-DevKitC, etc.)
* A USB cable for power supply and programming

### Configure the Project

```
make menuconfig
```

* Set serial port under Serial Flasher Options.

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
make -j4 flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

## Example Output

```
I (467) gpio: GPIO[14]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (937) main: ir tx nec: addr:55h;cmd:00h;repeat:4
I (2377) main: ir tx nec: addr:55h;cmd:01h;repeat:4
I (3817) main: ir tx nec: addr:55h;cmd:02h;repeat:4
I (5257) main: ir tx nec: addr:55h;cmd:03h;repeat:4
I (6697) main: ir tx nec: addr:55h;cmd:04h;repeat:4
I (8137) main: ir tx nec: addr:55h;cmd:05h;repeat:4
```

If you have a logic analyzer, you can use a logic analyzer to grab online data. The following table describes the pins we use by default (Note that you can also use other pins for the same purpose).

| pin name| function | gpio_num |
|:---:|:---:|:---:|
| IR TX|Infrared emission | GPIO_NUM_14 |

## Troubleshooting

* Program upload failure

    * Hardware connection is not correct: run `make monitor`, and reboot your board to see if there are any output logs.
    * The baud rate for downloading is too high: lower your baud rate in the `menuconfig` menu, and try again.

For any technical queries, please open an [issue](https://github.com/espressif/ESP8266_RTOS_SDK/issues) on GitHub. We will get back to you soon.