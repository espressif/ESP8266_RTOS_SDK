# _SPI Slave Example_  

_This example uses ESP8266 hspi Slave to send and receive data to another ESP8266 hspi Master_

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
I (500) spi_slave_example: init gpio
I (500) gpio: GPIO[4]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (520) spi_slave_example: init spi
I (3390) spi_slave_example: Slave wrote 3200 bytes in 4632 us
I (3490) spi_slave_example: Slave wrote 3200 bytes in 4616 us
I (3590) spi_slave_example: Slave wrote 3200 bytes in 4622 us
I (3690) spi_slave_example: Slave wrote 3200 bytes in 4611 us
I (3790) spi_slave_example: Slave wrote 3200 bytes in 4612 us
I (3890) spi_slave_example: Slave wrote 3200 bytes in 4612 us
I (3990) spi_slave_example: Slave wrote 3200 bytes in 4619 us
I (4090) spi_slave_example: Slave wrote 3200 bytes in 4607 us
I (4190) spi_slave_example: Slave wrote 3200 bytes in 4613 us
I (4290) spi_slave_example: Slave wrote 3200 bytes in 4609 us
I (4390) spi_slave_example: Slave wrote 3200 bytes in 4618 us
I (4490) spi_slave_example: Slave wrote 3200 bytes in 4619 us
I (4590) spi_slave_example: Slave wrote 3200 bytes in 4614 us
I (4690) spi_slave_example: Slave wrote 3200 bytes in 4613 us

```

* WAVE FORM:  

  - SPI_MASTER_WRITE_DATA_TO_SLAVE

    ![wave](wave_write_to_slave.png)  

  - SPI_MASTER_READ_DATA_FROM_SLAVE

    ![wave](wave_read_from_slave.png)