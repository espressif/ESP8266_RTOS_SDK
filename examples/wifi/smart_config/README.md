# smartconfig Example

This example shows how ESP8266 connects to AP with ESPTOUCH. Example does the following steps:

* Download ESPTOUCH APP from app store. [Android source code](https://github.com/EspressifApp/EsptouchForAndroid) and [iOS source code](https://github.com/EspressifApp/EsptouchForIOS) is available.

* Compile this example and upload it to an ESP8266.

* Make sure your phone connect to target AP (2.4GHz).

* Open ESPTOUCH app and input password. There will be success message after few sec.

### Example output

Here is an example of smartconfig console output.
```
I (162) boot: Loaded app from partition at offset 0x10000
mode : sta(ec:fa:bc:1d:33:e0)
add if0
SC version: V2.5.4
scandone
scandone
I (5) sc: SC_STATUS_FINDING_CHANNEL

TYPE: ESPTOUCH
T|PHONE MAC: 90 f0 52 0c 16 2d
T|AP MAC   : bc 5f f6 1b e8 1c
I (8) sc: SC_STATUS_GETTING_SSID_PSWD
T|pswd: 1234567890
T|ssid: TEST001
I (17) sc: SC_STATUS_LINK
I (17) sc: SSID:TEST001
I (17) sc: PASSWORD:1234567890
scandone
state: 0 -> 2 (b0)
state: 2 -> 3 (0)
state: 3 -> 5 (10)
add 0
aid 2
pm open phy_2,type:2 0 0
cnt 

connected with TEST001, channel 3
I (22) event: sta ip: 192.168.0.108, mask: 255.255.255.0, gw: 192.168.0.1
I (22) sc: WiFi Connected to ap
I (25) sc: SC_STATUS_LINK_OVER
I (25) sc: Phone ip: 192.168.0.108

I (25) sc: smartconfig over
```
