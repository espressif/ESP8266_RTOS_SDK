# ESP8266 MQTT Client Demo

## 1. Introduction

This MQTT demo is based on the Eclipse Paho MQTT library, and demonstrates a working MQTT client actions(subscribe, publish, ping). Using this MQTT demo, you can connect to the MQTT broker, subscribe to a topic and publish messages to the predefined topic.

Also, this demo will ping the MQTT broker in the defined interval if no sending or receiving action happens. And we add some APIs to realize the SSL functionality, these SSL APIs provide the one-way certification and two-way certification.

## 2. Configuration

Some basic configurations need to be done before starting this demo and are listed in the include/user_config.h.

* Wi-Fi SSID & Password
* MQTT Broker Address(can be a domain name) & MQTT Port
>Note: There is a publically accessible sandbox server for the Eclipse IoT projects available at iot.eclipse.org, please get some reference information from the website: https://iot.eclipse.org/getting-started

## 3. Description

### 3.1 MQTT-Normal

This section describes the mqtt informations and API for MQTT client without SSL functionality.

#### 3.1.1 MQTT Info

For this MQTT demo, mqtt-related informations are defined in the mqtt_client_thread(), and they are listed below.

* two buffers(i.e. sendbuf[80] & readbuf[80]) to store packets to be sent and received
* MQTTVersion, ClientID, KeepAliveInterval, etc are defined using **MQTTPacket_connectData_initializer**
* Command_timeout is defined as 30s, and you can use this value as default
* The subscribe topic is defined as "ESP8266/sample/sub"
* The subscribe message handler is "void messageArrived(MessageData* data)"
* The publish topic is defined as "ESP8266/sample/pub"
* The published message's QoS type is QoS2

These informarions are only defined as a demonstration, you can change them appropriately according to your own requirements.

#### 3.1.2 Major API

1.Platform-Related

* NetworkInit(): used to initialize **Network** structure, which includes read/write functions, etc.
* NetworkConnect(): used to create socket and connect to the MQTT broker

2.MQTT-Related

* MQTTClientInit(): used to initialize **MQTTClient** structure, which includes MQTT client information
* MQTTStartTask(): a task used to perform MQTT **keep alive**
* MQTTConnect(): used to perform MQTT connect
* MQTTSubscribe(): used to subscribe to a topic
* MQTTPublish(): used to publish messages to a topic

### 3.2 MQTT-SSL

This section describes the mqtt informations and API for MQTT client with SSL functionality enabled.

#### 3.2.1 MQTT Info

The aforementioned informations in the **MQTT Info** section of **MQTT-Normal** are also used for MQTT-SSL. As for SSL functionality, some more information will be needed and are listed below in the "Added-Info" section.

1.Existed-Info

This section is the same with the **MQTT Info** section of **MQTT-Normal**.

2.Added-Info

* May need header files of CA (and client certificate & key) included in the include/ directory
* May need length of the CA (and client certificate & key) files
* Need a **ssl_ca_crt_key_t** structure initialized using the CA (and client certificate & key) files

#### 3.2.2 Major API

When SSL is enabled, the Platform-related API are different with **MQTT-Normal** section.

1.Platform-related

* NetworkInitSSL(): used to initialize **Network** structure, which includes SSL read/write functions, etc.
* NetworkConnectSSL(): used to create socket and connect to the MQTT broker with SSL enabled

2.MQTT-Related

This section is the same with the "MQTT-Related" section of "MQTT-Normal".

#### 3.2.3 SSL Special

For SSL functionality, three certification ways may be used: no certification, one-way certification and two-way certification. The specific configurations for each of them are described below:

1.No Certification

* No CA file and client certificate & key files need to be included
* Define a **ssl_ca_crt_key_t** structure
* Set the **cacrt**, **cert** and **key** parameters within the structure to be **NULL**
* Recommend to set the **verify_mode** parameter to **SSL_VERIFY_NONE**
* Set the **method** parameter to **TLSv1_1_client_method()** or **TLSv1_2_client_method()**
* Set the **frag_len** parameter with a value between **2048** and **8192**

2.One-way Certification

* CA file shall be included, also length of the CA file shall be provided
* Define a **ssl_ca_crt_key_t** structure
* Set the **cacrt** parameter within the structure to the array in the CA file
* Set the **cacrt_len** parameter to length of the CA file
* Set the **verify_mode** parameter to **SSL_VERIFY_PEER**
* Set the **method** parameter to **TLSv1_1_client_method()** or **TLSv1_2_client_method()**
* Set the **frag_len** parameter with a value between **2048** and **8192**

3.Two-way Certification

* CA file and client certificate & key files shall be included
* Also length of the CA file and client certificate & key files shall be provided
* Define a **ssl_ca_crt_key_t** structure
* Set the **cacrt** parameter within the structure to the array in the CA file
* Set the **cacrt_len** parameter to length of the CA file
* Set the **cert** parameter within the structure to the array in the client certificate file
* Set the **cert_len** parameter to length of the client certificate file
* Set the **key** parameter within the structure to the array in the client key file
* Set the **key_len** parameter to length of the client key file
* Set the **verify_mode** parameter to **SSL_VERIFY_PEER**
* Set the **method** parameter to **TLSv1_1_client_method()** or **TLSv1_2_client_method()**
* Set the **frag_len** parameter with a value between **2048** and **8192**

>Note: two-way certification is decided by the SSL Server side, so on the client side we just provide all the files needed by the two-way certification.

#### 3.2.4 SSL Demo

The following shows a simple demo of the MQTT client SSL functionality, and only the different places compared with MQTT-Normal demo are displayed. The names of CA file, client certificate & key files are just a demonstration, changing these properly according to your own files.

```c
#include "openssl/ssl.h"
#include "CA.h"
#include "cert.h"
#include "key.h"

ssl_ca_crt_key_t ssl_cck;

#define SSL_CA_CERT_KEY_INIT(s,a,b,c,d,e,f)  ((ssl_ca_crt_key_t *)s)->cacrt = a;\
                                             ((ssl_ca_crt_key_t *)s)->cacrt_len = b;\
                                             ((ssl_ca_crt_key_t *)s)->cert = c;\
                                             ((ssl_ca_crt_key_t *)s)->cert_len = d;\
                                             ((ssl_ca_crt_key_t *)s)->key = e;\
                                             ((ssl_ca_crt_key_t *)s)->key_len = f;

static void mqtt_client_thread(void *pvParameters)
{
    ......
    NetworkInitSSL(&network);
    ......
    SSL_CA_CERT_KEY_INIT(&ssl_cck, ca_crt, ca_crt_len, client_crt, client_crt_len, client_key, client_key_len);

    if ((rc = NetworkConnectSSL(&network, address, MQTT_PORT, &ssl_cck, TLSv1_1_client_method(), SSL_VERIFY_NONE, 8192)) != 1) {
       printf("Return code from network connect ssl is %d\n", rc);
    }
    ......
}
```

## 4. Compiling & Execution

Once all the aforementioned works are done, we can compile and download the MQTT client (SSL) demo, and a few more steps will be needed.

* Export SDK_PATH & BIN_PATH, and run gen_misc.sh to compile and generate binary files
* Download the binary files to flash and run, also you can use UART console to watch the output log

All these being done, the MQTT client demo will:

* Connect to the MQTT Broker
* Subscribe to the topic "ESP8266/sample/sub"
* Publish messages to the topic "ESP8266/sample/pub" every 1 seconds
* MQTT keep alive interval is 60s, so if no sending and receiving actions happended during this interval, ping request will be sent and ping response is expected to be received.