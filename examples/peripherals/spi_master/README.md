# _SPI Master Example_  

_This example uses the ESP8266 hspi Master to send and receive data to another ESP8266 hspi Slave_

## How to use example  

### Hardware Required  

* Connection:  

| Signal    | Slave  | Master |
|-----------|--------|--------|
| SCLK      | GPIO14 | GPIO14 |
| MISO      | GPIO12 | GPIO12 |
| MOSI      | GPIO13 | GPIO13 |
| CS        | GPIO15 | GPIO15 |
| HANDSHARK | GPIO4  | GPIO4  |
| GND       | GND    | GND    |

* Note:

When the ESP8266 is powered on, it is necessary to keep the GPIO15 low to enter the Flash mode, so the Master and the Slave have different power-on sequences.

```
Master OFF -> Slave ON -> Master ON
```

### Configure the project  

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

* LOG:  

```
I (516) spi_master_example: init gpio
I (526) gpio: GPIO[4]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:1 
I (536) spi_master_example: init spi
I (556) spi_master_example: Master wrote 3200 bytes in 4302 us
I (656) spi_master_example: Master wrote 3200 bytes in 4519 us
I (766) spi_master_example: Master wrote 3200 bytes in 4522 us
I (866) spi_master_example: Master wrote 3200 bytes in 4520 us
I (966) spi_master_example: Master wrote 3200 bytes in 4521 us
I (1066) spi_master_example: Master wrote 3200 bytes in 4520 us
I (1166) spi_master_example: Master wrote 3200 bytes in 4522 us
I (1266) spi_master_example: Master wrote 3200 bytes in 4521 us
I (1366) spi_master_example: Master wrote 3200 bytes in 4520 us
I (1466) spi_master_example: Master wrote 3200 bytes in 4520 us
I (1566) spi_master_example: Master wrote 3200 bytes in 4520 us
I (1666) spi_master_example: Master wrote 3200 bytes in 4519 us
I (1766) spi_master_example: Master wrote 3200 bytes in 4521 us
I (1866) spi_master_example: Master wrote 3200 bytes in 4519 us
I (1966) spi_master_example: Master wrote 3200 bytes in 4520 us
```

* WAVE FORM:  

  - SPI_MASTER_WRITE_DATA_TO_SLAVE

    ![wave](wave_write_to_slave.png)  

  - SPI_MASTER_READ_DATA_FROM_SLAVE

    ![wave](wave_read_from_slave.png)