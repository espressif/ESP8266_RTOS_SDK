#!/usr/bin/env bash

apt-get update
apt-get install -y git autoconf build-essential gperf bison flex texinfo libtool libncurses5-dev wget gawk libc6-dev python-serial libexpat-dev unzip
apt-get install linux-headers-$( uname -r )
apt-get install -y virtualbox-fuse

mkdir -p /opt/Espressif
chown vagrant:vagrant -R /opt/Espressif


su vagrant <<'EOF'
cd /opt/Espressif

git clone --depth 1 -b lx106 git://github.com/jcmvbkbc/crosstool-NG.git 
cd crosstool-NG
./bootstrap && ./configure --prefix=`pwd` && make && make install
./ct-ng xtensa-lx106-elf
./ct-ng build

echo "export PATH=$PWD/builds/xtensa-lx106-elf/bin:\$PATH" | sudo tee -a /root/.bashrc
echo "export PATH=$PWD/builds/xtensa-lx106-elf/bin:\$PATH" >> ~/.bashrc

cd /opt/Espressif
wget -O esp_iot_sdk_v0.9.3_14_11_21.zip https://github.com/esp8266/esp8266-wiki/raw/master/sdk/esp_iot_sdk_v0.9.3_14_11_21.zip
wget -O esp_iot_sdk_v0.9.3_14_11_21_patch1.zip https://github.com/esp8266/esp8266-wiki/raw/master/sdk/esp_iot_sdk_v0.9.3_14_11_21_patch1.zip
unzip esp_iot_sdk_v0.9.3_14_11_21.zip
unzip -o esp_iot_sdk_v0.9.3_14_11_21_patch1.zip
mv esp_iot_sdk_v0.9.3 ESP8266_SDK
mv License ESP8266_SDK/

cd /opt/Espressif/ESP8266_SDK
sed -i -e 's/xt-ar/xtensa-lx106-elf-ar/' -e 's/xt-xcc/xtensa-lx106-elf-gcc/' -e 's/xt-objcopy/xtensa-lx106-elf-objcopy/' Makefile
mv examples/IoT_Demo .

cd /opt/Espressif/ESP8266_SDK
wget -O lib/libc.a https://github.com/esp8266/esp8266-wiki/raw/master/libs/libc.a
wget -O lib/libhal.a https://github.com/esp8266/esp8266-wiki/raw/master/libs/libhal.a
wget -O include.tgz https://github.com/esp8266/esp8266-wiki/raw/master/include.tgz
tar -xvzf include.tgz

cd /opt/Espressif
wget -O esptool_0.0.2-1_i386.deb https://github.com/esp8266/esp8266-wiki/raw/master/deb/esptool_0.0.2-1_i386.deb
sudo dpkg -i esptool_0.0.2-1_i386.deb

cd /opt/Espressif
git clone https://github.com/themadinventor/esptool esptool-py
sudo ln -s $PWD/esptool-py/esptool.py crosstool-NG/builds/xtensa-lx106-elf/bin/

cd /opt/Espressif
git clone https://github.com/esp8266/source-code-examples.git
cd source-code-examples/blinky

make && echo "Ready to code!"
EOF
