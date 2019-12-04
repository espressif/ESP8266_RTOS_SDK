# ESP8266 RTOS Software Development Kit

[![Documentation Status](https://readthedocs.com/projects/espressif-esp8266-rtos-sdk/badge/?version=latest)](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/?badge=latest)


# ** IMPORTANT NOTICE **

## About this repository
A new branching model is applied to this repository, which consists of a master branch and release branches.

### 1. Master branch
The master branch is an integration branch where bug fixes/features are gathered for compiling and functional testing.

### 2. Release branch
The release branch is where releases are maintained and hot fixes (with names like *release/v2.x.x*) are added.
Please ensure that all your production-related work are tracked with the release branches.

With this new model, we can push out bug fixes more quickly and achieve simpler maintenance.

## Roadmap
*ESP8266_RTOS_SDK*'s framework is quite outdated and different from the current *[esp-idf](https://github.com/espressif/esp-idf)* and we are planning to migrate *ESP8266_RTOS_SDK* to *esp-idf* eventually after *v2.0.0*.

However, we will firstly provide a new version of ESP8266 SDK (*ESP8266_RTOS_SDK v3.0*), which shares the same framework with *esp-idf* (esp-idf style), as a work-around, because the multi-CPU architecture is not supported by *esp-idf* for the time being.

Actions to be taken for *ESP8266_RTOS_SDK v3.0* include the following items:

1. Modify the framework to esp-idf style
2. Restructure some core libraries including Wi-Fi libraries and libmain
3. Update some third-party libraries including FreeRTOS, lwIP, mbedTLS, noPoll, libcoap, SPIFFS, cJSON, wolfSSL, etc.
4. Update some drivers
5. Others

---

# Developing With the ESP8266_RTOS_SDK

## Get toolchain

v5.2.0

* [Windows](https://dl.espressif.com/dl/xtensa-lx106-elf-win32-1.22.0-100-ge567ec7-5.2.0.zip)
* [Mac](https://dl.espressif.com/dl/xtensa-lx106-elf-macos-1.22.0-100-ge567ec7-5.2.0.tar.gz)
* [Linux(64)](https://dl.espressif.com/dl/xtensa-lx106-elf-linux64-1.22.0-100-ge567ec7-5.2.0.tar.gz)
* [Linux(32)](https://dl.espressif.com/dl/xtensa-lx106-elf-linux32-1.22.0-100-ge567ec7-5.2.0.tar.gz)

If you are still using old version SDK(< 3.0), please use toolchain v4.8.5, as following:

* [Windows](https://dl.espressif.com/dl/xtensa-lx106-elf-win32-1.22.0-88-gde0bdc1-4.8.5.tar.gz)
* [Mac](https://dl.espressif.com/dl/xtensa-lx106-elf-osx-1.22.0-88-gde0bdc1-4.8.5.tar.gz)
* [Linux(64)](https://dl.espressif.com/dl/xtensa-lx106-elf-linux64-1.22.0-88-gde0bdc1-4.8.5.tar.gz)
* [Linux(32)](https://dl.espressif.com/dl/xtensa-lx106-elf-linux32-1.22.0-88-gde0bdc1-4.8.5.tar.gz)

## Get ESP8266_RTOS_SDK

Besides the toolchain (that contains programs to compile and build the application), you also need ESP8266 specific API / libraries. They are provided by Espressif in [ESP8266_RTOS_SDK](https://github.com/espressif/ESP8266_RTOS_SDK) repository. To get it, open terminal, navigate to the directory you want to put ESP8266_RTOS_SDK, and clone it using `git clone` command:

```
cd ~/esp
git clone https://github.com/espressif/ESP8266_RTOS_SDK.git
```

ESP8266_RTOS_SDK will be downloaded into `~/esp/ESP8266_RTOS_SDK`.

## Setup Path to ESP8266_RTOS_SDK

The toolchain programs access ESP8266_RTOS_SDK using `IDF_PATH` environment variable. This variable should be set up on your PC, otherwise projects will not build. Setting may be done manually, each time PC is restarted. Another option is to set up it permanently by defining `IDF_PATH` in user profile.

For manually, the command:
```
export IDF_PATH=~/esp/ESP8266_RTOS_SDK
```

## Start a Project
Now you are ready to prepare your application for ESP8266. To start off quickly, we can use `examples/get-started/hello_world` project from `examples` directory in SDK.

Once you've found the project you want to work with, change to its directory and you can configure and build it.

## Connect

You are almost there. To be able to proceed further, connect ESP8266 board to PC, check under what serial port the board is visible and verify if serial communication works. Note the port number, as it will be required in the next step.

## Configuring the Project

Being in terminal window, go to directory of `hello_world` application by typing `cd ~/esp/ESP8266_RTOS_SDK/examples/get-started/hello_world`. Then start project configuration utility `menuconfig`:

```
cd ~/esp/ESP8266_RTOS_SDK/examples/get-started/hello_world
make menuconfig
```

In the menu, navigate to `Serial flasher config` > `Default serial port` to configure the serial port, where project will be loaded to. Confirm selection by pressing enter, save configuration by selecting `< Save >` and then exit application by selecting `< Exit >`.

> Note:
	On Windows, serial ports have names like COM1. On MacOS, they start with `/dev/cu.`. On Linux, they start with `/dev/tty`.

Here are couple of tips on navigation and use of `menuconfig`:

* Use up & down arrow keys to navigate the menu.
* Use Enter key to go into a submenu, Escape key to go out or to exit.
* Type `?` to see a help screen. Enter key exits the help screen.
* Use Space key, or `Y` and `N` keys to enable (Yes) and disable (No) configuration items with checkboxes "`[*]`"
* Pressing `?` while highlighting a configuration item displays help about that item.
* Type `/` to search the configuration items.

Once done configuring, press Escape multiple times to exit and say "Yes" to save the new configuration when prompted.

## Compiling the Project

`make all`

... will compile app based on the config.

## Flashing the Project

When `make all` finishes, it will print a command line to use esptool.py to flash the chip. However you can also do this from make by running:

`make flash`

This will flash the entire project (app, bootloader and init data bin) to a new chip. The settings for serial port flashing can be configured with `make menuconfig`.

You don't need to run `make all` before running `make flash`, `make flash` will automatically rebuild anything which needs it.

## Viewing Serial Output

The `make monitor` target uses the [idf_monitor tool](https://esp-idf.readthedocs.io/en/latest/get-started/idf-monitor.html) to display serial output from the ESP32. idf_monitor also has a range of features to decode crash output and interact with the device. [Check the documentation page for details](https://esp-idf.readthedocs.io/en/latest/get-started/idf-monitor.html).

Exit the monitor by typing Ctrl-].

To flash and monitor output in one pass, you can run:

`make flash monitor`

## Compiling & Flashing Just the App

After the initial flash, you may just want to build and flash just your app, not the bootloader and init data bin:

* `make app` - build just the app.
* `make app-flash` - flash just the app.

`make app-flash` will automatically rebuild the app if it needs it.

(In normal development there's no downside to reflashing the bootloader and init data bin each time, if they haven't changed.)

> Note:
> Recommend to use these 2 commands if you have flashed bootloader and init data bin.

## Parallel Builds

ESP8266_RTOS_SDK supports compiling multiple files in parallel, so all of the above commands can be run as `make -jN` where `N` is the number of parallel make processes to run (generally N should be equal to or one more than the number of CPU cores in your system.)

Multiple make functions can be combined into one. For example: to build the app & bootloader using 5 jobs in parallel, then flash everything, and then display serial output from the ESP32 run:

```
make -j5 app-flash monitor
```

## Erasing Flash

The `make flash` target does not erase the entire flash contents. However it is sometimes useful to set the device back to a totally erased state. To erase the entire flash, run `make erase_flash`.

This can be combined with other targets, ie `make erase_flash flash` will erase everything and then re-flash the new app, bootloader and init data bin.

## Updating ESP8266_RTOS_SDK

After some time of using ESP8266_RTOS_SDK-IDF, you may want to update it to take advantage of new features or bug fixes. The simplest way to do so is by deleting existing `ESP8266_RTOS_SDK` folder and cloning it again.

Another solution is to update only what has changed. This method is useful if you have a slow connection to GitHub. To do the update run the following commands::

```
cd ~/esp/ESP8266_RTOS_SDK
git pull
```

The ``git pull`` command is fetching and merging changes from ESP8266_RTOS_SDK repository on GitHub.
