*************
General Notes
*************

Adding this content here is to improve the user's development efficiency and avoid stepping into known problems.

1. Bootloader
^^^^^^^^^^^^^

V3.1 updated the bootloader to initialize SPI flash I/O mode and clock. So if you are using the V3.0 bootloader,
and now upgrade to the new SDK, please disable the following configuration in the menuconfig:

::

    "Bootloader config  --->
        [ ] Bootloader init SPI flash"

2. OTA
^^^^^^

We split the native OTA example into several sub-examples to let custemors to choose which application matches the scenario they really want. `examples/system/ota/native_ota <https://github.com/espressif/ESP8266_RTOS_SDK/tree/master/examples/system/ota/native_ota/>`_.

3. 802.11n only AP
^^^^^^^^^^^^^^^^^^

For better compatibility, the SDK is in bg mode by default. And application can set it to be bgn mode for reconnecting when it fails to connect some 11n only APs, refer to the `examples/wifi/simple_wifi <https://github.com/espressif/ESP8266_RTOS_SDK/tree/master/examples/wifi/simple_wifi/>`_.

4. JTAG I/O
^^^^^^^^^^^

In some cases, if enable JTAG I/O (default options), it will cost some more current so that the hardware will cost more power.
So if users don't use Jtag or these GPIOs directly and want to save more power, please enable this option in the menuconfig:

::

    "Bootloader config  --->
        [ ] Bootloader disable JTAG I/O"
