WOLFSSL INTRODUCTION

### WHAT ABOUT WOLFSSL

The WOLFSSL embedded SSL library (formerly CyaSSL) is a lightweight SSL/TLS library written in ANSI C and targeted for embedded, RTOS, and resource-constrained environments – primarily because of its small size, speed, and feature set.  It is commonly used in standard operating environments as well because of its royalty-free pricing and excellent cross platform support.  wolfSSL supports industry standards up to the current TLS 1.2 and DTLS 1.2 levels, is up to 20 times smaller than OpenSSL, and offers progressive ciphers such as ChaCha20, Curve25519, NTRU, and Blake2b. User benchmarking and feedback reports dramatically better performance when using wolfSSL over OpenSSL.

### BEFORE YOU GET STARTED
- Requirements
    - RTOS SDK
- Optional
    - Basic knowledge of server/client communication
    - Basic knowledge of SSL/TLS

The more you know, the easier it will be to get going. There are a number of links in the Helpful Links section to read up on SSL/TLS.

### QUICK START

- Example
  - This project demonstrates a https client which connects to "www.howsmyssl.com" by default, you can connect to other https servers by modify WEB_SERVER and WEB_PORT.
- Compile
    - Clone ESP8266_RTOS_SDK, e.g., to ~/ESP8266_RTOS_SDK.
      - $ git clone https://github.com/espressif/ESP8266_RTOS_SDK.git
    - Add IDF_PATH:
          - $export IDF_PATH=~/ESP8266_RTOS_SDK
      - run `make menuconfig`
          - Modify SSID and PASSWORD under menu "Example Configuration".
      - run `make`
- Download:
    - run `make flash monitor`

### HELPFUL LINKS

In general, these are links which will be useful for using both wolfSSL, as well as networked and secure applications in general. Furthermore, there is a more comprehensive tutorial that can be found in Chapter 11 of the official wolfSSL manual. The examples in the wolfSSL package and Chapter 11 do appropriate error checking, which is worth taking a look at. For a more comprehensive API, check out chapter 17 of the official manual.

- WOLFSSL Manual (https://www.wolfssl.com/docs/wolfssl-manual/)
- WOLFSSL GitHub
  (https://github.com/wolfssl/wolfssl)
