
# Important

The example only services the ESP8285 or modules integrated the flash which size is `1MB`.

---

# Simple OTA Demo

This example demonstrates a working OTA (over the air) firmware update workflow.

This example is a *simplified demonstration*, for production firmware updates you should use a secure protocol such as HTTPS.

---

# Aim

An app running on ESP8266 can upgrade itself by downloading a new app "image" binary file, and storing it in flash.

In this example, the ESP8266 has 2 images in flash: OTA_0, OTA_1. Each of these is a self-contained partition. The number of OTA image partition is determined by the partition table layout.

Flash the example through serial port with command "make flash" to update the OTA_0 app image. In first boot, the bootloader loads this OTA_0 app image which then will execute an OTA update (triggered in the example code). The OTA update will download a new image from an http server and save it into the OTA_1 partition. After that, the example code will update the ota_data partition to indicate the new app partition, and then reboot, which leads to the second boot. During the second boot, the bootloader will read the ota_data, and select to run the new OTA image.

# Custom partition configuration

If customers want to use their own partition tables with specific partition location. Please see following steps:

## Step 1: Create partition file

Create a partition managment file with "cvs" formate, please refer to "doc/en/api-guides/partition-tables.rst"

## Step 2: Select custom partition mode

1. Select custom partition tables at "menuconfig":

```
Partition Table  --->
    Partition Table (XXXXXX)  --->
        (X) Custom partition table CSV
```

2. Configurate custom partition location at:

```
(XXXXXX)Custom partition CSV file
```

Note: System will add the absolute path of the project to the head of the "Custom partition CSV file" automatically when compling.

3. Configurate patition table location if necessary:

```
(XXXXXX)Partition table offset address at flash
```

**make ota flash** will only download the app1 at **OTA_0 partition offset**.

# Workflow

The OTA_workflow.png diagram demonstrates the overall workflow:

![OTA Workflow diagram](../OTA_workflow.png)

## Step 1: Connect to AP

Connect your host PC to the same AP that you will use for the ESP8266.

## Step 2: Build OTA Example

To make the OTA example easy to understand and use, here the example will show `self-update-self`. 

Jump into the OTA example directory, and type `make menuconfig` to configure the OTA example. Under the "Example Configuration" submenu, fill in the following details:

* WiFi SSID & Password
* IP address of your host PC as "HTTP Server"
* HTTP Port number (if using the Python HTTP server above, the default is correct)

If serving the "OTA" example, you can leave the default filename as-is.

Enable the firmware compatibility upgrade function:

```
    Component config  --->
        ESP8266-specific  --->
            [*] ESP8266 update from old SDK by OTA
```

If you old SDK storing RF parameters is customized and want the new firmware to use it, please enable RF parameters reuse:

```
    Component config  --->
        ESP8266-specific  --->
            [*]   Load old RF Parameters
```

Configurate the flash size:

```
    Serial flasher config  --->
        Flash size (2 MB)  --->
            (X) 1 MB
```

If you want to connect to the original AP of old SDK, then configurate as following:

```
    Example Configuration  --->
        [*] Connect to the original AP 
```

Save your changes, and type `make ota` to build the example to generate the real OTA binary firmware.

The name of final generated binary firmware is `ota.v2_to_v3.ota.bin` and the binary firmware locates at directory of `build`.
And only this one can be used to be updated by this example.

You can read the document `docs/en/api-guilds/fota-from-old-new.rst` to know more about the compatibility upgrade function.

## Step 2: Run HTTP Server

Python has a built-in HTTP server that can be used for example purposes.

Open a new terminal to run the HTTP server, then run these commands to build the example and start the server, then build project:

Start http server at the directory of `build`:

```
cd build
python -m SimpleHTTPServer 8070
```

The `8070` is the http server's TCP port which is configurated at menu.

While the server is running, the contents of the build directory can be browsed at http://localhost:8070/

NB: On some systems, the command may be `python2 -m SimpleHTTPServer`.

If you have any firewall software running that will block incoming access to port 8070, configure it to allow access while running the example.

## Step 4: Flash OTA Example

When flashing, use the `erase_flash` target first to erase the entire flash (this deletes any leftover data in the ota_data partition). Then flash the factory image over serial:

```
make erase_flash flash
```

(The `make erase_flash flash` means "erase everything, then flash". `make flash` only erases the parts of flash which are being rewritten.)

## Step 5: Run the OTA Example

When the example starts up, it will print "ota: Starting OTA example..." then:

1. Connect to the AP with configured SSID and password.
2. Connect to the HTTP server and download the new image.
3. Write the image to flash, and configure the next boot from this image.
4. Reboot

# Troubleshooting

* Check whether your PC can ping the ESP8266 at its IP, and make sure that the IP, AP and other configuration settings are correct in menuconfig.
* Check if there is any firewall software on the PC that prevents incoming connections.
* Check whether you can see the configured file (default hello_world.ota.bin) when browsing the file listing at http://127.0.0.1/
* If you have another PC or a phone, try viewing the file listing from the separate host.

## Error "ota_begin error err=0x104"

If you see this error then check that the configured (and actual) flash size is large enough for the partitions in the partition table. The default "two OTA slots" partition table only works with 4MB flash size. To use OTA with smaller flash sizes, create a custom partition table CSV (look in components/partition_table) and configure it in menuconfig.

If changing partition layout, it is usually wise to run "make erase_flash" between steps.

## Production Implementation

If scaling this example for production use, please consider:

* Using an encrypted communications channel such as HTTPS.
* Dealing with timeouts or WiFi disconnections while flashing.
