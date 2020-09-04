# Iperf Example

## Note about iperf version
The iperf example doesn't support all features in standard iperf. It's compitable with iperf version 2.x.

## Introduction
This example implements the protocol used by the common performance measurement tool [iPerf](https://iperf.fr/). 
Performance can be measured between two ESP8266 running this example, or between a single ESP8266 and a computer running the iPerf tool

Demo steps to test station TCP Tx performance: 

1. Build the iperf example with sdkconfig.defaults, which contains performance test specific configurations

2. Run the demo as station mode and join the target AP
   sta ssid password

3. Run iperf as server on AP side
   iperf -s -i 3

4. Run iperf as client on ESP8266 side
   iperf -c 192.168.4.1 -i 3 -t 60

The console output, which is printed by station TCP TX throughput test, looks like:

>esp8266> sta aptest
>
>I (5325) iperf: sta connecting to 'aptest'
>
>esp8266> I (6017) event: ip: 192.168.4.2, mask: 255.255.255.0, gw: 192.168.4.1
>
>esp8266> iperf -s -i 3 -t 60
>
>I (14958) iperf: mode=tcp-server sip=192.168.4.1:5001, dip=0.0.0.0:5001, interval=3, time=60
>
>Interval Bandwidth
>
>esp8266> accept: 192.168.4.2,63201
>
>0-   3 sec       5.50 Mbits/sec
>
>3-   6 sec       6.83 Mbits/sec
>
>6-   9 sec       6.81 Mbits/sec
>
>9-  12 sec       6.88 Mbits/sec
>
>12-  15 sec      6.71 Mbits/sec
>
>15-  18 sec      6.80 Mbits/sec
>
>18-  21 sec      6.83 Mbits/sec


Steps to test station/soft-AP TCP/UDP RX/TX throughput are similar as test steps in station TCP TX.

If you want to improve the performance, need choose the sdkconfig.defaults config to build bin.

1. OS configuration: i. Enable Full cache; ii. CPU frequence 160M.
2. WIFI configuration: i. AMPDU enable; ii. Set RX Buffer Num to 26; iii. Set RX Packet Num to 7; iv. Set TX Packet Num to 6; v. Set WIFI CONTINUOUS RX Buffer Num to 4.
3. LWIP configuration: 1. Set TCP MSS to 1460; ii. Set TCP SND WND to 11680 (8 * MSS); iii. Set TCP RCV WND to 7300 (5 * MSS); iv. Set TCP RECV BOX to 7 (5 + 2).

The following data is calculated by PC iperf with ESP8266 iperf example.

|Mbits/sec|TCP TX|TCP RX|UDP TX|UDP RX|
|:-----:|:-----:|:-----:|:-----:|:-----:|
||15.29|10.56|24.33|42.04|
|Disable ampdu|15.50|11.36|24.12|21.61|
|CPU 80M|10.54|7.91|16.08|25.71|
|Disable Full cache|7.78|6.30|10.32|13.21|
|Disable Full cache, disable ampdu, CPU 80M|5.99|5.19|8.09|11.25|


See the README.md file in the upper level 'examples' directory for more information about examples.
