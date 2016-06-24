# ESP8266_RTOS_SDK #

----------

ESP8266 SDK based on FreeRTOS.
   
## Note ##

APIs of "ESP8266_RTOS_SDK" are same as "ESP8266_NONOS_SDK"

More details in "Wiki" !

### about user_rf_cal_sector_set ###

Use this function to set the target flash sector to store RF_CAL parameters. 

The user_rf_cal_sector_set has to be added in application, but need NOT to be called. It will be called inside the SDK.
The system parameter area (4 flash sectors) has already been used, so the RF_CAL parameters will be stored in the target sector set by user_rf_cal_sector_set. Since we do not know which sector is available in user data area, users need to set an available sector in the user_rf_cal_sector_set for the SDK to store RF_CAL parameter. If the user_rf_cal_sector_set is not added in the application, the compilation will fail in link stage.

For example, refer to user_rf_cal_sector_set in SDK/examples/project_template/user/user_main.c

Note:

1. esp_init_data.bin has to be downloaded into flash at least once.
2. Download blank.bin to initialize the sector stored RF_CAL parameter (set by user_rf_cal_sector_set), and download esp_init_data.bin into flash, when the system needs to be initialized, or RF needs to be calibrated again.

## Requrements ##

You can use both xcc and gcc to compile your project, gcc is recommended.
For gcc, please refer to [esp-open-sdk](https://github.com/pfalcon/esp-open-sdk).

  
## Compile ##

Clone ESP8266_RTOS_SDK, e.g., to ~/ESP8266_RTOS_SDK.

    $git clone https://github.com/espressif/ESP8266_RTOS_SDK.git

Modify gen_misc.sh or gen_misc.bat:
For Linux:

    $export SDK_PATH=~/ESP8266_RTOS_SDK
    $export BIN_PATH=~/ESP8266_BIN

For Windows:

    set SDK_PATH=/c/ESP8266_RTOS_SDK
    set BIN_PATH=/c/ESP8266_BIN

ESP8266_RTOS_SDK/examples/project_template is a project template, you can copy this to anywhere, e.g., to ~/workspace/project_template.

Generate bin: 
For Linux:

    ./gen_misc.sh

For Windows:

    gen_misc.bat
   
Just follow the tips and steps.

## Download ##

eagle.app.v6.flash.bin, downloads to flash 0x00000

eagle.app.v6.irom0text.bin, downloads to flash 0x40000

blank.bin, downloads to flash 0x7E000
