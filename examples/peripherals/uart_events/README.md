# _UART Events Example_  

_This example shows how to use the UART driver to handle special UART events. It also reads data from UART0 directly, and echoes it to console._

* Compile and load example from terminl running `make flash monitor`
* Being in 'monotor' type samething to see the `UART_DATA` events and the typed data displayed.

## How to use example

### Configure the project

```
make menuconfig
```

* Set serial port under Serial Flasher Options.  
* `make monitor` baud rate set to what you set in the example.


### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
make -j4 flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

## Example Output  

* Paste `0123456789` to monitor, the `UART DATA` event will be received.

```
I (185) boot: Loaded app from partition at offset 0x10000
I (0) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (0) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (0) uart: queue free spaces: 100
I (15) uart_events: uart[0] event:
I (15) uart_events: [UART DATA]: 10
I (15) uart_events: [DATA EVT]:
0123456789
```