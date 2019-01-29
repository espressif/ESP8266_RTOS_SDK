
# Important

If your development board is based on **ESP8285** or **ESP8266 + 1MB flash**, you should read this document carefully, especially the Chapter **"Principle"**.

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

## Step 3: Configurate application location:

Configurate application location at "mennuconfig" like following base on partition table file.

If you select 1MB flash, application location configuration menu is like following:

```
Partition Table  --->

    [*] Support to setup partition parameter of APP2
    (0x5000) App1 offset address 
    (0x7B000) App1 size by bytes 
    (0x85000) App2 offset address
    (0x7b000) App2 size by bytes
```

If you select 2MB flash and above size, application location configuration menu is like following:

```
Partition Table  --->

    (0x10000) APP1 partition offset
    (0xF0000) APP1 partition size(by bytes)
```

Note: The firmware location information must be same as partition table file. **make ota flash** will only download the app1 at **APP1 partition offset**.

# Workflow

The OTA_workflow.png diagram demonstrates the overall workflow:

![OTA Workflow diagram](OTA_workflow.png)

## Step 1: Connect to AP

Connect your host PC to the same AP that you will use for the ESP8266.

## Step 2: Run HTTP Server

Python has a built-in HTTP server that can be used for example purposes.

For our upgrade example OTA file, we're going to use the `get-started/project_template` example.

Open a new terminal to run the HTTP server, then run these commands to build the example and start the server, if your board's flash size is "1 MB", you should firstly configure flash size to be "1 MB"(default is "2 MB") at "menuconfig" and then build project:

Configure 1MB flash if it is needed:

```
Serial flasher config  --->
    Flash size (2 MB)  --->
        (X) 1 MB
```

Build project:

```
cd $IDF_PATH/examples/get-started/project_template
make ota
```

Start http server at the directory of "build":

```
cd build
python -m SimpleHTTPServer 8070
```

While the server is running, the contents of the build directory can be browsed at http://localhost:8070/

NB: On some systems, the command may be `python2 -m SimpleHTTPServer`.

If you have any firewall software running that will block incoming access to port 8070, configure it to allow access while running the example.

## Step 3: Build OTA Example

Change back to the OTA example directory, and type `make menuconfig` to configure the OTA example. Under the "Example Configuration" submenu, fill in the following details:

* WiFi SSID & Password
* IP address of your host PC as "HTTP Server"
* HTTP Port number (if using the Python HTTP server above, the default is correct)

If serving the "project_template" example, you can leave the default filename as-is.

Configure 1MB flash if need:

```
Serial flasher config  --->
    Flash size (2 MB)  --->
        (X) 1 MB
```

Configurate the application location information and it must be the same as the OTA example's information, you can refer to the **Step 3: Configurate application location** of **Custom partition configuration**.

Save your changes, and type `make` to build the example.

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

# Principle

Command "make ota" will generate 3 binaries: "xxx(project name).app1.bin", "xxx(project name).app2.bin" and "xxx(project name).ota.bin". You should only upload the "xxx(project name).ota.bin" to your OTA server and let the app download it as the example.

"xxx.app1.bin" is for downloading to OTA_0 partition, and "xxx.app2.bin" is for downloading to OTA_1 partition. If your board's flash size is larger than "1 MB" or you select "Copy OTA" function, then "xxx.app1.bin" = "xxx.app2.bin" = "xxx.ota.bin". Otherwise If your board's flash size is "1 MB" and you don't select "Copy OTA" function, "xxx.app1.bin" != "xxx.app2.bin" != "xxx.ota.bin", "xxx.ota.bin" = "xxx.app1.bin" + "xxx.app2.bin". So the flash size configuration is very important. Otherwise if and at the last The example will select the binary it needs and download it into flash.

Based on the above theory, we can see that for ESP8266 + 2MB flash(or larger size), app1 and app2 are the same, you can download it directly without any distinction. But for ESP8285 (ESP8266 + 1MB flash), the ota0 (app1) and ota1 (app2) are different, you need to distinguish which one should be downloaded, and to what location, during FOTA. Now, the way in the example code is to synthesize app1 and app2 into an "xxxx (project name).ota.bin". And only write the target app (app1 or app2) into the flash, according to the location of download, when FOTA; the other part will be discarded.

On the other hand, if you want to use ESP8285(ESP8266 + 1MB flash) and don't want to upload 2 binaries for OTA, you can enable the "Copy OTA" function in menuconfig.

```
Component config  --->
    ESP8266-specific  --->
        [*] (**Expected**)Boot copy app
```

After enabling "Copy OTA" mode, the system will always download the app bin into ota_1 partition and then re-boot. After reboot, the bootloader will unpack the app bin and copy it to the ota_0 partition, then run the application in ota_0 partition.

# Troubleshooting

* Check whether your PC can ping the ESP8266 at its IP, and make sure that the IP, AP and other configuration settings are correct in menuconfig.
* Check if there is any firewall software on the PC that prevents incoming connections.
* Check whether you can see the configured file (default project_template.ota.bin) when browsing the file listing at http://127.0.0.1/
* If you have another PC or a phone, try viewing the file listing from the separate host.

## Error "ota_begin error err=0x104"

If you see this error then check that the configured (and actual) flash size is large enough for the partitions in the partition table. The default "two OTA slots" partition table only works with 4MB flash size. To use OTA with smaller flash sizes, create a custom partition table CSV (look in components/partition_table) and configure it in menuconfig.

If changing partition layout, it is usually wise to run "make erase_flash" between steps.

## Production Implementation

If scaling this example for production use, please consider:

* Using an encrypted communications channel such as HTTPS.
* Dealing with timeouts or WiFi disconnections while flashing.
