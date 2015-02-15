#!/bin/bash -x
make APP=$1
if [ $? == 0 ];then
rm ../bin/upgrade/user$1.bin

cd .output/eagle/debug/image/

if xtensa-lx106-elf-objcopy --version 2>/dev/null ; then
    CROSS_COMPILE=xtensa-lx106-elf-
fi
export CROSS_COMPILE
CROSS_COMPILE=${CROSS_COMPILE:-xt-}

${CROSS_COMPILE}objcopy --only-section .text -O binary eagle.app.v6.out eagle.app.v6.text.bin
${CROSS_COMPILE}objcopy --only-section .data -O binary eagle.app.v6.out eagle.app.v6.data.bin
${CROSS_COMPILE}objcopy --only-section .rodata -O binary eagle.app.v6.out eagle.app.v6.rodata.bin
${CROSS_COMPILE}objcopy --only-section .irom0.text -O binary eagle.app.v6.out eagle.app.v6.irom0text.bin

../../../../../tools/gen_appbin.py eagle.app.v6.out v6

../../../../../tools/gen_flashbin.py eagle.app.v6.flash.bin eagle.app.v6.irom0text.bin

cp eagle.app.flash.bin user$1.bin
cp user$1.bin ../../../../../bin/upgrade/

cd ../../../../../

else
echo "make error"
fi
