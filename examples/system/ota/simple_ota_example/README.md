# OTA Demo

## Introduction

Over The Air (OTA) updates can be performed in esp8266 in two ways:

- Using native APIs which are part of OTA component.
- Using simplified APIs which are part of `esp_https_ota`. It is an abstraction layer over OTA APIs to perform updates using HTTPS.

Both these methods are demonstrated in OTA Demo under `native_ota_example` and `simple_ota_example` respectively.

---

## Aim

An app running on ESP8266 can upgrade itself by downloading a new app "image" binary file, and storing it in flash.

In this example, the ESP8266 has 3 images in flash: factory, OTA_0, OTA_1. Each of these is a self-contained partition. The number of OTA image partition is determined by the partition table layout.

Flashing the example over serial with "make flash" updates the factory app image. On first boot, the bootloader loads this factory app image which then performs an OTA update (triggered in the example code). The update downloads a new image from a HTTPS server and saves it into the OTA_0 partition. At this point the example code updates the ota_data partition to indicate the new app partition, and resets. The bootloader reads ota_data, determines the new OTA image has been selected, and runs it.


## Workflow

The OTA_workflow.png diagram demonstrates the overall workflow:

![OTA Workflow diagram](../OTA_workflow.png)

### Step 1: Connect to AP

Connect your host PC to the same AP that you will use for the ESP8266.

### Step 2: Generate the OTA Binary
For our upgrade example OTA file, we're going to use the `get-started/hello_world` example.

Build the example:

```
cd $IDF_PATH/examples/get-started/hello_world
make
cd build
```

Note: You've probably noticed there is nothing special about the "hello_world" example when used for OTA updates. This is because any .bin app file which is built by ESP8266_RTOS_SDK can be used as an app image for OTA. The only difference is whether it is written to a factory partition or an OTA partition.

### Step 3: Run HTTPS Server

Open a new terminal and run these commands to start the HTTPS server:

Generate self-signed certificate and key:

*NOTE: `Common Name` of server certificate should be host-name of your server.*

```
openssl req -x509 -newkey rsa:2048 -keyout ca_key.pem -out ca_cert.pem -days 365

```

Copy the certificate to `server_certs` directory inside OTA example directory:

```
cp ca_cert.pem /path/to/ota/example/server_certs/
```


Start the HTTPS server:

```
openssl s_server -WWW -key ca_key.pem -cert ca_cert.pem -port 8070
```

Copy the generated binary(hello_world.bin) into the folder in which the HTTPS server is running.  
If you have any firewall software running that will block incoming access to port 8070, configure it to allow access while running the example.

### Step 4: Build OTA Example

Change back to the OTA example directory, and type `make menuconfig` to configure the OTA example. Under the "Example Configuration" submenu, fill in the following details:

* WiFi SSID & Password
* Firmware Upgrade URL. The URL will be look like this:

```
https://<host-ip-address>:<host-port>/<firmware-image-filename>

for e.g,
https://192.168.0.3:8070/hello_world.bin
```

Save your changes, and type `make` to build the example.

### Step 5: Flash OTA Example

When flashing, use the `make flash` to flash the factory image. This command will find if partition table has ota_data partition (as in our case) then ota_data will erase to initial. 
It allows to run the newly loaded app from a factory partition.

```
make flash
```

### Step 6: Run the OTA Example

When the example starts up, it will print "Starting OTA example..." then:

1. Connect to the AP with configured SSID and password.
2. Connect to the HTTP server and download the new image.
3. Write the image to flash, and configure the next boot from this image.
4. Reboot

## Troubleshooting

### General connectivity issues

* Check your PC can ping the ESP8266 at its IP, and that the IP, AP and other configuration settings are correct in menuconfig.
* Check if any firewall software is preventing incoming connections on the PC.
* Check whether you can see the configured file (default hello_world.bin), by checking the output of following command:

 ```
 curl -v https://<host-ip-address>:<host-port>/<firmware-image-filename>
 ```

* If you have another PC or a phone, try viewing the file listing from the separate host.

### Error "ota_begin error err=0x104"

If you see this error then check that the configured (and actual) flash size is large enough for the partitions in the partition table. The default "two OTA slots" partition table only works with 4MB flash size. To use OTA with smaller flash sizes, create a custom partition table CSV (look in components/partition_table) and configure it in menuconfig.

If changing partition layout, it is usually wise to run "make erase_flash" between steps.

### Error "mbedtls error: 0x7200"
        
CONFIG_MBEDTLS_SSL_IN_CONTENT_LEN = 16384 is set in sdkconfig.defaults in this example and this value(16384) is required to comply fully with TLS standards.
        
You can set a lower value to save RAM if the other end of the connection supports Maximum Fragment Length Negotiation Extension (max_fragment_length, see RFC6066) or you know for certain that it will never send a message longer than a certain number of bytes.
If the value is set too low, symptoms are a failed TLS handshake or a return value of MBEDTLS_ERR_SSL_INVALID_RECORD (-0x7200).
