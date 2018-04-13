1. Run ./gencrt.sh or if you have your own certificate, move to the openssl_server directory, the name is ca.crt,server.crt, server.key, client.crt and client.key.

    The server.crt and client.crt was generate by the same ca.crt in ./gencrt.sh.

    Server side needs ca.crt(to verify client.crt), server.crt, server.key

    Client side needs ca.crt(to verify server.crt), client.crt, client.key

    If you have two ca.crt to generate server.crt and client.crt respectively, client1.crt is generate by ca1.crt and client1.key, server2.crt is generate by ca2.crt and server2.key:

    Client side needs ca2.crt, client1.crt, client1.key.

    Server side needs ca1.crt, server2.crt, server2.key.

    Rename ca1.crt server2.crt server2.key to ca.crt server.crt server.key and run ./genheader.sh.

    Use ca2.crt in openssl s_client -CAfile option.

2. Run ./genheader.sh.

3. Modify thease two lines in file user_config.h to your local Wi-Fi SSID and Password.

    ```#define SSID "HUAWEI001"```

    ```#define PASSWORD ""```

4. Make sure that the computer and ESP8266 are in the same local area network.

5. Run ./gen_misc.sh.

6. Download bin file to ESP8266.

    Find server ip address in ESP8266 UART log: ip:192.168.3.6,mask:255.255.255.0,gw:192.168.3.1.

7. Run openssl s_client -CAfile ca.crt -cert client.crt -key client.key -verify 1 -tls1_1 -host 192.168.3.6 -port 443.


**ATTENTION**

**1. Make sure the free heap size larger than 30K.**

**2. Make sure the private key length larger than 2048.**

**3. Make sure the fragment size range is between 2048 and 8192.**
