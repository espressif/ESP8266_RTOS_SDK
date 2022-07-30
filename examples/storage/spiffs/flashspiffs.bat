rem mkspiffs -c data -b 4096 -p 256 -s 458752 spiffs.bin
python spiffsgen.py 0x70000 data spiffs.bin
python %IDF_PATH%/components/esptool_py/esptool/esptool.py --chip esp8266 --port COM5 --baud 115200 --before default_reset --after hard_reset write_flash -z 0x90000 spiffs.bin