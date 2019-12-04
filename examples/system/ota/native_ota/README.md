# Native OTA

We split native OTA example into several sub-examples to let custemors to choose which application scenario is they really want.

The projects' directory structure is following:

```
    1MB_flash --->                  # Only for ESP8266 + 1MB flash or ESP8285

        new_to_new                  # current firmware is based on new SDK and it does not update from old SDK's firmware
        (no old, no copy)           # new firmware is based on new SDK
                                    # it can use copy mode, but here not use


        new_to_new                  # current firmware is based on new SDK and it does not update from old SDK's firmware
        (no old, copy)              # new firmware is based on new SDK
                                    # it uses copy mode
                                    # copy mode: copy firmware from OTA1 partition to OTA0 partition and run OTA0's app, so only 1 firmware is needed


        new_to_new(old)             # current firmware is based on new SDK and it updates from old SDK's firmware
                                    # new firmware is based on new SDK
                                    # it must use copy mode


    2+MB_flash --->                 # Only for ESP8266 + 2MB(or larger size) flash

        new_to_new(no old)          # current firmware is based on new SDK and it does not update from old SDK's firmware
                                    # new firmware is based on new SDK


        new_to_new(old)             # current firmware is based on new SDK and it updates from old SDK's firmware
                                    # new firmware is based on new SDK
```
 