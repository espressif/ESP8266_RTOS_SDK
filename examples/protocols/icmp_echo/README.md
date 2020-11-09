# ICMP Echo-Reply (Ping) example

(See the README.md file in the upper level 'examples' directory for more information about examples.)

Ping is a useful network utility used to test if a remote host is reachable on the IP network. It measures the round-trip time for messages sent from the source host to a destination target that are echoed back to the source. 

Ping operates by sending Internet Control Message Protocol (ICMP) echo request packets to the target host and waiting for an ICMP echo reply.

**Notes:** Currently this example supports IPv4 & IPv6. When ping IPv6 address should set interface (-I) to 2.

## How to use example

### Hardware Required

This example should be able to run on any commonly available ESP8266 development board.

### Configure the project

```
idf.py menuconfig
```

In the `Example Connection Configuration` menu:

* If Wi-Fi interface is selected, you also have to set:
  * Wi-Fi SSID and Wi-Fi password that your board will connect to.

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT flash monitor
```

(Replace PORT with the name of the serial port to use.)

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP8266_RTOS_SDK to build projects.

## Example Output

```bash
==========================================================================
 |               PING help command                                         |
 |      send ICMP ECHO_REQUEST to network hosts                            |
 |                                                                         |
 |  ping  [-W <t>] [-i <t>] [-s <n>] [-c <n>] [-Q <n>] <host>              |
 |  -W, --timeout=<t>  Time to wait for a response, in seconds             |
 |  -i, --interval=<t>  Wait interval seconds between sending each packet  |
 |  -s, --size=<n>  Specify the number of data bytes to be sent            |
 |  -c, --count=<n>  Stop after sending count packets                      |
 |  -Q, --tos=<n>  Set Type of Service related bits in IP datagrams        |
 |  -I, --interface=<n>  Set Interface number to send out the packet       |
 |   <host>  Host address or IP address                                    |
 |                                                                         |
 ===========================================================================
```

* Run `ping` command to test reachable of remote server.

```bash
esp8266> ping www.espressif.com
64 bytes from 119.9.92.99 icmp_seq=1 ttl=51 time=36 ms
64 bytes from 119.9.92.99 icmp_seq=2 ttl=51 time=34 ms
64 bytes from 119.9.92.99 icmp_seq=3 ttl=51 time=37 ms
64 bytes from 119.9.92.99 icmp_seq=4 ttl=51 time=36 ms
64 bytes from 119.9.92.99 icmp_seq=5 ttl=51 time=33 ms

--- 119.9.92.99 ping statistics ---
5 packets transmitted, 5 received, 0% packet loss, time 176ms
```

* Run `ping` with a wrong domain name
```bash
esp8266> ping www.hello-world.io
ping: unknown host www.hello-world.io
Command returned non-zero error code: 0x1 (ERROR)
```

* Run `ping` with an unreachable server
```bash
esp8266> ping www.zoom.us
From 69.171.230.18 icmp_seq=1 timeout
From 69.171.230.18 icmp_seq=2 timeout
From 69.171.230.18 icmp_seq=3 timeout
From 69.171.230.18 icmp_seq=4 timeout
From 69.171.230.18 icmp_seq=5 timeout

--- 69.171.230.18 ping statistics ---
5 packets transmitted, 0 received, 100% packet loss, time 4996ms
```
* Run `ping` with a IPv6 address
```bash
esp8266> ping fe80::a33a:d586:a603:5bb2 -I 2
64 bytes from FE80::A33A:D586:A603:5BB2 icmp_seq=1 ttl=0 time=7 ms
esp8266> 64 bytes from FE80::A33A:D586:A603:5BB2 icmp_seq=2 ttl=0 time=19 ms
64 bytes from FE80::A33A:D586:A603:5BB2 icmp_seq=3 ttl=0 time=3 ms
64 bytes from FE80::A33A:D586:A603:5BB2 icmp_seq=4 ttl=0 time=7 ms
64 bytes from FE80::A33A:D586:A603:5BB2 icmp_seq=5 ttl=0 time=4 ms

--- FE80::A33A:D586:A603:5BB2 ping statistics ---
5 packets transmitted, 5 received, 0% packet loss, time 40ms
```
