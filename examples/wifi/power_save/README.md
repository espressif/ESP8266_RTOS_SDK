# Wifi Power Save Example

This example shows how to use power save mode of wifi.

Power save mode only works in station mode. If the modem sleep mode is enabled, station will switch between active and sleep state periodically after connecting to AP successfully. Station can keep connection with AP in modem sleep mode.

* No power save: This is default mode. And the esp8266 will work with full power.

* Minimum modem sleep: In minimum modem sleep mode, station wakes up every DTIM to receive beacon. In sleep state, RF, PHY and BB are turned off in order to reduce power consumption. .

* Maximum modem sleep: In maximum modem sleep mode, station wakes up every DTIM to receive beacon. In sleep state, CPU, RF, PHY and BB are turned off in order to reduce power consumption. .

* others: not supported yet.
