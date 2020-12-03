# smartconfig Example

This example shows how ESP8266 connects to AP with ESPTOUCH. Example does the following steps:

* Download ESPTOUCH APP from app store. [Android source code](https://github.com/EspressifApp/EsptouchForAndroid) and [iOS source code](https://github.com/EspressifApp/EsptouchForIOS) is available.

* Compile this example and upload it to an ESP8266.

* Make sure your phone connect to target AP (2.4GHz).

* Make sure that the smartconfig type selected on your device corresponds to the APP.

* Open ESPTOUCH app and input password. There will be success message after few sec.

## Support ESP-Touch-V2

* ESP-Touch-V2 is a new SmartConfig method developed by Espressif. With ESP-Touch-V2, the configuration time and success rate of connecting to an AP are highly improved when compares to the old version ESP-Touch, which we call it "ESP-Touch-V1" in this doc.  

* New features: In addition to get ssid and password, ESP-Touch-V2 supports to get user-defined reserved data and encrypted transmission.  

**ESP-Touch-V2 is not compatible with the old ESP-Touch-V1. It is recommended to use ESP-Touch-V2 in your new products, not your existing ESP-Touch-V1 products.**  

* Since ESP-Touch-V2 is implemented in a new way, it is not compatible with the old ESP-Touch-V1. The phone APP is also not compatible between V1 and V2. So, if you already have a product using ESP-Touch-V1, we do not recommend to update it to ESP-Touch-V2.  
If your phone APP has already supported ESP-Touch-V1, and wants to add ESP-Touch-V2 for future new products, then your phone APP must able to choose different SmartConfig methods, V1 or V2, otherwise, keep using ESP-Touch-V1 is the best choice.

## How to use example

### Configure the project

```
idf.py menuconfig
```

* Set serial port under Serial Flasher Options.

* Set WiFi SSID and WiFi Password and Maximal STA connections under Example Configuration Options.

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

## Example output

Here is an example of smartconfig (ESP-Touch) console output.
```
I (369) boot: Loaded app from partition at offset 0x10000
I (403) system_api: Base MAC address is not set, read default base MAC address from EFUSE
I (408) system_api: Base MAC address is not set, read default base MAC address from EFUSE
phy_version: 1163.0, 665d56c, Jun 24 2020, 10:00:08, RTOS new
I (478) phy_init: phy ver: 1163_0
I (497) wifi:pm stop
SC version: V3.0.0, TYPE: 0
sync_len: [60,643]
I (3689) smartconfig_example: Scan done
TYPE: ESPTOUCH, ch:13
T|PHONE MAC: e0:b5:5f:e5:05:9f
T|AP MAC: 8c:ab:8e:bb:82:08
I (51619) smartconfig_example: Found channel
T|pswd: espressif
T|ssid: TEST001
I (52877) smartconfig_example: Got SSID and password
I (52880) smartconfig_example: SSID:TEST001
I (52882) smartconfig_example: PASSWORD:espressif
I (58070) wifi:state: 0 -> 2 (b0)
I (58079) wifi:state: 2 -> 3 (0)
I (58088) wifi:state: 3 -> 5 (10)
I (58377) wifi:connected with TEST001, aid = 4, channel 13, HT20, bssid = 8c:ab:8e:bb:82:08
I (61567) tcpip_adapter: sta ip: 192.168.103.154, mask: 255.255.255.0, gw: 192.168.103.1
I (61574) smartconfig_example: WiFi Connected to ap
I (67667) smartconfig_example: smartconfig over

```
Here is an example of smartconfig (ESP-Touch-V2) console output.
```
I (369) boot: Loaded app from partition at offset 0x10000
I (396) system_api: Base MAC address is not set, read default base MAC address from EFUSE
I (401) system_api: Base MAC address is not set, read default base MAC address from EFUSE
phy_version: 1163.0, 665d56c, Jun 24 2020, 10:00:08, RTOS new
I (471) phy_init: phy ver: 1163_0
I (491) wifi:pm stop
SC version: V3.0.0, TYPE: 3
sync_len: [127,1200]
I (3683) smartconfig_example: Scan done
lock channel: 13
TYPE: ESPTOUCH_V2, ch: 13
offset:52 tods: 1
I (5528) smartconfig_example: Found channel
lock channel: 13
offset:52 tods: 0
total_seq: 7
I (15033) smartconfig_example: Got SSID and password
I (15036) smartconfig_example: SSID:TEST001
I (15039) smartconfig_example: PASSWORD:espressif
I (15044) smartconfig_example: RVD_DATA:1234567890
I (15414) wifi:state: 0 -> 2 (b0)
I (15468) wifi:state: 2 -> 3 (0)
I (15475) wifi:state: 3 -> 5 (10)
I (15821) wifi:connected with TEST001, aid = 4, channel 13, HT20, bssid = 8c:ab:8e:bb:82:08
I (16409) tcpip_adapter: sta ip: 192.168.103.154, mask: 255.255.255.0, gw: 192.168.103.1
I (16415) smartconfig_example: WiFi Connected to ap
I (22448) smartconfig_example: smartconfig over

```