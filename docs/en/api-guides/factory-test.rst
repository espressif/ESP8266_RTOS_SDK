Factory Test
************

1. Overview
===========

The document introduces how to develop, compile, download and run the factory test firmware.

The factory test software development kit is also an example of the SDK, and it is located at :example:`examples/system/factory-test`.

2. Development
==============

Users can use ready-to-use applications directly, or can also add custom application code into the factory test software development kit.

More details of adding customer components, please refer to :doc:`Documentation for the GNU Make based build system <build-system>`.   

Users can just develop the factory test application as normal examples of the SDK.

2.1 Application code
--------------------

Just like other applications, the entry function of factory test application is ``app_main``. It should be added into the source code file of users. 
For example, users can add the ``app_main`` into ``main.c`` of the above sample project.

Users can refer to the source code in file :idf_file:`/examples/system/factory-test/main/main.c` to build custom project.


2.2 Linking address
-------------------

The SDK's partition only supports two applications that named as ``ota_0`` and ``ota_1``.

In this case, we link the factory test firmware to the partition of ``ota_1``.
So, please do not flash the factory test firmware into the partition of ``ota_0``.


3. Compile
==========

To make the bootloader run the ``ota_1(factory test firmware)``, 
please enable the ``GPIO triggers boot from test app partition`` and set the ``correct`` GPIO of your development board in menuconfig::

    Bootloader config  --->
        [*] GPIO triggers boot from test app partition
        (12)  Number of the GPIO input to boot TEST partition

Using the partition table file which has two "OTA" definitions partition::

    Partition Table  --->
        Partition Table (Factory app, two OTA definitions)  --->
            (X) Factory app, two OTA definitions

Enable the console which is used for human-computer interaction::

    Component config  --->
        Virtual file system  --->
            [*] Using espressif VFS

Enable pthread for this function::

    Component config  --->
        PThreads  --->
            [*] Enable pthread

Then call command ``make app2`` in the terminal to compile the firmware which is able to run at ``ota_1`` partition.
The ``Make System`` will start compiling bootloader, partition table file, factory test firmware and so on one by one.


3.1 Special Commands
====================

1. ``make app2``: only compile factory test firmware which is able to run at ``ota_1``, ``with`` bootloader, partition table file and so on

2. ``make app2-flash``: flash(download) only the factory test firmware which is able to run at ``ota_1``, ``without`` bootloader, partition table file and so on

3. ``make app2-flash-all``: flash(download) the factory test firmware which is able to run at ``ota_1``, ``with`` bootloader, partition table file and so on


4. Download
===========

Input command ``make app2-flash-all`` in the terminal to download bootloader, partition table file and factory test firmware which is located at ``ota_1`` one by one.

If users only want to download factory test firmware, please use command ``make app2-flash`` instead.


5. Run
======

Please hold the ``correct`` GPIO, which is configured in the menuconfig in Section 3 ``Compile``, to be low level and power on.
Input command ``make monitor`` in the terminal, and then logs will appear like following::

    ets Jan  8 2013,rst cause:1, boot mode:(3,6)

    load 0x40100000, len 7872, room 16 
    0x40100000: _stext at ??:?

    tail 0
    chksum 0xf1
    load 0x3ffe8408, len 24, room 8 
    tail 0
    chksum 0x78
    load 0x3ffe8420, len 3604, room 8 
    tail 12
    chksum 0x1b
    I (64) boot: ESP-IDF v3.2-dev-354-gba1f90cd-dirty 2nd stage bootloader
    I (64) boot: compile time 13:56:17
    I (72) qio_mode: Enabling default flash chip QIO
    I (73) boot: SPI Speed      : 40MHz
    I (80) boot: SPI Mode       : QIO
    I (86) boot: SPI Flash Size : 2MB
    I (92) boot: Partition Table:
    I (98) boot: ## Label            Usage          Type ST Offset   Length
    I (109) boot:  0 nvs              WiFi data        01 02 00009000 00004000
    I (120) boot:  1 otadata          OTA data         01 00 0000d000 00002000
    I (132) boot:  2 phy_init         RF data          01 01 0000f000 00001000
    I (144) boot:  3 ota_0            OTA app          00 10 00010000 000f0000
    I (155) boot:  4 ota_1            OTA app          00 11 00110000 000f0000
    I (167) boot: End of partition table
    I (173) boot: No factory image, trying OTA 0
    I (5180) boot: Detect a boot condition of the test firmware
    I (5180) esp_image: segment 0: paddr=0x00110010 vaddr=0x40210010 size=0x37b18 (228120) map
    I (5263) esp_image: segment 1: paddr=0x00147b30 vaddr=0x3ffe8000 size=0x00718 (  1816) load
    I (5264) esp_image: segment 2: paddr=0x00148250 vaddr=0x3ffe8718 size=0x0019c (   412) load
    I (5275) esp_image: segment 3: paddr=0x001483f4 vaddr=0x40100000 size=0x084b0 ( 33968) load
    0x40100000: _stext at ??:?

    I (5299) boot: Loaded app from partition at offset 0x110000
    I (5340) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
    I (5340) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
    I (5530) phy_init: phy ver: 1055_12
    I (5530) reset_reason: RTC reset 1 wakeup 0 store 0, reason is 1
    I (5530) factory-test: SDK factory test firmware version:v3.2-dev-354-gba1f90cd-dirty

Then users can input test commands to start factory testing.

6. Test Commands
================

1. ``rftest_init``::

    parameters: no
    
    function: initialize RF to prepare for test

2. ``tx_contin_en <parameter 1>``::

    parameter 1: value 1 means that chip transmits packets continuously with 92% duty cycle,
                 value 0 means that "iqview" test mode

    function: set test mode

3. ``esp_tx <parameter 1> <parameter 2> <parameter 3>``::

    parameter 1: transmit channel which ranges from 1 to 14
    parameter 2: transmit rate which ranges from 0 to 23
    parameter 2: transmit power attenuation which ranges from -127 to 127, unit is 0.25dB

    function: start transmitting Wi-Fi packets

    note 1: command "wifitxout" is the same as "esp_tx"
    note 2: the function can be stopped by command "cmdstop"

4. ``esp_rx <parameter 1> <parameter 2>``::

    parameter 1: transmit channel which ranges from 1 to 14
    parameter 2: transmit rate which ranges from 0 to 23

    function: start receiving Wi-Fi packets

    note 1: the function can be stopped by command "cmdstop"

5. ``wifiscwout <parameter 1> <parameter 2> <parameter 3>``::

    parameter 1: enable signal, value 1 means enable, value 0 means disable
    parameter 2: transmit channel which ranges from 1 to 14
    parameter 3: transmit power attenuation which ranges from -127 to 127, unit is 0.25dB

    function: start transmitting single carrier Wi-Fi packets

    note 1: the function can be stopped by command "cmdstop"

6. ``cmdstop``::

    parameters: no

    function: stop transmitting or receiving Wi-Fi packets

    note 1: command "CmdStop" is the same as "cmdstop"
