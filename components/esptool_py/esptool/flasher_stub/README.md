This is the source of the software flasher stub.

esptool.py loads the flasher stub into memory and executes it to:

* Add features that the ESP8266 & ESP32 bootloader ROMs do not have.

* Add features to the ESP8266 bootloader ROM which are only in the ESP32 ROM.

* Improve flashing performance over the ROM bootloaders.

* Work around bugs in the ESP8266 ROM bootloader.

Thanks to [Cesanta](http://cesanta.com/) who provided the original ESP8266 stub loader upon which this loader is based.

# To Use

The stub loader is already automatically integrated into esptool.py. You don't need to do anything special to use it.

# To Build

If you want to build the stub to test modifications or updates, here's how:

* You will need both an ESP8266 gcc toolchain (xtensa-lx106-elf-) and an ESP32 toolchain (xtensa-esp32-elf-) on your PATH.

* Set the environment variables SDK_PATH to the path to an ESP8266 IoT NON-OS SDK directory (last stub was built with SDK v1.5.1).

* Set the environment variable IDF_PATH to the path to an ESP-IDF directory.

* Set any other environment variables you'd like to override in the Makefile.

* To build type `make`

# To Test

To test the build stub, you can either copy-paste the code from `build/stub_flasher_snippet.py` into esptool.py manually. Or there are some convenience wrappers to make testing quicker to iterate on:

* Running `esptool_test_stub.py` is the same as running `esptool.py`, only it uses the just-compiled stubs from the build directory.

* Running `run_tests_with_stub.py` is the same as running `test/test_esptool.py`, only it uses the just-compiled stubs from the build directory.
