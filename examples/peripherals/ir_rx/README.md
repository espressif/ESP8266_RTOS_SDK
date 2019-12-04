# IR RX Example

(See the README.md file in the upper level 'examples' directory for more information about examples.)

In this example, we use IO5 to recv the nec infrared signal

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
I (471) gpio: GPIO[5]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:2 
I (19161) main: addr1: 0x55, addr2: 0xaa, cmd1: 0x0, cmd2: 0xff
I (19171) main: ir rx nec data:  0x0
I (19211) main: addr1: 0x55, addr2: 0xaa, cmd1: 0x0, cmd2: 0xff
I (19221) main: ir rx nec data:  0x0
I (19321) main: addr1: 0x55, addr2: 0xaa, cmd1: 0x0, cmd2: 0xff
I (19331) main: ir rx nec data:  0x0
I (19431) main: addr1: 0x55, addr2: 0xaa, cmd1: 0x0, cmd2: 0xff
I (19431) main: ir rx nec data:  0x0
I (19541) main: addr1: 0x55, addr2: 0xaa, cmd1: 0x0, cmd2: 0xff
I (19541) main: ir rx nec data:  0x0
I (20601) main: addr1: 0x55, addr2: 0xaa, cmd1: 0x1, cmd2: 0xfe
I (20601) main: ir rx nec data:  0x1
I (20651) main: addr1: 0x55, addr2: 0xaa, cmd1: 0x1, cmd2: 0xfe
I (20661) main: ir rx nec data:  0x1
I (20761) main: addr1: 0x55, addr2: 0xaa, cmd1: 0x1, cmd2: 0xfe
I (20761) main: ir rx nec data:  0x1
I (20871) main: addr1: 0x55, addr2: 0xaa, cmd1: 0x1, cmd2: 0xfe
I (20871) main: ir rx nec data:  0x1
I (20981) main: addr1: 0x55, addr2: 0xaa, cmd1: 0x1, cmd2: 0xfe
I (20981) main: ir rx nec data:  0x1
I (22041) main: addr1: 0x55, addr2: 0xaa, cmd1: 0x2, cmd2: 0xfd
I (22041) main: ir rx nec data:  0x2
I (22091) main: addr1: 0x55, addr2: 0xaa, cmd1: 0x2, cmd2: 0xfd
I (22091) main: ir rx nec data:  0x2
I (22201) main: addr1: 0x55, addr2: 0xaa, cmd1: 0x2, cmd2: 0xfd
I (22201) main: ir rx nec data:  0x2
I (22311) main: addr1: 0x55, addr2: 0xaa, cmd1: 0x2, cmd2: 0xfd
I (22311) main: ir rx nec data:  0x2
I (22411) main: addr1: 0x55, addr2: 0xaa, cmd1: 0x2, cmd2: 0xfd
I (22421) main: ir rx nec data:  0x2
```

If you have a logic analyzer, you can use a logic analyzer to grab online data. The following table describes the pins we use by default (Note that you can also use other pins for the same purpose).

| pin name| function | gpio_num |
|:---:|:---:|:---:|
| IR RX|Infrared reception | GPIO_NUM_5 |

## Troubleshooting

* Program upload failure

    * Hardware connection is not correct: run `make monitor`, and reboot your board to see if there are any output logs.
    * The baud rate for downloading is too high: lower your baud rate in the `menuconfig` menu, and try again.

For any technical queries, please open an [issue](https://github.com/espressif/ESP8266_RTOS_SDK/issues) on GitHub. We will get back to you soon.