WOLFSSL INTRODUCTION

Licensed from wolfSSL to Espressif.

### WHAT ABOUT WOLFSSL

The WOLFSSL embedded SSL library (formerly CyaSSL) is a lightweight SSL/TLS library written in ANSI C and targeted for embedded, RTOS, and resource-constrained environments – primarily because of its small size, speed, and feature set.  It is commonly used in standard operating environments as well because of its royalty-free pricing and excellent cross platform support.  wolfSSL supports industry standards up to the current TLS 1.2 and DTLS 1.2 levels, is up to 20 times smaller than OpenSSL, and offers progressive ciphers such as ChaCha20, Curve25519, NTRU, and Blake2b. User benchmarking and feedback reports dramatically better performance when using wolfSSL over OpenSSL.

### BEFORE YOU GET STARTED
- Requirements
    - RTOS SDK
    - You can use both xcc and gcc to compile your project, gcc is recommended. 
- Optional
    - Basic knowledge of server/client communication
    - Basic knowledge of SSL/TLS

The more you know, the easier it will be to get going. There are a number of links in the Helpful Links section to read up on SSL/TLS.

### QUICK START

if you plan to use TLS cipher suites you must setting the information in user_settings.h for compatible with your needs.

- Example
  - The project support an example which run a client, it connects with "www.baidu.com" default, you can modify it by WOLFSSL_DEMO_TARGET_NAME and WOLFSSL_DEMO_TARGET_PORT.
  - Modify SSID and PASSWORD according to the actual access point which in user_config.h
- Compile
    - Clone ESP8266_RTOS_SDK, e.g., to ~/ESP8266_RTOS_SDK.
      - $ git clone https://github.com/espressif/ESP8266_RTOS_SDK.git
    - Modify gen_misc.sh or gen_misc.bat:
      - For Linux: 
          - $export SDK_PATH=~/ESP8266_RTOS_SDK
          - $export BIN_PATH=~/ESP8266_BIN
      - For Windows:
          - set SDK_PATH=/c/ESP8266_RTOS_SDK
          - set BIN_PATH=/c/ESP8266_BIN
- Generate bin
  - For Linux:
      - ./gen_misc.sh
  - For Windows:
      - gen_misc.bat

  - Just follow the tips and steps.
      - STEP 1: 1
      - STEP 2: 1
      - STEP 3: default
      - STEP 4: default
      - STEP 5: 2

- Download
  - blank.bin, downloads to flash 0x01fe000
  - esp_init_data_default.bin, downloads to flash 0x01fc000
  - boot.bin, downloads to flash 0x00000
  - user1.1024.new2.bin, downloads to flash 0x01000

### PORT

If you want to use wolfssl in your project, follow up those steps.
- STEP 1: Copy `wolfssl` folder to your project
- STEP 2: Modify `Makefile` to make sure that wolfssl will be compiled and linked
- STEP 3: Modify `Makefile` to define `WOLFSSL_USER_SETTINGS`
- STEP 4: Copy `user_setting.h` to your project's include folder
    - since WOLFSSL_USER_SETTINGS is defined, it'll allow you to use the setting by yourself in user_setting.h.

### HELPFUL LINKS

In general, these are links which will be useful for using both wolfSSL, as well as networked and secure applications in general. Furthermore, there is a more comprehensive tutorial that can be found in Chapter 11 of the official wolfSSL manual. The examples in the wolfSSL package and Chapter 11 do appropriate error checking, which is worth taking a look at. For a more comprehensive API, check out chapter 17 of the official manual.

- WOLFSSL Manual (https://www.wolfssl.com/docs/wolfssl-manual/)
- WOLFSSL GitHub
  (https://github.com/wolfssl/wolfssl)
