# WPA2 Enterprise Example

This example shows how ESP8266 connects to AP with wpa2 enterprise encryption. Example does the following steps:

1. Install CA certificate which is optional.
2. Install client certificate and client key which is required in TLS method and optional in PEAP and TTLS methods.
3. Set identity of phase 1 which is optional.
4. Set user name and password of phase 2 which is required in PEAP and TTLS methods.
5. Enable wpa2 enterprise.
6. Connect to AP.

*Note:* 1. certificate currently is generated when compiling the example and then stored in flash.
        2. The expiration date of the certificates is 2027/06/05.

## The file wpa2_ca.pem, wpa2_ca.key, wpa2_server.pem, wpa2_server.crt and wpa2_server.key can be used to configure AP with
   wpa2 enterprise encryption. The steps how to generate new certificates and keys using openssl is as follows:
   
1. wpa2_ca.pem wpa2_ca.key:
    openssl req -new -x509 -keyout wpa2_ca.key -out wpa2_ca.pem
2. wpa2_server.key:
    openssl req -new -key wpa2_server.key -out wpa2_server.csr
3. wpa2_csr:
    openssl req -new -key server.key -out server.csr
4. wpa2_server.crt:
    openssl ca -batch -keyfile wpa2_ca.key -cert wpa2_ca.pem -in wpa2_server.csr -key ca1234 -out wpa2_server.crt -extensions xpserver_ext -extfile xpextensions
5. wpa2_server.p12:
    openssl pkcs12 -export -in wpa2_server.crt -inkey wpa2_server.key -out wpa2_server.p12 -passin pass:sv1234 -passout pass:sv1234
6. wpa2_server.pem:
    openssl pkcs12 -in wpa2_server.p12 -out wpa2_server.pem -passin pass:sv1234 -passout pass:sv1234
7. wpa2_client.key:
    openssl genrsa -out wpa2_client.key 1024
8. wpa2_client.csr:
    openssl req -new -key wpa2_client.key -out wpa2_client.csr
9. wpa2_client.crt:
    openssl ca -batch -keyfile wpa2_ca.key -cert wpa2_ca.pem -in wpa2_client.csr -key ca1234 -out wpa2_client.crt -extensions xpclient_ext -extfile xpextensions
10. wpa2_client.p12:
    openssl pkcs12 -export -in wpa2_client.crt -inkey wpa2_client.key -out wpa2_client.p12
11. wpa2_client.pem:
    openssl pkcs12 -in wpa2_client.p12 -out wpa2_client.pem

### Example output

Here is an example of wpa2 enterprise(PEAP method) console output.

I (393) reset_reason: RTC reset 1 wakeup 0 store 0, reason is 1
I (413) example: Setting WiFi configuration SSID wpa2_test...
I (4981) wifi: state: 0 -> 2 (b0)
I (4986) wifi: state: 2 -> 3 (0)
I (4992) wifi: state: 3 -> 5 (10)
I (11005) example: ~~~~~~~~~~~
I (11007) example: IP:0.0.0.0
I (11010) example: MASK:0.0.0.0
I (11012) example: GW:0.0.0.0
I (11015) example: ~~~~~~~~~~~
I (11019) example: free heap:72588

I (13026) example: ~~~~~~~~~~~
I (13032) example: IP:192.168.2.121
I (13034) example: MASK:255.255.255.0
I (13037) example: GW:192.168.2.1
I (13040) example: ~~~~~~~~~~~
I (13045) example: free heap:72028

