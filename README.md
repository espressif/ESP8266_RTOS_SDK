# esp_iot_rtos_sdk #

----------

ESP8266 SDK based on FreeRTOS.
   
## Note ##

APIs of "esp_iot_rtos_sdk" are same as "esp_iot_sdk"

More details in "Wiki" !

## Requrements ##

You can use both xcc and gcc to compile your project, gcc is recommended.
For gcc, please refer to [esp-open-sdk](https://github.com/pfalcon/esp-open-sdk).

  
## Compile ##

Clone esp_iot_rtos_sdk, e.g., to ~/esp_iot_rtos_sdk.

    $git clone https://github.com/espressif/esp_iot_rtos_sdk.git

Set sdk path:

	$export SDK_PATH=~/esp_iot_rtos_sdk

Set bin path:

	$export BIN_PATH=~/esp8266_bin

Generated bins will be located here.

SDK_PATH and BIN_PATH **MUST** be set firstly, you can write to .bashrc or other shell init sript.

esp_iot_rtos_sdk/examples/project_template is a project template, you can copy this to anywhere, e.g., to ~/workspace/project_template.

Generate bin: 
	
	./gen_misc.sh
   
Just follow the tips and steps.

## Download ##

eagle.app.v6.flash.bin, downloads to flash 0x00000

eagle.app.v6.irom0text.bin, downloads to flash 0x40000

blank.bin, downloads to flash 0x7E000
