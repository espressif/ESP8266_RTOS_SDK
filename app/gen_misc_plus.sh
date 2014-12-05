#!/bin/bash -x
make APP=$1
if [ $? == 0 ];then
rm ../bin/upgrade/user$1.bin

cd .output/eagle/debug/image/

xt-objcopy --only-section .text -O binary eagle.app.v6.out eagle.app.v6.text.bin
xt-objcopy --only-section .data -O binary eagle.app.v6.out eagle.app.v6.data.bin
xt-objcopy --only-section .rodata -O binary eagle.app.v6.out eagle.app.v6.rodata.bin
xt-objcopy --only-section .irom0.text -O binary eagle.app.v6.out eagle.app.v6.irom0text.bin

../../../../../tools/gen_appbin.py eagle.app.v6.out v6

../../../../../tools/gen_flashbin.py eagle.app.v6.flash.bin eagle.app.v6.irom0text.bin

cp eagle.app.flash.bin user$1.bin
cp user$1.bin ../../../../../bin/upgrade/

cd ../../../../../

else
echo "make error"
fi