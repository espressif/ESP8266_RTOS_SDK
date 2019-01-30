*************
General Notes
*************

Adding this content here is to improve the user's development efficiency and avoid stepping into known problems.

1. Bootloader
^^^^^^^^^^^^^

V3.1 updated the bootloader to initialize SPI flash I/O mode and clock. So if you are using the V3.0 bootloader,
and now upgrade to the new SDK, please disable the following configuration in the menuconfig:
    
    - "Bootloader config  ---> [ ] Bootloader init SPI flash"
 
2. Sniffer or Smartconfig
^^^^^^^^^^^^^^^^^^^^^^^^^

We moved some functions from IRAM to flash, including `malloc` and `free` fucntions, to save more memory.
In this case, please do not read/write/erase flash during sniffer/promiscuous mode.
You need to disable the sniffer/promiscuous mode at first, then read/write/erase flash. 

3. ESP8285 or ESP8266 + 1MB flash
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

ESP8285 or ESP8266 + 1MB flash can use "Copy OTA Mode" for OTA, more details are in the `examples/system/ota <https://github.com/espressif/ESP8266_RTOS_SDK/tree/master/examples/system/ota/>`_.
