# Wifi WPS Example

This example shows how to use wps enrollee in esp8266.

Now we support to use enrollee feature in STA and APSTA mode.

* PBC_MODE: Start esp8266 and press the wps button on router, Then esp8266 will get the ssid&password by wps PBC mode.

* PIN_MODE: Start esp8266, It will enter wps mode and you'll see a pin code showing by COM. Enter this pin code in router and the esp8266 can get ssid&password by wps PIN mode.

More info in the code [wps.c](./main/wps.c).
