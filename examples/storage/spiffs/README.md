# SPIFFS example

This example demonstrates how to use SPIFFS at this software platform. Example does the following steps:

1. Enable SPIFFS at "menuconfig"
2. Use an "all-in-one" `esp_vfs_spiffs_register` function to:
    - initialize SPIFFS,
    - mount SPIFFS filesystem using SPIFFS library (and format, if the filesystem can not be mounted),
    - register SPIFFS filesystem in VFS, enabling C standard library and POSIX functions to be used.
3. Create a file using `fopen` and write to it using `fprintf`.
4. Rename the file. Before renaming, check if destination file already exists using `stat` function, and remove it using `unlink` function.
5. Open renamed file for reading, read back the line, and print it to the terminal.

SPIFFS partition size is set in partitions_example.csv file. See `docs/en/api-guides/partition-tables.rst` documentation for more information.

## How to use example

### Hardware required

This example does not require any special hardware, and can be run on any common development board.

### Configure the project

If using Make based build system, run `make menuconfig` and set serial port under Serial Flasher Options.

If using CMake based build system, no configuration is required.

### Build and flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
make -j4 flash monitor
```

Or, for CMake based build system (replace PORT with serial port name):

```
idf.py -p PORT flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

## Example output

Here is an example console output. In this case `format_if_mount_failed` parameter was set to `true` in the source code. SPIFFS was unformatted, so the initial mount has failed. SPIFFS was then formatted, and mounted again.

```
I (324) example: Initializing SPIFFS
W (324) SPIFFS: mount failed, -10025. formatting...
I (19414) example: Partition size: total: 896321, used: 0
I (19414) example: Opening file
I (19504) example: File written
I (19544) example: Renaming file
I (19584) example: Reading file
I (19584) example: Read from file: 'Hello World!'
I (19584) example: SPIFFS unmounted
```

To erase the contents of SPIFFS partition, run `make erase_flash` command (or `idf.py erase_flash`, if using CMake build system). Then upload the example again as described above.
