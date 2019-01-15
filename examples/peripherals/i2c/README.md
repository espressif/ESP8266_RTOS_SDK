# _I2C Example_

* This example will show you how to use I2C module:
 
    * read external i2c sensor, here we use a MPU6050 sensor for instance.

## Pin assignment

* master:
    * GPIO14 is assigned as the data signal of i2c master port
    * GPIO2 is assigned as the clock signal of i2c master port

## How to use example

### Hardware Required

* Connection:
    * connect sda/scl of sensor with GPIO14/GPIO2
    * no need to add external pull-up resistors, driver will enable internal pull-up resistors.

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

```
I (0) gpio: GPIO[14]| InputEn: 0| OutputEn: 1| OpenDrain: 1| Pullup: 1| Pulldown: 0| Intr:0
I (0) gpio: GPIO[2]| InputEn: 0| OutputEn: 1| OpenDrain: 1| Pullup: 1| Pulldown: 0| Intr:0
I (0) main: *******************

I (0) main: who_am_i: 68

I (0) main: TEMP: 26.51

I (0) main: sensor_data[0]: -1288

I (0) main: sensor_data[1]: 8796

I (0) main: sensor_data[2]: 11088

I (0) main: sensor_data[3]: -3408

I (0) main: sensor_data[4]: -223

I (0) main: sensor_data[5]: 67

I (0) main: sensor_data[6]: -11

I (0) main: error_count: 0

I (0) main: *******************

I (0) main: who_am_i: 68

I (0) main: TEMP: 26.55

I (0) main: sensor_data[0]: -1224

I (0) main: sensor_data[1]: 8748

I (0) main: sensor_data[2]: 11084

I (0) main: sensor_data[3]: -3392

I (0) main: sensor_data[4]: -318

I (0) main: sensor_data[5]: 235

I (0) main: sensor_data[6]: 21

I (0) main: error_count: 0

```
