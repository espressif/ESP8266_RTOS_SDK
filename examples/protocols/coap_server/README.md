
# CoAP server example

(See the README.md file in the upper level 'examples' directory for more information about examples.)  
This CoAP server example is adaptation of one of the [libcoap](https://github.com/obgm/libcoap) example.

CoAP server example would startup a daemon task, receive data from CoAP client and transmit data to CoAP client.

The Constrained Application Protocol (CoAP) is a specialized web transfer protocol for use with constrained nodes and constrained networks in the Internet of Things.   
The protocol is designed for machine-to-machine (M2M) applications such as smart energy and building automation.

please refer to [RFC7252](https://www.rfc-editor.org/rfc/pdfrfc/rfc7252.txt.pdf) for more details.

## How to use example

### Configure the project

```
idf.py menuconfig
```

* Set default serial port under Serial Flasher config
* Set WiFi SSID under Example Configuration
* Set WiFi Password under Example Configuration
* Set IPv4 multicast ip address for server to listen to
* Set IPv6 multicast ip address for server to listen to

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

## Example Output
current CoAP server would startup a daemon task,   
and the log is such as the following:  

```
...
I (1828) wifi:state: 0 -> 2 (b0)
I (1838) wifi:state: 2 -> 3 (0)
I (1849) wifi:state: 3 -> 5 (10)
I (1868) wifi:connected with HUAWEI_888, aid = 1, channel 1, HT20, bssid = 34:29:12:43:c5:40
I (3829) tcpip_adapter: sta ip: 192.168.3.2, mask: 255.255.255.0, gw: 192.168.3.1
I (3834) example_connect: Connected to HUAWEI_888
I (3837) example_connect: IPv4 address: 192.168.3.2
I (3846) example_connect: IPv6 address: fe80:0000:0000:0000:a6cf:12ff:fee8:733b
I (3864) CoAP_server: Configured IPV4 Multicast address 232.10.11.12
I (3875) CoAP_server: Configured IPV6 Multicast address ff02::fc
...
```

if a CoAP client query `/Espressif` resource, CoAP server would return `"no data"`  
until a CoAP client does a PUT with some data.

## libcoap Documentation
This can be found at https://libcoap.net/doc/reference/4.2.0/

## Troubleshooting
* Please make sure CoAP client fetchs or puts data under path: `/Espressif` or
fetches `/.well-known/core`

* libcoap logging can be increased by changing `#define COAP_LOGGING_LEVEL 0`
to `#define COAP_LOGGING_LEVEL 9`
